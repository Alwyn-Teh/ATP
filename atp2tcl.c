/* EDITION AC11 (REL002), ITD ACST.179 (95/06/22 11:50:00) -- OPEN */

/*+*******************************************************************

	Module Name:		atp2tcl.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / NORTEL

	Description:		This module contains the ATP-to-Tcl	Adaptor
						Interface functions.

*******************************************************************_*/

#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <tcl.h>

#if (TCL_MAJOR_VERSION < 7)
#	include <tclHash.h>
#endif
#if defined(__STDC__) || defined(__cplusplus)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

#include "atp2tclh.h"

/*
 * Copyright information on Tcl.
 */
char ** Tcl_Copyright = NULL;

static char tclversion[10];

static char tclheader[40] = "Tcl (Tool Command Language) v%s\0\0\0\0\0\0\0\0\0";

static char	*author		= "	Author:			Prof. John K. Ousterhout";
static char	*email		= "	Email:			john.ousterhout@eng.sun.com";
static char	*authorwww	= "	Home Page:		http://www.sunlabs.com/people/john.ousterhout/";
static char	*anonftpsun	= "	Anonymous		ftp: ftp.smli.com:/pub/tcl";
static char *anonftpucb = "					ftp.cs.berkeley.edu:/ucb/tcl";
static char *tclwwwpage = " Tcl Home Page:	http://www.sunlabs.com/research/tcl/\n";

static char *Tcl_Copyright_8_6[] = {
		NULL, NULL, NULL, NULL, NULL, NULL, NULL,

"		This software is copyrighted by the Regents of the University of",
"		California, Sun Microsystems, Inc., Scriptics Corporation, ActiveState",
"		Corporation and other parties.  The following terms apply to all files",
"		associated with the software unless explicitly disclaimed in",
"		individual files.",
"",
"		The authors hereby grant permission to use, copy, modify, distribute,",
"		and license this software and its documentation for any purpose, provided",
"		that existing copyright notices are retained in all copies and that this",
"		notice is included verbatim in any distributions. No written agreement,",
"		license, or royalty fee is required for any of the authorized uses.",
"		Modifications to this software may be copyrighted by their authors",
"		and need not follow the licensing terms described here, provided that",
"		the new terms are clearly indicated on the first page of each file where",
"		they apply.",
"",
"		IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY",
"		FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES",
"		ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY",
"		DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE",
"		POSSIBILITY OF SUCH DAMAGE.",
"",
"		THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,",
"		INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,",
"		FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE",
"		IS PROVIDED ON AN \"AS IS\" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE",
"		NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR",
"		MODIFICATIONS.",
"",
"		GOVERNMENT USE: If you are acquiring this software on behalf of the",
"		U.S. government, the Government shall have only \"Restricted Rights\"",
"		in the software and related documentation as defined in the Federal",
"		Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you",
"		are acquiring the software on behalf of the Department of Defense, the",
"		software shall be classified as \"Commercial Computer Software\" and the",
"		Government shall have only \"Restricted Rights\" as defined in Clause",
"		252.227-7014 (b) (3) of DFARs.  Notwithstanding the foregoing, the",
"		authors grant the U.S. Government and others acting in its behalf",
"		permission to use and distribute the software in accordance with the",
"		terms specified in this license.",
""
};

/*
	The following cloned and "hacked" data structures must mirror those
	of struct Namespace and struct Command found in tclInt.h
	respectively.
*/
typedef struct Hacked_Namespace {
	char					*filler_1;
	char					*filler_2;
	ClientData				filler_3;
	Tcl_NamespaceDeleteProc	*filler_4;
	struct Hacked_Namespace	*filler_5;
	Tcl_HashTable			filler_6;
	long					filler_7;
	Tcl_Interp				*filler_8;
	int						filler_9;
	int						filler_10;
	int						filler_11;
	Tcl_HashTable			cmdTable;
} Hacked_Namespace;

typedef struct Hacked_Command_8_6 {
    Tcl_HashEntry		*filler_A;
    Hacked_Namespace	*filler_B;
    int					filler_C;
    int					filler_D;
    void				*filler_E;
    Tcl_ObjCmdProc		*filler_F;
    ClientData			filler_G;
    Tcl_CmdProc			*filler_H;
    ClientData			clientData;
} Hacked_Command_8_6;

/* Tcl callback interface returning Atp_Result instead of int */
typedef Atp_Result	(*Atp2Tcl_CallbackType)_PROTO_((ClientData clientdata,
													Tcl_Interp *interp,
													int argc,
													char **argv));

/* Forward declarations for local functions. */
static	Tcl_HashTable *	Atp2Tcl_GetTclCommandTable _PROTO_((Tcl_Interp *));
static	void *			Atp2Tcl_FirstCmdTabEntry
										_PROTO_((Atp_CmdTabAccessType *));
static	void *			Atp2Tcl_NextCmdTabEntry
										_PROTO_((Atp_CmdTabAccessType *));
static	void *			Atp2Tcl_FindCmdTabEntry
										_PROTO_((Atp_CmdTabAccessType *,
												 void *key));
static char *			Atp2Tcl_GetCmdName _PROTO_((void *tablePtr,
													void *entryPtr));
static char *			Atp2Tcl_GetCmdDesc _PROTO_((void *entryPtr));
static void *			Atp2Tcl_GetCmdRecord _PROTO_((void *entryPtr));
static int				Atp2Tcl_AtpAdaptorUsed _PROTO_( (void *));
static void *			Atp2Tcl_ExtractClientData _PROTO_((void *, ...));
static void *			Atp2Tcl_GetCmdTabAccessRecord _PROTO_((void *));
static void				Atp2Tcl_SetResultString _PROTO_((char *, ...));
static Atp_Result		Atp2Tcl_GetTclManpage _PROTO_ ( (void *, ...));
static Atp_Result		Atp2Tcl_HelpCmdGlueCallback	_PROTO_((char *));
static Atp_Result		Atp_HelpCmdGlueProc _PROTO_((ClientData, Tcl_Interp *,
													 int, char * *));
static int				isBuiltInTclCmd _PROTO_((char *cmdname));
int						Atp2Tcl_UnknownCmd _PROTO_((ClientData clientdata, Tcl_Interp *interp,
													int argc, char **argv));

/* Variables */
static int	misc_helpid = 0;
static char * ManCmd_TclManPageHeader[] =	{
	"Accessing Tcl (Tool Command Language) manual entry:",
	"    The command \"man Tcl\" may be used to search for and display",
	"    the manual entry for Tcl. The search path used should be",
	"    standard or specified in the $MANPATH environment variable.\n",

	"See also:",
	"    help -key <keyword> - search by keyword",
	"    Tcl's \"exec\" command - usage e.g. \"exec man sh\"",

	NULL
};

void * (*Atp_GetExternalClientData)
				_PROTO_((_ATP_EXT_CMD_CALLBACK_INTERFACE_)) = NULL;

/* externs */

/* "help -lang" parmdef description strings (atphelpc.c) */
EXTERN char Atp_HelpLangCaseDesc[120];
EXTERN char Atp_HelpLangOptStrDesc[80];

/* Pointer to "help" command parameter table. */
EXTERN Atp_ParmDefEntry *Atp_HelpCmdParmsPtr;

/* Pointer to command record for "help" command. */
EXTERN Atp_CmdRec *Atp_HelpCmdRecPtr;
EXTERN char	**Atp_HiddenCommands;
EXTERN char * Atp_VerifyCmdRecParmDef _PROTO_(( Atp_CmdRec *Atp_CmdRecPtr ));
EXTERN Atp_Result (*Atp_GetFrontEndManpage) _PROTO_((void *, ...));
EXTERN Atp_CmdRec * Atp_FindCommand _PROTO_(( void *ptr, int mode ));
							/* 0 = find record, 1 = find command name */

/* Declare any hidden commands */
#ifdef ATP_MEM_DEBUG
static char *Atp2Tcl_HiddenCommands[] = {	"unknown",
											ATP_MEMDEBUG_CMD,
											"memory",
											NULL };
