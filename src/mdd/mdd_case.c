#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_case.c,v 1.10 2002/08/24 20:44:27 fabio Exp $
 * 
 * Author: Timothy Kam
 *
 * Copyright 1992 by the Regents of the University of California.
 *
 * All rights reserved.  Permission to use, copy, modify and distribute
 * this software is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.  This software
 * is made available as is, with no warranties.
 */

mdd_t *
mdd_case(
  mdd_manager *mgr,
  int mvar,
  array_t *child_list)
{
    mvar_type mv;
    mdd_t *mnode;
    mdd_t *tmp;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);

    mv = array_fetch(mvar_type, mvar_list, mvar);

    if (mv.values != array_n(child_list)) 
	fail("mdd_case: mvar.values different from length of child_list\n");

    if (mv.status == MDD_BUNDLED) {
	(void) fprintf(stderr, 
	"\nmdd_andsmooth: bundled variable %s used\n",mv.name);
	fail("");
    }

    if (mv.values == 1) {
	tmp = array_fetch(mdd_t *, child_list, 0);
	mnode = mdd_dup(tmp);
    }
    else {
        mnode = mdd_encode(mgr, child_list, &mv, mv.encode_length-1);
    }
    return mnode;
}


mdd_t *
mdd_encode(
  mdd_manager *mgr,
  array_t *child_list,
  mvar_type *mv_ptr,
  int index)
{
    array_t *new_child_list;
    int i;
    int child_count = 0;
    int q = array_n(child_list);
    bvar_type bv;
    mdd_t *f, *g, *h, *t;
    mdd_t *one, *zero;
    array_t *bvar_list;

    if (q == 1) {
        f =  array_fetch(mdd_t *, child_list, 0);
	h = mdd_dup(f);
	if (!mdd_is_tautology(f,1) && !mdd_is_tautology(f,0)) mdd_free(f);
        return h;
    }
    one = mdd_one(mgr);
    zero = mdd_zero(mgr);
    bvar_list = mdd_ret_bvar_list(mgr);

    new_child_list = array_alloc(mdd_t *, 0);

    bv = mdd_ret_bvar(mv_ptr, index, bvar_list);

    for (i=0; i<(q/2); i++) {

	f = mdd_dup(bv.node);
        h = array_fetch(mdd_t *, child_list, child_count++);
        g = array_fetch(mdd_t *, child_list, child_count++);
#if USE_ITE
#if BYPASS
	/* bypasses cases 	*/
	/* 1  = ite(F,1,1)	*/
	/* 0  = ite(F,0,0)	*/
	/* F  = ite(F,1,0)	*/
	/* !F = ite(F,0,1)	*/
	/* G  = ite(F,G,G)	*/
    	if (mdd_is_tautology(g,0) && mdd_is_tautology(h,0)) {
	    array_insert_last(mdd_t *, new_child_list, zero);
	}
    	else if (mdd_is_tautology(g,0) && mdd_is_tautology(h,1)) {
	    t = mdd_not(f);
	    array_insert_last(mdd_t *, new_child_list, t);
	}
	else if (mdd_is_tautology(g,1) && mdd_is_tautology(h,1)) {
	    array_insert_last(mdd_t *, new_child_list, one);
	}
	else if (mdd_is_tautology(g,1) && mdd_is_tautology(h,0)) {
	    t = mdd_dup(f);
	    array_insert_last(mdd_t *, new_child_list, t);
	}
	else if (mdd_equal(f,g)) {
	    t = mdd_dup(f);
	    array_insert_last(mdd_t *, new_child_list, t);
	}
	else {
    	    t = mdd_ite(f, g, h, 1, 1, 1);
	    array_insert_last(mdd_t *, new_child_list, t);
	}
	if (!mdd_is_tautology(g,1) && !mdd_is_tautology(g,0)) mdd_free(g);
	if (!mdd_is_tautology(h,1) && !mdd_is_tautology(h,0)) mdd_free(h);
#else
	t = mdd_ite(f, g, h, 1, 1, 1);
	if (!mdd_is_tautology(g,1) && !mdd_is_tautology(g,0)) mdd_free(g);
	if (!mdd_is_tautology(h,1) && !mdd_is_tautology(h,0)) mdd_free(h);
	array_insert_last(mdd_t *, new_child_list, t);
#endif
#else
	a1 = mdd_and(f,g,1,1);
	if (!mdd_is_tautology(g,1) && !mdd_is_tautology(g,0)) mdd_free(g);
	a2 = mdd_and(f,h,0,1);
	if (!mdd_is_tautology(h,1) && !mdd_is_tautology(h,0)) mdd_free(h);
	t = mdd_or(a1,a2,1,1);
	/* t = mdd_or(mdd_and(f,g,1,1), mdd_and(f,h,0,1), 1, 1); */
	if (!mdd_is_tautology(a1,1) && !mdd_is_tautology(a1,0)) mdd_free(a1);
	if (!mdd_is_tautology(a2,1) && !mdd_is_tautology(a2,0)) mdd_free(a2);
	array_insert_last(mdd_t *, new_child_list, t);
#endif

	mdd_free(f);
    }

    if (q & 1) { /* if q is odd */
	t = array_fetch(mdd_t *, child_list, child_count);
        array_insert_last(mdd_t *, new_child_list, t);
    }
    f =  mdd_encode(mgr, new_child_list, mv_ptr, index - 1);
    array_free(new_child_list);
    mdd_free(one);
    mdd_free(zero);
    return f;
}


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


