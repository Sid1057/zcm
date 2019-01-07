#include "zcm/zcm.h"
#include <unistd.h>
#include <sys/wait.h>
#include <mutex>

static std::mutex zcmLk;

#define CHANNEL "TEST_CHANNEL"

static zsize_t numrecv = 0;
static void handler(const zcm_recv_buf_t *rbuf, const zchar_t *channel, void *usr)
{
    std::unique_lock<std::mutex> lk(zcmLk);
    numrecv++;
}

zbool_t sub(zcm_t *zcm)
{
    zcm_sub_t* subs = zcm_subscribe(zcm, CHANNEL, handler, NULL);
    assert(subs);

    zcm_start(zcm);
    usleep(1e6);
    zcm_pause(zcm);
    zcm_flush(zcm);
    zcm_stop(zcm);

    zcm_unsubscribe(zcm, subs);

    std::unique_lock<std::mutex> lk(zcmLk);
    return numrecv == 1;
}

void pub(zcm_t *zcm)
{
    // Sleep for a moment to give the sub() process time to start
    usleep(1e5);

    zchar_t data = 'd';
    zcm_publish(zcm, CHANNEL, (zuint8_t*) &data, 1);
    usleep(1e5);
    zcm_publish(zcm, CHANNEL, (zuint8_t*) &data, 1);
    zcm_flush(zcm);
}

int main()
{
    pid_t pid;
    pid = ::fork();

    zcm_t *zcm = zcm_create("ipc");

    if (pid < 0) {
        printf("Fork failed!\n");
        exit(1);
    }

    else if (pid == 0) {
        pub(zcm);
        zcm_destroy(zcm);
    }

    else {
        zbool_t success = sub(zcm);
        zcm_destroy(zcm);
        zint_t status;
        ::wait(&status);
        if (success) {
            return 0;
        } else {
            printf("Failed!\n");
            return 1;
        }
    }

    return 0;
}
