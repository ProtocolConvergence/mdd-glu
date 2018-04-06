/*
 * $Id: mdd_bund.c,v 1.5 2002/08/27 03:24:30 fabio Exp $
 */

#include "mdd.h"

int
mdd_bundle_variables(
  mdd_manager *mgr,
  array_t *bundle_vars,
  char *mdd_var_name,
  int *mdd_id)
{
	array_t *mvar_list, *bvar_list;
	mvar_type var_i, new_var;
	int i, var_i_id;
	bvar_type *bit_i_ptr;
	
	mvar_list = mdd_ret_mvar_list(mgr);
	bvar_list = mdd_ret_bvar_list(mgr);

	new_var.mvar_id = array_n(mvar_list);
	*mdd_id = new_var.mvar_id;

	new_var.name = ALLOC( char, MAX( (int) strlen(mdd_var_name),
					 integer_get_num_of_digits(new_var.mvar_id) + 5 ) );

	if ( strcmp(mdd_var_name,"") != 0)  
		strcpy(new_var.name, mdd_var_name);
	else {
		strcpy(new_var.name,"");
		sprintf(new_var.name,"mv_%d", new_var.mvar_id);
	};

	new_var.encode_length = array_n(bundle_vars);
	new_var.status = MDD_ACTIVE;

	new_var.encoding = ALLOC(int, new_var.encode_length);
	
	new_var.bvars = array_alloc(int, 0);
	new_var.values = 1;

	for(i=0; i<array_n(bundle_vars); i++){
		var_i_id = array_fetch(int, bundle_vars, i);
		var_i = array_fetch(mvar_type, mvar_list, var_i_id);
		array_append(new_var.bvars, var_i.bvars);
		new_var.values *= var_i.values;
		if ( var_i.values != (int) pow(2.0, (double)var_i.encode_length) ) 
			printf("WARNING: Variable %s has %d values which is not a power of 2 \nmdd_bundle_variables: Bundling is ambiguous \n",var_i.name, var_i.values);
		var_i.status = MDD_BUNDLED;
	}

	array_insert_last( mvar_type, mvar_list, new_var);

	for(i=0; i< array_n(new_var.bvars); i++) {
		bit_i_ptr = array_fetch_p( bvar_type, bvar_list, mdd_ret_bvar_id(&new_var, i) );
		bit_i_ptr->mvar_id = new_var.mvar_id;
	}

	return TRUE;

}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


