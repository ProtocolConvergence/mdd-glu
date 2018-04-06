/**CFile***********************************************************************

  FileName    [heap.h]

  PackageName [heap]

  Synopsis    [Heap-based priority queue.]

  Description [This is the external header file for the heap-based
  priority queue.  The priority of each item is determined by an
  integer key.  The first element of the heap is the one with the
  smallest key.  Multiple items with the same key can be inserted.
  Refer to Chapter 7 of Cormen, Leiserson, and Rivest for the theory.
  (The only significant difference is that the array indices start
  from 0 in this implementation.)]

  SeeAlso     []

  Author      [Fabio Somenzi]

  Copyright   [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

  Revision    [$Id: heap.h,v 1.14 2012/02/04 23:54:57 fabio Exp $]

******************************************************************************/

#ifndef _HEAP
#define _HEAP

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/
#include "util.h"
#undef MAX
#undef MIN

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

typedef struct HeapSlot HeapSlot_t;

typedef struct Heap Heap_t;

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**Macro***********************************************************************

  Synopsis    [Iterates over the elements of a heap.]

  SideEffects [none]

******************************************************************************/
#define Heap_HeapForEachItem(                                              \
  /* Heap_t * */ heap /* heap whose element should be enumerated */,       \
  /* int */      i    /* local variable for iterator */,                   \
  /* void * */	 data /* heap item */                                      \
)                                                                          \
  for((i) = 0; (((i) < (heap)->nitems) && (data = (heap)->slots[i].item)); \
      (i)++)



/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

EXTERN Heap_t * Heap_HeapInit(int length);
EXTERN Heap_t * Heap_HeapInitCompare(int length, int (*compare)(const void *, const void *));
EXTERN void Heap_HeapFree(Heap_t *heap);
EXTERN int Heap_HeapInsert(Heap_t *heap, void *item, long key);
EXTERN int Heap_HeapInsertCompare(Heap_t *heap, void *item, long key);
EXTERN int Heap_HeapExtractMin(Heap_t *heap, void *item, long *key);
EXTERN int Heap_HeapCount(Heap_t *heap);
EXTERN Heap_t * Heap_HeapClone(Heap_t *source);
EXTERN int Heap_HeapTest(Heap_t *heap);
EXTERN int Heap_HeapTestCompare(Heap_t *heap);
EXTERN void Heap_HeapApplyForEachElement(Heap_t *heap, int (*compare)(const void *));

/**AutomaticEnd***************************************************************/

#endif /* _HEAP */
