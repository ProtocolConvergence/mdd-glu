/**CFile***********************************************************************

  FileName    [calPortIter.c]

  PackageName [calPort]

  Synopsis    [required]

  Description [optional]

  SeeAlso     [optional]

  Author      [Rajeev K. Ranjan. Modified from the CMU port package
  written by Tom Shiple.]

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

******************************************************************************/
#include "calPortInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/
typedef struct CalBddGenStruct CalBddGen_t;
/*
 * Traversal of BDD Formulas
 */

typedef enum {
    bdd_gen_cubes,
    bdd_gen_nodes
} bdd_gen_type;


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/
struct CalBddGenStruct {
  Cal_BddManager manager;
  bdd_gen_status status;
  bdd_gen_type type;
  union {
	struct {
      array_t *cube;	/* of bdd_literal */
	} cubes;
	struct {
      st_table *visited;	/* of bdd_node* */
	} nodes;
  } gen;	
  struct {
	int sp;
	CalBddNode_t **nodeStack;
    int *idStack;
  } stack;
  CalBddNode_t *node;
};




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
static void pop_node_stack(CalBddGen_t *gen);
static void push_node_stack(Cal_Bdd_t f, CalBddGen_t *gen);
static void pop_cube_stack(CalBddGen_t *gen);
static void push_cube_stack(Cal_Bdd_t f, CalBddGen_t *gen);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/
/*
 *    Defines an iterator on the onset of a BDD.  Two routines are
 *    provided: bdd_first_cube, which extracts one cube from a BDD and
 *    returns a bdd_gen structure containing the information necessary to
 *    continue the enumeration; and bdd_next_cube, which returns 1 if
 *     another cube was 
 *    found, and 0 otherwise. A cube is represented
 *    as an array of bdd_literal (which are integers in {0, 1, 2}),
 *    where 0 represents 
 *    negated literal, 1 for literal, and 2 for don't care.  Returns a
 *    disjoint 
 *    cover.  A third routine is there to clean up. 
 */

bdd_gen_status
bdd_gen_read_status(bdd_gen *gen)
{
  return ((CalBddGen_t *)gen)->status;
}

/**Function********************************************************************

  Synopsis           [required]

  Description        [bdd_first_cube - return the first cube of the function.
  A generator is returned that will iterate over the rest. Return the
  generator. ]

  SideEffects        [required]

  SeeAlso            [optional]

******************************************************************************/
bdd_gen *
bdd_first_cube(bdd_t *fn,array_t **cube)
{
  Cal_BddManager_t *manager;
  CalBddGen_t *gen;
  int i;
  long numVars;
  Cal_Bdd function;
  Cal_Bdd_t calBdd;
  
  if (fn == NIL(bdd_t)) {
	CalBddFatalMessage("bdd_first_cube: invalid BDD");
  }

  manager = fn->bddManager;
  function = fn->calBdd;
  
  /*
   *    Allocate a new generator structure and fill it in; the stack and the 
   *    cube will be used, but the visited table and the node will not be used.
   */
  gen = ALLOC(CalBddGen_t, 1);
  
    /*
     *    first - init all the members to a rational value for cube iteration
     */
  gen->manager = manager;
  gen->status = bdd_EMPTY;
  gen->type = bdd_gen_cubes;
  gen->gen.cubes.cube = NIL(array_t);
  gen->stack.sp = 0;
  gen->stack.nodeStack = NIL(CalBddNode_t *);
  gen->stack.idStack = NIL(int);
  gen->node = NIL(CalBddNode_t);
  
  numVars = Cal_BddVars(manager);
  gen->gen.cubes.cube = array_alloc(bdd_literal, numVars);
    
  /*
   * Initialize each literal to 2 (don't care).
   */
  for (i = 0; i < numVars; i++) {
    array_insert(bdd_literal, gen->gen.cubes.cube, i, 2);
  }
  
  /*
   * The stack size will never exceed the number of variables in the BDD, since
   * the longest possible path from root to constant 1 is the number
   * of variables in the BDD.
   */
  gen->stack.sp = 0;
  gen->stack.nodeStack = ALLOC(CalBddNode_t *, numVars);
  gen->stack.idStack = ALLOC(int, numVars);

  /*
   * Clear out the stack so that in bdd_gen_free, we can decrement the
   * ref count of those nodes still on the stack.
   */
  for (i = 0; i < numVars; i++) {
	gen->stack.nodeStack[i] = NIL(CalBddNode_t);
	gen->stack.idStack[i] = -1;
  }
  
  if (Cal_BddIsBddZero(manager, function)){
	/*
	 *    All done, for this was but the zero constant ...
	 *    We are enumerating the onset, (which is vacuous).
     *    gen->status initialized to bdd_EMPTY above, so this
     *    appears to be redundant.
	 */
	gen->status = bdd_EMPTY;
  } else {
	/*
	 *    Get to work enumerating the onset.  Get the first cube.  Note that
     *    if fn is just the constant 1, push_cube_stack will properly
     *    handle this. 
	 *    Get a new pointer to fn->node beforehand: this increments 
	 *    the reference count of fn->node; this is necessary, because
     *    when fn->node 
	 *    is popped from the stack at the very end, it's ref count is
     *    decremented. 
	 */
	gen->status = bdd_NONEMPTY;
    calBdd = CalBddGetInternalBdd(manager, function);
    push_cube_stack(calBdd, gen);
  }
  *cube = gen->gen.cubes.cube;
  return (bdd_gen *)(gen);
}

