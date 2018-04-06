#include <stdio.h>
#include <math.h>
#include "util.h"
#include "mdd.h"

/*
 * MDD Package
 *
 * $Id: mdd_util.c,v 1.39 2009/01/18 02:24:46 fabio Exp $
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

static	bdd_t*	mddRetOnvalBdd(mdd_manager *mddMgr, int mddId);
static	bdd_t*	mddIntRetOnvalBdd(mdd_manager *mddMgr, int valNum, int low, int hi, int level, array_t *bddVarArr);
static	void	mddFreeBddArr(array_t *bddArr);
	
/************************************************************************/
#define		mddGetVarById( mgr, id )	\
    array_fetch(mvar_type, mdd_ret_mvar_list((mgr)),(id))


int
toggle(int x)
{
    if (x == 0) return 1;
    else {
	if (x == 1) return 0;
	else {
	    fail("toggle: invalid boolean value\n");
	    return -1;
	}
    }
}

int
no_bit_encode(int n)
{
    int i = 0;
    int j = 1;

    if (n < 2) return 1; /* Takes care of mv.values <= 1 */

    while (j < n) {
	j = j * 2;
	i++;
    }
    return i;
}

void
print_mvar_list(mdd_manager *mgr)
{
    mvar_type mv;
    int i;
    int no_mvar;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);

    no_mvar = array_n(mvar_list);
    printf("print_mvar_list:\n");
    printf("id\tname\tvalues\tbits\tstride\tstart_vertex\n");
    for (i=0; i<no_mvar; i++) {
        mv = array_fetch(mvar_type, mvar_list, i);
        (void) printf("%d\t%s\t%d\t%d\n", 
		mv.mvar_id, mv.name, mv.values, 
		mv.encode_length);
    }
}

void
print_strides(array_t *mvar_strides)
{
    int i, s;

    (void) printf("mvar_strides: ");
    for (i=0; i<array_n(mvar_strides); i++) {
        s = array_fetch(int, mvar_strides, i);
        (void) printf("%d ", s);
    }
    (void) printf("\n");
}

void
print_bdd_list_id(array_t *bdd_list)
{
    bdd_t *b;
    int i, is_complemented;

    (void) printf("bdd_list id's: ");
    for (i=0; i<array_n(bdd_list); i++) {
        b = array_fetch(bdd_t *, bdd_list, i);
        (void)bdd_get_node(b, &is_complemented);
        if (is_complemented) (void) printf("!");
        (void) printf("%d ", bdd_top_var_id(b));
    }
    (void) printf("\n");
}

void
print_bvar_list_id(mdd_manager *mgr)
{
    bvar_type bv;
    int i, is_complemented;
    array_t *bvar_list = mdd_ret_bvar_list(mgr);
 
    (void) printf("bvar_list id's: ");
    for (i=0; i<array_n(bvar_list); i++) {
        bv = array_fetch(bvar_type, bvar_list, i);
        (void)bdd_get_node(bv.node,&is_complemented);
        if (is_complemented) (void) printf("!");
	(void) printf("%d ", bdd_top_var_id(bv.node));
    }
    (void) printf("\n");
}

void
print_bdd(bdd_manager *mgr, bdd_t *top)
{

    int is_complemented;
    bdd_t *child, *top_uncomp;

    if (bdd_is_tautology(top,1)) {
	(void) printf("ONE ");
        return;
    }
    if (bdd_is_tautology(top,0)) {
	(void) printf("ZERO ");
        return;
    }
    (void)bdd_get_node(top, &is_complemented);
    if (is_complemented != 0) (void) printf("!");
    (void) printf("%d ", bdd_top_var_id(top));
    (void) printf("v ");
    (void) printf("< ");

    if (is_complemented) top_uncomp = bdd_not(top);
    else top_uncomp = mdd_dup(top);

    child = bdd_then(top);

    print_bdd(mgr, child);
    (void) printf("> ");

    mdd_free(child);
    child = bdd_else(top);

    print_bdd(mgr, child);
    (void) printf("^ ");
    
    mdd_free(top_uncomp);
    mdd_free(child);

    return;
}

mvar_type 
find_mvar_id(mdd_manager *mgr, unsigned short id)
{
    mvar_type mv;
    bvar_type bv;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    array_t *bvar_list = mdd_ret_bvar_list(mgr);

    if (id >= array_n(bvar_list))
    	fail("find_mvar_id: invalid parameter range for id\n");
    bv = array_fetch(bvar_type, bvar_list, id);
    if ((bv.mvar_id < 0) || (bv.mvar_id >= array_n(mvar_list)))
    	fail("find_mvar_id: bvar contains invalid mvar_id\n");
    mv = array_fetch(mvar_type, mvar_list, bv.mvar_id);
    return mv;
}

void
clear_all_marks(mdd_manager *mgr)
{
    int i, j;
    mvar_type mv;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);


    for (i=0; i<array_n(mvar_list); i++) {
	mv = array_fetch(mvar_type, mvar_list, i);
	for (j=0; j<mv.encode_length; j++)
	    mv.encoding[j] = 2;
    }
}

void
mdd_mark(
  mdd_manager *mgr,
  bdd_t *top /**** was bdd_node *bnode; --- changed by Serdar ***/,
  int phase)
{
    int i, top_id, found = 0;
    int bit_position = 0; /* initialize for lint */
    mvar_type mv;

    top_id = bdd_top_var_id(top);
    mv = find_mvar_id(mgr, top_id);

    for (i=0; i<(mv.encode_length); i++){
		if ( mdd_ret_bvar_id( &mv, i) == top_id ){
			bit_position = i;
			found = 1;
			break;
		};
    };
   
    
    if (found == 0)
	fail("mdd_mark: interleaving error\n");

    mv.encoding[bit_position] = phase;
    
}

