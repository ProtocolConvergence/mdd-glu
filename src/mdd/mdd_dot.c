/*
 * MDD Package
 *
 * $Id: mdd_dot.c,v 1.9 2012/01/31 20:40:50 fabio Exp $
 *
 * Functions to convert mdds to graphviz's dot format for graphic display.
 * These functions are effectively no-ops unless the BDD package supports
 * the conversion of BDDs to the dot format.
 *
 * Author: Fabio Somenzi

 Copyright (c) 2009, Regents of the University of Colorado

 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 Neither the name of the University of Colorado nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 *
 */

#include "mdd.h"

/* Convert an array of mdds to dot format.
 * If name is the NULL pointer, "" is used.
 * If fp is the NULL pointer, stdout is used.
 */
void
mdd_array_dump_dot(
  array_t *mdds		/* of mdd_t *, mdds to be converted */,
  char *name		/* for output names */,
  FILE *fp		/* output file */
  )
{
  mdd_manager *mgr;
  array_t *mvar_list;
  int i, n_mvar, k;
  int n_outputs = array_n(mdds);
  int n_inputs = 0;
  bdd_node **outputs = NULL;
  char **onames = NULL;
  char **inames = NULL;

  if (mdds == NIL(array_t) || n_outputs == 0) return;

  mgr = mdd_get_manager(array_fetch(mdd_t *, mdds, 0));
  n_inputs = bdd_num_vars(mgr);
  mvar_list = mdd_ret_mvar_list(mgr);
  n_mvar = array_n(mvar_list);
  /* Collect output nodes and names. */
  outputs = ALLOC(bdd_node *, n_outputs);
  onames = ALLOC(char *, n_outputs);
  if (n_outputs == 1) {
    outputs[0] = array_fetch(mdd_t *, mdds, 0);
    onames[0] = strdup(name == NIL(char) ? "" : name);
  } else {
    for (i = 0; i < n_outputs; i++) {
      mdd_t *mdd = array_fetch(mdd_t *, mdds, i);
      outputs[i] = bdd_extract_node_as_is(mdd);
      if (name == NIL(char)) {
	onames[i] = ALLOC(char, integer_get_num_of_digits(i) + 1);
	sprintf(onames[i], "%d", i);
      } else {
	onames[i] = ALLOC(char, strlen(name) +
			  integer_get_num_of_digits(i) + 2);
	sprintf(onames[i], "%s %d", name, i);
      }
    }
  }
  inames = ALLOC(char *, n_inputs);
  /* Set BDD variable names. */
  k = 0;
  for (i = 0; i < n_mvar; i++) {
    int j;
    mvar_type mv = array_fetch(mvar_type, mvar_list, i);
    if (mv.encode_length == 1) {
      inames[k] = strdup(mv.name);
      k++;
    } else {
      for (j = 0; j < mv.encode_length; j++, k++) {
	inames[k] = ALLOC(char, strlen(mv.name) +
			  integer_get_num_of_digits(j) + 2);
	sprintf(inames[k], "%s %d", mv.name, j);
      }
    }
  }
  /* Get the job done. */
  bdd_dump_dot(mgr, n_outputs, outputs, inames, onames,
	       fp == NIL(FILE) ? stdout: fp);
  /* Clean up */
  FREE(outputs);
  for (i = 0; i < n_outputs; i++) {
    FREE(onames[i]);
  }
  FREE(onames);
  for (i = 0; i < n_inputs; i++) {
    FREE(inames[i]);
  }
  FREE(inames);
  
} /* mdd_array_dump_dot */


/* Convert one mdd to dot format. */
void
mdd_dump_dot(
  mdd_t *mdd		/* mdd to be converted */,
  char *name		/* for output names */,
  FILE *fp		/* output file */
  )
{
  array_t *arr = array_alloc(mdd_t *, 1);

  array_insert_last(mdd_t *, arr, mdd);
  mdd_array_dump_dot(arr, name, fp);
  array_free(arr);

} /* mdd_dump_dot */


/* Print covers of an array of mdds.
 * If name is the NULL pointer, "" is used.
 */
void
mdd_array_print_cover(
  array_t *mdds		/* of mdd_t *, mdds to be printed */,
  char *name		/* output name */,
  boolean disjoint	/* print disjoint covers */
  )
{
  mdd_manager *mgr;
  mdd_t *f;
  array_t *mvar_list;
  int i, n_mvar, n_bvar;
  int n_inputs = 0;
  char **inames = NULL;
  int height;

  if (mdds == NIL(array_t) || array_n(mdds) == 0) return;

  mgr = mdd_get_manager(array_fetch(mdd_t *, mdds, 0));
  n_inputs = bdd_num_vars(mgr);
  mvar_list = mdd_ret_mvar_list(mgr);
  n_mvar = array_n(mvar_list);
  n_bvar = array_n(mdd_ret_bvar_list(mgr));
  height = name == NIL(char) ? 0 : strlen(name);
  inames = ALLOC(char *, n_inputs);
  for (i = 0; i < n_inputs; i++) inames[i] = NIL(char);
  /* Set BDD variable names. */
  for (i = 0; i < n_mvar; i++) {
    mvar_type mv = array_fetch(mvar_type, mvar_list, i);
    if (mv.encode_length == 1) {
      int k = array_fetch(int, mv.bvars, 0);
      inames[k] = strdup(mv.name);
      height = MAX(height,strlen(inames[k]));
    } else {
      int j;
      for (j = 0; j < mv.encode_length; j++) {
	int k = array_fetch(int, mv.bvars, j);
	inames[k] = ALLOC(char, strlen(mv.name) +
			  integer_get_num_of_digits(j) + 2);
	sprintf(inames[k], "%s %d", mv.name, j);
	height = MAX(height,strlen(inames[k]));
      }
    }
  }

  /* First print header and then BDDs. */
  for (i = 0; i < height; i++) {
    int j;
    for (j = 0; j < n_inputs; j++) {
      if (inames[j] == NIL(char) || i >= strlen(inames[j])) {
	printf(" ");
      } else {
	printf("%c", inames[j][i]);
      }
    }
    if (disjoint) printf(" ");
    printf(" %c\n", name == NIL(char) || i >= strlen(name) ? ' ' : name[i]);
  }
  printf("\n");
  arrayForEachItem(mdd_t *, mdds, i, f) {
    if (disjoint) {
      bdd_print_minterm(f);
      printf("\n");
    } else {
      bdd_print_cover(f);
    }
  }
  /* Clean up. */
  for (i = 0; i < n_inputs; i++) {
    if (inames[i] != NIL(char)) FREE(inames[i]);
  }
  FREE(inames);

} /* mdd_array_print_cover */


/* Print a cover of an mdd.
 * If name is the NULL pointer, "" is used.
 */
void
mdd_print_cover(
  mdd_t *f		/* mdd to be printed */,
  char *name		/* output name */,
  boolean disjoint	/* print disjoint cover */
  )
{
  array_t *arr = array_alloc(mdd_t *, 1);

  array_insert_last(mdd_t *, arr, f);
  mdd_array_print_cover(arr, name, disjoint);
  array_free(arr);

} /* mdd_print_cover */
