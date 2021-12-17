/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpdebug.c

	Copyright:			BNR Europe Limited,	1992, 1995
						Bell-Northern Research/ BNR
						Northern Telecom / Nortel

	Description:		FOR ATP DEVELOPMENT	USE ONLY
						This module contains debugging facilities
						such as the "debug" command, available only
						if compiled with the DEBUG flag defined.

*******************************************************************_*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/* Global internal variable */
Atp_BoolType	Atp_DebugMode = 0;

/* Global internal function. */
Atp_Result		Atp_DebugModeCmd _PROTO_((void *, ...));

/*+*******************************************************************

	Function Name:		Atp_DebugModeCmd

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		A simple command which sets the debug mode
						flag Atp_DebugMode for global use.

	Parameters:			None - any arguments not used

	Global Variables:	Atp_DebugMode

	Results:			Atp_Result

	Calls:				Atp_Bool

	Called by:			Atp2Tcl_CreateCommand() in Atp2Tcl_Init() in
						atp2tcl.c to register command with ATP and
						Tcl; if not using Tcl, could be any other
						interpreter in use.

	Side effects:		fprintf to stdout instead of returning
						string to caller

	Notes:				May be modified or ditched as appropriate

	Modifications:
		Who			When			Description
	----------	---------------	--------------------
	Alwyn Teh	2 November 1992	Initial	Creation

*********************************************************************-*/
/* Parameter definition. */
static
ATP_DCL_PARMDEF(Atp_DebugMode_Parms)
	BEGIN_PARMS
		opt_bool_def("debug_mode", "debug mode switch", 1, NULL)
	END_PARMS
ATP_END_DCL_PARMDEF

Atp_ParmDefEntry *Atp_DebugModeParmsPtr = (Atp_ParmDefEntry *) Atp_DebugMode_Parms;

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_DebugModeCmd(void *dummy, ...)
#else
Atp_Result Atp_DebugModeCmd()
#endif
{
	Atp_DebugMode = Atp_Bool("debug_mode");

	(void) fprintf( stdout, "ATP Debug mode = %s\n",
					(Atp_DebugMode) ? "ON" : "OFF" );

	(void) fflush(stdout);

	return ATP_OK;
}

/* For debugging memory operations. */
#ifdef ATP_MEM_DEBUG
#if defined(__STDC__) || defined (__cplusplus)
char * Atp_MemDebug_Strdup ( char *s, char *filename, int line )
#else
char * Atp_MemDebug_Strdup(s,filename,line)
	char	*s;
	char	*filename;
	int		line;
#endif
{
	int size = strlen(s);
	char *copy = NULL;

	copy = Tcl_DbCkalloc(size+1,filename,line);
	copy = strcpy(copy,s);

	return copy;
}

#if defined(__STDC__) || defined(__cplusplus)
char * Atp_MemDebug_Calloc (size_t nelem, size_t elsize,
							char *filename, int line)
#else
char * Atp_MemDebug_Calloc(nelem,elsize,filename,line)
	size_t	nelem, elsize;
	char	*filename;
	int		line;
#endif
{
	size_t size = 0;
	char *newmem = NULL;

	size = nelem * elsize;

	newmem = Tcl_DbCkalloc(size, filename, line);

	newmem = (char *) memset((void *)newmem, 0, size);

	return newmem;
}

#endif /* ATP_MEM_DEBUG */