void
mdd_unmark(mdd_manager *mgr, bdd_t *top)
{
    int i, top_id, found = 0;
    int bit_position = 0; /* initialize for lint */
    mvar_type mv;


    top_id = bdd_top_var_id(top);
    mv = find_mvar_id(mgr, top_id);

    for (i=0; i<mv.encode_length; i++) 
		if ( mdd_ret_bvar_id( &mv, i) == top_id ){
			bit_position = i;
			found = 1;
			break;
		};

    if (found == 0)
	fail("mdd_unmark: interleaving error\n");
    mv.encoding[bit_position] = 2;
}

mvar_type 
find_mvar(mdd_manager *mgr, char *name)
{
    int i;
    mvar_type mv;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);

    for (i=0; i<array_n(mvar_list); i++) {
	mv = array_fetch(mvar_type, mvar_list, i);
        if (strcmp(mv.name, name) == 0) return mv;
    }
    fail("find_mvar: cannot find name in mvar_list\n");
    return mv;
}

array_t *
mdd_ret_mvar_list(mdd_manager *mgr)
{
    bdd_external_hooks *hook;    
    array_t *mvar_list;

    hook =  bdd_get_external_hooks(mgr);
    mvar_list = ((mdd_hook_type *)(hook->mdd))->mvar_list;

    return mvar_list;
}

void
mdd_set_mvar_list(mdd_manager *mgr, array_t *mvar_list)
{
    bdd_external_hooks *hook;    

    hook =  bdd_get_external_hooks(mgr);
    ((mdd_hook_type *)(hook->mdd))->mvar_list = mvar_list;
}


array_t *
mdd_ret_bvar_list(mdd_manager *mgr)
{
    bdd_external_hooks *hook;    
    array_t *bvar_list;

    hook =  bdd_get_external_hooks(mgr);
    bvar_list = ((mdd_hook_type *)(hook->mdd))->bvar_list;

    return bvar_list;
}


void
mdd_set_bvar_list(mdd_manager *mgr, array_t *bvar_list)
{
    bdd_external_hooks *hook;    

    hook =  bdd_get_external_hooks(mgr);
    ((mdd_hook_type *)(hook->mdd))->bvar_list = bvar_list;
}


int
mdd_ret_bvar_id(mvar_type *mvar_ptr, int i)
{
	
	return ( array_fetch(int, mvar_ptr->bvars, i) );
}

bvar_type
mdd_ret_bvar(mvar_type *mvar_ptr, int i, array_t *bvar_list)
{
	int bvar_id;
	
	bvar_id = array_fetch(int, mvar_ptr->bvars, i);
	
	return array_fetch(bvar_type, bvar_list, bvar_id);
}

/************************************************************************/
/* Given an Mdd, returns the num of onset points.  By construction of	*/
/* Mdd's, some points not in the range of Mdd vars may be included 	*/
/* in the onset. These fake points must first be removed.		*/
/************************************************************************/

double
mdd_count_onset(
  mdd_manager	*mddMgr,
  mdd_t		*aMdd,
  array_t	*mddIdArr)
{
	bdd_t		*onvalBdd, *aOnvalBdd, *onsetBdd, *tmpBdd;
	double		onsetNum;
	array_t		*bddVarArr;
	int		i, arrSize, mddId;

	arrSize = array_n( mddIdArr );
	onvalBdd = bdd_one( mddMgr );

	for ( i = 0 ; i < arrSize ; i++ ) {
	    mddId = array_fetch( int, mddIdArr, i );
	    aOnvalBdd = mddRetOnvalBdd( mddMgr, mddId );

	    tmpBdd = bdd_and( onvalBdd, aOnvalBdd, 1, 1 );
	    bdd_free( onvalBdd );
	    bdd_free( aOnvalBdd );
	    onvalBdd = tmpBdd;
	}
	onsetBdd = bdd_and( onvalBdd, aMdd, 1, 1 );
	bdd_free( onvalBdd );

	bddVarArr = mdd_id_array_to_bdd_array( mddMgr, mddIdArr );
	onsetNum = bdd_count_onset( onsetBdd, bddVarArr );
	bdd_free( onsetBdd );
	mddFreeBddArr( bddVarArr );
	return( onsetNum );
}		/* mdd_count_onset */

mdd_t *
mdd_onset_bdd(
  mdd_manager	*mddMgr,
  mdd_t		*aMdd,
  array_t	*mddIdArr)
{
	bdd_t		*onvalBdd, *aOnvalBdd, *onsetBdd, *tmpBdd;
	int		i, arrSize, mddId;

	arrSize = array_n( mddIdArr );
	onvalBdd = bdd_one( mddMgr );

	for ( i = 0 ; i < arrSize ; i++ ) {
	    mddId = array_fetch( int, mddIdArr, i );
	    aOnvalBdd = mddRetOnvalBdd( mddMgr, mddId );

	    tmpBdd = bdd_and( onvalBdd, aOnvalBdd, 1, 1 );
	    bdd_free( onvalBdd );
	    bdd_free( aOnvalBdd );
	    onvalBdd = tmpBdd;
	}
	onsetBdd = bdd_and( onvalBdd, aMdd, 1, 1 );
	bdd_free( onvalBdd );
	return( onsetBdd );
}		/* mdd_onset_bdd */