#else
static char *Atp2Tcl_HiddenCommands[] = {	"unknown",
											NULL };

#define MALLOC(size,fn)			malloc((size))
#define REALLOC(ptr,size,fn)	realloc((ptr),(size))
#define CALLOC(nelem,elsize,fn)	calloc((nelem),(elsize))
#define FREE(ptr)				free((ptr))

#endif

/*+*******************************************************************

	Function Name:		Atp2Tcl_InternalCreateCommand

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		This is where commands are created. Here,
						tell ATP and Tcl about the command to be
						created. Any command checking appropriate
						at creation time may be performed here.

	Parameters:			Superset of Tcl and ATP parameters for the
						interface, plus invisible arguments
						inserted by macro.

	Global Variables:	None - and shouldn't be	any

	Results:			Atp_Result

	Calls:				Atp_AssembleCmdRecord, Atp_RegisterCommand,
						Atp_VerifyCmdRecParmDef, Atp_AddCmdToHelpArea,
						Tcl_CreateCommand (obviously!).

	Called by:			Atp2Tcl_CreateCommand()	macro in application

	Side effects:		An entry is made in the ATP command
						register. Attempts to verify the parmdef.
						Command may be added to help area of
						on-line help system.

	Notes:				Atp2Tcl_CreateCommand()	can be #defined	to
						be Tcl_CreateCommand() directly. Here, it
						is #defined to call
						Atp2Tcl_InternalCreateCommand(). This is
						so that the parmdef table could be
						verified at command creation time before
						calling Tcl_CreateCommand().

Modifications:
		Who		    When				Description
------------- ----------------	----------------------------------
	Alwyn Teh	9 October 1992	Initial Creation
	Alwyn Teh	25 June 1993	Add command to help	area
	Alwyn Teh	27 June 1993	Return CmdRecPtr in
								return_cmd_rec if requested,
								create command in lower case.
	Alwyn Teh	1 July 1993		Return ATP_ERROR if no name.
	Alwyn Teh	9 July 1993		Allow help_id of 0 and still
								create command.
	Alwyn Teh	23 July 1993	Tweak error messages. Don't
								output filename/line number
								if not available. Use SYSTEM
								ERROR prefix properly, and
								left align err msgs.

*******************************************************************_*/

static char SysErrPrefix[]	 = "SYSTEM ERROR: ";
static char LeftAlignSpace[] = "              ";

/*
 *	Note that these error messages do not reside in atperror.c because this
 *	is the adaptor interface module. ATP itself does not know about Tcl.
 */
static char *Atp2Tcl_CreateCommand_errmsg [] = {
		"Atp2Tcl_CreateCommand() error in file %s (line %d)\n",
		"Cannot add command \"%s\" to help area id %d.\n",
		"Cannot create command \"%s\".\n",
		NULL
};

#if defined (__STDC__) || defined(__cplusplus)
Atp_Result Atp2Tcl_InternalCreateCommand
(
	Tcl_Interp			*interp,
	char				*name,
	char				*desc,
	int					help_id,
	Atp_Result			(*callback)(ClientData, Tcl_Interp *, int, char **),
	Atp_ParmDef			parmdef,
	int					sizeof_parmdef, /* number of parmdef entries */
	ClientData			clientdata,
	Tcl_CmdDeleteProc	*deleteproc,
	char				*filename,
	int					linenumber,
	Atp_CmdRec			**return_cmd_rec
)
#else
Atp_Result
Atp2Tcl_InternalCreateCommand(interp, name, desc, help_id,
							  callback, parmdef,
							  sizeof_parmdef, clientdata, deleteproc,
							  filename, linenumber, return_cmd_rec)
	Tcl_Interp			*interp;
	char				*name, *desc;
	int					help_id;
	Atp_Result			(*callback)();
	Atp_ParmDef			parmdef;
	int					sizeof_parmdef;	/* number of parmdef entries */
	ClientData			clientdata;
	Tcl_CmdDeleteProc	*deleteproc;
	char				*filename;
	int					linenumber;
	Atp_CmdRec			**return_cmd_rec;
#endif
{
		Atp_CmdRec	*CmdRecPtr = NULL;
		char		*result_string	=	NULL;

		if (name == NULL || *name == '\0')
		  return ATP_ERROR;

		CmdRecPtr = Atp_AssembleCmdRecord(name, desc,
										  (Atp_CmdCallBackType)callback,
										  parmdef, sizeof_parmdef,
										  (void *)clientdata);

		if (return_cmd_rec != NULL)
		  *return_cmd_rec = NULL; /* initial cmd rec return value */

		Atp_RegisterCommand((void *)interp, CmdRecPtr);

		result_string = Atp_VerifyCmdRecParmDef(CmdRecPtr);

		if (result_string == NULL) {
			/*
			 *	If help_id is 0, then command is not to be associated
			 *	with a help area. We can still go ahead and create
			 *	command.
			 */
			if (help_id != 0)
			{
				if (Atp_AddCmdToHelpArea(help_id, CmdRecPtr) == 0) {
				  (void) fprintf(stderr, SysErrPrefix);
				  if (filename != NULL) {
					(void) fprintf(stderr, Atp2Tcl_CreateCommand_errmsg[0],
								   filename, linenumber);
					(void) fprintf(stderr, LeftAlignSpace);
				  }
				  (void) fprintf(stderr, Atp2Tcl_CreateCommand_errmsg[1],
								 CmdRecPtr->cmdName, help_id);
				  (void) fflush(stderr);

				  return ATP_ERROR;
				}
			}

			Tcl_CreateCommand(interp, name, Atp2Tcl_Adaptor,
							  (ClientData)CmdRecPtr, deleteproc);

			if (return_cmd_rec != NULL)
			  *return_cmd_rec = CmdRecPtr;

			return ATP_OK;
		}
		else {
			(void) fprintf(stderr, SysErrPrefix);
			if (filename != NULL) {
			  (void) fprintf(stderr, Atp2Tcl_CreateCommand_errmsg[0],
					  	  	 filename, linenumber);
			  (void) fprintf (stderr, LeftAlignSpace);
			}
			(void) fprintf(stderr, "%s\n", result_string);
			(void) fprintf(stderr, LeftAlignSpace);
			(void) fprintf(stderr, Atp2Tcl_CreateCommand_errmsg[2],
								   CmdRecPtr->cmdName);
			(void) fflush(stderr);

			FREE(result_string);

			return ATP_ERROR;
		}

		return ATP_OK;
}

