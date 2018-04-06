/**CFile***********************************************************************

  FileName    [cmuPort.c]

  PackageName [cmu_port]

  Synopsis    [Port routines for CMU package.]

  Description [optional]

  SeeAlso     [optional]

  Author      [Thomas R. Shiple. Some changes by Rajeev K. Ranjan.]

  Copyright   [Copyright (c) 1994-1996 The Regents of the Univ. of California.
  All rights reserved.

  Permission is hereby granted, without written agreement and without license
  or royalty fees, to use, copy, modify, and distribute this software and its
  documentation for any purpose, provided that the above copyright notice and
  the following two paragraphs appear in all copies of this software.

  IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
  CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN
  "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO PROVIDE
  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.]

  Revision    [$Id: cmuPort.c,v 1.83 2012/02/04 21:43:23 fabio Exp $]

******************************************************************************/

#include "cmuPortInt.h" 
#ifndef	EPD_MAX_BIN
#include "epd.h" 
#endif

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Builds the bdd_t structure.]

  Description [Builds the bdd_t structure from manager and node.
  Assumes that the reference count of the node has already been
  increased.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_construct_bdd_t(bdd_manager *manager, bdd_node *func)
{
    bdd_t *result;
    cmu_bdd_manager mgr = (cmu_bdd_manager)manager;
    bdd fn = (bdd)func;

    if (fn == (struct bdd_ *) 0) {
	cmu_bdd_fatal("bdd_construct_bdd_t: possible memory overflow");
    }

    result = ALLOC(bdd_t, 1);
    result->mgr = mgr;
    result->node = fn;
    result->free = FALSE;
    return result;
}

/**Function********************************************************************
  Synopsis           [required]

  Description        [optional]

  SideEffects        [required]

  SeeAlso            [optional]
******************************************************************************/
bdd_package_type_t
bdd_get_package_name(void)
{
  return CMU;
}

/*
BDD Manager Allocation And Destruction ----------------------------------------
*/
void
bdd_end(bdd_manager *manager)
{
    bdd_external_hooks *hooks;
    cmu_bdd_manager mgr = (cmu_bdd_manager)manager;
    hooks = (bdd_external_hooks *) mgr->hooks;
    FREE(hooks); 
    cmu_bdd_quit(mgr);
}


bdd_manager *
bdd_start(int nvariables)
{
    struct bdd_manager_ *mgr;
    int i;
    bdd_external_hooks *hooks;
   
    mgr = cmu_bdd_init();    /*no args*/

    /*
     * Calls to UCB bdd_get_variable are translated into cmu_bdd_var_with_id calls.  However,
     * cmu_bdd_var_with_id assumes that single variable BDDs have already been created for 
     * all the variables that we wish to access.  Thus, following, we explicitly create n
     * variables.  We do not care about the return value of cmu_bdd_new_var_last; in the 
     * CMU package, the single variable BDDs are NEVER garbage collected.
     */
    for (i = 0; i < nvariables; i++) {
	(void) cmu_bdd_new_var_last(mgr);
    }

    hooks = ALLOC(bdd_external_hooks, 1);
    hooks->mdd = hooks->network = hooks->undef1 = (char *) 0;
    mgr->hooks = (char *) hooks;  /* new field added to CMU manager */

    return (bdd_manager *) mgr;
}

/*
BDD Variable Allocation -------------------------------------------------------
*/

bdd_t *
bdd_create_variable(bdd_manager *manager)
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  return bdd_construct_bdd_t(mgr, cmu_bdd_new_var_last(mgr));
}

bdd_t *
bdd_create_variable_after(bdd_manager *manager, bdd_variableId after_id)
{
  struct bdd_ *after_var;
  bdd_t 	*result;
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;

  after_var = cmu_bdd_var_with_id(mgr, (long)after_id + 1);
  
  result =  bdd_construct_bdd_t(mgr, cmu_bdd_new_var_after(mgr, after_var));
  
  /* No need to free after_var, since single variable BDDs are never garbage collected */
  
  return result;
}



bdd_t *
bdd_get_variable(bdd_manager *manager, bdd_variableId variable_ID)
{
  struct bdd_ *fn;
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  fn = cmu_bdd_var_with_id(mgr, (long) (variable_ID + 1));

  if (fn == (struct bdd_ *) 0) {
    int i;
    for (i=cmu_bdd_vars(mgr); i < variable_ID + 1; i++) {
      fn = cmu_bdd_new_var_last(mgr);
    }
  }
  
  return bdd_construct_bdd_t(mgr, fn);
}

/*
BDD Formula Management --------------------------------------------------------
*/

bdd_t *
bdd_dup(bdd_t *f)
{
  return bdd_construct_bdd_t(f->mgr, cmu_bdd_identity(f->mgr, f->node));
}

void
bdd_free(bdd_t *f)
{
  if (f == NIL(bdd_t)) {
	fail("bdd_free: trying to free a NIL bdd_t");			
  }
  
  if (f->free == TRUE) {
	fail("bdd_free: trying to free a freed bdd_t");			
  }	
  
  cmu_bdd_free(f->mgr, f->node);
  
  /*
   * In case the user tries to free this bdd_t again, set the free field to TRUE, 
     * and NIL out the other fields.  Then free the bdd_t structure itself.
     */
  f->free = TRUE;
  f->node = NIL(struct bdd_);
  f->mgr = NIL(struct bdd_manager_);
  FREE(f);  
}

/*
  Operations on BDD Formulas ----------------------------------------------------
  */

bdd_t *
bdd_and(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase)
{
  struct bdd_ *temp1, *temp2;
  bdd_t *result;
  struct bdd_manager_ *mgr;
  
  mgr = f->mgr;
  temp1 = ( (f_phase == TRUE) ? cmu_bdd_identity(mgr, f->node) : cmu_bdd_not(mgr, f->node));
  temp2 = ( (g_phase == TRUE) ? cmu_bdd_identity(mgr, g->node) : cmu_bdd_not(mgr, g->node));
  result = bdd_construct_bdd_t(mgr, cmu_bdd_and(mgr, temp1, temp2));
  cmu_bdd_free(mgr, temp1);
  cmu_bdd_free(mgr, temp2);
  return result;
}

bdd_t *
bdd_and_with_limit(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase,
  unsigned int limit)
{
  /* Unsupported: fall back on standard AND. */
  return bdd_and(f, g, f_phase, g_phase);
}

bdd_t *
bdd_and_array(
  bdd_t *f,
  array_t *g_array,
  boolean f_phase,
  boolean g_phase)
{
  struct bdd_ *temp1, *temp2, *result;
  bdd_t *g;
  struct bdd_manager_ *mgr;
  int i;

  mgr = f->mgr;
  result = ((f_phase == TRUE) ? cmu_bdd_identity(mgr, f->node) :
            cmu_bdd_not(mgr, f->node)); 

  for (i = 0; i < array_n(g_array); i++) {
    g = array_fetch(bdd_t *, g_array, i);
    temp1 = result;
    temp2 = ((g_phase == TRUE) ? cmu_bdd_identity(mgr, g->node) :
             cmu_bdd_not(mgr, g->node));
    result = cmu_bdd_and(mgr, temp1, temp2);
    cmu_bdd_free(mgr, temp1);
    cmu_bdd_free(mgr, temp2);
    if (result == NULL)
      return(NULL);
  }

  return(bdd_construct_bdd_t(mgr, result));
}

