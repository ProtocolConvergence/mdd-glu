#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_init.c,v 1.21 2009/04/11 02:07:47 fabio Exp $
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

int
integer_get_num_of_digits(int value)
{
    double x;
    int num_of_digits;

    if (value > 0)	 x = log10((double) value);
    else if (value == 0) x = 0;
    else		 fail("mdd_init: internal error, value less than 0\n");

/*
 * Notice that floor(x) return a double,
 * so it needs to be cast into an integer using
 * this following expression.
 */
    num_of_digits = (int) floor(x) + 1;

    return num_of_digits;
}

static void
mdd_name_variables(
  array_t *mvar_list,
  int no_mvars,
  array_t **mvar_names_ptr)
{

    char *istr;
    int i;

    if (*mvar_names_ptr == NIL(array_t)) {
	/* if no variable names are given, use generic naming */
	/* mv_0, mv_1, ... for variable 0, 1, ... */
	*mvar_names_ptr = array_alloc(char *, 0);
	for (i=0; i<no_mvars; i++) {
	    /* compose a name for the mvar */
	    istr = ALLOC(char, integer_get_num_of_digits(i + array_n(mvar_list)) + 4);
	    sprintf(istr, "mv_%d", (i + array_n(mvar_list)));
	    array_insert_last(char *, *mvar_names_ptr, istr);

	}
    }
    else {
	if (no_mvars != array_n(*mvar_names_ptr))
	    fail("mdd_init: inconsistent size of mvar_names\n");
    }
}


static void
mdd_record_variables(
  mdd_manager *mgr,
  int current_vertex_before,
  int start_mvar,
  int no_mvars,
  array_t *mvar_names,
  array_t *mvar_values,
  array_t *mvar_strides)
{

    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    array_t *bvar_list = mdd_ret_bvar_list(mgr);
    int stride_count = 1;
    int prev_stride = -1;
	int current_vertex = current_vertex_before;
    int stride, i, j,
	bits, n_bytes;
    mvar_type mv;
    bvar_type bv;
    char *name_str, *mv_name;

#if MDD_VERBOSE
	printf("bdd variable insertion ordering: ");
#endif

    /* mvar_values -> mvar_list */
    for (i=0; i<no_mvars; i++) {
	mv.mvar_id = start_mvar + i;

	/* compose a name for the mvar */
	name_str = array_fetch(char *, mvar_names, i);
	n_bytes = strlen (name_str) + 1;
	mv_name = ALLOC(char, n_bytes);
	strcpy(mv_name, name_str);
	mv.name = mv_name;

	/* register no. of values for the mvar */
	mv.values = array_fetch(int, mvar_values, i);

	/* length of ecoding bits for mvar */
	bits = no_bit_encode(mv.values);
	mv.encode_length = bits;

	/* register the starting bdd vertex no. for the mvar */
	mv.bvars = array_alloc(int, 0);

	if (bits > 0)
	    stride = array_fetch(int, mvar_strides, i);
	else
	    stride = 1;
	for(j=0; j<bits; j++) array_insert_last(int, mv.bvars, current_vertex + (j * stride) );


	/* create place-holder for bit-encoding of the mvar */
	mv.encoding = ALLOC(int, bits);

	/* create bdd variables and put them in bvar_list */
	for (j=0; j<bits; j++) {
	    bv.node = bdd_get_variable(mgr, current_vertex + stride*j);
	    bv.mvar_id = mv.mvar_id;

#if MDD_VERBOSE
		printf("%d ", current_vertex + stride*j);
#endif

	    array_insert(bvar_type, bvar_list, current_vertex + stride*j, bv);
	}

	/* insert the completed mvar_type element to mvar_list */
	mv.status = MDD_ACTIVE;
	array_insert_last(mvar_type, mvar_list, mv);

	if ((prev_stride != -1 ) && (stride != prev_stride)) {
	    printf("mdd_record_variables: processing %s\n", mv.name);
	    fail("mdd_record_variables: inconsistency found in mvar_stride\n");
	}

	/* register the stride for mvar interleaving */
	/* and update current bdd vertex count */
	if (stride_count == stride) {
	    stride_count = 1;
	    current_vertex = current_vertex + stride*(bits-1) + 1;
	    prev_stride = -1;
	}
	else {
	    stride_count++;
	    current_vertex++;
	    prev_stride = stride;
	}
    }

#if MDD_VERBOSE
	printf("\n");
#endif

    /* init all encodings to 2's */
    clear_all_marks(mgr);

#if MONITOR
    print_mvar_list(mgr);
    (void) printf("%d bdd variables created\n", array_n(bvar_list));
    print_bvar_list_id(mgr);
#endif

}


