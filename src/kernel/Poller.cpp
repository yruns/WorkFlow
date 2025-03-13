//
// Created by yruns on 2025/3/12.
//

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <errno.h>
#include <limits.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "List.h"
#include "Poller.h"
#include "RBTree.h"

namespace
{
        int __poller_create_pfd()
        {
                // 内部逻辑
                return epoll_create(1);
        }

        int __poller_create_timerfd()
        {
                return timerfd_create(CLOCK_MONOTONIC, 0);
        }

        int __poller_set_timerfd(const int              timerfd,
                                 const struct timespec *abstime)
        {
                struct itimerspec timer;
                timer.it_interval = {};
                timer.it_value    = *abstime;
                return timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &timer,
                                       nullptr);
        }

        long __timeout_cmp(const struct PollerNode *node1,
                           const struct PollerNode *node2)
        {
                long ret = node1->timeout.tv_sec - node2->timeout.tv_sec;

                if (ret == 0)
                        ret = node1->timeout.tv_nsec - node2->timeout.tv_nsec;

                return ret;
        }

        int __poller_del_fd(const int fd, const int pfd)
        {
                return epoll_ctl(pfd, EPOLL_CTL_DEL, fd, nullptr);
        }

        int __poller_close_timerfd(const int fd) { return close(fd); }

        int __poller_close_pfd(const int fd) { return close(fd); }

        int __poller_add_timerfd(const int timerfd, const int pfd)
        {
                struct epoll_event ev;
                ev.events   = EPOLLIN | EPOLLET;
                ev.data.ptr = nullptr;
                ev.data.fd  = timerfd;
                return epoll_ctl(pfd, EPOLL_CTL_ADD, timerfd, &ev);
        }

        int __poller_create_timer(const int pfd)
        {
                const int timerfd = __poller_create_timerfd();

                if (timerfd >= 0)
                {
                        if (__poller_add_timerfd(timerfd, pfd) >= 0)
                        {
                                return timerfd;
                        }
                        __poller_close_timerfd(timerfd);
                }
                return -1;
        }

        int __poller_add_fd(const int fd, const int event, void *data,
                            const int pfd)
        {
                struct epoll_event ev;
                ev.events   = event;
                ev.data.ptr = data;
                return epoll_ctl(pfd, EPOLL_CTL_ADD, fd, &ev);
        }

        int __poller_open_pipe(Poller &poller)
        {
                int pipefd[2];

                if (pipe(pipefd) >= 0)
                {
                        if (__poller_add_fd(pipefd[0], EPOLLIN,
                                            reinterpret_cast<void *>(1),
                                            poller.pfd()) >= 0)
                        {
                                poller.setPipeRead(pipefd[0]);
                                poller.setPipeWrite(pipefd[1]);
                                return 0;
                        }

                        close(pipefd[0]);
                        close(pipefd[1]);
                }

                return -1;
        }

} // namespace


Poller::Poller(const struct PollerParams *params)
{
        m_pfd = __poller_create_pfd();
        if (m_pfd >= 0)
        {
                const int timerfd = __poller_create_timer(m_pfd);
                if (timerfd >= 0)
                {
                        m_timerfd = timerfd;
                        std::unique_lock lock(m_mutex);
                        m_maxOpenFiles = params->maxOpenFiles;
                        m_callback     = params->callback;
                        m_context      = params->content;

                        m_stopped = 1;
                }
                __poller_close_pfd(m_pfd);
        }
}

int Poller::start()
{
        std::unique_lock lock(m_mutex);
        if (__poller_open_pipe(*this) >= 0)
        {
                m_thread.reset(new std::thread(&Poller::threadRoutine, this));
                this->m_stopped = 0;
        }
        return -this->m_stopped;
}

void Poller::handleRead(struct PollerNode *node)
{
        ssize_t nLeft = 0;
        size_t  n;
        char   *p;

        while (1)
        {
                p = m_buf;
                if (!node->data.ssl)
                {
                        nLeft = read(node->data.fd, p, POLLER_BUFSIZE);
                        if (nLeft < 0)
                        {
                                if (errno == EAGAIN)
                                        return;
                        }
                } else
                {
                        // TODO: SSL_READ
                }

                if (nLeft <= 0)
                        break;

                do
                {
                        n = nLeft;
                        if (this->appendMessage(p, &n, node) >= 0)
                        {
                                nLeft -= n;
                                p += n;
                        } else
                                nLeft = -1;
                } while (nLeft > 0);

                if (nLeft < 0)
                        break;

                if (node->removed)
                        return;
        }

        if (this->removeNode(node))
                return;

        if (nLeft == 0)
        {
                node->error = 0;
                node->state = PR_ST_FINISHED;
        } else
        {
                node->error = errno;
                node->state = PR_ST_ERROR;
        }

        delete node->res;
        this->m_callback(reinterpret_cast<PollerResult *>(node),
                         this->m_context);
}

