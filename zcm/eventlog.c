#include "zcm/eventlog.h"
#include "zcm/util/ioutils.h"
#include <assert.h>
#include <string.h>

#define freadu32(f, x) fread32((f), (zint32_t*) (x))
#define freadu64(f, x) fread64((f), (zint64_t*) (x))

#define MAGIC ((zuint32_t) 0xEDA1DA01L)

zcm_eventlog_t *zcm_eventlog_create(const zchar_t *path, const zchar_t *mode)
{
    assert(!strcmp(mode, "r") || !strcmp(mode, "w") || !strcmp(mode, "a"));
    if(*mode == 'w')
        mode = "wb";
    else if(*mode == 'r')
        mode = "rb";
    else if(*mode == 'a')
        mode = "ab";
    else
        return NULL;

    zcm_eventlog_t *l = (zcm_eventlog_t*) zcm_calloc(1, sizeof(zcm_eventlog_t));

    l->f = fopen(path, mode);
    if (!l->f) {
        zcm_free(l);
        return NULL;
    }

    l->eventcount = 0;

    return l;
}

void zcm_eventlog_destroy(zcm_eventlog_t *l)
{
    fflush(l->f);
    fclose(l->f);
    zcm_free(l);
}

FILE *zcm_eventlog_get_fileptr(zcm_eventlog_t *l)
{
    return l->f;
}

static zbool_t sync_stream(zcm_eventlog_t *l)
{
    zuint32_t magic = 0;
    zint32_t r;
    do {
        r = fgetc(l->f);
        if (r < 0) return zfalse;
        magic = (magic << 8) | r;
    } while(magic != MAGIC);
    return ztrue;
}

static zbool_t sync_stream_backwards(zcm_eventlog_t *l)
{
    zuint32_t magic = 0;
    zint32_t r;
    do {
        if (ftello (l->f) < 2) return zfalse;
        fseeko (l->f, -2, SEEK_CUR);
        r = fgetc(l->f);
        if (r < 0) return zfalse;
        magic = ((magic >> 8) & 0x00ffffff) | (((zuint32_t)r << 24) & 0xff000000);
    } while(magic != MAGIC);
    fseeko (l->f, sizeof(zuint32_t) - 1, SEEK_CUR);
    return ztrue;
}

static zcm_retcode_t get_next_event_time(zuint64_t* timestamp, zcm_eventlog_t *l)
{
    if (sync_stream(l)) return ZCM_EOF;

    zint64_t event_num;
    if (0 != freadu64(l->f, &event_num)) return ZCM_EOF;
    if (0 != freadu64(l->f,  timestamp)) return ZCM_EOF;
    fseeko(l->f, -(sizeof(zint64_t) * 2 + sizeof(zint32_t)), SEEK_CUR);

    l->eventcount = event_num;

    return ZCM_EOK;
}

zcm_retcode_t zcm_eventlog_seek_to_timestamp(zcm_eventlog_t *l, zuint64_t timestamp)
{
    fseeko (l->f, 0, SEEK_END);
    zoff_t file_len = ftello(l->f);

    zuint64_t cur_time;
    zfloat64_t frac1 = 0;               // left bracket
    zfloat64_t frac2 = 1;               // right bracket
    zfloat64_t prev_frac = -1;
    zfloat64_t frac;                    // current position

    while (1) {
        frac = 0.5 * (frac1 + frac2);
        zoff_t offset = (zoff_t)(frac * file_len);
        fseeko(l->f, offset, SEEK_SET);
        zcm_retcode_t ret;
        if ((ret = get_next_event_time(&cur_time, l)) != ZCM_EOK)
            return ret;

        if ((frac > frac2) || (frac < frac1) || (frac1>=frac2))
            break;

        zfloat64_t df = frac-prev_frac;
        if (df < 0)
            df = -df;
        if (df < 1e-12)
            break;

        if (cur_time == timestamp)
            break;

        if (cur_time < timestamp)
            frac1 = frac;
        else
            frac2 = frac;

        prev_frac = frac;
    }

    return ZCM_EOK;
}

