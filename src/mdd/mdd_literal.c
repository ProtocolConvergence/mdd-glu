#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_literal.c,v 1.10 2002/08/24 21:48:15 fabio Exp $
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

/* Returns \/_{i in values}(mddid == i) */
/* Algorithm used to be quadratic.  Is linear now */
mdd_t *
mdd_literal(
  mdd_manager *mgr,
  int mddid,
  array_t *values)
{
  mvar_type mvar;
  mdd_t *one, *zero;
  array_t *allValues;    /* Holds one in pos i iff value i is allowed */
  int i;                 /* iterator                                  */   
  int value;             /* iterates over values                      */
  mdd_t *result;
  
  mvar = mdd_get_var_by_id(mgr, mddid);
  one = mdd_one(mgr);
  zero = mdd_zero(mgr);
  allValues = array_alloc(mdd_t *, mvar.values);

  /* first set every value to zero */
  for(i = 0; i < mvar.values; i++)
    array_insert(mdd_t *, allValues, i, zero);
  
  /* then set requested values to one */
  arrayForEachItem(int, values, i, value)
    array_insert(mdd_t *, allValues, value, one);
  
  result = mdd_case(mgr, mddid, allValues);
  array_free(allValues);

  mdd_free(one);
  mdd_free(zero);
  
  return result;
}


/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


