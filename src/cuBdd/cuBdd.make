CSRC_cu += cuddAPI.c cuddAddAbs.c cuddAddApply.c cuddAddFind.c cuddAddIte.c \
        cuddAddInv.c cuddAddNeg.c cuddAddWalsh.c cuddAndAbs.c \
        cuddAnneal.c cuddApa.c cuddApprox.c cuddBddAbs.c cuddBddCorr.c \
	cuddBddIte.c cuddBridge.c cuddCache.c cuddCheck.c cuddClip.c \
	cuddCof.c cuddCompose.c cuddDecomp.c cuddEssent.c cuddExact.c \
	cuddExport.c cuddGenCof.c cuddGenetic.c \
        cuddGroup.c cuddHarwell.c cuddInit.c cuddInteract.c \
	cuddLCache.c cuddLevelQ.c \
        cuddLinear.c cuddLiteral.c cuddMatMult.c cuddPriority.c \
        cuddRead.c cuddRef.c cuddReorder.c cuddSat.c cuddSign.c \
        cuddSolve.c cuddSplit.c cuddSubsetHB.c cuddSubsetSP.c cuddSymmetry.c \
	cuddTable.c cuddUtil.c cuddWindow.c cuddZddCount.c cuddZddFuncs.c \
	cuddZddGroup.c cuddZddIsop.c cuddZddLin.c cuddZddMisc.c cuddZddPort.c \
	cuddZddReord.c cuddZddSetop.c cuddZddSymm.c cuddZddUtil.c 
HEADERS_cu += cudd.h cuddInt.h
MISC += testcudd.c r7x8.1.mat doc/cudd.ps doc/cuddAllAbs.html \
	doc/cuddAllDet.html doc/cuddExtAbs.html doc/cuddExtDet.html \
	doc/cuddIntro.css doc/cuddIntro.html doc/footnode.html \
	doc/img10.png doc/img11.png doc/img12.png doc/img13.png doc/img14.png \
	doc/img15.png doc/img16.png doc/img17.png doc/img18.png \
	doc/img19.png doc/img1.png doc/img20.png doc/img21.png \
	doc/img22.png doc/img2.png doc/img3.png doc/img4.png doc/img5.png \
	doc/img6.png doc/img7.png doc/img8.png doc/img9.png doc/index.html \
	doc/node1.html doc/node2.html doc/node3.html doc/node4.html \
	doc/node5.html doc/node6.html doc/node7.html doc/node8.html \
	doc/icons/blueball.png doc/icons/ch_begin.png \
	doc/icons/ch_beg_r.png doc/icons/ch_delet.png \
	doc/icons/ch_del_r.png doc/icons/ch_end.png \
	doc/icons/ch_end_r.png doc/icons/contents.png \
	doc/icons/crossref.png doc/icons/footnote.png \
	doc/icons/greenball.png doc/icons/image.png \
	doc/icons/index.png doc/icons/next_g.png \
	doc/icons/next.png doc/icons/nx_grp_g.png \
	doc/icons/nx_grp.png doc/icons/orangeball.png \
	doc/icons/pinkball.png doc/icons/prev_g.png \
	doc/icons/prev.png doc/icons/purpleball.png \
	doc/icons/pv_grp_g.png doc/icons/pv_grp.png \
	doc/icons/redball.png doc/icons/up_g.png \
	doc/icons/up.png doc/icons/whiteball.png \
	doc/icons/yellowball.png

DEPENDENCYFILES = $(CSRC_cu)