bdd_t *
bdd_multiway_and(bdd_manager *manager, array_t *bddArray)
{
  int i;
  bdd temp, result;
  bdd_t *operand;
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  result = cmu_bdd_one(mgr);
  for (i=0; i<array_n(bddArray); i++){
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = cmu_bdd_and(mgr, result, operand->node);
    cmu_bdd_free(mgr, result);
    result = temp;
  }
  return bdd_construct_bdd_t(mgr, result);
}

bdd_t *
bdd_multiway_or(bdd_manager *manager, array_t *bddArray)
{
  int i;
  bdd temp, result;
  bdd_t *operand;
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  result = cmu_bdd_zero(mgr);
  for (i=0; i<array_n(bddArray); i++){
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = cmu_bdd_or(mgr, result, operand->node);
    cmu_bdd_free(mgr, result);
    result = temp;
  }
  return bdd_construct_bdd_t(mgr, result);
}

bdd_t *
bdd_multiway_xor(bdd_manager *manager, array_t *bddArray)
{
  int i;
  bdd temp, result;
  bdd_t *operand;
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  result = cmu_bdd_zero(mgr);
  for (i=0; i<array_n(bddArray); i++){
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = cmu_bdd_xor(mgr, result, operand->node);
    cmu_bdd_free(mgr, result);
    result = temp;
  }
  return bdd_construct_bdd_t(mgr, result);
}

array_t *
bdd_pairwise_or(bdd_manager *manager, array_t *bddArray1, array_t
                *bddArray2) 
{
  int i;
  bdd_t *operand1, *operand2;
  array_t *resultArray;
  bdd result;
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  if (array_n(bddArray1) != array_n(bddArray2)){
    fprintf(stderr, "bdd_pairwise_or: Arrays of different lengths.\n");
    return NIL(array_t);
  }
  resultArray = array_alloc(bdd_t *, 0);
  for (i=0; i<array_n(bddArray1); i++){
    operand1 = array_fetch(bdd_t *, bddArray1, i);
    operand2 = array_fetch(bdd_t *, bddArray2, i);
    result = cmu_bdd_or(mgr, operand1->node, operand2->node);
    array_insert_last(bdd_t*, resultArray,
                      bdd_construct_bdd_t(mgr, result)); 
  }
  return resultArray;
}

array_t *
bdd_pairwise_and(bdd_manager *manager, array_t *bddArray1, array_t
                 *bddArray2) 
{
  int i;
  bdd_t *operand1, *operand2;
  array_t *resultArray;
  bdd result;
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  if (array_n(bddArray1) != array_n(bddArray2)){
    fprintf(stderr, "bdd_pairwise_and: Arrays of different lengths.\n");
    return NIL(array_t);
  }
  resultArray = array_alloc(bdd_t *, 0);
  for (i=0; i<array_n(bddArray1); i++){
    operand1 = array_fetch(bdd_t *, bddArray1, i);
    operand2 = array_fetch(bdd_t *, bddArray2, i);
    result = cmu_bdd_and(mgr, operand1->node, operand2->node);
    array_insert_last(bdd_t*, resultArray,
                      bdd_construct_bdd_t(mgr, result)); 
  }
  return resultArray;
}

array_t *
bdd_pairwise_xor(bdd_manager *manager, array_t *bddArray1, array_t
                 *bddArray2) 
{
  int i;
  bdd_t *operand1, *operand2;
  array_t *resultArray;
  bdd result;
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  if (array_n(bddArray1) != array_n(bddArray2)){
    fprintf(stderr, "bdd_pairwise_xor: Arrays of different lengths.\n");
    return NIL(array_t);
  }
  resultArray = array_alloc(bdd_t *, 0);
  for (i=0; i<array_n(bddArray1); i++){
    operand1 = array_fetch(bdd_t *, bddArray1, i);
    operand2 = array_fetch(bdd_t *, bddArray2, i);
    result = cmu_bdd_xor(mgr, operand1->node, operand2->node);
    array_insert_last(bdd_t*, resultArray,
                      bdd_construct_bdd_t(mgr, result)); 
  }
  return resultArray;
}

