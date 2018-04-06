/*
 * $Id: graph_dfs.c,v 1.9 2005/04/15 23:24:21 fabio Exp $
 */

#include "graph_int.h"

static vertex_t *
find_an_end_vertex(vertex_t *v, st_table *visit_list)
{
    edge_t *e;
    vertex_t *dest;
    lsGen gen;

    if (lsLength(g_get_out_edges(v)) == 0) {
        return(v);
    }
    foreach_out_edge (v,gen,e) {
	(void) lsFinish(gen);
	dest = g_e_dest(e);
        if (st_insert(visit_list,(char *) dest,(char *) 0) == 1) {
	    return(NIL(vertex_t));
	}
	return(find_an_end_vertex(dest,visit_list));
        /* NOTREACHED */
    }
    /* no free out_edges */
    return(NIL(vertex_t));
}

static int
dfs_recurr(vertex_t *v, st_table *dfs_list, array_t *dfs_array) 
{
    edge_t *e;
    lsGen gen;
    int val;

    if (st_lookup_int(dfs_list,(char *) v, &val)) {
        return(val == 0);
    }
    (void) st_insert(dfs_list,(char *) v,(char *) 1);

    foreach_in_edge (v,gen,e) {
        if (!dfs_recurr(g_e_source(e),dfs_list,dfs_array)) {
	    return(0);
	}
    }
    (void) st_insert(dfs_list,(char *) v,(char *) 0);
    array_insert_last(vertex_t *,dfs_array,v);
    return(1);
}

static array_t *
g_dfs_int(graph_t *g)
{
    vertex_t *v;
    lsGen gen;
    array_t *dfs_array;
    st_table *visit_list,*dfs_list;
    int cycle = FALSE;

    dfs_array = array_alloc(vertex_t *,0);
    visit_list = st_init_table(st_ptrcmp,st_ptrhash);
    dfs_list = st_init_table(st_ptrcmp,st_ptrhash);

    foreach_vertex (g,gen,v) {
        if (!st_is_member(dfs_list,(char *) v)) {
	    (void) st_insert(visit_list,(char *) v,(char *) 0);
	    v = find_an_end_vertex(v,visit_list);
	    if (v == NIL(vertex_t) || !dfs_recurr(v,dfs_list,dfs_array)) {
	        cycle = TRUE;
		(void) lsFinish(gen);
		break;
	    }
	}
    }

    st_free_table(visit_list);
    st_free_table(dfs_list);
    if (cycle == TRUE) {
        array_free(dfs_array);
        return(NIL(array_t));
    }
    return(dfs_array);
}

array_t *
g_dfs(graph_t *g)
{
    array_t *x;

    x = g_dfs_int(g);
    if (x == NIL(array_t)) {
        fail("g_dfs: Graph has cycle");
    }
    return(x);
}

int
g_is_acyclic(graph_t *g)
{
    array_t *x;

    x = g_dfs_int(g);
    if (x) {
        array_free(x);
	return(TRUE);
    }
    return(FALSE);
}


static int
reachable_recurr(vertex_t *v, st_table *dfs_list)
{
    edge_t *e;
    lsGen gen;
    int val;

    if (st_lookup_int(dfs_list,(char *) v, &val)) {
        return(val == 0);
    }
    (void) st_insert(dfs_list,(char *) v,(char *) 1);

    foreach_out_edge (v,gen,e) {
        reachable_recurr(g_e_dest(e),dfs_list);
    }
    (void) st_insert(dfs_list,(char *) v,(char *) 0);

    return(1);
}

/* compute reachable states from the initial states */
st_table *
g_reachable(graph_t *g, st_table *init)
{
    vertex_t *v;
    st_table *dfs_list;
    st_generator *stgen;

    dfs_list = st_init_table(st_ptrcmp, st_ptrhash);

    st_foreach_item(init, stgen, &v, NIL(char *)) {
	reachable_recurr(v,dfs_list);
    }

    return (dfs_list);
}


