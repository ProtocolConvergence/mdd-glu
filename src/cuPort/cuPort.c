/**CFile***********************************************************************

  FileName    [cuPort.c]

  PackageName [cudd]

  Synopsis [SIS/VIS interface to the Decision Diagram Package of the University
  of Colorado.]

  Description [This file implements an interface between the functions in the
    Berkeley BDD package and the functions provided by the CUDD (decision
    diagram) package from the University of Colorado. The CUDD package is a
    generic implementation of a decision diagram data structure. For the time
    being, only Boole expansion is implemented and the leaves in the in the
    nodes can be the constants zero, one or any arbitrary value.]

  Author      [Abelardo Pardo, Kavita Ravi]

  Copyright   [Copyright (c) 1995-2004, Regents of the University of Colorado

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  Neither the name of the University of Colorado nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.]

******************************************************************************/

#include "cuPortInt.h"

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

#ifndef lint
static char rcsid[] DD_UNUSED = "$Id: cuPort.c,v 1.135 2012/02/04 21:43:23 fabio Exp $";
#endif

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void InvalidType( FILE *file, const char *field, const char *expected);


/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Builds the bdd_t structure.]

  Description [Builds the bdd_t structure from manager and node.
  Assumes that the reference count of the node has already been
  increased. If it fails to create a new bdd_t structure it disposes of
  the node to simplify error handling for the caller. Returns a
  pointer to the newly created structure if successful; NULL
  otherwise.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_construct_bdd_t(bdd_manager *mgr, bdd_node *fn)
{
  bdd_t *result;

  result = ALLOC(bdd_t, 1);
  if (result == NULL) {
    Cudd_RecursiveDeref((DdManager *)mgr,(DdNode *)fn);
    return(NULL);
  }
  result->mgr = (DdManager *) mgr;
  result->node = (DdNode *) fn;
  result->free = FALSE;
  return(result);

} /* end of bdd_construct_bdd_t */


/**Function********************************************************************

  Synopsis           [Function to identify the bdd package being used]

  SideEffects        []

******************************************************************************/
bdd_package_type_t
bdd_get_package_name(void)
{
  return(CUDD);

} /* end of bdd_get_package_name */


/**Function********************************************************************

  Synopsis    [Terminates the bdd package.]

  SideEffects []

******************************************************************************/
void
bdd_end(bdd_manager *mgr)
{
  DdManager *manager;

  manager = (DdManager *)mgr;
  if (manager->hooks != NULL) FREE(manager->hooks);
  Cudd_Quit(manager);

} /* end of bdd_end */


/**Function********************************************************************

  Synopsis    [Starts the manager with nvariables variables.]

  SideEffects []

******************************************************************************/
bdd_manager *
bdd_start(int nvariables)
{
  DdManager *mgr;
  bdd_external_hooks *hooks;

  mgr =  Cudd_Init((unsigned int)nvariables, 0, CUDD_UNIQUE_SLOTS,
		   CUDD_CACHE_SLOTS, getSoftDataLimit() / 10 * 9);

  hooks = ALLOC(bdd_external_hooks,1);
  hooks->mdd = hooks->network = hooks->undef1 = (char *) 0;
  mgr->hooks = (char *) hooks;

  return(bdd_manager *)mgr;

} /* end of bdd_start */


/**Function********************************************************************

  Synopsis    [Creates a new variable in the manager.]

  SideEffects [Modifies the manager]

  SeeAlso     [bdd_create_variable_after]

******************************************************************************/
bdd_t *
bdd_create_variable(bdd_manager *mgr)
{
  DdNode *var;
  DdManager *dd = (DdManager *) mgr;
  DdNode *one = DD_ONE(dd);

  if (dd->size >= CUDD_MAXINDEX -1) return(NULL);
  do {
    dd->reordered = 0;
    var = cuddUniqueInter(dd,dd->size,one,Cudd_Not(one));
  } while (dd->reordered == 1);

  if (var == NULL) return((bdd_t *)NULL);
  cuddRef(var);
  return(bdd_construct_bdd_t(dd,var));

} /* end of bdd_create_variable */


/**Function********************************************************************

  Synopsis    [Creates a new variable and positions it after the
  variable with the specified index.]

  SideEffects [Modifies the manager.]

  SeeAlso     [bdd_create_variable]

******************************************************************************/
bdd_t *
bdd_create_variable_after(bdd_manager *mgr, bdd_variableId after_id)
{
  DdNode *var;
  DdManager *dd = (DdManager *) mgr;
  int level;

  if (after_id >= (bdd_variableId) dd->size) return(NULL);
  level = 1 + dd->perm[after_id];
  var = Cudd_bddNewVarAtLevel(dd,level);
  if (var == NULL) return((bdd_t *)NULL);
  cuddRef(var);
  return(bdd_construct_bdd_t(dd,var));

} /* end of bdd_create_variable_after */


/**Function********************************************************************

  Synopsis    [Returns the BDD representing the variable with given ID.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_get_variable(bdd_manager *mgr, bdd_variableId variable_ID)
{
  DdNode *var;
  DdManager *dd = (DdManager *) mgr;
  DdNode *one = DD_ONE(dd);

  if (variable_ID >= CUDD_MAXINDEX -1) return(NULL);
  do {
    dd->reordered = 0;
    var = cuddUniqueInter(dd,(int)variable_ID,one,Cudd_Not(one));
  } while (dd->reordered == 1);

  if (var == NULL) return((bdd_t *)NULL);
  cuddRef(var);
  return(bdd_construct_bdd_t(dd,var));

} /* end of bdd_get_variable */


/**Function********************************************************************

  Synopsis    [Creates a copy of the BDD.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_dup(bdd_t *f)
{
  cuddRef(f->node);
  return(bdd_construct_bdd_t(f->mgr,f->node));

} /* end of bdd_dup */


/**Function********************************************************************

  Synopsis    [Deletes the BDD of f.]

  SideEffects []

******************************************************************************/
void
bdd_free(bdd_t *f)
{
  if (f == NULL) {
    fail("bdd_free: trying to free a NULL bdd_t");
  }

  if (f->free == TRUE) {
    fail("bdd_free: trying to free a freed bdd_t");
  }

  Cudd_RecursiveDeref(f->mgr,f->node);
  /* This is a bit overconservative. */
  f->node = NULL;
  f->mgr = NULL;
  f->free = TRUE;
  FREE(f);
  return;

} /* end of bdd_free */


/**Function********************************************************************

  Synopsis    [And of two BDDs.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_and(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase)
{
  DdManager *dd;
  DdNode *newf, *newg, *fandg;

  /* Make sure both BDDs belong to the same manager. */
  assert(f->mgr == g->mgr);

  /* Modify the phases of the operands according to the parameters. */
  newf = Cudd_NotCond(f->node,!f_phase);
  newg = Cudd_NotCond(g->node,!g_phase);

  /* Perform the AND operation. */
  dd = f->mgr;
  fandg = Cudd_bddAnd(f->mgr,newf,newg);
  if (fandg == NULL) return(NULL);
  cuddRef(fandg);

  return(bdd_construct_bdd_t(dd,fandg));

} /* end of bdd_and */


/**Function********************************************************************

  Synopsis    [And of two BDDs with limit on new nodes.]

  Description [If more new nodes than specified by limit must be created,
  this function returns NULL.  The caller must be prepared for this
  occurrence.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_and_with_limit(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase,
  unsigned int limit)
{
  DdManager *dd;
  DdNode *newf, *newg, *fandg;

  /* Make sure both BDDs belong to the same manager. */
  assert(f->mgr == g->mgr);

  /* Modify the phases of the operands according to the parameters. */
  newf = Cudd_NotCond(f->node,!f_phase);
  newg = Cudd_NotCond(g->node,!g_phase);

  /* Perform the AND operation. */
  dd = f->mgr;
  fandg = Cudd_bddAndLimit(f->mgr,newf,newg,limit);
  if (fandg == NULL) return(NULL);
  cuddRef(fandg);

  return(bdd_construct_bdd_t(dd,fandg));

} /* end of bdd_and_with_limit */


/**Function********************************************************************

  Synopsis    [And of a BDD and an array of BDDs.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_and_array(
  bdd_t *f,
  array_t *g_array,
  boolean f_phase,
  boolean g_phase)
{
  bdd_t *g;
  DdNode *result, *temp;
  int i;
  DdNode *newf, *newg;

  /* Modify the phases of the operands according to the parameters. */
  newf = Cudd_NotCond(f->node,!f_phase);

  Cudd_Ref(result = newf);

  for (i = 0; i < array_n(g_array); i++) {
    g = array_fetch(bdd_t *, g_array, i);

    /* Modify the phases of the operands according to the parameters. */
    newg = Cudd_NotCond(g->node,!g_phase);

    temp = Cudd_bddAnd(f->mgr, result, newg);
    if (temp == NULL) {
      Cudd_RecursiveDeref(f->mgr, result);
      return(NULL);
    }
    cuddRef(temp);
    Cudd_RecursiveDeref(f->mgr, result);
    result = temp;
  }

  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_and_array */


/**Function********************************************************************

  Synopsis           [Takes the and of an array of functions.]

  SideEffects        [None]

******************************************************************************/
bdd_t *
bdd_multiway_and(bdd_manager *manager, array_t *bddArray)
{
  DdManager *mgr;
  bdd_t *operand;
  DdNode *result, *temp;
  int i;

  mgr = (DdManager *)manager;

  Cudd_Ref(result = DD_ONE(mgr));

  for (i = 0; i < array_n(bddArray); i++) {
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = Cudd_bddAnd(mgr, result, operand->node);
    if (temp == NULL) {
      Cudd_RecursiveDeref(mgr, result);
      return(NULL);
    }
    cuddRef(temp);
    Cudd_RecursiveDeref(mgr, result);
    result = temp;
  }

  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_multiway_and */


/**Function********************************************************************

  Synopsis           [Takes the or of an array of functions.]

  SideEffects        []

******************************************************************************/
bdd_t *
bdd_multiway_or(bdd_manager *manager, array_t *bddArray)
{
  DdManager *mgr;
  bdd_t *operand;
  DdNode *result, *temp;
  int i;

  mgr = (DdManager *)manager;
  Cudd_Ref(result = Cudd_Not(DD_ONE(mgr)));

  for (i = 0; i < array_n(bddArray); i++) {
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = Cudd_bddOr(mgr, result, operand->node);
    if (temp == NULL) {
      Cudd_RecursiveDeref(mgr, result);
      return(NULL);
    }
    cuddRef(temp);
    Cudd_RecursiveDeref(mgr, result);
    result = temp;
  }

  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_multiway_or */


/**Function********************************************************************

  Synopsis           [Takes the xor of an array of functions.]

  SideEffects        [None]

******************************************************************************/
bdd_t *
bdd_multiway_xor(bdd_manager *manager, array_t *bddArray)
{
  DdManager *mgr;
  bdd_t *operand;
  DdNode *result, *temp;
  int i;

  mgr = (DdManager *)manager;

  Cudd_Ref(result = Cudd_Not(DD_ONE(mgr)));

  for (i = 0; i < array_n(bddArray); i++) {
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = Cudd_bddXor(mgr, result, operand->node);
    if (temp == NULL) {
      Cudd_RecursiveDeref(mgr, result);
      return(NULL);
    }
    cuddRef(temp);
    Cudd_RecursiveDeref(mgr, result);
    result = temp;
  }

  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_multiway_xor */


/**Function********************************************************************

  Synopsis    [Takes the pairwise or of two arrays of bdds of the same length.]

  SideEffects [None]

******************************************************************************/
array_t *
bdd_pairwise_or(bdd_manager *manager, array_t *bddArray1, array_t *bddArray2)
{
  DdManager *mgr;
  bdd_t *op1, *op2;
  bdd_t *unit;
  DdNode *result;
  array_t *resultArray;
  int i;

  mgr = (DdManager *)manager;

  if (array_n(bddArray1) != array_n(bddArray2)) {
    (void) fprintf(stderr,
		   "bdd_pairwise_or: Arrays of different lengths.\n");
    return(NULL);
  }

  resultArray = array_alloc(bdd_t *, array_n(bddArray1));
  for (i = 0; i < array_n(bddArray1); i++) {
    op1 = array_fetch(bdd_t *, bddArray1, i);
    op2 = array_fetch(bdd_t *, bddArray2, i);

    result = Cudd_bddOr(mgr, op1->node, op2->node);
    if (result == NULL) {
      int j;
      bdd_t *item;
      for (j = 0; j < array_n(resultArray); j++) {
	item = array_fetch(bdd_t *, resultArray, j);
	bdd_free(item);
      }
      array_free(resultArray);
      return((array_t *)NULL);
    }
    cuddRef(result);

    unit = bdd_construct_bdd_t(mgr, result);
    array_insert(bdd_t *, resultArray, i, unit);
  }

  return(resultArray);

} /* end of bdd_pairwise_or */


/**Function********************************************************************

  Synopsis [Takes the pairwise and of two arrays of bdds of the same length.]

  SideEffects [required]

******************************************************************************/
array_t *
bdd_pairwise_and(bdd_manager *manager, array_t *bddArray1, array_t *bddArray2)
{
  DdManager *mgr;
  bdd_t *op1, *op2;
  bdd_t *unit;
  DdNode *result;
  array_t *resultArray;
  int i;

  mgr = (DdManager *)manager;

  if (array_n(bddArray1) != array_n(bddArray2)) {
    (void) fprintf(stderr,
		   "bdd_pairwise_or: Arrays of different lengths.\n");
    return(NULL);
  }

  resultArray = array_alloc(bdd_t *, array_n(bddArray1));
  for (i = 0; i < array_n(bddArray1); i++) {
    op1 = array_fetch(bdd_t *, bddArray1, i);
    op2 = array_fetch(bdd_t *, bddArray2, i);

    result = Cudd_bddAnd(mgr, op1->node, op2->node);
    if (result == NULL) {
      int j;
      bdd_t *item;
      for (j = 0; j < array_n(resultArray); j++) {
	item = array_fetch(bdd_t *, resultArray, j);
	bdd_free(item);
      }
      array_free(resultArray);
      return((array_t *)NULL);
    }
    cuddRef(result);

    unit = bdd_construct_bdd_t(mgr, result);
    array_insert(bdd_t *, resultArray, i, unit);
  }

  return(resultArray);

} /* end of bdd_pairwise_and */


/**Function********************************************************************

  Synopsis [Takes the pairwise xor of two arrays of bdds of the same length.]

  SideEffects [required]

******************************************************************************/
array_t *
bdd_pairwise_xor(bdd_manager *manager, array_t *bddArray1, array_t *bddArray2)
{
  DdManager *mgr;
  bdd_t *op1, *op2;
  bdd_t *unit;
  DdNode *result;
  array_t *resultArray;
  int i;

  mgr = (DdManager *)manager;

  if (array_n(bddArray1) != array_n(bddArray2)) {
    (void) fprintf(stderr,
		   "bdd_pairwise_or: Arrays of different lengths.\n");
    return(NULL);
  }

  resultArray = array_alloc(bdd_t *, array_n(bddArray1));
  for (i = 0; i < array_n(bddArray1); i++) {
    op1 = array_fetch(bdd_t *, bddArray1, i);
    op2 = array_fetch(bdd_t *, bddArray2, i);

    result = Cudd_bddXor(mgr, op1->node, op2->node);
    if (result == NULL) {
      int j;
      bdd_t *item;
      for (j = 0; j < array_n(resultArray); j++) {
	item = array_fetch(bdd_t *, resultArray, j);
	bdd_free(item);
      }
      array_free(resultArray);
      return((array_t *)NULL);
    }
    cuddRef(result);

    unit = bdd_construct_bdd_t(mgr, result);
    array_insert(bdd_t *, resultArray, i, unit);
  }

  return(resultArray);

} /* end of bdd_pairwise_xor */


/**Function********************************************************************

  Synopsis    [Abstracts variables from the product of two BDDs.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_and_smooth(
  bdd_t *f,
  bdd_t *g,
  array_t *smoothing_vars	/* of bdd_t *'s */)
{
  int i;
  bdd_t *variable;
  DdNode *cube, *tmpDd, *result;
  DdManager *mgr;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);

  /* CUDD needs the smoothing variables passed as a cube.
   * Therefore we must build that cube from the indices of variables
   * in the array before calling the procedure.
   */
  mgr = f->mgr;
  Cudd_Ref(cube = DD_ONE(mgr));
  for (i = 0; i < array_n(smoothing_vars); i++) {
    variable = array_fetch(bdd_t *,smoothing_vars,i);

    /* Make sure the variable belongs to the same manager. */
    assert(mgr == variable->mgr);

    tmpDd = Cudd_bddAnd(mgr,cube,variable->node);
    if (tmpDd == NULL) {
      Cudd_RecursiveDeref(mgr,cube);
      return(NULL);
    }
    cuddRef(tmpDd);
    Cudd_RecursiveDeref(mgr, cube);
    cube = tmpDd;
  }

  /* Perform the smoothing */
  result = Cudd_bddAndAbstract(mgr,f->node,g->node,cube);
  if (result == NULL) {
    Cudd_RecursiveDeref(mgr, cube);
    return(NULL);
  }
  cuddRef(result);
  /* Get rid of temporary results. */
  Cudd_RecursiveDeref(mgr, cube);

  /* Build the bdd_t structure for the result */
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_and_smooth */


/**Function********************************************************************

  Synopsis    [Abstracts variables from the product of two BDDs with limit
  on new nodes.]

  Description [If more new nodes than specified by limit must be created,
  this function returns NULL.  The caller must be prepared for this
  occurrence.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_and_smooth_with_limit(
  bdd_t *f,
  bdd_t *g,
  array_t *smoothing_vars /* of bdd_t *'s */,
  unsigned int limit)
{
  int i;
  bdd_t *variable;
  DdNode *cube, *tmpDd, *result;
  DdManager *mgr;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);

  /* CUDD needs the smothing variables passed as a cube.
   * Therefore we must build that cube from the indices of variables
   * in the array before calling the procedure.
   */
  mgr = f->mgr;
  Cudd_Ref(cube = DD_ONE(mgr));
  for (i = 0; i < array_n(smoothing_vars); i++) {
    variable = array_fetch(bdd_t *,smoothing_vars,i);

    /* Make sure the variable belongs to the same manager. */
    assert(mgr == variable->mgr);

    tmpDd = Cudd_bddAnd(mgr,cube,variable->node);
    if (tmpDd == NULL) {
      Cudd_RecursiveDeref(mgr,cube);
      return(NULL);
    }
    cuddRef(tmpDd);
    Cudd_RecursiveDeref(mgr, cube);
    cube = tmpDd;
  }

  /* Perform the smoothing */
  result = Cudd_bddAndAbstractLimit(mgr,f->node,g->node,cube,limit);
  if (result == NULL) {
    Cudd_RecursiveDeref(mgr, cube);
    return(NULL);
  }
  cuddRef(result);
  /* Get rid of temporary results. */
  Cudd_RecursiveDeref(mgr, cube);

  /* Build the bdd_t structure for the result */
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_and_smooth_with_limit */


/**Function********************************************************************

  Synopsis    [Abstracts variables from the product of two BDDs.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_and_smooth_with_cube(bdd_t *f, bdd_t *g, bdd_t *cube)
{
  DdNode *result;
  DdManager *mgr;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);

  /* The Boulder package needs the smothing variables passed as a cube.
   * Therefore we must build that cube from the indices of variables
   * in the array before calling the procedure.
   */
  mgr = f->mgr;

  /* Perform the smoothing */
  result = Cudd_bddAndAbstract(mgr,f->node,g->node,cube->node);
  if (result == NULL)
    return(NULL);
  cuddRef(result);

  /* Build the bdd_t structure for the result */
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_and_smooth_with_cube */


/**Function********************************************************************

  Synopsis [Abstracts variables from the product of two
  BDDs. Computation is clipped at a certain depth.]

  Description [Abstracts variables from the product of two
  BDDs. Computation is clipped at a certain depth. This procedure is
  similar to bdd_and_smooth but large depth recursions are
  avoided. maxDepth specifies the recursion depth. over specifies
  which kind of approximation is used 0 - under approximation and 1 -
  for over approximation. ]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_clipping_and_smooth(
  bdd_t *f,
  bdd_t *g,
  array_t *smoothing_vars /* of bdd_t *'s */,
  int maxDepth,
  int over)
{
  int i;
  bdd_t *variable;
  DdNode *cube,*tmpDd,*result;
  DdManager *mgr;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);

  /* The Boulder package needs the smothing variables passed as a cube.
   * Therefore we must build that cube from the indices of variables
   * in the array before calling the procedure.
   */
  mgr = f->mgr;
  Cudd_Ref(cube = DD_ONE(mgr));
  for (i = 0; i < array_n(smoothing_vars); i++) {
    variable = array_fetch(bdd_t *,smoothing_vars,i);

    /* Make sure the variable belongs to the same manager. */
    assert(mgr == variable->mgr);

    tmpDd = Cudd_bddAnd(mgr,cube,variable->node);
    if (tmpDd == NULL) {
      Cudd_RecursiveDeref(mgr,cube);
      return(NULL);
    }
    cuddRef(tmpDd);
    Cudd_RecursiveDeref(mgr, cube);
    cube = tmpDd;
  }

  /* Perform the smoothing */
  result = Cudd_bddClippingAndAbstract(mgr,f->node,g->node,cube, maxDepth, over);
  if (result == NULL) {
    Cudd_RecursiveDeref(mgr, cube);
    return(NULL);
  }
  cuddRef(result);
  /* Get rid of temporary results. */
  Cudd_RecursiveDeref(mgr, cube);

  /* Build the bdd_t structure for the result */
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_clipping_and_smooth */


/**Function********************************************************************

  Synopsis    [Abstracts variables from the exclusive OR of two BDDs.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_xor_smooth(
  bdd_t *f,
  bdd_t *g,
  array_t *smoothing_vars /* of bdd_t *'s */)
{
  int i;
  bdd_t *variable;
  DdNode *cube,*tmpDd,*result;
  DdManager *mgr;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);

  /* The Boulder package needs the smothing variables passed as a cube.
   * Therefore we must build that cube from the indices of variables
   * in the array before calling the procedure.
   */
  mgr = f->mgr;
  Cudd_Ref(cube = DD_ONE(mgr));
  for (i = 0; i < array_n(smoothing_vars); i++) {
    variable = array_fetch(bdd_t *,smoothing_vars,i);

    /* Make sure the variable belongs to the same manager. */
    assert(mgr == variable->mgr);

    tmpDd = Cudd_bddAnd(mgr,cube,variable->node);
    if (tmpDd == NULL) {
      Cudd_RecursiveDeref(mgr,cube);
      return(NULL);
    }
    cuddRef(tmpDd);
    Cudd_RecursiveDeref(mgr, cube);
    cube = tmpDd;
  }

  /* Perform the smoothing */
  result = Cudd_bddXorExistAbstract(mgr,f->node,g->node,cube);
  if (result == NULL) {
    Cudd_RecursiveDeref(mgr, cube);
    return(NULL);
  }
  cuddRef(result);
  /* Get rid of temporary results. */
  Cudd_RecursiveDeref(mgr, cube);

  /* Build the bdd_t structure for the result */
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_xor_smooth */


