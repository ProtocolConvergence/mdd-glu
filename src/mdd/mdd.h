/*
 * $Id: mdd.h,v 1.34 2012/02/04 23:54:58 fabio Exp $
 */

#ifndef MDD_DEFINED
#define MDD_DEFINED


#include "util.h"
#include "array.h"
#include "st.h"
#include "var_set.h"

#include "bdd.h" 

/*    #include "bdd_int.h"   */

/************************************************************************/
#define str_len         10
#define MONITOR         0
#define MDD_VERBOSE     0
#define SOLUTION        0
#define BDD_SIZE        1
#define USE_ITE         1
#define BYPASS          1
/* code may not be fully debugged for interleaving */
#define INTERLEAVE      0

#define mdd_and bdd_and
#define mdd_and_with_limit bdd_and_with_limit
#define mdd_and_array bdd_and_array
#define mdd_cofactor_minterm bdd_cofactor
#define mdd_constant bdd_constant
#define mdd_dup bdd_dup
/* mdd_equal returns 1 if two MDD's are identical */
/* mdd_eq is a totally different function */
/* which returns an MDD instead */
#define mdd_equal bdd_equal
#define mdd_equal_mod_care_set bdd_equal_mod_care_set
#define mdd_closest_cube bdd_closest_cube
#define mdd_free bdd_free
#define mdd_get_manager bdd_get_manager
#define mdd_is_tautology bdd_is_tautology
#define mdd_ite bdd_ite
/* mdd_lequal checks for implication, */
/* mdd_lequal(f,g,1,0) returns the value of (f => g') */
/* mdd_leq is a totally different function */
#define mdd_lequal bdd_leq
#define mdd_lequal_mod_care_set bdd_lequal_mod_care_set
#define mdd_lequal_array bdd_leq_array
#define mdd_multiway_and bdd_multiway_and
#define mdd_multiway_or bdd_multiway_or
#define mdd_multiway_xor bdd_multiway_xor
#define mdd_not bdd_not
#define mdd_one bdd_one
#define mdd_or bdd_or
#define mdd_size bdd_size
#define mdd_size_multiple bdd_size_multiple
#define mdd_top_var_id bdd_top_var_id
#define mdd_xor bdd_xor
#define mdd_xnor bdd_xnor
#define mdd_zero bdd_zero
#define mdd_EMPTY bdd_EMPTY

#define mdd_first_solution mdd_first_cube
#define mdd_next_solution mdd_next_cube

#define mdd_eq_c(m,a,b)  	mdd_func1c(m,a,b,eq2)
#define mdd_geq_c(m,a,b) 	mdd_func1c(m,a,b,geq2)
#define mdd_gt_c(m,a,b)  	mdd_func1c(m,a,b,gt2)
#define mdd_leq_c(m,a,b) 	mdd_func1c(m,a,b,leq2)
#define mdd_lt_c(m,a,b)  	mdd_func1c(m,a,b,lt2)
#define mdd_neq_c(m,a,b) 	mdd_func1c(m,a,b,neq2)

#define mdd_eq(m,a,b)  		mdd_func2(m,a,b,eq2)
#define mdd_geq(m,a,b) 		mdd_func2(m,a,b,geq2)
#define mdd_gt(m,a,b)  		mdd_func2(m,a,b,gt2)
#define mdd_leq(m,a,b) 		mdd_func2(m,a,b,leq2)
#define mdd_lt(m,a,b)  		mdd_func2(m,a,b,lt2)
#define mdd_neq(m,a,b) 		mdd_func2(m,a,b,neq2)
#define mdd_unary_minus(m,a,b) 	mdd_func2(m,a,b,unary_minus2)

#define mdd_eq_plus(m,a,b,c)	mdd_func3(m,a,b,c,eq_plus3)
#define mdd_geq_plus(m,a,b,c)	mdd_func3(m,a,b,c,geq_plus3)
#define mdd_gt_plus(m,a,b,c)	mdd_func3(m,a,b,c,gt_plus3)
#define mdd_leq_plus(m,a,b,c)	mdd_func3(m,a,b,c,leq_plus3)
#define mdd_lt_plus(m,a,b,c)	mdd_func3(m,a,b,c,lt_plus3)
#define mdd_neq_plus(m,a,b,c)	mdd_func3(m,a,b,c,neq_plus3)

