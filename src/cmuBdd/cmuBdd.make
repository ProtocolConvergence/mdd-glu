CSRC_cmu += bdd.c bdd_is_cube.c bddapply.c bddassoc.c bddblk.c bddcache.c bddcmp.c bddcomp.c bddcproject.c bdddump.c bddhash.c bddmisc.c bddprimes.c bddprint.c bddprprofile.c bddqnt.c bddreduce.c bddrelprod.c bddreorder.c bddsat.c bddsize.c bddsupport.c bddswap.c bddunique.c bddwarn.c mtbdd.c 
HEADERS_cmu += bddint.h bdduser.h 
MISC += testbdd.c bdd.3 

DEPENDENCYFILES = $(CSRC_cmu)