static zcm_eventlog_event_t *zcm_event_read_helper(zcm_eventlog_t *l, zbool_t rewindWhenDone)
{
    zcm_eventlog_event_t *le =
        (zcm_eventlog_event_t*) zcm_calloc(1, sizeof(zcm_eventlog_event_t));

    if (0 != freadu64(l->f, &le->eventnum) ||
        0 != freadu64(l->f, &le->timestamp) ||
        0 != freadu32(l->f, &le->channellen) ||
        0 != freadu32(l->f, &le->datalen)) {
        zcm_free(le);
        return NULL;
    }

    // Sanity check the channel length and data length
    if (le->channellen == 0 || le->channellen >= ZCM_CHANNEL_MAXLEN) {
        fprintf(stderr, "Log event has invalid channel length: %d\n", le->channellen);
        zcm_free(le);
        return NULL;
    }
    if (le->datalen == 0) {
        fprintf(stderr, "Log event has invalid data length: %d\n", le->datalen);
        zcm_free(le);
        return NULL;
    }

    le->channel = (zchar_t *) zcm_calloc(sizeof(zchar_t), le->channellen + 1);
    if (fread(le->channel, 1, le->channellen, l->f) != le->channellen) {
        zcm_free(le->channel);
        zcm_free(le);
        return NULL;
    }

    le->data = zcm_calloc(sizeof(zuint8_t), le->datalen + 1);
    if (fread(le->data, 1, le->datalen, l->f) != le->datalen) {
        zcm_free(le->channel);
        zcm_free(le->data);
        zcm_free(le);
        return NULL;
    }

    // Check that there's a valid event or the EOF after this event.
    zuint32_t next_magic;
    if (0 == freadu32(l->f, &next_magic)) {
        if (next_magic != MAGIC) {
            fprintf(stderr, "Invalid header after log data\n");
            zcm_free(le->channel);
            zcm_free(le->data);
            zcm_free(le);
            return NULL;
        }
        fseeko (l->f, -4, SEEK_CUR);
    }
    if (rewindWhenDone) {
        fseeko (l->f, -(sizeof(zint64_t) * 2 + sizeof(zint32_t) * 3 +
                        le->datalen + le->channellen), SEEK_CUR);
    }
    return le;
}

zcm_eventlog_event_t *zcm_eventlog_read_next_event(zcm_eventlog_t *l)
{
    if (sync_stream(l)) return NULL;
    return zcm_event_read_helper(l, 0);
}

zcm_eventlog_event_t *zcm_eventlog_read_prev_event(zcm_eventlog_t *l)
{
    if (!sync_stream_backwards(l)) return NULL;
    return zcm_event_read_helper(l, 1);
}

zcm_eventlog_event_t *zcm_eventlog_read_event_at_offset(zcm_eventlog_t *l, zoff_t offset)
{
    fseeko(l->f, offset, SEEK_SET);
    if (sync_stream(l)) return NULL;
    return zcm_event_read_helper(l, 0);
}

void zcm_eventlog_free_event(zcm_eventlog_event_t *le)
{
    if (le->data) zcm_free(le->data);
    if (le->channel) zcm_free(le->channel);
    memset(le, 0, sizeof(zcm_eventlog_event_t));
    zcm_free(le);
}

zbool_t zcm_eventlog_write_event(zcm_eventlog_t *l, const zcm_eventlog_event_t *le)
{
    if (0 != fwrite32(l->f, MAGIC)) return zfalse;

    if (0 != fwrite64(l->f, l->eventcount)) return zfalse;

    if (0 != fwrite64(l->f, le->timestamp)) return zfalse;
    if (0 != fwrite32(l->f, le->channellen)) return zfalse;
    if (0 != fwrite32(l->f, le->datalen)) return zfalse;

    if (le->channellen != fwrite(le->channel, sizeof(zchar_t), le->channellen, l->f))
        return zfalse;
    if (le->datalen != fwrite(le->data, sizeof(zuint8_t), le->datalen, l->f))
        return zfalse;

    l->eventcount++;

    return ztrue;
}