int
mdd_epd_count_onset(
  mdd_manager	*mddMgr,
  mdd_t		*aMdd,
  array_t	*mddIdArr,
  EpDouble	*epd)
{
	bdd_t		*onvalBdd, *aOnvalBdd, *onsetBdd, *tmpBdd;
	array_t		*bddVarArr;
	int		i, arrSize, mddId;
	int		status;

	arrSize = array_n( mddIdArr );
	onvalBdd = bdd_one( mddMgr );

	for ( i = 0 ; i < arrSize ; i++ ) {
	    mddId = array_fetch( int, mddIdArr, i );
	    aOnvalBdd = mddRetOnvalBdd( mddMgr, mddId );

	    tmpBdd = bdd_and( onvalBdd, aOnvalBdd, 1, 1 );
	    bdd_free( onvalBdd );
	    bdd_free( aOnvalBdd );
	    onvalBdd = tmpBdd;
	}
	onsetBdd = bdd_and( onvalBdd, aMdd, 1, 1 );
	bdd_free( onvalBdd );

	bddVarArr = mdd_id_array_to_bdd_array( mddMgr, mddIdArr );
	status = bdd_epd_count_onset( onsetBdd, bddVarArr, epd );
	if (status)
	    return(status);
	bdd_free( onsetBdd );
	mddFreeBddArr( bddVarArr );
	return(0);
}		/* mdd_epd_count_onset */

/************************************************************************/
static	bdd_t*
mddRetOnvalBdd(
  mdd_manager	*mddMgr,
  int		mddId)
{
	bdd_t		*onvalBdd;
	mvar_type	mVar;
	int		valNum, high;
	array_t		*bddVarArr;	
	
	mVar = mddGetVarById( mddMgr, mddId );
	valNum = mVar.values;
	high = (int) pow( (double) 2, (double) mVar.encode_length ); 
	assert( (valNum == 1)  || ( (valNum <= high) && (valNum > high/2) ));
	if ( valNum == high )
	    onvalBdd = bdd_one( mddMgr );
	else {
	    bddVarArr = mdd_id_to_bdd_array( mddMgr, mddId );
	    onvalBdd = mddIntRetOnvalBdd( mddMgr, valNum, 0, high, 
					  0, bddVarArr );
	    mddFreeBddArr( bddVarArr );
	}
	return( onvalBdd );
}		/* mddRetOnvalBdd */	

/************************************************************************/
static	bdd_t*
mddIntRetOnvalBdd(
  mdd_manager *mddMgr,
  int valNum,
  int low,
  int hi,
  int level,
  array_t *bddVarArr)
{
	int		mid;
	bdd_t		*curVar, *recBdd;
	bdd_t		*onvalBdd = NIL(bdd_t); /* initialized for lint */

	mid = (low + hi) / 2;
	curVar = array_fetch( bdd_t *, bddVarArr, level );

	if 	( valNum > mid ) {
	    recBdd = mddIntRetOnvalBdd( mddMgr, valNum, mid, hi, 
					level+1, bddVarArr );
	    onvalBdd = bdd_or( recBdd, curVar, 1, 0 );
	    bdd_free( recBdd );
	}
	else if ( valNum < mid ) {
	    recBdd = mddIntRetOnvalBdd( mddMgr, valNum, low, mid, 
					level+1, bddVarArr );
	    onvalBdd = bdd_and( recBdd, curVar, 1, 0 );
	    bdd_free( recBdd );
	}
	else if ( valNum == mid ) 
	    onvalBdd = bdd_not( curVar );
	return( onvalBdd );
}		/* mddIntRetOnvalBdd */

/************************************************************************/
/* Given an array of bdd nodes, frees the array.			*/

static void
mddFreeBddArr(array_t *bddArr)
{
	int	i, arrSize;

	arrSize = array_n( bddArr );
	for ( i = 0 ; i < arrSize ; i++ ) 
	    bdd_free( array_fetch( bdd_t *, bddArr, i ) );
	array_free( bddArr );
}		/* mddFreeBddArr */

array_t  *
mdd_ret_bvars_of_mvar(mvar_type *mvar_ptr)
{
	return mvar_ptr->bvars;
}

/************************************************************************/
/* mdd_get_care_set returns the care set of the mdd manager */ 

static mdd_t *mdd_get_care_set(mdd_manager *mdd_mgr)
{
    mdd_t *temp;
    mvar_type mv;
    mdd_manager *bdd_mgr;

    int mvar_id,i,j,val_j,value;
    array_t *mvar_list;
    bdd_t *care_set, *care_val, *care_cube,*bit_j;
    
    mvar_list = mdd_ret_mvar_list(mdd_mgr);
    bdd_mgr = mdd_mgr;
    
    care_set = bdd_one(bdd_mgr);
    
    for (mvar_id =0; mvar_id < array_n(mvar_list); mvar_id++)
        {
            mv = array_fetch(mvar_type, mvar_list, mvar_id);
            care_val = bdd_zero(bdd_mgr);
                
            for (i=0; i< (mv.values); i++)
                {
                    value = i;
                    care_cube = bdd_one(bdd_mgr);
                    for(j=0; j< mv.encode_length; j++ )
                        {
                            bit_j = bdd_get_variable(bdd_mgr,mdd_ret_bvar_id(&mv, j));
                            val_j = value % 2;
                            value = value/2;
                            temp = care_cube;
                            care_cube = bdd_and(temp,bit_j,1,val_j);
                            bdd_free(temp);
                        }
                    temp = care_val;
                    care_val = bdd_or(temp,care_cube,1,1);
                    bdd_free(temp);
                    bdd_free(care_cube);
                }
            temp = care_set;
            care_set = bdd_and(temp,care_val,1,1);
            bdd_free(care_val);
            bdd_free(temp);
        }
    return care_set;
}

/* Corrected mdd_cproject */
/* returns only valid carepoints */

