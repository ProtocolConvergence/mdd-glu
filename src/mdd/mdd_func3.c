#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_func3.c,v 1.10 2002/08/25 05:30:12 fabio Exp $
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
mdd_func3(
  mdd_manager *mgr,
  int mvar1,
  int mvar2,
  int mvar3,
  boolean (*func3)(int, int, int))
{
    mvar_type x, y, z;
    array_t *child_list_x, *child_list_y, *child_list_z;
    int i, j, k;
    mdd_t *tx, *ty, *tz;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    mdd_t *one, *zero;

    one = mdd_one(mgr);
    zero = mdd_zero(mgr);

    x = array_fetch(mvar_type, mvar_list, mvar1);
    y = array_fetch(mvar_type, mvar_list, mvar2);
    z = array_fetch(mvar_type, mvar_list, mvar3);

    if (x.status == MDD_BUNDLED) {
	(void) fprintf(stderr, 
		"\nWarning: mdd_func3, bundled variable %s is used\n", x.name);
	fail("");
    }

    if (y.status == MDD_BUNDLED) {
	(void) fprintf(stderr,
		"\nWarning: mdd_func3, bundled variable %s is used\n", y.name);
	fail("");
    }

    if (z.status == MDD_BUNDLED) {
	(void) fprintf(stderr, 
		"\nWarning: mdd_func3, bundled variable %s is used\n", z.name);
	fail("");
    }


    child_list_x = array_alloc(mdd_t *, 0);
    for (i=0; i<x.values; i++) {
	child_list_y = array_alloc(mdd_t *, 0);
	for (j=0; j<y.values; j++) {
	    child_list_z = array_alloc(mdd_t *, 0);
	    for (k=0; k<z.values; k++) {
	        if (func3(i,j,k))
		    array_insert_last(mdd_t *, child_list_z, one);
                else
                    array_insert_last(mdd_t *, child_list_z, zero);
	    }
	    tz = mdd_case(mgr, mvar3, child_list_z);
	    array_insert_last(mdd_t *, child_list_y, tz);
	    array_free(child_list_z);
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
eq_plus3(int x, int y, int z)
{
    return (x == y + z);
}

boolean 
geq_plus3(int x, int y, int z)
{
    return (x >= y + z);
}

boolean 
gt_plus3(int x, int y, int z)
{
    return (x > y + z);
}

boolean 
leq_plus3(int x, int y, int z)
{
    return (x <= y + z);
}

boolean 
lt_plus3(int x, int y, int z)
{
    return (x < y + z);
}

boolean 
neq_plus3(int x, int y, int z)
{
    return (x != y + z);
}

boolean 
eq_minus3(int x, int y, int z)
{
    return (x == y - z);
}

boolean 
geq_minus3(int x, int y, int z)
{
    return (x >= y - z);
}

boolean 
gt_minus3(int x, int y, int z)
{
    return (x > y - z);
}

boolean 
leq_minus3(int x, int y, int z)
{
    return (x <= y - z);
}

boolean 
lt_minus3(int x, int y, int z)
{
    return (x < y - z);
}

boolean 
neq_minus3(int x, int y, int z)
{
    return (x != y - z);
}


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