mdd_manager *
mdd_init(
  array_t *mvar_values,
  array_t *mvar_names,
  array_t *mvar_strides)
{
    array_t *mvar_list;
    array_t *bvar_list;
    int i, no_mvars, current_vertex;
    int vertex_sum;
    mdd_manager *mgr;
    mdd_hook_type *mdd_hook;
    bdd_external_hooks *hook;
    boolean free_mvar_strides_flag;
    boolean free_mvar_names_flag;

    mdd_hook = ALLOC(mdd_hook_type, 1);

    /* global information about all mvar for mdd_manager */
    mvar_list = array_alloc(mvar_type, 0);

    /* global information about all bvar for mdd_manager */
    bvar_list = array_alloc(bvar_type, 0);

    /* create the hook to the bdd_manager */
    mdd_hook->mvar_list = mvar_list;
    mdd_hook->bvar_list = bvar_list;

    /* if some array arguments are NIL */
    no_mvars = array_n(mvar_values);

    free_mvar_names_flag = (mvar_names == NIL(array_t));
    mdd_name_variables(mvar_list, no_mvars, &mvar_names);

    /* create mdd manager */
    vertex_sum = 0;
    for (i=0; i<no_mvars; i++) {
	vertex_sum = vertex_sum + no_bit_encode(array_fetch(int,mvar_values,i));
    }
    mgr = bdd_start(vertex_sum);

    hook =  bdd_get_external_hooks(mgr);

    hook->mdd = (char *) mdd_hook;

    current_vertex = 0;

    if (mvar_strides == NIL(array_t)) {
	/* if no strides are specified, the variables are not interleaved */
	/* i.e. mvar_strides = 1 */
	/* must set a flag to know that this array needs to be freed */
	free_mvar_strides_flag = TRUE;
	mvar_strides = array_alloc(int, 0);
	for (i=0; i<no_mvars; i++) {
	    array_insert_last(int, mvar_strides, 1);
	}
    }
    else {
	free_mvar_strides_flag = FALSE;
	if (no_mvars != array_n(mvar_strides))
	    fail("mdd_init: inconsistent size of mvar_strides\n");
    }


    mdd_record_variables(mgr, current_vertex, 0, no_mvars, mvar_names, mvar_values, mvar_strides);

    if (free_mvar_strides_flag) {
      array_free(mvar_strides);
    }

    if (free_mvar_names_flag) {
      for (i = 0; i < array_n(mvar_names); i++) {
	char *name = array_fetch(char *, mvar_names, i);
	FREE(name);
      }
      array_free(mvar_names);
    }

    return mgr;
}


unsigned int
mdd_create_variables(
  mdd_manager *mgr,
  array_t *mvar_values,
  array_t *mvar_names,
  array_t *mvar_strides)
{
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    array_t *bvar_list = mdd_ret_bvar_list(mgr);
    int i, no_mvars, current_vertex, start_mvar;
    int vertex_sum;
    bdd_t *temp;
    boolean free_mvar_strides_flag;
    boolean free_mvar_names_flag;


    /* if some array arguments are NIL */
    no_mvars = array_n(mvar_values);

    free_mvar_names_flag = (mvar_names == NIL(array_t));
    mdd_name_variables(mvar_list, no_mvars, &mvar_names);

    if (mvar_strides == NIL(array_t)) {
	/* if no strides are specified, the variables are not interleaved */
	/* i.e. mvar_strides = 1 */
	/* must set a flag to know that this array needs to be freed */
	free_mvar_strides_flag = TRUE;
	mvar_strides = array_alloc(int, 0);
	for (i=0; i<no_mvars; i++) {
	    array_insert_last(int, mvar_strides, 1);
	}
    }
    else {
	free_mvar_strides_flag = FALSE;
	if (no_mvars != array_n(mvar_strides))
	    fail("mdd_init: inconsistent size of mvar_strides\n");
    }

    vertex_sum = 0;
    for (i=0; i<no_mvars; i++) {
	vertex_sum = vertex_sum + no_bit_encode(array_fetch(int,mvar_values,i));
    }
    current_vertex = array_n(bvar_list);
    for (i=0; i<vertex_sum; i++) {
	temp =  bdd_get_variable(mgr, i+current_vertex);
	bdd_free(temp);
    }

    start_mvar = array_n(mvar_list);

    mdd_record_variables(mgr, current_vertex, start_mvar, no_mvars, mvar_names, mvar_values, mvar_strides);

    if (free_mvar_strides_flag) {
      array_free(mvar_strides);
    }

    if (free_mvar_names_flag) {
      for (i = 0; i < array_n(mvar_names); i++) {
	char *name = array_fetch(char *, mvar_names, i);
	FREE(name);
      }
      array_free(mvar_names);
    }

    return start_mvar;
}