mdd_t *mdd_cproject(
  mdd_manager *mgr,
  mdd_t *T,
  array_t *mvars)
{
    mdd_t *care_set, *new_T, *T_proj;
     array_t *bdd_vars;
    int i, j, mv_no;
    mvar_type mv;
    bdd_t *temp;
    array_t *mvar_list = mdd_ret_mvar_list(mgr);


    care_set = mdd_get_care_set(mgr);
    new_T = bdd_and(T,care_set,1,1);
    bdd_free(care_set);

      if ( mvars == NIL(array_t) ) {
        T_proj = bdd_dup(T);
        printf("\nWARNING: Empty Array of Smoothing Variables\n");
        return T_proj;
    }
    else if ( array_n(mvars) == 0) {
        T_proj = bdd_dup(T);
        printf("\nWARNING: Empty Array of Smoothing Variables\n");
        return T_proj;
    }
                
        
    bdd_vars = array_alloc(bdd_t*, 0);     
    for (i=0; i<array_n(mvars); i++) {
        mv_no = array_fetch(int, mvars, i);
        mv = array_fetch(mvar_type, mvar_list, mv_no);
        if (mv.status == MDD_BUNDLED) {
            (void) fprintf(stderr, 
                        "\nmdd_smooth: bundled variable %s used\n",mv.name);
            fail("");
        }

        for (j = 0;j < mv.encode_length; j ++) {
            temp = bdd_get_variable(mgr, mdd_ret_bvar_id(&mv,j) );
            array_insert_last(bdd_t *, bdd_vars, temp);
        }
    }
        
  
    T_proj = bdd_cproject(new_T,bdd_vars);
    bdd_free(new_T);

    for (i=0; i<array_n(bdd_vars); i++) {
        temp = array_fetch(bdd_t *, bdd_vars, i);
        bdd_free(temp);
    }
    array_free(bdd_vars);

    
    return T_proj;
}

void
mdd_print_support(mdd_t *f)
{
    mdd_manager *mgr = bdd_get_manager(f);
    array_t *support_list = mdd_get_support(mgr, f);
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    int nSupports = array_n(support_list);
    int i, j;
    mvar_type mv;
    int id;

    for (i = 0; i < nSupports; i++) {
	id = array_fetch(int, support_list, i);
	mv = array_fetch(mvar_type, mvar_list, id);
	if (id == mv.mvar_id)
	    printf("[%d] = %s\n", i, mv.name);
	else { /* needs to be checked */
	    for (j = 0; j < array_n(mvar_list); j++) {
		mv = array_fetch(mvar_type, mvar_list, j);
		if (id == mv.mvar_id) {
		    printf(" [%d] = %s\n", i, mv.name);
		    break;
		}
	    }
	}
    }

    array_free(support_list);
}

void
mdd_print_support_to_file(FILE *fout, char *format, mdd_t *f)
{
    mdd_manager *mgr = bdd_get_manager(f);
    array_t *support_list = mdd_get_support(mgr, f);
    array_t *mvar_list = mdd_ret_mvar_list(mgr);
    int nSupports = array_n(support_list);
    int i, j;
    mvar_type mv;
    int id;

    for (i = 0; i < nSupports; i++) {
	id = array_fetch(int, support_list, i);
	mv = array_fetch(mvar_type, mvar_list, id);
	if (id == mv.mvar_id)
	    fprintf(fout, format, mv.name);
	else { /* needs to be checked */
	    for (j = 0; j < array_n(mvar_list); j++) {
		mv = array_fetch(mvar_type, mvar_list, j);
		if (id == mv.mvar_id) {
		    fprintf(fout, format, mv.name);
		    break;
		}
	    }
	}
    }

    array_free(support_list);
}

char *
mdd_read_var_name(mdd_t *f)
{
    mdd_manager *mgr;
    array_t *support_list;
    array_t *mvar_list;
    int i, id;
    mvar_type mv;

    if (bdd_size(f) != 2) {
	fprintf(stderr,
	  "** mdd error: mdd_read_var_name can be called for a variable\n");
	return(NIL(char));
    }

    mgr = bdd_get_manager(f);
    support_list = mdd_get_support(mgr, f);
    mvar_list = mdd_ret_mvar_list(mgr);

    id = array_fetch(int, support_list, 0);
    mv = array_fetch(mvar_type, mvar_list, id);
    if (id == mv.mvar_id) {
	array_free(support_list);
	return(mv.name);
    } else { /* needs to be checked */
	for (i = 0; i < array_n(mvar_list); i++) {
	    mv = array_fetch(mvar_type, mvar_list, i);
	    if (id == mv.mvar_id) {
		array_free(support_list);
		return(mv.name);
	    }
	}
    }

    array_free(support_list);
    return(NIL(char));
}

int
mdd_read_mdd_id(mdd_t *f)
{
    mdd_manager *mgr;
    array_t *support_list;
    int id;

    if (bdd_size(f) != 2) {
	fprintf(stderr,
	  "** mdd error: mdd_read_mdd_id can be called for a variable\n");
	return(0);
    }

    mgr = bdd_get_manager(f);
    support_list = mdd_get_support(mgr, f);
    id = array_fetch(int, support_list, 0);
    array_free(support_list);
    return(id);
}

/**Function********************************************************************

  Synopsis    [Returns an array of BDD ids corresponding to a MDD variable.]

  Description [This function takes an MddId. It returns an array of BDD ids
  corresponding to the bits.]

  SideEffects []

******************************************************************************/
array_t *
mdd_id_to_bdd_id_array(mdd_manager *mddManager, int mddId)
{
  array_t     *bddIdArray;
  mvar_type   mddVar;
  array_t     *mvar_list;
  int         i, j;

  mvar_list = mdd_ret_mvar_list(mddManager);
  mddVar = array_fetch(mvar_type, mvar_list, mddId);
  bddIdArray = array_alloc(int, mddVar.encode_length);
  
  for (i=0; i<mddVar.encode_length; i++){
    j = mdd_ret_bvar_id(&mddVar, i);
    array_insert_last(int, bddIdArray, j);
  }
  return bddIdArray;
}