void Poller::handleWrite(struct PollerNode *node)
{
        struct iovec *iov   = node->data.writeIov;
        size_t        count = 0;
        ssize_t       nLeft = 0;
        int           iovcnt;
        int           ret = 0;

        while (node->data.iovcnt > 0)
        {
                if (!node->data.ssl)
                {
                        iovcnt = node->data.iovcnt;
                        if (iovcnt > IOV_MAX)
                                iovcnt = IOV_MAX;

                        nLeft = writev(node->data.fd, iov, iovcnt);
                        if (nLeft < 0)
                        {
                                ret = errno == EAGAIN ? 0 : -1;
                                break;
                        }
                } else if (iov->iov_len > 0)
                {
                        // TODO SSL Write
                } else
                        nLeft = 0;

                count += nLeft;
                do
                {
                        if (nLeft >= iov->iov_len)
                        {
                                nLeft -= iov->iov_len;
                                iov->iov_base =
                                        static_cast<char *>(iov->iov_base) +
                                        iov->iov_len;
                                iov->iov_len = 0;
                                iov++;
                                node->data.iovcnt--;
                        } else
                        {
                                iov->iov_base =
                                        static_cast<char *>(iov->iov_base) +
                                        nLeft;
                                iov->iov_len -= nLeft;
                                break;
                        }
                } while (node->data.iovcnt > 0);
        }

        node->data.writeIov = iov;
        if (node->data.iovcnt > 0 && ret >= 0)
        {
                if (count == 0)
                        return;

                if (node->data.partialWritten(count, node->data.context) >= 0)
                        return;
        }

        if (this->removeNode(node))
                return;

        if (node->data.iovcnt == 0)
        {
                node->error = 0;
                node->state = PR_ST_FINISHED;
        } else
        {
                node->error = errno;
                node->state = PR_ST_ERROR;
        }

        this->m_callback(castPollerNodeToResult(node), this->m_context);
}

void Poller::handleListen(struct PollerNode *node)
{
        struct PollerNode      *res = node->res;
        struct sockaddr_storage ss;
        struct sockaddr        *addr = reinterpret_cast<struct sockaddr *>(&ss);
        socklen_t               addrlen;
        void                   *result;
        int                     sockfd;

        while (1)
        {
                addrlen = sizeof(sockaddr_storage);
                sockfd  = accept(node->data.fd, addr, &addrlen);
                if (sockfd < 0)
                {
                        if (errno == EAGAIN || errno == EMFILE ||
                            errno == ENFILE)
                                return;
                        else if (errno == ECONNABORTED)
                                continue;
                        else
                                break;
                }

                result = node->data.accept(addr, addrlen, sockfd,
                                           node->data.context);
                if (!result)
                        break;

                res->data        = node->data;
                res->data.result = result;
                res->error       = 0;
                res->state       = PR_ST_SUCCESS;

                this->m_callback(castPollerNodeToResult(res), this->m_context);

                res       = new PollerNode{};
                node->res = res;
                if (!res)
                        break;
                ;

                if (node->removed)
                        return;
        }

        if (this->removeNode(node))
                return;

        node->error = errno;
        node->state = PR_ST_ERROR;
        delete node->res;
        this->m_callback(castPollerNodeToResult(node), this->m_context);
}

