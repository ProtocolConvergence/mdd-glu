#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mvar2bdds.c,v 1.10 2002/08/27 03:16:56 fabio Exp $
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
mvar2bdds(mdd_manager *mgr, array_t *mvars)
{
    array_t *bdd_vars;
    int i, j, mv_no;
    mvar_type mv;
    bdd_t *temp;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);

    bdd_vars = array_alloc(bdd_t *, 0);	
    for (i=0; i<array_n(mvars); i++) {
        mv_no = array_fetch(int, mvars, i);
	mv = array_fetch(mvar_type, mvar_list, mv_no);
        for (j = 0; j < mv.encode_length; j ++) {
	    temp = bdd_get_variable(mgr, mdd_ret_bvar_id(&mv,j) );
	    array_insert_last(bdd_t *, bdd_vars, temp);
	}
    }
    return (bdd_vars);
}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