/**Function********************************************************************

  Synopsis    [Returns an array of Bdd_t's corresponding to a Mdd variable.]

  Description [This function takes an MddId. It returns an array of bdd_t's
  corresponding to the bits.]

  SideEffects []

******************************************************************************/
array_t *
mdd_id_to_bdd_array(mdd_manager *mddManager, int mddId)
{
  array_t	*bddArray;
  mvar_type	mddVar;
  int		i, id;
  
  mddVar = mddGetVarById(mddManager, mddId);
  bddArray = array_alloc(bdd_t*, mddVar.encode_length);
  
  for (i = 0; i < mddVar.encode_length; i++) {
    id = mdd_ret_bvar_id(&mddVar, i);
    array_insert_last(bdd_t*, bddArray, bdd_get_variable(mddManager, id));
  }
  return bddArray;
}


/**Function********************************************************************

  Synopsis    [Returns an array of binary vars(bdd_t *) for a given mdd
  id array.]

  Description []

  SideEffects []

******************************************************************************/
array_t *
mdd_id_array_to_bdd_array(mdd_manager *mddManager, array_t *mddIdArray)
{
  array_t	*bddArray;
  int		i, j;
  int		id, size;
  mvar_type	mddVar;

  bddArray = array_alloc(bdd_t*, 0);
  size = array_n(mddIdArray);

  for (i = 0; i < size; i++) {
    id = array_fetch(int, mddIdArray, i);
    mddVar = mddGetVarById(mddManager, id);
    for (j = 0; j < mddVar.encode_length; j++) {
      id = mdd_ret_bvar_id(&mddVar, j);
      array_insert_last(bdd_t *, bddArray, bdd_get_variable(mddManager, id));
    }
  }
  return bddArray;
}

/**Function********************************************************************

  Synopsis    [Returns an array of bddId's corresponding to an array of Mdd
  ids.] 

  Description [This function takes an array of MddId's. For each MddId it
  returns an array of bddId's corresponding to the bits. These arrays of bddId's
  are concatenated together and returned.]

  SideEffects []

******************************************************************************/
array_t *
mdd_id_array_to_bdd_id_array(mdd_manager *mddManager, array_t *mddIdArray)
{
  array_t *bddIdArray;
  int i;

  bddIdArray = array_alloc(int, 0);
  for (i=0; i<array_n(mddIdArray); i++){
    int mddId;
    array_t *tmpBddIdArray;
    mddId = array_fetch(int, mddIdArray, i);
    tmpBddIdArray = mdd_id_to_bdd_id_array(mddManager, mddId);
    array_append(bddIdArray, tmpBddIdArray);
    array_free(tmpBddIdArray);
  }
  return bddIdArray;
}


/**Function********************************************************************

  Synopsis    [Returns a bdd cube from a given mdd id array.]

  Description []

  SideEffects []

******************************************************************************/
mdd_t *
mdd_id_array_to_bdd_cube(mdd_manager *mddManager, array_t *mddIdArray)
{
  int		i, j;
  int		id, size;
  mvar_type	mddVar;
  mdd_t		*cube, *var, *tmp;
  int		nVars;
  char		*vars;

  size = array_n(mddIdArray);
  nVars = bdd_num_vars(mddManager);
  vars = ALLOC(char, sizeof(char) * nVars);
  memset(vars, 0, sizeof(char) * nVars);

  for (i = 0; i < size; i++) {
    id = array_fetch(int, mddIdArray, i);
    mddVar = mddGetVarById(mddManager, id);
    for (j = 0; j < mddVar.encode_length; j++) {
      id = mdd_ret_bvar_id(&mddVar, j);
      vars[bdd_get_level_from_id(mddManager, id)] = 1;
    }
  }
  cube = mdd_one(mddManager);
  for (i = nVars - 1; i >= 0; i--) {
    if (vars[i] == 0)
      continue;
    id = (int)bdd_get_id_from_level(mddManager, (long)i);
    var = bdd_get_variable(mddManager, id);
    tmp = mdd_and(cube, var, 1, 1);
    mdd_free(cube);
    mdd_free(var);
    cube = tmp;
  }
  FREE(vars);
  return cube;
}

/**Function********************************************************************

  Synopsis    [Returns the number of bdd variables from mdd id array.]

  Description [Returns the number of bdd variables from mdd id array.]

  SideEffects []

******************************************************************************/
int
mdd_get_number_of_bdd_vars(mdd_manager *mddManager, array_t *mddIdArray)
{
  int i, n;

  n = 0;
  for (i=0; i<array_n(mddIdArray); i++){
    int mddId;
    array_t *tmpBddIdArray;
    mddId = array_fetch(int, mddIdArray, i);
    tmpBddIdArray = mdd_id_to_bdd_id_array(mddManager, mddId);
    n += array_n(tmpBddIdArray);
    array_free(tmpBddIdArray);
  }
  return n;
}

/**Function********************************************************************

  Synopsis    [Returns the number of bdd support of a mdd.]

  Description [Returns the number of bdd support of a mdd.]

  SideEffects []

******************************************************************************/
int
mdd_get_number_of_bdd_support(mdd_manager *mddManager, mdd_t *f)
{
    array_t *bvar_list = mdd_ret_bvar_list(mddManager);
    var_set_t *vset;
    int i, number = 0;

    vset = bdd_get_support(f);
    for (i = 0; i < array_n(bvar_list); i++) {
	if (var_set_get_elt(vset, i) == 1) {
	    number++;
	}
    }

    (void) var_set_free(vset);
    return number;
}

