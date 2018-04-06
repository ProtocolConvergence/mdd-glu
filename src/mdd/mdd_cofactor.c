#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_cofactor.c,v 1.10 2009/01/07 17:14:47 fabio Exp $
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



/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void mdd_traverse(mdd_manager *mgr, bdd_t *top, boolean *mvar_present);
static array_t * mvars_extract(mdd_manager *mgr, mdd_t *fn);

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

mdd_t *
mdd_cofactor(
  mdd_manager *mgr,
  mdd_t *fn,
  mdd_t *cube)
{
    array_t *mvars;
    mdd_t *top;

    mvars = mvars_extract(mgr, cube);
    top = mdd_and_smooth(mgr, fn, cube, mvars);
    array_free(mvars);
    return top;
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

static void
mdd_traverse(
  mdd_manager *mgr,
  bdd_t *top,
  boolean *mvar_present)
{
    bvar_type bv;
    array_t *bvar_list = mdd_ret_bvar_list(mgr);
    int is_complemented;
    bdd_t *uncomp_top, *child, *temp_child;

    if (bdd_is_tautology(top,1)) {
        return;
    }
    if (bdd_is_tautology(top,0)) {
        return;
    }

    (void)bdd_get_node(top,&is_complemented);

    bv = array_fetch(bvar_type, bvar_list, bdd_top_var_id(top));
    mvar_present[bv.mvar_id] = 1;

    if (is_complemented) uncomp_top = bdd_not(top);
    else uncomp_top = mdd_dup(top);

    child = bdd_then(uncomp_top);
    (void) bdd_get_node(child,&is_complemented);
    if (is_complemented) {
        temp_child = child;
        child = bdd_not(temp_child);
        mdd_free(temp_child);
    }
    
    mdd_traverse(mgr, child , mvar_present);
    mdd_free(child);

    child = bdd_else(uncomp_top);
    (void) bdd_get_node(child,&is_complemented);
    if (is_complemented) {
        temp_child = child;
        child = bdd_not(temp_child);
        mdd_free(temp_child);
    }

    mdd_traverse(mgr, child, mvar_present);
    
 
    mdd_free(child);
    mdd_free(uncomp_top);
    return;    
}


static array_t *
mvars_extract(
  mdd_manager *mgr,
  mdd_t *fn)
{
    int i, no_mvar;
    boolean *mvar_present;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    array_t *mvars;
                                                                          

    mvars = array_alloc(int, 0);
    no_mvar = array_n(mvar_list);
    mvar_present = ALLOC(boolean, no_mvar);
    for (i=0; i<no_mvar; i++) mvar_present[i] = 0;
    mdd_traverse(mgr, fn, mvar_present);
    for (i=0; i<no_mvar; i++) 
	if (mvar_present[i] == 1) array_insert_last(int, mvars, i);
    FREE(mvar_present);
    return mvars;
}
