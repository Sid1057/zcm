#include <errno.h>

#include "udpm.hpp"
#include "buffers.hpp"
#include "udpmsocket.hpp"
#include "mempool.hpp"

#include "zcm/transport.h"
#include "zcm/transport_registrar.h"
#include "zcm/transport_register.hpp"

#include "util/StringUtil.hpp"

#define MTU (1<<28)

static zint32_t utimeInSeconds()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (zint32_t)tv.tv_sec;
}

/**
 * udpm_params_t:
 * @mc_addr:        multicast address
 * @mc_port:        multicast port
 * @mc_ttl:         if 0, then packets never leave local host.
 *                  if 1, then packets stay on the local network
 *                        and never traverse a router
 *                  don't use > 1.  that's just rude.
 * @recv_buf_size:  requested size of the kernel receive buffer, set with
 *                  SO_RCVBUF.  0 indicates to use the default settings.
 *
 */
struct Params
{
    zstring_t ip;
    struct in_addr addr;
    zuint16_t port;
    zuint8_t  ttl;
    zsize_t   recv_buf_size;

    Params(const zstring_t& ip, zuint16_t port, zsize_t recv_buf_size, zuint8_t ttl)
    {
        // TODO verify that the IP and PORT are vaild
        this->ip = ip;
        inet_aton(ip.c_str(), (struct in_addr*) &this->addr);
        this->port = port;
        this->recv_buf_size = recv_buf_size;
        this->ttl = ttl;
    }
};

struct UDPM
{
    Params params;
    UDPMAddress destAddr;

    UDPMSocket recvfd;
    UDPMSocket sendfd;

    /* size of the kernel UDP receive buffer */
    zsize_t kernel_rbuf_sz = 0;
    zsize_t kernel_sbuf_sz = 0;
    zbool_t warned_about_small_kernel_buf = zfalse;

    MessagePool pool {MAX_FRAG_BUF_TOTAL_SIZE, MAX_NUM_FRAG_BUFS};

    /* other variables */
    zuint32_t  udp_rx = 0;            // packets received and processed
    zuint32_t  udp_discarded_bad = 0; // packets discarded because they were bad
                                         // somehow
    zfloat64_t udp_low_watermark = 1.0; // least buffer available
    zint32_t   udp_last_report_secs = 0;

    zuint32_t  msg_seqno = 0; // rolling counter of how many messages transmitted

    /***** Methods ******/
    UDPM(const zstring_t& ip, zuint16_t port, zsize_t recv_buf_size, zuint8_t ttl);
    zbool_t init();
    ~UDPM();

    zcm_retcode_t sendmsg(zcm_msg_t msg);
    zcm_retcode_t recvmsg(zcm_msg_t *msg, zint_t timeout);

  private:
    // These returns non-null when a full message has been received
    Message *recvShort(Packet *pkt, zuint32_t sz);
    Message *recvFragment(Packet *pkt, zuint32_t sz);
    Message *readMessage(zint_t timeout);

    Message *m = nullptr;

    zbool_t selftest();
    void checkForMessageLoss();
};

Message *UDPM::recvShort(Packet *pkt, zuint32_t sz)
{
    MsgHeaderShort *hdr = pkt->asHeaderShort();

    zsize_t clen = hdr->getChannelLen();
    if (clen > ZCM_CHANNEL_MAXLEN) {
        ZCM_DEBUG("bad channel name length");
        udp_discarded_bad++;
        return NULL;
    }

    udp_rx++;

    Message *msg = pool.allocMessageEmpty();
    msg->utime = pkt->utime;
    msg->channel = hdr->getChannelPtr();
    msg->channellen = clen;
    msg->data = hdr->getDataPtr();
    msg->datalen = hdr->getDataLen(sz);
    pool.moveBuffer(msg->buf, pkt->buf);

    return msg;
}

