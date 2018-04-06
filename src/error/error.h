/*
 * $Id: error.h,v 1.3 2012/02/04 23:54:57 fabio Exp $
 *
 */
EXTERN void error_init(void);
EXTERN void error_append(char *);
EXTERN char *error_string(void);
EXTERN void error_cleanup(void);
