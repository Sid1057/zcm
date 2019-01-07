#include <iostream>
#include <thread>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include "zcm/transport_registrar.h"
#include "util/TimeUtil.hpp"
using namespace std;

#define HZ 500
#define MSG_COUNT 100

static zbool_t BIG_MESSAGE = ztrue;
volatile zbool_t running_recv = ztrue;
volatile zbool_t running_send = ztrue;
static void sighandler(zint_t sig) { running_recv = zfalse; }

static zcm_trans_t *makeTransport()
{
    const zchar_t *url = getenv("ZCM_DEFAULT_URL");
    if (!url) {
        fprintf(stderr, "ERR: Unable to find environment variable ZCM_DEFAULT_URL\n");
        return NULL;
    }
    auto *u = zcm_url_create(url);
    auto *creator = zcm_transport_find(zcm_url_protocol(u));
    if (!creator) {
        fprintf(stderr, "ERR: Failed to create transport for '%s'\n", url);
        return NULL;
    }
    return creator(u);
}

static zcm_msg_t makeMasterMsg()
{
    zcm_msg_t msg;
    msg.utime = TimeUtil::utime();
    msg.channel = "FOO";
    msg.len = BIG_MESSAGE ? 500000 : 1000;
    msg.buf = (zuint8_t*) zcm_malloc(msg.len);
    for (zsize_t i = 0; i < msg.len; i++)
        msg.buf[i] = (zchar_t)(i & 0xff);

    return msg;
}

#define fail(...) \
    do {\
        fprintf(stderr, "Err:"); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
        exit(1); \
    } while(0)

static void verifySame(zcm_msg_t *a, zcm_msg_t *b)
{
    if (b->utime < a->utime || a->utime == 0 || b->utime == 0)
        fail("Utime failure!");
    if (strcmp(a->channel, b->channel) != 0)
        fail("Channels don't match!");
    if (a->len != b->len)
        fail("Lengths don't match!");
    for (zsize_t i = 0; i < a->len; i++)
        if (a->buf[i] != b->buf[i])
            fail("Data doesn't match at index %d", (zint_t)i);
}

static void send()
{
    auto *trans = makeTransport();
    if (!trans)
        exit(1);

    usleep(10000); // sleep 10ms so the recv thread can come up

    zcm_msg_t msg = makeMasterMsg();
    for (zsize_t i = 0; i < MSG_COUNT && running_send; i++) {
        zcm_trans_sendmsg(trans, msg);
        usleep(1000000 / HZ);
    }
}

static void recv()
{
    auto *trans = makeTransport();
    if (!trans)
        exit(1);

    // Tell the transport to give us all of the channels
    zcm_trans_recvmsg_enable(trans, NULL, ztrue);

    zcm_msg_t master = makeMasterMsg();
    zuint64_t start = TimeUtil::utime();
    zsize_t i;
    for (i = 0; i < MSG_COUNT && running_recv; i++) {
        zcm_msg_t msg;
        zcm_retcode_t ret = zcm_trans_recvmsg(trans, &msg, 100);
        if (ret == ZCM_EOK) {
            verifySame(&master, &msg);
        }
    }
    zuint64_t end = TimeUtil::utime();

    cout << "Received " << (i * 100 / MSG_COUNT) << "\% of the messages in "
         << ((end - start) / 1e6) << " seconds" <<  endl;

    running_send = zfalse;
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        string opt {argv[1]};
        if (opt == "--small")
            BIG_MESSAGE = zfalse;
    }

    signal(SIGINT, sighandler);

    std::thread sendThread {send};
    std::thread recvThread {recv};

    recvThread.join();
    sendThread.join();

    return 0;
}