/**Function********************************************************************

  Synopsis    [Return a minimum size BDD between bounds.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_between(bdd_t *f_min, bdd_t *f_max)
{
  bdd_t *care_set, *ret;

  if (bdd_equal(f_min, f_max)) {
    return (bdd_dup(f_min));
  }
  care_set = bdd_or(f_min, f_max, 1, 0);
  ret = bdd_minimize(f_min, care_set);
  bdd_free(care_set);
  /* The size of ret is never larger than the size of f_min. We need
  ** only to check ret against f_max. */
  if (bdd_size(f_max) <= bdd_size(ret)) {
    bdd_free(ret);
    return(bdd_dup(f_max));
  } else {
    return(ret);
  }

} /* end of bdd_between */


/**Function********************************************************************

  Synopsis [Computes the cube of an array of mdd ids. The cube
  is positive unate.  Returns a pointer to the result if successful;
  NULL otherwise.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_compute_cube(bdd_manager *mgr, array_t *vars)
{
  DdNode *result;
  DdNode **nodeArray;
  int i, id;

  if (vars == NIL(array_t)) return NIL(bdd_t);
  if (array_n(vars) == 0) return NIL(bdd_t);
  /* create an array of DdNodes */
  nodeArray = ALLOC(DdNode *, array_n(vars));
  arrayForEachItem(int, vars, i, id) {
    assert(id < bdd_num_vars(mgr));
    nodeArray[i] = Cudd_bddIthVar((DdManager *)mgr, id);
  }
  result = Cudd_bddComputeCube((DdManager *)mgr, (DdNode **)nodeArray,
			       NULL, array_n(vars));
  FREE(nodeArray);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_compute_cube */


/**Function********************************************************************

  Synopsis [Computes the cube of an array of mdd ids. The phase if
  each variable is given in the phase array.  Returns a pointer to the
  result if successful; NULL otherwise.]

  SideEffects [none]

******************************************************************************/
bdd_t *
bdd_compute_cube_with_phase(bdd_manager *mgr, array_t *vars, array_t *phase)
{
  DdNode *result;
  DdNode **nodeArray;
  int *phaseArray;
  int i, id, ph;

  if (vars == NIL(array_t)) return NIL(bdd_t);
  if (array_n(vars) == 0) return NIL(bdd_t);
  if (phase != NIL(array_t) && array_n(vars) != array_n(phase))
    return NIL(bdd_t);
  /* create an array of DdNodes */
  nodeArray = ALLOC(DdNode *, array_n(vars));
  phaseArray = NIL(int);
  if (phase != NIL(array_t)) phaseArray = ALLOC(int, array_n(phase));
  arrayForEachItem(int, vars, i, id) {
    assert(id < bdd_num_vars(mgr));
    nodeArray[i] = Cudd_bddIthVar((DdManager *)mgr, id);
  }
  arrayForEachItem(int, phase, i, ph) {
    assert(ph == 0 || ph == 1);
    phaseArray[i] = ph;
  }
  result = Cudd_bddComputeCube((DdManager *)mgr, (DdNode **)nodeArray,
			       phaseArray, array_n(vars));
  FREE(nodeArray);
  if (phaseArray != NIL(int)) FREE(phaseArray);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_compute_cube_with_phase */


/**Function********************************************************************

  Synopsis    [Computes the cofactor of f with respect to g.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_cofactor(bdd_t *f, bdd_t *g)
{
  DdNode *result;

  /* Make sure both operands belong to the same manager */
  assert(f->mgr == g->mgr);

  /* We use Cudd_bddConstrain instead of Cudd_Cofactor for generality. */
  result = Cudd_bddConstrain(f->mgr,f->node,
			     g->node);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_cofactor */


/**Function********************************************************************

  Synopsis    [Computes the cofactor of f with respect to g.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_cofactor_array(bdd_t *f, array_t *bddArray)
{
  bdd_t *operand;
  DdNode *result, *temp;
  int i;

  Cudd_Ref(result = f->node);

  for (i = 0; i < array_n(bddArray); i++) {
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = Cudd_bddConstrain(f->mgr, result, operand->node);
    if (temp == NULL) {
      Cudd_RecursiveDeref(f->mgr, result);
      return(NULL);
    }
    cuddRef(temp);
    Cudd_RecursiveDeref(f->mgr, result);
    result = temp;
  }

  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_cofactor_array */


/**Function********************************************************************

  Synopsis    [Computes the cofactor of f with respect to g.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_var_cofactor(bdd_t *f, bdd_t *g)
{
  DdNode *result;

  /* Make sure both operands belong to the same manager */
  assert(f->mgr == g->mgr);

  result = Cudd_Cofactor(f->mgr,f->node,
			 g->node);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_var_cofactor */


/**Function********************************************************************

  Synopsis    [Computes the cofactor of f with respect to g in a safe manner.]

  Description [Performs safe minimization of a BDD. Given the BDD
  <code>f</code> of a function to be minimized and a BDD
  <code>c</code> representing the care set, Cudd_bddLICompaction
  produces the BDD of a function that agrees with <code>f</code>
  wherever <code>c</code> is 1.  Safe minimization means that the size
  of the result is guaranteed not to exceed the size of
  <code>f</code>. This function is based on the DAC97 paper by Hong et
  al..  Returns a pointer to the result if successful; NULL
  otherwise.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_compact(bdd_t *f, bdd_t *g)
{
  DdNode *result;

  /* Make sure both operands belong to the same manager */
  assert(f->mgr == g->mgr);

  result = Cudd_bddLICompaction(f->mgr,f->node,
				g->node);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_compact */


/**Function********************************************************************

  Synopsis    [Computes a bdd between l and u.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_squeeze(bdd_t *l, bdd_t *u)
{
  DdNode *result;

  /* Make sure both operands belong to the same manager */
  assert(l->mgr == u->mgr);

  result = Cudd_bddSqueeze(l->mgr,l->node,
			   u->node);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(l->mgr,result));

} /* end of bdd_squeeze */


/**Function********************************************************************

  Synopsis    [Functional composition of a function by a variable.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_compose(bdd_t *f, bdd_t *v, bdd_t *g)
{
  DdNode *result;

  /* Make sure all operands belong to the same manager. */
  assert(f->mgr == g->mgr);
  assert(f->mgr == v->mgr);

  result = Cudd_bddCompose(f->mgr,f->node,
			   g->node,
			   (int)Cudd_Regular(v->node)->index);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_compose */


/**Function********************************************************************

  Synopsis [Composes a BDD with a vector of BDDs. Given a vector of
  BDDs, creates a new BDD by substituting the BDDs for the variables
  of the BDD f.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_vector_compose(bdd_t *f, array_t *varArray, array_t *funcArray)
{
  int i, n, nVars, index;
  bdd_t *var, *func;
  DdNode *result;
  DdNode **vector;

  assert(array_n(varArray) == array_n(funcArray));
  n = array_n(varArray);
  nVars = ((DdManager *)f->mgr)->size;
  vector = ALLOC(DdNode *, sizeof(DdNode *) * nVars);
  memset(vector, 0, sizeof(DdNode *) * nVars);

  for (i = 0; i < n; i++) {
    var = array_fetch(bdd_t *, varArray, i);
    func = array_fetch(bdd_t *, funcArray, i);
    index = (int)bdd_top_var_id(var);
    vector[index] = (DdNode *)func->node;
    cuddRef(vector[index]);
  }
  for (i = 0; i < nVars; i++) {
    if (!vector[i]) {
      vector[i] = Cudd_bddIthVar((DdManager *)f->mgr, i);
      cuddRef(vector[i]);
    }
  }

  result = Cudd_bddVectorCompose(f->mgr, f->node, vector);

  for (i = 0; i < nVars; i++)
    Cudd_RecursiveDeref((DdManager *)f->mgr, vector[i]);
  FREE(vector);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_vector_compose */


/**Function********************************************************************

  Synopsis    [Universal Abstraction of Variables.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_consensus(
  bdd_t *f,
  array_t *quantifying_vars /* of bdd_t *'s */)
{
  int i;
  bdd_t *variable;
  DdNode *cube,*tmpDd,*result;
  DdManager *mgr;

  /* The Boulder package needs the smothing variables passed as a cube.
   * Therefore we must build that cube from the indices of the variables
   * in the array before calling the procedure.
   */
  mgr = f->mgr;
  Cudd_Ref(cube = DD_ONE(mgr));
  for (i = 0; i < array_n(quantifying_vars); i++) {
    variable = array_fetch(bdd_t *,quantifying_vars,i);

    /* Make sure the variable belongs to the same manager */
    assert(mgr == variable->mgr);

    tmpDd = Cudd_bddAnd(mgr,cube,variable->node);
    if (tmpDd == NULL) {
      Cudd_RecursiveDeref(mgr, cube);
      return(NULL);
    }
    cuddRef(tmpDd);
    Cudd_RecursiveDeref(mgr, cube);
    cube = tmpDd;
  }

  /* Perform the consensus */
  result = Cudd_bddUnivAbstract(mgr,f->node,cube);
  if (result == NULL) {
    Cudd_RecursiveDeref(mgr, cube);
    return(NULL);
  }
  cuddRef(result);
  /* Get rid of temporary results */
  Cudd_RecursiveDeref(mgr, cube);

  /* Build the bdd_t structure for the result */
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_consensus */


/**Function********************************************************************

  Synopsis    [Universal Abstraction of Variables.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_consensus_with_cube(
  bdd_t *f,
  bdd_t *cube)
{
  DdNode *result;
  DdManager *mgr;

  mgr = f->mgr;
  /* Perform the consensus */
  result = Cudd_bddUnivAbstract(mgr,f->node,cube->node);
  if (result == NULL)
    return(NULL);
  cuddRef(result);

  /* Build the bdd_t structure for the result */
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_consensus_with_cube */


/**Function********************************************************************

  Synopsis    [The compatible projection function.]

  Description [The compatible projection function. The reference minterm
  is chosen based on the phases of the quantifying variables. If all
  variables are in positive phase, the minterm 111...111 is used as
  reference.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_cproject(
  bdd_t *f,
  array_t *quantifying_vars /* of bdd_t* */)
{
  DdManager *dd;
  DdNode *cube;
  DdNode *res;
  bdd_t *fi;
  int nvars, i;

  if (f == NULL) {
    fail ("bdd_cproject: invalid BDD");
  }

  nvars = array_n(quantifying_vars);
  if (nvars <= 0) {
    fail("bdd_cproject: no projection variables");
  }
  dd = f->mgr;

  cube = DD_ONE(dd);
  cuddRef(cube);
  for (i = nvars - 1; i >= 0; i--) {
    DdNode *tmpp;
    fi = array_fetch(bdd_t *, quantifying_vars, i);
    tmpp = Cudd_bddAnd(dd,fi->node,cube);
    if (tmpp == NULL) {
      Cudd_RecursiveDeref(dd,cube);
      return(NULL);
    }
    cuddRef(tmpp);
    Cudd_RecursiveDeref(dd,cube);
    cube = tmpp;
  }

  res = Cudd_CProjection(dd,f->node,cube);
  if (res == NULL) {
    Cudd_RecursiveDeref(dd,cube);
    return(NULL);
  }
  cuddRef(res);
  Cudd_RecursiveDeref(dd,cube);

  return(bdd_construct_bdd_t(dd,res));

} /* end of bdd_cproject */


/**Function********************************************************************

  Synopsis    [Returns the else branch of a BDD.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_else(bdd_t *f)
{
  DdNode *result;

  result = Cudd_E(f->node);
  result =  Cudd_NotCond(result,Cudd_IsComplement(f->node));
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_else */


/**Function********************************************************************

  Synopsis    [ITE.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_ite(
  bdd_t *i,
  bdd_t *t,
  bdd_t *e,
  boolean i_phase,
  boolean t_phase,
  boolean e_phase)
{
  DdNode *newi,*newt,*newe,*ite;

  /* Make sure both bdds belong to the same mngr */
  assert(i->mgr == t->mgr);
  assert(i->mgr == e->mgr);

  /* Modify the phases of the operands according to the parameters */
  if (!i_phase) {
    newi = Cudd_Not(i->node);
  } else {
    newi = i->node;
  }
  if (!t_phase) {
    newt = Cudd_Not(t->node);
  } else {
    newt = t->node;
  }
  if (!e_phase) {
    newe = Cudd_Not(e->node);
  } else {
    newe = e->node;
  }

  /* Perform the ITE operation */
  ite = Cudd_bddIte(i->mgr,newi,newt,newe);
  if (ite == NULL) return(NULL);
  cuddRef(ite);
  return(bdd_construct_bdd_t(i->mgr,ite));

} /* end of bdd_ite */


/**Function********************************************************************

  Synopsis    [Restrict operator as described in Coudert et al. ICCAD90.]

  Description [Restrict operator as described in Coudert et
  al. ICCAD90.  Always returns a BDD not larger than the input
  <code>f</code> if successful; NULL otherwise.]

  SideEffects [none]

******************************************************************************/
bdd_t *
bdd_minimize(bdd_t *f, bdd_t *c)
{
  DdNode *result;
  bdd_t *output;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == c->mgr);

  result = Cudd_bddRestrict(f->mgr, f->node, c->node);
  if (result == NULL) return(NULL);
  cuddRef(result);

  output = bdd_construct_bdd_t(f->mgr,result);
  return(output);

} /* end of bdd_minimize */


/**Function********************************************************************

  Synopsis    [Restrict operator as described in Coudert et al. ICCAD90.]

  Description [Restrict operator as described in Coudert et
  al. ICCAD90.  Always returns a BDD not larger than the input
  <code>f</code> if successful; NULL otherwise.]

  SideEffects [none]

******************************************************************************/
bdd_t *
bdd_minimize_array(bdd_t *f, array_t *bddArray)
{
  bdd_t *operand;
  DdNode *result, *temp;
  int i;

  Cudd_Ref(result = f->node);

  for (i = 0; i < array_n(bddArray); i++) {
    operand = array_fetch(bdd_t *, bddArray, i);
    temp = Cudd_bddRestrict(f->mgr, result, operand->node);
    if (temp == NULL) {
      Cudd_RecursiveDeref(f->mgr, result);
      return(NULL);
    }
    cuddRef(temp);
    Cudd_RecursiveDeref(f->mgr, result);
    result = temp;
  }

  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_minimize_array */


/**Function********************************************************************

  Synopsis    [Subset (superset) operator.]

  Description [It computes a bdd which is a subset (superset) of the
  given operand and has less nodes. approxDir specifies over/under
  approximation. The number of variables is an estimate of the support
  of the operand, and threshold is the maximum number of vertices
  allowed in the result. The technique applied to eliminate nodes is
  to remove a child of a node, starting with the root, that contribute
  to fewer minterms than the other child. Refer to Ravi & Somenzi
  ICCAD95.]

  SideEffects [none]

******************************************************************************/
bdd_t *
bdd_approx_hb(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold)
{
  DdNode *result;
  bdd_t *output;

  switch (approxDir) {
  case BDD_OVER_APPROX:
    result = Cudd_SupersetHeavyBranch(f->mgr, f->node, numVars, threshold);
    break;
  case BDD_UNDER_APPROX:
    result = Cudd_SubsetHeavyBranch(f->mgr, f->node, numVars, threshold);
    break;
  default:
    result = NULL;
  }
  if (result == NULL) return(NULL);
  cuddRef(result);

  output = bdd_construct_bdd_t(f->mgr,result);
  return(output);

} /* end of bdd_approx_hb */


/**Function********************************************************************

  Synopsis    [Subset (superset) operator.]

  Description [It computes a bdd which is a subset (superset) of the
  given operand and it has less nodes. approxDir specifies over/under
  approximation. The number of variables is an estimate of the support
  of the operand, and threshold is the maximum number of vertices
  allowed in the result. If unsure, pass NULL for the number of
  variables.  The method used is to extract the smallest cubes in the
  bdd which also correspond to the shortest paths in the bdd to the
  constant 1. hardlimit indicates that the node limit is strict. Refer
  to Ravi and Somenzi ICCAD95.]

  SideEffects [none]

******************************************************************************/
bdd_t *
bdd_approx_sp(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold,
  int hardlimit)
{
  DdNode *result;
  bdd_t *output;

  switch (approxDir) {
  case BDD_OVER_APPROX:
    result = Cudd_SupersetShortPaths(f->mgr, f->node, numVars, threshold, hardlimit);
    break;
  case BDD_UNDER_APPROX:
    result = Cudd_SubsetShortPaths(f->mgr, f->node, numVars, threshold, hardlimit);
    break;
  default:
    result = NULL;
  }

  if (result == NULL) return(NULL);
  cuddRef(result);

  output = bdd_construct_bdd_t(f->mgr,result);
  return(output);

} /* end of bdd_approx_sp */


/**Function********************************************************************

  Synopsis    [Subset (superset) operator.]

  Description [It computes a bdd which is a subset (superset) of the
  given operand and it has less nodes. The bdd chooses to preserve
  nodes that contribute a large number and throws away those that
  contribute fewer minterms and dominate a large number of
  nodes. approxDir specifies over/under approximation. numVars is the
  number of variables in the true support of f. threshold is a limit
  specified on the number of nodes. safe is a parameter to ensure that
  the result is never larger than the operand. quality is a factor
  that affects replacement of nodes: 1 is the default value. Values
  for quality imply that the ratio of the density of the result of
  replaced nodes to the original original is equal to the value. Refer
  to Shiple thesis.]

  SideEffects [none]

******************************************************************************/
bdd_t *
bdd_approx_ua(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold,
  int safe,
  double quality)
{
  DdNode *result;
  bdd_t *output;

  switch (approxDir) {
  case BDD_OVER_APPROX:
    result = Cudd_OverApprox(f->mgr, f->node, numVars, threshold, safe, quality);
    break;
  case BDD_UNDER_APPROX:
    result = Cudd_UnderApprox(f->mgr, f->node, numVars, threshold, safe, quality);
    break;
  default:
    result = NULL;
  }
  if (result == NULL) return(NULL);
  cuddRef(result);

  output = bdd_construct_bdd_t(f->mgr,result);
  return(output);

} /* end of bdd_approx_ua */


/**Function********************************************************************

  Synopsis    [Subset (superset) operator.]

  Description [It computes a bdd which is a subset (superset) of the
  given operand and it has less nodes.The bdd chooses to preserve
  nodes that contribute a large number and throws away those that
  contribute fewer minterms and dominate a large number of nodes. Some
  nodes may be remapped to existing nodes in the BDD. approxDir
  specifies over/under approximation. numVars is the number of
  variables in the true support of f. threshold is a limit specified
  on the number of nodes. safe is a parameter to ensure that the
  result is never larger than the operand. quality is a factor that
  affects replacement of nodes: 1 is the default value. Values for
  quality imply that the ratio of the density of the result with
  replaced nodes to the original bdd is equal to the value. Refer to
  Shiple, Somenzi DAC98. ]

  SideEffects [none]

******************************************************************************/
bdd_t *
bdd_approx_remap_ua(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold,
  double quality)
{
  DdNode *result;
  bdd_t *output;

  switch (approxDir) {
  case BDD_OVER_APPROX:
    result = Cudd_RemapOverApprox((DdManager *)f->mgr, (DdNode *)f->node, numVars, threshold, quality);
    break;
  case BDD_UNDER_APPROX:
    result = Cudd_RemapUnderApprox((DdManager *)f->mgr, (DdNode *)f->node, numVars, threshold, quality);
    break;
  default:
    result = NULL;
  }

  if (result == NULL) return(NULL);
  cuddRef(result);

  output = bdd_construct_bdd_t((DdManager *)f->mgr,result);
  return(output);

} /* end of bdd_approx_remap_ua */


/**Function********************************************************************

  Synopsis    [Subset (superset) operator.]

  Description [It computes a bdd which is a subset (superset) of the
  given operand and it has less nodes.The bdd chooses to preserve
  nodes that contribute a large number and throws away those that
  contribute fewer minterms and dominate a large number of nodes. Some
  nodes may be remapped to existing nodes in the BDD. approxDir
  specifies over/under approximation. numVars is the number of
  variables in the true support of f. threshold is a limit specified
  on the number of nodes. safe is a parameter to ensure that the
  result is never larger than the operand. quality is a factor that
  affects replacement of nodes: 1 is the default value. Values for
  quality imply that the ratio of the density of the result with
  replaced nodes to the original bdd is equal to the value. Refer
  Shiple, Somenzi DAC98. The only difference between this function and
  bdd_approx_remap_ua is that this function takes a bias BDD and tries
  to lean the approximation towards the bias]

  SideEffects [none]

******************************************************************************/
bdd_t *
bdd_approx_biased_rua(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  bdd_t *bias,
  int numVars,
  int threshold,
  double quality,
  double qualityBias)
{
  DdNode *result;
  bdd_t *output;

  assert(bias->mgr == f->mgr);
  switch (approxDir) {
  case BDD_OVER_APPROX:
    result = Cudd_BiasedOverApprox((DdManager *)f->mgr, (DdNode *)f->node, (DdNode *)bias->node,  numVars, threshold, quality, qualityBias);
    break;
  case BDD_UNDER_APPROX:
    result = Cudd_BiasedUnderApprox((DdManager *)f->mgr, (DdNode *)f->node, (DdNode *)bias->node, numVars, threshold, quality, qualityBias);
    break;
  default:
    result = NULL;
  }

  if (result == NULL) return(NULL);
  cuddRef(result);

  output = bdd_construct_bdd_t((DdManager *)f->mgr,result);
  return(output);

} /* end of bdd_approx_biased_rua */


/**Function********************************************************************

  Synopsis    [Subset (superset) operator.]

  Description [It computes a bdd which is a subset (superset) of the
  given operand and it has less nodes. approxDir specifies over/under
  approximation. The number of variables is an estimate of the support
  of the operand, and threshold is the maximum number of vertices
  allowed in the result. It applies short paths with the given
  threshold first and then uses remap_ua to increase density.]

  SideEffects [none]

******************************************************************************/
bdd_t *
bdd_approx_compress(
  bdd_t *f,
  bdd_approx_dir_t approxDir,
  int numVars,
  int threshold)
{
  DdNode *result;
  bdd_t *output;

  switch (approxDir) {
  case BDD_OVER_APPROX:
    result = Cudd_SupersetCompress(f->mgr, f->node, numVars, threshold);
    break;
  case BDD_UNDER_APPROX:
    result = Cudd_SubsetCompress(f->mgr, f->node, numVars, threshold);
    break;
  default:
    result = NULL;
  }

  if (result == NULL) return(NULL);
  cuddRef(result);

  output = bdd_construct_bdd_t(f->mgr,result);
  return(output);

} /* end of bdd_approx_compress */


/**Function********************************************************************

  Synopsis    [Finds a shortest path in a DD.]

  Description [Finds a shortest path in a DD. f is the DD we want to
  get the shortest path for; weight\[i\] is the weight of the THEN arc
  coming from the node whose index is i. If weight is NULL, then unit
  weights are assumed for all THEN arcs. All ELSE arcs have 0 weight.
  If non-NULL, both weight and support should point to arrays with at
  least as many entries as there are variables in the manager.
  Returns the shortest path as the BDD of a cube.]

  SideEffects [support contains on return the true support of f.
  If support is NULL on entry, then Cudd_ShortestPath does not compute
  the true support info. length contains the length of the path.]

******************************************************************************/
bdd_t *
bdd_shortest_path(
  bdd_t *f,
  int *weight,
  int *support,
  int *length)
{
  DdNode *result;
  bdd_t *output;

  result = Cudd_ShortestPath(f->mgr, f->node, weight, support, length);
  if (result == NULL) return(NULL);
  cuddRef(result);

  output = bdd_construct_bdd_t(f->mgr,result);
  return(output);

} /* end of bdd_shortest_path */


/**Function********************************************************************

  Synopsis    [Negation.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_not(bdd_t *f)
{
  DdNode *result;

  Cudd_Ref(result = Cudd_Not(f->node));
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_not */


/**Function********************************************************************

  Synopsis    [Returns the one BDD.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_one(bdd_manager *mgr)
{
  DdNode *result;

  Cudd_Ref(result = DD_ONE((DdManager *)mgr));
  return(bdd_construct_bdd_t((DdManager *)mgr,result));

} /* end of bdd_one */


/**Function********************************************************************

  Synopsis    [Or of two BDDs.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_or(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase)
{
  DdNode *newf,*newg,*forg;
  bdd_t *result;

  /* Make sure both bdds belong to the same mngr */
  assert(f->mgr == g->mgr);

  /* Modify the phases of the operands according to the parameters */
  if (f_phase) {
    newf = Cudd_Not(f->node);
  } else {
    newf = f->node;
  }
  if (g_phase) {
    newg = Cudd_Not(g->node);
  } else {
    newg = g->node;
  }

  /* Perform the OR operation */
  forg = Cudd_bddAnd(f->mgr,newf,newg);
  if (forg == NULL) return(NULL);
  forg = Cudd_Not(forg);
  cuddRef(forg);
  result = bdd_construct_bdd_t(f->mgr,forg);

  return(result);

} /* end of bdd_or */


