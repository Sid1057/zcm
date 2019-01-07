#pragma once

#include <vector>

#include "zcm/zcm.h"

#ifndef ZCM_EMBEDDED
#include "zcm/eventlog.h"
#endif

#if __cplusplus > 199711L
#include <functional>
#endif

namespace zcm {

typedef zcm_recv_buf_t ReceiveBuffer;
typedef zcm_msg_handler_t MsgHandler;
class Subscription;

class ZCM
{
  public:
    #ifndef ZCM_EMBEDDED
    inline ZCM();
    inline ZCM(const zstring_t& transport);
    #endif
    inline ZCM(zcm_trans_t* zt);
    virtual inline ~ZCM();

    virtual inline zbool_t good() const;
    virtual inline zcm_retcode_t  err() const; // returns zcm_errno()
    virtual inline const zchar_t* strerror() const;
    virtual inline const zchar_t* strerrno(zcm_retcode_t err) const;

    #ifndef ZCM_EMBEDDED
    virtual inline          void run();
    virtual inline          void start();
    virtual inline          void stop();
    virtual inline          void pause();
    virtual inline          void resume();
    virtual inline zcm_retcode_t handle();
    virtual inline          void setQueueSize(zuint32_t sz);
    #endif
    virtual inline zcm_retcode_t handleNonblock();
    virtual inline          void flush();

  public:
    inline zcm_retcode_t publish(const zstring_t& channel, const zuint8_t* data, zuint32_t len);

    // Note: if we make a publish binding that takes a const message reference, the compiler does
    //       not select the right version between the pointer and reference versions, so when the
    //       user intended to call the pointer version, the reference version is called and causes
    //       compile errors (turns the input into a double pointer). We have to choose one or the
    //       other for the api.
    template <class Msg>
    inline zcm_retcode_t publish(const zstring_t& channel, const Msg* msg);

    inline Subscription* subscribe(const zstring_t& channel,
                                   void (*cb)(const ReceiveBuffer* rbuf,
                                              const zstring_t& channel,
                                              void* usr),
                                   void* usr);

    template <class Msg, class Handler>
    inline Subscription* subscribe(const zstring_t& channel,
                                   void (Handler::*cb)(const ReceiveBuffer* rbuf,
                                                       const zstring_t& channel,
                                                       const Msg* msg),
                                   Handler* handler);

    template <class Handler>
    inline Subscription* subscribe(const zstring_t& channel,
                                   void (Handler::*cb)(const ReceiveBuffer* rbuf,
                                                       const zstring_t& channel),
                                   Handler* handler);

    template <class Msg>
    inline Subscription* subscribe(const zstring_t& channel,
                                   void (*cb)(const ReceiveBuffer* rbuf,
                                              const zstring_t& channel,
                                              const Msg* msg, void* usr),
                                   void* usr);

    #if __cplusplus > 199711L
    template <class Msg>
    inline Subscription* subscribe(const zstring_t& channel,
                                   std::function<void (const ReceiveBuffer* rbuf,
                                                       const zstring_t& channel,
                                                       const Msg* msg)> cb);
    #endif

    inline void unsubscribe(Subscription* sub);

    virtual inline zcm_t* getUnderlyingZCM();

  protected:
    /**** Methods for inheritor override ****/
    virtual inline zcm_retcode_t publishRaw(const zstring_t& channel,
                                            const zuint8_t* data,
                                            zuint32_t len);

    // Set the value of "rawSub" with your underlying subscription. "rawSub" will be passed
    // (by reference) into unsubscribeRaw when zcm->unsubscribe() is called on a cpp subscription
    virtual inline void subscribeRaw(void*& rawSub, const zstring_t& channel,
                                     MsgHandler cb, void* usr);

    // Unsubscribes from a raw subscription. Effectively undoing the actions of subscribeRaw
    virtual inline void unsubscribeRaw(void*& rawSub);

  private:
    zcm_t* zcm;
    std::vector<Subscription*> subscriptions;
};

// New class required to allow the Handler callbacks and zstring_t channel names
class Subscription
{
    friend class ZCM;
    void* rawSub;

  protected:
    void* usr;
    void (*callback)(const ReceiveBuffer* rbuf, const zstring_t& channel, void* usr);

  public:
    virtual ~Subscription() {}

    void* getRawSub() const
    { return rawSub; }

    inline void dispatch(const ReceiveBuffer* rbuf, const zstring_t& channel)
    { (*callback)(rbuf, channel, usr); }

    static inline void dispatch(const ReceiveBuffer* rbuf, const zchar_t* channel, void* usr)
    { ((Subscription*)usr)->dispatch(rbuf, channel); }
};

// TODO: why not use or inherit from the existing zcm data structures for the below

#ifndef ZCM_EMBEDDED
struct LogEvent
{
    zuint64_t eventnum;
    zuint64_t timestamp;
    zstring_t channel;
    zuint32_t datalen;
    zuint8_t* data;
};

struct LogFile
{
    /**** Methods for ctor/dtor/check ****/
    inline LogFile(const zstring_t& path, const zstring_t& mode);
    inline ~LogFile();
    inline zbool_t good() const;
    inline void close();

    /**** Methods general operations ****/
    inline zcm_retcode_t seekToTimestamp(zuint64_t timestamp);
    inline FILE* getFilePtr();

    /**** Methods for read/write ****/
    // NOTE: user should NOT hold-onto the returned ptr across successive calls
    inline const LogEvent* readNextEvent();
    inline const LogEvent* readPrevEvent();
    inline const LogEvent* readEventAtOffset(zoff_t offset);
    inline zcm_retcode_t   writeEvent(const LogEvent* event);

  private:
    inline const LogEvent* cplusplusIfyEvent(zcm_eventlog_event_t* le);
    LogEvent curEvent;
    zcm_eventlog_t* eventlog;
    zcm_eventlog_event_t* lastevent;
};
#endif

#define __zcm_cpp_impl_ok__
#include "zcm-cpp-impl.hpp"
#undef __zcm_cpp_impl_ok__

}
