/* EDITION AC04 (REL002), ITD ACST.162 (95/05/04 20:24:12) -- CLOSED */

/*+*******************************************************************

	Module Name:		atppage.c

	Copyright:			BNR Europe Limited, 1992 - 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation of
						output paging.

********************************************************************-*/

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#if defined(__STDC__) || defined(__cplusplus)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

#include "atph.h"
#include "atpsysex.h"
#include "atpframh.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/* Local variables */
static char	*pager = "more -"; /* must be in $PATH */
static FILE	*pager_fp = NULL;

/* Local functions */
static int Atp_VideoCharsUsed _PROTO_((char *str));

/* External global function declarations. */
int Atp_PagingMode _PROTO_((int flag));
int Atp_PageLongerThanScreen _PROTO_((char *output_string));
int Atp_PagingNeeded = 1; /* default assumption */

/*+******************************************************************

	Function Name:		Atp_PagingMode

	Copyright:			BNR Europe Limited,	1992
						Bell-Northern Research
						Northern Telecom

	Description:		Sets and/or returns	the current	paging mode.

	Modifications:
		Who			When				Description
	----------	----------------	---------------------
	Alwyn Teh	3 November 1992		Initial Creation
	Alwyn Teh	24 June 1993		Change default mode
									from ON to AUTO,
									rewrite to use switch

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_PagingMode(int flag)
#else
int
Atp_PagingMode(flag)
	int flag;
#endif
{
	static int Atp_PagingModeFlag = ATP_PAGING_MODE_AUTO; /* default */

	switch(flag) {
		case ATP_PAGING_MODE_OFF:
		case ATP_PAGING_MODE_ON:
		case ATP_PAGING_MODE_AUTO:	Atp_PagingModeFlag = flag;
									break;
		case ATP_QUERY_PAGING_MODE:
		default: break;
	}

	return Atp_PagingModeFlag;
}

/*+********************************************************************

	Function Name:		Atp_PageLongerThanScreen

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Counts the number of newline characters to
						determine whether the text should be paged.

	Modifications:
		Who			When				Description
	----------	-------------	-----------------------------
	Alwyn Teh	23 June 1993	Modified from old function
								Atp_SetPagingIfNecessary

**********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
int Atp_PageLongerThanScreen(char *output_string)
#else
int
Atp_PageLongerThanScreen(output_string)
	char *output_string;
#endif
{
	register char	*src;
	char			*lines_env_str = getenv("LINES");
	int				maxlines = (lines_env_str != NULL && *lines_env_str != '\0') ?
								atoi(lines_env_str) : ATP_DEFAULT_LINES;
	int				lines = 0;

	if (output_string == NULL)
	  return 0;

	for (src = output_string; *src != 0; src++)
	{
	   if (*src == '\n')
		 lines++;
	}

	if (lines >= maxlines)
	  return lines;
	else
	  return 0;
}

/*+******************************************************************

	Function Name:		Atp_OutputPager

	Copyright:			BNR Europe Limited, 1993, 1994
						Bell-Northern Research
						Northern Telecom

	Description:		Implements output paging - accepts printf
						style format string and arguments; pipes
						output to pager (e.g. more) if paging is
						necessary.

	Modifications:
		Who			When				Description
	----------	-------------	-----------------------------
	Alwyn Teh	24 June 1993	Initial Creation
	Alwyn Teh	27 July 1994	Pipe through pager if
								- paging mode is ON, or
								- AUTO and paging needed, or
								- AUTO and video effects needed
								  for, say, external manpage.

*******************************************************************-*/
#if defined(__STDC__) || defined(___cplusplus)
int Atp_OutputPager(char *format, ...)
#else
int Atp_OutputPager(format, va_alist)
	char *format;
	va_dcl