/*+*******************************************************************

	Function Name:		Atp2Tcl_Adaptor

	Copyright:			BNR Europe Limited, 1992, 1993, 1994
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		This function is the interface adaptor
						between ATP and Tcl.

						It is supplied by Atp2Tcl_CreateCommand() to
						Tcl_CreateCommand() as the callback function
						for Tcl to call when an Atp2Tcl command is
						found. The actual command callback is
						inserted in the client data field. (See
						Atp_AssembleCmdRecord() in atpinit.c)

						Atp2Tcl_Adaptor will initiate the processing
						of Tcl parsed parameter tokens. When this
						is completed, it calls the command callback
						which will be able to access ATP parsed
						parameters using the provided parameter
						retrieval functions.

	Parameters:			Conforms to Tcl callback parameter
						interface, i.e. clientData, interp, argc,
						argv

	Results:			Returns any valid Tcl command callback
						return code (int), i.e. TCL_OK, TCL_ERROR,
						TCL_RETURN, TCL_BREAK or TCL_CONTINUE.

	Calls:				Atp_ProcessParameters, Atp_ExecuteCallback,
						Tcl_SetResult.

	Called by:			Tcl_Eval(),	Tcl_RecordAndEval(), ... etc.

	Side effects:		None

	Notes:				None

	Modifications:
		Who			When				Description
	----------- --------------	-----------------------------
	Alwyn Teh	30 June 1992	Initial Creation
	Alwyn Teh	7 June 1993		Tidy-up
	Alwyn Teh	27 July 1994	Remove duplicated use of
								Atp_PageLongerThanScreen()
								and Atp_PagingNeeded since
								Atp_OutputPager() already
								handles that.

*********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)

	int Atp2Tcl_Adaptor
	(
		ClientData	clientData,
		Tcl_Interp	*interp,
		int			argc,
		char		*argv[]
	)
#else
	int Atp2Tcl_Adaptor(clientData, interp, argc, argv)
		ClientData	clientData;
		Tcl_Interp	*interp;
		int			argc;
		char		*argv[];
#endif
	{
		Atp_CmdRec	*CmdRecPtr;
		void		*parmstore = NULL;
		char		*return_string = NULL;
		int			result = ATP_OK;

		/* "Extract" embedded information from the client data field. */
		CmdRecPtr = (Atp_CmdRec *)clientData;

		/* Process parameter tokens */
		result = Atp_ProcessParameters(CmdRecPtr, argc, argv,
									   &return_string, &parmstore);

		if (result == ATP_OK) {
			/*
			 *	Atp_ExecuteCallbaek() calls the command callback with
			 *	the parmstore.
			 *
			 *	Pass the original client data as part of the Tcl
			 *	callback interface.
			 */
			result = Atp_ExecuteCallback(CmdRecPtr->callBack, parmstore,
										 (ClientData)CmdRecPtr->clientData,
										 interp, argc, argv);
		}
		else {
			/*
			 *	Return code "result" is ATP_ERROR or ATP_RETURN.
			 *	All returned result strings such as error messages and
			 *	help information page are dynamic so Tcl needs to be
			 *	informed. There should be no exceptions.
			 */
			Tcl_SetResult(interp, return_string, free);
		}

		/* Convert ATP's result code to TCL's result code. */
		result = (result == ATP_ERROR) ? TCL_ERROR : TCL_OK;

		return result;
	}

/*+*****************************************************************»*

	Function Name:		Atp2Tcl_Init
						Atp2Tcl_InterpDeleteProc
						Atp2Tcl_HelpCmdDeleteProc
						Do_Tcl_Version_Copyright_Stuff

	Copyright:			BNR Europe Limited, 1992, 1994, 1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		First ATP2TCL function called to initialise
						everything that needs to be in place before
						commands can be processed.

	Parameters:			Tcl_Interp

	Global Variables:	quite a few - read the code

	Results:			Atp_Result

	Calls:				a number of functions - read the code

	Called by:			application

	Side effects:		N/A

	Notes:				N/A

	Modifications:
		Who			When				Description
	----------- -----------------  	----------------------------
	Alwyn Teh	14 September 1992	Initial Creation
	Alwyn Teh	25 June 1993		Declare TCL_VERSION,
									add "unknown" command.
	Alwyn Teh	10 January 1994		Declare function for
									getting the Tcl manpage.
	Alwyn Teh	17	January 1994	Change malloc to MALLOC
									for memory debugging.
	Alwyn Teh	18	July 1994		Capitalize first letter
									of command description
	Alwyn Teh	28 July 1994		Command "?" discontinued.
	Alwyn Teh	9 August 1994		Hide the "debug" command
									when compiled using -g by
									not including it in the
									misc help area.
	Alwyn Teh	12 April 1995		Arrange for Tcl to cleanup
									ATP if/when interp is deleted.
	Alwyn Teh	22 June 1995		Introduce linkage compatibility
									with Tcl v7.3 and v7.4. Do not
									rely on compile time compat
									using TCL_VERSION macro...etc.
	Alwyn Teh	25 December 2021	Bring Atp2Tcl to be compatible
									with Tcl 8.6.12
	Alwyn Teh	27 December 2021	Use Tcl_GetStringResult(interp)
									instead of interp->result

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void Do_Tcl_Version_Copyright_Stuff(Tcl_Interp *interp)
#else
static void Do_Tcl_Version_Copyright_Stuff(interp)
	Tcl_Interp *interp;
#endif
{
	int result = 0;
	char *version_posn = NULL;

	/* This is the Tcl to ATP adaptor, identify the frontend as Tcl. */
	strcpy(NameOfFrontEndToAtpAdaptor, "Tcl");

	/* Dynamically obtain the Tcl version (was static/compile time). */
	if ( interp != NULL )
	{
		result = Tcl_Eval(interp, "info tclversion");
		if (result == TCL_OK) {
		  strcpy (tclversion, Tcl_GetStringResult(interp));
		}
		else {
		  sprintf(tclversion, "%s", TCL_VERSION); /* should never happen */
		}
	}
	else {
	  sprintf(tclversion, "%s", TCL_VERSION); /* should never happen */
	}

	VersionOfFrontEndToAtpAdaptor = tclversion;

	version_posn = strstr(tclheader, "%s");
	strcpy(version_posn, tclversion);

	/*
		Complete the contents of the Tcl copyright banner.
		Only one of the two banners will be used in one executable,
		whether libatp.a linked at compile time or run-time.
	*/
	Tcl_Copyright_8_6[0] = tclheader;
	Tcl_Copyright_8_6[1] = author;
	Tcl_Copyright_8_6[2] = email;
	Tcl_Copyright_8_6[3] = authorwww;
	Tcl_Copyright_8_6[4] = anonftpsun;
	Tcl_Copyright_8_6[5] = anonftpucb;
	Tcl_Copyright_8_6[6] = tclwwwpage;

	Tcl_Copyright = Tcl_Copyright_8_6;

	/* Check out the Interp structure. */
	if ( interp != NULL )
	{
		Tcl_HashTable *tablePtr = Atp2Tcl_GetTclCommandTable(interp);
		char *hashstats = NULL;
		hashstats = Tcl_HashStats(tablePtr);
		if ( hashstats != NULL )
		{
		  char *env_var = NULL;
		  env_var = getenv("_ATP2TCL_ENV_VAR_");
		  if ( env_var != NULL )
		    if ( strstr(env_var, "print_tcl_hashtable") != NULL )
			  printf("Tcl Command Hash Table:\n%s\n", hashstats);
		}
		else
		{
		  fprintf (stderr, "Unable to obtain Tcl command hash table.\n");
		  exit(1);
		}
	}
}

#if defined(__STDC__) || defined(__cplusplus)
static void Atp2Tcl_InterpDeleteProc(ClientData clientdata, Tcl_Interp *interp)
#else
static void Atp2Tcl_InterpDeleteProc(clientdata, interp)
	ClientData clientdata;
	Tcl_Interp *interp;
#endif
{
	Atp_CmdTabAccessType *Atp2Tcl_CmdTableAccessDesc;
	Atp2Tcl_CmdTableAccessDesc = (Atp_CmdTabAccessType *)clientdata;

	if (Atp2Tcl_CmdTableAccessDesc != NULL)
	{
		if (Atp2Tcl_CmdTableAccessDesc->TableSearch != NULL)
		  FREE(Atp2Tcl_CmdTableAccessDesc->TableSearch);
		FREE(Atp2Tcl_CmdTableAccessDesc);
	}

	Atp_DeleteCmdGrp((void *)interp);
}

#if defined (__STDC__) || defined (__cplusplus)
static void Atp2Tcl_HelpCmdDeleteProc(ClientData clientdata, Tcl_Interp *interp)
#else
static void Atp2Tcl_HelpCmdDeleteProc(clientdata, interp)
	ClientData clientdata;
	Tcl_Interp *interp;
#endif
{
		Atp_HelpCmdDeleteProc();
}

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp2Tcl_Init( Tcl_Interp *interp )
#else
Atp_Result Atp2Tcl_Init(interp)
	Tcl_Interp *interp;