Message *UDPM::recvFragment(Packet *pkt, zuint32_t sz)
{
    MsgHeaderLong *hdr = pkt->asHeaderLong();

    // any existing fragment buffer for this message source?
    FragBuf *fbuf = pool.lookupFragBuf((struct sockaddr_in*)&pkt->from);

    zuint32_t msg_seqno = hdr->getMsgSeqno();
    zuint32_t data_size = hdr->getMsgSize();
    zuint32_t fragment_offset = hdr->getFragmentOffset();
    zuint16_t fragment_no = hdr->getFragmentNo();
    zuint16_t fragments_in_msg = hdr->getFragmentsInMsg();
    zuint32_t frag_size = hdr->getFragmentSize(sz);
    zchar_t *data_start = hdr->getDataPtr();

    // discard any stale fragments from previous messages
    if (fbuf && ((fbuf->msg_seqno != msg_seqno) ||
                 (fbuf->buf.size != data_size + fbuf->channellen+1))) {
        pool.removeFragBuf(fbuf);
        ZCM_DEBUG("Dropping message (missing %d fragments)", fbuf->fragments_remaining);
        fbuf = NULL;
    }

    if (data_size > MTU) {
        ZCM_DEBUG("rejecting huge message (%d bytes)", data_size);
        return NULL;
    }

    // create a new fragment buffer if necessary
    if (!fbuf && fragment_no == 0) {
        zchar_t *channel = (zchar_t*) (hdr + 1);
        zint_t channel_sz = strlen(channel);
        if (channel_sz > ZCM_CHANNEL_MAXLEN) {
            ZCM_DEBUG("bad channel name length");
            udp_discarded_bad++;
            return NULL;
        }

        fbuf = pool.addFragBuf(channel_sz + 1 + data_size);
        fbuf->last_packet_utime = pkt->utime;
        fbuf->msg_seqno = msg_seqno;
        fbuf->fragments_remaining = fragments_in_msg;
        fbuf->channellen = channel_sz;
        fbuf->from = *(struct sockaddr_in*)&pkt->from;
        memcpy(fbuf->buf.data, data_start, frag_size);

        --fbuf->fragments_remaining;
        return NULL;
    }

    if (!fbuf) return NULL;
    recvfd.checkAndWarnAboutSmallBuffer(data_size, kernel_rbuf_sz);

    if (fbuf->channellen+1 + fragment_offset + frag_size > fbuf->buf.size) {
        ZCM_DEBUG("dropping invalid fragment (off: %d, %d / %zu)",
                fragment_offset, frag_size, fbuf->buf.size);
        pool.removeFragBuf(fbuf);
        return NULL;
    }

    // copy data
    memcpy(fbuf->buf.data + fbuf->channellen+1 + fragment_offset, data_start, frag_size);

    fbuf->last_packet_utime = pkt->utime;
    if (--fbuf->fragments_remaining > 0)
        return NULL;

    // we've received all the fragments, return a new Message
    Message *msg = pool.allocMessageEmpty();
    msg->utime = fbuf->last_packet_utime;
    msg->channel = fbuf->buf.data;
    msg->channellen = fbuf->channellen;
    msg->data = fbuf->buf.data + fbuf->channellen + 1;
    msg->datalen = fbuf->buf.size - (fbuf->channellen + 1);
    pool.moveBuffer(msg->buf, fbuf->buf);

    // don't need the fragment buffer anymore
    pool.removeFragBuf(fbuf);

    return msg;
}

void UDPM::checkForMessageLoss()
{
    // ISSUE-101 TODO: add this back
    // TODO warn about message loss somewhere else.
    // zuint32_t ring_capacity = ringbuf->get_capacity();
    // zuint32_t ring_used = ringbuf->get_used();

    // zfloat64_t buf_avail = ((zfloat64_t)(ring_capacity - ring_used)) / ring_capacity;
    // if (buf_avail < udp_low_watermark)
    //     udp_low_watermark = buf_avail;

    // zint32_t tm = utimeInSeconds();
    // zint_t elapsedsecs = tm - udp_last_report_secs;
    // if (elapsedsecs > 2) {
    //    if (udp_discarded_bad > 0 || udp_low_watermark < 0.5) {
    //        fprintf(stderr,
    //                "%d ZCM loss %4.1f%% : %5d err, "
    //                "buf avail %4.1f%%\n",
    //                (zint_t) tm,
    //                udp_discarded_bad * 100.0 / (udp_rx + udp_discarded_bad),
    //                udp_discarded_bad,
    //                100.0 * udp_low_watermark);

    //        udp_rx = 0;
    //        udp_discarded_bad = 0;
    //        udp_last_report_secs = tm;
    //        udp_low_watermark = HUGE;
    //    }
    // }
}

// read continuously until a complete message arrives
Message *UDPM::readMessage(zint_t timeout)
{
    Packet *pkt = pool.allocPacket(ZCM_MAX_UNFRAGMENTED_PACKET_SIZE);
    UDPM::checkForMessageLoss();

    Message *msg = NULL;
    while (!msg) {
        // // wait for either incoming UDP data, or for an abort message
        if (!recvfd.waitUntilData(timeout))
            break;

        zint_t sz = recvfd.recvPacket(pkt);
        if (sz < 0) {
            ZCM_DEBUG("udp_read_packet -- recvmsg");
            udp_discarded_bad++;
            continue;
        }

        ZCM_DEBUG("Got packet of size %d", sz);

        if (sz < (zint_t)sizeof(MsgHeaderShort)) {
            // packet too short to be ZCM
            udp_discarded_bad++;
            continue;
        }

        zuint32_t magic = pkt->asHeaderShort()->getMagic();
        if (magic == ZCM_MAGIC_SHORT)
            msg = recvShort(pkt, sz);
        else if (magic == ZCM_MAGIC_LONG)
            msg = recvFragment(pkt, sz);
        else {
            ZCM_DEBUG("ZCM: bad magic");
            udp_discarded_bad++;
            continue;
        }
    }

    pool.freePacket(pkt);
    return msg;
}

