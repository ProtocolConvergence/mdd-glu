/******************************************************************************

  PackageName [bdd]

  Synopsis    [Package-independent BDD interface functions.]

  Description []

  SeeAlso     []

  Author      [Thomas R. Shiple. Modified by Rajeev K. Ranjan.]

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

  Revision    [$Id: bdd.h,v 1.82 2012/02/04 21:43:23 fabio Exp $]

******************************************************************************/

#ifndef _BDD_H
#define _BDD_H
#include "util.h"
#include "array.h"
#include "st.h"
#include "var_set.h"
#include "avl.h"
#ifndef	EPD_MAX_BIN
#include "epd.h"
#endif

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/
typedef int boolean;
typedef void bdd_manager;
typedef unsigned int bdd_variableId; /* the id of the variable in a bdd node */
typedef void bdd_mgr_init; /* Typecasting to void to avoid error messages */
typedef int bdd_literal;	        /* integers in the set { 0, 1, 2 } */
typedef struct bdd_t bdd_t;
typedef void bdd_node;
typedef void bdd_gen;
typedef struct bdd_external_hooks_struct bdd_external_hooks;
typedef void bdd_block;
typedef double BDD_VALUE_TYPE;

/*---------------------------------------------------------------------------*/
/* Enumerated type declarations                                              */
/*---------------------------------------------------------------------------*/

/*
 * Dynamic reordering.
 */
typedef enum {
  BDD_REORDER_SIFT,
  BDD_REORDER_WINDOW,
  BDD_REORDER_SAME,
  BDD_REORDER_RANDOM,
  BDD_REORDER_RANDOM_PIVOT,
  BDD_REORDER_SIFT_CONVERGE,
  BDD_REORDER_SYMM_SIFT,
  BDD_REORDER_SYMM_SIFT_CONV,
  BDD_REORDER_LINEAR,
  BDD_REORDER_LINEAR_CONVERGE,
  BDD_REORDER_EXACT,
  BDD_REORDER_WINDOW2,
  BDD_REORDER_WINDOW3,
  BDD_REORDER_WINDOW4,
  BDD_REORDER_WINDOW2_CONV,
  BDD_REORDER_WINDOW3_CONV,
  BDD_REORDER_WINDOW4_CONV,
  BDD_REORDER_GROUP_SIFT,
  BDD_REORDER_GROUP_SIFT_CONV,
  BDD_REORDER_ANNEALING,
  BDD_REORDER_GENETIC,
  BDD_REORDER_LAZY_SIFT,
  BDD_REORDER_NONE
} bdd_reorder_type_t;

typedef enum {
  CMU,
  CAL,
  CUDD
} bdd_package_type_t;

typedef enum {
    bdd_EMPTY,
    bdd_NONEMPTY
} bdd_gen_status;

typedef enum {
    BDD_PRE_GC_HOOK,
    BDD_POST_GC_HOOK,
    BDD_PRE_REORDERING_HOOK,
    BDD_POST_REORDERING_HOOK
} bdd_hook_type_t;

typedef enum {
    BDD_OVER_APPROX,
    BDD_UNDER_APPROX
} bdd_approx_dir_t;

typedef enum {
    BDD_CONJUNCTS,
    BDD_DISJUNCTS
} bdd_partition_type_t;

typedef enum {
  BDD_REORDER_VERBOSITY_DEFAULT,
  BDD_REORDER_NO_VERBOSITY,
  BDD_REORDER_VERBOSITY,
  BDD_REORDER_MORE_VERBOSITY
}bdd_reorder_verbosity_t;

typedef enum {
  BDD_APPROX_HB,
  BDD_APPROX_SP,
  BDD_APPROX_COMP,
  BDD_APPROX_UA,
  BDD_APPROX_RUA,
  BDD_APPROX_BIASED_RUA
} bdd_approx_type_t;


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/
struct bdd_external_hooks_struct {
    char *network;
    char *mdd;
    char *undef1;
};


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/
/*
** It is assumed that dynamic reordering will not occur while there are open
** generators.  It is the user's responsibility to make sure dynamic
** reordering doesn't occur. As long as new nodes are not created during
** generation, and you don't explicitly call dynamic reordering, you should be
** okay. 
*/

