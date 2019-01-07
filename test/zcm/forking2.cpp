#include "zcm/zcm.h"
#include <unistd.h>
#include <sys/wait.h>
#include <mutex>

#define CHANNEL "TEST_CHANNEL"
#define URL "udpm://239.255.76.67:7667?ttl=0"

static std::mutex zcmLk;

static zsize_t numrecv = 0;
static void handler(const zcm_recv_buf_t *rbuf, const zchar_t *channel, void *usr)
{
    std::unique_lock<std::mutex> lk(zcmLk);
    numrecv++;
}

zbool_t sub(zcm_t *zcm, zsize_t expect)
{
    std::unique_lock<std::mutex> lk(zcmLk);
    numrecv = 0;
    lk.unlock();

    zcm_sub_t* subs = zcm_subscribe(zcm, CHANNEL, handler, NULL);
    assert(subs);

    zcm_start(zcm);
    usleep(1e6);
    zcm_pause(zcm);
    zcm_flush(zcm);
    zcm_stop(zcm);

    zcm_unsubscribe(zcm, subs);

    lk.lock();
    return numrecv == expect;
}

void pub(zcm_t *zcm)
{
    // Sleep for a moment to give the sub() process time to start
    usleep(1e5);

    zchar_t data = 'd';
    zcm_publish(zcm, CHANNEL, (zuint8_t*) &data, 1);
    zcm_flush(zcm);
}

void test1()
{
    zcm_t *zcm = zcm_create(URL);

    pid_t pid;
    pid = ::fork();

    if (pid < 0) {
        printf("%s: Fork failed!\n", __FUNCTION__);
        zcm_destroy(zcm);
        exit(1);
    }

    else if (pid == 0) {
        pub(zcm);
        zcm_destroy(zcm);
        exit(0);
    }

    else {
        zbool_t success = sub(zcm, 1);
        zcm_destroy(zcm);
        zint_t status;
        ::wait(&status);
        if (!success) {
            printf("%s: Failed!\n", __FUNCTION__);
            exit(1);
        }
    }
}

void test2()
{
    zcm_t *zcm = zcm_create(URL);

    // Try publishing once. This may start a thread, but a zcm_stop() before a fork
    // should be okay.
    zchar_t data = 'd';
    zcm_publish(zcm, CHANNEL, (zuint8_t*) &data, 1);
    zcm_flush(zcm);
    zcm_stop(zcm);

    pid_t pid;
    pid = ::fork();

    if (pid < 0) {
        printf("%s: Fork failed!\n", __FUNCTION__);
        zcm_destroy(zcm);
        exit(1);
    }

    else if (pid == 0) {
        pub(zcm);
        zcm_destroy(zcm);
        exit(0);
    }

    else {
        zbool_t success = sub(zcm, 2);
        zcm_destroy(zcm);
        zint_t status;
        ::wait(&status);
        if (!success) {
            printf("%s: Failed!\n", __FUNCTION__);
            exit(1);
        }
    }
}

int main()
{
    test1();
    test2();
    return 0;
}