zcm_retcode_t UDPM::sendmsg(zcm_msg_t msg)
{
    zint_t channel_size = strlen(msg.channel);
    if (channel_size > ZCM_CHANNEL_MAXLEN) {
        fprintf(stderr, "ZCM Error: channel name too long [%s]\n", msg.channel);
        return ZCM_EINVALID;
    }

    zint_t payload_size = channel_size + 1 + msg.len;
    if (payload_size <= ZCM_SHORT_MESSAGE_MAX_SIZE) {
        // message is short.  send in a single packet

        MsgHeaderShort hdr;
        hdr.setMagic(ZCM_MAGIC_SHORT);
        hdr.setMsgSeqno(msg_seqno);

        zssize_t status = sendfd.sendBuffers(destAddr,
                              (zchar_t*)&hdr, sizeof(hdr),
                              (zchar_t*)msg.channel, channel_size+1,
                              (zchar_t*)msg.buf, msg.len);

        zint_t packet_size = sizeof(hdr) + payload_size;
        ZCM_DEBUG("transmitting %u byte [%s] payload (%d byte pkt)",
                  msg.len, msg.channel, packet_size);
        msg_seqno++;

        if (status == packet_size) return ZCM_EOK;

        if (status >= 0) return ZCM_EAGAIN;

        switch (errno) {
            case EAGAIN:
                return ZCM_EAGAIN;
                break;
            case ENOMEM:
                return ZCM_EMEMORY;
                break;
            default:
                return ZCM_EUNKNOWN;
                break;
        }
    }


    else {
        // message is large.  fragment into multiple packets
        zint_t fragment_size = ZCM_FRAGMENT_MAX_PAYLOAD;
        zint_t nfragments = payload_size / fragment_size +
            !!(payload_size % fragment_size);

        if (nfragments > 65535) {
            fprintf(stderr, "ZCM error: too much data for a single message\n");
            return ZCM_EINVALID;
        }

        // acquire transmit lock so that all fragments are transmitted
        // together, and so that no other message uses the same sequence number
        // (at least until the sequence # rolls over)

        ZCM_DEBUG("transmitting %d byte [%s] payload in %d fragments",
                  payload_size, msg.channel, nfragments);

        zuint32_t fragment_offset = 0;

        MsgHeaderLong hdr;
        hdr.magic = htonl(ZCM_MAGIC_LONG);
        hdr.msg_seqno = htonl(msg_seqno);
        hdr.msg_size = htonl(msg.len);
        hdr.fragment_offset = 0;
        hdr.fragment_no = 0;
        hdr.fragments_in_msg = htons(nfragments);

        // first fragment is special.  insert channel before data
        zsize_t firstfrag_datasize = fragment_size - (channel_size + 1);
        assert(firstfrag_datasize <= msg.len);

        zint_t packet_size = sizeof(hdr) + (channel_size + 1) + firstfrag_datasize;
        fragment_offset += firstfrag_datasize;

        zssize_t status = sendfd.sendBuffers(destAddr,
                                             (zchar_t*)&hdr, sizeof(hdr),
                                             (zchar_t*)msg.channel, channel_size+1,
                                             (zchar_t*)msg.buf, firstfrag_datasize);

        // transmit the rest of the fragments
        for (zuint16_t frag_no = 1; packet_size == status && frag_no < nfragments; frag_no++) {
            hdr.fragment_offset = htonl(fragment_offset);
            hdr.fragment_no = htons(frag_no);

            zint_t fraglen = std::min(fragment_size, (zint_t)msg.len - (zint_t)fragment_offset);
            status = sendfd.sendBuffers(destAddr,
                                        (zchar_t*)&hdr, sizeof(hdr),
                                        (zchar_t*)(msg.buf + fragment_offset), fraglen);

            fragment_offset += fraglen;
            packet_size = sizeof(hdr) + fraglen;
        }

        // sanity check
        if (0 == status) {
            assert(fragment_offset == msg.len);
        }

        msg_seqno++;
    }

    return ZCM_EOK;
}

zcm_retcode_t UDPM::recvmsg(zcm_msg_t *msg, zint32_t timeout)
{
    if (m) pool.freeMessage(m);

    m = readMessage(timeout);
    if (m == nullptr) return ZCM_EAGAIN;

    msg->utime = (zuint64_t) m->utime;
    msg->channel = m->channel;
    msg->len = m->datalen;
    msg->buf = (zuint8_t*) m->data;

    return ZCM_EOK;
}

UDPM::~UDPM()
{
    ZCM_DEBUG("closing zcm context");
}

