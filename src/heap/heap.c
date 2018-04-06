/**CFile***********************************************************************

  FileName    [heap.c]

  PackageName [heap]

  Synopsis    [Heap-based priority queue.]

  Description [This file contains the functions to maintain a priority
  queue implemented as a heap.]

  SeeAlso     []

  Author      [Fabio Somenzi]

  Copyright   [This file was created at the University of Colorado at
  Boulder.  The University of Colorado at Boulder makes no warranty
  about the suitability of this software for any purpose.  It is
  presented on an AS IS basis.]

******************************************************************************/

#include "heapInt.h"


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

#ifndef lint
static char rcsid[] UNUSED = "$Id: heap.c,v 1.19 2012/02/04 23:54:57 fabio Exp $";
#endif


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/



/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

static void HeapHeapify(Heap_t *heap);
static void HeapHeapifyCompare(Heap_t *heap);
static int HeapResize(Heap_t *heap);

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Initializes a priority queue.]

  Description [Initializes a priority queue. Returns a pointer to the
  heap if successful; NULL otherwise.  The queue is implemented as a
  heap.  The first element of the heap is the one with the smallest
  key.]

  SideEffects [None]

  SeeAlso     [Heap_HeapFree]

******************************************************************************/
Heap_t *
Heap_HeapInit(
  int length)
{
  Heap_t *heap;

  heap = ALLOC(Heap_t, 1);
  if (heap == NIL(Heap_t)) return NIL(Heap_t);
  heap->length = length;
  heap->nitems = 0;
  heap->compare = 0;
  heap->slots = ALLOC(HeapSlot_t, length);
  if (heap->slots == NIL(HeapSlot_t)) {
    FREE(heap);
    return NIL(Heap_t);
  }
  return heap;

} /* Heap_HeapInit */


/**Function********************************************************************

  Synopsis    [Initializes a priority queue.]

  Description [Initializes a priority queue. Returns a pointer to the
  heap if successful; NULL otherwise.  The queue is implemented as a
  heap.  The first element of the heap is the one with the smallest
  key.]

  SideEffects [None]

  SeeAlso     [Heap_HeapFree]

******************************************************************************/
Heap_t *
Heap_HeapInitCompare(
  int length, int (*compare)(const void *, const void *))
{
  Heap_t *heap;

  heap = ALLOC(Heap_t, 1);
  if (heap == NIL(Heap_t)) return NIL(Heap_t);
  heap->length = length;
  heap->nitems = 0;
  heap->compare = compare;
  heap->slots = ALLOC(HeapSlot_t, length);
  if (heap->slots == NIL(HeapSlot_t)) {
    FREE(heap);
    return NIL(Heap_t);
  }
  return heap;

} /* Heap_HeapInitCompare */


/**Function********************************************************************

  Synopsis    [Frees a priority queue.]

  Description []

  SideEffects [None]

  SeeAlso     [Heap_HeapInit]

******************************************************************************/
void
Heap_HeapFree(
  Heap_t *heap)
{
  FREE(heap->slots);
  FREE(heap);
  return;

} /* Heap_HeapFree */


/**Function********************************************************************

  Synopsis    [Inserts an item in a priority queue.]

  Description [Inserts an item in a priority queue.  Returns 1 if
  successful; 0 otherwise.]

  SideEffects [None]

  SeeAlso     [Heap_HeapExtractMin]

******************************************************************************/
int
Heap_HeapInsert(
  Heap_t *heap,
  void *item,
  long key)
{
  HeapSlot_t *slots;
  int i = heap->nitems;

  if (i == heap->length && !HeapResize(heap)) return 0;
  slots = heap->slots;
  heap->nitems++;
  while (i > 0 && KEY(slots, PARENT(i)) > key) {
    ITEM(slots, i) = ITEM(slots, PARENT(i));
    KEY(slots, i) = KEY(slots, PARENT(i));
    i = PARENT(i);
  }
  ITEM(slots, i) = item;
  KEY(slots, i) = key;
  return 1;

} /* Heap_HeapInsert */

