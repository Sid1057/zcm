#include "udpmsocket.hpp"
#include "buffers.hpp"

// Platform specifics
#ifdef WIN32
struct Platform
{
    static void closesocket(zint_t fd) { closesocket(fd); }
    static void setKernelBuffers(zint_t fd)
    {
        // Windows has small (8k) buffer by default
        // increase the send buffer to a reasonable amount.
        zint_t send_size = 256 * 1024;
        zint_t recv_size = 2048 * 1024;

        setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (zchar_t*)&send_size, sizeof(send_size));
        setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (zchar_t*)&recv_size, sizeof(recv_size));
    }

    static zbool_t setMulticastGroup(zint_t fd, struct in_addr multiaddr)
    {
        struct ip_mreq mreq;
        mreq.imr_multiaddr = multiaddr;
        mreq.imr_interface.s_addr = INADDR_ANY;
        ZCM_DEBUG("ZCM: joining multicast group");
        setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (zchar_t*)&mreq, sizeof(mreq));
        // ignore any errors in windows... see issue LCM #60
        return ztrue;
    }

    static void checkRoutingTable(UDPMAddress& addr)
    {
        // UNIMPL
    }
};
#else
struct Platform
{
    static void closesocket(zint_t fd) { close(fd); }
    static void setKernelBuffers(zint_t fd) {}
    static zbool_t setMulticastGroup(zint_t fd, struct in_addr multiaddr)
    {
        struct ip_mreq mreq;
        mreq.imr_multiaddr = multiaddr;
        mreq.imr_interface.s_addr = INADDR_ANY;
        ZCM_DEBUG("ZCM: joining multicast group");
        zint_t ret = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (zchar_t*)&mreq, sizeof(mreq));
        if (ret < 0) {
            perror("setsockopt (IPPROTO_IP, IP_ADD_MEMBERSHIP)");
            return false;
        }
        return ztrue;
    }
    static void checkRoutingTable(UDPMAddress& addr)
    {
#ifdef __linux__
        // TODO
#endif
    }
};
#endif

UDPMSocket::UDPMSocket()
{
}

UDPMSocket::~UDPMSocket()
{
    close();
}

zbool_t UDPMSocket::isOpen()
{
    return fd != -1;
}

void UDPMSocket::close()
{
    if (fd != -1) {
        Platform::closesocket(fd);
        fd = -1;
    }
}

zbool_t UDPMSocket::init()
{
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("allocating ZCM udpm socket");
        return false;
    }
    return ztrue;
}

zbool_t UDPMSocket::joinMulticastGroup(struct in_addr multiaddr)
{
    Platform::setKernelBuffers(fd);

    // Set-up the multicast group
    if (!Platform::setMulticastGroup(fd, multiaddr)) {
        this->close();
        return false;
    }

    return ztrue;
}

zbool_t UDPMSocket::setTTL(zuint8_t ttl)
{
    if (ttl == 0)
        ZCM_DEBUG("ZCM multicast TTL set to 0.  Packets will not leave localhost");

    ZCM_DEBUG("ZCM: setting multicast packet TTL to %d", ttl);
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
                   (zchar_t *) &ttl, sizeof (ttl)) < 0) {
        perror("setsockopt(IPPROTO_IP, IP_MULTICAST_TTL)");
        return false;
    }
    return ztrue;
}

zbool_t UDPMSocket::bindPort(zuint16_t port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return false;
    }
    return ztrue;
}

zbool_t UDPMSocket::setReuseAddr()
{
    // allow other applications on the local machine to also bind to this
    // multicast address and port
    zint_t opt = 1;
    ZCM_DEBUG("ZCM: setting SO_REUSEADDR");
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   (zchar_t*)&opt, sizeof (opt)) < 0) {
        perror ("setsockopt (SOL_SOCKET, SO_REUSEADDR)");
        return false;
    }
    return ztrue;
}

zbool_t UDPMSocket::setReusePort()
{
#ifdef USE_REUSEPORT
    /* Mac OS and FreeBSD require the REUSEPORT option in addition
     * to REUSEADDR or it won't let multiple processes bind to the
     * same port, even if they are using multicast. */
    zint_t opt = 1;
    ZCM_DEBUG("ZCM: setting SO_REUSEPORT");
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                   (zchar_t*)&opt, sizeof (opt)) < 0) {
        perror("setsockopt (SOL_SOCKET, SO_REUSEPORT)");
        return false;
    }