UDPM::UDPM(const zstring_t& ip, zuint16_t port, zsize_t recv_buf_size, zuint8_t ttl)
    : params(ip, port, recv_buf_size, ttl),
      destAddr(ip, port)
{
}

zbool_t UDPM::init()
{
    ZCM_DEBUG("Initializing ZCM UDPM context...");
    ZCM_DEBUG("Multicast %s:%d", params.ip.c_str(), params.port);
    UDPMSocket::checkConnection(params.ip, params.port);

    sendfd = UDPMSocket::createSendSocket(params.addr, params.ttl);
    if (!sendfd.isOpen()) return zfalse;
    kernel_sbuf_sz = sendfd.getSendBufSize();

    recvfd = UDPMSocket::createRecvSocket(params.addr, params.port);
    if (!recvfd.isOpen()) return zfalse;
    kernel_rbuf_sz = recvfd.getRecvBufSize();

    if (!this->selftest()) {
        // self test failed.  destroy the read thread
        fprintf(stderr, "ZCM self test failed!!\n"
                "Check your routing and firewall settings\n");
        return zfalse;
    }

    return ztrue;
}

zbool_t UDPM::selftest()
{
#ifdef ENABLE_SELFTEST
    ZCM_DEBUG("UDPM conducting self test");
    assert(0 && "unimpl");
#endif
    return ztrue;
}

// Define this the class name you want
#define ZCM_TRANS_CLASSNAME TransportUDPM

struct ZCM_TRANS_CLASSNAME : public zcm_trans_t
{
    UDPM udpm;

    ZCM_TRANS_CLASSNAME(const zstring_t& ip, zuint16_t port, zsize_t recv_buf_size, zuint8_t ttl)
        : udpm(ip, port, recv_buf_size, ttl)
    {
        trans_type = ZCM_BLOCKING;
        vtbl = &methods;
    }

    zbool_t init() { return udpm.init(); }

    /********************** STATICS **********************/
    static zcm_trans_methods_t methods;
    static ZCM_TRANS_CLASSNAME *cast(zcm_trans_t *zt)
    {
        assert(zt->vtbl == &methods);
        return (ZCM_TRANS_CLASSNAME*)zt;
    }

    static zuint32_t _getMtu(zcm_trans_t *zt)
    { return MTU; }

    static zcm_retcode_t _sendmsg(zcm_trans_t *zt, zcm_msg_t msg)
    { return cast(zt)->udpm.sendmsg(msg); }

    static zcm_retcode_t _recvmsgEnable(zcm_trans_t *zt, const zchar_t *channel, zbool_t enable)
    { return ZCM_EOK; }

    static zcm_retcode_t _recvmsg(zcm_trans_t *zt, zcm_msg_t *msg, zint32_t timeout)
    { return cast(zt)->udpm.recvmsg(msg, timeout); }

    static void _destroy(zcm_trans_t *zt)
    { delete cast(zt); }

    static const TransportRegister regUdpm;
};

zcm_trans_methods_t ZCM_TRANS_CLASSNAME::methods = {
    &ZCM_TRANS_CLASSNAME::_getMtu,
    &ZCM_TRANS_CLASSNAME::_sendmsg,
    &ZCM_TRANS_CLASSNAME::_recvmsgEnable,
    &ZCM_TRANS_CLASSNAME::_recvmsg,
    NULL, // update
    &ZCM_TRANS_CLASSNAME::_destroy,
};

static const zchar_t *optFind(zcm_url_opts_t *opts, const zstring_t& key)
{
    for (zsize_t i = 0; i < opts->numopts; i++)
        if (key == opts->name[i])
            return opts->value[i];
    return NULL;
}

static zcm_trans_t *createUdpm(zcm_url_t *url)
{
    auto *ip = zcm_url_address(url);
    vector<zstring_t> parts = StringUtil::split(ip, ':');
    if (parts.size() != 2) {
        ZCM_DEBUG("ERROR: Url format is <ip-address>:<port-num>");
        return nullptr;
    }
    auto& address = parts[0];
    auto& port = parts[1];

    auto *opts = zcm_url_opts(url);
    auto *ttl = optFind(opts, "ttl");
    if (!ttl) {
        ZCM_DEBUG("No ttl specified. Using default ttl=0");
        ttl = "0";
    }
    zsize_t recv_buf_size = 1024;
    auto *trans = new ZCM_TRANS_CLASSNAME(address, atoi(port.c_str()), recv_buf_size, atoi(ttl));
    if (!trans->init()) {
        delete trans;
        return nullptr;
    } else {
        return trans;
    }
}

#ifdef USING_TRANS_UDPM
// Register this transport with ZCM
const TransportRegister ZCM_TRANS_CLASSNAME::regUdpm(
    "udpm", "Transfer data via UDP Multicast (e.g. 'udpm')", createUdpm);
#endif