static int
EF_recurr(vertex_t *v, st_table *EF_list)
{
    edge_t *e;
    lsGen gen;
    int val;

    if (st_lookup_int(EF_list,(char *) v, &val)) {
        return(val == 0);
    }
    (void) st_insert(EF_list,(char *) v,(char *) 1);

    foreach_in_edge (v,gen,e) {
        EF_recurr(g_e_source(e),EF_list);
    }
    (void) st_insert(EF_list,(char *) v,(char *) 0);

    return(1);
}

/* compute EF(goal) of an automaton, might include unreachable states */
st_table *
g_EF(graph_t *g, st_table *goal)
{
    vertex_t *v;
    st_table *EF_list;
    st_generator *stgen;

    EF_list = st_init_table(st_ptrcmp, st_ptrhash);

    st_foreach_item(goal, stgen, &v, NIL(char *)) {
	EF_recurr(v,EF_list);
    }

    return (EF_list);
}

static void
searchSCC(
  vertex_t *v,
  st_table *scc,
  st_table *old /*visited*/,
  lsList   stack,
  st_table *onstack,
  int *countr)
{
    int lowlink_v, dfnumber_v, lowlink_w, dfnumber_w;
    vertex_t *x, *w;
    st_table *component;
    lsGen gen;
    edge_t *e;

    lowlink_v = dfnumber_v = *countr;
    (*countr)++;
    st_insert(old, (char *)v, (char *)(long)dfnumber_v);
    st_insert(onstack, (char *)v, (char *)(long)lowlink_v);
    lsNewBegin(stack, (lsGeneric)v, NIL(lsHandle));

    foreach_out_edge (v,gen,e) {
	w = g_e_dest(e);
	if (!st_is_member(old, (char *)w)) {
	    searchSCC(w,scc,old,stack,onstack,countr);
	    if(st_lookup_int(onstack, (char *)w, &lowlink_w) &&
	       lowlink_w < lowlink_v) {
		lowlink_v = lowlink_w;
		st_insert(onstack, (char *)v, (char *)(long)lowlink_v);
	    }
	}else {
	    st_lookup_int(old, (char *)w, &dfnumber_w);
	    if (dfnumber_w < dfnumber_v && st_is_member(onstack, (char *)w)) {
		if (dfnumber_w < lowlink_v) {
		    lowlink_v = dfnumber_w;
		    st_insert(onstack, (char *)v, (char *)(long)lowlink_v);
		}
	    }
	}
    }

    /* put current SCC into st_table, and then insert into 'scc' */
    if (dfnumber_v == lowlink_v) {
	component = st_init_table(st_ptrcmp, st_ptrhash);
	while (lsDelBegin(stack, &x) != LS_NOMORE) {
	    st_insert(component, (char *)x, NIL(char)); 
	    st_delete(onstack, &x, NIL(char *));
	    if (v == x) break;
	}
	st_insert(scc, (char *)component, NIL(char)); /* is component fair? */
    }
}

/* compute the strongly connected components of a graph (Tarjan's alg.) */
st_table *
g_SCC(graph_t *g, st_table *init)
{
    vertex_t *v;
    lsList stack;
    st_table *old, *onstack, *scc;
    st_generator *stgen;
    int countr = 0;

    scc = st_init_table(st_ptrcmp, st_ptrhash);
    
    stack = lsCreate();
    old = st_init_table(st_ptrcmp, st_ptrhash);
    onstack = st_init_table(st_ptrcmp, st_ptrhash);
    st_foreach_item(init, stgen, &v, NIL(char *)) {
	if (!st_is_member(old, (char *)v))
	    searchSCC(v,scc,old,stack,onstack,&countr);
    }
    lsDestroy(stack, (void (*)(lsGeneric))0 );
    st_free_table(old);
    st_free_table(onstack);

    return scc;
}
