#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_uminus.c,v 1.9 2002/08/24 21:48:15 fabio Exp $
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
mdd_unary_minus_s(
  mdd_manager *mgr,
  int mvar1,
  int mvar2)
{
    return (mdd_unary_minus(mgr, mvar1, mvar2));
}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