/**Function********************************************************************

  Synopsis    [Existential abstraction of variables.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_smooth(
  bdd_t *f,
  array_t *smoothing_vars /* of bdd_t *'s */)
{
  int i;
  bdd_t *variable;
  DdNode *cube,*tmpDd,*result;
  DdManager *mgr;
  DdNode **vars;
  int nVars, level;

  /* The Boulder package needs the smoothing variables passed as a cube.
   * Therefore we must build that cube from the indices of the variables
   * in the array before calling the procedure.
   */
  mgr = f->mgr;
  nVars = mgr->size;
  vars = ALLOC(DdNode *, sizeof(DdNode *) * nVars);
  memset(vars, 0, sizeof(DdNode *) * nVars);
  for (i = 0; i < array_n(smoothing_vars); i++) {
    variable = array_fetch(bdd_t *,smoothing_vars,i);

    /* Make sure the variable belongs to the same manager. */
    assert(mgr == variable->mgr);

    level = Cudd_ReadPerm(mgr, Cudd_NodeReadIndex(variable->node));
    vars[level] = variable->node;
  }
  Cudd_Ref(cube = DD_ONE(mgr));
  for (i = nVars - 1; i >= 0; i--) {
    if (!vars[i])
      continue;
    tmpDd = Cudd_bddAnd(mgr,cube,vars[i]);
    if (tmpDd == NULL) {
      Cudd_RecursiveDeref(mgr, cube);
      return(NULL);
    }
    cuddRef(tmpDd);
    Cudd_RecursiveDeref(mgr, cube);
    cube = tmpDd;
  }
  FREE(vars);

  /* Perform the smoothing */
  result = Cudd_bddExistAbstract(mgr,f->node,cube);
  if (result == NULL) {
    Cudd_RecursiveDeref(mgr, cube);
    return(NULL);
  }
  cuddRef(result);

  /* Get rid of temporary results */
  Cudd_RecursiveDeref(mgr, cube);

  /* Build the bdd_t structure for the result */
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_smooth */


/**Function********************************************************************

  Synopsis    [Existential abstraction of variables.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_smooth_with_cube(bdd_t *f, bdd_t *cube)
{
  DdNode *result;
  DdManager *mgr;

  mgr = f->mgr;

  /* Perform the smoothing */
  result = Cudd_bddExistAbstract(mgr,f->node,cube->node);
  if (result == NULL)
    return(NULL);
  cuddRef(result);

  /* Build the bdd_t structure for the result */
  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_smooth_with_cube */


/**Function********************************************************************

  Synopsis    [Permutes the variables.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_substitute(
  bdd_t *f,
  array_t *old_array /* of bdd_t *'s */,
  array_t *new_array /* of bdd_t *'s */)
{
  int i,from,to;
  int *permut;
  bdd_t *variable;
  DdNode *result;

  /* Make sure both arrays have the same number of elements. */
  assert(array_n(old_array) == array_n(new_array));

  /* Allocate and fill the array with the trivial permutation. */
  permut = ALLOC(int, Cudd_ReadSize((DdManager *)f->mgr));
  for (i = 0; i < Cudd_ReadSize((DdManager *)f->mgr); i++) permut[i] = i;

  /* Modify the permutation by looking at both arrays old and new. */
  for (i = 0; i < array_n(old_array); i++) {
    variable = array_fetch(bdd_t *, old_array, i);
    from = Cudd_Regular(variable->node)->index;
    variable = array_fetch(bdd_t *, new_array, i);
    /* Make sure the variable belongs to this manager. */
    assert(f->mgr == variable->mgr);

    to = Cudd_Regular(variable->node)->index;
    permut[from] = to;
  }

  result = Cudd_bddPermute(f->mgr,f->node,permut);
  FREE(permut);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_substitute */


/**Function********************************************************************

  Synopsis    [Permutes the variables.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_substitute_with_permut(
  bdd_t *f,
  int *permut)
{
  DdNode *result;

  result = Cudd_bddPermute(f->mgr,f->node,permut);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_substitute_with_permut */


/**Function********************************************************************

  Synopsis    [Permutes the variables.]

  SideEffects []

******************************************************************************/
array_t *
bdd_substitute_array(
  array_t *f_array,
  array_t *old_array,	/* of bdd_t *'s */
  array_t *new_array)	/* of bdd_t *'s */
{
  int	i;
  bdd_t	*f, *new_;
  array_t *substitute_array = array_alloc(bdd_t *, 0);

  arrayForEachItem(bdd_t *, f_array, i, f) {
    new_ = bdd_substitute(f, old_array, new_array);
    array_insert_last(bdd_t *, substitute_array, new_);
  }
  return(substitute_array);

} /* end of bdd_substitute_array */


/**Function********************************************************************

  Synopsis    [Permutes the variables.]

  SideEffects []

******************************************************************************/
array_t *
bdd_substitute_array_with_permut(
  array_t *f_array,
  int *permut)
{
  int	i;
  bdd_t	*f, *new_;
  array_t *substitute_array = array_alloc(bdd_t *, 0);

  arrayForEachItem(bdd_t *, f_array, i, f) {
    new_ = bdd_substitute_with_permut(f, permut);
    array_insert_last(bdd_t *, substitute_array, new_);
  }
  return(substitute_array);

} /* end of bdd_substitute_array_with_permut */


/**Function********************************************************************

  Synopsis    [Returns the pointer of the BDD.]

  SideEffects []

******************************************************************************/
void *
bdd_pointer(bdd_t *f)
{
  return((void *)f->node);

} /* end of bdd_pointer */


/**Function********************************************************************

  Synopsis    [Returns the Then branch of the BDD.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_then(bdd_t *f)
{
  DdNode *result;

  result = Cudd_T(f->node);
  result =  Cudd_NotCond(result,Cudd_IsComplement(f->node));
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_then */


/**Function********************************************************************

  Synopsis    [Returns the BDD of the top variable.]

  Description [Returns the BDD of the top variable of the argument. If
  the argument is constant, it returns the constant function itself.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_top_var(bdd_t *f)
{
  DdNode *result;

  if (Cudd_IsConstant(f->node)) {
    result = f->node;
  } else {
    result = f->mgr->vars[Cudd_Regular(f->node)->index];
  }
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_top_var */


/**Function********************************************************************

  Synopsis    [Computes the exclusive nor of two BDDs.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_xnor(bdd_t *f, bdd_t *g)
{
  DdNode *result;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);

  result = Cudd_bddXnor(f->mgr,f->node,g->node);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_xnor */


/**Function********************************************************************

  Synopsis    [Computes the exclusive or of two BDDs.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_xor(bdd_t *f, bdd_t *g)
{
  DdNode *result;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);

  result = Cudd_bddXor(f->mgr,f->node,g->node);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_xor */


/**Function********************************************************************

  Synopsis    [Returns the constant logical zero BDD.]

  SideEffects [bdd_read_zero]

******************************************************************************/
bdd_t *
bdd_zero(bdd_manager *mgr)
{
  DdManager *manager;
  DdNode *result;

  manager = (DdManager *)mgr;
  Cudd_Ref(result = Cudd_Not(DD_ONE((manager))));
  return(bdd_construct_bdd_t(manager,result));

} /* end of bdd_zero */


/**Function********************************************************************

  Synopsis    [Equality check.]

  SideEffects []

******************************************************************************/
boolean
bdd_equal(bdd_t *f, bdd_t *g)
{
  return(f->node == g->node);

} /* end of bdd_equal */


/**Function********************************************************************

  Synopsis    [Equality check with don't care conditions.]

  Description [Returns 1 if f equals g wherever careSet is 1.]

  SideEffects [None: No new nodes are created.]

******************************************************************************/
boolean
bdd_equal_mod_care_set(
  bdd_t *f,
  bdd_t *g,
  bdd_t *careSet)
{
  /* Make sure all operands belong to the same manager. */
  assert(f->mgr == g->mgr && f->mgr == careSet->mgr);
  return(Cudd_EquivDC(f->mgr, f->node, g->node, Cudd_Not(careSet->node)));

} /* end of bdd_equal_mod_care_set */


/**Function********************************************************************

  Synopsis    [Returns a BDD included in the intersection of f and g.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_intersects(
  bdd_t *f,
  bdd_t *g)
{
  DdNode *result;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);

  result = Cudd_bddIntersect(f->mgr,f->node,g->node);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_intersects */


/**Function********************************************************************

  Synopsis    [Returns a BDD included in f at minimum distance from g.]

  SideEffects [The distance is returned as a side effect in dist.]

******************************************************************************/
bdd_t *
bdd_closest_cube(
  bdd_t *f,
  bdd_t *g,
  int *dist)
{
  DdNode *result;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);

  result = Cudd_bddClosestCube(f->mgr,f->node,g->node,dist);
  if (result == NULL) return(NULL);
  cuddRef(result);
  return(bdd_construct_bdd_t(f->mgr,result));

} /* end of bdd_closest_cube */


/**Function********************************************************************

  Synopsis    [Checks a BDD for tautology.]

  SideEffects []

******************************************************************************/
boolean
bdd_is_tautology(bdd_t *f, boolean phase)
{
  if (phase) {
    return(f->node == DD_ONE(f->mgr));
  } else {
    return(f->node == Cudd_Not(DD_ONE(f->mgr)));
  }

} /* end of bdd_is_tautology */


/**Function********************************************************************

  Synopsis    [Tests for containment of f in g.]

  SideEffects [None: No new nodes are created.]

******************************************************************************/
boolean
bdd_leq(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase)
{
  DdNode *newf, *newg;

  /* Make sure both operands belong to the same manager. */
  assert(f->mgr == g->mgr);
  newf = Cudd_NotCond(f->node, !f_phase);
  newg = Cudd_NotCond(g->node, !g_phase);

  return(Cudd_bddLeq(f->mgr,newf,newg));

} /* end of bdd_leq */


/**Function********************************************************************

  Synopsis    [Implication check with don't care conditions.]

  Description [Returns 1 if f implies g wherever careSet is 1.]

  SideEffects [None: No new nodes are created.]

******************************************************************************/
boolean
bdd_lequal_mod_care_set(
  bdd_t *f,
  bdd_t *g,
  boolean f_phase,
  boolean g_phase,
  bdd_t *careSet)
{
  DdNode *newf, *newg;

  /* Make sure all operands belong to the same manager. */
  assert(f->mgr == g->mgr && f->mgr == careSet->mgr);
  newf = Cudd_NotCond(f->node, !f_phase);
  newg = Cudd_NotCond(g->node, !g_phase);

  return(Cudd_bddLeqUnless(f->mgr, newf, newg, Cudd_Not(careSet->node)));

} /* end of bdd_lequal_mod_care_set */


/**Function********************************************************************

  Synopsis    [Tests for containment of f in g.]

  SideEffects []

******************************************************************************/
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

} /* end of bdd_leq_array */


/**Function********************************************************************

  Synopsis    [Counts the number of minterms in the on set.]

  SideEffects []

******************************************************************************/
double
bdd_count_onset(
  bdd_t *f,
  array_t *var_array /* of bdd_t *'s */)
{
  return(Cudd_CountMinterm(f->mgr,f->node,array_n(var_array)));

} /* end of bdd_count_onset */


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
  return(Cudd_EpdCountMinterm(f->mgr,f->node,array_n(var_array),epd));

} /* end of bdd_epd_count_onset */


/**Function********************************************************************

  Synopsis    [Returns the free field of the BDD.]

  SideEffects []

******************************************************************************/
int
bdd_get_free(bdd_t *f)
{
  return(f->free);

} /* end of bdd_get_free */


/**Function********************************************************************

  Synopsis    [Obtains the manager of the BDD.]

  SideEffects []

******************************************************************************/
bdd_manager *
bdd_get_manager(bdd_t *f)
{
  return(f->mgr);

} /* end of bdd_get_manager */


/**Function********************************************************************

  Synopsis    [Returns the node of the BDD.]

  SideEffects [Sets is_complemented.]

******************************************************************************/
bdd_node *
bdd_get_node(
  bdd_t *f,
  boolean *is_complemented /* return */)
{
  if (Cudd_IsComplement(f->node)) {
    *is_complemented = TRUE;
    return(Cudd_Regular(f->node));
  }
  *is_complemented = FALSE;
  return(f->node);

} /* end of bdd_get_node */


/**Function********************************************************************

  Synopsis    [Obtains the support of the BDD.]

  SideEffects []

******************************************************************************/
var_set_t *
bdd_get_support(bdd_t *f)
{
  int i, size, *support;
  var_set_t *result;

  size = (unsigned int)Cudd_ReadSize((DdManager *)f->mgr);
  support = Cudd_SupportIndex(f->mgr,f->node);
  if (support == NULL) return(NULL);

  result = var_set_new((int) f->mgr->size);
  for (i = 0; i < size; i++) {
    if (support[i])
      var_set_set_elt(result, i);
  }
  FREE(support);

  return(result);

} /* end of bdd_get_support */


/**Function********************************************************************

  Synopsis    [Checks whether a BDD is a support of f.]

  SideEffects []

******************************************************************************/
int
bdd_is_support_var(bdd_t *f, bdd_t *var)
{
  return(bdd_is_support_var_id(f, bdd_top_var_id(var)));

} /* end of bdd_is_support_var */


/**Function********************************************************************

  Synopsis    [Checks whether a BDD index is a support of f.]

  SideEffects []

******************************************************************************/
int
bdd_is_support_var_id(bdd_t *f, int index)
{
  DdNode *support, *scan;

  support = Cudd_Support(f->mgr,f->node);
  if (support == NULL) return(-1);
  cuddRef(support);

  scan = support;
  while (!cuddIsConstant(scan)) {
    if (scan->index == index) {
      Cudd_RecursiveDeref(f->mgr,support);
      return(1);
    }
    scan = cuddT(scan);
  }
  Cudd_RecursiveDeref(f->mgr,support);

  return(0);

} /* end of bdd_is_support_var_id */


/**Function********************************************************************

  Synopsis    [Obtains the array of indices of an array of variables.]

  SideEffects []

******************************************************************************/
array_t *
bdd_get_varids(array_t *var_array)
{
  int i;
  int index;
  bdd_t *var;
  array_t *result = array_alloc(int,array_n(var_array));

  for (i = 0; i < array_n(var_array); i++) {
    var = array_fetch(bdd_t *, var_array, i);
    index = Cudd_Regular(var->node)->index;
    (void) array_insert_last(int, result, index);
  }
  return(result);

} /* end of bdd_get_varids */


/**Function********************************************************************

  Synopsis    [Returns the number of variables in the manager.]

  SideEffects []

******************************************************************************/
unsigned int
bdd_num_vars(bdd_manager *mgr)
{
  unsigned int size;
  size = (unsigned int)Cudd_ReadSize((DdManager *)mgr);
  return(size);

} /* end of bdd_num_vars */


/**Function********************************************************************

  Synopsis    [Prints the BDD.]

  SideEffects []

******************************************************************************/
void
bdd_print(bdd_t *f)
{
  (void) cuddP(f->mgr,f->node);

} /* end of bdd_print */


/**Function********************************************************************

  Synopsis    [Prints statistics about the package.]

  SideEffects []

******************************************************************************/
void
bdd_print_stats(bdd_manager *mgr, FILE *file)
{
  Cudd_PrintInfo((DdManager *)mgr, file);

  /* Print some guidance to the parameters */
  (void) fprintf(file, "\nMore detailed information about the semantics ");
  (void) fprintf(file, "and values of these parameters\n");
  (void) fprintf(file, "can be found in the documentation about the CU ");
  (void) fprintf(file, "Decision Diagram Package.\n");

  return;

} /* end of bdd_print_stats */


