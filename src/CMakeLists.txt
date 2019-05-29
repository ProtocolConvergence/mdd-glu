cmake_minimum_required(VERSION 2.8)

project(mdd-glu)

include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckTypeSize)
include(TestBigEndian)


### Maybe these are important?
#-DPAGE_SIZE=4096
#-DLG_PAGE_SIZE2

### Make it fast.
### But also make Protocon not crash in OpenMP mode due to a race condition...
add_compile_options(
  -g -O3 -mtune=native
  -malign-double
  )
add_definitions(
  -DNDEBUG
  )

### Basic assumed stuff.
add_definitions(
  -DHAVE_ASSERT_H
  -DHAVE_STDARG_H
  -DHAVE_SYS_FILE_H
  -DHAVE_SYS_STAT_H
  -DSTDC_HEADERS

  -DHAVE_GETENV
  -DHAVE_IEEE_754
  -DHAVE_STRCHR
  -DHAVE_STRSTR
  )

### Type sizes.
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)
check_type_size("void*" SIZEOF_VOID_P)
add_definitions(
  -DSIZEOF_INT=${SIZEOF_INT}
  -DSIZEOF_LONG=${SIZEOF_LONG}
  -DSIZEOF_VOID_P=${SIZEOF_VOID_P}
  )

### Endianness (of floating point... so maybe not the best check here).
test_big_endian(EPD_BIG_ENDIAN)
if (EPD_BIG_ENDIAN)
  add_definitions(-DEPD_BIG_ENDIAN)
endif ()


### Check system-dependent includes.
function (check_include_to_define include_name definition_flag)
  check_include_file(${include_name} tmp)
  if (tmp)
    add_definitions(${definition_flag})
  endif ()
endfunction ()

check_include_to_define("stdarg.h" -DHAVE_STDARG_H)
check_include_to_define("errno.h" -DHAVE_ERRNO_H)
check_include_to_define("pwd.h" -DHAVE_PWD_H)
check_include_to_define("sys/resource.h" -DHAVE_SYS_RESOURCE_H)
check_include_to_define("sys/time.h" -DHAVE_SYS_TIME_H)
check_include_to_define("sys/times.h" -DHAVE_SYS_TIMES_H)
check_include_to_define("sys/types.h" -DHAVE_SYS_TYPES_H)
check_include_to_define("unistd.h" -DHAVE_UNISTD_H)


### Check system-dependent symbols.
function (check_symbol_to_define symbol_name include_files definition_flag)
  check_symbol_exists(${symbol_name} ${include_files} tmp)
  if (tmp)
    add_definitions(${definition_flag})
  endif ()
endfunction ()

check_symbol_to_define("valloc" "stdlib.h" -DHAVE_VALLOC)
check_symbol_to_define("getrlimit" "sys/time.h;sys/resource.h" -DHAVE_GETRLIMIT)


add_library(glu STATIC
  array/array.c
  avl/avl.c
  epd/epd.c
  error/error.c
  graph/graph.c
  graph/graph_dfs.c
  graph/graph_s.c
  heap/heap.c
  list/list.c
  mem/memblock.c
  mem/memrec.c
  mtr/mtrBasic.c
  mtr/mtrGroup.c
  sparse/cols.c
  sparse/matrix.c
  sparse/rows.c
  st/st.c
  util/cpu_stats.c
  util/cpu_time.c
  util/datalimit.c
  util/getopt.c
  util/pathsearch.c
  util/prtime.c
  util/qsort.c
  util/random.c
  util/safe_mem.c
  util/strsav.c
  util/texpand.c
  util/tmpfile.c
  var_set/var_set.c
  )
target_include_directories(glu PRIVATE
  array
  graph
  list
  st
  util
  )

add_library(mdd OBJECT
  mdd/mdd_add.c
  mdd/mdd_andsmoot.c
  mdd/mdd_bund.c
  mdd/mdd_case.c
  mdd/mdd_cofactor.c
  mdd/mdd_consensus.c
  mdd/mdd_dot.c
  mdd/mdd_func1c.c
  mdd/mdd_func2.c
  mdd/mdd_func2c.c
  mdd/mdd_func2cmod.c
  mdd/mdd_func3.c
  mdd/mdd_ineq_s.c
  mdd/mdd_init.c
  mdd/mdd_intv.c
  mdd/mdd_iter.c
  mdd/mdd_literal.c
  mdd/mdd_mod.c
  mdd/mdd_quit.c
  mdd/mdd_search.c
  mdd/mdd_smooth.c
  mdd/mdd_substit.c
  mdd/mdd_support.c
  mdd/mdd_uminus.c
  mdd/mdd_util.c
  mdd/mvar2bdds.c
  )
target_include_directories(mdd PRIVATE
  array
  avl
  epd
  bdd
  st
  util
  var_set
  )

