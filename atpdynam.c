/* EDITION AB03 (REL002), ITD ACST.177 (95/06/21 05:33:00) -- CLOSED but edited */

/*+*****************************************************************************

	Module Name:			atpdynam.c

	Copyright:				BNR Europe Limited, 1993 - 1995
							Bell-Northern Research / BNR
							Northern Telecom / Nortel

	NAME
	Atp_DvsPrintf()	-		Dynamic Varargs String PRINTF

	Atp_AdvPrintf()	-		Accumulating Dynamic Varargs PRINTF

	Atp_AdvGets()	-		Accumulating Dynamic Varargs GET String
	Atp_AdvGetsn()	-		Accumulating Dynamic Varargs GET String + Number

	Atp_AdvSetDefBufsize()	- Sets the default minimum buffer size

	Atp_AdvGetBufsize()		- Gets the actual current buffer size
	Atp_AdvGetDefBufsize()	- Gets the default minimum buffer size

	Atp_AdvResetDefBufsize() -	Resets the default buffer size value
								to its default value

	Atp_AdvResetBuffer()	- Resets the buffer

	DESCRIPTION

		This suite of routines is used to format a string into a
		dynamically allocated piece of memory, incrementally if
		required, and returns the result.

		The advantage of these routines over the use of mallocO and
		sprintfO is that it is not necessary to know the maximum size
		of the resultant string in advance.

	NOTES

		Includes optional memory debugging using Tcl when cc
		-DTCL_MEM_DEBUG is defined.

*****************************************************************************-*/

/********************************** includes **********************************/

#include <stdlib.h>
#include <stdio.h>

#if defined(__STDC__) || defined(__cplusplus)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

#include "atph.h"
#include "atpsysex.h"

#ifdef TCL_MEM_DEBUG
#	include <tcl.h>
#endif

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/********************************** DEFINES ***********************************/

#ifndef NULL
#define NULL ((char *)0)
#endif

#define DEFAULT_BUFSIZE 4096
// #define DEFAULT_BUFSIZE 10

/***************************** private VARIABLES ******************************/

typedef struct
{
	char	*buffer;		/*	pointer to beginning of buffer */
	char	*curr_ptr;		/*	pointer to current location in buffer to write in */
	int		bufsize;		/*	number of bytes which have been malloced for buffer */
	int		default_bufsize; /* default buffer size */
	int		curr_strlen;	/* current length of string written to buffer */
} atpdynam_type;

static atpdynam_type atpdynam_defaults	= { NULL, NULL, 0, DEFAULT_BUFSIZE, 0 };
static atpdynam_type atpdynam			= { NULL, NULL, 0, DEFAULT_BUFSIZE, 0 };

/***************************** private functions ******************************/

static FILE *dev_null_fd = (FILE *)0;

static void free_dev_null_fd()
{
	if (dev_null_fd != (FILE *)0)
	  fclose(dev_null_fd);
}

#if defined(__STDC__) || defined(__cplusplus)
static int dynamic_strlen( char *fmtstr, va_list ap )
#else
static int dynamic_strlen(fmtstr, ap)
	char *fmtstr;
	va_list ap;
#endif
{
	va_list ap2;

	static int first_time = 0;
	int count = 0;

	if (dev_null_fd == (FILE *)0) {
	  dev_null_fd = fopen("/dev/null", "w");

	/* fopen() returns NULL if error occurs. */
	if (dev_null_fd == (FILE *)0)
	  return -1;

	if (first_time == 0)
	  first_time = 1;
	}

	va_copy(ap2, ap);
	count = vfprintf(dev_null_fd, fmtstr, ap2);
	printf("atpdynam.c: dynamic_strlen(fmtstr %s, ap)...\n", fmtstr);
	va_end(ap2);
	va_copy(ap2, ap);
	printf(">>>");
	vprintf(fmtstr, ap2);
	printf("<<<\n");

	if (dev_null_fd != (FILE *)0 && first_time)
	{
		first_time = 0;
#if __STDC__ || hpux
		atexit(free_dev_null_fd);
#else
#  if sun || sun2 || sun3 || sun4
		on_exit(free_dev_null_fd);
#  endif
#endif
	}

	printf("\natpdynam.c: dynamic_strlen(fmtstr, ap) returns count = %d\n", count);

	va_end(ap2);

	/* vfprintf() returns EOF if error occurs. */
	if (count == EOF)
	  return -1;
	else
	  return count; /* excluding terminating NULL character */

}