/**Function********************************************************************

  Synopsis           [required]

  Description        [bdd_next_cube - get the next cube on the generator.
  Returns {TRUE, FALSE} when {more, no more}.]

  SideEffects        [required]

  SeeAlso            [optional]

******************************************************************************/
boolean
bdd_next_cube(bdd_gen *gen_, array_t **cube)
{
  CalBddGen_t *gen = (CalBddGen_t *) gen_;
  pop_cube_stack(gen);
  if (gen->status == bdd_EMPTY) {
    return (FALSE);
  }
  *cube = gen->gen.cubes.cube;
  return (TRUE);
}

bdd_gen *
bdd_first_disjoint_cube(bdd_t *fn,array_t **cube)
{
  return(bdd_first_cube(fn,cube));
}

boolean
bdd_next_disjoint_cube(bdd_gen *gen_, array_t **cube)
{
  return(bdd_next_cube(gen_,cube));
}

/**Function********************************************************************

  Synopsis           [required]

  Description        [bdd_first_node - enumerates all bdd_node in fn. 
  Return the generator.]

  SideEffects        [required]

  SeeAlso            [optional]

******************************************************************************/
bdd_gen *
bdd_first_node(bdd_t *fn, bdd_node **node)
{
  Cal_BddManager_t *manager;
  CalBddGen_t *gen;
  long numVars;
  int i;
  Cal_Bdd_t calBdd;
  Cal_Bdd function;
  
  if (fn == NIL(bdd_t)) {
	CalBddFatalMessage("bdd_first_node: invalid BDD");
  }
  
  manager = fn->bddManager;
  function = fn->calBdd;
  
  /*
   *    Allocate a new generator structure and fill it in; the
   *    visited table will be used, as will the stack, but the
   *    cube array will not be used.
   */
  gen = ALLOC(CalBddGen_t, 1);
  
  /*
   *    first - init all the members to a rational value for node iteration.
   */
  gen->manager = manager;
  gen->status = bdd_NONEMPTY;
  gen->type = bdd_gen_nodes;
  gen->gen.nodes.visited = NIL(st_table);
  gen->stack.sp = 0;
  gen->stack.nodeStack = NIL(CalBddNode_t *);
  gen->stack.idStack = NIL(int);
  gen->node = NIL(CalBddNode_t);
  
  /* 
   * Set up the hash table for visited nodes.  Every time we visit a node,
   * we insert it into the table.
   */
  gen->gen.nodes.visited = st_init_table(st_ptrcmp, st_ptrhash);
  
  /*
   * The stack size will never exceed the number of variables in the BDD, since
   * the longest possible path from root to constant 1 is the number
   * of variables in the BDD.
   */
  gen->stack.sp = 0;
  numVars = Cal_BddVars(manager);
  gen->stack.nodeStack = ALLOC(CalBddNode_t *, numVars);
  gen->stack.idStack = ALLOC(int, numVars);
  /*
   * Clear out the stack so that in bdd_gen_free, we can decrement the
   * ref count of those nodes still on the stack.
   */
  for (i = 0; i < numVars; i++) {
	gen->stack.nodeStack[i] = NIL(CalBddNode_t);
	gen->stack.idStack[i] = -1;
  }
  
  /*
   * Get the first node.  Get a new pointer to fn->node beforehand:
   * this increments 
   * the reference count of fn->node; this is necessary, because when fn->node
   * is popped from the stack at the very end, it's ref count is decremented.
   */
  calBdd = CalBddGetInternalBdd(manager, function);
  push_node_stack(calBdd, gen);
  gen->status = bdd_NONEMPTY;
  
  *node = (bdd_node *)gen->node;	/* return the node */
  return (bdd_gen *)(gen);	/* and the new generator */
}

