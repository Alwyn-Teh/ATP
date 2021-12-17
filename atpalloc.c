/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpalloc.c

	Copyright:			BNR Europe Limited, 1992, 1994, 1995
						Bell-Northern Research/ BNR
						Northern Telecom / Nortel

	Description:		Each of these functions will call either
						malloc, calloc or realloc, then, if fn, a
						function pointer, if not NULL, will be used
						to call fn{) if malloc, calloc or realloc
						returns NULL.

	Notes:				For memory debugging. Tcl's malloc functions
						may be used when the cc -DTCL_MEM_DEBUG flag
						is defined.

********************************************************************-*/

#include <unistd.h>
#include <stddef.h>
// #include <malloc.h>

#ifdef TCL_MEM_DEBUG
# include <tcl.h>
#endif

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__
#endif

/* Constants */
#define BELL '\007' /* for debugging, indicates realloc() called */

/* Declarations */
void Atp_OutOfMemoryHandler _PROTO_((unsigned size,
									 char *filename,
									 int line_number));

/* Variables */
static void (*Atp_NoMemHandlerFunc) _PROTO_((unsigned size,
											 char *filename,
											 int line_number))
										= Atp_OutOfMemoryHandler; /* default */

/*+*******************************************************************

	Function Name:		Atp_Malloc

	Copyright:			BNR Europe Limited, 1992, 1994
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Just like malloc(), but with added value.
						fn is a function pointer. The function is
						called when malloc() returns NULL when no
						memory is available or if the arena has been
						detectably corrupted by storing outside the
						bounds of a block.
	Modifications:
		Who			When				Description
	----------	---------------	--------------------------
	Alwyn Teh	28 July 1992	Initial Creation
	Alwyn Teh	18 January 1994	Memory debugging using Tcl

*******************************************************************-*/
#if defined (__STDC__) || defined(__cplusplus)
void * Atp_Malloc
(
	unsigned	size,
	void		(*fn)(unsigned size, char *filename, int line_number),
	char		* filename,
	int			line_number
)
#else
void *
Atp_Malloc(size, fn, filename, line_number)
	unsigned	size;
	void		(*fn)();
	char		*filename;
	int			line_number;
#endif
{
		char *ptr = NULL;

#ifdef TCL_MEM_DEBUG
		ptr = (char *) Tcl_DbCkalloc(size, filename, line_number)
#else
		ptr = (char *) malloc(size);
#endif

		if (ptr != NULL)
		  return ptr;
		else {
		  if (fn != NULL) {
		    /* Call tidy-up function, may NOT return */
		    (*fn) (size, filename, line_number);
		  }
		  else {
		    if (Atp_NoMemHandlerFunc != NULL)
		      Atp_NoMemHandlerFunc(size, filename, line_number);
		  }
		  return NULL;
		}
}

/*+*******************************************************************

	Function Name:		Atp_Calloc

	Copyright:			BNR Europe Limited, 1992, 1994
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Just like calloc(), but with added value.
						fn is a function pointer. The function is
						called when calloc() returns NULL when no
						memory is available or if the arena has been
						detectably corrupted by storing outside the
						bounds of a block.
	Modifications:
		Who			When				Description
	----------	---------------	------------------------------
	Alwyn Teh	28 July 1992	Initial Creation
	Alwyn Teh	18 January 1994 Memory debugging using Tcl

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void * Atp_Calloc
(
	unsigned	nelem,
	unsigned	elsize,
	void		(*fn)(unsigned size, char *filename, int line_number),
	char		*filename,
	int			line_number
)
#else
void *
Atp_Calloc(nelem, elsize, fn, filename, line_number)
	unsigned	nelem, elsize;
	void		(*fn)();
	char		*filename;
	int			line_number;
#endif
{
		char			*ptr = NULL;
		unsigned int 	size = nelem * elsize;

#ifdef TCL_MEM_DEBUG
		ptr = (char *) Atp_MemDebug_Calloc(nelem,elsize,filename,line_number);
#else
		ptr = (char *) calloc(nelem, elsize);
#endif

		if (ptr != NULL)
		  return ptr;
		else {
		  if (fn != NULL) {
		    /* Call tidy-up function, may NOT return */
		    (*fn)(size, filename, line_number);
		  }
		  else {
		    if (Atp_NoMemHandlerFunc != NULL)
		      Atp_NoMemHandlerFunc(size, filename, line_number);
		  }
		  return NULL;
		}
}

