#pragma once

#include "udpm.hpp"
#include "mempool.hpp"

/************************* Packet Headers *******************/

struct MsgHeaderShort
{
    // Layout
  private:
    // NOTE: These are set to private only because they are in network format.
    //       Thus, they are not safe to access directly.
    zu32 magic;
    zu32 msg_seqno;

    // Converted data
  public:
    zu32  getMagic()         { return ntohl(magic); }
    void setMagic(zu32 v)    { magic = htonl(v); }
    zu32  getMsgSeqno()      { return ntohl(msg_seqno); }
    void setMsgSeqno(zu32 v) { msg_seqno = htonl(v); }

    // Computed data
  public:
    // Note: Channel starts after the header
    const zchar_t *getChannelPtr() { return (zchar_t*)(this+1); }
    zsize_t getChannelLen() { return strlen(getChannelPtr()); }

    // Note: Data starts after the channel and null
    zchar_t *getDataPtr() { return (zchar_t*)getChannelPtr() + getChannelLen() + 1; }
    zsize_t getDataOffset() { return sizeof(*this) + getChannelLen() + 1; }
    zsize_t getDataLen(zsize_t pktsz) { return pktsz - getDataOffset(); }
};

struct MsgHeaderLong
{
    // Layout
  // TODO: make this private
  //private:
    zu32 magic;
    zu32 msg_seqno;
    zu32 msg_size;
    zu32 fragment_offset;
    zu16 fragment_no;
    zu16 fragments_in_msg;

    // Converted data
  public:
    zu32 getMagic()          { return ntohl(magic); }
    zu32 getMsgSeqno()       { return ntohl(msg_seqno); }
    zu32 getMsgSize()        { return ntohl(msg_size); }
    zu32 getFragmentOffset() { return ntohl(fragment_offset); }
    zu16 getFragmentNo()     { return ntohs(fragment_no); }
    zu16 getFragmentsInMsg() { return ntohs(fragments_in_msg); }

    // Computed data
  public:
    zu32 getFragmentSize(zsize_t pktsz) { return pktsz - sizeof(*this); }
    zchar_t *getDataPtr() { return (zchar_t*)(this+1); }
};

// if fragment_no == 0, then header is immediately followed by NULL-terminated
// ASCII-encoded channel name, followed by the payload data
// if fragment_no > 0, then header is immediately followed by the payload data

/******************** message buffer **********************/
struct Buffer
{
    zchar_t *data = nullptr;
    zsize_t size = 0;

    Buffer(){}
  private:
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

  public:
    Buffer(Buffer&& o) : data(o.data), size(o.size)
    {
        o.data = nullptr;
    }

    Buffer& operator=(Buffer&& other)
    {
        assert(this->data == nullptr &&
               "Error: Buffer MUST be deallocated before a move can occur");
        this->data = other.data;
        this->size = other.size;
        other.data = nullptr;
        return *this;
    }
};

struct Message
{
    zi64               utime;       // timestamp of first datagram receipt

    const zchar_t     *channel;     // points into 'buf'
    zsize_t            channellen;  // length of channel

    zchar_t           *data;        // points into 'buf'
    zsize_t            datalen;     // length of data

    // Backing store buffer that contains the actual data
    Buffer buf;

    Message() { memset(this, 0, sizeof(*this)); }
};

struct Packet
{
    zi64             utime;      // timestamp of first datagram receipt
    zsize_t          sz;         // size received

    struct sockaddr from;       // sender
    socklen_t       fromlen;

    // Backing store buffer that contains the actual data
    Buffer          buf;

    Packet() { memset(this, 0, sizeof(*this)); }
    MsgHeaderShort *asHeaderShort() { return (MsgHeaderShort*)buf.data; }
    MsgHeaderLong  *asHeaderLong()  { return (MsgHeaderLong* )buf.data; }
};

/******************** fragment buffer **********************/
struct FragBuf
{
    zi64     last_packet_utime;
    zu32     msg_seqno;
    zu16     fragments_remaining;

    // The channel starts at the beginning of the buffer. The data
    // follows immediately after the channel and its NULL
    zsize_t  channellen;
    struct sockaddr_in from;

    // Fields set by the allocator object
    Buffer buf;

    bool matchesSockaddr(struct sockaddr_in *addr);
};

/************** A pool to handle every alloc/dealloc operation on Message objects ******/
struct MessagePool
{
    MessagePool(zsize_t maxSize, zsize_t maxBuffers);
    ~MessagePool();

    // Buffer
    Buffer allocBuffer(zsize_t sz);
    void freeBuffer(Buffer& buf);

    // Packet
    Packet *allocPacket(zsize_t maxsz);
    void freePacket(Packet *p);

    // Message
    Message *allocMessage();
    Message *allocMessageEmpty();
    void freeMessage(Message *b);

    // FragBuf
    FragBuf *addFragBuf(zu32 data_size);
    FragBuf *lookupFragBuf(struct sockaddr_in *key);
    void removeFragBuf(FragBuf *fbuf);

    void transferBufffer(Message *to, FragBuf *from);
    void moveBuffer(Buffer& to, Buffer& from);

  private:
    void _freeMessageBuffer(Message *b);
    void _removeFragBuf(zsize_t index);

  private:
    MemPool mempool;
    vector<FragBuf*> fragbufs;
    zsize_t maxSize;
    zsize_t maxBuffers;
    zsize_t totalSize = 0;
};