#endif
{
	Tcl_HashTable			*tablePtr;
	Atp_CmdTabAccessType	*Atp2Tcl_CmdTableAccessDesc;

	if ( interp == NULL )
	{
		(void) fprintf(stderr, "Cannot initialize ATP with Tcl.\n");
		(void) fprintf(stderr, "Atp2Tcl_Init(Tcl_Interp * = 0); !!!\n");
		(void) fflush(stderr);
		return ATP_ERROR;
	}

	/* Declare the name and version of the front-end tokeniser used. */
	Do_Tcl_Version_Copyright_Stuff(interp);

	/*
	 *	Obtain space for a Atp_CmdTabAccessType record as there
	 *	may be more than one interpreter in an application.
	 */
	Atp2Tcl_CmdTableAccessDesc =
	  (Atp_CmdTabAccessType *) MALLOC(sizeof (Atp_CmdTabAccessType) , NULL);

	if (Atp2Tcl_CmdTableAccessDesc == NULL)
	  return ATP_ERROR;

	(void) memset(Atp2Tcl_CmdTableAccessDesc, 0,
					sizeof (Atp_CmdTabAccessType));

	/* Extract pointer to command table from the interpreter. */
	tablePtr = Atp2Tcl_GetTclCommandTable(interp);

	/*
	 *	Register command table pointer with the command table access
	 *	descriptor.
	 */
	Atp2Tcl_CmdTableAccessDesc->CommandTablePtr = (void *) tablePtr;

	/*
	 *	Provide callback functions for ATP to access the table
	 *	entries.
	 */
	Atp2Tcl_CmdTableAccessDesc->FirstCmdTabEntry =
									Atp2Tcl_FirstCmdTabEntry;
	Atp2Tcl_CmdTableAccessDesc->NextCmdTabEntry =
									Atp2Tcl_NextCmdTabEntry;
	Atp2Tcl_CmdTableAccessDesc->FindCmdTabEntry =
									Atp2Tcl_FindCmdTabEntry;

	/* Get space for table search structure. */
	Atp_IsLangBuiltInCmd = isBuiltInTclCmd;

	/* Get space for table search structure. */
	Atp2Tcl_CmdTableAccessDesc->TableSearch =
			(Tcl_HashSearch *) MALLOC(sizeof(Tcl_HashSearch), NULL);
	if (Atp2Tcl_CmdTableAccessDesc->TableSearch == NULL)
	  return ATP_ERROR;
	(void) memset(Atp2Tcl_CmdTableAccessDesc->TableSearch, 0,
					sizeof(Tcl_HashSearch));

	/*
	 *	Register functions for returning command name and description.
	 */
	Atp2Tcl_CmdTableAccessDesc->CmdName	= Atp2Tcl_GetCmdName;
	Atp2Tcl_CmdTableAccessDesc->CmdDesc	= Atp2Tcl_GetCmdDesc;
	Atp2Tcl_CmdTableAccessDesc->CmdRec	= Atp2Tcl_GetCmdRecord;

	/* Initialise ATP Adaptor indicator function. */
	Atp_AdaptorUsed = Atp2Tcl_AtpAdaptorUsed;

	/* Initialise ATP client data extraction function. */
	Atp_GetExternalClientData = Atp2Tcl_ExtractClientData;

	/*
	 *	Initialise ATP command table access record retrieval function.
	 */
	Atp_GetCmdTabAccessRecord = Atp2Tcl_GetCmdTabAccessRecord;

	/* Initialise ATP help page command return function. */
	Atp_ReturnDynamicHelpPage = Atp2Tcl_SetResultString;

	/* Initialise ATP adaptor return string function. */
	Atp_ReturnDynamicStringToAdaptor = Atp2Tcl_SetResultString;

	/*
	 *	Tell ATP what the return code values for OK and ERROR are
	 *	for the frontend interface.
	 */
	Atp_Adaptor_FrontEnd_ReturnCode_OK		= TCL_OK;
	Atp_Adaptor_FrontEnd_ReturnCode_ERROR	= TCL_ERROR;

	/* Initialise any other ATP internal things. */
	Atp_Initialise();

	/* Arrange for Tcl to cleanup ATP if/when the Tcl interp is deleted. */
	Tcl_CallWhenDeleted(interp, Atp2Tcl_InterpDeleteProc,
						(ClientData)Atp2Tcl_CmdTableAccessDesc);

	/* Get Help Area ID for built-in commands. */
	misc_helpid = Atp_CreateHelpArea(ATP_HELPCMD_OPTION_MISC, NULL);

	/* Print "Tcl" in "help -lang" parmdef description strings. */
	{
		char *fmtstr = strdup(Atp_HelpLangCaseDesc);
		sprintf(Atp_HelpLangCaseDesc, fmtstr, NameOfFrontEndToAtpAdaptor);
		free(fmtstr);
		fmtstr = strdup(Atp_HelpLangOptStrDesc);
		sprintf(Atp_HelpLangOptStrDesc, fmtstr, NameOfFrontEndToAtpAdaptor);
		free(fmtstr);
	}

	/* Create help command first. */
	(void)
	Atp2Tcl_InternalCreateCommand(interp, ATP_HELP_CMDNAME,
								  "HELP Information",
								  misc_helpid,
								  Atp_HelpCmdGlueProc,
								  Atp_HelpCmdParmsPtr,
								  Atp_NoOfPDentries(Atp_HelpCmdParmsPtr),
								  (ClientData) Atp2Tcl_CmdTableAccessDesc,
								  (Tcl_CmdDeleteProc *) NULL,
#ifdef DEBUG
								  __FILE__, __LINE__,
#else
								  NULL, 0,
#endif
								  &Atp_HelpCmdRecPtr) ;

	/*
	 *	Arrange for Tcl to cleanup "help" command contents
	 *	(e.g. help areas) if/when the Tcl interp is deleted.
	 *	There is only one "help" command in an application.
	 *	So, multiple Tcl interpreters NOT supported yet!
	 */
	Tcl_CallWhenDeleted(interp, Atp2Tcl_HelpCmdDeleteProc, (ClientData)0);

	/*
	 *	Create On-line Man Page generating command for command
	 *	parmdefs.
	 */
	(void) Atp2Tcl_CreateCommand(interp, ATP_MANPAGE_CMDNAME,
						"Generates manpage for command automatically",
						misc_helpid,
						(Atp2Tcl_CallbackType)Atp_ManPageCmd,
						Atp_ManPage_PD_ptr,
						(ClientData) Atp2Tcl_CmdTableAccessDesc,
						(Tcl_CmdDeleteProc *) NULL);

	(void) Atp_AddHelpInfo(ATP_MANPAGE_HEADER, ATP_MANPAGE_CMDNAME,
						   Atp_ManPageHeader);

#ifdef DEBUG
	{
		(void) Atp_CreateHelpArea(ATP_HELPCMD_OPTION_MISC, NULL);

		/* Create debug mode command. */
		(void) Atp2Tcl_CreateCommand(interp, "debug",
						"Sets ATP Debug Mode to ON or OFF",
						0, (Atp2Tcl_CallbackType)Atp_DebugModeCmd,
						Atp_DebugModeParmsPtr,
						(ClientData) "debug",
						(Tcl_CmdDeleteProc *) NULL);
	}
#endif
	/*
	 *	Create "unknown" command primarily to search ATP's command
	 *	register in the event when Tcl doesn't recognise a command
	 *	because it's case sensitive. This means that, however, the
	 *	application cannot create another "unknown" command,
	 *	otherwise, it will replace this one.
	 */
	Tcl_CreateCommand(interp, "unknown",
					  (Atp2Tcl_CallbackType)Atp2Tcl_UnknownCmd,
					  (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

	Atp_HiddenCommands = Atp2Tcl_HiddenCommands; /* hide unknown command */

	/*
	 *	Register function with ATP which obtains the manpage, if any, of
	 *	the frontend language processor. Also, add help info on this.
	 */
	Atp_GetFrontEndManpage = Atp2Tcl_GetTclManpage;
	(void) Atp_AddHelpInfo(ATP_MANPAGE_HEADER, ATP_MANPAGE_CMDNAME,
						   ManCmd_TclManPageHeader);

	return ATP_OK;
}

/*+******************************************************************

	Function Name:		Atp2Tcl_UnknownCmd

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Provides second attempt to recognise command
						in ATP's own command register, ignoring
						case. If found, let the adaptor deal with
						it.	In other words, provides
						case-insensitive capability.

	Parameters:			Standard Tcl argument list

	Global Variables:	None

	Results:			int - Tcl result

	Calls:				Atp_FindCommand, Tcl_ResetResult,
						Tcl_AppendResult, Atp2Tcl_Adaptor.

	Called by:			from within Tcl - see "unknown" command facility

	Side effects:		N/A

	Notes:				What happens when you type in "unknown"?

	Modifications:
		Who			When				Description
	----------	--------------	-------------------------------
	Alwyn Teh	25 June 1993	Initial Creation
	Alwyn Teh	3 January 1994	Fix error msg when
								"unknown" entered and
								argv[1] = nil
	Alwyn Teh	4 January 1994	Remove static function
								declaration so as to
								make available externally

*************************************************************************/
#if defined (__STDC__) || defined(__cplusplus)
int Atp2Tcl_UnknownCmd
(
	ClientData	clientData,
	Tcl_Interp	*interp,
	int			argc,
	char		*argv[]
)
#else
int Atp2Tcl_UnknownCmd(clientData, interp, argc, argv)
	ClientData	clientData
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	int			result = TCL_OK;
	Atp_CmdRec	*cmdRecPtr = NULL;

	cmdRecPtr = Atp_FindCommand(argv[1], 1);

	if (cmdRecPtr == NULL) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "Invalid command name: \"",
						 (argv[1] == NULL) ? "" : argv[1], "\"", (char *)NULL);
		result = TCL_ERROR;
	}
	else {
		clientData = (ClientData) cmdRecPtr;
		result = Atp2Tcl_Adaptor(clientData, interp, argc-1, &argv[1]);
	}

	return result;
}