#endif
    return ztrue;
}

zbool_t UDPMSocket::enablePacketTimestamp()
{
    /* Enable per-packet timestamping by the kernel, if available */
#ifdef SO_TIMESTAMP
    zint_t opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &opt, sizeof(opt));
#endif
    return ztrue;
}

zbool_t UDPMSocket::enableLoopback()
{
    // NOTE: For support on SUN Operating Systems, send_lo_opt should be 'zuint8_t'
    //       We don't currently support SUN
    zuint32_t opt = 1;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, (zchar_t *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt (IPPROTO_IP, IP_MULTICAST_LOOP)");
        return false;
    }
    return ztrue;
}

zsize_t UDPMSocket::getRecvBufSize()
{
    zint_t size;
    uint retsize = sizeof(zint_t);
    getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (zchar_t*)&size, (socklen_t *)&retsize);
    ZCM_DEBUG("ZCM: receive buffer is %d bytes", size);
    return size;
}

zsize_t UDPMSocket::getSendBufSize()
{
    zint_t size;
    uint retsize = sizeof(zint_t);
    getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (zchar_t*)&size, (socklen_t *)&retsize);
    ZCM_DEBUG("ZCM: receive buffer is %d bytes", size);
    return size;
}

zbool_t UDPMSocket::waitUntilData(zint_t timeout)
{
    assert(isOpen());

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tm = {
        timeout / 1000,            /* seconds */
        (timeout % 1000) * 1000    /* micros */
    };

    zint_t status = select(fd + 1, &fds, 0, 0, &tm);
    if (status == 0) {
        // timeout
        return false;
    } else if (FD_ISSET(fd, &fds)) {
        // data is available
        return ztrue;
    } else {
        perror("udp_read_packet -- select:");
        return false;
    }
}

zint_t UDPMSocket::recvPacket(Packet *pkt)
{
    struct iovec vec;
    vec.iov_base = pkt->buf.data;
    vec.iov_len = pkt->buf.size;

    struct msghdr msg;
    memset(&msg, 0, sizeof(struct msghdr));
    msg.msg_name = &pkt->from;
    msg.msg_namelen = sizeof(struct sockaddr);
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;

#ifdef MSG_EXT_HDR
    // operating systems that provide SO_TIMESTAMP allow us to obtain more
    // accurate timestamps by having the kernel produce timestamps as soon
    // as packets are received.
    zchar_t controlbuf[64];
    msg.msg_control = controlbuf;
    msg.msg_controllen = sizeof(controlbuf);
    msg.msg_flags = 0;
#endif

    zint_t ret = ::recvmsg(fd, &msg, 0);
    pkt->fromlen = msg.msg_namelen;

    zbool_t got_utime = false;
#ifdef SO_TIMESTAMP
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    /* Get the receive timestamp out of the packet headers if possible */
    while (!pkt->utime && cmsg) {
        if (cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_type == SCM_TIMESTAMP) {
            struct timeval *t = (struct timeval*) CMSG_DATA (cmsg);
            pkt->utime = (zuint64_t) t->tv_sec * 1000000 + t->tv_usec;
            got_utime = ztrue;
            break;
        }
        cmsg = CMSG_NXTHDR(&msg, cmsg);
    }
#endif

    if (!got_utime) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        pkt->utime = (zuint64_t) tv.tv_sec * 1000000 + tv.tv_usec;
    }

    return ret;
}

ssize_t UDPMSocket::sendBuffers(const UDPMAddress& dest, const zchar_t *a, zsize_t alen)
{
    struct iovec iv;
    iv.iov_base = (zchar_t*)a;
    iv.iov_len = alen;;

    struct msghdr mhdr;
    mhdr.msg_name = dest.getAddrPtr();
    mhdr.msg_namelen = dest.getAddrSize();
    mhdr.msg_iov = &iv;
    mhdr.msg_iovlen = 1;
    mhdr.msg_control = NULL;
    mhdr.msg_controllen = 0;
    mhdr.msg_flags = 0;

    return ::sendmsg(fd, &mhdr, 0);
}