/**Function********************************************************************

  Synopsis [Sets the internal parameters of the package to the given values.]

  Description [The CUDD package has a set of parameters that can be assigned
  different values. This function receives a table which maps strings to
  values and sets the parameters represented by the strings to the pertinent
  values. Some basic type checking is done. It returns 1 if everything is
  correct and 0 otherwise.]

  SideEffects []

******************************************************************************/
int
bdd_set_parameters(
  bdd_manager *mgr,
  avl_tree *valueTable,
  FILE *file)
{
  Cudd_ReorderingType reorderMethod;
  Cudd_ReorderingType zddReorderMethod;
  st_table *newValueTable;
  st_generator *stgen;
  avl_generator *avlgen;
  char *paramName;
  char *paramValue;

  /* Initial value of the variables. */
  reorderMethod = CUDD_REORDER_SAME;
  zddReorderMethod = CUDD_REORDER_SAME;

  /* Build a new table with the parameter names but with
  ** the prefix removed. */
  newValueTable = st_init_table(st_ptrcmp, st_ptrhash);
  avl_foreach_item(valueTable, avlgen, AVL_FORWARD, (char **)&paramName,
		   (char **)&paramValue) {
    if (strncmp(paramName, "BDD.", 4) == 0) {
      st_insert(newValueTable, (char *)&paramName[4],
		(char *)paramValue);
    }
  }

  st_foreach_item(newValueTable, stgen, &paramName, &paramValue) {
    int uvalue;
    char *invalidChar;

    invalidChar = NIL(char);

    if (strcmp(paramName, "Hard limit for cache size") == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Hard limit for cache size",
		    "unsigned integer");
      }
      else {
	Cudd_SetMaxCacheHard((DdManager *) mgr, (unsigned int) uvalue);
      }
    }
    else if (strcmp(paramName, "Cache hit threshold for resizing") == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Cache hit threshold for resizing",
		    "unsigned integer");
      }
      else {
	Cudd_SetMinHit((DdManager *) mgr, (unsigned int) uvalue);
      }
    }
    else if (strcmp(paramName, "Garbage collection enabled") == 0) {
      if (strcmp(paramValue, "yes") == 0) {
	Cudd_EnableGarbageCollection((DdManager *) mgr);
      }
      else if (strcmp(paramValue, "no") == 0) {
	Cudd_DisableGarbageCollection((DdManager *) mgr);
      }
      else {
	InvalidType(file, "Garbage collection enabled", "(yes,no)");
      }
    }
    else if (strcmp(paramName, "Limit for fast unique table growth")
	     == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Limit for fast unique table growth",
		    "unsigned integer");
      }
      else {
	Cudd_SetLooseUpTo((DdManager *) mgr, (unsigned int) uvalue);
      }
    }
    else if (strcmp(paramName,
		    "Maximum number of variables sifted per reordering")
	     == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Maximum number of variables sifted per reordering",
		    "unsigned integer");
      }
      else {
	Cudd_SetSiftMaxVar((DdManager *) mgr, uvalue);
      }
    }
    else if (strcmp(paramName,
		    "Maximum number of variable swaps per reordering")
	     == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Maximum number of variable swaps per reordering",
		    "unsigned integer");
      }
      else {
	Cudd_SetSiftMaxSwap((DdManager *) mgr, uvalue);
      }
    }
    else if (strcmp(paramName,
		    "Maximum growth while sifting a variable") == 0) {
      double value;

      value = strtod(paramValue, &invalidChar);
      if (*invalidChar) {
	InvalidType(file, "Maximum growth while sifting a variable",
		    "real");
      }
      else {
	Cudd_SetMaxGrowth((DdManager *) mgr, value);
      }
    }
    else if (strcmp(paramName, "Dynamic reordering of BDDs enabled")
	     == 0) {
      if (strcmp(paramValue, "yes") == 0) {
	Cudd_AutodynEnable((DdManager *) mgr, reorderMethod);
      }
      else if (strcmp(paramValue, "no") == 0) {
	Cudd_AutodynDisable((DdManager *) mgr);
      }
      else {
	InvalidType(file, "Dynamic reordering of BDDs enabled",
		    "(yes,no)");
      }
    }
    else if (strcmp(paramName, "Default BDD reordering method") == 0) {
      Cudd_ReorderingType reorderInt;

      reorderMethod = (Cudd_ReorderingType) strtol(paramValue,
						   &invalidChar, 10);
      if (*invalidChar || reorderMethod < 0) {
	InvalidType(file, "Default BDD reordering method", "integer");
      }
      else {
	if (Cudd_ReorderingStatus((DdManager *) mgr, &reorderInt)) {
	  Cudd_AutodynEnable((DdManager *) mgr, reorderMethod);
	}
      }
    }
    else if (strcmp(paramName, "Dynamic reordering of ZDDs enabled")
	     == 0) {
      if (strcmp(paramValue, "yes") == 0) {
	Cudd_AutodynEnableZdd((DdManager *) mgr, zddReorderMethod);
      }
      else if (strcmp(paramValue, "no") == 0) {
	Cudd_AutodynDisableZdd((DdManager *) mgr);
      }
      else {
	InvalidType(file, "Dynamic reordering of ZDDs enabled", "(yes,no)");
      }
    }
    else if (strcmp(paramName, "Default ZDD reordering method") == 0) {
      Cudd_ReorderingType reorderInt;

      zddReorderMethod = (Cudd_ReorderingType) strtol(paramValue,
						      &invalidChar, 10);
      if (*invalidChar || zddReorderMethod < 0) {
	InvalidType(file, "Default ZDD reordering method", "integer");
      }
      else {
	if (Cudd_ReorderingStatusZdd((DdManager *) mgr, &reorderInt)) {
	  Cudd_AutodynEnableZdd((DdManager *) mgr, zddReorderMethod);
	}
      }
    }
    else if (strcmp(paramName, "Realignment of ZDDs to BDDs enabled")
	     == 0) {
      if (strcmp(paramValue, "yes") == 0) {
	Cudd_zddRealignEnable((DdManager *) mgr);
      }
      else if (strcmp(paramValue, "no") == 0) {
	Cudd_zddRealignDisable((DdManager *) mgr);
      }
      else {
	InvalidType(file, "Realignment of ZDDs to BDDs enabled",
		    "(yes,no)");
      }
    }
    else if (strcmp(paramName,
		    "Dead node counted in triggering reordering") == 0) {
      if (strcmp(paramValue, "yes") == 0) {
	Cudd_TurnOnCountDead((DdManager *) mgr);
      }
      else if (strcmp(paramValue, "no") == 0) {
	Cudd_TurnOffCountDead((DdManager *) mgr);
      }
      else {
	InvalidType(file,
		    "Dead node counted in triggering reordering",
		    "(yes,no)");
      }
    }
    else if (strcmp(paramName, "Group checking criterion") == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Group checking criterion", "integer");
      }
      else {
	Cudd_SetGroupcheck((DdManager *) mgr, (Cudd_AggregationType) uvalue);
      }
    }
    else if (strcmp(paramName, "Recombination threshold") == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Recombination threshold", "integer");
      }
      else {
	Cudd_SetRecomb((DdManager *) mgr, uvalue);
      }
    }
    else if (strcmp(paramName, "Symmetry violation threshold") == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Symmetry violation threshold", "integer");
      }
      else {
	Cudd_SetSymmviolation((DdManager *) mgr, uvalue);
      }
    }
    else if (strcmp(paramName, "Arc violation threshold") == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Arc violation threshold", "integer");
      }
      else {
	Cudd_SetArcviolation((DdManager *) mgr, uvalue);
      }
    }
    else if (strcmp(paramName, "GA population size") == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar  || uvalue < 0) {
	InvalidType(file, "GA population size", "integer");
      }
      else {
	Cudd_SetPopulationSize((DdManager *) mgr, uvalue);
      }
    }
    else if (strcmp(paramName, "Number of crossovers for GA") == 0) {

      uvalue = strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Number of crossovers for GA", "integer");
      }
      else {
	Cudd_SetNumberXovers((DdManager *) mgr, uvalue);
      }
    }
    else if (strcmp(paramName, "Next reordering threshold") == 0) {

      uvalue = (unsigned int) strtol(paramValue, &invalidChar, 10);
      if (*invalidChar || uvalue < 0) {
	InvalidType(file, "Next reordering threshold", "integer");
      }
      else {
	Cudd_SetNextReordering((DdManager *) mgr, uvalue);
      }
    }
    else {
      (void) fprintf(file, "Warning: Parameter %s not recognized.",
		     paramName);
      (void) fprintf(file, " Ignored.\n");
    }
  } /* end of st_foreach_item */

  /* Clean up. */
  st_free_table(newValueTable);

  return(1);

} /* end of bdd_set_parameters */


/**Function********************************************************************

  Synopsis    [Computes the number of nodes of a BDD.]

  SideEffects []

******************************************************************************/
int
bdd_size(bdd_t *f)
{
  return(Cudd_DagSize(f->node));

} /* end of bdd_size */


/**Function********************************************************************

  Synopsis    [Computes the number of nodes of a BDD.]

  SideEffects []

******************************************************************************/
int
bdd_node_size(bdd_node *f)
{
  return(Cudd_DagSize((DdNode *) f));

} /* end of bdd_node_size */


/**Function********************************************************************

  Synopsis    [Computes the shared size of an array of BDDs.]

  Description [Computes the shared size of an array of BDDs. Returns
  CUDD_OUT_OF_MEM in case of failure.]

  SideEffects []

******************************************************************************/
long
bdd_size_multiple(array_t *bddArray)
{
  DdNode **nodeArray;
  bdd_t *bddUnit;
  long result;
  int i;

  nodeArray = ALLOC(DdNode *, array_n(bddArray));
  if (nodeArray == NULL) return(CUDD_OUT_OF_MEM);
  for (i = 0; i < array_n(bddArray); i++) {
    bddUnit = array_fetch(bdd_t *, bddArray, i);
    nodeArray[i] = bddUnit->node;
  }

  result = Cudd_SharingSize(nodeArray,array_n(bddArray));

  /* Clean up */
  FREE(nodeArray);

  return(result);

} /* end of bdd_size_multiple */


/**Function********************************************************************

  Synopsis    [Accesses the id of the top variable.]

  SideEffects []

******************************************************************************/
bdd_variableId
bdd_top_var_id(bdd_t *f)
{
  return(Cudd_Regular(f->node)->index);

} /* end of bdd_top_var_id */


/**Function********************************************************************

  Synopsis    [Accesses the external_hooks field of the manager.]

  SideEffects []

******************************************************************************/
bdd_external_hooks *
bdd_get_external_hooks(bdd_manager *mgr)
{
  return((bdd_external_hooks *)(((DdManager *)mgr)->hooks));

} /* end of bdd_get_external_hooks */


/**Function********************************************************************

  Synopsis    [Adds a function to a hook.]

  SideEffects []

******************************************************************************/
int
bdd_add_hook(
  bdd_manager *mgr,
  int (*procedure)(bdd_manager *, char *, void *),
  bdd_hook_type_t whichHook)
{
  int retval;
  Cudd_HookType hook;
  switch (whichHook) {
  case BDD_PRE_GC_HOOK: hook = CUDD_PRE_GC_HOOK; break;
  case BDD_POST_GC_HOOK: hook = CUDD_POST_GC_HOOK; break;
  case BDD_PRE_REORDERING_HOOK: hook = CUDD_PRE_REORDERING_HOOK; break;
  case BDD_POST_REORDERING_HOOK: hook = CUDD_POST_REORDERING_HOOK; break;
  default: fprintf(stderr, "Dont know which hook"); return 0;
  }

  retval = Cudd_AddHook((DdManager *)mgr, (DD_HFP)procedure, hook);
  return retval;

} /* end of bdd_add_hook */


/**Function********************************************************************

  Synopsis    [Removes the function from the hook.]

  SideEffects []

******************************************************************************/
int
bdd_remove_hook(
  bdd_manager *mgr,
  int (*procedure)(bdd_manager *, char *, void *),
  bdd_hook_type_t whichHook)
{
  int retval;
  Cudd_HookType hook;
  switch (whichHook) {
  case BDD_PRE_GC_HOOK: hook = CUDD_PRE_GC_HOOK; break;
  case BDD_POST_GC_HOOK: hook = CUDD_POST_GC_HOOK; break;
  case BDD_PRE_REORDERING_HOOK: hook = CUDD_PRE_REORDERING_HOOK; break;
  case BDD_POST_REORDERING_HOOK: hook = CUDD_POST_REORDERING_HOOK; break;
  default: fprintf(stderr, "Dont know which hook"); return 0;
  }
  retval = Cudd_RemoveHook((DdManager *)mgr, (DD_HFP)procedure, hook);
  return retval;

} /* end of bdd_remove_hook */


/**Function********************************************************************

  Synopsis    [Enables reporting of reordering stats.]

  SideEffects []

******************************************************************************/
int
bdd_enable_reordering_reporting(bdd_manager *mgr)
{
  int retval;
  retval = Cudd_EnableReorderingReporting((DdManager *) mgr);
  return retval;

} /* end of bdd_enable_reordering_reporting */


/**Function********************************************************************

  Synopsis    [Disables reporting of reordering stats.]

  SideEffects []

******************************************************************************/
int
bdd_disable_reordering_reporting(bdd_manager *mgr)
{
  int retval;
  retval = Cudd_DisableReorderingReporting((DdManager *) mgr);
  return retval;

} /* end of bdd_disable_reordering_reporting */


/**Function********************************************************************

  Synopsis    [Enables monitoring of variable ordering.]

  SideEffects []

******************************************************************************/
int
bdd_enable_ordering_monitoring(bdd_manager *mgr)
{
  int retval;
  retval = Cudd_EnableOrderingMonitoring((DdManager *) mgr);
  return retval;

} /* end of bdd_enable_ordering_monitoring */


/**Function********************************************************************

  Synopsis    [Disables monitoring of variable ordering.]

  SideEffects []

******************************************************************************/
int
bdd_disable_ordering_monitoring(bdd_manager *mgr)
{
  int retval;
  retval = Cudd_DisableOrderingMonitoring((DdManager *) mgr);
  return retval;

} /* end of bdd_disable_ordering_monitoring */


/**Function********************************************************************

  Synopsis    [ Reporting of reordering stats.]

  SideEffects []

******************************************************************************/
bdd_reorder_verbosity_t
bdd_reordering_reporting(bdd_manager *mgr)
{
  int retval;
  bdd_reorder_verbosity_t reorderVerbosity;
  retval = Cudd_ReorderingReporting((DdManager *) mgr);
  switch(retval) {
  case 0: reorderVerbosity = BDD_REORDER_NO_VERBOSITY; break;
  case 1: reorderVerbosity = BDD_REORDER_VERBOSITY; break;
  default: reorderVerbosity = BDD_REORDER_VERBOSITY_DEFAULT; break;
  }
  return reorderVerbosity;

} /* end of bdd_reordering_reporting */


/**Function********************************************************************

  Synopsis    [Turns on or off garbage collection.]

  SideEffects []

******************************************************************************/
void
bdd_set_gc_mode(bdd_manager *mgr, boolean no_gc)
{
  if (no_gc) {
    Cudd_DisableGarbageCollection((DdManager *) mgr);
  } else {
    Cudd_EnableGarbageCollection((DdManager *) mgr);
  }
  return;

} /* end of bdd_set_gc_mode */


/**Function********************************************************************

  Synopsis    [Reorders the BDD pool.]

  SideEffects []

******************************************************************************/
void
bdd_dynamic_reordering(
  bdd_manager *mgr_,
  bdd_reorder_type_t algorithm_type,
  bdd_reorder_verbosity_t verbosity)
{
  DdManager *mgr;

  mgr = (DdManager *)mgr_;

  switch (algorithm_type) {
  case BDD_REORDER_SIFT:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_SIFT);
    break;
  case BDD_REORDER_WINDOW:
  case BDD_REORDER_WINDOW3_CONV:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_WINDOW3_CONV);
    break;
  case BDD_REORDER_NONE:
    Cudd_AutodynDisable(mgr);
    break;
  case BDD_REORDER_SAME:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_SAME);
    break;
  case BDD_REORDER_RANDOM:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_RANDOM);
    break;
  case BDD_REORDER_RANDOM_PIVOT:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_RANDOM_PIVOT);
    break;
  case BDD_REORDER_SIFT_CONVERGE:
    Cudd_AutodynEnable(mgr,CUDD_REORDER_SIFT_CONVERGE);
    break;
  case BDD_REORDER_SYMM_SIFT:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_SYMM_SIFT);
    break;
  case BDD_REORDER_SYMM_SIFT_CONV:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_SYMM_SIFT_CONV);
    break;
  case BDD_REORDER_WINDOW2:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_WINDOW2);
    break;
  case BDD_REORDER_WINDOW4:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_WINDOW4);
    break;
  case BDD_REORDER_WINDOW2_CONV:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_WINDOW2_CONV);
    break;
  case BDD_REORDER_WINDOW3:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_WINDOW3);
    break;
  case BDD_REORDER_WINDOW4_CONV:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_WINDOW4_CONV);
    break;
  case BDD_REORDER_GROUP_SIFT:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_GROUP_SIFT);
    break;
  case BDD_REORDER_GROUP_SIFT_CONV:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_GROUP_SIFT_CONV);
    break;
  case BDD_REORDER_ANNEALING:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_ANNEALING);
    break;
  case BDD_REORDER_GENETIC:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_GENETIC);
    break;
  case BDD_REORDER_EXACT:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_EXACT);
    break;
  case BDD_REORDER_LAZY_SIFT:
    Cudd_AutodynEnable(mgr, CUDD_REORDER_LAZY_SIFT);
    break;
  default:
    fprintf(stderr,"CU DD Package: Reordering algorithm not considered\n");
  }

  if (verbosity == BDD_REORDER_NO_VERBOSITY) {
    (void) bdd_disable_reordering_reporting((DdManager *)mgr);
  } else if (verbosity ==  BDD_REORDER_VERBOSITY) {
    (void) bdd_enable_reordering_reporting((DdManager *)mgr);
  } else if (verbosity ==  BDD_REORDER_MORE_VERBOSITY) {
    (void) bdd_enable_ordering_monitoring((DdManager *)mgr);
  }

} /* end of bdd_dynamic_reordering */


/**Function********************************************************************

  Synopsis    [Reorders the ZDD pool.]

  SideEffects []

******************************************************************************/
void
bdd_dynamic_reordering_zdd(
  bdd_manager *mgr_,
  bdd_reorder_type_t algorithm_type,
  bdd_reorder_verbosity_t verbosity)
{
  DdManager *mgr;

  mgr = (DdManager *)mgr_;

  switch (algorithm_type) {
  case BDD_REORDER_SIFT:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_SIFT);
    break;
  case BDD_REORDER_WINDOW:
  case BDD_REORDER_WINDOW3_CONV:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_WINDOW3_CONV);
    break;
  case BDD_REORDER_NONE:
    Cudd_AutodynDisable(mgr);
    break;
  case BDD_REORDER_SAME:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_SAME);
    break;
  case BDD_REORDER_RANDOM:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_RANDOM);
    break;
  case BDD_REORDER_RANDOM_PIVOT:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_RANDOM_PIVOT);
    break;
  case BDD_REORDER_SIFT_CONVERGE:
    Cudd_AutodynEnableZdd(mgr,CUDD_REORDER_SIFT_CONVERGE);
    break;
  case BDD_REORDER_SYMM_SIFT:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_SYMM_SIFT);
    break;
  case BDD_REORDER_SYMM_SIFT_CONV:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_SYMM_SIFT_CONV);
    break;
  case BDD_REORDER_WINDOW2:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_WINDOW2);
    break;
  case BDD_REORDER_WINDOW4:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_WINDOW4);
    break;
  case BDD_REORDER_WINDOW2_CONV:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_WINDOW2_CONV);
    break;
  case BDD_REORDER_WINDOW3:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_WINDOW3);
    break;
  case BDD_REORDER_WINDOW4_CONV:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_WINDOW4_CONV);
    break;
  case BDD_REORDER_GROUP_SIFT:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_GROUP_SIFT);
    break;
  case BDD_REORDER_GROUP_SIFT_CONV:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_GROUP_SIFT_CONV);
    break;
  case BDD_REORDER_ANNEALING:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_ANNEALING);
    break;
  case BDD_REORDER_GENETIC:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_GENETIC);
    break;
  case BDD_REORDER_EXACT:
    Cudd_AutodynEnableZdd(mgr, CUDD_REORDER_EXACT);
    break;
  default:
    fprintf(stderr,"CU DD Package: Reordering algorithm not considered\n");
  }
  if (verbosity == BDD_REORDER_NO_VERBOSITY) {
    (void) bdd_disable_reordering_reporting((DdManager *)mgr);
  } else if (verbosity ==  BDD_REORDER_VERBOSITY) {
    (void) bdd_enable_reordering_reporting((DdManager *)mgr);
  }

} /* end of bdd_dynamic_reordering_zdd */


/**Function********************************************************************

  Synopsis    [Calls reordering explicitly.]

  SideEffects []

******************************************************************************/
void
bdd_reorder(bdd_manager *mgr)
{
  /* 10 = whatever (Verbatim from file ddTable.c) */
  (void) Cudd_ReduceHeap((DdManager *)mgr,((DdManager *)mgr)->autoMethod,10);
  return;

} /* end of bdd_reorder */


/**Function********************************************************************

  Synopsis    [Gets the id variable for one level in the BDD.]

  SideEffects []

******************************************************************************/
bdd_variableId
bdd_get_id_from_level(bdd_manager *mgr, long level)
{
  int result;
  result = Cudd_ReadInvPerm((DdManager *) mgr, (int)level);
  return(result);

} /* end of bdd_get_id_from_level */


/**Function********************************************************************

  Synopsis    [Gets the level of the top variable of the BDD.]

  SideEffects []

******************************************************************************/
long
bdd_top_var_level(bdd_manager *mgr, bdd_t *fn)
{
  return((long) cuddI((DdManager *)mgr,Cudd_Regular(fn->node)->index));

} /* end of bdd_top_var_level */


/**Function********************************************************************

  Synopsis    [Returns TRUE if the argument BDD is a cube; FALSE
  otherwise.]

  SideEffects []

******************************************************************************/
boolean
bdd_is_cube(bdd_t *f)
{
  struct DdManager *manager;

  if (f == NULL) {
    fail("bdd_is_cube: invalid BDD");
  }
  if (f->free) fail ("Freed BDD passed to bdd_is_cube");
  manager =  f->mgr;
  return((boolean)Cudd_CheckCube(manager,f->node));

} /* end of bdd_is_cube */


/**Function********************************************************************

  Synopsis    [Builds a group of variables that should stay adjacent
  during reordering.]

  Description [Builds a group of variables that should stay adjacent
  during reordering. The group is made up of n variables. The first
  variable in the group is f. The other variables are the n-1
  variables following f in the order at the time of invocation of this
  function. Returns a handle to the variable group if successful; NULL
  otherwise.]

  SideEffects [Modifies the variable tree.]

******************************************************************************/
bdd_block *
bdd_new_var_block(bdd_t *f, long n)
{
  DdManager *manager;
  DdNode *node;
  MtrNode *group;
  int index;

  manager = (DdManager *) f->mgr;
  node = Cudd_Regular(f->node);
  index = node->index;
  if (index == CUDD_MAXINDEX)
    return(NULL);
  group = Cudd_MakeTreeNode(manager, index, n, MTR_DEFAULT);

  return((bdd_block *) group);

} /* end of bdd_new_var_block */


/**Function********************************************************************

  Synopsis    [Function that creates a variable of a given index.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_var_with_index(bdd_manager *manager, int index)
{
  DdNode *var;

  var = Cudd_bddIthVar((DdManager *) manager, index);
  cuddRef(var);
  return(bdd_construct_bdd_t(manager, var));

} /* end of bdd_var_with_index */


/**Function********************************************************************

  Synopsis    [Checks whether a variable is dependent on others in a
  function f. Returns 1 if it is, else 0. ]

  SideEffects []

******************************************************************************/
int
bdd_var_is_dependent(bdd_t *f, bdd_t *var)
{
  return (Cudd_bddVarIsDependent((DdManager *)f->mgr, (DdNode *)f->node,
				 (DdNode *)var->node));

} /* end of bdd_var_is_dependent */


/**Function********************************************************************

  Synopsis    []

  SideEffects []

******************************************************************************/
int
bdd_reordering_status(bdd_manager *mgr, bdd_reorder_type_t *method)
{
  int dyn;

  dyn = Cudd_ReorderingStatus((DdManager *)mgr, (Cudd_ReorderingType  *)method);
  switch (*method) {
  case CUDD_REORDER_SIFT:
    *method = BDD_REORDER_SIFT;
    break;
  case CUDD_REORDER_WINDOW3_CONV:
    *method = BDD_REORDER_WINDOW3_CONV;
    break;
  case CUDD_REORDER_NONE:
    *method = BDD_REORDER_NONE;
    break;
  case CUDD_REORDER_SAME:
    *method = BDD_REORDER_SAME;
    break;
  case CUDD_REORDER_RANDOM:
    *method = BDD_REORDER_RANDOM;
    break;
  case CUDD_REORDER_RANDOM_PIVOT:
    *method = BDD_REORDER_RANDOM_PIVOT;
    break;
  case CUDD_REORDER_SIFT_CONVERGE:
    *method = BDD_REORDER_SIFT_CONVERGE;
    break;
  case CUDD_REORDER_SYMM_SIFT:
    *method = BDD_REORDER_SYMM_SIFT;
    break;
  case CUDD_REORDER_SYMM_SIFT_CONV:
    *method = BDD_REORDER_SYMM_SIFT_CONV;
    break;
  case CUDD_REORDER_WINDOW2:
    *method = BDD_REORDER_WINDOW2;
    break;
  case CUDD_REORDER_WINDOW4:
    *method = BDD_REORDER_WINDOW4;
    break;
  case CUDD_REORDER_WINDOW2_CONV:
    *method = BDD_REORDER_WINDOW2_CONV;
    break;
  case CUDD_REORDER_WINDOW3:
    *method = BDD_REORDER_WINDOW3;
    break;
  case CUDD_REORDER_WINDOW4_CONV:
    *method = BDD_REORDER_WINDOW4_CONV;
    break;
  case CUDD_REORDER_GROUP_SIFT:
    *method = BDD_REORDER_GROUP_SIFT;
    break;
  case CUDD_REORDER_GROUP_SIFT_CONV:
    *method = BDD_REORDER_GROUP_SIFT_CONV;
    break;
  case CUDD_REORDER_ANNEALING:
    *method = BDD_REORDER_ANNEALING;
    break;
  case CUDD_REORDER_GENETIC:
    *method = BDD_REORDER_GENETIC;
    break;
  case CUDD_REORDER_EXACT:
    *method = BDD_REORDER_EXACT;
    break;
  default:
    break;
  }
  return(dyn);

} /* end of bdd_reordering_status */