/*
	Export dynamic_strlen as Atp_FormatStrlen to Atp_PrintfWordwrap in atphelp.c
*/
int (*Atp_FormatStrlen)_PROTO_(( char *fmtstr, va_list ap )) = dynamic_strlen;

#ifdef TCL_MEM_DEBUG
static char *	Atp_MemDebugFi1ename	= NULL;
static int		Atp_MemDebugLinenumber = 0;
#if defined(__STDC__) || defined(__cplusplus)
void Atp_MemDebugLocn( char *filename, int linenumber )
#else
void Atp_MemDebugLocn(filename,linenumber)
	char *filename;
	int linenumber;
#endif
{
	Atp_MemDebugFilename = filename;
	Atp_MemDebugLinenumber = linenumber;
}
#endif /* TCL_MEM_DEBUG */

/****************************** PUBLIC FUNCTIONS ******************************/

/*+****************************************************************************
 *	NAME
 *		Atp_DvsPrintf()	- Dynamic Varargs String PRINTF
 *
 *	DESCRIPTION
 *		This routine acts like sprintf(), but the resultant string is dynamic.
 *
 *	RETURNS
 *		int -	if outstr is supplied, returns the length of the resultant string
 *				(exclusive of the ending '\0’ byte);
 *				returns -1 if outstr is not supplied or if an error occurs.
 *
 *-****************************************************************************/
#ifdef TCL_MEM_DEBUG
#	if defined(__STDC__) ||	defined(__cplusplus)
	  int Atp_MemDebugDvsPrintf(char **outstr, char *fmtstr, ...)
#	else
	  int Atp_MemDebugDvsPrintf(outstr, fmtstr, va_alist)
		 char **outstr;
		 char *fmtstr;
		 va_dcl
#	endif
#else /* not TCL_MEM_DEBUG */
#	if defined(__STDC__) ||	defined(__cplusplus)
	  int Atp_DvsPrintf(char **outstr, char *fmtstr, ...)
#	else
	  int Atp_DvsPrintf(outstr, fmtstr, va_alist)
		 char **outstr;
		 char *fmtstr;
		 va_dcl
#	endif
#endif
{
	va_list		ap;
	int			numbytes = 0;
	char		*tmp = NULL;

	if (outstr == NULL)
	  return -1;

#if defined(__STDC__) || defined(__cplusplus)
	va_start(ap, fmtstr);
#else
	va_start(ap);
#endif

	numbytes = dynamic_strlen(fmtstr, ap);

	/* dynamic_strlen() returns -1 if an error has occurred. */
	if (numbytes == -1)
	  return -1;

#ifdef TCL_MEM_DEBUG
	tmp = (char *)Atp_MemDebug_Calloc(numbytes+l,sizeof(char),
									  Atp_MemDebugFilename,
									  Atp_MemDebugLinenumber);
#else
	tmp = (char *)calloc(numbytes+1, sizeof(char));
#endif

	if (tmp == NULL)
	  return -1;

	(void) vsprintf(tmp, fmtstr, ap);

	*outstr = tmp;

	va_end(ap);

	return numbytes;
}

/* destroy_dynamic_buffer() is used to reset the buffer. */
static void destroy_dynamic_buffer()
{
	if (atpdynam.buffer != NULL) {
#ifdef TCL_MEM_DEBUG
	  Tcl_DbCkfree(atpdynam.buffer,Atp_MemDebugFilename,Atp_MemDebugLinenumber);
#else
	  FREE(atpdynam.buffer);
#endif
	}
	atpdynam = atpdynam_defaults;
}