/*+*****************************************************************

	Function Name:		Atp2Tcl_GetPager

	Copyright:			BNR Europe Limited, 1993, 1994
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Provides output paging capability by
						creating the built-in "paging" and "pager"
						commands, then returning a pointer to the
						pager function to the application caller to
						use instead of a printf function.

	Parameters:			Tcl_Interp

	Global Variables:	Atp_OutputPager

	Results:			PFI - Pointer to Function returning Int

	Calls:				Atp2Tcl_CreateCommand

	Called by:			application (optional)

	Side effects:		see description above

	Notes:				N/A

	Modifications:
		Who			When					Description
	----------	-------------	------------------------------------
	Alwyn Teh	24 June 1993	Initial Creation
	Alwyn Teh	18 July	1994	Capitalize first letter
								of command description
	Alwyn Teh	27 July	1994	Rewrite description for
								paging command (it doesn't
								toggle anything).

********************************************************************/
#if defined(__STDC__) || defined(__cplusplus)
PFI Atp2Tcl_GetPager( Tcl_Interp *interp )
#else
PFI
Atp2Tcl_GetPager(interp)
	Tcl_Interp *interp;
#endif
{
	PFI pager_ptr = Atp_OutputPager;

	Atp_Result rc;

	if (interp == NULL) {
	  return NULL;
	}
	else
	{
		rc = Atp2Tcl_CreateCommand(interp, "paging",
								   "Set/query paging mode (ON, OFF or AUTO)",
								   misc_helpid,
								   (Atp2Tcl_CallbackType)Atp_PagingCmd,
								   Atp_PagingParmsPtr,
								   (ClientData) 0,
								   (Tcl_CmdDeleteProc *) 0);
		if (rc == ATP_OK) {
		  rc = Atp2Tcl_CreateCommand( interp, "pager",
									  "Set output pager to use",
									  misc_helpid,
									  (Atp2Tcl_CallbackType)Atp_PagerCmd,
									  Atp_PagerParmsPtr,
									  (ClientData) 0,
									  (Tcl_CmdDeleteProc *) 0);
		}

		if (rc == ATP_OK) {
		  return pager_ptr;
		}
	}

	return NULL;
}

/**********************************************************************

	Function Name:		Atp2Tcl_GetTclCommandTable

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Gets the Tcl command table by hacking at the
						Tcl_Interp/Tcl_Namespace structure.

	Parameters:			Tcl_Interp

	Global Variables:	None

	Results:			pointer to Tcl_HashTable

	Calls:				None

	Called by:			Atp2Tcl_Init, Atp2Tcl_AddHelpInfo

	Side effects:		N/A - always check for changes to Tcl_Interp
						in upgrade releases to Tcl

	Notes:				Purpose is to be able to gain access to the
						Tcl command register by the on-line help
						system.

	Modifications:
		Who			When					Description
	----------	------------------	-----------------------------
	Alwyn Teh	14 September 1992	Initial	Creation
	Alwyn Teh	25 December 2021	Changed getting command table
									from Interp to Namespace

**********************************************************************/
#if defined(__STDC__) || defined(__cplusplus)
static Tcl_HashTable *
Atp2Tcl_GetTclCommandTable( Tcl_Interp *interp )
#else
static Tcl_HashTable *
Atp2Tcl_GetTclCommandTable(interp)
	Tcl_Interp *interp;
#endif
{
	Tcl_Namespace *	tcl_namespace = Tcl_GetGlobalNamespace(interp);
	Hacked_Namespace * hacked_namespace = (Hacked_Namespace *) tcl_namespace;

	Tcl_HashTable * commandTablePtr = &hacked_namespace->cmdTable;

	return commandTablePtr;
}

/*+*******************************************************************

	Function Name:		Atp2Tcl_ExtractClientData

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Extracts the clientdata field from the relayed
						function call stack frame. This is achieved
						indirectly via Atp_GetExternalClientData. This
						ensures ATP remains independent of Tcl.

	Parameters:			stack frame, here, "cast" as ClientData by
						function definition

	Global Variables:	N/A

	Results:			returns clientdata as void pointer

	Calls:				N/A

	Called by:			N/A

	Side effects:		N/A

	Notes:				At present, the notion of a clientdata field is
						used by Tcl and is used by ATP for passing
						information. If a different frontend language
						is used which doesn't have this, some clever
						redesign to cope with both situations would be
						necessary.

	Modifications:
		Who			When					Description
	----------	------------------	-----------------------------
	Alwyn Teh	15 September 1992	Initial	Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void *
Atp2Tcl_ExtractClientData( void * clientData, ... )
#else
static void *
Atp2Tcl_ExtractClientData(c1ientData)
	void *	clientData;
#endif
/*
 * Unused arguments ignored: Tcl_Interp *interp; int argc; char *argv[];
 */
{
	return clientData;
}

