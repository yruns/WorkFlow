//
// Created by yruns on 2025/3/12.
//

#ifndef POLLER_H
#define POLLER_H

#include <functional>
#include <mutex>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <vector>

#include "RBTree.h"

#define POLLER_BUFSIZE (256 * 1024)
#define POLLER_EVENTS_MAX 256

struct PollerMessage
{
        std::function<int(const void *, std::size_t *, PollerMessage *)> append;
        char data[0];
};

struct PollerData
{
#define PD_OP_TIMER 0
#define PD_OP_READ 1
#define PD_OP_WRITE 2
#define PD_OP_LISTEN 3
#define PD_OP_CONNECT 4
#define PD_OP_RECVFROM 5
#define PD_OP_SSL_READ PD_OP_READ
#define PD_OP_SSL_WRITE PD_OP_WRITE
#define PD_OP_SSL_ACCEPT 6
#define PD_OP_SSL_CONNECT 7
#define PD_OP_SSL_SHUTDOWN 8
#define PD_OP_EVENT 9
#define PD_OP_NOTIFY 10

        short          operation;
        unsigned short iovcnt;
        int            fd;
        SSL           *ssl;

        union
        {
                std::function<PollerMessage *(void *)> createMessage;
                std::function<int(size_t, void *)>     partialWritten;
                std::function<void *(const struct sockaddr *, socklen_t, int,
                                     void *)>
                        accept;
                std::function<void *(const struct sockaddr *, socklen_t, void *,
                                     std::size_t, void *)>
                                                      recvfrom;
                std::function<void *(void *)>         event;
                std::function<void *(void *, void *)> notify;
        };
        void *context;
        union
        {
                PollerMessage *message;
                struct iovec  *writeIov;
                void          *result;
        };
};

struct PollerResult
{
#define PR_ST_SUCCESS 0
#define PR_ST_FINISHED 1
#define PR_ST_ERROR 2
#define PR_ST_DELETED 3
#define PR_ST_MODIFIED 4
#define PR_ST_STOPPED 5

        int               state;
        int               error;
        struct PollerData data;
        /* In callback, spaces of six pointers are available from here. */
};

struct PollerParams
{
        size_t                                             maxOpenFiles;
        std::function<void(struct PollerResult *, void *)> callback;
        void                                              *content;
};

struct PollerNode
{
        int                state;
        int                error;
        struct PollerData  data;
        struct list_head   list;
        char               inRbtree;
        char               removed;
        int                event;
        struct timespec    timeout;
        struct PollerNode *res;
};

inline PollerResult *castPollerNodeToResult(struct PollerNode *node)
{
        return reinterpret_cast<struct PollerResult *>(node);
}

class Poller
{
    public:
        explicit Poller(const struct PollerParams *params);

        int start();

        int pfd() const { return m_pfd; }

        void handleRead(struct PollerNode *node);

        void handleWrite(struct PollerNode *node);

        void handleListen(struct PollerNode *node);

        void handleConnect(struct PollerNode *node);

        void handleRecvFrom(struct PollerNode *node);

        void handleEvent(struct PollerNode *node);

        void handleTimeout(const struct PollerNode *timeNode);

        void handleNotify(struct PollerNode *node);

        int handlePipe() const;

        int removeNode(struct PollerNode *node);

        int appendMessage(const void *buf, size_t *n,
                          struct PollerNode *node) const;

        void *threadRoutine();

        void setTimer();

        void setPipeRead(const int pipefd) { m_pipeRead = pipefd; }

        void setPipeWrite(const int pipefd) { m_pipeWrite = pipefd; }


    private:
        typedef std::vector<struct PollerNode *> PollerNodePtrList;

        size_t                                             m_maxOpenFiles;
        std::function<void(struct PollerResult *, void *)> m_callback;
        void                                              *m_context;

        std::unique_ptr<std::thread> m_thread;
        int                          m_pfd;
        int                          m_timerfd;
        int                          m_pipeRead;
        int                          m_pipeWrite;
        int                          m_stopped;
        // struct rb_root               m_timeoutTree;
        // struct rb_node              *m_treeFirst;
        // struct rb_node              *m_treeLast;
        struct list_head  m_timeoutList;
        struct list_head  m_nonTimeoutList;
        PollerNodePtrList m_nodes;
        std::mutex        m_mutex;
        char              m_buf[POLLER_BUFSIZE];
};

#endif // POLLER_H