/**Function********************************************************************

  Synopsis [Given an Mvf representing the functionality of a multi-valued
  variable, it returns an array of Bdd's representing the characteristic
  function of the relation of the various bits of the multi-valued variable.] 

  Description [Suppose y is a k-valued variable and takes values
              0,1,..,k-1. Then the input to this function is an array with k
              Mdds each representing the onset of the respective value of the
              variable (the ith Mdd representing the onset when y takes the
              value (i-1). Suppose m bits are needed to encode the k values of
              y. Then internally y is represented as y_0, y_1, ...,
              y_(m-1). Now the functionality of each bit of y can be computed
              by proper boolean operation on the functions representing the
              onsets of various values of y. For instance if y is a 4-valued
              variable. To achieve that we do the following:
              For each bit b{
                relation = 0;
                For each value j of the variable{
                  Ej = Encoding function of the jth value
                  Fj = Onset function of the jth value
                  If (b appears in the positive phase in Ej) then
                     relation += b * Fj 
                  else if (b appears in the negative phase in Ej) then
                     relation += b'* Fj
                  else if (b does not appear in Ej) then
                     relation += Fj
                }
              }
              Note that the above algorithm does not handle the case when a
              bit appears in both phases in the encoding of any value of the
              variable. Hence the assumption behind the above algorithm is that
              the values are encoded as cubes.
              The case when the encoding are functions can be handled by more
              complex algorithm. In that case, we will not be able to build the
              relation for each bit separately. Something to be dealt with in
              the later work.
              ]              
  SideEffects []

******************************************************************************/
array_t *
mdd_fn_array_to_bdd_rel_array(
  mdd_manager *mddManager,
  int mddId,
  array_t *mddFnArray)
{
  array_t *bddRelationArray, *mddLiteralArray, *valueArray, *bddArray;
  mvar_type mddVar;
  int i, j, numValues, numEncodingBits;
  bdd_t *bdd, *bddRelation, *bddNot;
  bdd_t *mddFn, *posCofactor, *negCofactor, *tmpBddRelation;
  mdd_t *mddLiteral, *literalRelation;
  array_t *mvar_list;
  
  numValues = array_n(mddFnArray);
  /* simple binary case */
  if (numValues == 2) {
    bdd_t *onRelation, *offRelation;

    bddArray = mdd_id_to_bdd_array(mddManager, mddId);
    bdd = array_fetch(bdd_t *, bddArray, 0);
    array_free(bddArray);
    mddFn = array_fetch(mdd_t *, mddFnArray, 0);
    offRelation = bdd_and(bdd, mddFn, 0, 1);
    mddFn = array_fetch(mdd_t *, mddFnArray, 1);
    onRelation = bdd_and(bdd, mddFn, 1, 1);
    bdd_free(bdd);
    bddRelation = bdd_or(onRelation, offRelation, 1, 1);
    bdd_free(onRelation);
    bdd_free(offRelation);
    bddRelationArray = array_alloc(bdd_t *, 0);
    array_insert_last(bdd_t *, bddRelationArray, bddRelation);
    return bddRelationArray;
  }
  mvar_list = mdd_ret_mvar_list(mddManager);
  mddVar = array_fetch(mvar_type, mvar_list, mddId);
  assert(mddVar.values == numValues);

  /*
   * The following is to check whether each encoding is cube or not.
   * Since Berkeley MDD package always does the cube encoding this checking has
   * been turned off currently.
   */
  
  valueArray = array_alloc(int, 1);
  mddLiteralArray = array_alloc(mdd_t*, 0);
  for (i=0; i<numValues; i++){
    array_insert(int, valueArray, 0, i);
    /* Form the Mdd corresponding to this value */
    mddLiteral = mdd_literal(mddManager, mddId, valueArray);
    /* Check if this is a cube */
    if (bdd_is_cube(mddLiteral) == FALSE){ 
      fprintf(stderr,
	"The encoding of the variable %s for the value %d isnot a cube.\n",
	mddVar.name, i); 
      fprintf(stderr, "It can result in wrong answers.\n");
    } 
    array_insert_last(mdd_t*, mddLiteralArray, mddLiteral);
  }
  array_free(valueArray);

  bddRelationArray = array_alloc(bdd_t*, 0);
  numEncodingBits = mddVar.encode_length;
  bddArray = mdd_id_to_bdd_array(mddManager, mddId);
  for (i=0; i<numEncodingBits; i++) {
    bddRelation = bdd_zero((bdd_manager *)mddManager);
    bdd = array_fetch(bdd_t*, bddArray, i);
    bddNot = bdd_not(bdd);
    for (j=0; j<numValues; j++){
      mddLiteral = array_fetch(mdd_t*, mddLiteralArray, j);
      mddFn = array_fetch(mdd_t*, mddFnArray, j);
      posCofactor = bdd_cofactor(mddLiteral, bdd);
      if (bdd_is_tautology(posCofactor, 0)) {
	literalRelation = bdd_and(bddNot, mddFn, 1, 1);
	bdd_free(posCofactor);
      } else {
	negCofactor = bdd_cofactor(mddLiteral, bddNot);
	if (bdd_is_tautology(negCofactor, 0)) {
	  literalRelation = bdd_and(bdd, mddFn, 1, 1);
        } else {
	  assert(bdd_equal(posCofactor, negCofactor));
	  literalRelation = bdd_dup(mddFn);
	}
	bdd_free(posCofactor);
	bdd_free(negCofactor);
      }
      tmpBddRelation = bdd_or(bddRelation, literalRelation, 1, 1);
      bdd_free(literalRelation);
      bdd_free(bddRelation);
      bddRelation = tmpBddRelation;
    }
    array_insert_last(bdd_t*, bddRelationArray, bddRelation);
    bdd_free(bdd);
    bdd_free(bddNot);
  }
  /* Free stuff */
  mdd_array_free(mddLiteralArray);
  array_free(bddArray);
  return bddRelationArray;
}

