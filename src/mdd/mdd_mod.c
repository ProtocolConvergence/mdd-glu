/*
 * $Id: mdd_mod.c,v 1.5 2002/08/27 16:30:26 fabio Exp $
 *
 */

#include "mdd.h"

static void mod_block_build (mdd_t ****, int, bvar_type *, bvar_type *);
static void assert_zero (mdd_manager *, mdd_t **, bvar_type *);
static void single_var_block_build (mdd_t ****, bvar_type *, int);

int
getbit(int number, int position)
{
return ( (number >> position) & 1);
}

static void
assert_zero(
  mdd_manager *mgr,
  mdd_t **PINS_00_ptr,
  bvar_type *a_bit_i_ptr)
{
	mdd_t *zero;

	zero = mdd_zero(mgr);

	(*PINS_00_ptr) = mdd_ite( a_bit_i_ptr->node,  zero, (*PINS_00_ptr) , 1, 1, 1);		

	mdd_free(zero);

	return;
}

static void
single_var_block_build(
  mdd_t ****PINS_ptr,
  bvar_type *b_bit_i_ptr,
  int M)
{
	int i;

	for (i=0; i < M; i++)
		(*PINS_ptr)[0][i] = mdd_ite( b_bit_i_ptr->node, (*PINS_ptr)[0][(2*i+1)%M], (*PINS_ptr)[0][(2*i)%M], 1, 1, 1);

}

/* a = b (mod M) */
static void
mod_block_build(
  mdd_t ****PINS_ptr,
  int M /* modulus */,
  bvar_type *a_bit_i_ptr,
  bvar_type *b_bit_i_ptr)  
{
	int i, j;
	mdd_t *THEN, *ELSE;

	for(i=0; i<M; i++)
		for (j=0; j<M; j++){
			THEN = mdd_ite(b_bit_i_ptr->node, (*PINS_ptr)[(2*i+1)%M][(2*j+1)%M], 
							   (*PINS_ptr)[(2*i+1)%M][(2*j)%M],   1, 1, 1);
			
			ELSE = mdd_ite(b_bit_i_ptr->node, (*PINS_ptr)[(2*i)%M][(2*j+1)%M], 
							   (*PINS_ptr)[(2*i)%M][(2*j)%M],     1, 1, 1);
		
			(*PINS_ptr)[i][j] = mdd_ite(a_bit_i_ptr->node, THEN, ELSE, 1, 1, 1);
		}			
}

/* a = b (mod M) */
mdd_t *
mdd_mod(
  mdd_manager *mgr,
  int a_mvar_id,
  int b_mvar_id,
  int M /* Modulus */ )
{
	mdd_t ***PINS; 		/* Two dimensional array of mdd_t*'s used in forming the mdd */
	int i, j;
	mvar_type a, b;
	array_t *mvar_list = mdd_ret_mvar_list(mgr);
 	array_t *bvar_list = mdd_ret_bvar_list(mgr);
	bvar_type a_bit_i, b_bit_i;
	mdd_t *result, *less_than;

	a = array_fetch ( mvar_type, mvar_list, a_mvar_id);
	b = array_fetch ( mvar_type, mvar_list, b_mvar_id);

	/* Allocate two dimensional array PINS[0..M-1][0..M-1] of mdd_t *'s */
	PINS = ALLOC(mdd_t **, M);		
	for(i=0; i<M; i++) PINS[i] = ALLOC(mdd_t *, M);
	
	for(i=0; i< M; i++) 
		for(j=0; j< M; j++) PINS[i][j] = mdd_zero(mgr);

	for(i=0; i< M; i++) PINS[i][i] = mdd_one(mgr);

	for( i = 1 ; i <= (MIN(a.encode_length, b.encode_length)); i++){ 
		a_bit_i = mdd_ret_bvar( &a, (a.encode_length - i), bvar_list);
		b_bit_i = mdd_ret_bvar( &b, (b.encode_length - i), bvar_list);

		mod_block_build(&PINS, M,  &a_bit_i, &b_bit_i);	 
	}

	if ( a.encode_length > b.encode_length ) 
		for ( i = MIN(a.encode_length, b.encode_length)+1; i <= a.encode_length; i++){
			a_bit_i = mdd_ret_bvar( &a, (a.encode_length - i), bvar_list);
			assert_zero( mgr, &(PINS[0][0]), &a_bit_i);
		}

	if ( b.encode_length > a.encode_length )
		for ( i = MIN(a.encode_length, b.encode_length)+1; i <= b.encode_length; i++){
			b_bit_i = mdd_ret_bvar( &b, (b.encode_length - i), bvar_list);
			single_var_block_build( &PINS, &b_bit_i, M);
		}

	less_than = build_lt_c(mgr, a_mvar_id, M);

	result = mdd_and(PINS[0][0], less_than, 1, 1);
		
	return result;	


}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

