#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_support.c,v 1.12 2002/08/24 21:48:15 fabio Exp $
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

array_t *
mdd_get_support(mdd_manager *mdd_mgr, mdd_t *f)
{
    array_t *full_list, *support_list;
    array_t *mvar_list = mdd_ret_mvar_list(mdd_mgr);
    array_t *bvar_list = mdd_ret_bvar_list(mdd_mgr);
    var_set_t *vset;
    int i, list_length;
    bvar_type bv;
    boolean present;


    /* initialize full list of mvar id's */
    list_length = array_n(mvar_list);
    full_list = array_alloc(boolean, list_length);
    for (i = 0; i < array_n(mvar_list); i++) {
	array_insert(boolean, full_list, i, 0);
    }

    vset = bdd_get_support(f);
    for (i = 0; i < array_n(bvar_list); i++) {
	if (var_set_get_elt(vset, i) == 1) {
	    bv = array_fetch(bvar_type, bvar_list, i);
	    (void) array_insert(boolean, full_list, bv.mvar_id, 1);
	}
    }

    support_list = array_alloc(int, 0);
    for (i = 0; i < array_n(mvar_list); i++) {
	present = array_fetch(boolean, full_list, i);
	if (present) array_insert_last(int, support_list, i);
    }

    (void) array_free(full_list);
    (void) var_set_free(vset);

    return support_list;
}

array_t *
mdd_get_bdd_support_ids(mdd_manager *mdd_mgr, mdd_t *f)
{
    array_t *bdd_support_list;
    array_t *bvar_list = mdd_ret_bvar_list(mdd_mgr);
    var_set_t *vset;
    int i;

    bdd_support_list = array_alloc(int, 0);

    vset = bdd_get_support(f);
    for (i = 0; i < array_n(bvar_list); i++) {
	if (var_set_get_elt(vset, i) == 1) {
	    array_insert_last(int, bdd_support_list, i);
	}
    }

    (void) var_set_free(vset);
    return bdd_support_list;
}

array_t *
mdd_get_bdd_support_vars(mdd_manager *mdd_mgr, mdd_t *f)
{
    array_t *bdd_support_list;
    array_t *bvar_list = mdd_ret_bvar_list(mdd_mgr);
    var_set_t *vset;
    mdd_t *var;
    int i;

    bdd_support_list = array_alloc(mdd_t *, 0);
    
    vset = bdd_get_support(f);
    for (i = 0; i < array_n(bvar_list); i++) {
	if (var_set_get_elt(vset, i) == 1) {
	    var = bdd_var_with_index(mdd_mgr, i);
	    array_insert_last(mdd_t *, bdd_support_list, var);
	}
    }

    (void) var_set_free(vset);
    return bdd_support_list;
}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

