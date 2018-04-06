/*
 * $Id: memuser.h,v 1.3 2002/08/27 15:47:48 fabio Exp $
 *
 */

/* Memory management user-visible definitions */


#if !defined(_MEMUSERH)
#define _MEMUSERH

#ifndef ARGS
#  ifdef __STDC__
#    define ARGS(args)	args
#  else
#    define ARGS(args)  ()
#  endif
#endif

#ifdef __cplusplus
#  define EXTERN	extern "C"
#else
#  define EXTERN	extern
#endif

#include <stdio.h>



/* >>> Potentially machine dependent stuff */
/* See memint.h as well. */

typedef unsigned long INT_PTR;	/* Integral type that can hold a pointer */
typedef unsigned long SIZE_T;	/* Integral type that can hold the maximum */
				/* size of an object */

/* REQUIRED_ALIGNMENT is the alignment required by the machine hardware; */
/* it is provided for user use. */

#define REQUIRED_ALIGNMENT 4


/* Types */

#if defined(__STDC__)
typedef void *pointer;
#else
typedef char *pointer;
#endif


typedef struct rec_mgr_ *rec_mgr;


/* ALLOC_ALIGNMENT is the alignment for all storage returned by the */
/* storage allocation routines. */

#define ALLOC_ALIGNMENT 8


/* Round a size up for alignment */

#define ROUNDUP(size) ((((size)+ALLOC_ALIGNMENT-1)/ALLOC_ALIGNMENT)*ALLOC_ALIGNMENT)
#define ALIGN(size) ((((size)+REQUIRED_ALIGNMENT-1)/REQUIRED_ALIGNMENT)*REQUIRED_ALIGNMENT)


/* Block storage management routines */

EXTERN pointer mem_get_block ARGS((SIZE_T));
EXTERN void mem_free_block ARGS((pointer));
EXTERN pointer mem_resize_block ARGS((pointer, SIZE_T));
EXTERN void mem_copy ARGS((pointer, pointer, SIZE_T));
EXTERN void mem_zero ARGS((pointer, SIZE_T));
EXTERN void mem_fatal ARGS((char *));
EXTERN SIZE_T mem_allocation ARGS((void));


/* Record manager routines */

EXTERN pointer mem_new_rec ARGS((rec_mgr));
EXTERN void mem_free_rec ARGS((rec_mgr, pointer));
EXTERN rec_mgr mem_new_rec_mgr ARGS((int));
EXTERN void mem_free_rec_mgr ARGS((rec_mgr));


#undef ARGS
#undef EXTERN

#endif