/*+*******************************************************************

	Function Name:		Atp2Tcl_GetCmdTabAccessRecord

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Interface function for giving direct but
						isolated access to command table record.
						(see Atp_CmdTabAccessType in atph.h)

	Parameters:			clientdata obtained by Atp_GetExternalClientData

	Global Variables:	None

	Results:			void * but effectively pointer to Atp_CmdTabAccessType

	Calls:				N/A

	Called by:			via Atp_GetCmdTabAccessRecord by ATP help
						and manpage system

	Side effects:		N/A

	Notes:				N/A

	Modifications:
		Who			When					Description
	----------	------------------	-----------------------------
	Alwyn Teh	15 September 1992	Initial	Creation

********★***********************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
static void *
Atp2Tcl_GetCmdTabAccessRecord( void * clientData )
#else
static void *
Atp2Tcl_GetCmdTabAccessRecord(clientData)
void *	clientData;
#endif
{
	return clientData;
}

/*+*******************************************************************

	Function Name:		Atp2Tcl_FirstCmdTabEntry

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Obtains pointer to first entry in command register.

	Parameters:			Pointer to Atp_CmdTabAccessType record

	Global Variables:	None

	Results:			void * but effectively Tcl_HashEntry *
						or NULL if no entries found

	Calls:				Tcl_FirstHashEntry

	Called by:			ATP on-line help and manpage systems

	Side effects:		None

	Notes:				Read Tcl manpages on how to use its hash table

	Modifications:
		Who			When					Description
	----------	------------------	-----------------------------
	Alwyn Teh	14 September 1992	Initial	Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void *
Atp2Tcl_FirstCmdTabEntry( Atp_CmdTabAccessType *CmdTableAccessDescPtr )
#else
static void *
Atp2Tcl_FirstCmdTabEntry (CmdTableAccessDescPtr)
	Atp_CmdTabAccessType *CmdTableAccessDescPtr
#endif
{
	return (void *)
			Tcl_FirstHashEntry (
				(Tcl_HashTable *)CmdTableAccessDescPtr->CommandTablePtr,
				(Tcl_HashSearch *)CmdTableAccessDescPtr->TableSearch
			);
}

/*+*******************************************************************

	Function Name:		Atp2Tcl_NextCmdTabEntry

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Gets the next entry in Tcl's command table.

	Parameters:			Pointer to Atp_CmdTabAccessType	record

	Global Variables:	N/A

	Results:			void * (Pointer to next table entry Tcl_HashEntry *)
						or NULL if end of table

	Calls:				Tcl_NextHashEntry

	Called by:			ATP online help and manpage	system

	Side effects:		None

	Notes:				Read Tcl manpages on how to use its hash table

	Modifications:
		Who			When					Description
	----------	------------------	----------------------------
	Alwyn Teh	14 September 1992	Initial	Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void *
Atp2Tcl_NextCmdTabEntry( Atp_CmdTabAccessType *CmdTableAccessDescPtr )
#else
static void *
Atp2Tcl_NextCmdTabEntry (CmdTableAccessDescPtr)
	Atp_CmdTabAccessType *CmdTableAccessDescPtr;
#endif
{
	return (void *)
			Tcl_NextHashEntry((Tcl_HashSearch *)
								CmdTableAccessDescPtr->TableSearch);
}

/*+********************************************************************

	Function Name:		Atp2Tcl_FindCmdTabEntry

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Finds a specific entry in the command table
						given a key (i.e. command name).

	Parameters:			Pointer to Atp_CmdTabAccessType	record, and
						pointer to search key.

	Global Variables:	None

	Results:			void * (Pointer to table entry Tcl_HashEntry *)
						or NULL if not found

	Calls:				Tcl_FindHashEntry

	Called by:			ATP online help and manpage system

	Side effects:		N/A

	Notes:				Read Tcl manpages on how to use its hash table.
						Ensure that Tcl uses command name as key in hash
						table.
	Modifications:
		Who			When				Description
	----------	---------------	-------------------------------
	Alwyn Teh	8 October 1992	Initial	Creation

********************************************************************-*/
#if defined(__STDC__) || defined (_cplusplus)
static void *
Atp2Tcl_FindCmdTabEntry ( Atp_CmdTabAccessType *CmdTableAccessDescPtr,
						  void *key )
#else
static void *
Atp2Tcl_FindCmdTabEntry(CmdTableAccessDescPtr, key)
	Atp_CmdTabAccessType	*CmdTableAccessDescPtr;
	void					*key;
#endif
{
	Tcl_HashTable *tablePtr = (Tcl_HashTable *)
									CmdTableAccessDescPtr->CommandTablePtr;

	return Tcl_FindHashEntry(tablePtr, (char *)key);
}

/*+*******************************************************************

	Function Name:		Atp2Tcl_GetCmdName

	Copyright:			BNR Europe Limited,	1992
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Get a command's name given hash table and entry.

	Parameters:			Pointers to Tcl_HashTable and Tcl_HashEntry

	Global Variables:	None

	Results:			char * (command name)

	Calls:				Tcl_GetHashKey

	Called by:			ATP online help and	manpage system

	Side effects:		N/A

	Notes:				N/A

	Modifications:
		Who			When					Description
	-----------	------------------	------------------------------
	Alwyn Teh	15 September 1992	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static char *
Atp2Tcl_GetCmdName( void *tablePtr, void *entryPtr )
#else
static char *
Atp2Tcl_GetCmdName(tablePtr, entryPtr)
	void *tablePtr;
	void *entryPtr;
#endif
{
	char *cmdName;

	cmdName = (char *) Tcl_GetHashKey((Tcl_HashTable *)tablePtr,
									  (Tcl_HashEntry *)entryPtr);

	return cmdName;
}
/*+******************************************************************

	Function Name:		Atp2Tcl_GetCmdDesc
						Atp2Tcl_GetAtpCmdRec (22/6/95)

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Given the command table entry, extract the
						command's description. This is achieved by
						accessing the hash value. Tcl's command
						record, then extracting the clientdata
						field, which in turn leads to the ATP
						command record containing the command
						description!

	Parameters:			Tcl_HashEntry *

	Global Variables:	None

	Results:			char * (command description)

	Calls:				Tcl_GetHashValue, isAtpCommand

	Called by:			ATP online help and manpage system

	Side effects:		None

	Notes:				N/A

	Modifications:
		Who			When					Description
	----------	------------------	-----------------------------
	Alwyn Teh	16 September 1992	Initial Creation
	Alwyn Teh	22 June 1995		Support Tcl 7.3 and 7.4
									incompatible Command
									discrimination at runtime.
	Alwyn Teh	25 December 2021	Changed hacked Command version
									to Tcl 8.6

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static Atp_CmdRec *Atp2Tcl_GetAtpCmdRec( void *entryPtr )
#else
static Atp_CmdRec *Atp2Tcl_GetAtpCmdRec( entryPtr )
		void *entryPtr;
#endif
{
	void		*TclCmdPtr = NULL;
	Atp_CmdRec	*AtpCmdPtr = NULL;

	if ( entryPtr == NULL )
	  return NULL;

	TclCmdPtr = (void *) Tcl_GetHashValue((Tcl_HashEntry *)entryPtr);

	AtpCmdPtr = (Atp_CmdRec *)((Hacked_Command_8_6 *)TclCmdPtr)->clientData;

	return AtpCmdPtr;
}
#if defined (__STDC__) || defined (_cplusplus)
static char *
Atp2Tcl_GetCmdDesc( void *entryPtr )
#else
static char *
Atp2Tcl_GetCmdDesc(entryPtr)
	void *entryPtr;
#endif
{
	Atp_CmdRec *AtpCmdPtr = NULL;

	AtpCmdPtr = Atp2Tcl_GetAtpCmdRec(entryPtr);

	if (isAtpCommand(AtpCmdPtr))
	  return (char *) AtpCmdPtr->cmdDesc;
	else
	  return NULL;
}

/*+*******************************************************************

	Function Name:		Atp2Tcl_GetCmdRecord

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Given the command table entry, extract the
						ATP command record. This is achieved by
						accessing the hash value. Tcl's command
						record, then extracting the clientdata
						field, which in turn leads to the ATP
						command record itself.

	Parameters:			Pointer to Tcl_HashEntry

	Global Variables:	None

	Results:			void * (but effectively Atp_CmdRec *)

	Calls:				Tcl_GetHashValue, isAtpCommand

	Called by:			ATP online help and manpage system
						Atp2Tcl_AddHelpInfo

	Side effects:		N/A

	Notes:				N/A

	Modifications:
		Who			When				Description
	----------	---------------	-------------------------------
	Alwyn Teh	8 October 1992	Initial Creation
	Alwyn Teh	22 June 1995	Support Tcl 7.3 and 7.4
								incompatible Command
								discrimination at runtime.

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void *
Atp2Tcl_GetCmdRecord( void *entryPtr )
#else
static void *
Atp2Tcl_GetCmdRecord(entryPtr)
	void * entryPtr
#endif
{
	Atp_CmdRec *AtpCmdPtr = NULL;

	AtpCmdPtr = Atp2Tcl_GetAtpCmdRec(entryPtr);

	if (isAtpCommand(AtpCmdPtr))
	  return (void *) AtpCmdPtr;
	else
	  return NULL;
}

/*+*******************************************************************

	Function Name:		Atp2Tcl_AtpAdaptorUsed

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Verify whether a command table entry uses the
						ATP2TCL adaptor - i.e. is it an ATP command?

	Parameters:			Tcl_HashEntry *

	Global Variables:	None

	Results:			int - 1 or 0 (i.e. boolean TRUE	or FALSE)

	Calls:				Tcl_GetHashValue, isAtpCommand

	Called by:			ATP help system via AtpAdaptorUsed

	Side effects:		N/A

	Notes:				N/A

	Modifications:
		Who			When					Description
	----------	------------------	-----------------------------
	Alwyn Teh	16 September 1992	Initial Creation
	Alwyn Teh	22 June 1995		Support Tcl 7.3 and 7.4
									incompatible Command
									discrimination at runtime.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static int
Atp2Tcl_AtpAdaptorUsed( void *entryPtr )
#else
static int
Atp2Tcl_AtpAdaptorUsed(entryPtr)
		void *entryPtr;
#endif
{
	Atp_CmdRec *AtpCmdPtr = NULL;

	AtpCmdPtr = Atp2Tcl_GetAtpCmdRec(entryPtr);

	if (isAtpCommand(AtpCmdPtr))
	  return 1;
	else
	  return 0;
}

/*+****************+**************************************************

	Function Name:		Atp2Tcl_SetResultString

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		For returning result strings.

	Parameters:			string, ClientData, Tcl_Interp

	Global Variables:	N/A

	Results:			void

	Calls:				Tcl_SetResult

	Called by:			ATP help system
						Atp_ReturnDynamicHeIpPage
						Atp_ReturnDynamicStringToAdaptor

	Side effects:		Tcl handles string as dynamic and releases it

	Notes:				N/A

	Modifications:
		Who			When					Description
	----------	------------------	----------------------------
	Alwyn Teh	15 September 1992	Initial	Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void
Atp2Tcl_SetResultString( char *return_string, ... )
#else
static void
Atp2Tcl_SetResultString(return_string, va_alist)
	char *return_string;
	va_dcl
#endif
{
	va_list ap;
	Tcl_Interp *interp;

#if defined(__STDC__) || defined(__cplusplus)
	va_start(ap, return_string);
#else
	va_start(ap);
#endif

	(void)va_arg(ap, ClientData);

	interp = va_arg(ap, Tcl_Interp *);

	Tcl_SetResult(interp, return_string, TCL_VOLATILE);

	va_end(ap);
}

/*+****************+**************************************************

	Function Name:		Atp2Tcl_AddHelpInfo

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Adds English textual paragraph(s) to the
						help information of command manpages or help
						system help areas or help summary info for a
						command. This uses a general-purpose common
						reusable format.

	Parameters:			Tcl_Interp for accessing command table, text
						type code, name and text paragraphs.

	Global Variables:	None

	Results:			Atp_Result

	Calls:				Atp_AddHelpInfo, Atp2Tcl_GetTclCommandTable,
						Tcl_FindHashEntry, Atp2Tcl_GetCmdRecord,
						Atp_AppendCmdHelpInfo.

	Called by:			application

	Side effects:		N/A

	Notes:				N/A

	Modifications:
		Who			When				Description
	----------	-------------	------------------------------
	Alwyn Teh	10 June 1993	Initial Creation
	Alwyn Teh	29 June 1993	Change manpage descriptive
								paragraph to be manpage
								header. Add footer text,
								and general help info
								summary text for use by
								the "help" command.
	Alwyn Teh	15 July 1993	Support new text type
								ATP_HELP_AREA_SUMMARY but
								at present it does not
								tie in with any particular
								interp.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp2Tcl_AddHelpInfo
(
	Tcl_Interp	*interp,
	int			text_type,
	char		*HelpName,
	char		**DescText
)
#else
Atp_Result
Atp2Tcl_AddHelpInfo(interp, text_type, HelpName, DescText)
	Tcl_Interp	*interp;
	int			text_type;	/*	can	be ATP_HELP_SUMMARY,
								ATP_MANPAGE_HSADER or
								ATP_MANPAGE_FOOTER,
								and now ATP_HELP_AREA_SUMMARY */
	char		*HelpName, **DescText;