bdd_t *
bdd_and_smooth(
  bdd_t *f,
  bdd_t *g,
  array_t *smoothing_vars /* of bdd_t *'s */)
{
  int num_vars, i;
  bdd_t *fn, *result;
  struct bdd_ **assoc;
  struct bdd_manager_ *mgr;
  
  num_vars = array_n(smoothing_vars);
  if (num_vars <= 0) {
	cmu_bdd_fatal("bdd_and_smooth: no smoothing variables");
  }
  
  assoc = ALLOC(struct bdd_ *, num_vars+1);
  
  for (i = 0; i < num_vars; i++) {
	fn = array_fetch(bdd_t *, smoothing_vars, i);
	assoc[i] = fn->node;
  }
  assoc[num_vars] = (struct bdd_ *) 0;
  
  mgr = f->mgr;
  cmu_bdd_temp_assoc(mgr, assoc, 0);
  (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current association */
  
  result = bdd_construct_bdd_t(mgr, cmu_bdd_rel_prod(mgr, f->node, g->node));
  FREE(assoc);
  return result;
}

bdd_t *
bdd_and_smooth_with_limit(
  bdd_t *f,
  bdd_t *g,
  array_t *smoothing_vars /* of bdd_t *'s */,
  unsigned int limit)
{
  /* Unsupported: fall back to standard and_smooth */
  return bdd_and_smooth(f, g, smoothing_vars);
}

bdd_t *
bdd_between(bdd_t *f_min, bdd_t *f_max)
{
  bdd_t *temp, *ret;
  long size1, size2, size3; 
  temp = bdd_or(f_min, f_max, 1, 0);
  ret = bdd_minimize(f_min, temp);
  bdd_free(temp);
  size1 = bdd_size(f_min);
  size2 = bdd_size(f_max);
  size3 = bdd_size(ret);
  if (size3 < size1) {
	if (size3 < size2){
      return ret;
    }
    else {
      bdd_free(ret);
      return bdd_dup(f_max);
    }
  }
  else {
    bdd_free(ret);
    if (size1 < size2){
      return bdd_dup(f_min);
    }
    else {
      return bdd_dup(f_max);
    }
  }
}

bdd_t *
bdd_cofactor(bdd_t *f, bdd_t *g)
{
  return bdd_construct_bdd_t(f->mgr, cmu_bdd_cofactor(f->mgr, f->node, g->node));
}

bdd_t *
bdd_cofactor_array(bdd_t *f, array_t *bddArray)
{
  bdd_t *operand;
  struct bdd_ *result, *temp;
  int i;

  result = cmu_bdd_identity(f->mgr, f->node);

  for (i = 0; i < array_n(bddArray); i++) {
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = cmu_bdd_cofactor(f->mgr, result, operand->node);
    if (temp == NULL) {
      cmu_bdd_free(f->mgr, result);
      return(NULL);
    }
    cmu_bdd_free(f->mgr, result);
    result = temp;
  }

  return(bdd_construct_bdd_t(f->mgr, result));
}

bdd_t *
bdd_compose(
  bdd_t *f,
  bdd_t *v,
  bdd_t *g)
{
  return bdd_construct_bdd_t(f->mgr, cmu_bdd_compose(f->mgr, f->node, v->node, g->node));
}

bdd_t *
bdd_consensus(
  bdd_t *f,
  array_t *quantifying_vars /* of bdd_t *'s */)
{
  int num_vars, i;
  bdd_t *fn, *result;
  struct bdd_ **assoc;
  struct bdd_manager_ *mgr;
  
  num_vars = array_n(quantifying_vars);
  if (num_vars <= 0) {
	cmu_bdd_fatal("bdd_consensus: no quantifying variables");
  }
  
  assoc = ALLOC(struct bdd_ *, num_vars+1);
  
  for (i = 0; i < num_vars; i++) {
	fn = array_fetch(bdd_t *, quantifying_vars, i);
	assoc[i] = fn->node;
  }
  assoc[num_vars] = (struct bdd_ *) 0;
  
  mgr = f->mgr;
  cmu_bdd_temp_assoc(mgr, assoc, 0);
  (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current association */
  
  result = bdd_construct_bdd_t(mgr, cmu_bdd_forall(mgr, f->node));
  FREE(assoc);
  return result;
}


bdd_t *
bdd_cproject(
  bdd_t *f,
  array_t *quantifying_vars /* of bdd_t* */)
{
  int num_vars, i;
  bdd_t *fn, *result;
  struct bdd_ **assoc;
  struct bdd_manager_ *mgr;
  
  if (f == NIL(bdd_t)) fail ("bdd_cproject: invalid BDD");
  
  num_vars = array_n(quantifying_vars);
  if (num_vars <= 0) {
    printf("Warning: bdd_cproject: no projection variables\n");
    result = bdd_dup(f);
  }
  else {
    assoc = ALLOC(struct bdd_ *, num_vars+1);
    for (i = 0; i < num_vars; i++) {
      fn = array_fetch(bdd_t *, quantifying_vars, i);
      assoc[i] = fn->node;
    }
    assoc[num_vars] = (struct bdd_ *) 0;
    mgr = f->mgr;
    cmu_bdd_temp_assoc(mgr, assoc, 0);
    (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current a
                                       ssociation */
    
    result = bdd_construct_bdd_t(mgr, cmu_bdd_project(mgr, f->node));
    FREE(assoc);
  }
  return result;
}


bdd_t *
bdd_else(bdd_t *f)
{
  return bdd_construct_bdd_t(f->mgr, cmu_bdd_else(f->mgr, f->node));
}


bdd_t *
bdd_ite(
  bdd_t *i,
  bdd_t *t,
  bdd_t *e,
  boolean i_phase,
  boolean t_phase,
  boolean e_phase)
{
  struct bdd_ *temp1, *temp2, *temp3;
  bdd_t *result;
  struct bdd_manager_ *mgr;
  
  mgr = i->mgr;
  temp1 = ( (i_phase == TRUE) ? cmu_bdd_identity(mgr, i->node) : cmu_bdd_not(mgr, i->node));
  temp2 = ( (t_phase == TRUE) ? cmu_bdd_identity(mgr, t->node) : cmu_bdd_not(mgr, t->node));
  temp3 = ( (e_phase == TRUE) ? cmu_bdd_identity(mgr, e->node) : cmu_bdd_not(mgr, e->node));
  result = bdd_construct_bdd_t(mgr, cmu_bdd_ite(mgr, temp1, temp2, temp3));
  cmu_bdd_free(mgr, temp1);
  cmu_bdd_free(mgr, temp2);
  cmu_bdd_free(mgr, temp3);
  return result;
}

bdd_t *
bdd_minimize(bdd_t *f, bdd_t *c)
{
  bdd_t *result = bdd_construct_bdd_t(f->mgr, cmu_bdd_reduce(f->mgr,
                                                             f->node,
                                                             c->node)); 
  if (bdd_size(result) < bdd_size(f)){
    return result;
  }
  else{
    bdd_free(result);
    return bdd_dup(f);
  }
}

bdd_t *
bdd_minimize_array(bdd_t *f, array_t *bddArray)
{
  bdd_t *operand;
  struct bdd_ *result, *temp;
  bdd_t *final;
  int i;

  result = cmu_bdd_identity(f->mgr, f->node);
  for (i = 0; i < array_n(bddArray); i++) {
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = cmu_bdd_reduce(f->mgr, result, operand->node);
    if (temp == NULL) {
      cmu_bdd_free(f->mgr, result);
      return(NULL);
    }
    cmu_bdd_free(f->mgr, result);
    result = temp;
  }

  final = bdd_construct_bdd_t(f->mgr, result);

  if (bdd_size(final) < bdd_size(f)){
    return final;
  }
  else{
    bdd_free(final);
    return bdd_dup(f);
  }
}


bdd_t *
bdd_not(bdd_t *f)
{
  return bdd_construct_bdd_t(f->mgr, cmu_bdd_not(f->mgr, f->node));
}

bdd_t *
bdd_one(bdd_manager *manager)
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  return bdd_construct_bdd_t(mgr, cmu_bdd_one(mgr));
}

bdd_t *
bdd_or(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase)
{
  struct bdd_ *temp1, *temp2;
  bdd_t *result;
  struct bdd_manager_ *mgr;
  
  mgr = f->mgr;
  temp1 = ( (f_phase == TRUE) ? cmu_bdd_identity(mgr, f->node) : cmu_bdd_not(mgr, f->node));
  temp2 = ( (g_phase == TRUE) ? cmu_bdd_identity(mgr, g->node) : cmu_bdd_not(mgr, g->node));
  result = bdd_construct_bdd_t(mgr, cmu_bdd_or(mgr, temp1, temp2));
  cmu_bdd_free(mgr, temp1);
  cmu_bdd_free(mgr, temp2);
  return result;
}

bdd_t *
bdd_smooth(
  bdd_t *f,
  array_t *smoothing_vars /* of bdd_t *'s */)
{
  int num_vars, i;
  bdd_t *fn, *result;
  struct bdd_ **assoc;
  struct bdd_manager_ *mgr;
  
  num_vars = array_n(smoothing_vars);
  if (num_vars <= 0) {
	cmu_bdd_fatal("bdd_smooth: no smoothing variables");
  }
  
  assoc = ALLOC(struct bdd_ *, num_vars+1);
  
  for (i = 0; i < num_vars; i++) {
	fn = array_fetch(bdd_t *, smoothing_vars, i);
	assoc[i] = fn->node;
  }
  assoc[num_vars] = (struct bdd_ *) 0;
  
  mgr = f->mgr;
  cmu_bdd_temp_assoc(mgr, assoc, 0);
  (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current association */
  
  result = bdd_construct_bdd_t(mgr, cmu_bdd_exists(mgr, f->node));
  FREE(assoc);
  return result;
}

bdd_t *
bdd_substitute(
  bdd_t *f,
  array_t *old_array /* of bdd_t *'s */,
  array_t *new_array /* of bdd_t *'s */)
{
    int num_old_vars, num_new_vars, i;
    bdd_t *fn_old, *fn_new, *result;
    struct bdd_ **assoc;
    struct bdd_manager_ *mgr;

    num_old_vars = array_n(old_array);
    num_new_vars = array_n(new_array);
    if (num_old_vars != num_new_vars) {
	cmu_bdd_fatal("bdd_substitute: mismatch of number of new and old variables");
    }

    assoc = ALLOC(struct bdd_ *, 2*(num_old_vars+1));

    for (i = 0; i < num_old_vars; i++) {
	fn_old = array_fetch(bdd_t *, old_array, i);
	fn_new = array_fetch(bdd_t *, new_array, i);
	assoc[2*i]   = fn_old->node;
	assoc[2*i+1] = fn_new->node;
    }
    assoc[2*num_old_vars]   = (struct bdd_ *) 0;
    assoc[2*num_old_vars+1] = (struct bdd_ *) 0;  /* not sure if we need this second 0 */

    mgr = f->mgr;
    cmu_bdd_temp_assoc(mgr, assoc, 1);
    (void) cmu_bdd_assoc(mgr, -1);  /* set the temp association as the current association */

    result = bdd_construct_bdd_t(mgr, cmu_bdd_substitute(mgr, f->node));
    FREE(assoc);
    return result;
}

array_t *
bdd_substitute_array(
  array_t *f_array,
  array_t *old_array /* of bdd_t *'s */,
  array_t *new_array /* of bdd_t *'s */)
{
  int	i;
  bdd_t	*f, *new_;
  array_t *substitute_array = array_alloc(bdd_t *, 0);

  arrayForEachItem(bdd_t *, f_array, i, f) {
    new_ = bdd_substitute(f, old_array, new_array);
    array_insert_last(bdd_t *, substitute_array, new_);
  }
  return(substitute_array);
}

void *
bdd_pointer(bdd_t *f)
{
    return((void *)f->node);
}

bdd_t *
bdd_then(bdd_t *f)
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_then(f->mgr, f->node));
}

bdd_t *
bdd_top_var(bdd_t *f)
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_if(f->mgr, f->node));
}