#define mdd_eq_minus(m,a,b,c)	mdd_func3(m,a,b,c,eq_minus3)
#define mdd_geq_minus(m,a,b,c)	mdd_func3(m,a,b,c,geq_minus3)
#define mdd_gt_minus(m,a,b,c)	mdd_func3(m,a,b,c,gt_minus3)
#define mdd_leq_minus(m,a,b,c)	mdd_func3(m,a,b,c,leq_minus3)
#define mdd_lt_minus(m,a,b,c)	mdd_func3(m,a,b,c,lt_minus3)
#define mdd_neq_minus(m,a,b,c)	mdd_func3(m,a,b,c,neq_minus3)

#define mdd_eq_plus_c(m,a,b,c)	mdd_func2c(m,a,b,c,eq_plus3)
#define mdd_geq_plus_c(m,a,b,c)	mdd_func2c(m,a,b,c,geq_plus3)
#define mdd_gt_plus_c(m,a,b,c)	mdd_func2c(m,a,b,c,gt_plus3)
#define mdd_leq_plus_c(m,a,b,c)	mdd_func2c(m,a,b,c,leq_plus3)
#define mdd_lt_plus_c(m,a,b,c)	mdd_func2c(m,a,b,c,lt_plus3)
#define mdd_neq_plus_c(m,a,b,c)	mdd_func2c(m,a,b,c,neq_plus3)


#define mdd_eq_plus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,eq_plus3mod)
#define mdd_geq_plus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,geq_plus3mod)
#define mdd_gt_plus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,gt_plus3mod)
#define mdd_leq_plus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,leq_plus3mod)
#define mdd_lt_plus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,lt_plus3mod)
#define mdd_neq_plus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,neq_plus3mod)

#define mdd_eq_minus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,eq_minus3mod)
#define mdd_geq_minus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,geq_minus3mod)
#define mdd_gt_minus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,gt_minus3mod)
#define mdd_leq_minus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,leq_minus3mod)
#define mdd_lt_minus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,lt_minus3mod)
#define mdd_neq_minus_c_mod(m,a,b,c)	mdd_func2c_mod(m,a,b,c,neq_minus3mod)

#define mdd_eq_s(m,a,b) 	mdd_ineq_template_s(m,a,b,0,0,1)
#define mdd_geq_s(m,a,b) 	mdd_ineq_template_s(m,a,b,0,1,1)
#define mdd_gt_s(m,a,b)		mdd_ineq_template_s(m,a,b,0,1,0)
#define mdd_neq_s(m,a,b)	mdd_ineq_template_s(m,a,b,1,1,0)
#define mdd_leq_s(m,a,b)	mdd_ineq_template_s(m,a,b,1,0,1)
#define mdd_lt_s(m,a,b)		mdd_ineq_template_s(m,a,b,1,0,0)

/**** for backward compatibility only ****/
#define mdd_eq_g(m,a,b)  	mdd_func2(m,a,b,eq2)
#define mdd_geq_g(m,a,b) 	mdd_func2(m,a,b,geq2)
#define mdd_gt_g(m,a,b)  	mdd_func2(m,a,b,gt2)
#define mdd_leq_g(m,a,b) 	mdd_func2(m,a,b,leq2)
#define mdd_lt_g(m,a,b)  	mdd_func2(m,a,b,lt2)
#define mdd_neq_g(m,a,b) 	mdd_func2(m,a,b,neq2)
#define mdd_init_name(v,n,s) 	mdd_init(v,n,s)

/* mdd_int.h */
#define MDD_NOT(node)           ((bdd_node *) ((int) (node) ^ 01))
#define MDD_REGULAR(node)       ((bdd_node *) ((int) (node) & ~01))
#define MDD_IS_COMPLEMENT(node) ((int) (node) & 01)

#define MDD_ONE(bdd)            (bdd)->one
#define MDD_ZERO(bdd)           (MDD_NOT(MDD_ONE(bdd)))

#ifndef MAX
#define MAX(a,b) (a) > (b) ? (a) : (b)
#endif

#ifndef MIN
#define MIN(a,b) (a) < (b) ? (a) : (b)
#endif

    typedef bdd_t mdd_t;
    typedef bdd_manager mdd_manager;

typedef enum {
	MDD_ACTIVE,
	MDD_BUNDLED
} mvar_status;

struct mvar_type {
    int mvar_id;		/* mvar id */
    mvar_status status;         /* Whether the mvar is currently being used or 
                                   has been bundled into another mvar */
    char *name;			/* name of mvar */
    int values;			/* no. of values mvar can take */
    int encode_length;		/* no. of binary variables, bvar's, */
				/* needed to encode mvar */
    array_t *bvars;             /* array of bvar_id's from most significant bit to least 
				   significant bit, has size encode_length */
    int *encoding;		/* internal use only */
};
typedef struct mvar_type mvar_type;