#endif
{
	Tcl_HashTable	*command_table;
	void			*CmdEntry;
	Atp_CmdRec		*CmdRecPtr;
	Atp_Result		result = ATP_OK;

	if (interp == NULL || HelpName == NULL || DescText == NULL)
	  return ATP_ERROR;

	if (text_type != ATP_HELP_SUMMARY &&
		text_type != ATP_MANPAGE_HEADER &&
		text_type != ATP_MANPAGE_FOOTER &&
		text_type != ATP_HELP_AREA_SUMMARY)
	return ATP_ERROR;

	/*
	 *	Help area summary info does not relate to one command.
	 *	Call Atp_AddHelpInfo{) to handle this case.
	 */
	if (text_type == ATP_HELP_AREA_SUMMARY) {
		result = Atp_AddHelpInfo(text_type, HelpName, DescText);
	}
	else
	{
		/* Get the Tcl command table. */
		command_table = Atp2Tcl_GetTclCommandTable(interp);
		if (command_table == NULL)
		  return ATP_ERROR;

		/* Find the command. */
		CmdEntry = Tcl_FindHashEntry(command_table, HelpName);
		if (CmdEntry == NULL)
		  return ATP_ERROR;

		/* See if it is an ATP command. */
		CmdRecPtr = (Atp_CmdRec *) Atp2Tcl_GetCmdRecord(CmdEntry);
		if (CmdRecPtr == NULL)
		  return ATP_ERROR;

		/* Enter the help text. */
		result = Atp_AppendCmdHelpInfo(CmdRecPtr, text_type, DescText);
	}

	return result;
}
/*+*****************************************************************

	Function Name:		Atp2Tcl_GetTclManpage

	Copyright:			BNR Europe Limited, 1994, 1995
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		Gets the Tcl manpage, if any.
						Looks in the $MANPATH environment variable.

	Modifications:
		Who			When					Description
	----------	----------------	---------------------------------
	Alwyn Teh	10 January 1994		Initial Creation
	Alwyn Teh	17 January 1994		Change strdup to Atp_Strdup
									for memory debugging
	Alwyn Teh	20 January 1994		Delete unused ClientData
									parameter
	Alwyn Teh	31 January 1994		Change arguments list.
									Return manpage via interp.
									This avoids ATP freeing manpage
									and causes core dump. Tcl frees
									the buffer eventually in next
									call to Tcl_AppendResult() or
									Tcl_AppendElement{). No need
									to strdup error message now.
	Alwyn Teh	18 July 1994		Port to Tcl v7.3
									Tcl_Eval() no longer has "flags"
									and "termPtr" arguments.
	Alwyn Teh	21 July 1994		Function to be callable from an
									ATP command for searching other
									Tcl command manpages.
	Alwyn Teh	25 July 1994		Handle Tcl procedures, otherwise
									wasted sh attempt to search in
									the system.
	Alwyn Teh	15 August 1994		Display any default values for
									arguments to a proc.
	Alwyn Teh	10	April 1995		Redirect stderr to /dev/null
									when getting Tcl manpages;
									due to operational problem
									on SUN platform.
	Alwyn Teh	27 December 2021	Use Tcl_GetStringResult(interp)
									instead of interp->result

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static Atp_Result Atp2Tcl_GetTclManpage( void * _clientData, ... )
#else
static Atp_Result Atp2Tcl_GetTclManpage(_clientData, va_alist)
	ClientData _clientData; /* not used */
	va_dcl