/**Function********************************************************************

  Synopsis    []

  SideEffects []

******************************************************************************/
int
bdd_reordering_zdd_status(bdd_manager *mgr, bdd_reorder_type_t *method)
{
  int dyn;
  dyn = Cudd_ReorderingStatusZdd((DdManager *)mgr, (Cudd_ReorderingType  *)method);
  switch (*method) {
  case CUDD_REORDER_SIFT:
    *method = BDD_REORDER_SIFT;
    break;
  case CUDD_REORDER_WINDOW3_CONV:
    *method = BDD_REORDER_WINDOW3_CONV;
    break;
  case CUDD_REORDER_NONE:
    *method = BDD_REORDER_NONE;
    break;
  case CUDD_REORDER_SAME:
    *method = BDD_REORDER_SAME;
    break;
  case CUDD_REORDER_RANDOM:
    *method = BDD_REORDER_RANDOM;
    break;
  case CUDD_REORDER_RANDOM_PIVOT:
    *method = BDD_REORDER_RANDOM_PIVOT;
    break;
  case CUDD_REORDER_SIFT_CONVERGE:
    *method = BDD_REORDER_SIFT_CONVERGE;
    break;
  case CUDD_REORDER_SYMM_SIFT:
    *method = BDD_REORDER_SYMM_SIFT;
    break;
  case CUDD_REORDER_SYMM_SIFT_CONV:
    *method = BDD_REORDER_SYMM_SIFT_CONV;
    break;
  case CUDD_REORDER_WINDOW2:
    *method = BDD_REORDER_WINDOW2;
    break;
  case CUDD_REORDER_WINDOW4:
    *method = BDD_REORDER_WINDOW4;
    break;
  case CUDD_REORDER_WINDOW2_CONV:
    *method = BDD_REORDER_WINDOW2_CONV;
    break;
  case CUDD_REORDER_WINDOW3:
    *method = BDD_REORDER_WINDOW3;
    break;
  case CUDD_REORDER_WINDOW4_CONV:
    *method = BDD_REORDER_WINDOW4_CONV;
    break;
  case CUDD_REORDER_GROUP_SIFT:
    *method = BDD_REORDER_GROUP_SIFT;
    break;
  case CUDD_REORDER_GROUP_SIFT_CONV:
    *method = BDD_REORDER_GROUP_SIFT_CONV;
    break;
  case CUDD_REORDER_ANNEALING:
    *method = BDD_REORDER_ANNEALING;
    break;
  case CUDD_REORDER_GENETIC:
    *method = BDD_REORDER_GENETIC;
    break;
  case CUDD_REORDER_EXACT:
    *method = BDD_REORDER_EXACT;
    break;
  default:
    break;
  }
  return(dyn);

} /* end of bdd_reordering_zdd_status */


/**Function********************************************************************

  Synopsis           [Converts a bdd to an add.]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_bdd_to_add(bdd_manager *mgr, bdd_node *fn)
{
  DdNode *result;
  result = Cudd_BddToAdd((DdManager *)mgr,(DdNode *)fn);
  return((bdd_node *)result);

} /* end of bdd_bdd_to_add */


/**Function********************************************************************

  Synopsis           [Permutes the variables in a given function using the permut array..]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_permute(
  bdd_manager *mgr,
  bdd_node *fn,
  int *permut)
{
  DdNode *result;
  result = Cudd_addPermute((DdManager *)mgr, (DdNode *)fn, permut);
  return(result);

} /* end of bdd_add_permute */


/**Function********************************************************************

  Synopsis           [Permutes the variables in a given function using the permut array..]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_bdd_permute(
  bdd_manager *mgr,
  bdd_node *fn,
  int *permut)
{
  DdNode *result;
  result = Cudd_bddPermute((DdManager *)mgr, (DdNode *)fn, permut);
  return(result);

} /* end of bdd_bdd_permute */


/**Function********************************************************************

  Synopsis           [References a bdd]

  SideEffects        []

******************************************************************************/
void
bdd_ref(bdd_node *fn)
{
  Cudd_Ref((DdNode *)fn);
  return;

} /* end of bdd_ref */


/**Function********************************************************************

  Synopsis [Decreases the reference count of node.If f dies,
  recursively decreases the reference counts of its children.  It is
  used to dispose of a DD that is no longer needed.]

  SideEffects []

******************************************************************************/
void
bdd_recursive_deref(bdd_manager *mgr, bdd_node *f)
{
  Cudd_RecursiveDeref((DdManager *)mgr, (DdNode *)f);

} /* end of bdd_recursive_deref */


/**Function********************************************************************

  Synopsis           [Existentially abstracts out the variables from the function]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_exist_abstract(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node *vars)
{
  DdNode *result;
  result = Cudd_addExistAbstract((DdManager *)mgr, (DdNode *)fn,
				 (DdNode *)vars);
  return(result);

} /* end of bdd_add_exist_abstract */


/**Function********************************************************************

  Synopsis           [Performs the apply operation on ADds]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_apply(
  bdd_manager *mgr,
  bdd_node *(*operation)(bdd_manager *, bdd_node **, bdd_node **),
  bdd_node *fn1,
  bdd_node *fn2)
{
  DdNode *result;
  result = Cudd_addApply((DdManager *)mgr,
			 (DdNode *(*)(DdManager *, DdNode **, DdNode **))
			 operation, (DdNode *)fn1, (DdNode *)fn2);
  return(result);

} /* end of bdd_add_apply */


/**Function********************************************************************

  Synopsis           [Performs the non-simple compose on ADds]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_nonsim_compose(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node **vector)
{
  DdNode *result;
  result = Cudd_addNonSimCompose((DdManager *)mgr, (DdNode *)fn,
				 (DdNode **)vector);
  return(result);

} /* end of bdd_add_nonsim_compose */


/**Function********************************************************************

  Synopsis           [Computes the residue ADD of n variables with respect to m]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_residue(
  bdd_manager *mgr,
  int n,
  int m,
  int options,
  int top)
{
  DdNode *result;
  result = Cudd_addResidue((DdManager *)mgr, n, m, options, top);
  return(result);

} /* end of bdd_add_residue */


/**Function********************************************************************

  Synopsis           [Performs the vector compose on ADds]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_vector_compose(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node **vector)
{
  DdNode *result;
  result = Cudd_addVectorCompose((DdManager *)mgr, (DdNode *)fn,
				 (DdNode **)vector);
  return(result);

} /* end of bdd_add_vector_compose */


/**Function********************************************************************

  Synopsis           [Performs the times (multiplication operation)  on Adds]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_times(
  bdd_manager *mgr,
  bdd_node **fn1,
  bdd_node **fn2)
{
  DdNode *result;
  result = Cudd_addTimes((DdManager *)mgr, (DdNode **)fn1, (DdNode **)fn2);
  return(result);

} /* end of bdd_add_times */


/**Function********************************************************************

  Synopsis           [Performs the zero reference count check on the manager.]

  SideEffects        []

******************************************************************************/
int
bdd_check_zero_ref(bdd_manager *mgr)
{
  int result;
  result = Cudd_CheckZeroRef((DdManager *)mgr);
  return(result);

} /* end of bdd_check_zero_ref */


/**Function********************************************************************

  Synopsis           [Disables dynamic reordering in the manager.]

  SideEffects        []

******************************************************************************/
void
bdd_dynamic_reordering_disable(bdd_manager *mgr)
{
  Cudd_AutodynDisable((DdManager *)mgr);
  return;

} /* end of bdd_dynamic_reordering_disable */


/**Function********************************************************************

  Synopsis           [Disables dynamic reordering for ZDD in the manager.]

  SideEffects        []

******************************************************************************/
void
bdd_dynamic_reordering_zdd_disable(bdd_manager *mgr)
{
  Cudd_AutodynDisableZdd((DdManager *)mgr);
  return;

} /* end of bdd_dynamic_reordering_zdd_disable */


/**Function********************************************************************

  Synopsis           [Performs the xnor (\equiv operation)  on Adds]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_xnor(bdd_manager *mgr, bdd_node **fn1, bdd_node **fn2)
{
  DdNode *result;
  result = Cudd_addXnor((DdManager *)mgr, (DdNode **)fn1, (DdNode **)fn2);
  return(result);

} /* end of bdd_add_xnor */


/**Function********************************************************************

  Synopsis           [Shuffles the variables in the manager in the given order.]

  SideEffects        []

******************************************************************************/
int
bdd_shuffle_heap(bdd_manager *mgr, int *permut)
{
  int result;
  result = Cudd_ShuffleHeap((DdManager *)mgr, permut);
  return(result);

} /* end of bdd_shuffle_heap */


/**Function********************************************************************

  Synopsis           [Performs compose operation on  ADds]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_compose(
  bdd_manager *mgr,
  bdd_node *fn1,
  bdd_node *fn2,
  int var)
{
  DdNode *result;
  result = Cudd_addCompose((DdManager *)mgr, (DdNode *)fn1,
			   (DdNode *)fn2, var);
  return(result);

} /* end of bdd_add_compose */


/**Function********************************************************************

  Synopsis           [Gets the ith add variable in the manager ]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_ith_var(bdd_manager *mgr, int i)
{
  DdNode *result;
  result = Cudd_addIthVar((DdManager *)mgr, i);
  return(result);

} /* end of bdd_add_ith_var */


/**Function********************************************************************

  Synopsis           [Gets the level of the ith variable in the manager ]

  SideEffects        []

******************************************************************************/
int
bdd_get_level_from_id(bdd_manager *mgr, int id)
{
  int level;
  level = Cudd_ReadPerm((DdManager *)mgr, id);
  return(level);

} /* end of bdd_get_level_from_id */


/**Function********************************************************************

  Synopsis [Existentially abstracts out the variables from the function.
  Here the fn is assumed to be a BDD function.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_exist_abstract(bdd_manager *mgr, bdd_node *fn, bdd_node *cube)
{
  DdNode *result;
  result = Cudd_bddExistAbstract((DdManager *)mgr, (DdNode *)fn,
				 (DdNode *)cube);
  return(result);

} /* end of bdd_bdd_exist_abstract */


/**Function********************************************************************

  Synopsis [Compares two ADDs for equality within tolerance. pr is verbosity
  level.]

  SideEffects []

******************************************************************************/
int
bdd_equal_sup_norm(
  bdd_manager *mgr,
  bdd_node *fn,
  bdd_node *gn,
  BDD_VALUE_TYPE tolerance,
  int pr)
{
  int result;
  result = Cudd_EqualSupNorm((DdManager *)mgr, (DdNode *)fn,
			     (DdNode *)gn, (CUDD_VALUE_TYPE)tolerance, pr);
  return(result);

} /* end of bdd_equal_sup_norm */


/**Function********************************************************************

  Synopsis [Reads constant logic zero bdd_node.]


  SideEffects [bdd_zero]

******************************************************************************/
bdd_node *
bdd_read_logic_zero(bdd_manager *mgr)
{
  DdNode *result;
  result = Cudd_ReadLogicZero((DdManager *)mgr);

  return(result);

} /* end of bdd_read_logic_zero */


/**Function********************************************************************

  Synopsis [Get the ith bdd node in the manager.]


  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_ith_var(bdd_manager *mgr, int i)
{
  DdNode *result;
  result = Cudd_bddIthVar((DdManager *)mgr, i);

  return(result);

} /* end of bdd_bdd_ith_var */


/**Function********************************************************************

  Synopsis           [Performs the divide operation on ADDs]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_divide(bdd_manager *mgr, bdd_node **fn1, bdd_node **fn2)
{
  DdNode *result;
  result = Cudd_addDivide((DdManager *)mgr, (DdNode **)fn1, (DdNode **)fn2);

  return(result);

} /* end of bdd_add_divide */


/**Function********************************************************************

  Synopsis [Performs the constrain operation.]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_bdd_constrain(bdd_manager *mgr, bdd_node *f, bdd_node *c)
{
  DdNode *result;
  result = Cudd_bddConstrain((DdManager *)mgr, (DdNode *)f, (DdNode *)c);

  return(result);

} /* end of bdd_bdd_constrain */


/**Function********************************************************************

  Synopsis [Performs the restrict operation.]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_bdd_restrict(bdd_manager *mgr, bdd_node *f, bdd_node *c)
{
  DdNode *result;
  result = Cudd_bddRestrict((DdManager *)mgr, (DdNode *)f, (DdNode *)c);

  return(result);

} /* end of bdd_bdd_restrict */


/**Function********************************************************************

  Synopsis [Computes the hamming distance ADD between two sets of variables.]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_hamming(
  bdd_manager *mgr,
  bdd_node **xVars,
  bdd_node **yVars,
  int nVars)
{
  DdNode *result;
  result = Cudd_addHamming((DdManager *)mgr, (DdNode **)xVars,
			   (DdNode **)yVars, nVars);

  return(result);

} /* end of bdd_add_hamming */


/**Function********************************************************************

  Synopsis [Performs the ITE operation for ADDs.]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_ite(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g,
  bdd_node *h)
{
  DdNode *result;
  result = Cudd_addIte((DdManager *)mgr, (DdNode *)f, (DdNode *)g,
		       (DdNode *)h);

  return(result);

} /* end of bdd_add_ite */


/**Function********************************************************************

  Synopsis [Finds the maximum discriminant of f. Returns a pointer to a
  constant ADD.]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_find_max(bdd_manager *mgr, bdd_node *f)
{
  DdNode *result;
  result = Cudd_addFindMax((DdManager *)mgr, (DdNode *)f);

  return(result);

} /* end of bdd_add_find_max */


/**Function********************************************************************

  Synopsis [Picks one on-set cube randomly from the given DD. The cube is
  written into an array of characters. The array must have at least as many
  entries as there are variables. Returns 1 if successful; 0 otherwise.]

  SideEffects []

******************************************************************************/
int
bdd_bdd_pick_one_cube(bdd_manager *mgr, bdd_node *node, char *string)
{
  return(Cudd_bddPickOneCube((DdManager *)mgr, (DdNode *)node, string));

} /* end of bdd_bdd_pick_one_cube */


/**Function********************************************************************

  Synopsis [Swap two sets of variables in ADD f]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_add_swap_variables(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **x,
  bdd_node **y,
  int n)
{
  DdNode *result;
  result = Cudd_addSwapVariables((DdManager *)mgr, (DdNode *)f,
				 (DdNode **)x, (DdNode **)y, n);

  return(result);

} /* end of bdd_add_swap_variables */


/**Function********************************************************************

  Synopsis [Computes the disjunction of two BDDs f and g.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_or(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = Cudd_bddOr((DdManager *)mgr, (DdNode *)f, (DdNode *)g);

  return(result);

} /* end of bdd_bdd_or */


/**Function********************************************************************

  Synopsis [Computes the cube of an array of BDD variables.If
  non-null, the phase argument indicates which literal of each
  variable should appear in the cube. If phase\[i\] is nonzero, then
  the positive literal is used. If phase is NULL, the cube is positive
  unate.  Returns a pointer to the result if successful; NULL
  otherwise.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_compute_cube(
  bdd_manager *mgr,
  bdd_node **vars,
  int *phase,
  int n)
{
  DdNode *result;
  result = Cudd_bddComputeCube((DdManager *)mgr, (DdNode **)vars,
			       phase, n);

  return(result);

} /* end of bdd_bdd_compute_cube */


/**Function********************************************************************

  Synopsis    [Builds a cube of BDD variables from an array of indices.]

  Description [Builds a cube of BDD variables from an array of indices.
  Returns a pointer to the result if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [bdd_bdd_compute_cube]

******************************************************************************/
bdd_node *
bdd_indices_to_cube(bdd_manager *mgr, int *idArray, int n)
{
  DdNode *result;
  result = Cudd_IndicesToCube((DdManager *)mgr, idArray, n);

  return(result);

} /* end of bdd_indices_to_cube */


/**Function********************************************************************

  Synopsis [Computes the conjunction of two BDDs f and g.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_and(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = Cudd_bddAnd((DdManager *)mgr, (DdNode *)f, (DdNode *)g);

  return(result);

} /* end of bdd_bdd_and */


/**Function********************************************************************

  Synopsis [Multiply two matrices represented by A and B. A is assumed to
  depend on x (rows) and z (columns). B is assumed to depend on z (rows)
  and y (columns). The product depends on x and y. Only z needs to be
  explicitly identified.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_matrix_multiply(
  bdd_manager *mgr,
  bdd_node *A,
  bdd_node *B,
  bdd_node **z,
  int nz)
{
  DdNode *result;
  result = Cudd_addMatrixMultiply((DdManager *)mgr, (DdNode *)A,
				  (DdNode *)B, (DdNode **)z, nz);

  return(result);

} /* end of bdd_add_matrix_multiply */


/**Function********************************************************************

  Synopsis [Computes the cube of an array of ADD variables.  If
  non-null, the phase argument indicates which literal of each
  variable should appear in the cube. If phase\[i\] is nonzero, then the
  positive literal is used. If phase is NULL, the cube is positive unate.
  Returns a pointer to the result if successful; NULL otherwise.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_compute_cube(
  bdd_manager *mgr,
  bdd_node **vars,
  int *phase,
  int n)
{
  DdNode *result;
  result = Cudd_addComputeCube((DdManager *)mgr, (DdNode **)vars, phase, n);

  return(result);

} /* end of bdd_add_compute_cube */


/**Function********************************************************************

  Synopsis [Returns the ADD for constant c.]

  Description [Returns the ADD for constant c if successful. NULL otherwise.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_const(bdd_manager *mgr, BDD_VALUE_TYPE c)
{
  DdNode *result;
  result = Cudd_addConst((DdManager *)mgr, (CUDD_VALUE_TYPE)c);

  return(result);

} /* end of bdd_add_const */


/**Function********************************************************************

  Synopsis [Swaps two sets of variables of the same size (x and y) in
  the BDD f.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_swap_variables(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **x,
  bdd_node **y,
  int n)
{
  DdNode *result;
  result = Cudd_bddSwapVariables((DdManager *)mgr, (DdNode *)f,
				 (DdNode **)x, (DdNode **)y, n);

  return(result);

} /* end of bdd_bdd_swap_variables */


/**Function********************************************************************

  Synopsis [Counts the number of minters in the on set of f which depends on
  atmost n variables.]

  SideEffects []

******************************************************************************/
double
bdd_count_minterm(bdd_manager *mgr, bdd_node *f, int n)
{
  double result;
  result = Cudd_CountMinterm((DdManager *)mgr, (DdNode *)f, n);

  return(result);

} /* end of bdd_count_minterm */


/**Function********************************************************************

  Synopsis [Converts an ADD to a BDD by replacing all
  discriminants greater than or equal to value with 1, and all other
  discriminants with 0. Returns a pointer to the resulting BDD if
  successful; NULL otherwise.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_bdd_threshold(
  bdd_manager *mgr,
  bdd_node *f,
  BDD_VALUE_TYPE value)
{
  DdNode *result;
  result = Cudd_addBddThreshold((DdManager *) mgr, (DdNode *) f,
				(CUDD_VALUE_TYPE)value);

  return(result);

} /* end of bdd_add_bdd_threshold */


/**Function********************************************************************

  Synopsis [Converts an ADD to a BDD by replacing all discriminants strictly
  greater than value with 1, and all other discriminants with 0. Returns a
  pointer to the resulting BDD if successful; NULL otherwise.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_bdd_strict_threshold(
  bdd_manager *mgr,
  bdd_node *f,
  BDD_VALUE_TYPE value)
{
  DdNode *result;
  result = Cudd_addBddStrictThreshold((DdManager *) mgr, (DdNode *) f,
				      (CUDD_VALUE_TYPE)value);

  return(result);

} /* end of bdd_add_bdd_strict_threshold */


/**Function********************************************************************

  Synopsis [Reads the epsilon parameter of the manager.]

  SideEffects []

******************************************************************************/
BDD_VALUE_TYPE
bdd_read_epsilon(bdd_manager *mgr)
{
  return((DdManager *)mgr)->epsilon;

} /* end of bdd_read_epsilon */


/**Function********************************************************************

  Synopsis [Reads the constant 1 of the manager.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_read_one(bdd_manager *mgr)
{
  return(DD_ONE((DdManager *)mgr));

} /* end of bdd_read_one */


/**Function********************************************************************

  Synopsis [Pick a random minterm from the onset of f.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_pick_one_minterm(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **vars,
  int n)
{
  DdNode *result;
  result = Cudd_bddPickOneMinterm((DdManager *)mgr, (DdNode *)f,
				  (DdNode **)vars, n);

  return(result);

} /* end of bdd_bdd_pick_one_minterm */


/**Function********************************************************************

  Synopsis [Pick a random minterm from the onset of f.]

  SideEffects []

******************************************************************************/
bdd_t *
bdd_pick_one_minterm(bdd_t *f, array_t *varsArray /* of bdd_t * */)
{
  DdNode **vars, *minterm;
  int i, n;

  n = array_n(varsArray);
  vars = ALLOC(DdNode *, n);
  if (vars == NIL(DdNode *)) return NIL(bdd_t);
  for (i = 0; i < n; i++) {
    bdd_t *var = array_fetch(bdd_t *, varsArray, i);
    assert(f->mgr == var->mgr);
    vars[i] = var->node;
  }
  minterm = Cudd_bddPickOneMinterm(f->mgr, f->node, vars, n);
  cuddRef(minterm);
  FREE(vars);
  if (minterm == NIL(DdNode)) return NIL(bdd_t);
  return bdd_construct_bdd_t(f->mgr,minterm);

} /* end of bdd_pick_one_minterm */


