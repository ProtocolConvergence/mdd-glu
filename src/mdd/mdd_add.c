#include "mdd.h"

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

EXTERN void addition_block_build(mdd_manager *mgr, mdd_t **A, mdd_t **B, bvar_type *bx_ptr, bvar_type *by_ptr, bvar_type *bz_ptr);
EXTERN void one_var_and_carry_add_block(mdd_manager *mgr, mdd_t **A, mdd_t **B, bvar_type *bz_ptr, bvar_type *blv_ptr);


void
addition_block_build(
  mdd_manager *mgr,
  mdd_t **A,
  mdd_t **B,
  bvar_type *bx_ptr,
  bvar_type *by_ptr,
  bvar_type *bz_ptr)
{
	mdd_t *zero;
	mdd_t 	*C, *D, *E, *F,
		*G, *H, *I;

	zero = mdd_zero(mgr);	
		
	G = mdd_ite( by_ptr->node, zero , *A  ,  1, 1, 1 );
	H = mdd_ite( by_ptr->node, *A   , *B  ,  1, 1, 1 );
	I = mdd_ite( by_ptr->node, *B   , zero,  1, 1, 1 );

	C = mdd_ite( bx_ptr->node, zero, G   ,  1, 1, 1 );
	D = mdd_ite( bx_ptr->node, G   , H   ,  1, 1, 1 );
	E = mdd_ite( bx_ptr->node, H   , I   ,  1, 1, 1 );
	F = mdd_ite( bx_ptr->node, I   , zero,  1, 1, 1 );
		
	*A = mdd_ite( bz_ptr->node, D   , C   ,  1, 1, 1 );
	*B = mdd_ite( bz_ptr->node, F   , E   ,  1, 1, 1 );

	mdd_free(G);
	mdd_free(H);
	mdd_free(I);
	mdd_free(C);
	mdd_free(D);
	mdd_free(E);
	mdd_free(F);
	mdd_free(zero);

	return;
}


void
one_var_and_carry_add_block(
  mdd_manager *mgr,
  mdd_t **A,
  mdd_t **B,
  bvar_type *bz_ptr,
  bvar_type *blv_ptr)
{
	mdd_t *C, *D, *E;
	mdd_t *zero;

	zero = mdd_zero(mgr);	

	C = mdd_ite( blv_ptr->node,  zero,   *A, 1, 1, 1);
	D = mdd_ite( blv_ptr->node,    *A,   *B, 1, 1, 1);
	E = mdd_ite( blv_ptr->node,    *B, zero, 1, 1, 1);

	*A = mdd_ite( bz_ptr->node,    D, C, 1, 1, 1);
	*B = mdd_ite( bz_ptr->node, zero, E, 1, 1, 1);

	mdd_free(C);
	mdd_free(D);
	mdd_free(E);

	mdd_free(zero);

	return;
}