/*
 *    foreach_bdd_cube(fn, gen, cube)
 *    bdd_t *fn;
 *    bdd_gen *gen;
 *    array_t *cube;	- return
 *
 *    foreach_bdd_cube(fn, gen, cube) {
 *        ...
 *    }
 */
#define foreach_bdd_cube(fn, gen, cube)\
  for((gen) = bdd_first_cube(fn, &cube);\
      (bdd_gen_read_status(gen) != bdd_EMPTY) ? TRUE: bdd_gen_free(gen);\
      (void) bdd_next_cube(gen, &cube))

/*
 *    foreach_bdd_disjoint_cube(fn, gen, cube)
 *    bdd_t *fn;
 *    bdd_gen *gen;
 *    array_t *cube;	- return
 *
 *    foreach_bdd_disjoint_cube(fn, gen, cube) {
 *        ...
 *    }
 */
#define foreach_bdd_disjoint_cube(fn, gen, cube)\
  for((gen) = bdd_first_disjoint_cube(fn, &cube);\
      (bdd_gen_read_status(gen) != bdd_EMPTY) ? TRUE: bdd_gen_free(gen);\
      (void) bdd_next_disjoint_cube(gen, &cube))

/*
 *    foreach_bdd_node(fn, gen, node)
 *    bdd_t *fn;
 *    bdd_gen *gen;
 *    bdd_node *node;	- return
 */
#define foreach_bdd_node(fn, gen, node)\
  for((gen) = bdd_first_node(fn, &node);\
      (bdd_gen_read_status(gen) != bdd_EMPTY) ? TRUE: bdd_gen_free(gen);\
      (void) bdd_next_node(gen, &node))


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/


/**AutomaticEnd***************************************************************/

EXTERN bdd_package_type_t bdd_get_package_name(void);

/*
 * BDD Manager Allocation And Destruction
 */
EXTERN bdd_manager *bdd_start(int);
EXTERN void bdd_end(bdd_manager *);

/*
 * BDD variable management
 */
EXTERN bdd_t *bdd_create_variable(bdd_manager *);		
EXTERN bdd_t *bdd_create_variable_after(bdd_manager *, bdd_variableId);
EXTERN bdd_t *bdd_get_variable(bdd_manager *, bdd_variableId);	
EXTERN bdd_t *bdd_create_variable_after(bdd_manager *, bdd_variableId);
EXTERN bdd_t * bdd_var_with_index(bdd_manager *manager, int index);
EXTERN bdd_node *bdd_add_ith_var(bdd_manager *mgr, int i);


/*
 * BDD Formula Management
 */
EXTERN bdd_t *bdd_dup(bdd_t *);
EXTERN void bdd_free(bdd_t *);

/* 
 * Operations on BDD Formulas
 */