/**Function********************************************************************

  Synopsis    [Inserts an item in a priority queue.]

  Description [Inserts an item in a priority queue.  Returns 1 if
  successful; 0 otherwise.]

  SideEffects [None]

  SeeAlso     [Heap_HeapExtractMin]

******************************************************************************/
int
Heap_HeapInsertCompare(
  Heap_t *heap,
  void *item,
  long key)
{
  HeapSlot_t *slots;
  int i = heap->nitems;

  if (i == heap->length && !HeapResize(heap)) return 0;
  slots = heap->slots;
  heap->nitems++;
  while (i > 0 && (*(heap->compare))((char *)(long)KEY(slots, PARENT(i)), (char *)(long)key)) {
    ITEM(slots, i) = ITEM(slots, PARENT(i));
    KEY(slots, i) = KEY(slots, PARENT(i));
    i = PARENT(i);
  }
  ITEM(slots, i) = item;
  KEY(slots, i) = key;
  return 1;

} /* Heap_HeapInsertCompare */



/**Function********************************************************************

  Synopsis    [Extracts the element with the minimum key from a priority
  queue.]

  Description [Extracts the element with the minimum key from a
  priority queue.  Returns 1 if successful; 0 otherwise.]

  SideEffects [The minimum key and the associated item are returned as
  side effects.]

  SeeAlso     [Heap_HeapInsert]

******************************************************************************/
int
Heap_HeapExtractMin(
  Heap_t *heap,
  void *item,
  long *key)
{
  HeapSlot_t *slots = heap->slots;

  if (heap->nitems == 0) return 0;
  *(void **)item = ITEM(slots, 0);
  *key = KEY(slots, 0);
  heap->nitems--;
  /* The next three lines are redundant if the queue is empty. */
  ITEM(slots, 0) = ITEM(slots, heap->nitems);
  KEY(slots, 0) = KEY(slots, heap->nitems);
  if(heap->compare) HeapHeapifyCompare(heap);
  else              HeapHeapify(heap);

  return 1;

} /* Heap_HeapExtractMin */


/**Function********************************************************************

  Synopsis    [Returns the number of items in a priority queue.]

  Description []

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int
Heap_HeapCount(
  Heap_t *heap)
{
  return(heap->nitems);

} /* Heap_HeapCount */


/**Function********************************************************************

  Synopsis    [Clones a priority queue.]

  Description []

  SideEffects [None]

  SeeAlso     [Heap_HeapInit]

******************************************************************************/
Heap_t *
Heap_HeapClone(
  Heap_t *source)
{
  Heap_t *dest;
  int i;
  int nitems = source->nitems;
  HeapSlot_t *sslots = source->slots;
  HeapSlot_t *dslots;

  dest = Heap_HeapInit(source->length);
  if (dest == NULL) return(NULL);
  dest->nitems = nitems;
  dslots = dest->slots;
  for (i = 0; i < nitems; i++) {
    KEY(dslots, i) = KEY(sslots, i);
    ITEM(dslots, i) = ITEM(sslots, i);
  }
  return(dest);

} /* Heap_HeapClone */


/**Function********************************************************************

  Synopsis    [Tests the heap property of a priority queue.]

  Description [Tests the heap property of a priority queue.  Returns 1 if
  successful; 0 otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int
Heap_HeapTest(
  Heap_t *heap)
{
  HeapSlot_t *slots = heap->slots;
  int nitems = heap->nitems;
  int i;

  for (i = 1; i < nitems; i++) {
    if (KEY(slots,i) < KEY(slots, PARENT(i)))
      return 0;
  }
  return 1;

} /* Heap_HeapTest */

