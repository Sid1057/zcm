#include "zcm/transport_registrar.h"
#include <string.h>

#define ZCM_TRANSPORTS_MAX 128
static const zchar_t *t_name[ZCM_TRANSPORTS_MAX];
static const zchar_t *t_desc[ZCM_TRANSPORTS_MAX];
static zcm_trans_create_func *t_creator[ZCM_TRANSPORTS_MAX];
static zsize_t t_index = 0;

zbool_t zcm_transport_register(const zchar_t *name, const zchar_t *desc,
                               zcm_trans_create_func *creator)
{
    if (t_index >= ZCM_TRANSPORTS_MAX)
        return zfalse;

    // Does this name already exist?
    for (zsize_t i = 0; i < t_index; i++)
        if (strcmp(t_name[i], name) == 0)
            return zfalse;

    t_name[t_index] = name;
    t_desc[t_index] = desc;
    t_creator[t_index] = creator;
    t_index++;

    return ztrue;
}

zcm_trans_create_func *zcm_transport_find(const zchar_t *name)
{
    for (zsize_t i = 0; i < t_index; i++)
        if (strcmp(t_name[i], name) == 0)
            return t_creator[i];
    return NULL;
}

void zcm_transport_help(FILE *f)
{
    fprintf(f, "Transport Name       Type            Description\n");
    fprintf(f, "-------------------------------------------------------------------------------------\n");
    for (zsize_t i = 0; i < t_index; i++) {
        const zchar_t *type = "UNKNOWN";
        fprintf(f, "%-20s %-15s %s\n", t_name[i], type, t_desc[i]);
    }
}