struct bvar_type {
    mdd_t *node;
    int mvar_id;
};
typedef struct bvar_type bvar_type;

struct mdd_hook_type {
    array_t *mvar_list;
    array_t *bvar_list;	
};

typedef struct mdd_hook_type mdd_hook_type;

struct mdd_gen {
    mdd_manager *manager;
    bdd_gen *bdd_generator;
    bdd_gen_status status;
    array_t *cube;		/* array of literals {0,1,2} of all vars */
    array_t *minterm;		/* current minterm */
    array_t *var_list;		/* list of var id's */
    boolean out_of_range;
}; 

typedef struct mdd_gen mdd_gen;

#define foreach_mdd_minterm(fn, gen, minterm, var_list)\
  for((gen) = mdd_first_minterm(fn, &minterm, var_list);\
      ((gen)->status != bdd_EMPTY) ? TRUE: mdd_gen_free(gen);\
      (void) mdd_next_minterm(gen, &minterm))
                                                                      

extern mdd_hook_type mdd_hook;


/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN mdd_manager *mdd_init(array_t *mvar_values, array_t *mvar_names, array_t *mvar_strides);
EXTERN mdd_manager *mdd_init_empty(void);
EXTERN unsigned int mdd_create_variables(mdd_manager *mgr, array_t *mvar_values, array_t *mvar_names, array_t *mvar_strides);
EXTERN unsigned int mdd_create_variables_after(mdd_manager*, int, array_t*, array_t*, array_t* );
EXTERN unsigned int mdd_create_variables_interleaved(mdd_manager*, int, int, array_t*);
EXTERN void mdd_quit(mdd_manager *mgr);
EXTERN void mdd_restart(mdd_manager *mgr);
EXTERN void mdd_var_expunge_bdd_variable(mdd_manager *mgr, int bddId, int val);
EXTERN mdd_t *mdd_case(mdd_manager *mgr, int mvar, array_t *child_list);
EXTERN mdd_t *mdd_consensus(mdd_manager *mgr, mdd_t *fn, array_t *mvars);
EXTERN mdd_t *mdd_encode(mdd_manager *mgr, array_t *child_list, mvar_type *mv_ptr, int index);
EXTERN mdd_t *mdd_literal(mdd_manager *mgr, int mddid, array_t *values);
EXTERN void   mdd_search(mdd_manager *mgr, bdd_t *top, int phase, boolean minterms);
EXTERN mdd_t *mdd_cofactor(mdd_manager *mgr, mdd_t *fn, mdd_t *cube);
EXTERN mdd_t *mdd_smooth(mdd_manager *mgr, mdd_t *fn, array_t *mvars);
EXTERN mdd_t *mdd_and_smooth(mdd_manager *mgr, mdd_t *f, mdd_t *g, array_t *mvars);
EXTERN mdd_t *mdd_and_smooth_with_limit(mdd_manager *mgr, mdd_t *f, mdd_t *g, array_t *mvars, unsigned int limit);
EXTERN mdd_t *mdd_substitute(mdd_manager *mgr, mdd_t *fn, array_t	*old_mvars, array_t *new_mvars);
EXTERN array_t *mdd_substitute_array(mdd_manager *mgr, array_t *fn_array, array_t *old_mvars, array_t *new_mvars);
EXTERN array_t *mdd_get_support(mdd_manager *mdd_mgr, mdd_t *f);
EXTERN array_t *mdd_get_bdd_support_ids(mdd_manager *mdd_mgr, mdd_t *f);
EXTERN array_t *mdd_get_bdd_support_vars(mdd_manager *mdd_mgr, mdd_t *f);
EXTERN mdd_t *mdd_interval(mdd_manager *mgr, int mvar_id, int low, int high); 
EXTERN double mdd_count_onset(mdd_manager *mddMgr, mdd_t *aMdd, array_t *mddIdArr);
EXTERN mdd_t *mdd_onset_bdd(mdd_manager *mddMgr, mdd_t *aMdd, array_t *mddIdArr);
EXTERN int mdd_epd_count_onset(mdd_manager *mddMgr, mdd_t *aMdd, array_t *mddIdArr, EpDouble *epd);
EXTERN array_t  * mdd_ret_bvars_of_mvar(mvar_type *mvar_ptr);
EXTERN mdd_t *mdd_cproject(mdd_manager *mgr, mdd_t *T, array_t *mvars);
EXTERN mdd_t *mdd_mod(mdd_manager *mgr, int a_mvar_id, int b_mvar_id, int M);
EXTERN mdd_t *mdd_ineq_template_s(mdd_manager *mgr, int mvar1, int mvar2,	int zero_then_val, int one_else_val, int bottom_val);
EXTERN mdd_t *mdd_add_s(mdd_manager *mgr, int sum_id, int mvar_id1, int mvar_id2 );
/* mdd_iter.c */
EXTERN mdd_gen *mdd_first_minterm(mdd_t *f, array_t **minterm_p, array_t *var_list);
EXTERN boolean mdd_next_minterm(mdd_gen *mgen, array_t **minterm_p);
EXTERN void mdd_print_array(array_t *array);
EXTERN int mdd_gen_free(mdd_gen *mgen);
EXTERN mdd_t *mdd_func1c(mdd_manager *mgr, int mvar1, int mvar2, boolean (*func1c) (int, int));
EXTERN mdd_t *mdd_func2(mdd_manager *mgr, int mvar1, int mvar2, boolean (*func2) (int, int));
/* functions of 2 variables used by mdd_func2()) */
EXTERN boolean eq2(int x, int y);
EXTERN boolean geq2(int x, int y);
EXTERN boolean gt2(int x, int y);
EXTERN boolean leq2(int x, int y);
EXTERN boolean lt2(int x, int y);
EXTERN boolean neq2(int x, int y);
EXTERN boolean unary_minus2(int x, int y);
EXTERN mdd_t *mdd_func2c(mdd_manager *mgr,int mvar1, int mvar2, int constant, boolean (*func3)(int, int, int));
EXTERN mdd_t *mdd_func2c_mod(mdd_manager *mgr,int mvar1, int mvar2, int constant, boolean (*func4)(int, int, int, int));

