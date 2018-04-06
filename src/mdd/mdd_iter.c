#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_iter.c,v 1.8 2002/08/27 16:30:26 fabio Exp $
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

/* this routine returns the first minterm (2's in cube becomes 0's), */
/* given an mdd generator, containing a cube and a list of variables */
/* to be included in the minterm */
   
static array_t *
first_minterm(mdd_gen *mgen)
{
    array_t *minterm;
    bdd_literal literal;
    int i, j;

    array_t *mvar_list = mdd_ret_mvar_list(mgen->manager);
    int mv_id;
    mvar_type mv;
    boolean out_of_range;  /* a minterm is out of range if one variable is */
    array_t *solution;
    int value;

    out_of_range = 0;
                                                                                
    minterm = array_alloc(bdd_literal, 0);
    solution = array_alloc(int, 0);
    /* loop once for each mvar */
    for (i=0; i<array_n(mgen->var_list); i++) {
	mv_id = array_fetch(int, mgen->var_list, i);
	mv = array_fetch(mvar_type, mvar_list, mv_id);
	/* loop for each bvar */
        value = 0;
        for (j=0; j<mv.encode_length; j++) {
	    literal = array_fetch(bdd_literal, mgen->cube, mdd_ret_bvar_id(&mv, j) ); 
            if (literal == 0) {
		array_insert_last(bdd_literal, minterm, 0);
		value = value*2 + 0;
	    }
	    else if (literal == 1) {
		array_insert_last(bdd_literal, minterm, 1);
		value = value*2 + 1;
	    }
	    else { /*if (literal == 2) */
		array_insert_last(bdd_literal, minterm, 0);
		value = value*2 + 0;
            }
	}
	(void) array_insert_last(int, solution, value);
	if (value >= mv.values) out_of_range = 1;
    }
    mgen->minterm = minterm;
    mgen->out_of_range = out_of_range;

    return (solution);
}


/* this routine returns the next minterm, given a current minterm, */
/* a cube and a list of variables to be included in the minterm */
/* if no more, return NIL(array_t). */

static array_t *
next_minterm(mdd_gen *mgen)
{
    int carry;
    int i, j, k;
    array_t *minterm;
    bdd_literal literal;
    bdd_literal prev_literal;
    array_t *mvar_list = mdd_ret_mvar_list(mgen->manager);
    int mv_id;
    mvar_type mv;
    boolean out_of_range;
    array_t *solution;
    int value;

    out_of_range = 0;

    carry = 1;
    k = 0;

    minterm = array_alloc(bdd_literal, array_n(mgen->minterm)); 
    solution = array_alloc(int, 0);
    /* loop once for each mvar */
    for (i=0; i<array_n(mgen->var_list); i++) {
	mv_id = array_fetch(int, mgen->var_list, i);
	mv = array_fetch(mvar_type, mvar_list, mv_id);
	value = 0;
	/* loop for each bvar */
        for (j=0; j<mv.encode_length; j++) {
            literal = array_fetch(bdd_literal, mgen->cube, mdd_ret_bvar_id(&mv, j) );

	    prev_literal = array_fetch(bdd_literal, mgen->minterm, k);
            if ((literal == 2) && (carry == 1)) {
		if (prev_literal == 0) {
		    array_insert(bdd_literal, minterm, k, 1);
		    carry = 0;
		    value = value*2 + 1;
		}
		else if (prev_literal == 1) {
		    array_insert(bdd_literal, minterm, k, 0);
		    carry = 1;
		    value = value*2 + 0;
		}
	    }
	    else if ((literal == 2) && (carry == 0)) {
		    array_insert(bdd_literal, minterm, k, prev_literal);
		    value = value*2 + prev_literal;
	    }
	    else { /* if literal == 0 or 1 */
                    array_insert(bdd_literal, minterm, k, literal);
		    value = value*2 + literal;
	    }
	    k++;
	}
	array_insert_last(int, solution, value);
	if (value >= mv.values) out_of_range = 1;
    }

    if (carry == 1) {
	/* no more minterms */
	array_free(minterm);
	minterm = NIL(array_t);
	array_free(solution);
	solution = NIL(array_t);
    }
    array_free(mgen->minterm);
    mgen->minterm = minterm;
    mgen->out_of_range = out_of_range;

    return (solution);
}


static array_t *
next_valid_minterm(mdd_gen *mgen)
{
    array_t *solution;
    array_t *cube;

    solution = next_minterm(mgen);

    /* check if done with current cube */
    if (mgen->minterm == NIL(array_t)) {
	(void) bdd_next_cube(mgen->bdd_generator, &cube);
        mgen->status = bdd_gen_read_status(mgen->bdd_generator);
	mgen->cube = cube;
	if (mgen->status != bdd_EMPTY) {
	    if (solution != NIL(array_t) ) array_free(solution);
	    solution = first_minterm(mgen);
	}
	else {
	    mgen->minterm = NIL(array_t);
	    mgen->cube = NIL(array_t);
	}
    }
    return (solution);
}


