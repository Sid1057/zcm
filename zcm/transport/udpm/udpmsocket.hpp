#pragma once
#include "udpm.hpp"
#include "buffers.hpp"

class UDPMAddress
{
  public:
    UDPMAddress(const zstring_t& ip, zu16 port)
    {
        this->ip = ip;
        this->port = port;

        memset(&this->addr, 0, sizeof(this->addr));
        this->addr.sin_family = AF_INET;
        inet_aton(ip.c_str(), &this->addr.sin_addr);
        this->addr.sin_port = htons(port);
    }

    const zstring_t& getIP() const { return ip; }
    zu16 getPort() const { return port; }
    struct sockaddr* getAddrPtr() const { return (struct sockaddr*)&addr; }
    zsize_t getAddrSize() const { return sizeof(addr); }

  private:
    zstring_t ip;
    zu16 port;
    struct sockaddr_in addr;
};

class UDPMSocket
{
  public:
    UDPMSocket();
    ~UDPMSocket();
    zbool_t isOpen();
    void close();

    zbool_t init();
    zbool_t joinMulticastGroup(struct in_addr multiaddr);
    zbool_t setTTL(zu8 ttl);
    zbool_t bindPort(zu16 port);
    zbool_t setReuseAddr();
    zbool_t setReusePort();
    zbool_t enablePacketTimestamp();
    zbool_t enableLoopback();
    zbool_t setDestination(const zstring_t& ip, zu16 port);

    zsize_t getRecvBufSize();
    zsize_t getSendBufSize();

    // Returns true when there is a packet available for receiving
    zbool_t waitUntilData(zint_t timeout);
    zint_t recvPacket(Packet *pkt);

    zssize_t sendBuffers(const UDPMAddress& dest, const zchar_t *a, zsize_t alen);
    zssize_t sendBuffers(const UDPMAddress& dest, const zchar_t *a, zsize_t alen,
                         const zchar_t *b, zsize_t blen);
    zssize_t sendBuffers(const UDPMAddress& dest, const zchar_t *a, zsize_t alen,
                         const zchar_t *b, zsize_t blen, const zchar_t *c, zsize_t clen);

    static zbool_t checkConnection(const zstring_t& ip, zu16 port);
    void checkAndWarnAboutSmallBuffer(zsize_t datalen, zsize_t kbufsize);

    static UDPMSocket createSendSocket(struct in_addr multiaddr, zu8 ttl);
    static UDPMSocket createRecvSocket(struct in_addr multiaddr, zu16 port);

  private:
    SOCKET fd = -1;
    zbool_t warnedAboutSmallBuffer = false;

  private:
    // Disallow copies
    UDPMSocket(const UDPMSocket&) = delete;
    UDPMSocket& operator=(const UDPMSocket&) = delete;

  public:
    // Allow moves
    UDPMSocket(UDPMSocket&& other) { std::swap(this->fd, other.fd); }
    UDPMSocket& operator=(UDPMSocket&& other) { std::swap(this->fd, other.fd); return *this; }
};
