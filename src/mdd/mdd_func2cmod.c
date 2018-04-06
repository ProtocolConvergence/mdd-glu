#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_func2cmod.c,v 1.4 2002/08/25 05:30:12 fabio Exp $
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
mdd_func2c_mod(
  mdd_manager *mgr, 
  int mvar1,
  int mvar2,
  int constant,
  boolean (*func4)(int, int, int, int))
{
    mvar_type x, y;
    array_t *child_list_x, *child_list_y;
    int i, j;
    mdd_t *tx, *ty;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    mdd_t *one, *zero;

    x = array_fetch(mvar_type, mvar_list, mvar1);
    y = array_fetch(mvar_type, mvar_list, mvar2);

    if (x.status == MDD_BUNDLED) {
	(void) fprintf(stderr, 
		"\nWarning: mdd_func2c, bundled variable %s is used\n", x.name);
	fail("");
    }

    if (y.status == MDD_BUNDLED) {
	(void) fprintf(stderr,
		"\nWarning: mdd_func2c, bundled variable %s is used\n", y.name);
	fail("");
    }

    if((x.values   != y.values)  || ( constant < 0) || ( constant >= x.values) ) {
      (void) fprintf(stderr, "\n mdd_func2c_mod: Cannot operate with two different ranges\n");
      exit(1);
    }
    

    one = mdd_one(mgr);
    zero = mdd_zero(mgr);

    child_list_x = array_alloc(mdd_t *, 0);
    for (i=0; i<x.values; i++) {
	child_list_y = array_alloc(mdd_t *, 0);
	for (j=0; j<y.values; j++) {
	    if (func4(i,j,constant,x.values))
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

    mdd_free(zero);
    mdd_free(one);

    return tx;
}



/***** internal functions *****/       

boolean 
eq_plus3mod(int x, int y, int z, int range)
{
    return (x == (y + z) % range);
}

boolean 
geq_plus3mod(int x, int y, int z, int range)
{
    return (x >= (y + z) % range);
}

boolean 
gt_plus3mod(int x, int y, int z, int range)
{
    return (x > (y + z) % range);
}

boolean 
leq_plus3mod(int x, int y, int z, int range)
{
    return (x <= (y + z) % range);
}

boolean 
lt_plus3mod(int x, int y, int z, int range)
{
    return (x < (y + z) % range);
}

boolean 
neq_plus3mod(int x, int y, int z, int range)
{
    return (x != (y + z) % range);
}

boolean 
eq_minus3mod(int x, int y, int z, int range)
{
    return (x == (y - z) % range);
}

boolean 
geq_minus3mod(int x, int y, int z, int range)
{
    return (x >= (y - z) % range);
}

boolean 
gt_minus3mod(int x, int y, int z, int range)
{
    return (x > (y - z) % range);
}

boolean 
leq_minus3mod(int x, int y, int z, int range)
{
    return (x <= (y - z) % range);
}

boolean 
lt_minus3mod(int x, int y, int z, int range)
{
    return (x < (y - z) % range);
}

boolean 
neq_minus3mod(int x, int y, int z, int range)
{
    return (x != (y - z) % range);
}


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