/*+*******************************************************************

	Function Name:		Atp_Realloc

	Copyright:			BNR Europe Limited, 1992, 1994
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Just like realloc(), but with added value.
						fn is a function pointer. The function is
						called when realloc() returns NOLL when no
						memory is available or if the arena has been
						detectably corrupted by storing outside the
						bounds of a block.
	Modifications:
		Who			When				Description
	----------	---------------	-------------------------------
	Alwyn Teh	28 July 1992	Initial Creation
	Alwyn Teh	18 January 1994	Memory debugging using Tcl

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void * Atp_Realloc
(
	void		*ptr,
	unsigned	size,
	void		(*fn)(unsigned size, char *filename, int line_number),
	char		*filename,
	int			line_number
)
#else
void *
Atp_Realloc(ptr, size, fn, filename, line_number)
	void		*ptr,
	unsigned	size;
	void		(*fn)();
	char		*filename
	int			line_number;
#endif
{
		char *realloc_ptr = NULL;

#ifdef TCL_MEM_DEBUG
		realloc_ptr = (char *)Tcl_DbCkrealloc(ptr, size, filename, line_number);
#else
		realloc_ptr = (char *)realloc(ptr, size);
#endif

		if (realloc_ptr != NULL) {
#ifdef DEBUG
		  if {Atp_DebugMode) {
			/*
				Alert audible or visual bell to indicate realloc being
				used.
			*/
			(void) fprintf(stderr, "%c", BELL);
			(void) fflush(stderr);
			(void) sleep(1);
		  }
#endif
		  return realloc_ptr;
		}
		else {
		  if (fn != NULL) {
			/* Call tidy-up function, may NOT return */
			(*fn)(size, filename, line_number);
		  }
		  else {
			if (Atp_NoMemHandlerFunc != NULL)
			  Atp_NoMemHandlerFunc(size, filename, line_number);
		  }
		  return NULL;
		}
}
/********************************************************************

	Function Name:		Atp_OutOfMemoryHandler
	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Handles out-of-memory condition.

	Modifications:
		Who			When			Description
	----------	--------------	------------------------
	Alwyn Teh	28 July 1992	Initial Creation

******************************************************************-*/
#if defined (__STDC_) || defined (__cplusplus)
void Atp_OutOfMemoryHandler
(
	unsigned	size,
	char		*filename,
	int			line_number
)
#else
void
Atp_OutOfMemoryHandler(size, filename, line_number)
	unsigned	size;
	char		*filename;
	int			line_number;
#endif
{
		char	*errmsg = NULL;

		/*
			Atp_MakeErrorMsg() itself uses malloc, which if again
			returns NULL, will print error message to stderr.
		*/
		Atp_ShowErrorLocation();
		errmsg = Atp_MakeErrorMsg(filename, line_number,
								  ATP_ERRCODE_MALLOC_RETURNS_NULL, size);
		Atp_HyperSpace(errmsg);
}

/*********************************************************************

	Function Name:		Atp_SetDefaultMallocErrorHandler

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Sets the default memory allocation error handler.

	Modifications:
		Who			When			Description
	----------	------------	---------------------
	Alwyn Teh	28 July 1992	Initial Creation

*******************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
void Atp_SetDefaultMallocErrorHandler
(
	void (*fn)(unsigned size, char *filename, int line_number)
)
#else
void
Atp_SetDefaultMallocErrorHandler(fn)
	void (*fn) () ;
#endif
{
	Atp_NoMemHandlerFunc = fn;
}