unsigned int
mdd_create_variables_after(
  mdd_manager *mgr,
  int	after_mvar_id,
  array_t *mvar_values,
  array_t *mvar_names,
  array_t *mvar_strides)
{
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    array_t *bvar_list = mdd_ret_bvar_list(mgr);
    mvar_type after_mv, fol_mv;
    int i, no_mvars, current_vertex, start_mvar;
    int vertex_sum;
    int after_bv_id;
    bdd_t *temp;
    bvar_type sec_bit_of_mv;
    int sec_bit_level, fol_id;
    boolean free_mvar_strides_flag;
    boolean free_mvar_names_flag;

    after_mv = array_fetch( mvar_type, mvar_list, after_mvar_id );

    if (after_mv.encode_length > 1) {
				sec_bit_of_mv = mdd_ret_bvar(&after_mv, 1, bvar_list);
				sec_bit_level = bdd_top_var_level( mgr, sec_bit_of_mv.node );
				/* first_bit_of_last_interleaved_mvar */
				fol_id = bdd_get_id_from_level( mgr, sec_bit_level - 1 );
	}
	else
	fol_id = mdd_ret_bvar_id(&after_mv, 0);

	fol_mv = array_fetch( mvar_type, mvar_list, fol_id );

    after_bv_id = mdd_ret_bvar_id(&fol_mv, fol_mv.encode_length - 1);

    /* if some array arguments are NIL */
    no_mvars = array_n(mvar_values);

    free_mvar_names_flag = (mvar_names == NIL(array_t));
    mdd_name_variables(mvar_list, no_mvars, &mvar_names);

    if (mvar_strides == NIL(array_t)) {
	/* if no strides are specified, the variables are not interleaved */
	/* i.e. mvar_strides = 1 */
	/* must set a flag to know that this array needs to be freed */
	free_mvar_strides_flag = TRUE;
	mvar_strides = array_alloc(int, 0);
	for (i=0; i<no_mvars; i++) {
	    array_insert_last(int, mvar_strides, 1);
	}
    }
    else {
	free_mvar_strides_flag = FALSE;
	if (no_mvars != array_n(mvar_strides))
	    fail("mdd_init: inconsistent size of mvar_strides\n");
    }

    vertex_sum = 0;
    for (i=0; i<no_mvars; i++) {
	vertex_sum = vertex_sum + no_bit_encode(array_fetch(int,mvar_values,i));
    }
    for (i=0; i<vertex_sum; i++) {
	temp =  bdd_create_variable_after(mgr, after_bv_id + i);
	bdd_free(temp);
    }

    current_vertex = array_n(bvar_list);
    start_mvar = array_n(mvar_list);

    mdd_record_variables(mgr, current_vertex, start_mvar, no_mvars, mvar_names, mvar_values, mvar_strides);

    if (free_mvar_strides_flag) {
      array_free(mvar_strides);
    }

    if (free_mvar_names_flag) {
      for (i = 0; i < array_n(mvar_names); i++) {
	char *name = array_fetch(char *, mvar_names, i);
	FREE(name);
      }
      array_free(mvar_names);
    }

    return start_mvar;

}


unsigned int
mdd_create_variables_interleaved(
  mdd_manager *mgr,
  int inter_var_id,
  int no_mvars,
  array_t *mvar_names)
{
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    array_t *bvar_list = mdd_ret_bvar_list(mgr);
    mvar_type inter_var;
    int i, j, current_vertex, start_mvar;
    bdd_t *temp;
    array_t *mvar_values;
    array_t *mvar_strides;
    boolean free_mvar_names_flag;

    inter_var = array_fetch( mvar_type, mvar_list, inter_var_id );

    /* if some array arguments are NIL */
    free_mvar_names_flag = (mvar_names == NIL(array_t));
    mdd_name_variables(mvar_list, no_mvars, &mvar_names);


    for (j=0; j<inter_var.encode_length; j++) {
       for (i=0; i<no_mvars; i++) {
		temp = bdd_create_variable_after( mgr, mdd_ret_bvar_id(&inter_var, j) );
		bdd_free( temp );
       }
    }

    current_vertex = array_n(bvar_list);
    start_mvar = array_n(mvar_list);

    mvar_values = array_alloc(int, 0);
    mvar_strides = array_alloc(int, 0);
    for(i=0; i<no_mvars; i++) {
	array_insert_last(int, mvar_values, inter_var.values);
	array_insert_last(int, mvar_strides, no_mvars);
    }

    mdd_record_variables(mgr, current_vertex, start_mvar, no_mvars, mvar_names, mvar_values, mvar_strides);

    array_free(mvar_values);
    array_free(mvar_strides);

    if (free_mvar_names_flag) {
      for (i = 0; i < array_n(mvar_names); i++) {
	char *name = array_fetch(char *, mvar_names, i);
	FREE(name);
      }
      array_free(mvar_names);
    }

    return start_mvar;
}