/**Function********************************************************************

  Synopsis [Pick arbitrary number of minterms evenly distributed from the
  onset of f.]

  SideEffects []

******************************************************************************/
array_t *
bdd_bdd_pick_arbitrary_minterms(
  bdd_t *f,
  array_t *varsArray,
  int n,
  int k)
{
  int i;
  DdNode **minterms, **vars;
  bdd_t *var;
  array_t *resultArray;

  vars = ALLOC(DdNode *, n);
  if (vars == NULL)
    return((array_t *)NULL);
  for (i = 0; i < n; i++) {
    var = array_fetch(bdd_t *, varsArray, i);
    vars[i] = var->node;
  }

  minterms = (DdNode **)Cudd_bddPickArbitraryMinterms((DdManager *)f->mgr,
						      (DdNode *)f->node, (DdNode **)vars, n, k);

  resultArray = array_alloc(bdd_t *, k);
  for (i = 0; i < k; i++) {
    cuddRef(minterms[i]);
    array_insert(bdd_t *, resultArray, i,
		 bdd_construct_bdd_t(f->mgr,minterms[i]));
  }

  FREE(vars);
  FREE(minterms);
  return(resultArray);

} /* end of bdd_bdd_pick_arbitrary_minterms */


/**Function********************************************************************

  Synopsis    [Extracts a subset from a BDD with mask variables.]

  Description [Extracts a subset from a BDD in the following procedure.
  1. Compute the weight for each mask variable by counting the number of
     minterms for both positive and negative cofactors of the BDD with
     respect to each mask variable. (weight = #positive - #negative)
  2. Find a representative cube of the BDD by using the weight. From the
     top variable of the BDD, for each variable, if the weight is greater
     than 0.0, choose THEN branch, othereise ELSE branch, until meeting
     the constant 1.
  3. Quantify out the variables not in maskVars from the representative
     cube and if a variable in maskVars is don't care, replace the
     variable with a constant(1 or 0) depending on the weight.
  4. Make a subset of the BDD by multiplying with the modified cube.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
bdd_t *
bdd_subset_with_mask_vars(bdd_t *f, array_t *varsArray, array_t *maskVarsArray)
{
  int i;
  DdNode *subset, **vars, **maskVars;
  bdd_t *var;
  int	n = array_n(varsArray);
  int	m = array_n(maskVarsArray);

  vars = ALLOC(DdNode *, n);
  if (vars == NULL)
    return((bdd_t *)NULL);
  for (i = 0; i < n; i++) {
    var = array_fetch(bdd_t *, varsArray, i);
    vars[i] = var->node;
  }

  maskVars = ALLOC(DdNode *, m);
  if (maskVars == NULL) {
    FREE(vars);
    return((bdd_t *)NULL);
  }
  for (i = 0; i < m; i++) {
    var = array_fetch(bdd_t *, maskVarsArray, i);
    maskVars[i] = var->node;
  }

  subset = (DdNode *)Cudd_SubsetWithMaskVars((DdManager *)f->mgr,
					     (DdNode *)f->node, (DdNode **)vars, n, (DdNode **)maskVars, m);
  if (subset == NULL) return((bdd_t *)NULL);

  cuddRef(subset);
  FREE(vars);
  FREE(maskVars);

  return(bdd_construct_bdd_t(f->mgr,subset));

} /* end of bdd_subset_with_mask_vars */


/**Function********************************************************************

  Synopsis [Read constant zero of the manager. This is different from the
  logical zero which is the complement of logical one.]

  SideEffects [bdd_zero]

******************************************************************************/
bdd_node *
bdd_read_zero(bdd_manager *mgr)
{
  return(DD_ZERO((DdManager *)mgr));

} /* bdd_read_zero */


/**Function********************************************************************

  Synopsis [Returns a new BDD variable.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_new_var(bdd_manager *mgr)
{
  DdNode *result;
  result = Cudd_bddNewVar((DdManager *)mgr);

  return(result);

} /* end of bdd_bdd_new_var */


/**Function********************************************************************

  Synopsis [Takes the AND of two BDDs and simultaneously abstracts the
  variables in cube.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_and_abstract(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node *g,
  bdd_node *cube)
{
  DdNode *result;
  result = Cudd_bddAndAbstract((DdManager *)mgr, (DdNode *)f,
			       (DdNode *)g, (DdNode *)cube);
  return(result);

} /* end of bdd_bdd_and_abstract */


/**Function********************************************************************

  Synopsis [Decreases the reference count of node.]

  SideEffects []

******************************************************************************/
void
bdd_deref(bdd_node *f)
{
  Cudd_Deref((DdNode *)f);

} /* end of bdd_deref */


/**Function********************************************************************

  Synopsis [Integer and floating point addition.Returns NULL if not
  a terminal case; f+g otherwise.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_plus(bdd_manager *mgr, bdd_node **fn1, bdd_node **fn2)
{
  DdNode *result;
  result = Cudd_addPlus((DdManager *)mgr, (DdNode **)fn1, (DdNode **)fn2);
  return(result);

} /* end of bdd_add_plus */


/**Function********************************************************************

  Synopsis [Returns the number of times reordering has occurred.]

  SideEffects []

******************************************************************************/
int
bdd_read_reorderings(bdd_manager *mgr)
{
  return(Cudd_ReadReorderings((DdManager *)mgr));

} /* end of bdd_read_reorderings */


/**Function********************************************************************

  Synopsis [Returns the threshold for the next dynamic reordering.]

  SideEffects []

******************************************************************************/
int
bdd_read_next_reordering(bdd_manager *mgr)
{
  return(Cudd_ReadNextReordering((DdManager *)mgr));

} /* end of bdd_read_next_reordering */


/**Function********************************************************************

  Synopsis [Sets the threshold for the next dynamic reordering.]

  SideEffects []

******************************************************************************/
void
bdd_set_next_reordering(bdd_manager *mgr, int next)
{
  Cudd_SetNextReordering((DdManager *)mgr, next);

} /* end of bdd_set_next_reordering */


/**Function********************************************************************

  Synopsis [Computes the exclusive-nor of f and g.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_xnor(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = Cudd_bddXnor((DdManager *)mgr, (DdNode *)f, (DdNode *)g);

  return(result);

} /* end of bdd_bdd_xnor */


/**Function********************************************************************

  Synopsis [Composes a BDD with a vector of BDDs.Given a vector of
  BDDs, creates a new BDD by substituting the BDDs for the variables
  of the BDD f.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_vector_compose(bdd_manager *mgr, bdd_node *f, bdd_node **vector)
{
  DdNode *result;
  result = Cudd_bddVectorCompose((DdManager *)mgr, (DdNode *)f,
				 (DdNode **)vector);
  return(result);

} /* end of bdd_bdd_vector_compose */


/**Function********************************************************************

  Synopsis [Extracts a BDD node from the bdd_t structure without making
  it regular.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_extract_node_as_is(bdd_t *fn)
{
  return((bdd_node *)fn->node);

} /* end of bdd_extract_node_as_is */


/**Function********************************************************************

  Synopsis [Returns a zdd node with index i and g and h as its children.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_zdd_get_node(
  bdd_manager *mgr,
  int id,
  bdd_node *g,
  bdd_node *h)
{
  DdNode *result;
  result = cuddZddGetNode((DdManager *)mgr, id, (DdNode *)g,
			  (DdNode *)h);

  return(result);

} /*end of bdd_zdd_get_node */


/**Function********************************************************************

  Synopsis [Computes the product of two cover represented by ZDDs. The covers
  on which bdd_zdd_product operates use two ZDD variables for each
  function variable (one ZDD variable for each literal of the variable). Those
  two ZDD variables should be adjacent in the order.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_zdd_product(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = Cudd_zddProduct((DdManager *)mgr, (DdNode *)f, (DdNode *)g);
  return(result);

} /* end of bdd_zdd_product */


/**Function********************************************************************

  Synopsis [Computes the product of two cover represented by ZDDs. The covers
  on which bdd_zdd_product_recur operates use two ZDD variables for each
  function variable (one ZDD variable for each literal of the variable). Those
  two ZDD variables should be adjacent in the order.  This is a recursive
  procedure. It returns the ZDD of the product if successful. Reference count
  of the result is not incremented. NULL is returned if re-ordering takes place
  or if memory is exhausted.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_zdd_product_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = cuddZddProduct((DdManager *)mgr, (DdNode *)f, (DdNode *)g);
  return(result);

} /* end of bdd_zdd_product_recur */


/**Function********************************************************************

  Synopsis [Computes the union of two ZDDs.]

  Description [Computes the union of two ZDDs. Returns a pointer to the
  result if successful; NULL otherwise.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_zdd_union(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = Cudd_zddUnion((DdManager *)mgr, (DdNode *)f, (DdNode *)g);
  return(result);

} /* end of bdd_zdd_union */


/**Function********************************************************************

  Synopsis [Computes the union of two ZDDs.]

  Description [Computes the union of two ZDDs. Returns a pointer to the
  result if successful; NULL otherwise.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_zdd_union_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = cuddZddUnion((DdManager *)mgr, (DdNode *)f, (DdNode *)g);
  return(result);

} /* end of bdd_zdd_union_recur */


/**Function********************************************************************

  Synopsis [Applies weak division to two ZDDs representing two covers. The
  result of weak division depends on the variable order. The covers on which
  bdd_zdd_weak_div operates use two ZDD variables for each function
  variable (one ZDD variable for each literal of the variable). Those two ZDD
  variables should be adjacent in the order.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_zdd_weak_div(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = Cudd_zddWeakDiv((DdManager *)mgr, (DdNode *)f, (DdNode *)g);
  return(result);

} /* end of bdd_zdd_weak_div */


/**Function********************************************************************

  Synopsis [Applies weak division to two ZDDs representing two covers. The
  result of weak division depends on the variable order. The covers on which
  bdd_zdd_weak_div_recur operates use two ZDD variables for each function
  variable (one ZDD variable for each literal of the variable). Those two ZDD
  variables should be adjacent in the order. This is a recursive procedure. It
  returns a pointer to the result if successful; Reference count of the result
  is not incremented. NULL is returned if re-ordering takes place or if memory
  is exhausted.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_zdd_weak_div_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = cuddZddWeakDiv((DdManager *)mgr, (DdNode *)f, (DdNode *)g);

  return(result);

} /* end of bdd_zdd_weak_div_recur */


/**Function********************************************************************

  Synopsis [Computes an irredundant sum of products (ISOP) in ZDD form from
  BDDs. This is a recursive procedure. Returns the pointer to the ZDD on
  success. Reference count of the result is not incremented. NULL in the case
  of re-ordering or if memory is exhausted.]

  SideEffects [zdd_I holds the pointer to the ZDD for the ISOP on successful
  return.]

******************************************************************************/
bdd_node *
bdd_zdd_isop_recur(
  bdd_manager *mgr,
  bdd_node *L,
  bdd_node *U,
  bdd_node **zdd_I)
{
  DdNode *result;
  result = cuddZddIsop((DdManager *)mgr, (DdNode *)L, (DdNode *)U,
		       (DdNode **)zdd_I);

  return(result);

} /* end of bdd_zdd_isop_recur */


/**Function********************************************************************

  Synopsis [Computes an irredundant sum of products (ISOP) in ZDD form from
  BDDs. This is an interface to an external function.]

  SideEffects [zdd_I holds the pointer to the ZDD for the ISOP on successful
  return.]

  SeeAlso [bdd_zdd_isop_recur]

******************************************************************************/
bdd_node *
bdd_zdd_isop(
  bdd_manager *mgr,
  bdd_node *L,
  bdd_node *U,
  bdd_node **zdd_I)
{
  DdNode *result;
  result = Cudd_zddIsop((DdManager *)mgr, (DdNode *)L, (DdNode *)U,
			(DdNode **)zdd_I);

  return(result);

} /* end of bdd_zdd_isop */


/**Function********************************************************************

  Synopsis    [Computes the three-way decomposition of f w.r.t. v.]

  Description [Computes the three-way decomposition of function f (represented
  by a ZDD) w.r.t respect to variable v. Returns 1 on failure, 0 on
  success. Reference counts of f1, f0 and fd are not incremented. ]

  SideEffects [The results are returned in f1, f0, and fd. They are NULL in
  case of failure.]

******************************************************************************/
int
bdd_zdd_get_cofactors3(
  bdd_manager *mgr,
  bdd_node *f,
  int v,
  bdd_node **f1,
  bdd_node **f0,
  bdd_node **fd)
{
  int result;
  result = cuddZddGetCofactors3((DdManager *)mgr, (DdNode *)f, v,
				(DdNode **)f1, (DdNode **)f0,
				(DdNode **)fd);

  return(result);

} /* end of bdd_zdd_get_cofactors3 */


/**Function********************************************************************

  Synopsis    [Recursive procedure to compute AND of two bdd_nodes.]

  Description [Recursive procedure to compute AND of two bdd_nodes.  Returns
  the pointer to the BDD on success. The reference count of the result is not
  incremented. NULL is returned in case of reordering or if memory is
  exhausted.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_and_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = cuddBddAndRecur((DdManager *)mgr, (DdNode *)f,
			   (DdNode *)g);
  return(result);

} /* end of bdd_bdd_and_recur */


/**Function********************************************************************

  Synopsis    [Returns a bdd_node whose index is v and g and h as its
  children.]

  Description [Returns a bdd_node whose index is v and g and h as its
  children. Returns the bdd_node after success. The reference count of the
  returned BDD is not incremented. Returns NULL in case of reordering or if
  memory is exhausted.]

  SideEffects [none]

******************************************************************************/
bdd_node *
bdd_unique_inter(
  bdd_manager *mgr,
  int v,
  bdd_node *f,
  bdd_node *g)
{
  DdNode *result;
  result = cuddUniqueInter((DdManager *)mgr, v, (DdNode *)f,
			   (DdNode *)g);
  return(result);

} /* end of bdd_unique_inter */


/**Function********************************************************************

  Synopsis    [Returns a bdd_node whose index is v and f and g as its
  children.]

  Description [Returns a bdd_node whose index is v and f and g as its
  children. Returns the bdd_node after success. The reference count of the
  returned BDD is not incremented. Returns NULL in case of reordering or if
  memory is exhausted.]

  SideEffects [none]

******************************************************************************/
bdd_node *
bdd_unique_inter_ivo(
  bdd_manager *mgr,
  int v,
  bdd_node *f,
  bdd_node *g)
{
  DdNode *result;
  DdNode *t;

  t = cuddUniqueInter((DdManager *)mgr, v, (DdNode *)bdd_read_one(mgr),
		      (DdNode *)bdd_not_bdd_node(bdd_read_one(mgr)));
  if (t == NULL)
    return(NULL);
  Cudd_Ref(t);
  result = cuddBddIteRecur((DdManager *)mgr, t, (DdNode *)f, (DdNode *)g);
  Cudd_RecursiveDeref((DdManager *)mgr,(DdNode *)t);
  return(result);

} /* end of bdd_unique_inter_ivo */


/**Function********************************************************************

  Synopsis    [Computes the set difference of two ZDDs.]

  Description [Computes the set difference of two ZDDs. Returns a pointer to
  the result if successful. The reference count of the result is not
  incremented. NULL is returned in case of re-ordering of if memory is
  exhausted.]

  SideEffects [none]

******************************************************************************/
bdd_node *
bdd_zdd_diff(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = Cudd_zddDiff((DdManager *)mgr, (DdNode *)f, (DdNode *)g);
  return(result);

} /* end of bdd_zdd_diff */


/**Function********************************************************************

  Synopsis    [Computes the set difference of two ZDDs.]

  Description [Computes the set difference of two ZDDs. Returns a pointer to
  the result if successful. The reference count of the result is not
  incremented. NULL is returned in case of re-ordering of if memory is
  exhausted.]

  SideEffects [none]

******************************************************************************/
bdd_node *
bdd_zdd_diff_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = cuddZddDiff((DdManager *)mgr, (DdNode *)f, (DdNode *)g);
  return(result);

} /* end of bdd_zdd_diff_recur */


/**Function********************************************************************

  Synopsis    [Returns the number of ZDD variables.]

  Description [Returns the number of ZDD variables.]

  SideEffects [none]

******************************************************************************/
int
bdd_num_zdd_vars(bdd_manager *mgr)
{
  return(((DdManager *)mgr)->sizeZ);

} /* end of bdd_num_zdd_vars */


/**Function********************************************************************

  Synopsis    [Makes the bdd_node a regular one.]

  Description [Makes the bdd_node a retular one.]

  SideEffects [none]

******************************************************************************/
bdd_node *
bdd_regular(bdd_node *f)
{
  return(Cudd_Regular((DdNode *)f));

} /* end of bdd_regular */


/**Function********************************************************************

  Synopsis    [Returns 1 if the bdd_node is a constant; 0 otherwise.]

  Description [Returns 1 if the bdd_node is a constant; 0 otherwise.]

  SideEffects [none]

******************************************************************************/
int
bdd_is_constant(bdd_node *f)
{
  return(Cudd_IsConstant((DdNode *)f));

} /* end of bdd_is_constant */


/**Function********************************************************************

  Synopsis    [Returns 1 if the bdd_node is complemented. 0 otherwise.]

  Description [Returns 1 if the bdd_node is complemented. 0 otherwise.]]

  SideEffects [none]

******************************************************************************/
int
bdd_is_complement(bdd_node *f)
{
  return(Cudd_IsComplement((DdNode *)f));

} /* end of bdd_is_complement */


/**Function********************************************************************

  Synopsis    [Returns the then child of f.]

  Description [Returns the then child of f. This is different from
  bdd_then.]

  SideEffects [none]

  SeeAlso [bdd_then]

******************************************************************************/
bdd_node *
bdd_bdd_T(bdd_node *f)
{
  return(Cudd_T((DdNode *)f));

} /* end of bdd_bdd_T */


/**Function********************************************************************

  Synopsis    [Returns the else child of f.]

  Description [Returns the else child of f. This is different from
  bdd_else.]

  SideEffects []

  SeeAlso [bdd_else]
******************************************************************************/
bdd_node *
bdd_bdd_E(bdd_node *f)
{
  return(Cudd_E((DdNode *)f));

} /* end of bdd_bdd_E */


/**Function********************************************************************

  Synopsis    [Returns the complement of a bdd_node.]

  Description [Returns the complement of a bdd_node.]

  SideEffects []

  SeeAlso [bdd_not]
******************************************************************************/
bdd_node *
bdd_not_bdd_node(bdd_node *f)
{
  return(Cudd_Not((DdNode *)f));

} /* end of bdd_not_bdd_node */


/**Function********************************************************************

  Synopsis    [Recursively derefs a ZDD.]

  Description [Recursively derefs a ZDD.]

  SideEffects [bdd_recursive_deref]


******************************************************************************/
void
bdd_recursive_deref_zdd(bdd_manager *mgr, bdd_node *f)
{
  Cudd_RecursiveDerefZdd((DdManager *)mgr, (DdNode *)f);

} /* end of bdd_recursive_deref_zdd */


/**Function********************************************************************

  Synopsis    [Count the number of mintems of a ZDD.]

  Description [Count the number of mintems of a ZDD.]

  SideEffects []

******************************************************************************/
int
bdd_zdd_count(bdd_manager *mgr, bdd_node *f)
{
  return(Cudd_zddCount((DdManager *)mgr, (DdNode *)f));

} /* end of bdd_zdd_count */


/**Function********************************************************************

  Synopsis    [Returns the level of a of a bdd_node with index, index.]

  Description [Returns the level of a of a bdd_node with index, index.]

  SideEffects []

******************************************************************************/
int
bdd_read_zdd_level(bdd_manager *mgr, int index)
{
  return(Cudd_ReadPermZdd((DdManager *)mgr, index));

} /* end of bdd_read_zdd_level  */


/**Function********************************************************************

  Synopsis [Creates multiplicity number of ZDD vars for each BDD var.]

  Description [Creates one or more ZDD variables for each BDD variable.  If
  some ZDD variables already exist, only the missing variables are created.
  Parameter multiplicity allows the caller to control how many variables are
  created for each BDD variable in existence. For instance, if ZDDs are used to
  represent covers, two ZDD variables are required for each BDD variable.  The
  order of the BDD variables is transferred to the ZDD variables. If a variable
  group tree exists for the BDD variables, a corresponding ZDD variable group
  tree is created by expanding the BDD variable tree. In any case, the ZDD
  variables derived from the same BDD variable are merged in a ZDD variable
  group. If a ZDD variable group tree exists, it is freed. Returns 1 if
  successful; 0 otherwise.]

  SideEffects []

******************************************************************************/
int
bdd_zdd_vars_from_bdd_vars(bdd_manager *mgr, int multiplicity)
{
  return(Cudd_zddVarsFromBddVars((DdManager *)mgr, multiplicity));

} /* end of bdd_zdd_vars_from_bdd_vars */


/**Function********************************************************************

  Synopsis [Enables the alignment of ZDD vars with that of corresponding BDD
  vars.]

  Description [Enables the alignment of ZDD vars with that of corresponding BDD
  vars.]

  SideEffects []

******************************************************************************/
void
bdd_zdd_realign_enable(bdd_manager *mgr)
{
  Cudd_zddRealignEnable((DdManager *)mgr);

} /* end of bdd_zdd_realign_enable */


/**Function********************************************************************

  Synopsis [Disables the alignment of ZDD vars with that of corresponding BDD
  vars.]

  Description [Disables the alignment of ZDD vars with that of corresponding BDD
  vars.]

  SideEffects []

******************************************************************************/
void
bdd_zdd_realign_disable(bdd_manager *mgr)
{
  Cudd_zddRealignDisable((DdManager *)mgr);

} /* end of bdd_zdd_realign_disable */


/**Function********************************************************************

  Synopsis [Returns the value of the variable for the alignment of ZDD vars
  with that of corresponding BDD vars.]

  Description [Returns the value of the variable for the alignment of ZDD vars
  with that of corresponding BDD vars.]

  SideEffects []

******************************************************************************/
int
bdd_zdd_realignment_enabled(bdd_manager *mgr)
{
  return(Cudd_zddRealignmentEnabled((DdManager *)mgr));

} /* end of bdd_zdd_realignment_enabled */


/**Function********************************************************************

  Synopsis [Enables the alignment of BDD vars with that of corresponding ZDD
  vars.]

  Description [Enables the alignment of BDD vars with that of corresponding ZDD
  vars.]

  SideEffects []

******************************************************************************/
void
bdd_realign_enable(bdd_manager *mgr)
{
  Cudd_bddRealignEnable((DdManager *)mgr);

} /* end of bdd_realign_enable */