mdd_gen *
mdd_first_minterm(
  mdd_t *f,
  array_t **solution_p   /* minterm = array of values */,
  array_t *variable_list)
{
    mdd_gen *mgen;	/* mdd generator */
    bdd_gen *bgen;	/* bdd generator for bdd_first_cube, bdd_next_cube */
    array_t *cube;	/* array of literals {0,1,2} */
    array_t *allvar_list; 
    array_t *mvar_list;
    array_t *smoothing_list;
    int i, j, k;
    array_t *solution = NIL(array_t); /* initialize for lint */
    array_t *var_list;
    boolean i_is_present;
    mdd_t *mdd_tmp, *f_copy;
    bdd_manager *mgr;

    if ( f != NIL(mdd_t) ) {
	/* new code added by Timothy */
	/* f is first smoothed by all variables NOT present in var_list */

	f_copy = mdd_dup(f); 

	smoothing_list = array_alloc(int, 0);
    
    
	mgr = bdd_get_manager(f_copy);
	mvar_list = mdd_ret_mvar_list(mgr);                                                                           

	for (i = 0; i < array_n(mvar_list); i++) {
	    i_is_present = FALSE;
	    for (j = 0; j < array_n(variable_list); j++) {
		k = array_fetch(int, variable_list, j);
		if (k == i) i_is_present = TRUE;
	    }
	    if (i_is_present == FALSE) array_insert_last(int, smoothing_list, i);
	}
	if (array_n(smoothing_list) > 0) {
	    mdd_tmp = mdd_smooth(mgr, f_copy, smoothing_list);
	    (void) mdd_free(f_copy);
	    f_copy = mdd_dup(mdd_tmp);
	    (void) mdd_free(mdd_tmp);
    
	}

	(void) array_free(smoothing_list);

	bgen = bdd_first_cube(f_copy, &cube);
	(void) mdd_free(f_copy);
    
	mgen = ALLOC(mdd_gen, 1);
	mgen->manager = mgr;
	mgen->bdd_generator = bgen;
	mgen->status = bdd_gen_read_status(mgen->bdd_generator);
	mgen->cube = cube; 		/* store in mdd gen for later use */
	mgen->out_of_range = 0;
	if (mgen->status != bdd_EMPTY) {
	    if (variable_list != NIL(array_t)) {
		var_list = array_dup(variable_list);
		mgen->var_list = var_list;
	    }
	    else {
		allvar_list = array_alloc(int, 0);
		for (i=0; i<array_n(mvar_list); i++) 
		    array_insert_last(int, allvar_list, i);
		mgen->var_list = allvar_list;
	    }
	    solution = first_minterm(mgen);
	    while (mgen->out_of_range) {
		array_free(solution);
		solution = next_valid_minterm(mgen);
	    }
	}
	else {

	    /* mgen->status == bdd_EMPTY */
	    if (variable_list != NIL(array_t)) {
                var_list = array_dup(variable_list);
                mgen->var_list = var_list;
            }
            else {
                allvar_list = array_alloc(int, 0);
                for (i=0; i<array_n(mvar_list); i++)
                    array_insert_last(int, allvar_list, i);
                mgen->var_list = allvar_list;
            }

	    mgen->minterm = NIL(array_t);
	    mgen->cube = NIL(array_t);
	    /* previous solution will not be freed here */
	    /* but by the calling procedure */
	}
	*solution_p = solution;
    }
    else /* f = NIL */{
      mgen = ALLOC(mdd_gen, 1);
      mgen->manager = NIL(mdd_manager);
      mgen->bdd_generator = NIL(bdd_gen);
      mgen->status = bdd_EMPTY;
      mgen->cube = NIL(array_t);
      mgen->minterm = NIL(array_t);
      mgen->out_of_range = 0;
      mgen->var_list = NIL(array_t);
    }

    return (mgen);
}


boolean
mdd_next_minterm(
  mdd_gen *mgen,
  array_t **solution_p 	/* minterm = array of values */)
{
    array_t *solution;
    solution = next_valid_minterm(mgen);
    while ((mgen->out_of_range) && (mgen->status != bdd_EMPTY)) {
	array_free(solution);
	solution = next_valid_minterm(mgen);
    }
    *solution_p = solution;
    if (mgen->status != bdd_EMPTY)
	return (1);
    else
	return (0);
}


void
mdd_print_array(array_t *array)
{
    int i, value;
    for (i=0; i<array_n(array); i++) {
	value = array_fetch(int, array, i);
        printf("%d ",value);
    }
    printf("\n");
}


int
mdd_gen_free(mdd_gen *mgen)
{
    if (mgen->minterm != NIL(array_t)) array_free(mgen->minterm);
/*  if (mgen->cube != NIL(array_t)) array_free(mgen->cube); */
/*  mgen->cube gets freed in bdd_gen_free(mgen->bdd_generator) below */
    if (mgen->var_list != NIL(array_t)) array_free(mgen->var_list);
    bdd_gen_free(mgen->bdd_generator);
    FREE(mgen);

    return (0);
}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