/**Function********************************************************************

  Synopsis    [Tests the heap property of a priority queue.]

  Description [Tests the heap property of a priority queue.  Returns 1 if
  successful; 0 otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int
Heap_HeapTestCompare(
  Heap_t *heap)
{
  HeapSlot_t *slots = heap->slots;
  int nitems = heap->nitems;
  int i;

  for (i = 1; i < nitems; i++) {
    if ((*(heap->compare))((char *)(long)KEY(slots, PARENT(i)), (char *)(long)KEY(slots,i)))
      return 0;
  }
  return 1;

} /* Heap_HeapTest */




/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Maintains the heap property of a priority queue.]

  Description []

  SideEffects [None]

  SeeAlso     [Heap_HeapExtractMin]

******************************************************************************/
static void
HeapHeapify(
  Heap_t *heap)
{
  int nitems = heap->nitems;
  HeapSlot_t *slots = heap->slots;
  int i = 0;
  int smallest = 0;
  void *item = ITEM(slots, 0);
  long key = KEY(slots, 0);

  while (1) {
    int left = LEFT(i);
    int right = RIGHT(i);
    int minkey;
    if (left < nitems && (minkey = KEY(slots, left)) < key) {
      smallest = left;
    } else {
      minkey = key;
    }
    if (right < nitems && KEY(slots, right) < minkey) {
      smallest = right;
    }
    if (smallest == i) break;
    KEY(slots, i) = KEY(slots, smallest);
    ITEM(slots, i) = ITEM(slots, smallest);
    i = smallest;
  }
  KEY(slots, i) = key;
  ITEM(slots, i) = item;
  return;

} /* HeapHeapify */

/**Function********************************************************************

  Synopsis    [Tests the heap property of a priority queue.]

  Description [Tests the heap property of a priority queue.  Returns 1 if
  successful; 0 otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static void
HeapHeapifyCompare(
  Heap_t *heap)
{
  int nitems = heap->nitems;
  HeapSlot_t *slots = heap->slots;
  int i = 0;
  int smallest = 0;
  void *item = ITEM(slots, 0);
  int key = KEY(slots, 0);
  int minkey;


  while (1) {
    int left = LEFT(i);
    int right = RIGHT(i);
    if (left < nitems && (*(heap->compare))((char *)(long)key, (char *)(long)(minkey = KEY(slots, left)))) {
      smallest = left;
    } else {
      minkey = key;
    }
    if (right < nitems && (*(heap->compare))((char *)(long)minkey, (char *)(long)KEY(slots, right))) {
      smallest = right;
    }
    if (smallest == i) break;
    KEY(slots, i) = KEY(slots, smallest);
    ITEM(slots, i) = ITEM(slots, smallest);
    i = smallest;
  }
  KEY(slots, i) = key;
  ITEM(slots, i) = item;
  return;

} /* HeapHeapifyCompare */


/**Function********************************************************************

  Synopsis    [Resizes a priority queue.]

  Description [Resizes a priority queue by doubling the number of
  available slots.  Returns 1 if successful; 0 otherwise.]

  SideEffects [None]

  SeeAlso     [Heap_HeapInsert]

******************************************************************************/
static int
HeapResize(
  Heap_t *heap)
{
  int oldlength = heap->length;
  int newlength = 2 * oldlength;
  HeapSlot_t *oldslots = heap->slots;
  HeapSlot_t *newslots = REALLOC(HeapSlot_t, oldslots, newlength);
  if (newslots == NIL(HeapSlot_t)) return 0;
  heap->length = newlength;
  heap->slots = newslots;
  if (heap->compare) {
    assert(Heap_HeapTestCompare(heap));
  }
  else {
    assert(Heap_HeapTest(heap));
  }
  return 1;

} /* HeapResize */

/**Function********************************************************************

  Synopsis    [Apply function for each element of heap.]

  Description [Apply function for each element of heap. 
               Returns 1 if successful; 0 otherwise.]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void
Heap_HeapApplyForEachElement(Heap_t *heap, int (*compare)(const void *))
{
int i;

  for(i=0; i<heap->nitems; i++) {
    (*compare)(heap->slots[i].item);
  }
  return;
}