/* should be in ../test */
EXTERN mdd_t *mdd_func3(mdd_manager *mgr, int mvar1, int mvar2, int mvar3, boolean (*func3)(int, int, int));
/* functions of 3 variables used by mdd_func3()) */
EXTERN boolean eq_plus3(int x, int y, int z);
EXTERN boolean geq_plus3(int x, int y, int z);
EXTERN boolean gt_plus3(int x, int y, int z);
EXTERN boolean leq_plus3(int x, int y, int z);
EXTERN boolean lt_plus3(int x, int y, int z);
EXTERN boolean neq_plus3(int x, int y, int z);
/* actually functions below can be obtained from the set above */
/* by just suppling the negation of the constant value */
EXTERN boolean eq_minus3(int x, int y, int z);
EXTERN boolean geq_minus3(int x, int y, int z);
EXTERN boolean gt_minus3(int x, int y, int z);
EXTERN boolean leq_minus3(int x, int y, int z);
EXTERN boolean lt_minus3(int x, int y, int z);
EXTERN boolean neq_minus3(int x, int y, int z);

EXTERN array_t *mdd_id_to_bdd_id_array(mdd_manager *mddManager, int mddId);
EXTERN array_t *mdd_id_to_bdd_array(mdd_manager *mddManager, int mddId);
EXTERN array_t *mdd_id_array_to_bdd_array(mdd_manager *mddManager, array_t *mddIdArray);
EXTERN array_t *mdd_id_array_to_bdd_id_array(mdd_manager *mddManager, array_t *mddIdArray);
EXTERN mdd_t *mdd_id_array_to_bdd_cube(mdd_manager *mddManager, array_t *mddIdArray);
EXTERN int mdd_get_number_of_bdd_vars(mdd_manager *mddManager, array_t *mddIdArray);
EXTERN int mdd_get_number_of_bdd_support(mdd_manager *mddManager, mdd_t *f);
EXTERN array_t *mdd_fn_array_to_bdd_rel_array(mdd_manager *mddManager, int mddId, array_t *mddFnArray);
EXTERN array_t *mdd_fn_array_to_bdd_fn_array(mdd_manager *mddManager, int mddId, array_t *mddFnArray);
EXTERN array_t *mdd_pick_arbitrary_minterms(mdd_manager *mgr, mdd_t *f, array_t *mddIdArr, int n);
EXTERN mdd_t *mdd_subset_with_mask_vars(mdd_manager *mgr, mdd_t *f, array_t *mddIdArr, array_t *maskIdArr);
EXTERN mvar_type mdd_get_var_by_id(mdd_manager *mddMgr, int id);
EXTERN void mdd_print_support(mdd_t *f);
EXTERN void mdd_print_support_to_file(FILE *fout, char *format, mdd_t *f);
EXTERN char *mdd_read_var_name(mdd_t *f);
EXTERN int mdd_read_mdd_id(mdd_t *f);
EXTERN int mdd_check_support(mdd_manager *mddMgr, mdd_t *mdd, array_t *supportIdArray);
EXTERN int mdd_equal_mod_care_set_array(mdd_t *aSet, mdd_t *bSet, array_t *CareSetArray);
EXTERN int mdd_lequal_mod_care_set_array(mdd_t *aSet, mdd_t *bSet, boolean aPhase, boolean bPhase, array_t *CareSetArray);

