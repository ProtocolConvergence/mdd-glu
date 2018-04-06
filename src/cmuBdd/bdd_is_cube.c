#include "bddint.h"
/*
 * Recursively determine if f is a cube. f is a cube if there is a single
 * path to the constant one.
 */
int
cmu_bdd_is_cube(struct bdd_manager_ *manager, struct bdd_ *f)
{
  struct bdd_ *f0, *f1;

  BDD_SETUP(f);
  if (BDD_IS_CONST(f)){
    if (f == BDD_ZERO(manager)){
      cmu_bdd_fatal("cmu_bdd_is_cube called with 0");
    }
    else return 1;
  }
  BDD_COFACTOR(BDD_INDEXINDEX(f), f, f1, f0);
  
  /*
   * Exactly one branch of f must point to ZERO to be a cube.
   */
  if (f1 == BDD_ZERO(manager)) {
	return (cmu_bdd_is_cube(manager, f0));
  } else if (f0 == BDD_ZERO(manager)) {
	return (cmu_bdd_is_cube(manager, f1));
  } else { /* not a cube, because neither branch is zero */
	return 0;
  }
}