#endif
{
	static int free_pager_fp_proc_registered = 0;
	int	rc;
	int	video_On = 0;
	va_list ap;

#if defined (__STDC__) || defined (__cplusplus)
	va_start (ap, format);
#else
	va_start(ap);
#endif

	if (Atp_PagingMode(ATP_QUERY_PAGING_MODE) == ATP_PAGING_MODE_AUTO) {
	  char *tmp;
	  Atp_CallFrame	callframe;

	  /* Extract stack frame from variable arguments list. */
	  Atp_CopyCallFrame(&callframe, ap);

	  Atp_DvsPrintf(&tmp, format, ATP_FRAME_RELAY(callframe));

	  Atp_PagingNeeded = (Atp_PageLongerThanScreen (tmp)) ? 1 : 0;

	  video_On = Atp_VideoCharsUsed(tmp);

	  FREE(tmp);
	}

	/* If paging mode ON/AUTO and paging needed, then page output. */
	if ((Atp_PagingMode(ATP_QUERY_PAGING_MODE) == ATP_PAGING_MODE_AUTO
		 && (Atp_PagingNeeded || video_On)) ||
		 ((Atp_PagingMode(ATP_QUERY_PAGING_MODE) == ATP_PAGING_MODE_ON)))
	{
	  if (pager_fp == NULL && pager != NULL)
		pager_fp = popen (pager, "w"); /* create pipe to pager process */
		/* stdin is duplicated for pager and cannot be pclose()ed */

	  /*
			Either pager set to “none", or pager_fp is NULL due to popen()
			error, so squirt output to stdout instead.
	   */
	  if (pager == NULL || pager_fp == NULL)
	  {
		rc = vfprintf (stdout, format, ap);
		(void) fflush(stdout); /* flush output */
	  }
	  else
	  {
		rc = vfprintf(pager_fp, format, ap); /* write to pipe */
		(void) fflush (pager_fp); /* flush output */
		(void) pclose (pager_fp); /* close pipe */
		pager_fp = NULL;
	  }
	}
	else {
	  /* paging not used, write direct to stdout */
	  rc = vfprintf(stdout, format, ap);
	  (void) fflush(stdout); /* flush output */
	}

	/* Reset paging flag to default ON for possible non-ATP commands. */
	Atp_PagingNeeded = 1;

	va_end(ap);

	return rc;
}

/*+*****************************************************************

	Function Name:		Atp_PagingCmd

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Paging command allows end-user	to have control
						over paging mode.

	Modifications:
		Who			When				Description
	----------	-----------------	------------------------------
	Alwyn Teh	24 June 1993		Initial Creation
	Alwyn Teh	29 September 1993	CASE has a new 3rd argument	-
									case keyvalue
	Alwyn Teh	30 September 1993	Atp_ChoiceDescriptor fields
									reordered.

********************************************************************-*/
static Atp_ChoiceDescriptor def_paging_choice = { ATP_QUERY_PAGING_MODE };

ATP_DCL_PARMDEF(Atp_PagingParms)
	BEGIN_PARMS
		BEGIN_OPT_CHOICE("paging mode", "set paging mode", &def_paging_choice, NULL)
			CASE("off", "turn paging OFF",   ATP_PAGING_MODE_OFF)
			CASE("on", "turn paging ON",     ATP_PAGING_MODE_ON)
			CASE("auto", "AUTOmatic paging", ATP_PAGING_MODE_AUTO)
			CASE("q", "(q)uery paging mode", ATP_QUERY_PAGING_MODE)
		END_OPT_CHOICE
	END_PARMS
ATP_END_DCL_PARMDEF

/* Export parmdef for paging command as pointer. */
Atp_ParmDefEntry *Atp_PagingParmsPtr = (Atp_ParmDefEntry *) Atp_PagingParms;

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_PagingCmd(ATP_FRAME_ELEM_TYPE ArgvO, ...)
#else
Atp_Re sult Atp_PagingCmd(va_alist)
	va_dcl
#endif
{
	va_list			ap;
	Atp_CallFrame	callframe;
	int				rc;
	char			*rs;
	Atp_BoolType	paging_mode;

/* Extract stack frame from variable arguments list. */
#if defined (__STDC__) || defined (__cplusplus)
	callframe.stack[0] = ArgvO;
	va_start(ap,ArgvO);
	rc = Atp_CopyCallFrameElems( &callframe.stack[1], ap, 1 );
#else
	va_start(ap);
	Atp_CopyCallFrame(&callframe, ap);
#endif

	va_end(ap);

	paging_mode = (Atp_BoolType) Atp_Index("paging mode");

	/* Set paging mode */
	paging_mode = (Atp_BoolType) Atp_PagingMode(paging_mode);

	rc = Atp_DvsPrintf(	&rs, "Output paging mode is set to %s%s",
						(paging_mode == 0) ? "OFF" :
						(paging_mode == 1) ? "ON" : "AUTO",
						(pager == NULL && paging_mode != 0) ?
						", but pager is set to \"none\"." : "." );

	if (rc >= 0)
	  Atp_ReturnDynamicStringToAdaptor(rs,ATP_FRAME_RELAY(callframe));

	return ATP_OK;
}