/**Function********************************************************************

  Synopsis [Disables the alignment of BDD vars with that of corresponding ZDD
  vars.]

  Description [Disables the alignment of BDD vars with that of corresponding ZDD
  vars.]

  SideEffects []

******************************************************************************/
void
bdd_realign_disable(bdd_manager *mgr)
{
  Cudd_bddRealignDisable((DdManager *)mgr);

} /* end of bdd_realign_disable */


/**Function********************************************************************

  Synopsis [Returns the value of the variable for the alignment of BDD vars
  with that of corresponding ZDD vars.]

  Description [Returns the value of the variable for the alignment of BDD vars
  with that of corresponding ZDD vars.]

  SideEffects []

******************************************************************************/
int
bdd_realignment_enabled(bdd_manager *mgr)
{
  return(Cudd_bddRealignmentEnabled((DdManager *)mgr));

} /* end of bdd_realignment_enabled */


/**Function********************************************************************

  Synopsis    [Returns the index of bdd_node f.]

  Description [Returns the index of bdd_node f.]

  SideEffects []

  SeeAlso     [bdd_top_var_id]

******************************************************************************/
int
bdd_node_read_index(bdd_node *f)
{
  return(Cudd_NodeReadIndex((DdNode *)f));

} /* end of bdd_node_read_index */


/**Function********************************************************************

  Synopsis    [Reads the next field of a DdNode.]

  Description [Reads the next field of a DdNode.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_read_next(bdd_node *f)
{
  return(((DdNode *)f)->next);

} /* end of bdd_read_next */


/**Function********************************************************************

  Synopsis [Sets the next field of a DdNode. This function should NOT be used
  by an external user. This is provided here as a patch.  This will not be a
  part of any further release.]

  Description [Sets the next field of a DdNode. This function should NOT be
  used by an external user. This is provided here as a patch.  This will not be
  a part of any further release.]

  SideEffects []

******************************************************************************/
void
bdd_set_next(bdd_node *f, bdd_node *g)
{
  ((DdNode *)f)->next = (DdNode *)g;

} /* end of bdd_set_next */


/**Function********************************************************************

  Synopsis [Read the reordered field of the manager.]

  Description [Read the reordered field of the manager.]

  SideEffects []

******************************************************************************/
int
bdd_read_reordered_field(bdd_manager *mgr)
{
  return(((DdManager *)mgr)->reordered);

} /* end of bdd_read_reordered_field */


/**Function********************************************************************

  Synopsis [Set the reordered field of the manager.This is NOT to be
  used by an external user. This function will not be a part of future
  release.]

  Description [Set the reordered field of the manager.This is NOT to be
  used by an external user. This function will not be a part of future
  release.]

  SideEffects []

******************************************************************************/
void
bdd_set_reordered_field(bdd_manager *mgr, int n)
{
  ((DdManager *)mgr)->reordered = n;

} /* end of bdd_set_reordered_field */


/**Function********************************************************************

  Synopsis [Implements the recursive call of bdd_add_apply.]

  Description [Implements the recursive call of bdd_add_apply. This should be
  used only in recursive procedures where the order of the variables needs to
  remain constant during the entire operation of the procedure. Returns a
  pointer to the result if successful. NULL is returned if reordering takes
  place or if memory is exhausted.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_apply_recur(
  bdd_manager *mgr,
  bdd_node *(*operation)(bdd_manager *, bdd_node **, bdd_node **),
  bdd_node *fn1,
  bdd_node *fn2)
{
  DdNode *result;
  result = cuddAddApplyRecur((DdManager *)mgr,
			     (DdNode *(*)(DdManager *, DdNode **, DdNode **))
			     operation, (DdNode *)fn1, (DdNode *)fn2);
  return(result);

} /* end of bdd_add_apply_recur */


/**Function********************************************************************

  Synopsis [Returns the value of the ADD node.]

  Description [Returns the value of the ADD node.]

  SideEffects []

******************************************************************************/
BDD_VALUE_TYPE
bdd_add_value(bdd_node *f)
{
  return(Cudd_V((DdNode *)f));

} /* end of bdd_add_value */


/**Function********************************************************************

  Synopsis [Prints minterms of the bdd.]

  Description [.]

  SideEffects []

******************************************************************************/
int
bdd_print_minterm(bdd_t *f)
{
  int result;
  result = Cudd_PrintMinterm((DdManager *)f->mgr, (DdNode *)f->node);
  return result;

} /* end of bdd_print_minterm */


/**Function********************************************************************

  Synopsis [Prints cover of the bdd.]

  Description [.]

  SideEffects []

******************************************************************************/
int
bdd_print_cover(bdd_t *f)
{
  int result;
  result = Cudd_bddPrintCover((DdManager *)f->mgr,
			      (DdNode *)f->node, (DdNode *)f->node);
  return result;

} /* end of bdd_print_cover */


/**Function********************************************************************

  Synopsis [Reads the plus inifinity field of the BDD manager.]

  Description [Reads the plus inifinity field of the BDD manager.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_read_plus_infinity(bdd_manager *mgr)
{
  DdNode *result;
  result = Cudd_ReadPlusInfinity((DdManager *)mgr);
  return (bdd_node *)result;

} /* end of bdd_read_plus_infinity */


/**Function********************************************************************

  Synopsis    [Selects pairs from R using a priority function.]

  Description [Selects pairs from a relation R(x,y) (given as a BDD)
  in such a way that a given x appears in one pair only. Uses a
  priority function to determine which y should be paired to a given
  x.  bdd_priority_select returns a pointer to the selected function
  if successful; NULL otherwise. Three of the arguments--x, y, and
  z--are vectors of BDD variables. The first two are the variables on
  which R depends. The third is a vector of auxiliary variables, used
  during the computation. This vector is optional. If a NULL value is
  passed instead, bdd_priority_select will create the working
  variables on the fly.  The sizes of x and y (and z if it is not
  NULL) should equal n.  The priority function Pi can be passed as a
  BDD, or can be built by Cudd_PrioritySelect. If NULL is passed
  instead of a bdd_node *, parameter Pifunc is used by
  Cudd_PrioritySelect to build a BDD for the priority
  function. (Pifunc is a pointer to a C function.) If Pi is not NULL,
  then Pifunc is ignored. Pifunc should have the same interface as the
  standard priority functions (e.g., bdd_dxygtdxz).]

  SideEffects [If called with z == NULL, will create new variables in
  the manager.]

  SeeAlso     [bdd_dxygtdxz bdd_xgty]

******************************************************************************/
bdd_node *
bdd_priority_select(
  bdd_manager *mgr,
  bdd_node *R,
  bdd_node **x,
  bdd_node **y,
  bdd_node **z,
  bdd_node *Pi,
  int n,
  bdd_node  *(*Pifunc)(bdd_manager *, int, bdd_node **, bdd_node **, bdd_node **))
{
  DdNode *result;
  result = Cudd_PrioritySelect((DdManager *)mgr,(DdNode *)R,
			       (DdNode **)x,(DdNode **)y,
			       (DdNode **)z,(DdNode *)Pi,
			       n,(DdNode *(*)(DdManager *, int, DdNode **,
					      DdNode **, DdNode **))Pifunc);
  return (bdd_node *)result;

} /* end of bdd_priority_select */


/**Function********************************************************************

  Synopsis [Set the background value of BDD manager.]

  Description [Set the background value of BDD manager.]

  SideEffects []

******************************************************************************/
void
bdd_set_background(bdd_manager *mgr, bdd_node *f)
{
  Cudd_SetBackground((DdManager *)mgr,(DdNode *)f);

} /* end of bdd_set_background */


/**Function********************************************************************

  Synopsis [Read the background value of BDD manager.]

  Description [Read the background value of BDD manager.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_read_background(bdd_manager *mgr)
{
  DdNode *result;
  result = Cudd_ReadBackground((DdManager *)mgr);
  return (bdd_node *)result;

} /* end of bdd_read_background */


/**Function********************************************************************

  Synopsis [Returns the cofactor of f w.r.t g]

  Description [Returns the cofactor of f w.r.t g]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_cofactor(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = Cudd_Cofactor((DdManager *)mgr,(DdNode *)f,
			 (DdNode *)g);
  return (bdd_node *)result;

} /* end of bdd_bdd_cofactor */


/**Function********************************************************************

  Synopsis [Returns the ITE of f,g and h]

  Description [Returns the ITE of f,g and h]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_ite(bdd_manager *mgr, bdd_node *f, bdd_node *g, bdd_node *h)
{
  DdNode *result;
  result = Cudd_bddIte((DdManager *)mgr,(DdNode *)f,
		       (DdNode *)g,(DdNode *)h);
  return (bdd_node *)result;

} /* end of bdd_bdd_ite */


/**Function********************************************************************

  Synopsis [Integer and floating point subtraction.Returns NULL if not a
  terminal case; f-g otherwise.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_minus(bdd_manager *mgr, bdd_node **fn1, bdd_node **fn2)
{
  DdNode *result;
  result = Cudd_addMinus((DdManager *)mgr, (DdNode **)fn1, (DdNode **)fn2);
  return((bdd_node *)result);

} /* end of bdd_add_plus */


/**Function********************************************************************

  Synopsis    [Generates a BDD for the function d(x,y) &gt; d(x,z).]

  Description [This function generates a BDD for the function d(x,y)
  &gt; d(x,z);
  x, y, and z are N-bit numbers, x\[0\] x\[1\] ... x\[N-1\],
  y\[0\] y\[1\] ...  y\[N-1\], and z\[0\] z\[1\] ...  z\[N-1\],
  with 0 the most significant bit.
  The distance d(x,y) is defined as:
	\sum_{i=0}^{N-1}(|x_i - y_i| \cdot 2^{N-i-1}).
  The BDD is built bottom-up.
  It has 7*N-3 internal nodes, if the variables are ordered as follows:
  x\[0\] y\[0\] z\[0\] x\[1\] y\[1\] z\[1\] ... x\[N-1\] y\[N-1\] z\[N-1\]. ]

  SideEffects [None]

  SeeAlso     [bdd_xgty]

******************************************************************************/
bdd_node *
bdd_dxygtdxz(
  bdd_manager *mgr,
  int N,
  bdd_node **x,
  bdd_node **y,
  bdd_node **z)
{
  DdNode *result;
  result = Cudd_Dxygtdxz((DdManager *)mgr,N,(DdNode **)x,
			 (DdNode **)y,(DdNode **)z);
  return((bdd_node *)result);

} /* end of bdd_dxygtdxz */


/**Function********************************************************************

  Synopsis [Universally abstracts out the variables from the function]

  SideEffects        []

******************************************************************************/
bdd_node *
bdd_bdd_univ_abstract(bdd_manager *mgr, bdd_node *fn, bdd_node *vars)
{
  DdNode *result;
  result = Cudd_bddUnivAbstract((DdManager *)mgr, (DdNode *)fn,
				(DdNode *)vars);
  return((bdd_node *)result);

} /* end of bdd_bdd_univ_abstract */


/**Function********************************************************************

  Synopsis    [Computes the compatible projection of R w.r.t. cube Y.]

  Description [Computes the compatible projection of relation R with
  respect to cube Y.]

  SideEffects [None]

******************************************************************************/
bdd_node *
bdd_bdd_cprojection(bdd_manager *mgr, bdd_node *R, bdd_node *Y)
{
  DdNode *result;
  result = Cudd_CProjection((DdManager *)mgr,(DdNode *)R,
			    (DdNode *)Y);
  return (bdd_node *)result;

} /* end of bdd_bdd_cprojection */


/**Function********************************************************************

  Synopsis    [Computes the correlation of f and g.]

  Description [Computes the correlation of f and g. If f == g, their
  correlation is 1. If f == g', their correlation is 0.  Returns the
  fraction of minterms in the ON-set of the EXNOR of f and g.]

  SideEffects [None]

******************************************************************************/
double
bdd_correlation(bdd_t *f, bdd_t *g)
{
  double result ;
  assert(f->mgr == g->mgr);
  result = Cudd_bddCorrelation(f->mgr, f->node, g->node);
  return (result);

} /* end of bdd_correlation */


/**Function********************************************************************

  Synopsis    [Computes 2 partitions of a function.]

  Description [Computes 2 partitions of a function. Method based on
  DAC98 - Ravi, Somenzi. Picks decomposition points and replaces one
  child in each conjunct with 1 (0). returns 2 conjuncts(disjuncts).]

  SideEffects []

******************************************************************************/
int
bdd_gen_decomp(bdd_t *f, bdd_partition_type_t partType, bdd_t ***conjArray)
{
  DdNode **ddArray = NULL;
  int i, num = 0;
  bdd_t *result;

  switch (partType) {
  case BDD_CONJUNCTS:
    num = Cudd_bddGenConjDecomp(f->mgr, f->node, &ddArray);
    break;
  case BDD_DISJUNCTS:
    num = Cudd_bddGenDisjDecomp(f->mgr, f->node, &ddArray);
    break;
  }
  if ((ddArray == NULL) || (!num)) {
    return 0;
  }

  *conjArray = ALLOC(bdd_t *, num);
  if ((*conjArray) == NULL) goto outOfMem;
  for (i = 0; i < num; i++) {
    result = ALLOC(bdd_t, 1);
    if (result == NULL) {
      FREE(*conjArray);
      goto outOfMem;
    }
    result->mgr = f->mgr;
    result->node = ddArray[i];
    result->free = FALSE;
    (*conjArray)[i] = result;
  }
  FREE(ddArray);
  return (num);

 outOfMem:
  for (i = 0; i < num; i++) {
    Cudd_RecursiveDeref((DdManager *)f->mgr,(DdNode *)ddArray[i]);
  }
  FREE(ddArray);
  return(0);

} /* end of bdd_gen_decomp */


/**Function********************************************************************

  Synopsis    [Computes 2 partitions of a function.]

  Description [Computes 2 partitions of a function. Method based on
  Cabodi 94. Picks a var and replaces one child in each conjunct with
  1 (0). returns 2 conjuncts(disjuncts).]

  SideEffects []

******************************************************************************/
int
bdd_var_decomp(bdd_t *f, bdd_partition_type_t partType, bdd_t ***conjArray)
{
  DdNode **ddArray = NULL;
  int i, num = 0;
  bdd_t *result;

  switch (partType) {
  case BDD_CONJUNCTS:
    num = Cudd_bddVarConjDecomp(f->mgr, f->node, &ddArray);
    break;
  case BDD_DISJUNCTS:
    num = Cudd_bddVarDisjDecomp(f->mgr, f->node, &ddArray);
    break;
  }
  if ((ddArray == NULL) || (!num)) {
    return 0;
  }

  *conjArray = ALLOC(bdd_t *, num);
  if ((*conjArray) == NULL) goto outOfMem;
  for (i = 0; i < num; i++) {
    result = ALLOC(bdd_t, 1);
    if (result == NULL) {
      FREE(*conjArray);
      goto outOfMem;
    }
    result->mgr = f->mgr;
    result->node = (ddArray)[i];
    result->free = FALSE;
    (*conjArray)[i] = result;
  }
  FREE(ddArray);
  return (num);

 outOfMem:
  for (i = 0; i < num; i++) {
    Cudd_RecursiveDeref((DdManager *)f->mgr,(DdNode *)ddArray[i]);
  }
  FREE(ddArray);
  return(0);

} /* end of bdd_var_decomp */


/**Function********************************************************************

  Synopsis    [Computes 2 partitions of a function.]

  Description [Computes 2 partitions of a function.  Picks a subset of
  a function and minimizes the rest of the function w.r.t. the subset.
  returns 2 conjuncts(disjuncts).]

  SideEffects []

******************************************************************************/
int
bdd_approx_decomp(bdd_t *f, bdd_partition_type_t partType, bdd_t ***conjArray)
{
  DdNode **ddArray = NULL;
  int i, num = 0;
  bdd_t *result;

  switch (partType) {
  case BDD_CONJUNCTS:
    num = Cudd_bddApproxConjDecomp(f->mgr, f->node, &ddArray);
    break;
  case BDD_DISJUNCTS:
    num = Cudd_bddApproxDisjDecomp(f->mgr, f->node, &ddArray);
    break;
  }
  if ((ddArray == NULL) || (!num)) {
    return 0;
  }

  *conjArray = ALLOC(bdd_t *, num);
  if ((*conjArray) == NULL) goto outOfMem;
  for (i = 0; i < num; i++) {
    result = ALLOC(bdd_t, 1);
    if (result == NULL) {
      FREE(*conjArray);
      goto outOfMem;
    }
    result->mgr = f->mgr;
    result->node = ddArray[i];
    result->free = FALSE;
    (*conjArray)[i] = result;
  }
  FREE(ddArray);
  return (num);

 outOfMem:
  for (i = 0; i < num; i++) {
    Cudd_RecursiveDeref((DdManager *)f->mgr,(DdNode *)ddArray[i]);
  }
  FREE(ddArray);
  return(0);

} /* end of bdd_approx_decomp */


/**Function********************************************************************

  Synopsis    [Computes 2 partitions of a function.]

  Description [Computes 2 partitions of a function.  Picks a subset of
  a function and minimizes the rest of the function w.r.t. the
  subset. Performs this iteratively.  returns 2 conjuncts(disjuncts).]

  SideEffects []

******************************************************************************/
int
bdd_iter_decomp(bdd_t *f, bdd_partition_type_t partType, bdd_t ***conjArray)
{
  DdNode **ddArray;
  int i, num = 0;
  bdd_t *result;

  switch (partType) {
  case BDD_CONJUNCTS:
    num = Cudd_bddIterConjDecomp(f->mgr, f->node, &ddArray);
    break;
  case BDD_DISJUNCTS:
    num = Cudd_bddIterDisjDecomp(f->mgr, f->node, &ddArray);
    break;
  }
  if ((ddArray == NULL) || (!num)) {
    return 0;
  }

  *conjArray = ALLOC(bdd_t *, num);
  if ((*conjArray) == NULL) goto outOfMem;
  for (i = 0; i < num; i++) {
    result = ALLOC(bdd_t, 1);
    if (result == NULL) {
      FREE(*conjArray);
      goto outOfMem;
    }
    result->mgr = f->mgr;
    result->node = ddArray[i];
    result->free = FALSE;
    (*conjArray)[i] = result;
  }
  FREE(ddArray);
  return (num);

 outOfMem:
  for (i = 0; i < num; i++) {
    Cudd_RecursiveDeref((DdManager *)f->mgr,(DdNode *)ddArray[i]);
  }
  FREE(ddArray);
  return(0);

} /* end of bdd_iter_decomp */


/**Function********************************************************************

  Synopsis    [Solves a Boolean equation.]

  SideEffects [The array passed in g is filled with the solutions.]

******************************************************************************/
bdd_t *
bdd_solve_eqn(
  bdd_t *f,
  array_t *g	/* of bdd_t *'s, initially empty */,
  array_t *unknowns	/* of bdd_t *'s */)
{
  int i;
  bdd_t *variable;
  DdNode *cube, *tmpDd, *result, **G;
  DdManager *mgr;
  int *yIndex = NIL(int);

  /* CUDD needs the y variables passed as a cube.
   * Therefore we must build that cube from the indices of variables
   * in the array before calling the procedure.
   */
  mgr = f->mgr;
  Cudd_Ref(cube = DD_ONE(mgr));
  for (i = 0; i < array_n(unknowns); i++) {
    variable = array_fetch(bdd_t *,unknowns,i);

    /* Make sure the variable belongs to the same manager. */
    assert(mgr == variable->mgr);

    tmpDd = Cudd_bddAnd(mgr,cube,variable->node);
    if (tmpDd == NULL) {
      Cudd_RecursiveDeref(mgr,cube);
      return(NULL);
    }
    cuddRef(tmpDd);
    Cudd_RecursiveDeref(mgr, cube);
    cube = tmpDd;
  }

  /* Allocate memory for the solutions. */
  G = ALLOC(DdNode *,array_n(unknowns));
  for (i = 0; i < array_n(unknowns); i++) {
    G[i] = NIL(DdNode);
  }

  /* Solve the equation */
  result = Cudd_SolveEqn(mgr,f->node,cube,G,&yIndex,array_n(unknowns));
  if (result == NULL) {
    Cudd_RecursiveDeref(mgr, cube);
    return(NULL);
  }
  cuddRef(result);
  /* Get rid of temporary results. */
  Cudd_RecursiveDeref(mgr, cube);

  /* Build the bdd_t structure for the solutions. */
  for (i = 0; i < array_n(unknowns); i++) {
    bdd_t *tmp = bdd_construct_bdd_t(mgr,G[i]);
    array_insert_last(bdd_t *, g, tmp);
  }

  return(bdd_construct_bdd_t(mgr,result));

} /* end of bdd_slve_eqn */


/**Function********************************************************************

  Synopsis    [Reports the number of nodes in the manager.]

  SideEffects []

******************************************************************************/
int
bdd_read_node_count(bdd_manager *mgr)
{
  return(Cudd_ReadNodeCount((DdManager *)mgr));

} /* end of bdd_read_node_count */


/**Function********************************************************************

  Synopsis    [Computes the fraction of minterms in the on-set of all the
  positive cofactors of a BDD, called signatures.]

  SideEffects [Creates an array of doubles as large as the number of
  variables in the manager + 1. The extra position is to the fraction
  of minterms in the on-set of the function.]

******************************************************************************/
double *
bdd_cof_minterm(bdd_t *f)
{
  double *signatures;
  signatures = Cudd_CofMinterm((DdManager *)f->mgr, (DdNode *)f->node);
  return (signatures);

} /* end of bdd_cof_minterm */


/**Function********************************************************************

  Synopsis [Estimates the size of the cofactor of f with respect to
  var in the specified phase. Return the number of nodes in the
  estimated size.]

  SideEffects []

******************************************************************************/
int
bdd_estimate_cofactor(bdd_t *f, bdd_t *var, int phase)
{
  return (Cudd_EstimateCofactor((DdManager *)f->mgr, (DdNode *)f->node,
				(int)bdd_top_var_id(var), phase));

} /* end of bdd_estimate_cofactor */


/**Function********************************************************************

  Synopsis [Tests if the varid is unate in f in the specified
  phase. If yes, return 1, else 0.]

  SideEffects []

******************************************************************************/
int
bdd_test_unate(bdd_t *f, int varId, int phase)
{
  DdNode *result;
  DdNode *one = DD_ONE((DdManager *)f->mgr);

  if (phase) {
    result = Cudd_Increasing((DdManager *)f->mgr, (DdNode *)f->node, varId);
  } else {
    result = Cudd_Decreasing((DdManager *)f->mgr, (DdNode *)f->node, varId);
  }

  if (result == one) {
    return 1;
  } else {
    return 0;
  }

} /* end of bdd_test_unate */