EXTERN boolean eq_plus3mod(int x, int y, int z, int range);
EXTERN boolean geq_plus3mod(int x, int y, int z, int range);
EXTERN boolean gt_plus3mod(int x, int y, int z, int range);
EXTERN boolean leq_plus3mod(int x, int y, int z, int range);
EXTERN boolean lt_plus3mod(int x, int y, int z, int range);
EXTERN boolean neq_plus3mod(int x, int y, int z, int range);
/* actually functions below can be obtained from the set above */
/* by just suppling the negation of the constant value */
EXTERN boolean eq_minus3mod(int x, int y, int z, int range);
EXTERN boolean geq_minus3mod(int x, int y, int z, int range);
EXTERN boolean gt_minus3mod(int x, int y, int z, int range);
EXTERN boolean leq_minus3mod(int x, int y, int z, int range);
EXTERN boolean lt_minus3mod(int x, int y, int z, int range);
EXTERN boolean neq_minus3mod(int x, int y, int z, int range);
EXTERN void mdd_array_dump_dot(array_t *mdds, char *name, FILE *fp);
EXTERN void mdd_dump_dot(mdd_t *mdd, char *name, FILE *fp);
EXTERN void mdd_array_print_cover(array_t *mdds, char *name, boolean disjoint);
EXTERN void mdd_print_cover(mdd_t *f, char *name, boolean disjoint);

/* should be in mdd_int.h */
EXTERN int toggle(int x);
EXTERN int no_bit_encode(int n);
EXTERN void print_strides(array_t *mvar_strides);
EXTERN void print_mvar_list(mdd_manager *mgr);
EXTERN void print_bdd_list_id(array_t *bdd_list);
EXTERN void print_bvar_list_id(mdd_manager *mgr);
EXTERN void print_bdd(bdd_manager *mgr, bdd_t *top);
EXTERN mvar_type find_mvar_id(mdd_manager *mgr, unsigned short id);
EXTERN void clear_all_marks(mdd_manager *mgr);
EXTERN void mdd_mark(mdd_manager *mgr, bdd_t *top, int phase);
EXTERN void mdd_unmark(mdd_manager *mgr, bdd_t *top);
EXTERN mvar_type find_mvar(mdd_manager *mgr, char *name);
EXTERN array_t *mdd_ret_mvar_list(mdd_manager *mgr);
EXTERN void mdd_set_mvar_list(mdd_manager *mgr, array_t *mvar_list);
EXTERN array_t *mdd_ret_bvar_list(mdd_manager *mgr);
EXTERN void mdd_set_bvar_list(mdd_manager *mgr, array_t *bvar_list);
EXTERN mdd_t *build_lt_c(mdd_manager *mgr, int mvar_id, int c);
EXTERN mdd_t *build_leq_c(mdd_manager *mgr, int mvar_id, int c);
EXTERN mdd_t *build_gt_c(mdd_manager *mgr, int mvar_id, int c);
EXTERN mdd_t *build_geq_c(mdd_manager *mgr, int mvar_id, int c);
EXTERN int getbit(int number, int position);
EXTERN int integer_get_num_of_digits(int value);
EXTERN int mdd_ret_bvar_id(mvar_type *mvar_ptr, int i);
EXTERN bvar_type mdd_ret_bvar(mvar_type *mvar_ptr, int i, array_t *bvar_list);
EXTERN void mdd_array_free(array_t *mddArray);
EXTERN void mdd_array_array_free(array_t *arrayBddArray);
EXTERN array_t *mdd_array_duplicate(array_t *mddArray);
EXTERN boolean mdd_array_equal(array_t *array1, array_t *array2);
EXTERN mdd_t *mdd_range_mdd(mdd_manager *mgr, array_t *support);

/* unsupported */
EXTERN int mdd_bundle_variables(mdd_manager *mgr, array_t *bundle_vars, char *mdd_var_name, int *mdd_id);
EXTERN mdd_t * mdd_unary_minus_s(mdd_manager *mgr, int mvar1, int mvar2);
EXTERN array_t * mvar2bdds(mdd_manager *mgr, array_t *mvars);
#endif
