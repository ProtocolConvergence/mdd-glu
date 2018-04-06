/**CFile***********************************************************************

  FileName    [heapInt.h]

  PackageName [heap]

  Synopsis    [Heap-based priority queue.]

  Description [This is the internal header file for the heap-based priority
  queue.]

  SeeAlso     []

  Author      [Fabio Somenzi]

  Copyright   [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

  Revision    [$Id: heapInt.h,v 1.7 2005/05/18 19:25:43 jinh Exp $]

******************************************************************************/

#ifndef _HEAPINT
#define _HEAPINT

/*---------------------------------------------------------------------------*/
/* Nested includes                                                           */
/*---------------------------------------------------------------------------*/
#include "heap.h"


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/**Struct**********************************************************************

  Synopsis    [Slot of a heap.]

  Description [Slot of a heap.  Each slots holds a generic object and an
  integer key.]

******************************************************************************/
struct HeapSlot {
  long key;
  void *item;
};


/**Struct**********************************************************************

  Synopsis    [Heap.]

  Description []

******************************************************************************/
struct Heap {
  int length;
  int nitems;
  struct HeapSlot *slots;
  int (*compare)(const void *, const void *);
};


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**Macro***********************************************************************

  Synopsis    [Returns the parent of the i-th element in a heap.]

  Description [Returns the parent of the i-th element in a heap.
  Argument <code>i</code> should be strictly positive, otherwise the
  result is implementation dependent.]

  SideEffects [none]

******************************************************************************/
#define PARENT(i)	(((i)-1)>>1)

/**Macro***********************************************************************

  Synopsis    [Returns the left child of the i-th element in a heap.]

  SideEffects [none]

******************************************************************************/
#define LEFT(i)		(((i)<<1)+1)

/**Macro***********************************************************************

  Synopsis    [Returns the right child of the i-th element in a heap.]

  SideEffects [none]

******************************************************************************/
#define RIGHT(i)	(((i)+1)<<1)

/**Macro***********************************************************************

  Synopsis    [Returns the item stored in the i-th element in a heap.]

  SideEffects [none]

******************************************************************************/
#define ITEM(p,i)	((p)[i].item)

/**Macro***********************************************************************

  Synopsis    [Returns the key of the i-th element in a heap.]

  SideEffects [none]

******************************************************************************/
#define KEY(p,i)	((p)[i].key)


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/

#endif /* _HEAPINT */