add_library(cal STATIC
  $<TARGET_OBJECTS:mdd>
  calBdd/calApplyReduce.c
  calBdd/calAssociation.c
  calBdd/calBddCompose.c
  calBdd/calBddITE.c
  calBdd/calBddManager.c
  calBdd/calBddOp.c
  calBdd/calBddSatisfy.c
  calBdd/calBddSize.c
  calBdd/calBddSubstitute.c
  calBdd/calBddSupport.c
  calBdd/calBddSwapVars.c
  calBdd/calBddVarSubstitute.c
  calBdd/calBlk.c
  calBdd/cal.c
  calBdd/calCacheTableTwo.c
  calBdd/calDump.c
  calBdd/calGC.c
  calBdd/calHashTable.c
  calBdd/calHashTableOne.c
  calBdd/calHashTableThree.c
  calBdd/calInteract.c
  calBdd/calMem.c
  calBdd/calMemoryManagement.c
  calBdd/calPipeline.c
  calBdd/calPrint.c
  calBdd/calPrintProfile.c
  calBdd/calQuant.c
  calBdd/calReduce.c
  calBdd/calReorderBF.c
  calBdd/calReorderDF.c
  calBdd/calReorderUtil.c
  calBdd/calTerminal.c
  calBdd/calUtil.c
  calPort/calPort.c
  calPort/calPortIter.c
  )
target_include_directories(cal PRIVATE
  array
  avl
  bdd
  calBdd
  epd
  st
  util
  var_set
  )

add_library(cmu STATIC
  $<TARGET_OBJECTS:mdd>
  cmuBdd/bddapply.c
  cmuBdd/bddassoc.c
  cmuBdd/bddblk.c
  cmuBdd/bdd.c
  cmuBdd/bddcache.c
  cmuBdd/bddcmp.c
  cmuBdd/bddcomp.c
  cmuBdd/bddcproject.c
  cmuBdd/bdddump.c
  cmuBdd/bddhash.c
  cmuBdd/bdd_is_cube.c
  cmuBdd/bddmisc.c
  cmuBdd/bddprimes.c
  cmuBdd/bddprint.c
  cmuBdd/bddprprofile.c
  cmuBdd/bddqnt.c
  cmuBdd/bddreduce.c
  cmuBdd/bddrelprod.c
  cmuBdd/bddreorder.c
  cmuBdd/bddsat.c
  cmuBdd/bddsize.c
  cmuBdd/bddsupport.c
  cmuBdd/bddswap.c
  cmuBdd/bddunique.c
  cmuBdd/bddwarn.c
  cmuBdd/mtbdd.c
  cmuPort/cmuPort.c
  cmuPort/cmuPortIter.c
  )
target_include_directories(cmu PRIVATE
  array
  avl
  cmuBdd
  bdd
  epd
  mem
  st
  util
  var_set
  )

add_library(cu STATIC
  $<TARGET_OBJECTS:mdd>
  cuBdd/cuddAddAbs.c
  cuBdd/cuddAddApply.c
  cuBdd/cuddAddFind.c
  cuBdd/cuddAddInv.c
  cuBdd/cuddAddIte.c
  cuBdd/cuddAddNeg.c
  cuBdd/cuddAddWalsh.c
  cuBdd/cuddAndAbs.c
  cuBdd/cuddAnneal.c
  cuBdd/cuddApa.c
  cuBdd/cuddAPI.c
  cuBdd/cuddApprox.c
  cuBdd/cuddBddAbs.c
  cuBdd/cuddBddCorr.c
  cuBdd/cuddBddIte.c
  cuBdd/cuddBridge.c
  cuBdd/cuddCache.c
  cuBdd/cuddCheck.c
  cuBdd/cuddClip.c
  cuBdd/cuddCof.c
  cuBdd/cuddCompose.c
  cuBdd/cuddDecomp.c
  cuBdd/cuddEssent.c
  cuBdd/cuddExact.c
  cuBdd/cuddExport.c
  cuBdd/cuddGenCof.c
  cuBdd/cuddGenetic.c
  cuBdd/cuddGroup.c
  cuBdd/cuddHarwell.c
  cuBdd/cuddInit.c
  cuBdd/cuddInteract.c
  cuBdd/cuddLCache.c
  cuBdd/cuddLevelQ.c
  cuBdd/cuddLinear.c
  cuBdd/cuddLiteral.c
  cuBdd/cuddMatMult.c
  cuBdd/cuddPriority.c
  cuBdd/cuddRead.c
  cuBdd/cuddRef.c
  cuBdd/cuddReorder.c
  cuBdd/cuddSat.c
  cuBdd/cuddSign.c
  cuBdd/cuddSolve.c
  cuBdd/cuddSplit.c
  cuBdd/cuddSubsetHB.c
  cuBdd/cuddSubsetSP.c
  cuBdd/cuddSymmetry.c
  cuBdd/cuddTable.c
  cuBdd/cuddUtil.c
  cuBdd/cuddWindow.c
  cuBdd/cuddZddCount.c
  cuBdd/cuddZddFuncs.c
  cuBdd/cuddZddGroup.c
  cuBdd/cuddZddIsop.c
  cuBdd/cuddZddLin.c
  cuBdd/cuddZddMisc.c
  cuBdd/cuddZddPort.c
  cuBdd/cuddZddReord.c
  cuBdd/cuddZddSetop.c
  cuBdd/cuddZddSymm.c
  cuBdd/cuddZddUtil.c
  cuPort/cuPort.c
  cuPort/cuPortIter.c
  )
target_include_directories(cu PRIVATE
  array
  avl
  bdd
  cuBdd
  epd
  mtr
  st
  util
  var_set
  )