/*+*****************************************************************

	Function Name:		Atp_PagerCmd

	Copyright:			BNR Europe Limited, 1993-1995
						Bell-Northern Research
						Northern Telecom

	Description:		Pager command allows end-user control over
						type of pager used.

	Modifications:
		Who			When					Description
	----------	-----------------	-------------------------------
	Alwyn Teh	24 June 1993		Initial Creation
	Alwyn Teh	29 September 1993	CASE has a new 3rd argument -
									case keyvalue
	Alwyn Teh	30 September 1993	Use enums for CASES
	Alwyn Teh	10 January 1994		Default pager should be
									"q" for query. Typing
									"pager" shouldn't reset
									it to "more".
	Alwyn Teh	27 July 1994		Remove "none" option for
									disabling pager 'cos Greg
									didn't like it.
	Alwyn Teh	25 April 1995		Check pathname is not a
									directory. Use strerror()
									to get errno message where
									possible.

*******************************************************************-*/
/* CASE enums */
#define PAGER_MORE	201
#define PAGER_PAGE	202
#define PAGER_PG	203
#define PAGER_USER	204
#define PAGER_NONE	205
#define PAGER_QUERY	206

static Atp_ChoiceDescriptor default_pager = { PAGER_QUERY, (void *) "q"};

#if defined(__STDC__) || defined(__cplusplus)
static char *pathexists(char **strptr, Atp_BoolType isUserValue)
#else
static
	char *pathexists(strptr /*, isUserValue */)
	char **strptr;
	/* Atp_BoolType isUserValue; */
#endif
{
	static char	*rs = NULL;
	char		*pathname = NULL;
	char		*errcode;
	int			rc;

	if (rs != NULL) {
	  FREE(rs);
	  rs = NULL;
	}

	if (strptr != NULL) {
	  pathname = Atp_GetToken(*strptr, NULL, NULL);

	  if (pathname != NULL) {
		/* Find out if pathname exists and ok to use */
		rc = access (pathname, (F_OK|X_OK));

		if (rc != 0) {
#if defined(__STDC__) || defined (__cplusplus) || defined(hpux)
		  errcode = strerror(errno);
		  Atp_DvsPrintf(&rs, "Cannot use pathname: \"%s\" - %s.",
				  	  	pathname, errcode);
#else
		  char errno_str[5] ;
		  (void) sprintf(errno_str, "%d", errno);
		  errcode = (errno == ENOTDIR) ? "ENOTDIR" :
					(errno == ENOENT) ? "ENOENT" :
					(errno == EACCES) ? "EACCES" :
					(errno == EROFS) ? "EROFS" :
					(errno == ETXTBSY) ? "ETXTBSY" :
					(errno == EFAULT) ? "EFAULT" :
					(errno == EINVAL) ? "EINVAL" :
					(errno == EIO) ? "EIO" :
					(errno == ELOOP) ? "ELOOP" :
					(errno == ENAMETOOLCNG) ? "ENAMETOOLONG" : errno_str;
		  Atp_DvsPrintf (&rs, "Cannot use pathname; \"%s\", errno = %s", pathname, errcode);
#endif

		  if (pathname != NULL) FREE (pathname);

		  return rs;
		}
		else {
		  /* Make sure pathname is not a directory. */
		  struct stat stat_buf;
		  stat(pathname, &stat_buf);
		  if ((stat_buf.st_mode & S_IFMT) == S_IFDIR)
		  {
		    Atp_DvsPrintf (&rs,
				  	  	   "Cannot use directory pathname as pager: \"%s\"", pathname);
		    if (pathname != NULL) FREE(pathname);
		    return rs;
		  }
		  else
		  {
		    if (pathname != NULL) FREE(pathname);
		    return NULL;
		  }
		}
	  }
	  else
		return NULL;
	}
	else
	  return NULL;
}