/* in1 + in2 = sum */
mdd_t *
mdd_add_s(
  mdd_manager *mgr,
  int sum_id,
  int mvar_id1,
  int mvar_id2)
{
	mdd_t *one, *zero;
	array_t *mvar_list = mdd_ret_mvar_list(mgr);
 	array_t *bvar_list = mdd_ret_bvar_list(mgr);
	int     config, 
	    	no_common_to_all_bits, 
	   	no_common_in_bits, i;

	bvar_type bx, by, bz, z_carry, blv;
	mvar_type x, y, z, long_var;
	mdd_t 	*A, *B, *range_check;
	mdd_t *result = NIL(mdd_t); /* initialize for lint */
        

	
	x = array_fetch(mvar_type, mvar_list, mvar_id1);
	y = array_fetch(mvar_type, mvar_list, mvar_id2);
	z = array_fetch(mvar_type, mvar_list, sum_id);

	/* Ensures that the i_th bit of x has smaller index
	   than the i_th bit of y */

	if  ( ( mdd_ret_bvar_id(&x,x.encode_length) ) >
              ( mdd_ret_bvar_id(&y,y.encode_length) )  )  {
			y = array_fetch(mvar_type, mvar_list, mvar_id1);
			x = array_fetch(mvar_type, mvar_list, mvar_id2);
			}

	one = mdd_one(mgr);
	zero = mdd_zero(mgr);	
	
	no_common_in_bits = MIN(x.encode_length,y.encode_length);
	no_common_to_all_bits = MIN(no_common_in_bits, z.encode_length);

	A = mdd_dup(one);
	B = mdd_dup(zero);

	for (i = 1; i <= no_common_to_all_bits; i++){

		bx = mdd_ret_bvar(&x,x.encode_length-i,bvar_list); 
		by = mdd_ret_bvar(&y,y.encode_length-i,bvar_list);
		bz = mdd_ret_bvar(&z,z.encode_length-i,bvar_list);
		addition_block_build( mgr, &A, &B, &bx, &by, &bz);
	}


	if ( z.encode_length > no_common_to_all_bits ){
		if ( x.encode_length != y.encode_length ){
			if ( x.encode_length == no_common_in_bits ){ 
				long_var = y;   
				/* short_var = x; */
			}
			else { 
				long_var = x;   
				/* short_var = y; */
			}
		
			if ( z.encode_length > long_var.encode_length) {
			config = 1;	 
			}
			else
			{
			config = 2;		
			}
		}
		else 
			config = 3;			
	}
	else 
		config = 4;			


	switch (config) {
		
		case 1: /* z > long_var , short_var */

			for ( i = no_common_to_all_bits + 1; i <= long_var.encode_length; i++){

				z_carry = mdd_ret_bvar(&z,z.encode_length-i,bvar_list);
				blv = mdd_ret_bvar(&long_var,long_var.encode_length-i,bvar_list);

				one_var_and_carry_add_block(mgr, &A, &B, &bz, &blv);
			}
			
			z_carry = mdd_ret_bvar(&z,z.encode_length - long_var.encode_length - 1, bvar_list);

			A = mdd_ite( z_carry.node, B, A, 1, 1, 1);

			for ( i = long_var.encode_length + 2; i <= z.encode_length; i++){
				z_carry = mdd_ret_bvar(&z,z.encode_length-i, bvar_list);
				A = mdd_ite( z_carry.node, zero, A, 1, 1, 1);				
			}
			result = mdd_dup(A);
			break;

		case 2: /* short_var < z < long_var */

			for ( i = no_common_to_all_bits + 1; i <= z.encode_length; i++){

				z_carry = mdd_ret_bvar(&z,z.encode_length-i,bvar_list);
				blv = mdd_ret_bvar(&long_var,long_var.encode_length-i,bvar_list);

				one_var_and_carry_add_block(mgr, &A, &B, &bz, &blv);

			}

			result = mdd_or(A,B,1,1);
			break;

		case 3: /* z> long_var = short_var */ 

			z_carry = mdd_ret_bvar(&z,z.encode_length - no_common_to_all_bits - 1,bvar_list);
			A = mdd_ite( z_carry.node, B, A, 1, 1, 1);

			for ( i = no_common_to_all_bits + 2; i <= z.encode_length; i++){
				z_carry = mdd_ret_bvar(&z, z.encode_length - i, bvar_list);
				A = mdd_ite( z_carry.node, zero, A, 1, 1, 1);
			}	

			result = mdd_dup(A);
			break;

		case 4: /* z <= long_var, z <= short_var */
			result = mdd_or(A,B,1,1);
			break;
	}
	
	mdd_free(A);
	mdd_free(B);


	mdd_free(one);
	mdd_free(zero);
	
	range_check = build_lt_c(mgr, sum_id, z.values);
	result = mdd_and(result, range_check, 1, 1);

	mdd_free(range_check);

	return result;
}


