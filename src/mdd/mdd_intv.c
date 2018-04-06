#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_intv.c,v 1.9 2002/08/24 21:48:15 fabio Exp $
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

/* var <= c */
mdd_t *
build_leq_c(
  mdd_manager *mgr,
  int mvar_id,
  int c)
{
	return build_lt_c(mgr, mvar_id, c+1);
}

mdd_t *
build_lt_c(
  mdd_manager *mgr,
  int mvar_id,
  int c)
{
/*	mdd_t *A, *one, *zero;
	array_t *mvar_list = mdd_ret_mvar_list(mgr);
	array_t *bvar_list = mdd_ret_bvar_list(mgr);
	mvar_type a_mv;
	int i;
	bvar_type bit_i;
        mdd_t *temp_A;

	one = mdd_one(mgr);
	zero = mdd_zero(mgr);

	a_mv = array_fetch( mvar_type, mvar_list, mvar_id);


	if ( a_mv.values <= c ) {
		mdd_free(zero);
		return one;
	}

	A = mdd_zero(mgr);

	for(i=1; i <= a_mv.encode_length; i++){
		bit_i = mdd_ret_bvar(&a_mv, (a_mv.encode_length - i), bvar_list);
                temp_A = A;
		if ( getbit(c,i-1) == 0 ) {
			A = mdd_ite(bit_i.node, zero, temp_A, 1, 1, 1);
                }
		else {
			A = mdd_ite(bit_i.node, temp_A, one, 1, 1, 1);
                }
                bdd_free(temp_A);
	}

	mdd_free(one);
	mdd_free(zero);
		
	return A;
    */

/* Temporary fix until the routines are rewritten taking care of don't cares. */    

    return mdd_lt_c(mgr, mvar_id, c);
} 


mdd_t *
build_geq_c(
  mdd_manager *mgr,
  int mvar_id,
  int c)
{
/*
  mdd_t *A, *one, *zero;
	array_t *bvar_list = mdd_ret_bvar_list(mgr),
	        *mvar_list = mdd_ret_mvar_list(mgr);
	mvar_type a_mv;
	int i;
	bvar_type bit_i;
        mdd_t *temp_A;

	one = mdd_one(mgr);
	zero = mdd_zero(mgr);

	a_mv = array_fetch( mvar_type, mvar_list, mvar_id);

	if ( a_mv.values <= c ) { 
		mdd_free( one );
		return zero;
	}


	A = mdd_one(mgr);

	for(i=1; i <= a_mv.encode_length; i++){
		bit_i = mdd_ret_bvar(&a_mv, (a_mv.encode_length - i), bvar_list );
                temp_A = A;
		if ( getbit(c,i-1) == 0 ) {
			A = mdd_ite(bit_i.node, one, temp_A, 1, 1, 1);
                }
		else {
			A = mdd_ite(bit_i.node, temp_A, zero, 1, 1, 1);
                     }
        	bdd_free(temp_A);	
	}

	mdd_free(one);
	mdd_free(zero);
		
	return A;
    */
    return mdd_geq_c(mgr, mvar_id, c);
}


mdd_t *
build_gt_c(
  mdd_manager *mgr,
  int mvar_id,
  int c)
{
	return build_geq_c(mgr, mvar_id, c+1);
}

/* low <= var <= high */
mdd_t *
mdd_interval(
  mdd_manager *mgr,
  int mvar_id,
  int low,
  int high)
{
	mdd_t *HIGH_MDD, *LOW_MDD, *result;


    LOW_MDD = build_geq_c(mgr,mvar_id,low);
	HIGH_MDD = build_leq_c(mgr,mvar_id,high);

	result = mdd_and(HIGH_MDD, LOW_MDD, 1, 1);

	mdd_free(LOW_MDD);
	mdd_free(HIGH_MDD);

	return result;
}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