void Poller::handleConnect(struct PollerNode *node)
{
        socklen_t len = sizeof(int);
        int       error;

        if (getsockopt(node->data.fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
                error = errno;

        if (this->removeNode(node))
                return;

        if (error == 0)
        {
                node->error = 0;
                node->state = PR_ST_FINISHED;
        } else
        {
                node->error = error;
                node->state = PR_ST_ERROR;
        }

        this->m_callback(castPollerNodeToResult(node), this->m_context);
}

void Poller::handleRecvFrom(struct PollerNode *node)
{
        struct PollerNode      *res = node->res;
        struct sockaddr_storage ss;
        struct sockaddr        *addr = reinterpret_cast<struct sockaddr *>(&ss);
        socklen_t               addrlen;
        void                   *result;
        ssize_t                 n;

        while (1)
        {
                addrlen = sizeof(struct sockaddr_storage);
                n = recvfrom(node->data.fd, this->m_buf, POLLER_BUFSIZE, 0,
                             addr, &addrlen);

                if (n < 0)
                {
                        if (errno == EAGAIN)
                                return;
                        else
                                break;
                }
                result = node->data.recvfrom(addr, addrlen, this->m_buf, n,
                                             node->data.context);

                if (!result)
                        break;

                res->data        = node->data;
                res->data.result = result;
                res->error       = 0;
                res->state       = PR_ST_SUCCESS;
                this->m_callback(castPollerNodeToResult(res), this->m_context);

                res       = new PollerNode{};
                node->res = res;
                if (!res)
                        break;

                if (node->removed)
                        return;
        }

        if (this->removeNode(node))
                return;

        node->error = errno;
        node->state = PR_ST_ERROR;
        delete node->res;
        this->m_callback(castPollerNodeToResult(node), this->m_context);
}

void Poller::handleEvent(struct PollerNode *node)
{
        struct PollerNode *res = node->res;
        unsigned long long cnt = 0;
        unsigned long long value;
        void              *result;
        ssize_t            n;

        while (1)
        {
                n = read(node->data.fd, &value, sizeof(unsigned long long));
                if (n == sizeof(unsigned long long))
                        cnt += value;
                else
                {
                        if (n >= 0)
                                errno = EINVAL;
                        break;
                }
        }

        if (errno == EAGAIN)
        {
                while (1)
                {
                        if (cnt == 0)
                                return;

                        cnt--;
                        result = node->data.event(node->data.context);
                        if (!result)
                                break;

                        res->data        = node->data;
                        res->data.result = result;
                        res->error       = 0;
                        res->state       = PR_ST_SUCCESS;
                        this->m_callback(castPollerNodeToResult(res),
                                         this->m_context);

                        res       = new PollerNode{};
                        node->res = res;
                        if (!res)
                                break;

                        if (node->removed)
                                return;
                }
        }

        if (cnt != 0)
                write(node->data.fd, &cnt, sizeof(unsigned long long));

        if (this->removeNode(node))
                return;

        node->error = errno;
        node->state = PR_ST_ERROR;
        delete node->res;
        this->m_callback(castPollerNodeToResult(node), this->m_context);
}

void Poller::handleNotify(struct PollerNode *node)
{
        struct PollerNode *res = node->res;
        void              *result;
        ssize_t            n;

        while (1)
        {
                n = read(node->data.fd, &result, sizeof(void *));
                if (n == sizeof(void *))
                {
                        result = node->data.notify(result, node->data.context);
                        if (!result)
                                break;

                        res->data        = node->data;
                        res->data.result = result;
                        res->error       = 0;
                        res->state       = PR_ST_SUCCESS;
                        this->m_callback(castPollerNodeToResult(res),
                                         this->m_context);

                        res       = new PollerNode{};
                        node->res = res;
                        if (!res)
                                break;

                        if (node->removed)
                                return;
                } else if (n < 0 && errno == EAGAIN)
                        return;
                else
                {
                        if (n > 0)
                                errno = EINVAL;
                        break;
                }
        }

        if (this->removeNode(node))
                return;

        if (n == 0)
        {
                node->error = 0;
                node->state = PR_ST_FINISHED;
        } else
        {
                node->error = errno;
                node->state = PR_ST_ERROR;
        }

        delete node->res;
        this->m_callback(castPollerNodeToResult(node), this->m_context);
}

int Poller::handlePipe() const
{
        struct PollerNode **node = (struct PollerNode **) (this->m_buf);
        int                 stop = 0;
        int                 n;

        n = read(this->m_pipeRead, node, POLLER_BUFSIZE) / sizeof(void *);
        for (int i = 0; i < n; i++)
        {
                if (node[i])
                {
                        free(node[i]->res);
                        this->m_callback(castPollerNodeToResult(node[i]),
                                         this->m_context);
                } else
                        stop = 1;
        }

        return stop;
}

void Poller::handleTimeout(const struct PollerNode *timeNode)
{
        struct PollerNode *node;
        struct list_head  *pos, *tmp;
        LIST_HEAD(timeo_list);

        {
                std::unique_lock<std::mutex> lock(this->m_mutex);
                list_for_each_safe(pos, tmp, &this->m_timeoutList)
                {
                        node = list_entry(pos, struct PollerNode, list);
                        if (__timeout_cmp(node, timeNode) > 0)
                                break;

                        if (node->data.fd >= 0)
                        {
                                this->m_nodes[node->data.fd] = nullptr;
                                __poller_del_fd(node->data.fd, this->m_pfd);
                        } else
                                node->removed = 1;

                        list_move_tail(pos, &timeo_list);
                }
        }


        list_for_each_safe(pos, tmp, &timeo_list)
        {
                node = list_entry(pos, struct PollerNode, list);
                if (node->data.fd >= 0)
                {
                        node->error = ETIMEDOUT;
                        node->state = PR_ST_ERROR;
                } else
                {
                        node->error = 0;
                        node->state = PR_ST_FINISHED;
                }

                free(node->res);
                this->m_callback(castPollerNodeToResult(node), this->m_context);
        }
}


int Poller::removeNode(struct PollerNode *node)
{
        int removed;

        {
                std::unique_lock lock(m_mutex);
                removed = node->removed;
                if (!removed)
                {
                        this->m_nodes[node->data.fd] = nullptr;

                        list_del(&node->list);
                        __poller_del_fd(node->data.fd, this->m_pfd);
                }
        }

        return removed;
}

int Poller::appendMessage(const void *buf, size_t *n,
                          struct PollerNode *node) const
{
        PollerMessage     *msg = node->data.message;
        struct PollerNode *res;
        int                ret;

        if (!msg)
        {
                res = new PollerNode({});
                msg = node->data.createMessage(node->data.context);
                if (!msg)
                {
                        delete res;
                        return -1;
                }

                node->data.message = msg;
                node->res          = res;
        } else
                res = node->res;

        ret = msg->append(buf, n, msg);
        if (ret < 0)
        {
                res->data  = node->data;
                res->error = 0;
                res->state = PR_ST_SUCCESS;
                m_callback(castPollerNodeToResult(res), m_context);

                node->data.message = nullptr;
                node->res          = nullptr;
        }

        return ret;
}

void *Poller::threadRoutine()
{
        epoll_event         events[POLLER_EVENTS_MAX];
        struct PollerNode   timeNode = {};
        struct PollerNode  *node;
        struct PollerParams p;
        int                 hasPipeEvent;
        int                 nEvents;

        while (1)
        {
                this->setTimer();
                nEvents = epoll_wait(m_pfd, events, POLLER_EVENTS_MAX, -1);
                clock_gettime(CLOCK_MONOTONIC, &timeNode.timeout);
                hasPipeEvent = 0;
                for (int i = 0; i < nEvents; i++)
                {
                        node = reinterpret_cast<struct PollerNode *>(
                                &events[i].data.ptr);
                        if (node <= reinterpret_cast<struct PollerNode *>(1))
                        {
                                if (node ==
                                    reinterpret_cast<struct PollerNode *>(1))
                                        hasPipeEvent = 1;
                                continue;
                        }

                        switch (node->data.operation)
                        {
                                case PD_OP_READ:
                                        handleRead(node);
                                        break;
                                case PD_OP_WRITE:
                                        handleWrite(node);
                                        break;
                                case PD_OP_LISTEN:
                                        handleListen(node);
                                        break;
                                case PD_OP_CONNECT:
                                        handleConnect(node);
                                        break;
                                case PD_OP_RECVFROM:
                                        handleRecvFrom(node);
                                        break;
                                        // case PD_OP_SSL_ACCEPT:
                                        //         __poller_handle_ssl_accept(node,
                                        //         poller);
                                        // break;
                                        // case PD_OP_SSL_CONNECT:
                                        //         __poller_handle_ssl_connect(node,
                                        //         poller);
                                        // break;
                                        // case PD_OP_SSL_SHUTDOWN:
                                        //         __poller_handle_ssl_shutdown(node,
                                        //         poller);
                                        // break;
                                case PD_OP_EVENT:
                                        handleEvent(node);
                                        break;
                                case PD_OP_NOTIFY:
                                        handleNotify(node);
                                        break;
                                default:
                                        break;
                        }
                }

                if (hasPipeEvent)
                {
                        if (handlePipe())
                                break;
                }

                handleTimeout(&timeNode);
        }
        return nullptr;
}

void Poller::setTimer()
{
        struct PollerNode *node = nullptr;
        struct timespec    abstime;

        std::unique_lock lock(m_mutex);
        if (!list_empty(&m_timeoutList))
                node = list_entry(m_timeoutList.next, struct PollerNode, list);

        if (node)
                abstime = node->timeout;
        else
        {
                abstime.tv_sec  = 0;
                abstime.tv_nsec = 0;
        }
        __poller_set_timerfd(m_timerfd, &abstime);
}