#endif
{
		va_list ap;
		ClientData clientData = (ClientData) _clientData;
		Tcl_Interp * interp;

#define ZeroTclTmpVars()	\
		Atp_DvsPrintf(&tmpcmd, "set __tmp__%d__ {}", pid); \
		Tcl_Eval(interp, tmpcmd); \
		FREE(tmpcmd); \
		Atp_DvsPrintf(&tmpcmd, "set __argv__%d__ {}", pid); \
		Tcl_Eval(interp, tmpcmd); \
		FREE(tmpcmd);

		char *synopsis_cmd =
			"foreach arg [info args %s] {\n\
				append __argv__%d__ { <} $arg {>}\n\
			}\n\
			set __argv__%d__";

		/*
		 *	Beware of args with no defaults, it's not the same as default
		 *	with value of empty string.
		 */
		char *defaults_cmd =
			"foreach arg [info args %s] {\n\
				if [info default %s $arg __tmp__%d__] then {\n\
				  lappend __argv__%d__ [list $arg $__tmp__%d__]\n\
				} else {\n\
				  lappend __argv__%d__ $arg\n\
				}\n\
			}; \n\
			set __argv__%d__\n" ;

		char *tmpcmd = NULL;
		pid_t pid = getpid();
		int result = ATP_OK;
		int isTclSummaryManpage = 0;
		char *name = Atp_Str("name"); /* as declared in "man"+"help" parmdef */
#if defined(__STDC__) || defined(__cplusplus)
		va_start(ap, _clientData);
#else
		va_start(ap);
#endif
		interp = va_arg(ap, Tcl_Interp *);
		if (Atp_Strcmp(NameOfFrontEndToAtpAdaptor, name) == 0) {
		  name = NameOfFrontEndToAtpAdaptor; /* case sensitive */
		  isTclSummaryManpage = 1;
		}

		/* If a built-in Tcl command, search for installed manpage. */
		if (isTclSummaryManpage || isBuiltInTclCmd(name)) {
		  fprintf(stdout, "Searching for and/or formatting manual entry.");
		  fprintf(stdout, " Wait...");
		  fflush(stdout);

		  (void) Atp_DvsPrintf (&tmpcmd, "exec man %s 2>/dev/null", name);
		  result = Tcl_Eval(interp, tmpcmd);
		  FREE(tmpcmd); tmpcmd = NULL;

		  if (result == TCL_OK)
		    fprintf(stdout, " done\n");
		  else
		    fprintf (stdout, "\n");
		    fflush(stdout);

		    result = (result == TCL_OK) ? ATP_OK : ATP_ERROR;
		}
		else
		/*
		 * See if command name is a Tcl proc, if so, make a manpage for it.
		 */
		if ((Tcl_VarEval(interp, "info procs ", name, NULL) == TCL_OK) &&
				(strcmp(Tcl_GetStringResult(interp), name) == 0))
		{
			char *rs = NULL;

			ZeroTclTmpVars();
			Atp_AdvResetBuffer();

			Atp_AdvPrintf("NAME\n	");
			Atp_AdvPrintf("%s", name);
			Atp_AdvPrintf("\n\nSYNOPSIS\n	");
			Atp_AdvPrintf("%s", name);

			Atp_DvsPrintf(&tmpcmd, synopsis_cmd, name, pid, pid);
			Tcl_Eval(interp, tmpcmd);
			Atp_AdvPrintf ("%s\n\n", Tcl_GetStringResult(interp));
			FREE(tmpcmd);

			ZeroTclTmpVars();

			Atp_AdvPrintf("DESCRIPTION\n\n");
			Atp_AdvPrintf("proc %s", name);
			Atp_DvsPrintf(&tmpcmd, defaults_cmd, name, name,
						  pid, pid, pid, pid, pid);
			Tcl_Eval(interp, tmpcmd);
			Atp_AdvPrintf(" {%s}", Tcl_GetStringResult(interp));
			Atp_AdvPrintf(" {\n");
			FREE(tmpcmd);

			/* Reset used variables to empty strings. */
			ZeroTclTmpVars ();

			Tcl_VarEval (interp, "info body ", name, NULL);
			Atp_AdvPrintf("%s", Tcl_GetStringResult(interp));

			Atp_AdvPrintf("\n}");

			rs = Atp_AdvGets();

			Tcl_SetResult(interp, rs, free);
		}
		else
		{
			(void) Atp_DvsPrintf( &tmpcmd,
						"No %s application manpage available for command \"%s\".",
						NameOfFrontEndToAtpAdaptor, name);
			Tcl_SetResult(interp, tmpcmd, free); /* tmpcmd freed by Tcl */
			result = ATP_ERROR;
		}

		va_end(ap);

		return result;
}

static char **TclBuiltInCmdList = NULL;

static void FreeBuiltInTclCmdList()
{
	Atp_FreeTokenList(TclBuiltInCmdList);
}

#if defined (__STDC__) || defined (_cplusplus)
static int isBuiltInTclCmd( char *cmdname )
#else
static int isBuiltInTclCmd(cmdname)
	char *cmdname;
#endif
{
	static int first_time = 1;
	static int count = 0;

	register int x;

	if (cmdname == NULL || *cmdname == '\0')
	  return 0;

	if (first_time)
	{
		Tcl_Interp *interp = NULL;
		int result = 0;

		interp = Tcl_CreateInterp();
		result = Tcl_Eval(interp, "info commands");

		if (result == TCL_OK)
		  TclBuiltInCmdList = Atp_Tokeniser(Tcl_GetStringResult(interp), &count);

		Tcl_DeleteInterp(interp);

		if (TclBuiltInCmdList != NULL)
		{
#if __STDC__ || hpux
			atexit(FreeBuiltInTclCmdList);
#else
#	if sun || sun2 || sun3 || sun4
			on_exit(FreeBuiltInTclCmdList)
#	endif
#endif
			first_time = 0;
		}
	}

	if (TclBuiltInCmdList != NULL)
		for (x = 0; x < count; x++)
		   if (tolower(*TclBuiltInCmdList[x]) == tolower(*cmdname))
		     if (Atp_Strcmp (TclBuiltInCmdList[x] , cmdname) == 0)
		       return 1;

	return 0;
}

/*
	Glue mechanism for help command to keep interface independent from
	Tcl. Necessary for ANSI C compliance since Atp_HelpCmd interface
	changed from varargs to stdarg requiring at least one fixed argument.

	Alwyn Teh (Fri Mar 17 15:16:52 GMT 1995)
*/
static ClientData		_Atp2Tcl_GlueClientData_	=	NULL;
static Tcl_Interp		*_Atp2Tcl_GlueInterp_		=	NULL;
static int				_Atp2Tcl_GlueArgc_			=	0;
static char *			*_Atp2Tcl_GlueArgv_			=	NULL;

#if defined(__STDC__) || defined(__cplusplus)
static Atp_Result Atp2Tcl_HelpCmdGlueCallback (char *HelpOption)
#else
static Atp_Result Atp2Tcl_HelpCmdGlueCallback (HelpOption)
			char *HelpOption;
#endif
{
	if (strcmp(HelpOption, ATP_HELPCMD_OPTION_CMDMANPG) == 0)
	{
	  return Atp_DisplayManPage(_Atp2Tcl_GlueClientData_,
								_Atp2Tcl_GlueInterp_,
								_Atp2Tcl_GlueArgc_,
								_Atp2Tcl_GlueArgv_);
	}
	else
	if (strcmp(HelpOption, ATP_HELPCMD_OPTION_LANG) == 0)
	{
		return Atp2Tcl_GetTclManpage(_Atp2Tcl_GlueClientData_,
									 _Atp2Tcl_GlueInterp_,
									 _Atp2Tcl_GlueArgc_,
									 _Atp2Tcl_GlueArgv_ );
	}
	return ATP_ERROR;
}

#if defined(__STDC__) || defined(__cplusplus)
static Atp_Result Atp_HelpCmdGlueProc(ClientData cd, Tcl_Interp *interp,
									  int argc, char **argv)
#else
static Atp_Result Atp_HelpCmdGlueProc(cd, interp, argc, argv)
			ClientData cd;
			Tcl_Interp *interp;
			int argc;
			char **argv;
#endif
{
	Atp_Result result = ATP_OK;
	char *ResultString = NULL;

	_Atp2Tcl_GlueClientData_	= cd;
	_Atp2Tcl_GlueInterp_		= interp;
	_Atp2Tcl_GlueArgc_			= argc;
	_Atp2Tcl_GlueArgv_			= argv;

	result = Atp_HelpCmd(cd, Atp2Tcl_HelpCmdGlueCallback, &ResultString);

	if (ResultString != NULL)
	  Tcl_SetResult(interp, ResultString, free);

	_Atp2Tcl_GlueClientData_	= NULL;
	_Atp2Tcl_GlueInterp_		= NULL;
	_Atp2Tcl_GlueArgc_			= 0 ;
	_Atp2Tcl_GlueArgv_			= NULL;

	return result;
}