void
mdd_array_free(array_t *mddArray)
{
  int i;

  if (mddArray != NIL(array_t)) {
    for (i = 0; i < array_n(mddArray); i++) {
      mdd_t *tempMdd = array_fetch(mdd_t *, mddArray, i);
      mdd_free(tempMdd);
    }
    array_free(mddArray);
  }
}

void
mdd_array_array_free(array_t *arrayBddArray)
{
  int		i;
  array_t	*bddArray;

  if (arrayBddArray != NIL(array_t)) {
    for (i = 0; i < array_n(arrayBddArray); i++) {
      bddArray = array_fetch(array_t *, arrayBddArray, i);
      mdd_array_free(bddArray);
    }
    array_free(arrayBddArray);
  }
}

array_t *
mdd_array_duplicate(array_t *mddArray)
{
  int      i;
  int      length = array_n(mddArray);
  array_t *result = array_alloc(mdd_t *, length);
  for (i = 0; i < length; i++) {
    mdd_t *tempMdd = array_fetch(mdd_t *, mddArray, i);
    array_insert(mdd_t *, result, i, mdd_dup(tempMdd));
  }

  return (result);
}


/* Return true iff two arrays of mdds are identical, i.e., they contain
   identical mdds in the same order */
boolean
mdd_array_equal(array_t *array1, array_t *array2)
{
  int i;

  assert(array1 != NIL(array_t) && array2 != NIL(array_t));

  if(array_n(array1) != array_n(array2))
    return FALSE;

  for(i = 0; i < array_n(array1); i++) {
    mdd_t *mdd1 = array_fetch(mdd_t *, array1, i);
    mdd_t *mdd2 = array_fetch(mdd_t *, array2, i);
    if(!mdd_equal(mdd1, mdd2))
      return FALSE;
  }

  return TRUE;
}

/* wrapper for mdd_init, to create a manager with no variables */
mdd_manager *
mdd_init_empty(void)
{
  array_t     *empty_array = array_alloc(int, 0);
  mdd_manager *mdd_mgr = mdd_init(empty_array, NIL(array_t), NIL(array_t));

  array_free(empty_array);
  return mdd_mgr;
}

/* This function re-encodes an MDD variable when a BDD variable in
   its encoding has a fixed value.
 */
void
mdd_var_expunge_bdd_variable(
  mdd_manager *mgr /* MDD manager */,
  int bddId /* variable that has constant value */,
  int val /* value at which variable is fixed */
  )
{
  array_t *mvar_list = mdd_ret_mvar_list(mgr);
  array_t *bvar_list = mdd_ret_bvar_list(mgr);
  array_t *bvars;

  bvar_type bddVar;
  mvar_type mvVar;
  int position, i, id, shift, values;

  /* Check arguments. */
  if (val != 0 && val != 1) fail("value must be either 0 or 1");
  if (bddId < 0 || bddId >= array_n(bvar_list)) fail("bddId out of range");

  /* Extract info. */
  bddVar = array_fetch(bvar_type, bvar_list, bddId);
  mvVar = array_fetch(mvar_type, mvar_list, bddVar.mvar_id);
  bvars = array_alloc(int,array_n(mvVar.bvars)-1);

  /* Expunge fixed variable from list of BDD variables. */
  position = array_n(mvVar.bvars); /* initialization for later checking */
  arrayForEachItem(int, mvVar.bvars, i, id) {
    if (id != bddId)
      array_insert_last(int, bvars, id);
    else
      position = i;
  }
  /* Make sure we wrote position in the loop. */
  assert(position < array_n(mvVar.bvars));

  /* Count number of surviving values. */
  shift = (mvVar.encode_length - position - 1);

  for (i = 0, values = 0; i < mvVar.values; i++) {
    values += (i >> shift) ^ ~val;
  }

  /* Update mvar_list */
  array_free(mvVar.bvars);
  mvVar.bvars = bvars;
  mvVar.values = values;
  mvVar.encode_length--;
  array_insert(mvar_type, mvar_list, bddVar.mvar_id, mvVar);

  /* Update bvar_list. */
  bvar_type bv = array_fetch(bvar_type, bvar_list, bddId);
  bv.mvar_id = -1;
  array_insert(bvar_type, bvar_list, bddId, bv);

} /* mdd_var_expunge_bdd_variable */

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
