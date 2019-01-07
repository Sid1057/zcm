#include <iostream>
#include <cmath>
#include <sys/time.h>
#include <unistd.h>
#include <random>

#include <zcm/zcm-cpp.hpp>
#include <zcm/message_tracker.hpp>
#include <zcm/transport/generic_serial_transport.h>

#include "util/TimeUtil.hpp"

#include "types/example_t.hpp"

using namespace std;

#define MTU (1<<12)

atomic_int callbackTriggered {0};

constexpr zfloat64_t periodExpected = 1e3;
constexpr zfloat64_t noiseStdDev = periodExpected / 30;

static void callback(example_t* msg, zuint64_t utime, void* usr)
{
    callbackTriggered++;
    delete msg;
}
static constexpr zuint32_t maxBufSize = 1e5;
static std::deque<zuint8_t> buf;

static std::random_device rd;
static std::minstd_rand gen(rd());
static std::normal_distribution<zfloat64_t> dist(0.0, noiseStdDev);

zuint32_t get(zuint8_t* data, zuint32_t nData, void* usr)
{
    zuint32_t ret = std::min(nData, (zuint32_t) buf.size());
    if (ret == 0) return 0;
    for (zuint32_t i = 0; i < ret; ++i) {
        data[i] = buf.front();
        buf.pop_front();
    }
    return ret;
}

zuint32_t put(const zuint8_t* data, zuint32_t nData, void* usr)
{
    zuint32_t ret = std::min(nData, (zuint32_t) (maxBufSize - buf.size()));
    if (ret == 0) return 0;
    for (zuint32_t i = 0; i < ret; ++i) buf.push_back(data[i]);
    assert(buf.size() <= maxBufSize);
    return ret;
}

zuint64_t timestamp_now(void* usr)
{
    static zuint64_t i = 100;
    static zuint64_t j = 0;
    static zbool_t shouldIncrement = ztrue;
    if (shouldIncrement) {
        j++;
        i += periodExpected + dist(gen);
    }
    shouldIncrement = !shouldIncrement;
    return i;
}

int main(int argc, char *argv[])
{
    constexpr zsize_t numMsgs = 500;

    zcm_trans_t* trans = zcm_trans_generic_serial_create(get, put, NULL, timestamp_now,
                                                         NULL, MTU, MTU * 5);

    zcm::ZCM zcmLocal(trans);
    zcm::MessageTracker<example_t> mt(&zcmLocal, "EXAMPLE", numMsgs, numMsgs, callback);

    example_t msg = {};

    zsize_t nTimesJitterWasRight = 0, nTimesHzWasRight = 0;

    zuint64_t now[numMsgs];
    for (zsize_t i = 0; i < numMsgs; ++i) {
        now[i] = (zuint64_t) (i * periodExpected);
        msg.utime = now[i];
        zcmLocal.publish("EXAMPLE", &msg);
        zcmLocal.flush();

        if (mt.getJitterUs() > noiseStdDev * 0.8 &&
            mt.getJitterUs() < noiseStdDev * 1.2) nTimesJitterWasRight++;

        if (mt.getHz() > 1e6 / periodExpected * 0.95 &&
            mt.getHz() < 1e6 / periodExpected * 1.05) nTimesHzWasRight++;
    }

    usleep(1e5);
    assert(callbackTriggered > 0);

    example_t* recv = mt.get(now[3]);
    assert(recv);
    assert((zuint64_t)recv->utime >= now[2] && (zuint64_t)recv->utime <= now[4]);
    delete recv;

    assert(nTimesJitterWasRight > 0.5 * numMsgs);
    assert(nTimesHzWasRight > 0.7 * numMsgs);
}
