#include "zcm/zcm-cpp.hpp"
#include "types/example_t.hpp"

#include "multi_file.hpp"

#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <cinttypes>

using namespace std;

#define vprintf(...) do { \
    if (verbose) { \
        printf(__VA_ARGS__); \
    } \
} while(0)

#define NUM_DATA 5
static zbool_t verbose = zfalse;
static zint_t  retval = 0;

static zchar_t data[NUM_DATA] = {'a', 'b', 'c', 'd', 'e'};

class Handler
{
  public:
    zsize_t  num_received = 0;
    zint32_t bytepacked_received = 0;

    void generic_handle(const zcm::ReceiveBuffer *rbuf, const zstring_t& channel)
    {
        vprintf("%" PRIi64 " - %s: ", rbuf->recv_utime, channel.c_str());
        zsize_t i;
        for (i = 0; i < rbuf->data_size; ++i) {
            vprintf("%c ", rbuf->data[i]);

            num_received++;
            zsize_t j;
            for (j = 0; j < NUM_DATA; ++j) {
                if (rbuf->data[i] == data[j]) {
                    bytepacked_received |= 1 << j;
                }
            }
        }
        vprintf("\n");
        fflush(stdout);
    }

    zsize_t  num_typed_received = 0;
    zint32_t bytepacked_typed_received = 0;
    void example_t_handle(const zcm::ReceiveBuffer *rbuf, const zstring_t& channel,
                          const example_t *msg)
    {
        vprintf("%" PRIi64 " - %s: ", rbuf->recv_utime, channel.c_str());
        vprintf("%ld", msg->utime);
        bytepacked_typed_received |= (zint32_t) msg->utime;
        num_typed_received++;
        vprintf("\n");
        MyStruct::greet(zfalse, msg);
        fflush(stdout);
    }
};

int main(int argc, const char *argv[])
{
    zsize_t sleep_time = 200000;

    for (zint_t i = 1; i < argc; ++i) {
        if (zstring_t(argv[i]) == "-h") {
            cout << "Usage: " << argv[0] << " [-h] [-v] [-s <sleep_us>]\n"
                 << "       -h help\n"
                 << "       -v verbose\n"
                 << "       -s <sleep_us>  sleep time (def = 200000), increase for valgrind\n";
            return 0;
        } else if (zstring_t(argv[i]) == "-v") {
            verbose = ztrue;
        } else if (zstring_t(argv[i]) == "-s") {
            if (++i == argc) {
                cerr << "option -s requires argument\n";
                return 1;
            } else {
                sleep_time = atoi(argv[i]);
            }
        }
    }

    MyStruct::greet();

    Handler handler;

    zstring_t transports[2] = {"ipc", "inproc"};
    for (zsize_t i = 0; i < 2; ++i) {
        zcm::ZCM zcm(transports[i]);
        vprintf("Creating zcm %s\n", transports[i].c_str());
        if (!zcm.good()) {
            cerr << "Failed to create zcm\n";
            return 1;
        }


        // TEST RAW DATA TRANSMISSION /////////////////////////////////////////
        handler.num_received = 0;
        handler.bytepacked_received = 0;
        zcm::Subscription *sub = zcm.subscribe("TEST", &Handler::generic_handle, &handler);
        zcm.publish("TEST", (zuint8_t*) data, sizeof(zuint8_t));

        // zmq sockets are documented as taking a small but perceptible amount of time
        // to actuall establish connection, so in order to actually receive messages
        // through them, we must wait for them to be set up
        usleep(sleep_time);

        vprintf("Starting zcm receive %s\n", transports[i].c_str());
        zcm.start();

        for (zsize_t j = 0; j < NUM_DATA; ++j) {
            zcm.publish("TEST", (zuint8_t*) data + j, sizeof(zuint8_t));
        }

        usleep(sleep_time);
        vprintf("Stopping zcm receive %s\n", transports[i].c_str());
        zcm.stop();

        vprintf("Unsubscribing zcm %s\n", transports[i].c_str());
        zcm.unsubscribe(sub);

        for (zsize_t j = 0; j < NUM_DATA; ++j) {
            if (!(handler.bytepacked_received & 1 << j)) {
                cerr << transports[i] << ": Missed a message: " << data[j] << endl;
                ++retval;
            }
        }
        if (handler.num_received != NUM_DATA && handler.num_received != NUM_DATA+1) {
            cerr << transports[i] << ": Received an unexpected number of messages: "
                 << handler.num_received << endl;
            ++retval;
        }


        // TEST TYPED DATA TRANSMISSION ///////////////////////////////////////
        handler.num_typed_received = 0;
        handler.bytepacked_typed_received = 0;

        example_t ex_data;
        ex_data.utime = 1 << 0;
        ex_data.position[0] = 1;
        ex_data.position[1] = 2;
        ex_data.position[2] = 3;
        ex_data.orientation[0] = 1;
        ex_data.orientation[1] = 0;
        ex_data.orientation[2] = 0;
        ex_data.orientation[3] = 0;
        ex_data.num_ranges = 100;
        ex_data.ranges.resize(ex_data.num_ranges);
        ex_data.name = "example string";
        ex_data.enabled = 1;

        zcm::Subscription *ex_sub = zcm.subscribe("EXAMPLE", &Handler::example_t_handle, &handler);
        zcm.publish("EXAMPLE", &ex_data);

        // zmq sockets are documented as taking a small but perceptible amount of time
        // to actuall establish connection, so in order to actually receive messages
        // through them, we must wait for them to be set up
        usleep(sleep_time);

        vprintf("Starting zcm receive %s\n", transports[i].c_str());
        zcm.start();

        for (zsize_t j = 0; j < NUM_DATA; ++j) {
            ex_data.utime = 1 << j;
            zcm.publish("EXAMPLE", &ex_data);
        }

        usleep(sleep_time);
        vprintf("Stopping zcm receive %s\n", transports[i].c_str());
        zcm.stop();

        vprintf("Unsubscribing zcm %s\n", transports[i].c_str());
        zcm.unsubscribe(ex_sub);

        for (zsize_t j = 0; j < NUM_DATA; ++j) {
            if (!(handler.bytepacked_received & 1 << j)) {
                ++retval;
            }
        }
        if (handler.num_received != NUM_DATA && handler.num_received != NUM_DATA+1) {
            ++retval;
        }
        for (zsize_t j = 0; j < NUM_DATA; ++j) {
            if (!(handler.bytepacked_typed_received & 1 << j)) {
                cerr << transports[i] << ": Missed a message: " << (1 << j) << endl;
                ++retval;
            }
        }
        if (handler.num_typed_received != NUM_DATA && handler.num_typed_received != NUM_DATA+1) {
            cerr << transports[i] << ": Received an unexpected number of messages: "
                 << handler.num_typed_received << endl;
            ++retval;
        }


        vprintf("Cleaning up zcm %s\n", transports[i].c_str());
    }

    return 0;
}