/**Function********************************************************************

  Synopsis           [required]

  Description        [bdd_next_node - get the next node in the BDD.
  Return {TRUE, FALSE} when {more, no more}. ]

  SideEffects        [required]

  SeeAlso            [optional]

******************************************************************************/
boolean
bdd_next_node(bdd_gen *gen_,bdd_node **node)
{
  CalBddGen_t *gen = (CalBddGen_t *) gen_;
  pop_node_stack(gen);
  if (gen->status == bdd_EMPTY) {
	return (FALSE);
  }
  *node = (bdd_node *) gen->node;
  return (TRUE);
}

/**Function********************************************************************

  Synopsis           [required]

  Description        [bdd_gen_free - frees up the space used by the generator.
  Return an int so that it is easier to fit in a foreach macro.
  Return 0 (to make it easy to put in expressions). ]

  SideEffects        [required]

  SeeAlso            [optional]

******************************************************************************/
int
bdd_gen_free(bdd_gen *gen_)
{
  CalBddGen_t *gen = (CalBddGen_t *) gen_;
  st_table *visited_table;

  switch (gen->type) {
    case bdd_gen_cubes:
      array_free(gen->gen.cubes.cube);
      gen->gen.cubes.cube = NIL(array_t);
      break;
    case bdd_gen_nodes:
      visited_table = gen->gen.nodes.visited;
      st_free_table(visited_table);
      visited_table = NIL(st_table);
      break;
  }
  FREE(gen->stack.nodeStack);
  FREE(gen->stack.idStack);
  FREE(gen);
  return (0);	/* make it return some sort of an int */
}

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/
/*
 *    Invariants:
 *
 *    gen->stack.stack contains nodes that remain to be explored.
 *
 *    For a cube generator,
 *        gen->gen.cubes.cube reflects the choices made to reach node
 *        at top of the stack. 
 *    For a node generator,
 *        gen->gen.nodes.visited reflects the nodes already visited in
 *         the BDD dag. 
 */

