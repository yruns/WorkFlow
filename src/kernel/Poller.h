//
// Created by yruns on 2025/3/12.
//

#ifndef POLLER_H
#define POLLER_H

#include <any>
#include <functional>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/types.h>

struct PollerMessage
{
    std::function<int(void *, std::size_t *, PollerMessage *)> append;
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

    short operation;
    unsigned short iovcnt;
    int fd;
    SSL *ssl;

    union
    {
        std::function<PollerMessage *(void *)> createMessage;
        std::function<int(size_t, void *)> partialWritten;
        std::function<void *(const struct sockaddr *, socklen_t, int, void *)>
                accept;
        std::function<void *(const struct sockaddr *, socklen_t, void *,
                             std::size_t, void *)>
                recvfrom;
        std::function<void *(void *)> event;
        std::function<void *(void *, void *)> notify;
    };
    void *content;
    union
    {
        PollerMessage *message;
        struct iovec *writeIov;
        void *result;
    };
};

struct PollerResult
{
#define PR_ST_SUCCESS		0
#define PR_ST_FINISHED		1
#define PR_ST_ERROR			2
#define PR_ST_DELETED		3
#define PR_ST_MODIFIED		4
#define PR_ST_STOPPED		5

    int state;
    int error;
    struct PollerData data;
    /* In callback, spaces of six pointers are available from here. */
};

struct PollerParams
{
    size_t maxOpenFiles;
    std::function<void(struct PollerResult*, void *)> callback;
    void *content;
};

class Poller
{
private:

};

#endif // POLLER_H