/**Function********************************************************************

  Synopsis [Finds the essential variable in a bdd f. Returns an
  array_t of vars which are the projection variables with the proper
  phase.]

  SideEffects [Creates an array_t of bdd_t. Freed by the caller. ]

******************************************************************************/
array_t *
bdd_find_essential(bdd_t *f)
{
  DdNode *C, *result, *scan, *cube;
  array_t *varArray = NIL(array_t);
  bdd_t *var;

  result = Cudd_FindEssential((DdManager *)f->mgr, (DdNode *)f->node);
  if (result == NULL) return NULL;
  cuddRef(result);

  cube = result;
  C = Cudd_Regular(cube);
  varArray = array_alloc(bdd_t *, 0);
  while (!cuddIsConstant(C)) {
    var = bdd_var_with_index(f->mgr, C->index);
    scan = cuddT(C);
    if (Cudd_NotCond(scan, !Cudd_IsComplement(cube)) == DD_ONE(f->mgr)) {
      array_insert_last(bdd_t *, varArray, bdd_not(var));
      bdd_free(var);
      scan = cuddE(C);
    } else {
      array_insert_last(bdd_t *, varArray, var);
    }
    cube = Cudd_NotCond(scan, Cudd_IsComplement(cube));
    C = Cudd_Regular(cube);
  }

  Cudd_RecursiveDeref((DdManager *)f->mgr,result);
  return varArray;

} /* end of bdd_find_essential */


/**Function********************************************************************

  Synopsis [Finds the essential variables in a bdd f. Returns a cube
  of the variables.]

  SideEffects [ ]

******************************************************************************/
bdd_t *
bdd_find_essential_cube(bdd_t *f)
{
  DdNode *cube;
  bdd_t *result;

  cube = Cudd_FindEssential((DdManager *)f->mgr, (DdNode *)f->node);
  if (cube == NULL) return NULL;
  cuddRef(cube);
  result = bdd_construct_bdd_t(f->mgr,cube);

  return(result);

} /* end of bdd_find_essential_cube */


/**Function********************************************************************

  Synopsis [Generates a BDD for the function x==y.]]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_xeqy(
  bdd_manager *mgr,
  int N,
  bdd_node **x,
  bdd_node **y)
{
  DdNode *result;
  result = Cudd_Xeqy((DdManager *)mgr,N,(DdNode **)x,
		     (DdNode **)y);
  return((bdd_node *)result);

} /* end of bdd_xeqy */


/**Function********************************************************************

  Synopsis [Rounds off the discriminants of an ADD.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_roundoff(bdd_manager *mgr, bdd_node *f, int N)
{
  DdNode *result;
  result = Cudd_addRoundOff((DdManager *)mgr,(DdNode *)f,N);
  return((bdd_node *)result);

} /* end of bdd_add_roundoff */


/**Function********************************************************************

  Synopsis    [Generates a BDD for the function x &gt; y.]

  Description [This function generates a BDD for the function x &gt; y.
  Both x and y are N-bit numbers, x\[0\] x\[1\] ... x\[N-1\] and
  y\[0\] y\[1\] ...  y\[N-1\], with 0 the most significant bit.
  The BDD is built bottom-up.
  It has 3*N-1 internal nodes, if the variables are ordered as follows:
  x\[0\] y\[0\] x\[1\] y\[1\] ... x\[N-1\] y\[N-1\].]

  SideEffects [None]

  SeeAlso     [bdd_dxygtdxz]

******************************************************************************/
bdd_node *
bdd_xgty(
  bdd_manager *mgr,
  int N,
  bdd_node **x,
  bdd_node **y)
{
  DdNode *result;
  result = Cudd_Xgty((DdManager *)mgr,N, NIL(DdNode *),
		     (DdNode **)x, (DdNode **)y);
  return((bdd_node *)result);

} /* end of bdd_xgty */


/**Function********************************************************************

  Synopsis [Computes the complement of an ADD a la C language.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_cmpl(bdd_manager *mgr, bdd_node *f)
{
  DdNode *result;
  result = Cudd_addCmpl((DdManager *)mgr,(DdNode *)f);
  return((bdd_node *)result);

} /* end of bdd_add_cmpl */


/**Function********************************************************************

  Synopsis    [Returns m minterms from a BDD.]

  Description [Returns <code>m</code> minterms from a BDD whose
  support has <code>n</code> variables at most.  The procedure tries
  to create as few extra nodes as possible. The function represented
  by <code>f</code> depends on at most <code>n</code> of the variables
  in <code>x</code>. Returns a BDD with <code>m</code> minterms of the
  on-set of f if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
bdd_node *
bdd_split_set(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **x,
  int n,
  double m)
{
  DdNode *result;
  result = Cudd_SplitSet((DdManager *)mgr,(DdNode *)f,
			 (DdNode **)x, n, m);
  return((bdd_node *)result);

} /* end of bdd_split_set */


/**Function********************************************************************

  Synopsis    [Checks for inconsistencies in the BDD manager.]

  Description [Checks for inconsistencies in the BDD manager.]

  SideEffects [None]

  SeeAlso [Cudd_DebugCheck]

******************************************************************************/
int
bdd_debug_check(bdd_manager *mgr)
{
  return Cudd_DebugCheck((DdManager *)mgr);

} /* end of bdd_debug_check */


/**Function********************************************************************

  Synopsis [Prints the minterns of f in the file stream fp. Precision
  can be specified in the last argument. Result is 1 if printing is
  successful, else 0.]

  SideEffects []

******************************************************************************/
int
bdd_print_apa_minterm(
  FILE *fp,
  bdd_t *f,
  int nvars,
  int precision)
{
  int result;
  result = Cudd_ApaPrintMintermExp(fp, (DdManager *)f->mgr,(DdNode *)f->node, nvars, precision);
  return(result);

} /* end of bdd_print_apa_minterm */


/**Function********************************************************************

  Synopsis [Compares the ratios of the minterms of 2 bdds and two numbers.
  The ratio compared is  ((Min(f1)/f1Num)/(Min(f2)/f2Num)). The procedure
  returns 1 if the ratio is greater than 1, 0 if they are equal and -1 if the
  ratio is less than 1. ]

  SideEffects []

******************************************************************************/
int
bdd_apa_compare_ratios(
  int nvars,
  bdd_t *f1,
  bdd_t *f2,
  int f1Num,
  int f2Num)
{
  int result;
  DdApaNumber f1Min, f2Min;
  int digits1, digits2;

  f1Min = Cudd_ApaCountMinterm((DdManager *)f1->mgr, (DdNode *)f1->node, nvars, &digits1);
  f2Min = Cudd_ApaCountMinterm((DdManager *)f2->mgr, (DdNode *)f2->node, nvars, &digits2);

  result = Cudd_ApaCompareRatios(digits1, f1Min, f1Num, digits2, f2Min, f2Num);
  return(result);

} /* end of bdd_apa_compare_ratios */


/**Function********************************************************************

  Synopsis [Computes the exclusive-or of f and g.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_xor(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  DdNode *result;
  result = Cudd_bddXor((DdManager *)mgr, (DdNode *)f, (DdNode *)g);

  return(result);

} /* end of bdd_bdd_xor */


/**Function********************************************************************

  Synopsis [Generates a blif file by dumpping BDDs. nBdds is the number
  of BDDs, bdds is the array of BDDs, inames is the array of primary
  input variable names, onames is the array of variable names of BDDs,
  and model is a model name in BLIF. inames, onames and model can be
  NULL.]

  SideEffects []

******************************************************************************/
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
  Cudd_DumpBlif((DdManager *)mgr, nBdds, (DdNode **)bdds, inames, onames,
		model, fp, 0);

} /* end of bdd_dump_blif */


/**Function********************************************************************

  Synopsis [Generates a blif body by dumpping BDDs. nBdds is the number
  of BDDs, bdds is the array of BDDs, inames is the array of primary
  input variable names, onames is the array of variable names of BDDs,
  and inames, onames and model can be NULL. This function prints out
  only .names body.]

  SideEffects []

******************************************************************************/
void
bdd_dump_blif_body(
  bdd_manager *mgr,
  int nBdds,
  bdd_node **bdds,
  char **inames,
  char **onames,
  FILE *fp)
{
  Cudd_DumpBlifBody((DdManager *)mgr, nBdds, (DdNode **)bdds, inames, onames,
		    fp, 0);

} /* end of bdd_dump_blif_body */


/**Function********************************************************************

  Synopsis [Generates a dot file by dumpping BDDs. nBdds is the number
  of BDDs, bdds is the array of BDDs, inames is the array of primary
  input variable names, and onames is the array of variable names of BDDs.
  inames, onames and model can be NULL.]

  SideEffects []

******************************************************************************/
void
bdd_dump_dot(
  bdd_manager *mgr,
  int nBdds,
  bdd_node **bdds,
  char **inames,
  char **onames,
  FILE *fp)
{
  Cudd_DumpDot((DdManager *)mgr, nBdds, (DdNode **)bdds, inames, onames, fp);

} /* end of bdd_dump_dot */


/**Function********************************************************************

  Synopsis [Converts a ZDD cover to a BDD graph.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_make_bdd_from_zdd_cover(bdd_manager *mgr, bdd_node *node)
{
  return((bdd_node *)Cudd_MakeBddFromZddCover((DdManager *)mgr, (DdNode *)node));

} /* end of bdd_make_bdd_from_zdd_cover */


/**Function********************************************************************

  Synopsis [Computes the complement of a ZDD cover.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_zdd_complement(bdd_manager *mgr, bdd_node *node)
{
  return((bdd_node *)Cudd_zddComplement((DdManager *)mgr, (DdNode *)node));

} /* end of bdd_zdd_complement */


/**Function********************************************************************

  Synopsis [Finds the variables on which a set of DDs depends.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_vector_support(bdd_manager *mgr, bdd_node **F, int n)
{
  return((bdd_node *)Cudd_VectorSupport((DdManager *)mgr,(DdNode **)F,n));

} /* end of bdd_bdd_vector_support */


/**Function********************************************************************

  Synopsis [Count the variables on which a set of DDs depend.]

  SideEffects []

******************************************************************************/
int
bdd_bdd_vector_support_size(bdd_manager *mgr, bdd_node **F, int n)
{
  return(Cudd_VectorSupportSize((DdManager *)mgr,(DdNode **)F,n));

} /* end of bdd_bdd_vector_support_size */


/**Function********************************************************************

  Synopsis [Count the variables on which a DD depends.]

  SideEffects []

******************************************************************************/
int
bdd_bdd_support_size(bdd_manager *mgr, bdd_node *F)
{
  return(Cudd_SupportSize((DdManager *)mgr,(DdNode *)F));

} /* end of bdd_bdd_support_size */


/**Function********************************************************************

  Synopsis [Returns the BDD of the variables on which F depends.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_support(bdd_manager *mgr, bdd_node *F)
{
  return((bdd_node *)Cudd_Support((DdManager *)mgr,(DdNode *)F));

} /* end of bdd_bdd_support */


/**Function********************************************************************

  Synopsis [Composes an ADD with a vector of ADDs.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_add_general_vector_compose(
  bdd_manager *mgr,
  bdd_node *f,
  bdd_node **vectorOn,
  bdd_node **vectorOff)
{
  return((bdd_node *)Cudd_addGeneralVectorCompose((DdManager *)mgr,
						  (DdNode *)f,
						  (DdNode **)vectorOn,
						  (DdNode **)vectorOff));

} /* end of bdd_add_general_vector_compose */


/**Function********************************************************************

  Synopsis [Computes the boolean difference of f w.r.t to variable x.]

  SideEffects []

******************************************************************************/
bdd_node *
bdd_bdd_boolean_diff(bdd_manager *mgr, bdd_node *f, int x)
{
  return ((bdd_node *)Cudd_bddBooleanDiff((DdManager *)mgr,(DdNode *)f,x));

} /* end of bdd_bdd_boolean_diff */


/**Function********************************************************************

  Synopsis [Check whether two BDDs intersect.]

  SideEffects []

******************************************************************************/
int
bdd_bdd_leq(bdd_manager *mgr, bdd_node *f, bdd_node *g)
{
  return Cudd_bddLeq((DdManager *)mgr,(DdNode *)f,(DdNode *)g);

} /* end of bdd_bdd_leq */


/**Function********************************************************************

  Synopsis    [Checks whether two bdds are same.]

  SideEffects []

******************************************************************************/
int
bdd_ptrcmp(bdd_t *f, bdd_t *g)
{
  if (f->node == g->node)
    return(0);
  else
    return(1);

} /* end of bdd_ptrcmp */


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

} /* end of bdd_ptrhash */


/**Function********************************************************************

  Synopsis    [Returns the peak memory in use.]

  SideEffects []

******************************************************************************/
long
bdd_read_peak_memory(bdd_manager *mgr)
{
  return((long) Cudd_ReadMemoryInUse((DdManager *) mgr));

} /* end of bdd_read_peak_memory */


/**Function********************************************************************

  Synopsis    [Returns the peak live node count.]

  SideEffects []

******************************************************************************/
int
bdd_read_peak_live_node(bdd_manager *mgr)
{
  return(Cudd_ReadPeakLiveNodeCount((DdManager *) mgr));

} /* end of bdd_read_peak_live_node */


/**Function********************************************************************

  Synopsis    [Sets a variable type to primary input.]

  SideEffects [none]

******************************************************************************/
int
bdd_set_pi_var(bdd_manager *mgr, int index)
{
  return Cudd_bddSetPiVar((DdManager *) mgr, index);

} /* bdd_set_pi_var */


/**Function********************************************************************

  Synopsis    [Sets a variable type to present state.]

  SideEffects [none]

******************************************************************************/
int
bdd_set_ps_var(bdd_manager *mgr, int index)
{
  return Cudd_bddSetPsVar((DdManager *) mgr, index);

} /* end of bdd_set_ps_var */


/**Function********************************************************************

  Synopsis    [Sets a variable type to next state.]

  SideEffects [none]

******************************************************************************/
int
bdd_set_ns_var(bdd_manager *mgr, int index)
{
  return Cudd_bddSetNsVar((DdManager *) mgr, index);

} /* end of bdd_set_ns_var */


/**Function********************************************************************

  Synopsis    [Checks whether a variable is primary input.]

  SideEffects [none]

******************************************************************************/
int
bdd_is_pi_var(bdd_manager *mgr, int index)
{
  return Cudd_bddIsPiVar((DdManager *) mgr, index);

} /* end of bdd_is_pi_var */


/**Function********************************************************************

  Synopsis    [Checks whether a variable is present state.]

  SideEffects [none]

******************************************************************************/
int
bdd_is_ps_var(bdd_manager *mgr, int index)
{
  return Cudd_bddIsPsVar((DdManager *) mgr, index);

} /* end of bdd_is_ps_var */


/**Function********************************************************************

  Synopsis    [Checks whether a variable is next state.]

  SideEffects [none]

******************************************************************************/
int
bdd_is_ns_var(bdd_manager *mgr, int index)
{
  return Cudd_bddIsNsVar((DdManager *) mgr, index);

} /* end of bdd_is_ns_var */


/**Function********************************************************************

  Synopsis    [Sets a corresponding pair index for a given index.]

  Description [Sets a corresponding pair index for a given index.
  These pair indices are present and next state variable.]

  SideEffects [none]

******************************************************************************/
int
bdd_set_pair_index(bdd_manager *mgr, int index, int pairidx)
{
  return Cudd_bddSetPairIndex((DdManager *) mgr, index, pairidx);

} /* end of bdd_set_pair_index */


/**Function********************************************************************

  Synopsis    [Reads a corresponding pair index for a given index.]

  Description [Reads a corresponding pair index for a given index.
  These pair indices are present and next state variable.]

  SideEffects [none]

******************************************************************************/
int
bdd_read_pair_index(bdd_manager *mgr, int index)
{
  return Cudd_bddReadPairIndex((DdManager *) mgr, index);

} /* end of bdd_read_pair_index */


/**Function********************************************************************

  Synopsis    [Sets a variable to be grouped.]

  Description [Sets a variable to be grouped. This function is used for
  lazy sifting.]

  SideEffects [none]

******************************************************************************/
int
bdd_set_var_to_be_grouped(bdd_manager *mgr, int index)
{
  return Cudd_bddSetVarToBeGrouped((DdManager *) mgr, index);

} /* end of bdd_set_var_to_be_grouped */


/**Function********************************************************************

  Synopsis    [Sets a variable to be a hard group.]

  Description [Sets a variable to be a hard group. This function is used
  for lazy sifting.]

  SideEffects [none]

******************************************************************************/
int
bdd_set_var_hard_group(bdd_manager *mgr, int index)
{
  return Cudd_bddSetVarHardGroup((DdManager *) mgr, index);

} /* end of bdd_set_var_hard_group */


/**Function********************************************************************

  Synopsis    [Resets a variable not to be grouped.]

  Description [Resets a variable not to be grouped. This function is
  used for lazy sifting.]

  SideEffects [none]

******************************************************************************/
int
bdd_reset_var_to_be_grouped(bdd_manager *mgr, int index)
{
  return Cudd_bddResetVarToBeGrouped((DdManager *) mgr, index);

} /* end of bdd_reset_var_to_be_grouped */


/**Function********************************************************************

  Synopsis    [Checks whether a variable is set to be grouped.]

  Description [Checks whether a variable is set to be grouped. This
  function is used for lazy sifting.]

  SideEffects [none]

******************************************************************************/
int
bdd_is_var_to_be_grouped(bdd_manager *mgr, int index)
{
  return Cudd_bddIsVarToBeGrouped((DdManager *) mgr, index);

} /* end of bdd_is_var_to_be_grouped */


/**Function********************************************************************

  Synopsis    [Checks whether a variable is set to be a hard group.]

  Description [Checks whether a variable is set to be a hard group. This
  function is used for lazy sifting.]

  SideEffects [none]

******************************************************************************/
int
bdd_is_var_hard_group(bdd_manager *mgr, int index)
{
  return Cudd_bddIsVarHardGroup((DdManager *) mgr, index);

} /* end of bdd_is_var_hard_group */


/**Function********************************************************************

  Synopsis    [Sets a variable to be ungrouped.]

  Description [Sets a variable to be ungrouped. This function is used
  for lazy sifting.]

  SideEffects [none]

******************************************************************************/
int
bdd_is_var_to_be_ungrouped(bdd_manager *mgr, int index)
{
  return Cudd_bddIsVarToBeUngrouped((DdManager *) mgr, index);

} /* end of bdd_is_var_to_be_ungrouped */


/**Function********************************************************************

  Synopsis    [Sets a variable to be ungrouped.]

  Description [Sets a variable to be ungrouped. This function is used
  for lazy sifting.]

  SideEffects [none]

******************************************************************************/
int
bdd_set_var_to_be_ungrouped(bdd_manager *mgr, int index)
{
  return Cudd_bddSetVarToBeUngrouped((DdManager *) mgr, index);

} /* end of bdd_set_var_to_be_ungrouped */


/**Function********************************************************************

  Synopsis    [Prevents sifting of a variable.]

  Description [This function sets a flag to prevent sifting of a
  variable.  Returns 1 if successful; 0 otherwise (i.e., invalid
  variable index).]

  SideEffects [Changes the "bindVar" flag in DdSubtable.]

******************************************************************************/
int
bdd_bind_var(bdd_manager *mgr, int index)
{
  return Cudd_bddBindVar((DdManager *) mgr, index);

} /* end of bdd_bind_var */


/**Function********************************************************************

  Synopsis    [Allows the sifting of a variable.]

  Description [This function resets the flag that prevents the sifting
  of a variable. In successive variable reorderings, the variable will
  NOT be skipped, that is, sifted.  Initially all variables can be
  sifted. It is necessary to call this function only to re-enable
  sifting after a call to Cudd_bddBindVar. Returns 1 if successful; 0
  otherwise (i.e., invalid variable index).]

  SideEffects [Changes the "bindVar" flag in DdSubtable.]

******************************************************************************/
int
bdd_unbind_var(bdd_manager *mgr, int index)
{
  return Cudd_bddUnbindVar((DdManager *) mgr, index);

} /* end of bdd_unbind_var */


/**Function********************************************************************

  Synopsis    [Checks whether lazy sifting is on.]

  SideEffects [none]

******************************************************************************/
int
bdd_is_lazy_sift(bdd_manager *mgr)
{
  Cudd_ReorderingType method;

  Cudd_ReorderingStatus((DdManager *) mgr, &method);
  if (method == CUDD_REORDER_LAZY_SIFT)
    return(1);
  return(0);

} /* end of bdd_is_lazy_sift */


/**Function********************************************************************

  Synopsis    [Frees the variable group tree of the manager.]

  SideEffects [None]

******************************************************************************/
void
bdd_discard_all_var_groups(bdd_manager *mgr)
{
  Cudd_FreeTree((DdManager *) mgr);

} /* end of bdd_discard_all_var_groups */


/**Function********************************************************************

  Synopsis    [Sets the time limit (in ms) for the manager.]

  SideEffects [None]

******************************************************************************/
void
bdd_set_time_limit(bdd_manager *mgr, unsigned long tl)
{
  Cudd_SetTimeLimit((DdManager *) mgr, tl);

} /* end of bdd_set_time_limit */


/**Function********************************************************************

  Synopsis    [Unsets the time limit for the manager.]

  SideEffects [None]

******************************************************************************/
void
bdd_unset_time_limit(bdd_manager *mgr)
{
  Cudd_UnsetTimeLimit((DdManager *) mgr);

} /* end of bdd_unset_time_limit */


/**Function********************************************************************

  Synopsis    [Resets the start time for the manager.]

  SideEffects [None]

******************************************************************************/
void
bdd_reset_start_time(bdd_manager *mgr)
{
  Cudd_ResetStartTime((DdManager *) mgr);

} /* end of bdd_reset_start_time */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis [Function to print a warning that an illegal value was read.]

  SideEffects        []

  SeeAlso            [bdd_set_parameters]

******************************************************************************/
static void
InvalidType(FILE *file, const char *field, const char *expected)
{
  (void) fprintf(file, "Warning: In parameter \"%s\"\n", field);
  (void) fprintf(file, "Illegal type detected. %s expected\n", expected);

} /* end of InvalidType */
