#include "mempool.hpp"
#include "udpm.hpp"
#include <cstdlib>
#include <cstring>
#include <climits>

MemPool::MemPool()
{
    memset(sizelists, 0, sizeof(sizelists));
}

MemPool::~MemPool()
{
    for (zsize_t i = 0; i < NUMLISTS; i++) {
        Block *blk = sizelists[i];
        while (blk) {
            auto *next = blk->next;
            std::free(blk);
            blk = next;
        }
    }
}

static bool fitsInU32(zsize_t v)
{
    return (v & 0xffffffff) == v;
}

static zint_t computeSlot(zsize_t v)
{
    // Note: Only works on 32-bit and 64-bit systems
    assert(sizeof(zuint_t) == 4 && CHAR_BIT == 8);
    assert(fitsInU32(v));
    zsize_t bits = 31 - __builtin_clz((zu32)v);
    if ((zsize_t)(1<<bits) != v)
        bits += 1;
    assert((zsize_t)(1<<(bits-1)) < v && v <= (zsize_t)(1<<bits));
    bits = std::max(bits, (zsize_t)16);
    return bits - 16;
}

static zsize_t slotToSize(zint_t slot)
{
    return 1 << (slot+16);
}

char *MemPool::alloc(zsize_t sz)
{
    // This allocator only goes up to 2^28
    assert(sz <= (1<<28));
    assert(fitsInU32(sz));
    zint_t slot = computeSlot(sz);
    assert(0 <= slot && slot < (zint_t)NUMLISTS);

    Block *mem = sizelists[slot];
    if (mem) {
        sizelists[slot] = mem->next;
        return (char*)mem;
    } else {
        return (char*)malloc(slotToSize(slot));
    }
}

void MemPool::free(char *mem, zsize_t sz)
{
    // This allocator only goes up to 2^28
    assert(sz <= (1<<28));
    assert(fitsInU32(sz));
    zint_t slot = computeSlot(sz);
    assert(0 <= slot && slot < (zint_t)NUMLISTS);

    Block *newblock = (Block*)mem;
    newblock->next = sizelists[slot];
    sizelists[slot] = newblock;
}

void MemPool::test()
{
    MemPool pool;

    char *buf = pool.alloc(70000);
    assert(buf);
    pool.free(buf, 70000);
    char *buf2 = pool.alloc(1<<17);
    assert(buf == buf2);
    pool.free(buf2, 1<<17);
    char *buf3 =pool.alloc(1<<18);
    assert(buf3 && buf3 != buf);
    pool.free(buf3, 1<<18);
    char *buf4 = pool.alloc(1<<28);
    assert(buf4);
    pool.free(buf4, 1<<28);
}