bdd_t *
bdd_xnor(bdd_t *f, bdd_t *g)
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_xnor(f->mgr, f->node, g->node));
}

bdd_t *
bdd_xor(bdd_t *f, bdd_t *g)
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_xor(f->mgr, f->node, g->node));
}

bdd_t *
bdd_zero(bdd_manager *manager)
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  return bdd_construct_bdd_t(mgr, cmu_bdd_zero(mgr));
}

/*
Queries about BDD Formulas ----------------------------------------------------
*/

boolean
bdd_equal(bdd_t *f, bdd_t *g)
{
    return (f->node == g->node);
}

boolean
bdd_equal_mod_care_set(bdd_t *f, bdd_t *g, bdd_t *CareSet)
{
  bdd_t	*diffBdd;
  boolean result;

  if (bdd_equal(f, g))
    return 1;

  diffBdd = bdd_xor(f, g);

  result = bdd_leq(diffBdd, CareSet, 1, 0);
  bdd_free(diffBdd);

  return(result);
}

bdd_t *
bdd_intersects(bdd_t *f, bdd_t *g)
{
    return bdd_construct_bdd_t(f->mgr, cmu_bdd_intersects(f->mgr, f->node, g->node));
}

bdd_t *
bdd_closest_cube(bdd_t *f, bdd_t *g, int *dist)
{
    return (NULL);
}

boolean
bdd_is_tautology(bdd_t *f, boolean phase)
{
    return ((phase == TRUE) ? (f->node == cmu_bdd_one(f->mgr)) : (f->node == cmu_bdd_zero(f->mgr)));
}

boolean
bdd_leq(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase)
{
    struct bdd_ *temp1, *temp2, *implies_fn;
    struct bdd_manager_ *mgr;
    boolean result_value;

    mgr = f->mgr;
    temp1 = ( (f_phase == TRUE) ? cmu_bdd_identity(mgr, f->node) : cmu_bdd_not(mgr, f->node));
    temp2 = ( (g_phase == TRUE) ? cmu_bdd_identity(mgr, g->node) : cmu_bdd_not(mgr, g->node));
    implies_fn = cmu_bdd_implies(mgr, temp1, temp2); /* returns a minterm of temp1*!temp2 */
    result_value = (implies_fn == cmu_bdd_zero(mgr));
    cmu_bdd_free(mgr, temp1);
    cmu_bdd_free(mgr, temp2);
    cmu_bdd_free(mgr, implies_fn);
    return result_value;
}

boolean
bdd_lequal_mod_care_set(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase,
  bdd_t *careSet)
{
  bdd_t *temp;
  boolean result;

  if (bdd_leq(f, g, f_phase, g_phase))
    return 1;

  temp = bdd_and(f, careSet, f_phase, 1);

  result = bdd_leq(temp, g, 1, g_phase);
  bdd_free(temp);

  return(result);
}

boolean
bdd_leq_array(
  bdd_t *f,
  array_t *g_array,
  boolean f_phase,
  boolean g_phase)
{
  int	i;
  bdd_t	*g;
  boolean result;

  arrayForEachItem(bdd_t *, g_array, i, g) {
    result = bdd_leq(f, g, f_phase, g_phase);
    if (g_phase) {
      if (!result)
	return(0);
    } else {
      if (result)
	return(1);
    }
  }
  if (g_phase)
    return(1);
  else
    return(0);
}

/*
Statistics and Other Queries --------------------------------------------------
*/

double 
bdd_count_onset(
  bdd_t *f,
  array_t *var_array /* of bdd_t *'s */)
{
    int num_vars;
    double fraction;

    num_vars = array_n(var_array);
    fraction = cmu_bdd_satisfying_fraction(f->mgr, f->node); /* cannot give support vars */
    return (fraction * pow((double) 2, (double) num_vars));
}

/**Function********************************************************************

  Synopsis    [Counts the number of minterms in the on set.]

  SideEffects []

******************************************************************************/
int
bdd_epd_count_onset(
  bdd_t *f,
  array_t *var_array /* of bdd_t *'s */,
  EpDouble *epd)
{
  double nMinterms;

  nMinterms = bdd_count_onset(f, var_array);
  EpdConvert(nMinterms, epd);
  return 0;
} /* end of bdd_epd_count_onset */

int
bdd_get_free(bdd_t *f)
{
    return (f->free);
}

bdd_manager *
bdd_get_manager(bdd_t *f)
{
    return (bdd_manager *) (f->mgr);
}

bdd_node *
bdd_get_node(
  bdd_t *f,
  boolean *is_complemented /* return */)
{
    *is_complemented = (boolean) TAG0(f->node);  /* using bddint.h */
    return ((bdd_node *) BDD_POINTER(f->node));  /* using bddint.h */
}

var_set_t *
bdd_get_support(bdd_t *f)
{
    struct bdd_ **support, *var;
    struct bdd_manager_ *mgr;
    long num_vars;
    var_set_t *result;
    int id, i;

    mgr = f->mgr;
    num_vars = cmu_bdd_vars(mgr);

    result = var_set_new((int) num_vars);
    support = (struct bdd_ **) mem_get_block((num_vars+1) * sizeof(struct bdd_ *));
    (void) cmu_bdd_support(mgr, f->node, support);

    for (i = 0; i < num_vars; ++i) {  /* can never have more than num_var non-zero entries in array */
	var = support[i]; 
	if (var == (struct bdd_ *) 0) {
	    break;  /* have reach end of null-terminated array */
	}
	id = (int) (cmu_bdd_if_id(mgr, var) - 1);  /* a variable is never garbage collected, so no need to free */
	var_set_set_elt(result, id);
    }

    mem_free_block((pointer)support);

    return result;
}

int
bdd_is_support_var(bdd_t *f, bdd_t *var)
{
    return(bdd_is_support_var_id(f, bdd_top_var_id(var)));
}

