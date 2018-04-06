#ifndef GRAPH_H
#define GRAPH_H

typedef char *gGeneric;

typedef struct graph_struct {
	gGeneric user_data;
} graph_t;

typedef struct vertex_struct {
	gGeneric user_data;
} vertex_t;

typedef struct edge_struct {
	gGeneric user_data;
} edge_t;

typedef void (*GRAPH_PFV)(gGeneric);
typedef gGeneric (*GRAPH_PFG)(gGeneric);

EXTERN graph_t *g_alloc(void);
EXTERN void g_free(graph_t *, void(*)(gGeneric), void(*)(gGeneric), void(*)(gGeneric));
EXTERN void g_check(graph_t *);
EXTERN graph_t *g_dup(graph_t *, gGeneric(*)(gGeneric), gGeneric(*)(gGeneric), gGeneric(*)(gGeneric));

EXTERN lsList g_get_vertices(graph_t *);

#define foreach_vertex(g, lgen, v)				\
	for (lgen = lsStart(g_get_vertices(g));			\
		lsNext(lgen, &v, LS_NH) == LS_OK	\
		   || ((void) lsFinish(lgen), 0); )

#define foreach_edge(g, lgen, e)				\
	for (lgen = lsStart(g_get_edges(g));			\
		lsNext(lgen, &e, LS_NH) == LS_OK	\
		   || ((void) lsFinish(lgen), 0); )

EXTERN vertex_t *g_add_vertex(graph_t *);
EXTERN void g_delete_vertex(vertex_t *, void (*)(gGeneric), void (*)(gGeneric));
EXTERN graph_t *g_vertex_graph(vertex_t *);

EXTERN lsList g_get_edges(graph_t *);
EXTERN lsList g_get_in_edges(vertex_t *);
EXTERN lsList g_get_out_edges(vertex_t *);

#define foreach_in_edge(v, lgen, e)				\
	for (lgen = lsStart(g_get_in_edges(v));			\
		lsNext(lgen, &e, LS_NH) == LS_OK	\
		   || ((void) lsFinish(lgen), 0); )

#define foreach_out_edge(v, lgen, e)				\
	for (lgen = lsStart(g_get_out_edges(v));		\
		lsNext(lgen, &e, LS_NH) == LS_OK	\
		   ||  ((void) lsFinish(lgen), 0); )

EXTERN edge_t *g_add_edge(vertex_t *, vertex_t *);
EXTERN void g_delete_edge(edge_t *, void (*)(gGeneric));
EXTERN graph_t *g_edge_graph(edge_t *);
EXTERN vertex_t *g_e_source(edge_t *);
EXTERN vertex_t *g_e_dest(edge_t *);

EXTERN array_t *g_dfs(graph_t *);
EXTERN int g_is_acyclic(graph_t *);
EXTERN array_t *g_graph_sort(graph_t *, int (*)(const void *, const void *));

EXTERN st_table *g_reachable(graph_t *, st_table *);
EXTERN st_table *g_EF(graph_t *, st_table *);
EXTERN st_table *g_SCC(graph_t *, st_table *);

#endif