/*+***************************************************************************
 *	NAME
 *		Atp_AdvPrintf()	- Accumulating Dynamic Varargs PRINTF
 *
 *	DESCRIPTION
 *		This routine accumulates the results of subsequent calls, in dynamic
 *		memory, which is then returned by Atp_AdvGets();
 *
 *		In the event of an error, the buffer is destroyed and -1 is returned.
 ★
 *	RETURNS
 *		int - the length of the resultant string, -1 if error occurs.
 *
 *-****************************************************************************/
#ifdef TCL_MEM_DEBUG
#	if defined(__STDC__) || defined(__cplusplus)
	  int Atp_MemDebugAdvPrintf(char *fmtstr, ...)
#	else
	  int Atp_MemDebugAdvPrintf(fmtstr, va_alist)
		 char *fmtstr;
		 va_dcl
#	endif
#else /* not TCL_MEM_DEBUG */
#	if defined(__STDC__) || defined(__cplusplus)
	  int Atp_AdvPrintf(char *fmtstr, ...)
#	else
	  int Atp_AdvPrintf(fmtstr, va_alist)
		 char *fmtstr;
		 va_dcl
# endif
#endif
{
	va_list	args, ap2;

	int		strsize;
	int		delta, remain_size;

	printf("atpdynam.c: Atp_AdvPrintf called...\n");

#if defined(__STDC__) || defined(__cplusplus)
   va_start(args, fmtstr);
#else
   va_start(args);
#endif

   delta = remain_size = 0;

   strsize = dynamic_strlen(fmtstr, args);
   printf("\natpdynam.c: Atp_AdvPrintf - dynamic_strlen returns strsize = %d\n", strsize);
   va_copy(ap2, args);
   vprintf(fmtstr, ap2); // was args
   printf("\nOK\n");
   va_end(ap2);

   if (strsize == -1) {
	 destroy_dynamic_buffer () ;
	 return -1;
   }

   /* First time, create a large buffer. */
	if (atpdynam.buffer == NULL)
	{
	  char *tmp = NULL;

	  atpdynam.bufsize = ((strsize+1) < atpdynam.default_bufsize) ?
						  atpdynam.default_bufsize :
						  strsize + 1 + atpdynam.default_bufsize;

#ifdef TCL_MEM_DEBUG
	  tmp = (char *)Atp_MemDebug_Calloc(atpdynam.bufsize, sizeof(char),
										Atp_MemDebugFi1ename,
										Atp_MemDebugLinenumber	);
#else
	  tmp = (char *)calloc(atpdynam.bufsize, sizeof(char));
#endif

	  if (tmp == NULL) {
#ifdef TCL_MEM_DEBUG
	    Atp_MemDebugLocn(__FILE__,__LINE__);
#endif
	    destroy_dynamic_buffer ();
	    return -1;
	  }
	  else {
	    atpdynam.buffer = atpdynam.curr_ptr = tmp;
	    atpdynam.curr_strlen = 0;
	  }
	}

	/* Calculate how much room is left in the buffer for writing to. */
	delta		= atpdynam.curr_ptr - atpdynam.buffer;
	remain_size	= atpdynam.bufsize - delta;

	printf("delta = %d\n", delta);
	printf("remain_size = %d\n", remain_size);

	/*
	 *	Increase buffer size if not enough room for string to fit in or
	 *	if remaining space after vsprintf less than half of buffer size.
	 */
	if (((strsize+1) >= remain_size) ||
		((remain_size - (strsize+1)) < (atpdynam.bufsize/2)))
	{
	  char *tmp = NULL;

	  atpdynam.bufsize += (strsize + 1 + atpdynam.default_bufsize);

	  /* Just in case we get stuck in a loop somewhere - like I did! */
	  if (atpdynam.bufsize >= ATP_MAX_DYNAM_STRING_SIZE) {
	    int delay, times;

	    /* Print some of the accummulated text, approx. 1 screenful. */
		atpdynam.buffer[2001] = '\0';
		fprintf(stderr, "%s...\n", atpdynam.buffer);
		fprintf (stderr, "%c", '\007');
		fflush(stderr);

		/* Alert user! Ring the bell */
		for (times=0; times<=10; times++) {
		   for (delay=0; delay<1000000; delay++) ;
		   fprintf(stderr,"%c",'\007');
		   fflush(stderr);
		}

		/* Tell the user what is going on... */
		fprintf(stderr, "\n\nSYSTEM ERROR: ");
		fprintf(stderr,
				"POSSIBLE INFINITE LOOP DETECTED USING Atp_AdvPrintf()\n");
		fprintf(stderr, "				Dynamic string length is %d, ",
				atpdynam.bufsize);
		fprintf(stderr, "maximum limit is %d.\n\n", ATP_MAX_DYNAM_STRING_SIZE);

		/* Reset dynamic buffer */
#ifdef TCL_MEM_DEBUG
		Atp_MemDebugLocn (__FILE__,__LINE__);
#endif
		destroy_dynamic_buffer();

		/* Return user to the prompt if possible. */
		Atp_HyperSpace(0);

		/* If all else fails, exit. */
		fprintf(stderr,
				">>> ******** CANNOT RECOVER - PROGRAM TERMINATING ******** <<<\n") ;
		fprintf(stderr, "\nType \"reset\" to reset terminal.\n\n");
		fflush(stderr);

		exit(1);
	  }

#ifdef TCL_MEM_DEBUG
	  tmp = (char *)Tcl_DbCkrealloc(atpdynam.buffer, atpdynam.bufsize,
									Atp_MemDebugFilename,
									Atp_MemDebugLinenumber);
#else
	  tmp = (char *)realloc(atpdynam.buffer, atpdynam.bufsize);
#endif

	  if (tmp == NULL) {
#ifdef TCL_MEM_DEBUG
	    Atp_MemDebugLocn (__FILE__,__LINE__) ;
#endif
	    destroy_dynamic_buffer() ;
	    return -1;
	  }
	  else {
	    /* realloc() may have changed buffer's location, update variables. */
	    atpdynam.buffer = tmp;
	    atpdynam.curr_ptr = atpdynam.buffer + delta;
	  }
	}

	printf("atpdynam.c: Atp_AdvPrintf calls vsprintf(...)\n");
	va_copy(ap2, args);
	vprintf(fmtstr, ap2); // was args
	printf("\nOK\n");
	va_end(ap2);
	printf("\natpdynam.curr_ptr: %s\n", atpdynam.curr_ptr);
	(void) vsprintf(atpdynam.curr_ptr, fmtstr, args); /* append new string */

	atpdynam.curr_ptr += strsize;
	atpdynam.curr_strlen += strsize;

	va_end(args);

	printf("atpdynam.c: Atp_AdvPrintf returns %d\n", atpdynam.curr_strlen);
	return (atpdynam.curr_strlen);
}

