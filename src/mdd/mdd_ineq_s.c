/*
 * $Id: mdd_ineq_s.c,v 1.4 2002/08/27 16:30:26 fabio Exp $
 *
 */

#include "mdd.h"

static int
mdd_is_care_bit(
  mvar_type mvar,
  int index)
{
    return ( getbit( ( (int) pow(2.0, (double)mvar.encode_length) ) - mvar.values, mvar.encode_length-index-1));
}


mdd_t *
mdd_ineq_template_s(
  mdd_manager *mgr,
  int mvar1,
  int mvar2,
  int zero_then_val /* J in Tim's thesis */,
  int one_else_val  /* K */,
  int bottom_val    /* A and B */)
{
    mvar_type x, y;
    bvar_type bx, by;
    mdd_t *one_top, *zero_top, *zero_then, *one_else,
          *one_top_else = mdd_one(mgr);
    mdd_t *one_top_then = mdd_one(mgr);
    mdd_t *zero_top_else = mdd_one(mgr);
    mdd_t *zero_top_then = mdd_one(mgr);
    mdd_t *compare;
    int i;

    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    array_t *bvar_list = mdd_ret_bvar_list(mgr);

    if (zero_then_val == 0) 
	zero_then = mdd_zero(mgr);
    else 
	zero_then = mdd_one(mgr);


     if (one_else_val == 0) 
	 one_else = mdd_zero(mgr);
     else 
	 one_else = mdd_one(mgr);

    if (bottom_val == 0) {
	one_top = mdd_zero(mgr);
	zero_top = mdd_zero(mgr);
    }
    else {
	one_top = mdd_one(mgr);
	zero_top = mdd_one(mgr);
    }


    x = array_fetch(mvar_type, mvar_list, mvar1);
    y = array_fetch(mvar_type, mvar_list, mvar2);

    if (x.values != y.values) 
	fail("mdd_ineq: 2 mvars have incompatible value ranges\n");

    if (x.status == MDD_BUNDLED) {
	(void) fprintf(stderr, 
		"\nWarning: mdd_ineq, bundled variable %s is used\n", x.name);
	fail("");
    }

    if (y.status == MDD_BUNDLED) {
	(void) fprintf(stderr,
		"\nWarning: mdd_ineq, bundled variable %s is used\n", y.name);
	fail("");
    }


    for (i=(x.encode_length-1); i>=0; i--) {

	bx = mdd_ret_bvar(&x, i, bvar_list);
	by = mdd_ret_bvar(&y, i, bvar_list);

	mdd_free(zero_top_else);
	zero_top_else = mdd_ite(by.node, zero_then, zero_top, 1, 1, 1);
	mdd_free(zero_top_then);
	zero_top_then = mdd_ite(by.node, zero_top,  one_else, 1, 1, 1);

	if (mdd_is_care_bit(x,i) == 0) {

		mdd_free(one_top_else);
		one_top_else = mdd_ite(by.node, zero_then, zero_top, 1, 1, 1);
		mdd_free(one_top_then);
		one_top_then = mdd_ite(by.node, one_top, one_else, 1, 1, 1);

		mdd_free(one_top);

		one_top = mdd_ite(bx.node, one_top_then, one_top_else, 1, 1, 1);
	}

	mdd_free(zero_top);
	zero_top = mdd_ite(bx.node, zero_top_then, zero_top_else, 1, 1, 1);

    }

    mdd_free(zero_then);
    mdd_free(one_else);

    mdd_free(zero_top_else);
    mdd_free(zero_top_then);
    mdd_free(one_top_else);
    mdd_free(one_top_then);

    mdd_free(zero_top);

    compare = mdd_eq(mgr, mvar1, mvar2);

    if  ( ( bdd_equal(compare, one_top) == 0) && (zero_then_val == 0) && (one_else_val == 0) && (bottom_val == 1) )
      printf("Error \n"); 

    mdd_free(compare);
    
    return one_top;
}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