int
bdd_is_support_var_id(bdd_t *f, int index)
{
    struct bdd_ **support, *var;
    struct bdd_manager_ *mgr;
    long num_vars;
    int id, i;

    mgr = f->mgr;
    num_vars = cmu_bdd_vars(mgr);

    support = (struct bdd_ **) mem_get_block((num_vars+1) * sizeof(struct bdd_ *));
    (void) cmu_bdd_support(mgr, f->node, support);

    for (i = 0; i < num_vars; ++i) {  /* can never have more than num_var non-zero entries in array */
	var = support[i]; 
	if (var == (struct bdd_ *) 0) {
	    break;  /* have reach end of null-terminated array */
	}
	id = (int) (cmu_bdd_if_id(mgr, var) - 1);  /* a variable is never garbage collected, so no need to free */
	if (id == index) {
	    mem_free_block((pointer)support);
	    return 1;
	}
    }

    mem_free_block((pointer)support);

    return 0;
}

array_t *
bdd_get_varids(array_t *var_array)
{
  int i;
  bdd_t *var;
  array_t *result;
 
  result = array_alloc(bdd_variableId, 0);
  for (i = 0; i < array_n(var_array); i++) {
    var = array_fetch(bdd_t *, var_array, i);
    array_insert_last(bdd_variableId, result, bdd_top_var_id(var));
  }
  return result;
}

unsigned int 
bdd_num_vars(bdd_manager *manager)
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  return (cmu_bdd_vars(mgr));
}

void
bdd_print(bdd_t *f)
{
    cmu_bdd_print_bdd(f->mgr, f->node, bdd_naming_fn_none, bdd_terminal_id_fn_none, (pointer) 0, stdout);
}

void
bdd_print_stats(bdd_manager *manager, FILE *file)
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  cmu_bdd_stats(mgr, file);
}

/**Function********************************************************************

  Synopsis [Sets the internal parameters of the package to the given values.]

  SideEffects []

******************************************************************************/
int
bdd_set_parameters(
  bdd_manager *mgr,
  avl_tree *valueTable,
  FILE *file)
{
  (void) fprintf(file, "Functionality not supported yet in the CMU package\n");
  return 1;
} /* End of bdd_set_parameters */

int
bdd_size(bdd_t *f)
{
    return ((int) cmu_bdd_size(f->mgr, f->node, 1));
}

int
bdd_node_size(bdd_node *f)
{
  return(0);
}

long
bdd_size_multiple(array_t *bdd_array)
{
    long result;
    struct bdd_ **vector_bdd;
    bdd_t *f;
    int i;
    struct bdd_manager_ *mgr;

    if ((bdd_array == NIL(array_t)) || (array_n(bdd_array) == 0))
        return 0;

    f = array_fetch(bdd_t*, bdd_array, 0);
    mgr = f->mgr;

    vector_bdd = (struct bdd_ **)
                        malloc((array_n(bdd_array)+1)*sizeof(struct bdd_ *));

    for(i=0; i<array_n(bdd_array);i++){
        f = array_fetch(bdd_t*, bdd_array, i);
        vector_bdd[i] = f->node;
    }
    vector_bdd[array_n(bdd_array)] = 0;
    result =  cmu_bdd_size_multiple(mgr, vector_bdd,1);
    FREE(vector_bdd);
    return result;
}

bdd_variableId
bdd_top_var_id(bdd_t *f)
{
    return ((bdd_variableId) (cmu_bdd_if_id(f->mgr, f->node) - 1));
}

/*
Miscellaneous -----------------------------------------------------------------
*/

bdd_external_hooks *
bdd_get_external_hooks(bdd_manager *manager)
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  return ((bdd_external_hooks *) mgr->hooks);
}


void
bdd_set_gc_mode(bdd_manager *manager, boolean no_gc)
{
  cmu_bdd_warning("bdd_set_gc_mode: translated to no-op in CMU package");
}

void 
bdd_dynamic_reordering(bdd_manager *manager, bdd_reorder_type_t
                       algorithm_type, bdd_reorder_verbosity_t verbosity) 
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
    switch(algorithm_type) {
    case BDD_REORDER_SIFT:
	cmu_bdd_dynamic_reordering(mgr, cmu_bdd_reorder_sift);
	break;
    case BDD_REORDER_WINDOW:
	cmu_bdd_dynamic_reordering(mgr, cmu_bdd_reorder_stable_window3);
	break;
    case BDD_REORDER_NONE:
	cmu_bdd_dynamic_reordering(mgr, cmu_bdd_reorder_none);
	break;
    default:
      fprintf(stderr,"CMU: bdd_dynamic_reordering: unknown algorithm type\n");
      fprintf(stderr,"Using SIFT method instead\n");
      cmu_bdd_dynamic_reordering(mgr, cmu_bdd_reorder_sift);
    }
}

void 
bdd_dynamic_reordering_zdd(bdd_manager *manager, bdd_reorder_type_t
                       algorithm_type, bdd_reorder_verbosity_t verbosity) 
{
    return;
}

void 
bdd_reorder(bdd_manager *manager)
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  cmu_bdd_reorder(mgr);
}

bdd_variableId
bdd_get_id_from_level(bdd_manager *manager, long level)
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  struct bdd_ *fn;

  fn = cmu_bdd_var_with_index(mgr, level);
  
  if (fn == (struct bdd_ *) 0) {
    /* variable should always be found, since they are created at bdd_start */
    cmu_bdd_fatal("bdd_get_id_from_level: assumption violated");
  }
  
  return ((bdd_variableId)(cmu_bdd_if_id(mgr, fn) - 1 ));
  
}

long
bdd_top_var_level(bdd_manager *manager, bdd_t *fn)
{
  cmu_bdd_manager mgr = (cmu_bdd_manager) manager;
  return cmu_bdd_if_index(mgr, fn->node);
}

/*
 * Return TRUE if f is a cube, else return FALSE.
 */
boolean
bdd_is_cube(bdd_t *f)
{
  struct bdd_manager_ *manager;

  if (f == NIL(bdd_t)) {
        fail("bdd_is_cube: invalid BDD");
  }
  if(  f->free ) fail ("Freed Bdd passed to bdd_is_cube");
  manager = f->mgr;
  return ((boolean)cmu_bdd_is_cube(manager, f->node));
}

bdd_block *
bdd_new_var_block(bdd_t *f, long length)
{
  struct bdd_manager_ *manager;
  if (f == NIL(bdd_t)) {
        fail("bdd_new_var_block: invalid BDD");
  }
  manager = f->mgr;
  return (bdd_block *)cmu_bdd_new_var_block(manager, f->node, length);
}

bdd_t *
bdd_var_with_index(bdd_manager *manager, int index)
{
  return bdd_construct_bdd_t(manager, 
		            cmu_bdd_var_with_index((cmu_bdd_manager) manager,
                                                   index));
}

bdd_t *
bdd_compact(bdd_t *f, bdd_t *g)
{
    return (NULL);
}


bdd_t *
bdd_squeeze(bdd_t *f, bdd_t *g)
{
    return (NULL);
}

double
bdd_correlation(bdd_t *f, bdd_t *g)
{
    return (0.0);
}

/**Function********************************************************************

  Synopsis    [Dummy functions defined in bdd.h]

  SideEffects []

******************************************************************************/
int
bdd_reordering_status(
  bdd_manager *mgr,
  bdd_reorder_type_t *method)
{
  return 0;
}

/**Function********************************************************************

  Synopsis    [Dummy functions defined in bdd.h]

  SideEffects []

******************************************************************************/