ssize_t UDPMSocket::sendBuffers(const UDPMAddress& dest, const zchar_t *a, zsize_t alen,
                                const zchar_t *b, zsize_t blen)
{
    struct iovec iv[2];
    iv[0].iov_base = (zchar_t*)a;
    iv[0].iov_len = alen;;
    iv[1].iov_base = (zchar_t*)b;
    iv[1].iov_len = blen;;

    struct msghdr mhdr;
    mhdr.msg_name = dest.getAddrPtr();
    mhdr.msg_namelen = dest.getAddrSize();
    mhdr.msg_iov = iv;
    mhdr.msg_iovlen = 2;
    mhdr.msg_control = NULL;
    mhdr.msg_controllen = 0;
    mhdr.msg_flags = 0;

    return ::sendmsg(fd, &mhdr, 0);
}

ssize_t UDPMSocket::sendBuffers(const UDPMAddress& dest, const zchar_t *a, zsize_t alen,
                                const zchar_t *b, zsize_t blen, const zchar_t *c, zsize_t clen)
{
    struct iovec iv[3];
    iv[0].iov_base = (zchar_t*)a;
    iv[0].iov_len = alen;;
    iv[1].iov_base = (zchar_t*)b;
    iv[1].iov_len = blen;;
    iv[2].iov_base = (zchar_t*)c;
    iv[2].iov_len = clen;;

    struct msghdr mhdr;
    mhdr.msg_name = dest.getAddrPtr();
    mhdr.msg_namelen = dest.getAddrSize();
    mhdr.msg_iov = iv;
    mhdr.msg_iovlen = 3;
    mhdr.msg_control = NULL;
    mhdr.msg_controllen = 0;
    mhdr.msg_flags = 0;

    return ::sendmsg(fd, &mhdr, 0);
}

zbool_t UDPMSocket::checkConnection(const string& ip, zuint16_t port)
{
    UDPMAddress addr{ip, port};
    SOCKET testfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (connect(testfd, addr.getAddrPtr(), addr.getAddrSize()) < 0) {
        perror ("connect");
        Platform::checkRoutingTable(addr);
        return false;
    }
    Platform::closesocket(testfd);
    return ztrue;
}

void UDPMSocket::checkAndWarnAboutSmallBuffer(zsize_t datalen, zsize_t kbufsize)
{
    // TODO: This should probably be in Platform
#ifdef __linux__
    if (warnedAboutSmallBuffer)
        return;

    const zsize_t MIN_KBUF_SIZE = (1<<18)+1;
    if (kbufsize < MIN_KBUF_SIZE && datalen > kbufsize) {
        warnedAboutSmallBuffer = ztrue;
        fprintf(stderr,
                "==== ZCM Warning ===\n"
                "ZCM detected that large packets are being received, but the kernel UDP\n"
                "receive buffer is very small. The possibility of dropping packets due to\n"
                "insufficient buffer space is very high.\n");
    }
#endif
}

UDPMSocket UDPMSocket::createSendSocket(struct in_addr multiaddr, zuint8_t ttl)
{
    // don't use connect() on the actual transmit socket, because linux then
    // has problems multicasting to localhost
    UDPMSocket sock;
    if (!sock.init())                        { sock.close(); return sock; }
    if (!sock.setTTL(ttl))                   { sock.close(); return sock; }
    if (!sock.enableLoopback())              { sock.close(); return sock; }
    if (!sock.joinMulticastGroup(multiaddr)) { sock.close(); return sock; }
    return sock;
}

UDPMSocket UDPMSocket::createRecvSocket(struct in_addr multiaddr, zuint16_t port)
{
    UDPMSocket sock;
    if (!sock.init())                        { sock.close(); return sock; }
    if (!sock.setReuseAddr())                { sock.close(); return sock; }
    if (!sock.setReusePort())                { sock.close(); return sock; }
    if (!sock.enablePacketTimestamp())       { sock.close(); return sock; }
    if (!sock.bindPort(port))                { sock.close(); return sock; }
    if (!sock.joinMulticastGroup(multiaddr)) { sock.close(); return sock; }
    return sock;
}
