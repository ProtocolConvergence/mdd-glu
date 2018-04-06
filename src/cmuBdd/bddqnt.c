/* BDD quantification routines */


#include "bddint.h"


static
bdd
cmu_bdd_exists_step(cmu_bdd_manager bddm, bdd f, long op, var_assoc vars)
{
  bdd temp1, temp2;
  bdd result;
  int quantifying;

  BDD_SETUP(f);
  if ((long)BDD_INDEX(bddm, f) > vars->last)
    {
      BDD_TEMP_INCREFS(f);
      return (f);
    }
  if (bdd_lookup_in_cache1(bddm, op, f, &result))
    return (result);
  quantifying=(vars->assoc[BDD_INDEXINDEX(f)] != 0);
  temp1=cmu_bdd_exists_step(bddm, BDD_THEN(f), op, vars);
  if (quantifying && temp1 == BDD_ONE(bddm))
    result=temp1;
  else
    {
      temp2=cmu_bdd_exists_step(bddm, BDD_ELSE(f), op, vars);
      if (quantifying)
	{
	  BDD_SETUP(temp1);
	  BDD_SETUP(temp2);
	  bddm->op_cache.cache_level++;
	  result=cmu_bdd_ite_step(bddm, temp1, BDD_ONE(bddm), temp2);
	  BDD_TEMP_DECREFS(temp1);
	  BDD_TEMP_DECREFS(temp2);
	  bddm->op_cache.cache_level--;
	}
      else
	result=bdd_find(bddm, BDD_INDEXINDEX(f), temp1, temp2);
    }
  bdd_insert_in_cache1(bddm, op, f, result);
  return (result);
}


/* cmu_bdd_exists_temp is used internally by cmu_bdd_rel_prod. */

bdd
cmu_bdd_exists_temp(cmu_bdd_manager bddm, bdd f, long op)
{
  if (bddm->curr_assoc_id != -1)
    op=OP_QNT+bddm->curr_assoc_id;
  return (cmu_bdd_exists_step(bddm, f, op, bddm->curr_assoc));
}


/* cmu_bdd_exists(bddm, f) returns the BDD for existentially quantifying */
/* out in f all variables which are associated with something in the */
/* current variable association. */

bdd
cmu_bdd_exists(cmu_bdd_manager bddm, bdd f)
{
  long op;

  if (bdd_check_arguments(1, f))
    {
      FIREWALL(bddm);
      if (bddm->curr_assoc_id == -1)
	op=bddm->temp_op--;
      else
	op=OP_QNT+bddm->curr_assoc_id;
      RETURN_BDD(cmu_bdd_exists_step(bddm, f, op, bddm->curr_assoc));
    }
  return ((bdd)0);
}


/* cmu_bdd_forall(bddm, f) returns the BDD for universally quantifying */
/* out in f all variables which are associated with something in the */
/* current variable association. */

bdd
cmu_bdd_forall(cmu_bdd_manager bddm, bdd f)
{
  bdd temp;

  if ((temp=cmu_bdd_exists(bddm, BDD_NOT(f))))
    return (BDD_NOT(temp));
  return ((bdd)0);
}
