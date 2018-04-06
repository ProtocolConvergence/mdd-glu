/*
 * $Id: graph_static.h,v 1.4 2012/02/04 23:54:57 fabio Exp $
 *
 */

/******************************* graph_static.h ************************/

EXTERN graph_t *g_alloc_static(int, int, int);
EXTERN void g_free_static(graph_t *, void (*)(gGeneric), void (*)(gGeneric), void (*)(gGeneric));
EXTERN graph_t *g_dup_static(graph_t *, char *(*)(gGeneric), char *(*)(gGeneric), char *(*)(gGeneric));
EXTERN void g_set_g_slot_static(graph_t *, int, char *);
EXTERN char *g_get_g_slot_static(graph_t *, int);
EXTERN void g_copy_g_slots_static(graph_t *, graph_t *, char *(*)(gGeneric));
EXTERN vertex_t *g_add_vertex_static(graph_t *);
EXTERN void g_delete_vertex_static(vertex_t *, void (*)(gGeneric), void (*)(gGeneric));
EXTERN void g_set_v_slot_static(vertex_t *, int, char *);
EXTERN char *g_get_v_slot_static(vertex_t *, int);
EXTERN void g_copy_v_slots_static(vertex_t *, vertex_t *, char *(*)(gGeneric));
EXTERN edge_t *g_add_edge_static(vertex_t *, vertex_t *);
EXTERN void g_delete_edge_static(edge_t *, void (*)(gGeneric));
EXTERN void g_set_e_slot_static(edge_t *, int, char *);
EXTERN char *g_get_e_slot_static(edge_t *, int);
EXTERN void g_copy_e_slots_static(edge_t *, edge_t *, char *(*)(gGeneric));