/*+****************************************************************************
 *
 *	NAME
 *		Atp_AdvGets()	- Accumulating Dynamic Varargs GET String
 *		Atp_AdvGetsn()	- Accumulating Dynamic Varargs GET String + number
 *
 *	DESCRIPTION
 *		Atp_AdvGets() returns the resulting string.
 *		Atp_AdvGetsn{) also updates the count with the number of bytes in
 *		the string
 *
 *	RETURNS
 *		char * - a pointer to the resultant string
 *
 *-****************************************************************************/
#ifdef TCL_MEM_DEBUG
char *Atp_MemDebugAdvGets{)
#else
char *Atp_AdvGets()
#endif
{
	char *ptr = atpdynam.buffer;

	/*
	 *	If Atp_AdvPrintf() has not been called before, return NULL.
	 *	Otherwise, the next if statement will crash on divide by zero!
	 */
	if (atpdynam.bufsize == 0)
	  return NULL;

	/* Reduce the size of buffer if 10% or more space unused. */
	if ((((atpdynam.curr_strlen+1)/atpdynam.bufsize)*100) <= 90.00) {
#ifdef TCL_MEM_DEBUG
	  ptr = (char *)Tcl_DbCkrealloc(ptr, atpdynam.curr_strlen+1,
									Atp_MemDebugFilename,
									Atp_MemDebugLinenumber);
#else
	  ptr = (char *)realloc(ptr, atpdynam.curr_strlen+1);
#endif

	  /* If realloc didn't manage to work, return the original buffer. */
	  if (ptr == NULL) ptr = atpdynam.buffer;
	}

	atpdynam = atpdynam_defaults;

	return ptr;
}

