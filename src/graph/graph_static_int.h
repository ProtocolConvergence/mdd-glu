/*
 * $Id: graph_static_int.h,v 1.2 2002/08/27 15:29:01 fabio Exp $
 *
 */

#include "graph_static.h"

typedef struct g_field_struct {
    int num_g_slots;
    int num_v_slots;
    int num_e_slots;
    gGeneric user_data;
} g_field_t;
