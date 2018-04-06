/**CHeaderFile*****************************************************************

  FileName    [cmuPortInt.h]

  PackageName [cmu_port]

  Synopsis    [Header file used by cmu_port.]

  Description [optional]

  SeeAlso     [optional]

  Author      [Rajeev K. Ranjan]

  Copyright   [Copyright (c) 1994-1996 The Regents of the Univ. of California.
  All rights reserved.

  Permission is hereby granted, without written agreement and without license
  or royalty fees, to use, copy, modify, and distribute this software and its
  documentation for any purpose, provided that the above copyright notice and
  the following two paragraphs appear in all copies of this software.

  IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
  CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN
  "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO PROVIDE
  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.]

  Revision    [$Id: cmuPortInt.h,v 1.1.1.2 1997/02/12 21:15:26 hsv Exp $]

******************************************************************************/

#ifndef _CMU_PORT_INT
#define _CMU_PORT_INT

#include "util.h"     /* includes math.h */
#include "array.h"
#include "st.h"
#include "bdd.h"      
#include "bddint.h"   /* CMU internal routines; for use in bdd_get_branches() and for BDD_POINTER */
/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/
#ifndef refany
/* this otta be a typedef somewhere */
#define refany char *	/* a void * or any sort of untyped pointer */
#endif

#ifndef any	/* Ansi C defines this right? */
/* this otta be a typedef somewhere */
#define any char	/* so that NIL(any) == refany */
#endif


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/
struct bdd_t {
  boolean free; /* TRUE if this is free, FALSE otherwise ... */
  struct bdd_ *node; /* ptr to the top node of the function */
  struct bdd_manager_ *mgr; /* the manager */
};


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Function prototypes                                                       */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/

#endif /* _CMU_PORT_INT */