/**Function********************************************************************

  Synopsis           [required]

  Description        [push_cube_stack - push a cube onto the stack to
  visit. Return nothing, just do it. 
  The BDD is traversed using depth-first search, with the ELSE branch
  searched before the THEN branch. 
  Caution: If you are creating new BDD's while iterating through the
  cubes, and a garbage collection happens to be performed during this
  process, then the BDD generator will get lost and an error will result. ]

  SideEffects        [required]

  SeeAlso            [optional]

******************************************************************************/
static void
push_cube_stack(Cal_Bdd_t f, CalBddGen_t *gen)
{
  bdd_variableId topId;
  Cal_Bdd_t f0, f1;
  Cal_BddManager_t *manager;

  manager = gen->manager;
  
  if (CalBddIsBddOne(manager, f)){
    return;
  }

  topId = f.bddId-1;
  CalBddGetElseBdd(f, f0);
  CalBddGetThenBdd(f, f1);

  if (CalBddIsBddZero(manager, f1)){ 
/*
 *    No choice: take the 0 branch.  Since there is only one branch to 
 *    explore from f, there is no need to push f onto the stack, because
 *    after exploring this branch we are done with f.  A consequence of 
 *    this is that there will be no f to pop either.  Same goes for the
 *    next case.  Decrement the ref count of f and of the branch leading
 *    to zero, since we will no longer need to access these nodes.
 */
    array_insert(bdd_literal, gen->gen.cubes.cube, topId, 0);
	push_cube_stack(f0, gen);
  }
  else if (CalBddIsBddZero(manager, f0)){
	/*
	 *    No choice: take the 1 branch
	 */
	array_insert(bdd_literal, gen->gen.cubes.cube, topId, 1);
	push_cube_stack(f1, gen);
  } else {
    /*
     * In this case, we must explore both branches of f.  We always choose
     * to explore the 0 branch first.  We must push f on the stack, so that
     * we can later pop it and explore its 1 branch. Decrement the ref count 
     * of f1 since we will no longer need to access this node.  Note that 
     * the parent of f1 was bdd_freed above or in pop_cube_stack.
     */
    gen->stack.nodeStack[gen->stack.sp] = f.bddNode;
    gen->stack.idStack[gen->stack.sp++] = f.bddId;
    array_insert(bdd_literal, gen->gen.cubes.cube, topId, 0);
    push_cube_stack(f0, gen);
  }
}

/**Function********************************************************************

  Synopsis           [required]

  Description        [optional]

  SideEffects        [required]

  SeeAlso            [optional]

******************************************************************************/
static void
pop_cube_stack(CalBddGen_t *gen)
{
  CalBddNode_t *fNode;
  int fId, fIndex, i;
  Cal_Bdd_t f1, f;
  Cal_BddManager_t *manager;
  
  manager = gen->manager;
  if (gen->stack.sp == 0) {
    /*
     * Stack is empty.  Have already explored both the 0 and 1 branches of 
     * the root of the BDD.
     */
	gen->status = bdd_EMPTY;
  } else {
    /*
     * Explore the 1 branch of the node at the top of the stack (since it is
     * on the stack, this means we have already explored the 0 branch).  We 
     * permanently pop the top node, and bdd_free it, since there are
     * no more edges left to explore. 
     */
	fNode = gen->stack.nodeStack[--gen->stack.sp];
	fId = gen->stack.idStack[gen->stack.sp];
	gen->stack.nodeStack[gen->stack.sp] = NIL(CalBddNode_t); /* overwrite */
                                                         /* with NIL */
	gen->stack.idStack[gen->stack.sp] = -1;
	array_insert(bdd_literal, gen->gen.cubes.cube, fId-1, 1);
    fIndex = manager->idToIndex[fId];
	for (i = fIndex + 1; i < array_n(gen->gen.cubes.cube); i++) {
      array_insert(bdd_literal, gen->gen.cubes.cube,
                   manager->indexToId[i]-1, 2); 
	}
    f.bddNode = fNode;
    f.bddId = fId;
    CalBddGetThenBdd(f, f1);
	push_cube_stack(f1, gen);
  }
}