ATP_DCL_PARMDEF(Atp_PagerParms)
	BEGIN_PARMS
		BEGIN_OPT_CHOICE("pager", "choose/set pager", &default_pager, NULL)
			CASE("more", "text filter for crt viewing", PAGER_MORE)
			CASE("page", "text filter for crt viewing", PAGER_PAGE)
			CASE("pg", "text filter for soft-copy terminals", PAGER_PG)
			BEGIN_CASE("user", "user supplied pager command", PAGER_USER)
				str_def("pathname","specified pager pathname",1,132,pathexists)
			END_CASE
#ifdef USE_PAGER_NONE
			CASE("none", "disable pager", PAGER_NONE)
#endif
			CASE("q", "(q)uery pager used", PAGER_QUERY)
		END_OPT_CHOICE
	END_PARMS
ATP_END_DCL_PARMDEF

/* Export parmdef for pager command as pointer. */
Atp_ParmDefEntry *Atp_PagerParmsPtr = (Atp_ParmDefEntry *)Atp_PagerParms;
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_PagerCmd(ATP_FRAME_ELEM_TYPE ArgvO, ...)
#else
Atp_Result Atp_PagerCmd(va_alist)
	va_dcl
#endif
{
	va_list			ap;
	Atp_CallFrame	callframe;
	int				rc = -1;
	int				i;
	char			*rs;
	static char		user_pager [133]; /* allow landscape screen */

/* Extract stack frame from variable arguments list. */
#if defined(__STDC__) || defined(__cplusplus)
	callframe.stack[0] = ArgvO;
	va_start(ap,ArgvO);
	rc = Atp_CopyCallFrameElems( &callframe.stack[1] , ap, 1 );
#else
	va_start(ap);
	Atp_CopyCallFrame(&callframe, ap);
#endif
	va_end(ap);

	switch(i = Atp_Num("pager")) {
		case PAGER_MORE :	pager = "more -"; /* must be in $PATH */
							break;
		case PAGER_PAGE :	pager = "page -"; /* must be in $PATH */
							break;
		case PAGER_PG :		pager = "pg -"; /* must be in $PATH */
							break;
		case PAGER_USER :	pager = Atp_Str("pathname"); /* dynamic string */
							/* make a static copy */
							strncpy(user_pager, pager, 133) ;
							/* assign to real pager */
							pager = user_pager;
							break;
#ifdef USE_PAGER_NONE
		case PAGER_NONE :	pager = NULL;
							user_pager[0] = '\0'; /* cancel any user pager */
							break;
#endif
		case PAGER_QUERY :
		default: break;
	}

	rc = Atp_DvsPrintf(&rs, "Pager = %s", (pager) ? pager : "(none)");

	/*
	 *	User has specified a pager, close the old one.
	 *	Atp_OutputPager() will open the new one when called.
	 */
	if ((0 <= i) && (i <= 4)) {
	  if (pager_fp != NULL)
	  {
		(void) pclose(pager_fp);
		printf("Atp_PagerCmd(): pclose(%s) called\n", pager);
	  }
	  pager_fp = NULL;
	}

	if (rc >= 0)
	  Atp_ReturnDynamicStringToAdaptor(rs,ATP_FRAME_RELAY(callframe));

	return ATP_OK;
}

/*+*****************************************************************

	Function Name:		Atp_VideoCharsUsed

	Copyright:			BNR Europe Limited,	1994
						Bell-Northern Research
						Northern Telecom

	Description:		Function to test whether string contains
						backslashed characters for video effects.

	Modifications:
		Who			When				Description
	----------	-------------	-----------------------------
	Alwyn Teh	27 July 1994	Initial	Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(___cplusplus)
static int Atp_VideoCharsUsed(char *str)
#else
static int Atp_VideoCharsUsed(str)
	char *str;
#endif
{
	register int x;

	if (str == NULL)
	  return 0;

	for (x = 0; str[x] != '\0'; x++) {
	   if (str[x] == '\b')
		 if (x > 0 && str[x+1] != '\0')
		   if ((str[x-1] == str[x+1]) || (str[x-1] == '_'))
			 return 1;
	}

	return 0;
}
