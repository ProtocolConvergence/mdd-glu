#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_func2.c,v 1.11 2002/08/25 05:30:12 fabio Exp $
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
mdd_func2(
  mdd_manager *mgr,
  int mvar1,
  int mvar2,
  boolean (*func2)(int, int))
{
    mvar_type x, y;
    array_t *child_list_x, *child_list_y;
    int i, j;
    mdd_t *tx, *ty;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    mdd_t *one, *zero;

    one = mdd_one(mgr);
    zero = mdd_zero(mgr);

    x = array_fetch(mvar_type, mvar_list, mvar1);
    y = array_fetch(mvar_type, mvar_list, mvar2);

    if (x.status == MDD_BUNDLED) {
	(void) fprintf(stderr, 
		"\nWarning: mdd_func2, bundled variable %s is used\n", x.name);
	fail("");
    }

    if (y.status == MDD_BUNDLED) {
	(void) fprintf(stderr,
		"\nWarning: mdd_func2 bundled variable %s is used\n", y.name);
	fail("");
    }

    child_list_x = array_alloc(mdd_t *, 0);
    for (i=0; i<x.values; i++) {
	child_list_y = array_alloc(mdd_t *, 0);
	for (j=0; j<y.values; j++) {
	    if (func2(i,j))
		array_insert_last(mdd_t *, child_list_y, one);
            else
                array_insert_last(mdd_t *, child_list_y, zero);
	}
	ty = mdd_case(mgr, mvar2, child_list_y);
	array_insert_last(mdd_t *, child_list_x, ty);
	array_free(child_list_y);
    }
    tx = mdd_case(mgr, mvar1, child_list_x);
    array_free(child_list_x);
    mdd_free(one);
    mdd_free(zero);
    return tx;
}


/***** internal functions *****/

boolean
eq2(int x, int y)
{
    return (x == y);
}

boolean
geq2(int x, int y)
{
    return (x >= y);
}

boolean
gt2(int x, int y)
{
    return (x > y);
}

boolean
leq2(int x, int y)
{
    return (x <= y);
}

boolean
lt2(int x, int y)
{
    return (x < y);
}

boolean
neq2(int x, int y)
{
    return (x != y);
}

boolean
unary_minus2(int x, int y)
{
    return (x+y == 0);
}


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