/**Function********************************************************************

  Synopsis [Given an Mvf representing the functionality of a multi-valued
  variable, it returns an array of Bdd's representing the characteristic
  function of the relation of the various bits of the multi-valued variable.] 

  Description [Suppose y is a k-valued variable and takes values
              0,1,..,k-1. Then the input to this function is an array with k
              Mdds each representing the onset of the respective value of the
              variable (the ith Mdd representing the onset when y takes the
              value (i-1). Suppose m bits are needed to encode the k values of
              y. Then internally y is represented as y_0, y_1, ...,
              y_(m-1). Now the functionality of each bit of y can be computed
              by proper boolean operation on the functions representing the
              onsets of various values of y. For instance if y is a 4-valued
              variable. To achieve that we do the following:
              For each bit b{
                relation = 0;
                For each value j of the variable{
                  Ej = Encoding function of the jth value
                  Fj = Onset function of the jth value
                  If (b appears in the positive phase in Ej) then
                     relation += b * Fj 
                  else if (b appears in the negative phase in Ej) then
                     relation += b'* Fj
                  else if (b does not appear in Ej) then
                     relation += Fj
                }
              }
              Note that the above algorithm does not handle the case when a
              bit appears in both phases in the encoding of any value of the
              variable. Hence the assumption behind the above algorithm is that
              the values are encoded as cubes.
              The case when the encoding are functions can be handled by more
              complex algorithm. In that case, we will not be able to build the
              relation for each bit separately. Something to be dealt with in
              the later work.
              ]              
  SideEffects []

******************************************************************************/
array_t *
mdd_fn_array_to_bdd_fn_array(
  mdd_manager *mddManager,
  int mddId,
  array_t *mddFnArray)
{
  array_t *bddFunctionArray, *mddLiteralArray, *valueArray, *bddArray;
  mvar_type mddVar;
  int i, j, numValues, numEncodingBits;
  bdd_t *bdd, *bddFunction, *bddNot;
  bdd_t *onSet, *offSet, *dcSet, *lower, *upper;
  bdd_t *mddFn, *posCofactor, *negCofactor, *tmp;
  mdd_t *mddLiteral;
  array_t *mvar_list;
  
  numValues = array_n(mddFnArray);
  /* simple binary case */
  if (numValues == 2) {
    bddFunctionArray = array_alloc(bdd_t *, 0);
    mddFn = array_fetch(mdd_t *, mddFnArray, 1);
    bddFunction = mdd_dup(mddFn);
    array_insert_last(bdd_t *, bddFunctionArray, bddFunction);
    return bddFunctionArray;
  }
  mvar_list = mdd_ret_mvar_list(mddManager);
  mddVar = array_fetch(mvar_type, mvar_list, mddId);
  assert(mddVar.values == numValues);

  /*
   * The following is to check whether each encoding is cube or not.
   * Since Berkeley MDD package always does the cube encoding this checking has
   * been turned off currently.
   */
  
  valueArray = array_alloc(int, 1);
  mddLiteralArray = array_alloc(mdd_t*, 0);
  for (i=0; i<numValues; i++){
    array_insert(int, valueArray, 0, i);
    /* Form the Mdd corresponding to this value */
    mddLiteral = mdd_literal(mddManager, mddId, valueArray);
    /* Check if this is a cube */
    if (bdd_is_cube(mddLiteral) == FALSE) {
      fprintf(stderr,
	"The encoding of the variable %s for the value %d isnot a cube.\n",
	mddVar.name, i); 
      fprintf(stderr, "It can result in wrong answers.\n");
    } 
    array_insert_last(mdd_t*, mddLiteralArray, mddLiteral);
  }
  array_free(valueArray);

  bddFunctionArray = array_alloc(bdd_t*, 0);
  numEncodingBits = mddVar.encode_length;
  bddArray = mdd_id_to_bdd_array(mddManager, mddId);
  for (i=0; i<numEncodingBits; i++) {
    onSet = bdd_zero((bdd_manager *)mddManager);
    offSet = bdd_zero((bdd_manager *)mddManager);
    dcSet = bdd_zero((bdd_manager *)mddManager);
    bdd = array_fetch(bdd_t*, bddArray, i);
    bddNot = bdd_not(bdd);
    for (j=0; j<numValues; j++) {
      mddLiteral = array_fetch(mdd_t*, mddLiteralArray, j);
      posCofactor = bdd_cofactor(mddLiteral, bdd);
      mddFn = array_fetch(mdd_t*, mddFnArray, j);

      if (bdd_is_tautology(posCofactor, 0)) {
	tmp = bdd_or(offSet, mddFn, 1, 1);
	bdd_free(offSet);
	offSet = tmp;
	bdd_free(posCofactor);
	continue;
      }

      negCofactor = bdd_cofactor(mddLiteral, bddNot);
      if (bdd_is_tautology(negCofactor, 0)) {
	tmp = bdd_or(onSet, mddFn, 1, 1);
	bdd_free(onSet);
	onSet = tmp;
	bdd_free(posCofactor);
	bdd_free(negCofactor);
	continue;
      }

      assert(bdd_equal(posCofactor, negCofactor));
      bdd_free(posCofactor);
      bdd_free(negCofactor);

      tmp = bdd_or(dcSet, mddFn, 1, 1);
      bdd_free(dcSet);
      dcSet = tmp;
    }
    bdd_free(bdd);
    bdd_free(bddNot);
    lower = bdd_and(onSet, offSet, 1, 0);
    bdd_free(offSet);
    upper = bdd_or(onSet, dcSet, 1, 1);
    bdd_free(onSet);
    bdd_free(dcSet);
    bddFunction = bdd_between(lower, upper);
    bdd_free(lower);
    bdd_free(upper);
    array_insert_last(bdd_t*, bddFunctionArray, bddFunction);
  }
  /* Free stuff */
  mdd_array_free(mddLiteralArray);
  array_free(bddArray);
  return bddFunctionArray;
}