#ifdef TCL_MEM_DEBUG
#	if defined(__STDC__) || defined(__cplusplus)
		char *Atp_MemDebugAdvGetsn( int *count )
#	else
		char *Atp_MemDebugAdvGetsn(count)
		int *count;
#	endif
#else
#	if defined(__STDC__) || defined(__cplusplus)
		char *Atp_AdvGetsn( int *count )
#	else
		char *Atp_AdvGetsn(count)
		int *count;
#	endif
#endif /* TCL_MEM_DEBUG */
{
	char *ptr = atpdynam.buffer;

	if (count != NULL)
	  *count = atpdynam.curr_strlen;

	/* If Atp_AdvPrintf() has not been called before, return NULL. */
	if (atpdynam.bufsize == 0)
	  return NULL;

	/* Reduce the size of buffer if 10% or more space unused. */
	if ((( (atpdynam.curr_strlen+1)/atpdynam.bufsize)*100) <= 90.00) {
#ifdef TCL_MEM_DEBUG
	  ptr = (char *)Tcl_DbCkrealloc(ptr, atpdynam.curr_strlen+l,
									Atp_MemDebugFilename,
									Atp_MemDebugLinenumber);
#else
	  ptr = (char *)realloc(ptr, atpdynam.curr_strlen+1);
#endif

	/* If realloc didn't manage to work, return the original buffer. */
	  if (ptr == NULL) ptr = atpdynam.buffer;
	}

	atpdynam = atpdynam_defaults;

	return ptr;
}

/*+*****************************************************************************
 *
 *	NAME
 *		Atp_AdvSetDefBufsize()		- Sets the default minimum buffer size
 *		Atp_AdvGetBufsize()			- Gets the actual current buffer size
 *		Atp_AdvGetDefBufsize()		- Gets the default minimum buffer size
 *		Atp_AdvResetDefBufsize()	- Resets the default buffer size to its default
 *		Atp_AdvResetBuffer()		- Resets the buffer
 *
 *	DESCRIPTION
 *		Atp_AdvSetDefBufsize(size) sets the default minimum buffer size for
 *		the Atp_AdvPrintf() function to use, but only if the specified size
 *		is more than zero, otherwise the buffer size remains unchanged.
 *
 *		The constant DEFAULT_BUFSIZE is used for the buffer size instead if
 *		Atp_AdvSetDefBufsize() is not used.
 *
 *		Atp_AdvGetBufsize() returns the actual current buffer size used.
 *		Atp_AdvGetDefBufsize() returns the default buffer size used.
 *
 *		Atp_AdvResetDefBufsize() resets the default buffer size
 *		to DEFAULT_BUFSIZE.
 *
 *		Atp_AdvResetBuffer() resets the buffer to its initial default state.
 *
 *		The buffer size specifies the amount of storage to allocate for a
 *		sequence of calls to Atp_AdvPrintf{). When storage needs to be
 *		increased, the specified buffer size is used as a minimum.
 *
 *	RETURNS
 *		int - actual size of buffer or default size of buffer
 *
 *-****************************************************************************/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_AdvSetDefBufsize( int size )
#else
int Atp_AdvSetDefBufsize(size)
   int size;
#endif
{
	if (size > 0)
	  atpdynam.default_bufsize =
			  atpdynam_defaults.default_bufsize = size;

	return atpdynam.default_bufsize;
}

int Atp_AdvGetBufsize()
{
	return atpdynam.bufsize;
}

int Atp_AdvGetDefBufsize()
{
	return atpdynam.default_bufsize;
}

int Atp_AdvResetDefBufsize()
{
	atpdynam.default_bufsize =
			atpdynam_defaults.default_bufsize = DEFAULT_BUFSIZE;

	return atpdynam.default_bufsize;
}

#ifdef TCL_MEM_DEBUG
int Atp_MemDebugAdvResetBuffer()
#else
int Atp_AdvResetBuffer()
#endif
{
	destroy_dynamic_buffer();

	(void) Atp_AdvResetDefBufsize();

	return atpdynam.bufsize;
}