/**Function********************************************************************

  Synopsis           [required]

  Description        [push_node_stack - push a node onto the stack.
  The same as push_cube_stack but for enumerating nodes instead of cubes.
  The BDD is traversed using depth-first search, with the ELSE branch
  searched before the THEN branch, and a node returned only after its
  children have been returned.  Note that the returned bdd_node
  pointer has the complement bit zeroed out.
  Caution: If you are creating new BDD's while iterating through the
  nodes, and a garbage collection happens to be performed during this
  process, then the BDD generator will get lost and an error will result. ]

  SideEffects        [required]

  SeeAlso            [optional]

******************************************************************************/
static void
push_node_stack(Cal_Bdd_t f, CalBddGen_t *gen)
{
  Cal_Bdd_t f0, f1;
  bdd_node *reg_f, *reg_f0, *reg_f1;

  reg_f = (CalBddNode_t *) CAL_BDD_POINTER(f.bddNode);  /* use of calInt.h */
  if (st_is_member(gen->gen.nodes.visited, (char *) reg_f)){
    /* 
     * Already been visited.
     */
	return;
  }
  
  if (CalBddIsBddConst(f)){
    /*
     * If f is the constant node and it has not been visited yet, then
     * put it in the visited table and set the gen->node pointer.
     * There is no need to put it in the stack because 
     * the constant node does not have any branches, and there is no
     * need to free f because constant nodes have a saturated
     * reference count. 
     */
	st_insert(gen->gen.nodes.visited, (char *) reg_f, NIL(char));
	gen->node = (CalBddNode_t *) reg_f;
  } else {
    /*
     * f has not been marked as visited.  We don't know yet if any of
     * its branches remain to be explored.  First get its branches.
     */
    CalBddGetElseBdd(f, f0);
    CalBddGetThenBdd(f, f1);

	reg_f0 = (CalBddNode_t *) CAL_BDD_POINTER(f0.bddNode);
	reg_f1 = (CalBddNode_t *) CAL_BDD_POINTER(f1.bddNode);

	if (st_is_member(gen->gen.nodes.visited, (char *) reg_f0) == 0){
      /* 
       * The 0 child has not been visited, so explore the 0 branch.
       * First push f on the stack.  Bdd_free f1 since we will not
       * need to access this exact pointer any more.
       */
      gen->stack.nodeStack[gen->stack.sp] = f.bddNode;
      gen->stack.idStack[gen->stack.sp++] = f.bddId;
      push_node_stack(f0, gen);
	} else{
      if (st_is_member(gen->gen.nodes.visited, (char *) reg_f1) == 0){
        /* 
         * The 0 child has been visited, but the 1 child has not been
         * visited, so explore the 1 branch.  First push f on the
         * stack. 
         */
        gen->stack.nodeStack[gen->stack.sp] = f.bddNode;
        gen->stack.idStack[gen->stack.sp++] = f.bddId;
        push_node_stack(f1, gen);
      } else {
        /*
         * Both the 0 and 1 children have been visited. Thus we are done
         * exploring from f.   
         * Mark f as visited (put it in the visited table), and set the
         * gen->node pointer. 
         */
        st_insert(gen->gen.nodes.visited, (char *) reg_f, NIL(char));
        gen->node = (CalBddNode_t *) reg_f;
      }
    }
  }
}

/**Function********************************************************************

  Synopsis           [required]

  Description        [optional]

  SideEffects        [required]

  SeeAlso            [optional]

******************************************************************************/

static void
pop_node_stack(CalBddGen_t *gen)
{
  CalBddNode_t *fNode;
  int fId;
  Cal_Bdd_t calBdd;
  
  if (gen->stack.sp == 0) {
    gen->status = bdd_EMPTY;
    return;
  }
  fNode = gen->stack.nodeStack[--gen->stack.sp]; 
  fId = gen->stack.idStack[gen->stack.sp];
  gen->stack.nodeStack[gen->stack.sp] = NIL(CalBddNode_t);
  gen->stack.idStack[gen->stack.sp] = -1;
  calBdd.bddNode = fNode;
  calBdd.bddId = fId;
  push_node_stack(calBdd, gen);
}