EXTERN bdd_t *bdd_and(bdd_t *, bdd_t *, boolean, boolean);
EXTERN bdd_t *bdd_and_with_limit(bdd_t *, bdd_t *, boolean, boolean, unsigned int);
EXTERN bdd_t *bdd_and_array(bdd_t *, array_t *, boolean, boolean);
EXTERN bdd_t *bdd_and_smooth(bdd_t *, bdd_t *, array_t *);
EXTERN bdd_t *bdd_and_smooth_with_limit(bdd_t *, bdd_t *, array_t *, unsigned int);
EXTERN bdd_t *bdd_and_smooth_with_cube(bdd_t *, bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_clipping_and_smooth(bdd_t *, bdd_t *, array_t *, int , int );
EXTERN bdd_t *bdd_xor_smooth(bdd_t *, bdd_t *, array_t *);
EXTERN bdd_t *bdd_between(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_cofactor(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_cofactor_array(bdd_t *, array_t *);
EXTERN bdd_t *bdd_var_cofactor(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_compose(bdd_t *, bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_vector_compose(bdd_t *, array_t *, array_t *);
EXTERN bdd_t *bdd_consensus(bdd_t *, array_t *);
EXTERN bdd_t *bdd_consensus_with_cube(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_cproject(bdd_t *, array_t *);
EXTERN bdd_t *bdd_else(bdd_t *);
EXTERN bdd_t *bdd_ite(bdd_t *, bdd_t *, bdd_t *, boolean, boolean, boolean);
EXTERN bdd_t *bdd_minimize(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_minimize_array(bdd_t *, array_t *);
EXTERN bdd_t *bdd_compact(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_squeeze(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_not(bdd_t *);
EXTERN bdd_t *bdd_one(bdd_manager *);
EXTERN bdd_t *bdd_or(bdd_t *, bdd_t *, boolean, boolean);
EXTERN bdd_t *bdd_smooth(bdd_t *, array_t *);
EXTERN bdd_t *bdd_smooth_with_cube(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_substitute(bdd_t *, array_t *, array_t *);
EXTERN bdd_t *bdd_substitute_with_permut(bdd_t *, int *);
EXTERN array_t *bdd_substitute_array(array_t *, array_t *, array_t *);
EXTERN array_t *bdd_substitute_array_with_permut(array_t *, int *);
EXTERN void *bdd_pointer(bdd_t *);
EXTERN bdd_t *bdd_then(bdd_t *);
EXTERN bdd_t *bdd_top_var(bdd_t *);
EXTERN bdd_t *bdd_xnor(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_xor(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_zero(bdd_manager *);
EXTERN bdd_t *bdd_multiway_and(bdd_manager *, array_t *);
EXTERN bdd_t *bdd_multiway_or(bdd_manager *, array_t *);
EXTERN bdd_t *bdd_multiway_xor(bdd_manager *, array_t *);
EXTERN array_t * bdd_pairwise_or(bdd_manager *manager, array_t *bddArray1, array_t *bddArray2);
EXTERN array_t * bdd_pairwise_and(bdd_manager *manager, array_t *bddArray1, array_t *bddArray2);
EXTERN array_t * bdd_pairwise_xor(bdd_manager *manager, array_t *bddArray1, array_t *bddArray2);
EXTERN bdd_t *bdd_approx_hb(bdd_t *, bdd_approx_dir_t , int , int );
EXTERN bdd_t *bdd_approx_sp(bdd_t *, bdd_approx_dir_t , int , int , int );
EXTERN bdd_t *bdd_approx_ua(bdd_t *, bdd_approx_dir_t , int , int , int , double );
EXTERN bdd_t *bdd_approx_remap_ua(bdd_t *, bdd_approx_dir_t , int , int , double );
EXTERN bdd_t *bdd_approx_biased_rua(bdd_t *, bdd_approx_dir_t , bdd_t *, int , int , double, double );
EXTERN bdd_t *bdd_approx_compress(bdd_t *, bdd_approx_dir_t , int , int );
EXTERN int bdd_var_decomp(bdd_t *, bdd_partition_type_t , bdd_t ***);
EXTERN int bdd_gen_decomp(bdd_t *, bdd_partition_type_t , bdd_t ***);
EXTERN int bdd_approx_decomp(bdd_t *, bdd_partition_type_t , bdd_t ***);
EXTERN int bdd_iter_decomp(bdd_t *, bdd_partition_type_t , bdd_t ***);
EXTERN bdd_t *bdd_solve_eqn(bdd_t *f, array_t *g, array_t *unknowns);
EXTERN bdd_t *bdd_shortest_path(bdd_t *f, int *weight, int *support, int *length);

EXTERN bdd_t *bdd_compute_cube(bdd_manager *mgr, array_t *vars);
EXTERN bdd_t *bdd_compute_cube_with_phase(bdd_manager *mgr, array_t *vars, array_t *phase);
EXTERN bdd_node *bdd_add_compose(bdd_manager *mgr, bdd_node *fn1, bdd_node *fn2, int var);
EXTERN bdd_node *bdd_add_xnor(bdd_manager *mgr, bdd_node **fn1, bdd_node **fn2 );
EXTERN bdd_node *bdd_add_times(bdd_manager *mgr, bdd_node **fn1, bdd_node **fn2);
EXTERN bdd_node *bdd_add_vector_compose(bdd_manager *mgr, bdd_node *fn, bdd_node **vector);
EXTERN bdd_node *bdd_add_residue(bdd_manager *mgr, int n, int m, int options, int top);
EXTERN bdd_node *bdd_add_nonsim_compose(bdd_manager *mgr, bdd_node *fn, bdd_node **vector);
EXTERN bdd_node *bdd_add_apply(bdd_manager *mgr, bdd_node *(*operation)(bdd_manager *, bdd_node **, bdd_node **), bdd_node *fn1, bdd_node *fn2);
EXTERN bdd_node *bdd_add_exist_abstract(bdd_manager *mgr, bdd_node *fn, bdd_node *vars);
EXTERN void bdd_recursive_deref(bdd_manager *mgr, bdd_node *f);
EXTERN void bdd_ref(bdd_node *fn);
EXTERN bdd_node *bdd_bdd_to_add(bdd_manager *mgr, bdd_node *fn);
EXTERN bdd_node *bdd_add_permute(bdd_manager *mgr, bdd_node *fn, int *permut);
EXTERN bdd_node *bdd_bdd_permute(bdd_manager *mgr, bdd_node *fn, int *permut);
EXTERN bdd_node * bdd_bdd_exist_abstract(bdd_manager *mgr, bdd_node *fn, bdd_node *cube);
/* Added by Balakrishna Kumthekar. There are potential duplicates. */

EXTERN int  bdd_equal_sup_norm (bdd_manager *mgr, bdd_node *fn, bdd_node *gn, BDD_VALUE_TYPE tolerance, int pr);
EXTERN bdd_node * bdd_read_logic_zero(bdd_manager *mgr);
EXTERN bdd_node * bdd_bdd_ith_var(bdd_manager *mgr, int i);
EXTERN bdd_node * bdd_add_divide(bdd_manager *mgr, bdd_node **fn1, bdd_node **fn2);
EXTERN bdd_node * bdd_bdd_constrain(bdd_manager *mgr, bdd_node *f, bdd_node *c);
EXTERN bdd_node * bdd_bdd_restrict(bdd_manager *mgr, bdd_node *f, bdd_node *c);
EXTERN bdd_node * bdd_add_hamming(bdd_manager *mgr, bdd_node **xVars, bdd_node **yVars, int nVars);
EXTERN bdd_node * bdd_add_ite(bdd_manager *mgr, bdd_node *f, bdd_node *g, bdd_node *h);
EXTERN bdd_node * bdd_add_find_max(bdd_manager *mgr, bdd_node *f);
EXTERN int bdd_bdd_pick_one_cube(bdd_manager *mgr, bdd_node *node, char *string);
EXTERN bdd_node * bdd_add_swap_variables(bdd_manager *mgr, bdd_node *f, bdd_node **x, bdd_node **y, int n);
EXTERN bdd_node * bdd_bdd_or(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_bdd_compute_cube(bdd_manager *mgr, bdd_node **vars, int *phase, int n);
EXTERN bdd_node * bdd_indices_to_cube(bdd_manager *mgr, int *idArray, int n);
EXTERN bdd_node * bdd_bdd_and(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_add_matrix_multiply(bdd_manager *mgr, bdd_node *A, bdd_node *B, bdd_node **z, int nz);
EXTERN bdd_node * bdd_add_compute_cube(bdd_manager *mgr, bdd_node **vars, int *phase, int n);
EXTERN bdd_node * bdd_add_const(bdd_manager *mgr, BDD_VALUE_TYPE c);
EXTERN bdd_node * bdd_bdd_swap_variables(bdd_manager *mgr, bdd_node *f, bdd_node **x, bdd_node **y, int n);
EXTERN double bdd_count_minterm(bdd_manager *mgr, bdd_node *f, int n);
EXTERN bdd_node * bdd_add_bdd_threshold(bdd_manager *mgr, bdd_node *f, BDD_VALUE_TYPE value);
EXTERN bdd_node * bdd_add_bdd_strict_threshold(bdd_manager *mgr,bdd_node *f,BDD_VALUE_TYPE value);
EXTERN BDD_VALUE_TYPE bdd_read_epsilon(bdd_manager *mgr); 
EXTERN bdd_node * bdd_read_one(bdd_manager *mgr);
EXTERN bdd_node * bdd_bdd_pick_one_minterm(bdd_manager *mgr, bdd_node *f, bdd_node **vars, int n);
EXTERN bdd_t * bdd_pick_one_minterm(bdd_t *f, array_t *varsArray);
EXTERN array_t * bdd_bdd_pick_arbitrary_minterms(bdd_t *f, array_t *varsArray, int n, int k);
EXTERN bdd_t * bdd_subset_with_mask_vars(bdd_t *f, array_t *varsArray, array_t *maskVarsArray);
EXTERN bdd_node * bdd_read_zero(bdd_manager *mgr);
EXTERN bdd_node * bdd_bdd_new_var(bdd_manager *mgr);
EXTERN bdd_node * bdd_bdd_and_abstract(bdd_manager *mgr, bdd_node *f, bdd_node *g, bdd_node *cube);

EXTERN int bdd_test_unate(bdd_t *f, int varId, int phase);
EXTERN array_t * bdd_find_essential(bdd_t *);
EXTERN bdd_t * bdd_find_essential_cube(bdd_t *);
EXTERN void bdd_deref(bdd_node *f);
EXTERN bdd_node * bdd_add_plus(bdd_manager *mgr, bdd_node **fn1, bdd_node **fn2);
EXTERN int bdd_read_reorderings(bdd_manager *mgr);
EXTERN int bdd_read_next_reordering(bdd_manager *mgr);
EXTERN void bdd_set_next_reordering(bdd_manager *mgr, int next);
EXTERN bdd_node * bdd_bdd_xnor(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_bdd_xor(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_bdd_vector_compose(bdd_manager *mgr, bdd_node *f, bdd_node **vector);

EXTERN bdd_node * bdd_zdd_get_node(bdd_manager *mgr, int id, bdd_node *g, bdd_node *h);
EXTERN bdd_node * bdd_zdd_product(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_zdd_product_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_zdd_union(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_zdd_union_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_zdd_weak_div(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_zdd_weak_div_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_zdd_isop_recur(bdd_manager *mgr, bdd_node *L, bdd_node *U, bdd_node **zdd_I);
EXTERN bdd_node * bdd_zdd_isop(bdd_manager *mgr, bdd_node *L, bdd_node *U, bdd_node **zdd_I);
EXTERN int bdd_zdd_get_cofactors3(bdd_manager *mgr, bdd_node *f, int v, bdd_node **f1, bdd_node **f0, bdd_node **fd);
EXTERN bdd_node * bdd_bdd_and_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_unique_inter(bdd_manager *mgr, int v, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_unique_inter_ivo(bdd_manager *mgr, int v, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_zdd_diff(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_zdd_diff_recur(bdd_manager *mgr, bdd_node *f, bdd_node *g);
EXTERN int bdd_num_zdd_vars(bdd_manager *mgr);
EXTERN bdd_node * bdd_regular(bdd_node *f);
EXTERN int bdd_is_constant(bdd_node *f);
EXTERN int bdd_is_complement(bdd_node *f);
EXTERN bdd_node * bdd_bdd_T(bdd_node *f);
EXTERN bdd_node * bdd_bdd_E(bdd_node *f);
EXTERN bdd_node * bdd_not_bdd_node(bdd_node *f);
EXTERN void bdd_recursive_deref_zdd(bdd_manager *mgr, bdd_node *f);
EXTERN int bdd_zdd_count(bdd_manager *mgr, bdd_node *f);
EXTERN int bdd_read_zdd_level(bdd_manager *mgr, int index);
EXTERN int bdd_zdd_vars_from_bdd_vars(bdd_manager *mgr, int multiplicity);
EXTERN void bdd_zdd_realign_enable(bdd_manager *mgr);
EXTERN void bdd_zdd_realign_disable(bdd_manager *mgr);
EXTERN int bdd_zdd_realignment_enabled(bdd_manager *mgr);
EXTERN void bdd_realign_enable(bdd_manager *mgr);
EXTERN void bdd_realign_disable(bdd_manager *mgr);
EXTERN int bdd_realignment_enabled(bdd_manager *mgr);
EXTERN int bdd_node_read_index(bdd_node *f);
EXTERN bdd_node * bdd_read_next(bdd_node *f);
/* This function should not be used by an external user. This will not be a
 * part of any future releases.
 */
EXTERN void bdd_set_next(bdd_node *f, bdd_node *g);
EXTERN bdd_node * bdd_add_apply_recur(bdd_manager *mgr, bdd_node *(*operation)(bdd_manager *, bdd_node **, bdd_node **), bdd_node *fn1, bdd_node *fn2);
EXTERN BDD_VALUE_TYPE bdd_add_value(bdd_node *f);

EXTERN bdd_node * bdd_read_plus_infinity(bdd_manager *mgr);
EXTERN bdd_node * bdd_priority_select(bdd_manager *mgr,bdd_node *R,bdd_node **x,bdd_node **y,bdd_node **z,bdd_node *Pi,int n, bdd_node *(*Pifunc)(bdd_manager *, int, bdd_node **, bdd_node **, bdd_node **));
EXTERN void bdd_set_background(bdd_manager *mgr,bdd_node *f);
EXTERN bdd_node * bdd_read_background(bdd_manager *mgr);
EXTERN bdd_node * bdd_bdd_cofactor(bdd_manager *mgr,bdd_node *f,bdd_node *g);
EXTERN bdd_node * bdd_bdd_ite(bdd_manager *mgr,bdd_node *f,bdd_node *g,bdd_node *h);
EXTERN bdd_node * bdd_add_minus(bdd_manager *mgr,bdd_node **fn1,bdd_node **fn2);
EXTERN bdd_node * bdd_dxygtdxz(bdd_manager *mgr,int N,bdd_node **x, bdd_node **y, bdd_node **z);
EXTERN bdd_node * bdd_bdd_univ_abstract(bdd_manager *mgr,bdd_node *fn,bdd_node *vars);
EXTERN bdd_node * bdd_bdd_cprojection(bdd_manager *mgr,bdd_node *R,bdd_node *Y);
EXTERN double *bdd_cof_minterm(bdd_t *);
EXTERN int bdd_var_is_dependent(bdd_t *, bdd_t *);
EXTERN int bdd_debug_check(bdd_manager *mgr);
EXTERN bdd_node * bdd_xeqy(bdd_manager *mgr, int N, bdd_node **x, bdd_node **y);
EXTERN bdd_node * bdd_add_roundoff(bdd_manager *mgr, bdd_node *f, int N);
EXTERN bdd_node * bdd_xgty(bdd_manager *mgr, int N, bdd_node **x, bdd_node **y);
EXTERN bdd_node * bdd_add_cmpl(bdd_manager *mgr, bdd_node *f);
EXTERN bdd_node * bdd_split_set(bdd_manager *mgr, bdd_node *f, bdd_node ** x, int n, double m);

/*
 * Queries about BDD Formulas
 */
EXTERN boolean bdd_equal(bdd_t *, bdd_t *);
EXTERN boolean bdd_equal_mod_care_set(bdd_t *, bdd_t *, bdd_t *);
EXTERN boolean bdd_equal_mod_care_set_array(bdd_t *, bdd_t *, array_t *);
EXTERN bdd_t *bdd_intersects(bdd_t *, bdd_t *);
EXTERN bdd_t *bdd_closest_cube(bdd_t *, bdd_t *, int *);
EXTERN boolean bdd_is_tautology(bdd_t *, boolean);
EXTERN boolean bdd_leq(bdd_t *, bdd_t *, boolean, boolean);
EXTERN boolean bdd_lequal_mod_care_set(bdd_t *, bdd_t *, boolean, boolean, bdd_t *);
EXTERN boolean bdd_lequal_mod_care_set_array(bdd_t *, bdd_t *, boolean, boolean, array_t *);
EXTERN boolean bdd_leq_array(bdd_t *, array_t *, boolean, boolean);
EXTERN double bdd_count_onset(bdd_t *, array_t *);
EXTERN int bdd_epd_count_onset(bdd_t *, array_t *, EpDouble *epd);
EXTERN int bdd_print_apa_minterm(FILE *, bdd_t *, int, int);
EXTERN int bdd_apa_compare_ratios(int, bdd_t *, bdd_t *, int, int);

EXTERN double bdd_correlation(bdd_t *, bdd_t *);
EXTERN int bdd_get_free(bdd_t *);
EXTERN bdd_manager *bdd_get_manager(bdd_t *);
EXTERN bdd_node *bdd_get_node(bdd_t *, boolean *);
EXTERN bdd_node *bdd_extract_node_as_is(bdd_t *);
EXTERN var_set_t *bdd_get_support(bdd_t *);
EXTERN int bdd_is_support_var(bdd_t *, bdd_t *);
EXTERN int bdd_is_support_var_id(bdd_t *, int);
EXTERN array_t *bdd_get_varids(array_t *);
EXTERN unsigned int bdd_num_vars(bdd_manager *);
EXTERN int bdd_read_node_count(bdd_manager *mgr);
EXTERN void bdd_print(bdd_t *);
EXTERN int bdd_print_minterm(bdd_t *);
EXTERN int bdd_print_cover(bdd_t *);
EXTERN void bdd_print_stats(bdd_manager *, FILE *);
EXTERN int bdd_set_parameters(bdd_manager *, avl_tree *valueTable, FILE *);
EXTERN int bdd_size(bdd_t *);
EXTERN int bdd_node_size(bdd_node *);
EXTERN long bdd_size_multiple(array_t *);
EXTERN boolean bdd_is_cube(bdd_t*);
EXTERN bdd_block * bdd_new_var_block(bdd_t *f, long length);
EXTERN long bdd_top_var_level(bdd_manager *manager, bdd_t *fn);
EXTERN bdd_variableId bdd_get_id_from_level(bdd_manager *, long);
EXTERN bdd_variableId bdd_top_var_id(bdd_t *);
EXTERN int bdd_get_level_from_id(bdd_manager *mgr, int id);
EXTERN int bdd_check_zero_ref(bdd_manager *mgr);
EXTERN int bdd_estimate_cofactor(bdd_t *, bdd_t *, int );
/* Reordering related stuff */
EXTERN void bdd_dynamic_reordering(bdd_manager *, bdd_reorder_type_t, bdd_reorder_verbosity_t);
EXTERN void bdd_dynamic_reordering_zdd(bdd_manager *, bdd_reorder_type_t, bdd_reorder_verbosity_t);
EXTERN int bdd_reordering_status(bdd_manager *mgr, bdd_reorder_type_t *method);
EXTERN int bdd_reordering_zdd_status(bdd_manager *mgr, bdd_reorder_type_t *method);
EXTERN void bdd_reorder(bdd_manager *);
EXTERN int bdd_shuffle_heap(bdd_manager *mgr, int *permut);
EXTERN void bdd_dynamic_reordering_disable(bdd_manager *mgr);
EXTERN void bdd_dynamic_reordering_zdd_disable(bdd_manager *mgr);
EXTERN int bdd_read_reordered_field(bdd_manager *mgr);
EXTERN int bdd_add_hook(bdd_manager *, int (*procedure)(bdd_manager *, char *, void *), bdd_hook_type_t );
EXTERN int bdd_remove_hook(bdd_manager *, int (*procedure)(bdd_manager *, char *, void *), bdd_hook_type_t );
EXTERN int bdd_enable_reordering_reporting(bdd_manager *);
EXTERN int bdd_disable_reordering_reporting(bdd_manager *);
EXTERN int bdd_enable_ordering_monitoring(bdd_manager *mgr);
EXTERN int bdd_disable_ordering_monitoring(bdd_manager *mgr);
EXTERN bdd_reorder_verbosity_t bdd_reordering_reporting(bdd_manager *);
/* This function should not be used by an external user. It will
 * not be a part of any future release.
 */
EXTERN void bdd_set_reordered_field(bdd_manager *mgr, int n);
EXTERN bdd_node *bdd_bdd_vector_support(bdd_manager *mgr,bdd_node **F,int n);
EXTERN int bdd_bdd_vector_support_size(bdd_manager *mgr,bdd_node **F, int n);
EXTERN int bdd_bdd_support_size(bdd_manager *mgr,bdd_node *F);
EXTERN bdd_node *bdd_bdd_support(bdd_manager *mgr,bdd_node *F);
EXTERN bdd_node *bdd_add_general_vector_compose(bdd_manager *mgr,bdd_node *f,bdd_node **vectorOn,bdd_node **vectorOff);
EXTERN int bdd_bdd_leq(bdd_manager *mgr,bdd_node *f,bdd_node *g);
EXTERN bdd_node *bdd_bdd_boolean_diff(bdd_manager *mgr,bdd_node *f,int x);

/*
** Generator related functions.
** These are NOT to be used directly; only indirectly in the macros.
*/
EXTERN bdd_gen_status bdd_gen_read_status(bdd_gen *gen);
EXTERN bdd_gen *bdd_first_cube(bdd_t *, array_t **);
EXTERN boolean bdd_next_cube(bdd_gen *, array_t **);
EXTERN bdd_gen *bdd_first_disjoint_cube(bdd_t *, array_t **);
EXTERN boolean bdd_next_disjoint_cube(bdd_gen *, array_t **);
EXTERN bdd_gen *bdd_first_node(bdd_t *, bdd_node **);
EXTERN boolean bdd_next_node(bdd_gen *, bdd_node **);
EXTERN int bdd_gen_free(bdd_gen *);

/* 
 * Miscellaneous
 */
EXTERN void bdd_set_gc_mode(bdd_manager *, boolean);
EXTERN bdd_external_hooks *bdd_get_external_hooks(bdd_manager *);
EXTERN bdd_t *bdd_construct_bdd_t(bdd_manager *mgr, bdd_node * fn);
EXTERN void bdd_dump_blif(bdd_manager *mgr, int nBdds, bdd_node **bdds, char **inames, char **onames, char *model, FILE *fp);
EXTERN void bdd_dump_blif_body(bdd_manager *mgr, int nBdds, bdd_node **bdds, char **inames, char **onames, FILE *fp);
EXTERN void bdd_dump_dot(bdd_manager *mgr, int nBdds, bdd_node **bdds, char **inames, char **onames, FILE *fp);
EXTERN bdd_node *bdd_make_bdd_from_zdd_cover(bdd_manager *mgr, bdd_node *node);
EXTERN bdd_node *bdd_zdd_complement(bdd_manager *mgr, bdd_node *node);
EXTERN int bdd_ptrcmp(bdd_t *f, bdd_t *g);
EXTERN int bdd_ptrhash(bdd_t *f,int size);
EXTERN long bdd_read_peak_memory(bdd_manager *mgr);
EXTERN int bdd_read_peak_live_node(bdd_manager *mgr);
EXTERN int bdd_set_pi_var(bdd_manager *mgr, int index);
EXTERN int bdd_set_ps_var(bdd_manager *mgr, int index);
EXTERN int bdd_set_ns_var(bdd_manager *mgr, int index);
EXTERN int bdd_is_pi_var(bdd_manager *mgr, int index);
EXTERN int bdd_is_ps_var(bdd_manager *mgr, int index);
EXTERN int bdd_is_ns_var(bdd_manager *mgr, int index);
EXTERN int bdd_set_pair_index(bdd_manager *mgr, int index, int pairIndex);
EXTERN int bdd_read_pair_index(bdd_manager *mgr, int index);
EXTERN int bdd_set_var_to_be_grouped(bdd_manager *mgr, int index);
EXTERN int bdd_set_var_hard_group(bdd_manager *mgr, int index);
EXTERN int bdd_reset_var_to_be_grouped(bdd_manager *mgr, int index);
EXTERN int bdd_is_var_to_be_grouped(bdd_manager *mgr, int index);
EXTERN int bdd_is_var_hard_group(bdd_manager *mgr, int index);
EXTERN int bdd_is_var_to_be_ungrouped(bdd_manager *mgr, int index);
EXTERN int bdd_set_var_to_be_ungrouped(bdd_manager *mgr, int index);
EXTERN int bdd_bind_var(bdd_manager *mgr, int index);
EXTERN int bdd_unbind_var(bdd_manager *mgr, int index);
EXTERN int bdd_is_lazy_sift(bdd_manager *mgr);
EXTERN void bdd_discard_all_var_groups(bdd_manager *mgr);
EXTERN void bdd_set_time_limit(bdd_manager *mgr, unsigned long tl);
EXTERN void bdd_unset_time_limit(bdd_manager *mgr);
EXTERN void bdd_reset_start_time(bdd_manager *mgr);
#endif 