array_t *
mdd_pick_arbitrary_minterms(
  mdd_manager	*mddMgr,
  mdd_t		*aMdd,
  array_t	*mddIdArr,
  int		n)
{
    bdd_t	*onvalBdd, *aOnvalBdd, *onsetBdd, *tmpBdd;
    array_t	*bddVarArr;
    int		i, arrSize, mddId;
    array_t	*mintermArray;

    arrSize = array_n( mddIdArr );
    onvalBdd = bdd_one( mddMgr );

    for ( i = 0 ; i < arrSize ; i++ ) {
	mddId = array_fetch( int, mddIdArr, i );
	aOnvalBdd = mddRetOnvalBdd( mddMgr, mddId );

	tmpBdd = bdd_and( onvalBdd, aOnvalBdd, 1, 1 );
	bdd_free( onvalBdd );
	bdd_free( aOnvalBdd );
	onvalBdd = tmpBdd;
    }
    onsetBdd = bdd_and( onvalBdd, aMdd, 1, 1 );
    bdd_free( onvalBdd );

    bddVarArr = mdd_id_array_to_bdd_array(mddMgr, mddIdArr);
    mintermArray = bdd_bdd_pick_arbitrary_minterms(onsetBdd, bddVarArr,
	array_n(bddVarArr), n);
    bdd_free(onsetBdd);
    mddFreeBddArr(bddVarArr);
    return(mintermArray);
}


mdd_t *
mdd_subset_with_mask_vars(
  mdd_manager	*mddMgr,
  mdd_t		*aMdd,
  array_t	*mddIdArr,
  array_t	*maskIdArr)
{
    bdd_t	*onvalBdd, *aOnvalBdd, *onsetBdd, *tmpBdd;
    array_t	*bddVarArr, *maskVarArr;
    int		i, arrSize, mddId;
    mdd_t	*subset;

    arrSize = array_n( mddIdArr );
    onvalBdd = bdd_one( mddMgr );

    for ( i = 0 ; i < arrSize ; i++ ) {
	mddId = array_fetch( int, mddIdArr, i );
	aOnvalBdd = mddRetOnvalBdd( mddMgr, mddId );

	tmpBdd = bdd_and( onvalBdd, aOnvalBdd, 1, 1 );
	bdd_free( onvalBdd );
	bdd_free( aOnvalBdd );
	onvalBdd = tmpBdd;
    }
    onsetBdd = bdd_and( onvalBdd, aMdd, 1, 1 );
    bdd_free( onvalBdd );

    bddVarArr = mdd_id_array_to_bdd_array(mddMgr, mddIdArr);
    maskVarArr = mdd_id_array_to_bdd_array(mddMgr, maskIdArr);

    subset = bdd_subset_with_mask_vars(onsetBdd, bddVarArr, maskVarArr);
    bdd_free(onsetBdd);
    mddFreeBddArr(bddVarArr);
    mddFreeBddArr(maskVarArr);
    return(subset);
}


/* Internal macro to access the mvar_type structure for each MDD variable. */
mvar_type
mdd_get_var_by_id(mdd_manager *mddMgr, int id)
{
    return(mddGetVarById(mddMgr, id));
}


/* checks whether all support variables of mdd appear in supportIdArray. */
int
mdd_check_support(
  mdd_manager	*mddMgr,
  mdd_t		*mdd,
  array_t	*supportIdArray)
{
  int		i, mddId;
  st_table	*supportTable = st_init_table(st_numcmp, st_numhash);
  array_t	*tmpIdArray;
  int		allSupportFlag = 1;

  for (i = 0; i < array_n(supportIdArray); i++) {
    mddId = array_fetch(int, supportIdArray, i);
    st_insert(supportTable, (char *)(long)mddId, NULL);
  }

  tmpIdArray = mdd_get_support(mddMgr, mdd);
  for (i = 0; i < array_n(tmpIdArray); i++) {
    mddId = array_fetch(int, tmpIdArray, i);
    if (!st_lookup(supportTable, (char *)(long)mddId, NULL)) {
      allSupportFlag = 0;
      break;
    }
  }

  st_free_table(supportTable);
  array_free(tmpIdArray);
  return(allSupportFlag);
}


boolean
mdd_equal_mod_care_set_array(mdd_t *aSet, mdd_t *bSet, array_t *careSetArray)
{
  mdd_t	*tmpMdd1, *tmpMdd2;
  mdd_t	*careSet;
  int i;
  boolean result;

  if (mdd_equal(aSet, bSet))
    return 1;

  arrayForEachItem(mdd_t *, careSetArray, i, careSet) {
    tmpMdd1 = mdd_and(aSet, careSet, 1, 1);
    tmpMdd2 = mdd_and(bSet, careSet, 1, 1);

    result = mdd_equal(tmpMdd1, tmpMdd2);
    mdd_free(tmpMdd1);
    mdd_free(tmpMdd2);
    if (result == 1)
      return 1;
  }

  return 0;
}


boolean
mdd_lequal_mod_care_set_array(mdd_t *aSet, mdd_t *bSet,
			      boolean aPhase, boolean bPhase,
			      array_t *careSetArray)
{
  mdd_t	*tmpMdd, *careSet;
  int	i, result;

  if (mdd_lequal(aSet, bSet, aPhase, bPhase))
    return 1;

  arrayForEachItem(mdd_t *, careSetArray, i, careSet) {
    tmpMdd = mdd_and(aSet, careSet, aPhase, 1);

    result = mdd_lequal(tmpMdd, bSet, 1, bPhase);
    mdd_free(tmpMdd);
    if (result == 1)
      return 1;
  }

  return 0;
}


/* Return the mdd that represents all valid assignments to the
   variables in the support.  Support is an array of mdd_ids. */
mdd_t *
mdd_range_mdd(
  mdd_manager *mgr,
  array_t *support
  )
{
  int var, varIndex;      /* iterates over support */
  mdd_t *range;

  range = mdd_one(mgr);
  arrayForEachItem(int, support, varIndex, var){
    mdd_t *rangeForVar;
    mdd_t *tmp;

    rangeForVar = mddRetOnvalBdd(mgr, var);
    tmp = mdd_and( rangeForVar, range, 1, 1);
    mdd_free(rangeForVar);
    mdd_free(range);
    range = tmp;
  }
  
  return range;
}

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