bdd_t *
bdd_compute_cube(
  bdd_manager *mgr,
  array_t *vars)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_compute_cube_with_phase(
  bdd_manager *mgr,
  array_t *vars,
  array_t *phase)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_clipping_and_smooth(
  bdd_t *f,
  bdd_t *g,
  array_t *smoothing_vars /* of bdd_t *'s */,
  int maxDepth,
  int over)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_approx_hb(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_approx_sp(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold,
  int hardlimit)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_approx_ua(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold,
  int safe,
  double quality)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_approx_remap_ua(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold,
  double quality)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_approx_biased_rua(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  bdd_t *bias,
  int numVars,
  int threshold,
  double quality,
  double quality1)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_approx_compress(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold)
{
  return NIL(bdd_t);
}

int
bdd_gen_decomp(
  bdd_t *f,
  bdd_partition_type_t partType,
  bdd_t ***conjArray)
{
  return 0;
}

int
bdd_var_decomp(
  bdd_t *f,
  bdd_partition_type_t partType,
  bdd_t ***conjArray)
{
  return 0;
}

int 
bdd_approx_decomp(
  bdd_t *f,
  bdd_partition_type_t partType,
  bdd_t ***conjArray)
{
  return 0;
}

int
bdd_iter_decomp(
  bdd_t *f,
  bdd_partition_type_t partType,
  bdd_t  ***conjArray)
{
  return 0;
}

bdd_t *
bdd_solve_eqn(
  bdd_t *f,
  array_t *g,
  array_t *unknowns)
{
  return NIL(bdd_t);
}

int
bdd_add_hook(
  bdd_manager *mgr,
  int (*procedure)(bdd_manager *, char *, void *),
  bdd_hook_type_t whichHook)
{
  return 0;
}

int
bdd_remove_hook(
  bdd_manager *mgr,
  int (*procedure)(bdd_manager *, char *, void *),
  bdd_hook_type_t whichHook)
{
  return 0;
}

int
bdd_enable_reordering_reporting(bdd_manager *mgr)
{
  return 0;
}

int
bdd_disable_reordering_reporting(bdd_manager *mgr)
{
  return 0;
}

int
bdd_enable_ordering_monitoring(bdd_manager *mgr)
{
  return 0;
}

int
bdd_disable_ordering_monitoring(bdd_manager *mgr)
{
  return 0;
}

bdd_reorder_verbosity_t 
bdd_reordering_reporting(bdd_manager *mgr)
{
  return BDD_REORDER_VERBOSITY_DEFAULT;
}

int 
bdd_print_apa_minterm(
  FILE *fp,
  bdd_t *f,
  int nvars,
  int precision)
{
  return 0;
}

int 
bdd_apa_compare_ratios(
  int nvars,
  bdd_t *f1,
  bdd_t *f2,
  int f1Num,
  int f2Num)
{
  return 0;
}

int
bdd_read_node_count(bdd_manager *mgr)
{
  return 0;
}


int
bdd_reordering_zdd_status(
  bdd_manager *mgr,
  bdd_reorder_type_t *method)
{
  return 0;
}


bdd_node *
bdd_bdd_to_add(
  bdd_manager *mgr,
  bdd_node *fn)
{
  return NIL(bdd_node);
}

bdd_node *
bdd_add_permute(
  bdd_manager *mgr,
  bdd_node *fn,
  int *permut)
{
  return NIL(bdd_node);
}

bdd_node *
bdd_bdd_permute(
  bdd_manager *mgr,
  bdd_node *fn,
  int *permut)
{
  return NIL(bdd_node);
}

void
bdd_ref(bdd_node *fn)
{
  return ;
}


void
bdd_recursive_deref(bdd_manager *mgr, bdd_node *f)
{
  return;
}


bdd_node *
bdd_add_exist_abstract(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node *vars)
{
  return NIL(bdd_node);
}


bdd_node *
bdd_add_apply(
  bdd_manager *mgr,
  bdd_node *(*operation)(bdd_manager *, bdd_node **, bdd_node **),
  bdd_node *fn1,
  bdd_node *fn2)
{
  return NIL(bdd_node);
}


bdd_node *
bdd_add_nonsim_compose(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node **vector)
{
  return NIL(bdd_node);
}


bdd_node *
bdd_add_residue(
  bdd_manager *mgr,
  int n,
  int m,
  int options,
  int top)
{
  return NIL(bdd_node);
}


bdd_node *
bdd_add_vector_compose(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node **vector)
{
  return NIL(bdd_node);
}


bdd_node *
bdd_add_times(
  bdd_manager *mgr,
  bdd_node **fn1,
  bdd_node **fn2)
{
  return NIL(bdd_node);
}
  

int
bdd_check_zero_ref(bdd_manager *mgr)
{
  return 0;
}


void
bdd_dynamic_reordering_disable(bdd_manager *mgr)
{
  return;
}

void
bdd_dynamic_reordering_zdd_disable(bdd_manager *mgr)
{
  return;
}


bdd_node *
bdd_add_xnor(
  bdd_manager *mgr,
  bdd_node **fn1,
  bdd_node **fn2)
{
  return NIL(bdd_node);
}


int
bdd_shuffle_heap(
  bdd_manager *mgr,
  int *permut)
{
  return 0;
}


bdd_node *
bdd_add_compose(
  bdd_manager *mgr,
  bdd_node *fn1,
  bdd_node *fn2,
  int var)
{
  return NIL(bdd_node);
}


bdd_node *
bdd_add_ith_var(
  bdd_manager *mgr,
  int i)
{
  return NIL(bdd_node);
}


int
bdd_get_level_from_id(
  bdd_manager *mgr,
  int id)
{
  return 0;
}


bdd_node *
bdd_bdd_exist_abstract(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node *cube)
{
  return NIL(bdd_node);
}


int
bdd_equal_sup_norm(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node *gn,
  BDD_VALUE_TYPE tolerance,
  int pr)
{
  return 0;
}


bdd_node *
bdd_read_logic_zero(bdd_manager *mgr)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_bdd_ith_var(bdd_manager *mgr, int i)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_add_divide(
  bdd_manager *mgr,
  bdd_node **fn1,
  bdd_node **fn2)
{
  return NIL(bdd_node);
}


bdd_node *
bdd_bdd_constrain(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *c)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_bdd_restrict(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *c)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_add_hamming(
  bdd_manager *mgr,
  bdd_node **xVars,
  bdd_node **yVars,
  int nVars)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_add_ite(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g,
  bdd_node *h)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_add_find_max(
  bdd_manager *mgr,
  bdd_node *f)
{
    return NIL(bdd_node);
}


int
bdd_bdd_pick_one_cube(
  bdd_manager *mgr,
  bdd_node *node,
  char *string)
{
    return 0;
}


bdd_node *
bdd_add_swap_variables(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **x,
  bdd_node **y,
  int n)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_bdd_swap_variables(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **x,
  bdd_node **y,
  int n)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_bdd_or(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_bdd_compute_cube(
  bdd_manager *mgr,
  bdd_node **vars,
  int *phase,
  int n)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_indices_to_cube(
  bdd_manager *mgr,
  int *idArray,
  int n)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_bdd_and(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_add_matrix_multiply(
  bdd_manager *mgr,
  bdd_node *A,
  bdd_node *B,
  bdd_node **z,
  int nz)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_add_compute_cube(
  bdd_manager *mgr,
  bdd_node **vars,
  int *phase,
  int n)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_add_const(
  bdd_manager *mgr,
  BDD_VALUE_TYPE c)
{
    return NIL(bdd_node);
}


double
bdd_count_minterm(
  bdd_manager *mgr,
  bdd_node *f,
  int n)
{
    return 0;
}


bdd_node *
bdd_add_bdd_threshold(
  bdd_manager *mgr,
  bdd_node *f,
  BDD_VALUE_TYPE value)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_add_bdd_strict_threshold(
  bdd_manager *mgr,
  bdd_node *f,
  BDD_VALUE_TYPE value)
{
    return NIL(bdd_node);
}

BDD_VALUE_TYPE
bdd_read_epsilon(bdd_manager *mgr)
{
    return 0;
}

bdd_node *
bdd_read_one(bdd_manager *mgr)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_bdd_pick_one_minterm(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **vars,
  int n)
{
    return NIL(bdd_node);
}


bdd_t *
bdd_pick_one_minterm(
  bdd_t *f,
  array_t *varsArray)
{
    return NIL(bdd_t);
}


array_t *
bdd_bdd_pick_arbitrary_minterms(
  bdd_t *f,
  array_t *varsArray,
  int n,
  int k)
{
    return NIL(array_t);
}

bdd_node *
bdd_read_zero(bdd_manager *mgr)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_bdd_new_var(bdd_manager *mgr)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_bdd_and_abstract(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g,
  bdd_node *cube)
{
    return NIL(bdd_node);
}

void
bdd_deref(bdd_node *f)
{
}

bdd_node *
bdd_add_plus(
  bdd_manager *mgr,
  bdd_node **fn1,
  bdd_node **fn2)
{
  return NIL(bdd_node);
}


int
bdd_read_reorderings(bdd_manager *mgr)
{
    return 0;
}

int
bdd_read_next_reordering(bdd_manager *mgr)
{
    return 0;
}

void
bdd_set_next_reordering(bdd_manager *mgr, int next)
{
}


bdd_node *
bdd_bdd_xnor(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_bdd_vector_compose(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **vector)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_extract_node_as_is(bdd_t *fn)
{
  return NIL(bdd_node);
}


bdd_node *
bdd_zdd_get_node(
  bdd_manager *mgr,
  int id,
  bdd_node *g,
  bdd_node *h)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_zdd_product(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_zdd_product_recur(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_zdd_union(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
  return NIL(bdd_node);
}


bdd_node *
bdd_zdd_weak_div(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_zdd_weak_div_recur(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_zdd_isop_recur(
  bdd_manager *mgr,
  bdd_node *L,
  bdd_node *U,
  bdd_node **zdd_I)
{
    return NIL(bdd_node);
}


bdd_node *
bdd_zdd_isop(
  bdd_manager *mgr,
  bdd_node *L,
  bdd_node *U,
  bdd_node **zdd_I)
{
    return NIL(bdd_node);
}

int
bdd_zdd_get_cofactors3(
  bdd_manager *mgr,
  bdd_node *f,
  int v,
  bdd_node **f1,
  bdd_node **f0,
  bdd_node **fd)
{
    return 0;
}

bdd_node *
bdd_bdd_and_recur(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
     return NIL(bdd_node);
}

bdd_node *
bdd_unique_inter(
  bdd_manager *mgr,
  int v,
  bdd_node *f,
  bdd_node *g)
{
     return NIL(bdd_node);
}

bdd_node *
bdd_unique_inter_ivo(
  bdd_manager *mgr,
  int v,
  bdd_node *f,
  bdd_node *g)
{
     return NIL(bdd_node);
}


bdd_node *
bdd_zdd_diff(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
} 

bdd_node *
bdd_zdd_diff_recur(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
} 

int
bdd_num_zdd_vars(bdd_manager *mgr)
{
    return -1;
}

bdd_node *
bdd_regular(bdd_node *f)
{
    return NIL(bdd_node);
}

int
bdd_is_constant(bdd_node *f)
{
    return 0;
}

int
bdd_is_complement(bdd_node *f)
{
    return 0;
}

bdd_node *
bdd_bdd_T(bdd_node *f)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_bdd_E(bdd_node *f)
{
    return NIL(bdd_node);
}

bdd_node *
bdd_not_bdd_node(bdd_node *f)
{
    return NIL(bdd_node);
} 

void
bdd_recursive_deref_zdd(bdd_manager *mgr, bdd_node *f)
{
    return;
} 

int
bdd_zdd_count(bdd_manager *mgr, bdd_node *f)
{
    return 0;
}

int
bdd_read_zdd_level(bdd_manager *mgr, int index)
{
    return -1;
} 

int
bdd_zdd_vars_from_bdd_vars(bdd_manager *mgr, int multiplicity)
{
   return 0;
} 

void
bdd_zdd_realign_enable(bdd_manager *mgr)
{
    return;
} 

void
bdd_zdd_realign_disable(bdd_manager *mgr)
{
    return;
} 

int
bdd_zdd_realignment_enabled(bdd_manager *mgr)
{
    return 0;
} 

void
bdd_realign_enable(bdd_manager *mgr)
{
    return;
} 

void
bdd_realign_disable(bdd_manager *mgr)
{
    return;
} 

int
bdd_realignment_enabled(bdd_manager *mgr)
{
    return 0;
} 

int
bdd_node_read_index(bdd_node *f)
{
    return -1;
}

bdd_node *
bdd_read_next(bdd_node *f)
{
    return NIL(bdd_node);
}


void
bdd_set_next(bdd_node *f, bdd_node *g)
{
    return;
}


int
bdd_read_reordered_field(bdd_manager *mgr)
{
    return -1;
}

void
bdd_set_reordered_field(bdd_manager *mgr, int n)
{
    return;
}

bdd_node *
bdd_add_apply_recur(
  bdd_manager *mgr,
  bdd_node *(*operation)(bdd_manager *, bdd_node **, bdd_node **),
  bdd_node *fn1,
  bdd_node *fn2)
{
    return NIL(bdd_node);
}

BDD_VALUE_TYPE
bdd_add_value(bdd_node *f)
{
    return 0.0; 
}

int
bdd_print_minterm(bdd_t *f)
{
  return 0;
}

int
bdd_print_cover(bdd_t *f)
{
  return 0;
}

bdd_t *
bdd_xor_smooth(
  bdd_t *f,
  bdd_t *g,
  array_t *smoothing_vars)
{
    return NIL(bdd_t);
}


bdd_node *
bdd_read_plus_infinity(bdd_manager *mgr)
{
    return NIL(bdd_node);

} /* end of bdd_read_plus_infinity */


bdd_node *
bdd_priority_select(
  bdd_manager *mgr,
  bdd_node *R,
  bdd_node **x,
  bdd_node **y,
  bdd_node **z,
  bdd_node *Pi,
  int n,
  bdd_node *(*Pifunc)(bdd_manager *, int, bdd_node **, bdd_node **, bdd_node **))
{
    return NIL(bdd_node);

} /* end of bdd_priority_select */


void
bdd_set_background(
  bdd_manager *mgr,
  bdd_node *f)
{
    return;
 
} /* end of bdd_set_background */


bdd_node *
bdd_read_background(bdd_manager *mgr)
{
  return NIL(bdd_node);

} /* end of bdd_read_background */


bdd_node *
bdd_bdd_cofactor(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);

} /* end of bdd_bdd_cofactor */


bdd_node *
bdd_bdd_ite(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g,
  bdd_node *h)
{
    return NIL(bdd_node);

} /* end of bdd_bdd_ite */


bdd_node *
bdd_add_minus(
  bdd_manager *mgr,
  bdd_node **fn1,
  bdd_node **fn2)
{
    return NIL(bdd_node);

} /* end of bdd_add_plus */


bdd_node *
bdd_dxygtdxz(
  bdd_manager *mgr,
  int N,
  bdd_node **x,
  bdd_node **y,
  bdd_node **z)
{
    return NIL(bdd_node);

} /* end of bdd_dxygtdxz */


bdd_node *
bdd_bdd_univ_abstract(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node *vars)
{
    return NIL(bdd_node);

} /* end of bdd_bdd_univ_abstract */


bdd_node *
bdd_bdd_cprojection(
  bdd_manager *mgr,
  bdd_node *R,
  bdd_node *Y)
{
    return NIL(bdd_node);

} /* end of bdd_bdd_cprojection */

bdd_node *
bdd_xeqy(
  bdd_manager *mgr,
  int N,
  bdd_node **x,
  bdd_node **y)
{
  return NIL(bdd_node);

} /* end of bdd_xeqy */

bdd_node *
bdd_add_roundoff(
  bdd_manager *mgr,
  bdd_node *f,
  int N)
{
  return NIL(bdd_node);

} /* end of bdd_add_roundoff */

bdd_node *
bdd_xgty(
  bdd_manager *mgr,
  int N,
  bdd_node **x,
  bdd_node **y)
{
  return NIL(bdd_node);

} /* end of bdd_xgty */

bdd_node *
bdd_add_cmpl(
  bdd_manager *mgr,
  bdd_node *f)
{
  return NIL(bdd_node);

} /* end of bdd_add_cmpl */

bdd_node *
bdd_split_set(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **x,
  int n,
  double m)
{
  return NIL(bdd_node);

} /* end of bdd_split_set */


int
bdd_debug_check(bdd_manager *mgr)
{
    return (-1);

} /* end of bdd_debug_check */

bdd_node *
bdd_bdd_xor(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
    return NIL(bdd_node);
}

void 
bdd_dump_blif(
  bdd_manager *mgr,
  int nBdds,
  bdd_node **bdds,
  char **inames,
  char **onames,
  char *model,
  FILE *fp)
{
  return;
}

void 
bdd_dump_blif_body(
  bdd_manager *mgr,
  int nBdds,
  bdd_node **bdds,
  char **inames,
  char **onames,
  FILE *fp)
{
  return;
}

void 
bdd_dump_dot(
  bdd_manager *mgr,
  int nBdds,
  bdd_node **bdds,
  char **inames,
  char **onames,
  FILE *fp)
{
  return;
}

bdd_node *
bdd_make_bdd_from_zdd_cover(bdd_manager *mgr, bdd_node *node)
{
    return(NIL(bdd_node));
}

bdd_node *
bdd_zdd_complement(bdd_manager *mgr, bdd_node *node)
{
    return(NIL(bdd_node));
}

bdd_node *
bdd_bdd_vector_support(
  bdd_manager *mgr,
  bdd_node **F,
  int n)
{
  return NIL(bdd_node);
}

int
bdd_bdd_vector_support_size(
  bdd_manager *mgr,
  bdd_node **F,
  int n)
{
  return -1;
}

int
bdd_bdd_support_size(
  bdd_manager *mgr,
  bdd_node *F)
{
  return -1;
}

bdd_node *
bdd_bdd_support(
  bdd_manager *mgr,
  bdd_node *F)
{
  return NIL(bdd_node);
}

bdd_node *
bdd_add_general_vector_compose(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **vectorOn,
  bdd_node **vectorOff)
{
  return NIL(bdd_node);
}

int
bdd_bdd_leq(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g)
{
  return -1;
} 

bdd_node *
bdd_bdd_boolean_diff(
  bdd_manager *mgr,
  bdd_node *f,
  int x)
{
  return NIL(bdd_node);
}

/**Function********************************************************************

  Synopsis    [Compares two bdds are same.]

  SideEffects []

******************************************************************************/
int
bdd_ptrcmp(bdd_t *f, bdd_t *g)
{
  if (f->node == g->node)
    return(0);
  else
    return(1);
}


/**Function********************************************************************

  Synopsis    [Returns the hash value of a bdd.]

  SideEffects []

******************************************************************************/
int
bdd_ptrhash(bdd_t *f, int size)
{
  int hash;

  hash = (int)((unsigned long)f->node >> 2) % size;
  return(hash);
}

bdd_t *
bdd_subset_with_mask_vars(
  bdd_t *f,
  array_t *varsArray,
  array_t *maskVarsArray)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_and_smooth_with_cube(
  bdd_t *f,
  bdd_t *g,
  bdd_t *cube)
{
  return NIL(bdd_t);
}

bdd_t *
bdd_smooth_with_cube(bdd_t *f, bdd_t *cube)
{
  int i;
  bdd_t *var, *res;
  array_t *smoothingVars;
  var_set_t *supportVarSet;

  smoothingVars = array_alloc(bdd_t *, 0);
  supportVarSet = bdd_get_support(f);
  for (i = 0; i < supportVarSet->n_elts; i++) {
    if (var_set_get_elt(supportVarSet, i) == 1) {
      var = bdd_var_with_index(f->mgr, i);
      array_insert_last(bdd_t *, smoothingVars, var);
    }
  }
  var_set_free(supportVarSet);

  res = bdd_smooth(f, smoothingVars);

  for (i = 0; i < array_n(smoothingVars); i++) {
    var = array_fetch(bdd_t *, smoothingVars, i);
    bdd_free(var);
  }
  array_free(smoothingVars);
  return res;
}

bdd_t *
bdd_substitute_with_permut(bdd_t *f, int *permut)
{
  return NIL(bdd_t);
}

array_t *
bdd_substitute_array_with_permut(
  array_t *f_array,
  int *permut)
{
  return NIL(array_t);
}

bdd_t *
bdd_vector_compose(
  bdd_t *f,
  array_t *varArray,
  array_t *funcArray)
{
  return NIL(bdd_t);
}

double *
bdd_cof_minterm(bdd_t *f)
{
  return(NIL(double));
}

int
bdd_var_is_dependent(bdd_t *f, bdd_t *var)
{
  return(0);
}

array_t *
bdd_find_essential(bdd_t *f)
{
  return(NIL(array_t));
}

bdd_t *
bdd_find_essential_cube(bdd_t *f)
{
  return(NIL(bdd_t));
}

int
bdd_estimate_cofactor(bdd_t *f, bdd_t *var, int phase)
{
  return(0);
}

long
bdd_read_peak_memory(bdd_manager *mgr)
{
  return(0);
}

int
bdd_read_peak_live_node(bdd_manager *mgr)
{
  return(0);
}

int
bdd_set_pi_var(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_set_ps_var(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_set_ns_var(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_is_pi_var(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_is_ps_var(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_is_ns_var(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_set_pair_index(bdd_manager *mgr, int index, int pairIndex)
{
    return(0);
}

int
bdd_read_pair_index(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_set_var_to_be_grouped(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_set_var_hard_group(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_reset_var_to_be_grouped(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_is_var_to_be_grouped(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_is_var_hard_group(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_is_var_to_be_ungrouped(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_set_var_to_be_ungrouped(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_bind_var(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_unbind_var(bdd_manager *mgr, int index)
{
    return(0);
}

int
bdd_is_lazy_sift(bdd_manager *mgr)
{
    return(0);
}

void
bdd_discard_all_var_groups(bdd_manager *mgr)
{
    return;
}

void
bdd_set_time_limit(bdd_manager *mgr, unsigned long tl)
{
    return;
}

void
bdd_unset_time_limit(bdd_manager *mgr)
{
    return;
}

void
bdd_reset_start_time(bdd_manager *mgr)
{
  return;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/



