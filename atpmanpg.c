/* EDITION AC05 (REL002), ITD ACST.178 (95/06/20 18:00:00) -- OPEN */

/*+*******************************************************************

	Module Name:		atpmanpg.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation
						of the ATP on-line MANPAGE help system.
						The manpage for a command is displayed
						with its name, synopsis in BNP notation,
						and description. The description
						consists of any optional header and
						footer textual information, with a core
						body of command parameter syntax
						definitions in BNF notation with type
						specifications. Nested parameters and
						constructs are handled by the use of
						reference sections, each described in the
						same manner with any other subsections.

	Notes:				The system makes extensive use of self
						and mutual recursion to obtain the
						aforementioned functionality.

	Modifications:
		Who			When					Description
	----------	-----------------	-----------------------------------
	Alwyn Teh	8 October 1992		Initial Creation
	Alwyn Teh	7 June 1993			Global change from
									Atp_PrintfHelpInfo() to
									Atp_AdvPrintf().

	Alwyn Teh	5	October 1993	See below...

					A repeat block within a repeat block is displayed as:
							<rpt_blk0> ::= ( < <rpt_blkl> > ... )

					The extra < > signs are only useful for, say,
							<rpt_blk0> ::= ( < <pl> <p2> <p3> > ... )

					So introduce NoOfParmsInConstruct() in
					Atp_PrintRepeatBlockInNotationFormat() to
					fix problem.

	Alwyn Teh	5 October 1993		Add	Atp_PrintOptionalDefault()
	Alwyn Teh	10	January	1994	Add	manpage header on
									"man" command
	Alwyn Teh	13	January	1994	Indent man syntax notation
									by 1 more space; refine the
									text to read better
	Alwyn Teh	31	January	1994	Fix	"man Tcl" core dump bug
									where ATP frees the manpage.
									Turns out that Tcl frees it
									eventually in the next call
									to Tcl_AppendResult() or
									Tcl_AppendElement(). Change
									Atp_GetFrontEndManpage()'s
									argument list.
	Alwyn Teh	21 July 1994		Unify HELP system by single
									point access via "help" command
	Alwyn Teh	3 August 1994		Wrap and indent long lines
									using Atp_PrintfWordWrap().
	Alwyn Teh	8 March 1995		Use stdarg.h instead of
									varargs.h for ANSI compliance.
	Alwyn Teh	27 March 1995		Add BCD digits parameter type.
	Alwyn Teh	29 March 1995		OPT_BEGIN_LIST has one too many
									set of square brackets, fix it.
	Alwyn Teh	20 June 1995		Support GENIE III in displaying
									protocol field value meanings.
									Reuse Atp_KeywordType field.

********************************************************************-*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if defined (__STDC__) || defined (__cplusplus)
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

/* Constants */
#define MANPG_EXPLAIN_INDENT	(ATP_MANPG_INDENT + 3 + 6)
#define BINARY_STRING_MAX_BITS	(sizeof(unsigned int) * 8)

/* Describe the notation used in manpages. */
char * Atp_ManPageHeader[] = {
	"The \"man\" command is used to provide help information in the",
	"form of manual entries (or \"manpages\"). If the manpage",
	"requested is on a command which uses the ATP (Advanced Token",
	"Parser) service, then it generates a manpage from the command's",
	"parameter definition table. Complex commands are displayed in",
	"sections referenced by index numbers where appropriate. If the",
	"manpage requested is for the frontend language processor used,",
	"then \"man\" attempts to search for the installed manual entry on",
	"the host system.\n",

	"Syntax notation used in generated manual entries:",
	"	<parm_name>		indicates the name of the parameter; you must",
	"					enter the value of the parameter in its place",
	"	keyword			indicates a literal keyword; you must type",
	"					\"keyword\" as shown",
	"	[ <parm> ]		indicates that the parameter <parm> is",
	"					optional; its default value	is used when the",
	"					user value is not specified	but is indicated",
	"					by a period '.' or omitted if <parm> is a",
	"					trailing parameter in the command",
	"	{ <a> | <b> }	indicates that you may enter either the value",
	"					of <a> or the value of <b>",
	"	( <a> ... )		indicates that <a> is repeatable; you must",
	"					also use the brackets to delimit the enclosed",
	"					items",
	NULL
};

/*
	Information on optional parameters and default values. Update
	this notice if and when necessary.
 */
static char	*OptParm_Default_Notice[] =
{
	"NOTE:	Optional parameters have default values. These can be",
	"		selected by typing a dot (\".\") instead of a user value. If",
	"		the optional parameters are located at the end of the",
	"		command, it is not necessary to enter anything to select the",
	"		default values.",
NULL
};

/* Macros */
#define OPTPARM_CHECK(parmcode) \
		optional_parm_present = (!optional_parm_present) ? \
				(AtpParmIsOptional(parmcode)) : 1;

/* Local variables */
static int	optional_parm_present = 0;
static int	column = 1; /* column number of cursor position on screen */
static int	column_limit = ATP_DEFAULT_COLUMNS; /* default */

/* Exported functions */
int		Atp_DisplayManPage _PROTO_((ATP_FRAME_ELEM_TYPE ArgvO, ...));
void	Atp_DisplayManPageSynopsis _PROTO_((CommandRecord *CmdRecPtr));
void	Atp_PrintListInNotationFormat
						_PROTO_((ParmDefTable parmdef,
								 Atp_PDindexType start_index,
								 int indent));
void	Atp_EnumerateProtocolFieldValues
						_PROTO_((Atp_KeywordType *KeyTabPtr, int indent));

/* Local functions */
static void CheckCursorColumnAndWrapLine _PROTO_((int required_length, int indent));
static char * Atp_GenerateParmDefManPage _PROTO_((CommandRecord *CmdRecPtr));
static void	Atp_DisplayManPageDescription _PROTO_((CommandRecord *CmdRecPtr));
static void	Atp_PrintParmsInNotationFormat _PROTO_((ParmDefTable parmdef,
													Atp_PDindexType start_index,
													int *NoOfParmsPtr,
													int indent));
static void	Atp_DescribeParameter _PROTO_((	ParmDefTable parmdef,
											Atp_PDindexType parm_index,
											Atp_PDindexType *parm_index_ptr,
											char *parent_section_number,
											int indent));
static void	Atp_PrintRepeatBlockInNotationFormat _PROTO_((ParmDefTable parmdef,
														  Atp_PDindexType start_index,
														  int indent));
static void	Atp_PrintChoiceInNotationFormat _PROTO_((ParmDefTable parmdef,
													 Atp_PDindexType start_index,
													 int indent));
static void	Atp_PrintParmSequence _PROTO_((	ParmDefTable parmdef,
											Atp_PDindexType start_index,
											Atp_PDindexType stop_index,
											int *count_ptr,
											int indent) );
static void	Atp_PrintConstructComponents _PROTO_((ParmDefTable parmdef,
												  Atp_PDindexType start_index,
												  char *parent_section_number,
												  int indent));
static void	Atp_PrintParmTypeAndRange _PROTO_((ParmDefEntry *parmdefentry_ptr, int indent));
static int	Atp_PrintOptionalDefault _PROTO_((ParmDefTable parmdef, int index, int indent));
static int	NoOfParmsInConstruct _PROTO_((ParmDefTable parmdef,
										  Atp_PDindexType start_index,
										  Atp_PDindexType stop_index));
static char * binary_string _PROTO_(( int val, int start_pos, int val_size ));

#define USE_DECIMAL_WIDTH 0
#if ( USE_DECIMAL_WIDTH == 1 )
static int decimal_width _PROTO_(( unsigned int no_of_bits ));
#endif

/* Internal global variables and functions. */
Atp_Result	(*Atp_ManPageCmd) _PROTO_((void *,...)) = Atp_DisplayManPage;
void		Atp_ResetManPgColumn _PROTO_((void));
Atp_Result	(*Atp_GetFrontEndManpage) _PROTO_((void *,...)) = NULL;
int			Atp_ManPgLineWrap_Flag = 1; /* defaults to on */

/* Exported global variables */
int	Atp_Used_By_G3O = 0;

/*+**********************************************************************

	Function Name:		Atp_DisplayManPage

	Copyright:			BNR Europe Limited, 1992, 1993, 1994
						Bell-Northern Research
						Northern Telecom

	Description:		This function is the implementation of the
						"man" command. It drives the whole process
						of manpage generation at the top level.

	Modifications:
		Who			When				Description
	----------	---------------	-----------------------------
	Alwyn Teh	8 October 1992	Initial Creation
	Alwyn Teh	7 June 1993		Tidy-up
	Alwyn Teh	7 June 1993		Use Atp_DvsPrintf () .
	Alwyn Teh	23 June 1993	Remove Atp_SetPagingIfNecessary
	Alwyn Teh	27 June 1993	Support non-case-sensitive
								mode search on ATP commands
	Alwyn Teh	21 July 1994	Export for use in atphelpc.c
								Tidy up for "help" command
	Alwyn Teh	22 July 1994	Support command manpages for
								frontend language (e.g. Tcl)

***********************************************************************-*/
static
ATP_DCL_PARMDEF(Atp_ManPage_Parms)
	BEGIN_PARMS
		str_def("name", "manpage entry name or command name",1,0,NULL)
	END_PARMS
ATP_END_DCL_PARMDEF

Atp_ParmDefEntry *Atp_ManPage_PD_ptr = (Atp_ParmDefEntry *) Atp_ManPage_Parms;

#if defined (__STDC__) || defined(__cplusplus)
int Atp_DisplayManPage (ATP_FRAME_ELEM_TYPE ArgvO, ...)
#else
int
Atp_DisplayManPage(va_alist)
va_dcl
#endif
{
	va_list					ap;
	Atp_CallFrame			callframe;

	void					*clientData, *CmdEntryPtr;
	Atp_CmdTabAccessType	*CmdTableAccessDesc;
	CommandRecord			*CmdRecPtr;
	char					*cmdname, *manpage, *errmsg;

/* Extract stack frame from variable arguments list. */
#if defined(__STDC__) || defined(__cplusplus)
	callframe.stack[0] = ArgvO;
	va_start(ap,ArgvO);
	Atp_CopyCallFrameElems( &callframe.stack[1], ap, 1 );
#else
	va_start(ap);
	Atp_CopyCallFrame(&callframe, ap);
#endif
	va_end(ap);

	/*
		Get external clientdata containing execution information
		required.
	 */
	clientData = (void *) (*Atp_GetExternalClientData)(ATP_FRAME_RELAY(callframe));

	/*
		Get command table access descriptor record pointer from
		the call stack.
	 */
	CmdTableAccessDesc = (Atp_CmdTabAccessType *)
							(*Atp_GetCmdTabAccessRecord)(clientData);

	/* Get pointer to entry in command table. */
	cmdname = Atp_StrToLower(Atp_Str("name"));
	CmdEntryPtr = (*CmdTableAccessDesc->FindCmdTabEntry)
									(CmdTableAccessDesc, cmdname);

	/* Command does not exists. */
	if (CmdEntryPtr == NULL) {
	/* See if language manpage (e.g. Tcl) required. */
	  if (Atp_Strcmp(NameOfFrontEndToAtpAdaptor, cmdname) == 0) {
		if (Atp_GetFrontEndManpage != NULL) {
		  Atp_Result result = ATP_OK;
		  result = (*Atp_GetFrontEndManpage)(ATP_FRAME_RELAY(callframe));
		  return result;
		}
	  }
	  Atp_DvsPrintf(&errmsg, "Command \"%s\" does not exist.", cmdname);
	  Atp_ReturnDynamicStringToAdaptor(errmsg,ATP_FRAME_RELAY(callframe));
	  return ATP_ERROR;
	}
	else {
	  /* Command exists, see if it's an ATP command. */
	  CmdRecPtr = (CommandRecord *)(*CmdTableAccessDesc->CmdRec)(CmdEntryPtr);
	  if (CmdRecPtr == NULL) {
		/* Not an ATP command, find Tcl command manpage. */
		if (Atp_GetFrontEndManpage != NULL) {
		  Atp_Result result = ATP_OK;
		  result = (*Atp_GetFrontEndManpage)(ATP_FRAME_RELAY(callframe));
		  return result;
		}
	  }
	  else {
		/* ATP command found, make manpage for it. */
		manpage = Atp_GenerateParmDefManPage(CmdRecPtr);

		/*
			Call externally supplied function which will accept
			the help page output.
		 */
		Atp_ReturnDynamicHelpPage(manpage, ATP_FRAME_RELAY(callframe));
	  }

	  return ATP_OK;
	}
}

/*+********************************************************************

	Function Name:		Atp_GenerateParmDefManPage

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Manpage generation function.

	Modifications:
		Who			When				Description
	----------	---------------	---------------------------------
	Alwyn Teh	8 October 1992	Initial Creation
	Alwyn Teh	27 October 1992	Write manpage description code
	Alwyn Teh	7 June 1993		Use Atp_AdvPrintf().
	Alwyn Teh	15 June 1993	Delete leading newline
								from final sentence.
	Alwyn Teh	22 July 1994	Ensure Atp_ManPgLineWrap_Flag
								set to 1 always since may be
								used elsewhere.
	Alwyn Teh	28 July 1994	Ditch last message to say
								type "<cmd> ?" to get help.
	Alwyn Teh	2 August 1994	Wrap long strings.

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static char * Atp_GenerateParmDefManPage( CommandRecord *CmdRecPtr )
#else
static char *
Atp_GenerateParmDefManPage(CmdRecPtr)
	CommandRecord *CmdRecPtr;
#endif
{
	int		indent			= ATP_MANPG_INDENT;
	char	*ManPage		= NULL;
	char	*error_msg		= NULL;
	char	*column_env_str = getenv("COLUMNS");
	int		colenv;

	Atp_ManPgLineWrap_Flag = 1; /* ensure set to 1 */
	column = 1;

	/* Set global variable column_limit. */
	if (column_env_str != NULL && *column_env_str != '\0')
	  column_limit = ((colenv = atoi(column_env_str)) > 0) ?
			  	  	  	  	  	  	  	  	  colenv : ATP_DEFAULT_COLUMNS;

	optional_parm_present = 0;

	error_msg = Atp_VerifyCmdRecParmDef((Atp_CmdRec *)CmdRecPtr);
	if (error_msg != NULL) {
	  return error_msg;
	}

	Atp_AdvPrintf ("\nNAME\n") ;
	column = 1;
	Atp_DisplayIndent(indent);
	column = indent;
	column =
	Atp_PrintfWordWrap(	Atp_AdvPrintf, column_limit, indent,
						strlen(CmdRecPtr->cmdNameOrig) + indent + 3,
						"%s - %s\n",
						CmdRecPtr->cmdNameOrig, CmdRecPtr->cmdDesc );

	Atp_AdvPrintf ("\nSYNOPSIS\n");
	column = 1;
	Atp_DisplayManPageSynopsis(CmdRecPtr);

	Atp_AdvPrintf ("\nDESCRIPTION\n");
	column = 1;
	Atp_DisplayCmdHelpInfo( (Atp_CmdRec *)CmdRecPtr,
							ATP_MANPAGE_HEADER,
							indent);
	Atp_DisplayManPageDescription(CmdRecPtr);

	if (optional_parm_present) {
	  int x;
	  Atp_AdvPrintf("\n");
	  for (x = 0; OptParm_Default_Notice[x] != NULL; x++)
		 Atp_AdvPrintf("	%s\n", OptParm_Default_Notice[x]);
	}

	Atp_AdvPrintf("\n");

	Atp_DisplayCmdHelpInfo( (Atp_CmdRec *)CmdRecPtr,
							ATP_MANPAGE_FOOTER,
							indent);

	ManPage = Atp_AdvGets();

	return ManPage;
}

/*+******************************************************************

	Function Name:		Atp_DisplayManPageSynopsis

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Generate the manpage synopsis only in BNF
						notation.
	Modifications:
		Who			When				Description
	----------	---------------	-----------------------------
	Alwyn Teh	8 October 1992	Initial Creation
	Alwyn Teh	7 June 1993		Use Atp_AdvPrintf().
	Alwyn Teh	24 July 1993	Export for use in atphelpc.c

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_DisplayManPageSynopsis( CommandRecord *CmdRecPtr )
#else
void Atp_DisplayManPageSynopsis (CmdRecPtr)
	CommandRecord *CmdRecPtr;
#endif
{
	int NoOfParms = 0;

	Atp_DisplayIndent(ATP_MANPG_INDENT);
	column += ATP_MANPG_INDENT;

	Atp_AdvPrintf ("%s", CmdRecPtr->cmdNameOrig);
	column += strlen(CmdRecPtr->cmdNameOrig);

	if ((CmdRecPtr->parmDef == NULL) || (Atp_IsEmptyParmDef(CmdRecPtr)))
	{
	  Atp_AdvPrintf("\n");
	  column = 1;
	  return;
	}
	else {
	  Atp_PrintParmsInNotationFormat(CmdRecPtr->parmDef,
									 (Atp_PDindexType) 0,
									 &NoOfParms,
									 column-1);
	  /*
		Record the number of parameters found in parmdef if not
		already done so.
	   */
	  if (CmdRecPtr->NoOfParameters == 0)
		CmdRecPtr->NoOfParameters = NoOfParms;

	  Atp_AdvPrintf("\n");
	  column = 1;
	}

	return;
}

/*+*******************************************************************

	Function Name:		Atp_DisplayManPageDescription

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Display manpage description	section
						containing syntax specifications and any
						supporting textual information as
						described in the module header above.

	Modifications:
		Who			When				Description
	----------	----------------	------------------------
	Alwyn Teh	27 October 1992		Initial Creation
	Alwyn Teh	7 June 1993			Use Atp_AdvPrintf().
	Alwyn Teh	10 June 1993		Display new manpage
									desc text paragraph.
	Alwyn Teh	30	June 1993		Move display of header & footer,
									if any, using new function
									Atp_DisplayCmdHelpInfo() to
									Atp_GenerateParmDefManPage().
	Alwyn Teh	24 December 1993
					Change name of Atp_AllParmsInParmDefAreOptional()
					to Atp_AllParmsInParmDefAreOptionalOrNull().

*********************************************************************/
#if defined(__STDC__) || defined(__cplusplus)
static void Atp_DisplayManPageDescription( CommandRecord *CmdRecPtr )
#else
static void
Atp_DisplayManPageDescription(CmdRecPtr)
	CommandRecord *CmdRecPtr;
#endif
{
	int		indent = ATP_MANPG_INDENT;
	int		count = 1;
	char	section_number[20];

	Atp_PDindexType parm_idx =0;

	Atp_DisplayIndent(indent);

	if ((CmdRecPtr->parmDef == NULL) || (Atp_IsEmptyParmDef(CmdRecPtr)))
	{
	  /* Print header text for description. */
	  Atp_AdvPrintf("Command \"%s\" does not have any parameters.\n",
			  	  	CmdRecPtr->cmdNameOrig);
	}
	else
	{
	  /* Print header text for description. */
	  Atp_AdvPrintf("Command \"%s\" accepts %d %sparameter%s%s\n",
					CmdRecPtr->cmdNameOrig,
					CmdRecPtr->NoOfParameters,
					(Atp_AllParmsInParmDefAreOptionalOrNull(CmdRecPtr)) ? "optional " : "",
					(CmdRecPtr->NoOfParameters > 1) ? "s" : "",
					(CmdRecPtr->NoOfParameters >1) ? ". These are:" : ":");

	  /* Skip BEGIN_PARMS. */
	  if (Atp_PARMCODE((CmdRecPtr->parmDef) [parm_idx].parmcode) == ATP_BPM)
		parm_idx++;

	  /* Print out the parameters one by one. */
	  while (parm_idx < CmdRecPtr->NoOfPDentries-1)
	  {
		/* Make top-level section number. */
		(void) sprintf(section_number, "%d", count);

		/* Describe parameter */
		Atp_DescribeParameter(CmdRecPtr->parmDef, parm_idx,
							  &parm_idx, section_number,
							  indent);
		parm_idx++;
		count++;
	  }
	}

	return;
}

/*+*******************************************************************

	Function Name:		Atp_DescribeParameter

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Describe one parmdef entry (parameter or
						construct) in a human readable format.

	Modifications:
		Who			When				Description
	----------	---------------	---------------------------
	Alwyn Teh	28 October 1992	Initial Creation
	Alwyn Teh	7 June 1993	Use Atp_AdvPrintf().
	Alwyn Teh	3 August 1994	Wrap and indent long lines.

********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
static void Atp_DescribeParameter
(
	ParmDefTable	parmdef,
	Atp_PDindexType	parm_index,
	Atp_PDindexType	*parm_index_ptr,
	char			*parent_section_number,
	int				indent
)
#else
static void
Atp_DescribeParameter(parmdef, parm_index, parm_index_ptr,
parent_section_number, indent)
	ParmDefTable	parmdef;
	Atp_PDindexType	parm_index, *parm_index_ptr;
	char			*parent_section_number;
	int				indent;
#endif
{
	static int		nested_level = -1;
	Atp_ParmCode	CurrParmCode = parmdef[parm_index].parmcode;
	char			section_number[50];
	char			*tmp = NULL;
	int				start_column = 0;

	nested_level++;

	/* Display parameter section. */
	Atp_AdvPrintf("\n");
	Atp_DisplayIndent(indent);
	Atp_DvsPrintf(&tmp, (nested_level) ? "%s " : "%s. ", parent_section_number);
	Atp_AdvPrintf(tmp);
	start_column = indent + strlen(tmp);
	FREE(tmp);

	/* Display parameter name and description */
	column =
	Atp_PrintfWordWrap(	Atp_AdvPrintf,
						-1, start_column,
						start_column+strlen(parmdef[parm_index].Name)+5,
						"<%s> - \"%s\"\n\n",
						parmdef[parm_index].Name,
						parmdef[parm_index].Desc );

	/* See if parameter is optional and take note of it. */
	OPTPARM_CHECK(parmdef[parm_index].parmcode);

	indent += 3;
	Atp_DisplayIndent(indent);
	column = indent;

	(void) strcpy(section_number, parent_section_number);

	/* Print definition of parameter. */
	if (isAtpRegularParm(CurrParmCode)) {
	  Atp_AdvPrintf ("<%s> is ", parmdef [parm_index] .Name);
	  column += strlen(parmdef[parm_index].Name) + 6;
	  Atp_PrintParmTypeAndRange(&parmdef[parm_index], indent);
	  Atp_PrintOptionalDefault(parmdef, parm_index, indent);
	}
	else
	if (isAtpBeginConstruct(CurrParmCode)) {
	  Atp_AdvPrintf("<%s> ::=", parmdef[parm_index].Name);
	  column += strlen(parmdef[parm_index].Name) + 6;
	  Atp_PrintParmsInNotationFormat(parmdef, parm_index, NULL, column+2);

	  if (parm_index_ptr != NULL)
		*parm_index_ptr = parmdef[parm_index].matchIndex;

	  /* Provide further explanation if necessary. */
	  Atp_PrintConstructComponents(parmdef, parm_index, section_number, indent);
	}
	else
	if (isAtpComplexParm(CurrParmCode)) {
	  ; /* nothing yet */
	}

	nested_level--;
}

/*+*******************************************************************

	Function Name:		Atp_PrintParmTypeAndRange

	Copyright:			BNR Europe Limited, 1992, 1993, 1995
						Bell-Northern Research
						Northern Telecom

	Description:		Prints parameter type and	range	information.

	Modifications:
		Who			When				Description
	----------	------------------	--------------------------------
	Alwyn Teh	28 October 1992		Initial Creation
	Alwyn Teh	7 June 1993			Use Atp_AdvPrintf().
	Alwyn Teh	27 March 1995		Add BCD	digits parameter type.
	Alwyn Teh	20 June 1995		Support	GENIE III by displaying
									protocol field value meanings.

********************************************************************-*/
#if defined (__STDC__) || defined(__cplusplus)
static void Atp_PrintParmTypeAndRange(ParmDefEntry *parmdefentry_ptr, int indent)
#else
static void
Atp_PrintParmTypeAndRange (parmdef entry_ptr, indent)
	ParmDefEntry	*parmdefentry_ptr;
	int				indent;
#endif
{
	char			head[256], tail[256];
	Atp_ParmCode	parmcode = parmdefentry_ptr->parmcode;

	head[0] = '\0';
	tail[0] = '\0';

	switch (Atp_PARMCODE(parmcode)) {
		case ATP_NUM: {
				(void) sprintf(head, "a number");

				if (!ATP_PARMRANGE_DONT_CARE(parmdefentry_ptr, Atp_NumType))
				  (void) sprintf(tail, " between %d and %d",
								 (Atp_NumType) parmdefentry_ptr->Min,
								 (Atp_NumType)parmdefentry_ptr->Max);

				CheckCursorColumnAndWrapLine(strlen(head) + strlen(tail), indent+3);

				Atp_AdvPrintf(head);
				Atp_AdvPrintf(tail);

				if (Atp_Used_By_G3O &&
					parmdefentry_ptr->KeyTabPtr != NULL &&
					parmdefentry_ptr->KeyTabPtr[0].keyword != NULL)
				{
				  Atp_AdvPrintf("\n");
				  Atp_EnumerateProtocolFieldValues(parmdefentry_ptr->KeyTabPtr, indent);
				}

				break;
		}
		case ATP_UNS_NUM: {
				(void) sprintf(head, "an unsigned number");

				if (!ATP_PARMRANGE_DONT_CARE(parmdefentry_ptr, Atp_UnsNumType))
				  (void) sprintf(tail, " between %u and %u",
								 (Atp_UnsNumType) parmdefentry_ptr->Min,
								 (Atp_UnsNumType)parmdefentry_ptr->Max);

				CheckCursorColumnAndWrapLine(strlen(head) + strlen(tail), indent+3);

				Atp_AdvPrintf(head);
				Atp_AdvPrintf(tail);

				if (Atp_Used_By_G3O &&
					parmdefentry_ptr->KeyTabPtr != NULL &&
					parmdefentry_ptr->KeyTabPtr[0].keyword != NULL )
				{
				  Atp_AdvPrintf("\n");
				  Atp_EnumerateProtocolFieldValues(parmdefentry_ptr->KeyTabPtr, indent);
				}

				break;
		}
		case ATP_REAL: {
				(void) sprintf(head, "a real number");

				if (!ATP_PARMRANGE_DONT_CARE(parmdefentry_ptr, Atp_RealType))
				  (void) sprintf(tail, " between %g and %g",
								 (Atp_RealType)parmdefentry_ptr->Min,
								 (Atp_RealType)parmdefentry_ptr->Max);
				CheckCursorColumnAndWrapLine(strlen(head) + strlen(tail), indent+3);

				Atp_AdvPrintf(head);
				Atp_AdvPrintf(tail);

				break;
		}
		case ATP_STR: {
				(void) sprintf(head, "a string");

				if (!ATP_PARMRANGE_DONT_CARE(parmdefentry_ptr, Atp_NumType))
				  (void) sprintf(tail, " between %d and %d characters long",
								 (Atp_NumType)parmdefentry_ptr->Min,
								 (Atp_NumType)parmdefentry_ptr->Max);

				CheckCursorColumnAndWrapLine(strlen(head) + strlen(tail), indent+3);

				Atp_AdvPrintf(head);
				Atp_AdvPrintf(tail);

				break;
		}
		case ATP_BOOL: {
				/* No need to update column here. */
				Atp_AdvPrintf("a boolean value\n");
				Atp_DisplayIndent(indent+3);
				Atp_AdvPrintf("boolean ::= {1|0|TRUE|FALSE|T|F|YES|NO|Y|N|ON|OFF}");
				break;
		}
		case ATP_DATA: {
				(void) sprintf(head, "a hexadecimal string");

				if (!ATP_PARMRANGE_DONT_CARE(parmdefentry_ptr, Atp_NumType))
				  (void) sprintf(tail, " between %d and %d bytes long",
								 (Atp_NumType)parmdefentry_ptr->Min,
								 (Atp_NumType)parmdefentry_ptr->Max);

				CheckCursorColumnAndWrapLine(strlen(head) + strlen(tail), indent+3);

				Atp_AdvPrintf(head);
				Atp_AdvPrintf(tail);

				break;
		}
		case ATP_BCD: {
				(void) sprintf(head, "a 4-bit BCD (Binary Coded Decimal) digits string");

				if (!ATP_PARMRANGE_DONT_CARE(parmdefentry_ptr, Atp_NumType))
				  (void) sprintf(tail, "between %d and %d digits long",
								 (Atp_NumType)parmdefentry_ptr->Min,
								 (Atp_NumType)parmdefentry_ptr->Max);

				column = Atp_PrintfWordWrap(Atp_AdvPrintf,
											-1, column, indent+3,
											"%s %s", head, tail);
				break;
		}
		case ATP_KEYS: {
				int index, desc_used, done;
				char *desc = "a keyword in the following list:";

				CheckCursorColumnAndWrapLine(strlen(desc), indent+3);

				Atp_AdvPrintf("%s\n", desc);
				Atp_DisplayIndent(indent+3);

				/* Print keyword selection list. */
				Atp_AdvPrintf ("{");
				column = indent+4;

				for (index = desc_used = done = 0; !done; index++)
				{
				   Atp_AdvPrintf ("%s", (parmdefentry_ptr->KeyTabPtr)[index].keyword);

				   column += strlen((parmdefentry_ptr->KeyTabPtr)[index].keyword) + 1;

				   done = ((parmdefentry_ptr->KeyTabPtr)[index+1].keyword == NULL);

				   if (!done)
				   {
					 CheckCursorColumnAndWrapLine (2, indent+5);
					 Atp_AdvPrintf(" |");
					 column += 2;
					 CheckCursorColumnAndWrapLine(
							 strlen((parmdefentry_ptr->KeyTabPtr)
									 [index+1].keyword)+1, indent+5);
				   }

				   if (!desc_used)
					 desc_used = ((parmdefentry_ptr->KeyTabPtr)
							 	  [index].KeywordDescription != NULL);
				}

				Atp_AdvPrintf(" }");

				/* If keyword descriptions were used, show these. */
				if (desc_used)
				{
				  Atp_AdvPrintf("\n");
				  Atp_DisplayIndent(indent+3);
				  Atp_AdvPrintf ("where ") ;
				  column = indent+3+6;
				  for (index = done = 0; !done; index++)
				  {
					  done = ((parmdefentry_ptr->KeyTabPtr)
							  [index+1].keyword == NULL);

					  if ((parmdefentry_ptr->KeyTabPtr)
						  [index].KeywordDescription != NULL)
					  {
						  char *Keyword = (parmdefentry_ptr->KeyTabPtr)[index].keyword;
						  char *Keydesc = (parmdefentry_ptr->KeyTabPtr)
												[index].KeywordDescription;
						  column =
						  Atp_PrintfWordWrap(Atp_AdvPrintf, -1,
											 column,
											 indent+9+strlen(Keyword)+5,
											 "%s is \"%s\"",
											 Keyword, Keydesc);
						  if (!done) {
							Atp_AdvPrintf("\n");
							column = 1;
						  }
						  Atp_DisplayIndent(indent+9);
						  column += indent+9;
					  }
				  }
				}
				break;
		}
		case ATP_NULL: {
				Atp_AdvPrintf("NULL");
				break;
		}
		case ATP_COM: {
				Atp_AdvPrintf("a parameter definition table");
				break;
		}
		default: break;
	}
	Atp_AdvPrintf("\n");
	column = 1;
}

/*+*****************************************************************

	Function Name:		Atp_PrintOptionalDefault

	Copyright:			BNR Europe Limited, 1993-1995
						Bell-Northern Research
						Northern Telecom

	Description:		Prints optional parameter or construct
						default value information.

	Modifications:
		Who			When					Description
	----------	----------------	-------------------------------
	Alwyn Teh	5 October 1993		Initial Creation
	Alwyn Teh	6 January 1994		Port to SUN workstation -
									suppress SUN's printf habit
									of displaying " (null) 11 when
									a string is NULL. This causes
									testcases to fail but which
									work on the HP. Fix this for
									the optional default value
									for the string parameter.
	Alwyn Teh	20	January 1994	Output "" when optional
									default choice name is NULL.
									Add 3rd parameter to function
									Atp_GetOptChoiceDefaultCaseName
	Alwyn Teh	21	January 1994	Output "" when optional default
									keyword value not found.
	Alwyn Teh	27	March 1995		Add BCD digits	parameter type.
	Alwyn Teh	28	March 1995		Display default databytes like
									so as to be consistent with
									BCD digits which get displayed.

*******************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
static int Atp_PrintOptionalDefault
(
	ParmDefTable	parmdef,
	int				index,
	int				indent
)
#else
static int
Atp_PrintOptionalDefault(parmdef, index, indent)
	ParmDefTable	parmdef;
	int				index, indent;
#endif
{
	Atp_ParmCode parmcode;

	if (parmdef == NULL)
	  return 0;

	parmcode = parmdef[index].parmcode;

	if (AtpParmIsOptional(parmcode)) {
	  if (isAtpRegularParm(parmcode) || isAtpBeginConstruct(parmcode)) {
		Atp_DisplayIndent(indent);
		Atp_AdvPrintf("Default value for <%s> is ", parmdef[index].Name);
		column += indent + 24 + strlen(parmdef[index].Name);
	  }

	  switch(Atp_PARMCODE(parmcode)) {
	    case ATP_NUM:
	    	Atp_AdvPrintf("%d",(Atp_NumType)parmdef[index].Default);
	    	break;
	    case ATP_UNS_NUM:
	    	Atp_AdvPrintf("%u",(Atp_UnsNumType)parmdef[index].Default);
	    	break;
	    case ATP_STR: {
	    	char *str = (char *)parmdef[index].DataPointer;
	    	column = Atp_PrintfWordWrap(Atp_AdvPrintf, -1, column, indent+3,
	    								"\"%s\"", (str == NULL) ? "" : str);
	    	break;
	    }
	    case ATP_KEYS: {
			Atp_Result result = ATP_OK;
			char *keystring = NULL;

			result = Atp_GetKeywordIndex(parmdef[index].KeyTabPtr,
										 (int)parmdef[index].Default,
										 &keystring, NULL, NULL);
			if (result == ATP_OK)
			  Atp_AdvPrintf("\"%s\"", keystring);
			else
			  Atp_AdvPrintf("\"\""); /* no keystring value */
			  break;
	    }
		case ATP_BOOL:
			Atp_AdvPrintf("%s",((Atp_BoolType)parmdef[index].Default==TRUE) ?
								"TRUE" : "FALSE");
			break;
		case ATP_BCH: {
			char *name = NULL;
			name = Atp_GetOptChoiceDefaultCaseName(parmdef,index,NULL);
			if (name == NULL)
			  Atp_AdvPrintf("\"\"");
			else
			  Atp_AdvPrintf("\"%s\"", name);
			break;
		}
		case ATP_BLS:
		case ATP_BRP:
		case ATP_BCS: /* Note: optional CASE does not exist */
			Atp_AdvPrintf("%s", (parmdef[index].DataPointer != NULL) ?
									"not displayed" : "NULL");
			break;

		case ATP_DATA: {
			Atp_DataDescriptor *def_hex_desc = (Atp_DataDescriptor *)
													parmdef[index].DataPointer;
			char *def_hex = (def_hex_desc != NULL) ?
									Atp_DisplayHexBytes(*def_hex_desc, " ")
									: NULL;
			if (def_hex == NULL)
			  Atp_AdvPrintf("(NULL)");
			else
			{
			  column =
			  Atp_PrintfWordWrap(Atp_AdvPrintf, -1, column, indent+3,
					  	  	  	  "{%s}", def_hex);
			  FREE(def_hex);
			}
			break;
		}

		case ATP_BCD: {
			Atp_DataDescriptor *bcd_digits = (Atp_DataDescriptor *)
													parmdef[index].DataPointer;
			char *bcd = (bcd_digits != NULL) ?
							Atp_DisplayBcdDigits(*bcd_digits) : NULL;
			if (bcd == NULL)
			  Atp_AdvPrintf("(NULL)");
			else
			{
			  column =
			  Atp_PrintfWordWrap(Atp_AdvPrintf, -1, column, indent+3,
					  	  	  	  "{%s}", bcd);
			  FREE(bcd);
			}
			break;
		}
		case ATP_NULL:
		case ATP_REAL:
		case ATP_COM:
		default:
			Atp_AdvPrintf("%g", parmdef[index].Default); /* anything */
			break;
		}
		Atp_AdvPrintf("\n");
		return 1;
	}
	return 0;
}

/*+********************************************************************

	Function Name:		Atp_PrintParmsInNotationFormat

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Prints parameter specification	in
						BNF notation.

	Modifications:
		Who			When				Description
	----------	----------------	--------------------
	Alwyn Teh	27 October 1992		Initial Creation
	Alwyn Teh	7 June 1993			Support CASE

*********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
static void Atp_PrintParmsInNotationFormat
(
	ParmDefTable	parmdef,
	Atp_PDindexType	start_index,
	int				*NoOfParmsPtr,
	int				indent
)
#else
static void
Atp_PrintParmslnNotationFormat(parmdef, start_index, NoOfParmsPtr, indent)
	ParmDefTable	parmdef;
	Atp_PDindexType	start_index;
	int				*NoOfParmsPtr;
	int				indent;
#endif
{
	register Atp_PDindexType	parm_idx = start_index;
	register Atp_ParmCode		CurrParmCode, Start_ParmCode;

	Atp_PDindexType stop_index = parmdef[start_index].matchIndex;
	int NoOfParms = 0;

	Start_ParmCode = parmdef[start_index].parmcode;

	/* Skip BEGIN_PARMS and BEGIN_REPEAT. */
	if (Atp_PARMCODE(Start_ParmCode) == ATP_BPM)
	{
	  Atp_PrintParmSequence(parmdef, 1, stop_index, &NoOfParms, indent);
	}
	else
	/* Scan parmdef and print parameters in notation format. */
	while (parm_idx < stop_index) {
		CurrParmCode = parmdef[parm_idx].parmcode;
		OPTPARM_CHECK(CurrParmCode);

		switch (Atp_PARMCODE(CurrParmCode)) {
			case ATP_BLS:
			case ATP_BCS:	Atp_PrintListInNotationFormat(
									parmdef, parm_idx, indent);
							break;
			case ATP_BRP:	Atp_PrintRepeatBlockInNotationFormat(
									parmdef, parm_idx, indent);
							break;
			case ATP_BCH:	Atp_PrintChoiceInNotationFormat(
									parmdef, parm_idx, indent);
							break;
			default:		break;
		}

		if (isAtpBeginConstruct(CurrParmCode)) {
		  parm_idx = parmdef[parm_idx].matchIndex;
		}

		parm_idx++;
		NoOfParms++;
	}

	/* Return number of parameters found if required. */
	if (NoOfParmsPtr != NULL)
	  *NoOfParmsPtr = NoOfParms;
}

/*+*******************************************************************

	Function Name:		Atp_PrintListlnNotationFormat

	Copyright:			BNR Europe Limited, 1992, 1993, 1994
						Bell-Northern Research
						Northern Telecom

	Description:		Prints simple LIST-like construct in
						BNF notation.

	Modifications:
		Who			When				Description
	----------	----------------	-----------------------
	Alwyn Teh	28 October 1992		Initial Creation
	Alwyn Teh	7 June 1993			Use Atp_AdvPrintf().
	Alwyn Teh	22 July 1994		Exported for atphelpc.c

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_PrintListInNotationFormat
(
	ParmDefTable	parmdef,
	Atp_PDindexType	start_index,
	int				indent
)
#else
void
Atp_PrintListInNotationFormat(parmdef, start_index, indent)
	ParmDefTable	parmdef;
	Atp_PDindexType	start_index;
	int				indent;
#endif
{
	Atp_ParmCode CurrParmCode = parmdef[start_index].parmcode;
	Atp_PDindexType stop_index = parmdef[start_index].matchIndex;

	OPTPARM_CHECK(CurrParmCode);

	/* Open square bracket to indicate optional parameter. */
	if (AtpParmIsOptional(CurrParmCode)) {
	  Atp_AdvPrintf(" [");
	  column += 2;
	}

	Atp_PrintParmSequence(parmdef, start_index+1, stop_index, NULL, indent);

	/* Close square bracket to delimit optional parameter. */
	if (AtpParmIsOptional(CurrParmCode)) {
	  Atp_AdvPrintf(" ]");
	  column += 2;
	}
}

/*+*******************************************************************

	Function Name:		Atp_PrintRepeatBlockInNotationFormat

	Copyright:	BNR Europe Limited, 1992, 1993
	Bell-Northern Research
	Northern Telecom

	Description:	Prints REPEAT BLOCK construct in BNF
	notation incorporating own syntax.

	Modifications:
		Who			When				Description
	----------	----------------	--------------------------
	Alwyn Teh	28 October 1992		Initial Creation
	Alwyn Teh	7 June 1993			Use Atp_AdvPrintf().
	Alwyn Teh	5 October 1993		Use NoOfParmsInConstruct().

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void Atp_PrintRepeatBlockInNotationFormat
(
	ParmDefTable	parmdef,
	Atp_PDindexType	start_index,
	int				indent
)
#else
static void
Atp_PrintRepeatBlockInNotationFormat(parmdef, start_index, indent)
	ParmDefTable	parmdef;
	Atp_PDindexType	start_index;
	int				indent
#endif
{
	Atp_ParmCode CurrParmCode	= parmdef[start_index].parmcode;
	Atp_PDindexType stop_index	= parmdef[start_index].matchIndex;
	int NoOfParms = 0;

	OPTPARM_CHECK(CurrParmCode);

	/* Open square bracket to indicate optional parameter. */
	if (AtpParmIsOptional(CurrParmCode)) {
	  Atp_AdvPrintf(" [");
	  column += 2;
	}

	/* Open repeat block */
	Atp_AdvPrintf(" (");
	column += 2;

	/* Mark begin repeat block if more than 1 item inside. */
	NoOfParms = NoOfParmsInConstruct(parmdef,start_index,stop_index);
	if (NoOfParms > 1) {
	  Atp_AdvPrintf(" <");
	  column += 2;
	}

	/* Scan parmdef and print parameters in notation format. */
	Atp_PrintParmSequence(parmdef, start_index+1, stop_index, NULL, indent);
	CurrParmCode = parmdef[stop_index].parmcode;

	/* Mark end of repeat block if more than 1 item inside. */
	if (NoOfParms >1) {
	  Atp_AdvPrintf(" >");
	  column += 2;
	}

	/* Ellipsis indicates above item can be repeated. */
	Atp_AdvPrintf(" ...");
	column += 4;

	/* Close repeat block */
	Atp_AdvPrintf(" )");
	column += 2;

	/* Close square bracket to delimit optional parameter. */
	if (AtpParmIsOptional(CurrParmCode)) {
	  Atp_AdvPrintf(" ]") ;
	  column += 2;
	}
}

/*+*******************************************************************

	Function Name:		Atp_PrintChoiceInNotationFormat

	Copyright:			BNR Europe Limited, 1992, 1993,	1995
						Bell-Northern Research
						Northern Telecom

	Description:		Prints CHOICE construct in BNF	notation.
						Handles syntax of selection of parameters
						and constructs.

	Modifications:
		Who			When					Description
	----------	----------------	----------------------------
	Alwyn Teh	28 October 1992		Initial Creation
	Alwyn Teh	7 June 1993			Support CASE
	Alwyn Teh	7 June 1993			Use Atp_AdvPrintf().
	Alwyn Teh	29 March 1995		Only print square brackets
									for optional regular parms;
									constructs take care of
									themselves.
	Alwyn Teh	30 March 1995		Fix bug where repeat block
									or choice name is not printed
									after selector keyword.

********************************************************************-*/
#if defined (__STDC__) || defined(__cplusplus)
static void Atp_PrintChoiceInNotationFormat
(
	ParmDefTable	parmdef,
	Atp_PDindexType	start_index,
	int				indent
)
#else
static void
Atp_PrintChoiceInNotationFormat(parmdef, start_index, indent)
	ParmDefTable	parmdef;
	Atp_PDindexType	start_index;
	int				indent;
#endif
{
	register Atp_PDindexType	parm_idx = start_index;
	register Atp_ParmCode		CurrParmCode;
	Atp_PDindexType				stop_index = parmdef[start_index].matchIndex;
	int							local_indent;

	CurrParmCode = parmdef[parm_idx].parmcode;
	OPTPARM_CHECK(CurrParmCode);
	local_indent = indent;

	/* Open square bracket to indicate optional parameter. */
	if (AtpParmIsOptional(CurrParmCode)) {
	  Atp_AdvPrintf(" [") ;
	  local_indent += 2;
	  column += 2;
	}

	/* Open parentheses to indicate start of selection list. */
	Atp_AdvPrintf(" {");
	local_indent += 2;
	column += 2;

	/*
		Go into body of choice construct, position on first item.
		Then scan choice construct, displaying selection list.
	*/
	parm_idx++;
	while (parm_idx < stop_index)
	{
		CurrParmCode = parmdef[parm_idx].parmcode;
		OPTPARM_CHECK(CurrParmCode);

		/* Print selector name. */
		CheckCursorColumnAndWrapLine(
				strlen(parmdef[parm_idx].Name) + 1, local_indent);
		Atp_AdvPrintf (" %s", parmdef [parm_idx] .Name) ;
		column += strlen(parmdef[parm_idx].Name) + 1;

		/* Open square bracket to indicate optional parameter. */
		if (AtpParmIsOptional(CurrParmCode) && isAtpRegularParm(CurrParmCode))
		{
		  CheckCursorColumnAndWrapLine(2, local_indent);
		  Atp_AdvPrintf (" [");
		  column += 2;
		}

		/*
			Print regular parameter name.
			Also print REPEAT BLOCK and CHOICE BLOCK names only as these may
			contain lots of nested constructs and parameters. They are handled
			in subsection. But LIST and CASE are handled differently.
		 */
		if (isAtpRegularParm(CurrParmCode) ||
			Atp_PARMCODE(CurrParmCode) == ATP_BRP ||
			Atp_PARMCODE(CurrParmCode) == ATP_BCH)
		{
			CheckCursorColumnAndWrapLine(
					strlen(parmdef[parm_idx].Name) + 3, local_indent);
			Atp_AdvPrintf(" <%s>", parmdef[parm_idx].Name);
			column += strlen(parmdef[parm_idx].Name) + 3;
		}
		else
		if (Atp_PARMCODE(CurrParmCode) == ATP_BLS ||
			Atp_PARMCODE(CurrParmCode) == ATP_BCS) {
		  Atp_PrintListInNotationFormat(parmdef, parm_idx, local_indent);
		}

		/* Close square bracket to delimit optional parameter. */
		if (AtpParmIsOptional(CurrParmCode) && isAtpRegularParm(CurrParmCode))
		{
		  CheckCursorColumnAndWrapLine(2, local_indent);
		  Atp_AdvPrintf(" ]") ;
		  column += 2;
		}

		/* Jump to "end" of parameter if it's a construct. */
		if (isAtpBeginConstruct(CurrParmCode)) {
		  parm_idx = parmdef[parm_idx].matchIndex;
		}

		/* Next item on selection list. */
		parm_idx++;

		/* If not end of choice list, print OR symbol */
		if (parm_idx < stop_index) {
		  CheckCursorColumnAndWrapLine(2, local_indent);
		  Atp_AdvPrintf(" |");
		  column += 2;
		}
	}

	CurrParmCode = parmdef[parm_idx].parmcode;

	/* Close parentheses to indicate end of selection list. */
	CheckCursorColumnAndWrapLine(2, local_indent);
	Atp_AdvPrintf(" }");
	column += 2;

	/* Close square bracket to delimit optional parameter. */
	if (AtpParmIsOptional(CurrParmCode)) {
	  CheckCursorColumnAndWrapLine(2, local_indent);
	  Atp_AdvPrintf(" ]");
	  column += 2;
	}
}

/*+*******************************************************************

	Function Name:		Atp_PrintParmSequence

	Copyright:			BNR Europe Limited, 1992,	1993
						Bell-Northern Research
						Northern Telecom

	Description:		Prints simple sequence of parameters.

	Modifications:
		Who			When			Description
	----------	---------------	----------------------
	Alwyn Teh	28 October 1992	Initial Creation
	Alwyn Teh	7 June 1993		Use Atp_AdvPrintf() .

*******************************************************************_*/
#if defined(__STDC__) || defined(__cplusplus)
static void Atp_PrintParmSequence
(
	ParmDefTable	parmdef,
	Atp_PDindexType	start_index,
	Atp_PDindexType	stop_index,
	int				*count_ptr,
	int				indent
)
#else
static void
Atp_PrintParmSequence(parmdef, start_index, stop_index, count_jptr, indent)
	ParmDefTable	parmdef;
	Atp_PDindexType	start_index, stop_index;
	int				*count_ptr, indent;
#endif
{
	register int counter = 0;
	register Atp_PDindexType parm_idx = start_index;
	Atp_ParmCode CurrParmCode;

	while (parm_idx < stop_index) {
		CurrParmCode = parmdef[parm_idx].parmcode;
		OPTPARM_CHECK(CurrParmCode);

		/* Open square bracket to indicate optional parameter. */
		if (AtpParmIsOptional(CurrParmCode)) {
		  Atp_AdvPrintf (" [");
		  column += 2;
		}

		/* Print parameter name. */
		CheckCursorColumnAndWrapLine(strlen(parmdef[parm_idx].Name) + 3, indent);
		Atp_AdvPrintf(" <%s>", parmdef[parm_idx].Name);
		column += strlen(parmdef[parm_idx].Name) + 3;

		/* Close square bracket to delimit optional parameter. */
		if (AtpParmIsOptional(CurrParmCode)) {
		  Atp_AdvPrintf(" ]");
		  column += 2;
		}

		if (isAtpBeginConstruct(CurrParmCode)) {
		  parm_idx = parmdef[parm_idx].matchIndex;
		}

		parm_idx++;
		counter++;
	}

	/* Return number of parameters found. */
	if (count_ptr != NULL)
	  *count_ptr = counter;
}

/*+*******************************************************************

	Function Name:		Atp_PrintConstructComponents

	Copyright:			BNR Europe Limited, 1992, 1993,	1994
						Bell-Northern Research
						Northern Telecom

	Description:		Prints specifications of construct
						components, whether parameters or
						constructs.

	Modifications:
	Who	When	Description
	Alwyn Teh	28 October 1992	Initial Creation
	Alwyn Teh	7 June 1993	Use Atp_AdvPrintf () .
	Alwyn Teh 	27 September 1993	Print repeat block range.
	Alwyn Teh	5 October 1993	Adjust indentation.
	Alwyn Teh	5 October 1993	Use Atp_PrintOptionalDefault () .
	Alwyn Teh	1 August 1994	Word wrap case description.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void Atp_PrintConstructComponents
(
	ParmDefTable	parmdef,
	Atp_PDindexType	start_index,
	char			*parent_section_number,
	int				indent
)
#else
static void
Atp_PrintConst ruetComponents(parmdef, start_index, parent_section_number, indent)
	ParmDefTable	parmdef;
	Atp_PDindexType	start_index;
	char			*parent_section_number;
	int				indent;
#endif
{
	register Atp_PDindexType parm_idx = start_index;

	Atp_PDindexType stop_index = parmdef[start_index].matchIndex;
	Atp_ParmCode	CurrParmCode, Start_ParmCode;
	char			section_number[50];
	char			buffer[50];
	int				new_section_counter = 0;
	int				section_parmidx[100];
	int				local_indent = indent;
	int				is_choice_selector = 0;
	int				nest_level = 0;
	int				count, default_printed =0;
	int				where_line_used = 0;

	Atp_AdvPrintf ("\n\n");
	Atp_DisplayIndent(local_indent);

	Atp_AdvPrintf("where ");
	column = local_indent + 6;

	/* For alignment with start of description after "where ". */
	local_indent = MANPG_EXPLAIN_INDENT;

	Start_ParmCode = parmdef[start_index].parmcode;

	if (isAtpBeginConstruct(Start_ParmCode)) {
	  nest_level++;

	  /*
	   *	If top level construct is a repeat block,
	   *	say how many instances are allowed.
	   */
	  if (Atp_PARMCODE(Start_ParmCode) == ATP_BRP) {
		Atp_NumType min, max;

		min = (Atp_NumType)(parmdef[start_index].Min);
		max = (Atp_NumType)(parmdef[start_index].Max);

		if (min > max)
		  (void) sprintf(buffer, "any number of times");
		else
		if (min == max) {
			switch(max) {
				case 1: (void) sprintf(buffer, "once only"); break;
				case 2: (void) sprintf(buffer, "twice"); break;
				default:(void) sprintf(buffer, "%d times", max); break;
			}
		}
		else
			(void) sprintf(buffer, "%d to %d times", min, max);

		Atp_AdvPrintf("item(s) in <%s> appear(s) %s\n\n",
					  parmdef[start_index].Name, buffer);
		where_line_used = 1;
	  }

	  default_printed = Atp_PrintOptionalDefault(parmdef, start_index,
			  	  	  	  	  	  	  	  	  	 (where_line_used) ? local_indent : 0);
	  where_line_used = (where_line_used) ? where_line_used : default_printed;
	}

	/* Scan parameters and describe each one in turn. */
	parm_idx++;

	while (parm_idx < stop_index) {
		CurrParmCode = parmdef[parm_idx].parmcode;
		OPTPARM_CHECK(CurrParmCode);

		/*
			See if this parameter is part of the choice selection
			list of keywords.
		 */
		if ((nest_level == 1) && (Atp_PARMCODE(Start_ParmCode) == ATP_BCH)) {
		  if ((parm_idx != start_index+1) || where_line_used) {
			Atp_AdvPrintf("\n");
			Atp_DisplayIndent(local_indent);
			column = local_indent;
		  }

		  CheckCursorColumnAndWrapLine(strlen(parmdef[parm_idx].Name) + 32, local_indent);

		  Atp_AdvPrintf("%s should be selected to indicate ", parmdef[parm_idx].Name);

		  column += strlen(parmdef[parm_idx].Name) + 32;
		  column = Atp_PrintfWordWrap(Atp_AdvPrintf,
									  column_limit, column,
									  local_indent + 3,
									  "\"%s\"\n", parmdef [parm_idx] .Desc);
		  is_choice_selector = 1;
		  column = 1;
		}
		else {
		  is_choice_selector = 0;
		}

		/* Display parameter descriptions. */
		if (isAtpRegularParm(CurrParmCode)) {
		  if (is_choice_selector) {
			Atp_DisplayIndent(local_indent);
			column = local_indent;

			/* No need to supply description again. */
			Atp_AdvPrintf("<%s> is ", parmdef[parm_idx].Name);
			column += strlen(parmdef[parm_idx].Name) + 6;
			Atp_PrintParmTypeAndRange(&parmdef[parm_idx], local_indent);
		  }
		  else {
			if (default_printed) {
			  Atp_AdvPrintf("\n"); /* spacing after default info */
			  default_printed = 0; /* unset 'cause in loop */
			}
			/*
			 *	No need to indent if following "where ” but should
			 *	indent if repeat block range and newline were displayed
			 *	after "where ", instead of parameter explanation.
			 *	Other parameters following shouldn't be affected.
			 */
			if ((parm_idx != start_index+1) || where_line_used) {
			  Atp_DisplayIndent(local_indent);
			  column = local_indent;
			  where_line_used = 0; /* unset flag */
			}
			column = Atp_PrintfWordWrap(Atp_AdvPrintf,
										-1, column, local_indent+3,
										"<%s> is \"%sV', ",
										parmdef[parm_idx] .Name,
										parmdef[parm_idx].Desc);

			Atp_PrintParmTypeAndRange(&parmdef[parm_idx], local_indent);
		  }
		  Atp_PrintOptionalDefault(parmdef, parm_idx, local_indent);
		}
		else
		if (isAtpBeginConstruct(CurrParmCode)) {
		  /*
				If a selector LIST, its contents would have been
				printed out before, here the components should be
				explained (in the following iterations of the while
				loop).
				NOTE: Same applies to CASE.
		   */
		  if (is_choice_selector &&
			  (Atp_PARMCODE(CurrParmCode) == ATP_BLS ||
			   Atp_PARMCODE(CurrParmCode) == ATP_BCS)) {
			nest_level++;
		  }
		  else {
			/*
				If it’s anything else, it's probably too complicated
				to explain on one line, so create a new section
				number and say it will be explained later.
			 */
			new_section_counter++;

			Atp_DisplayIndent(local_indent);
			column = local_indent;

			column = Atp_PrintfWordWrap(Atp_AdvPrintf,
							-1, column, local_indent+3,
							"<%s> is \"%s\",",
							parmdef[parm_idx].Name,
							parmdef[parm_idx].Desc);

			(void) sprintf(	buffer, " see section %s.%d\n",
							parent_section_number,
							new_section_counter );

			CheckCursorColumnAndWrapLine(strlen(buffer),local_indent);
			Atp_AdvPrintf(buffer);
			column = 1;

			/* Take note of newly created section to be revisited. */
			section_parmidx[new_section_counter] = parm_idx;

			parm_idx = parmdef[parm_idx].matchIndex;
		  }
		}

		parm_idx++;

		if (isAtpEndConstruct(parmdef[parm_idx].parmcode)) {
		  parm_idx++;
		  nest_level--;
		}
	}

	/* New sections to be further explained ... */
	if (new_section_counter != 0) {
	  for (count = 1; count <= new_section_counter; count++) {
		 (void) sprintf(section_number, "%s.%d",
				 	 	parent_section_number, count);
		 Atp_DescribeParameter(parmdef, section_parmidx[count],
				 	 	 	   NULL, section_number, ATP_MANPG_INDENT);
		 section_number[0] = '0';
	  }
	}
}

/*+*******************************************************************

	Function Name:		CheckCursorColumnAndWrapLine

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Function to take care of line formatting.

	Modifications:
		Who			When				Description
	----------	---------------	----------------------------
	Alwyn Teh	29 October 1992	Initial Creation
	Alwyn Teh	7 June 1993		Use Atp_AdvPrintf () .
	Alwyn Teh	24 July 1993	Line wrap if flag
								Atp_ManPgLineWarp_Flag
								is set to on.

********************************************************************-*/
#if defined (__STDC__) || defined(__cplusplus)
static void CheckCursorColumnAndWrapLine(int required_length, int indent)
#else
static void
CheckCursorColumnAndWrapLine (required_length, indent)
	int required_length, indent;
#endif
{
#define RIGHT_MARGIN 4

	if (Atp_ManPgLineWrap_Flag &&
		(column_limit - column - RIGHT_MARGIN) <= required_length)
	{
		Atp_AdvPrintf("\n");
		Atp_DisplayIndent(indent);
		column = indent;
	}
}

/* Resets column number to 1st column. (ACST.60 - 24/7/93) */
void Atp_ResetManPgColumn _PROTO_((void))
{
	column = 1;
}

/*+*****************************************************************

	Function Name:		NoOfParmsInConstruct

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Counts number of parameters and constructs in
						the next level inside a construct.

	Modifications:
		Who			When				Description
	----------	---------------	-----------------------------
	Alwyn Teh	5 October 1993	Initial	Creation

******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static int NoOfParmsInConstruct
(
	ParmDefTable	parmdef,
	Atp_PDindexType	start_index,
	Atp_PDindexType	stop_index
)
#else
static int NoOfParmsInConstruct(parmdef,start_index,stop_index)
	ParmDefTable	parmdef;
	Atp_PDindexType	start_index, stop_index;
#endif
{
	register int idx = 0;
	int count = 0;

	if (isAtpBeginConstruct(parmdef[start_index].parmcode)) {
	  for (idx=start_index+1; idx < stop_index; idx++) {
		 if (isAtpBeginConstruct(parmdef[idx].parmcode)) {
		   count += 1;
		   idx = parmdef[idx].matchIndex;
		 }
		 else
		 if (isAtpRegularParm(parmdef[idx].parmcode))
		   count += 1;
	  }
	}
	else
	  return 0;

	return count;
}

/*+*******************************************************************

	Function Name:		Atp_EnumerateProtocolFieldValues
						compare_keyvalues

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research
						Northern Telecom

	Description:		Display field values and meanings
						in support of GENIE III protocol applications.

	Modifications:
		Who			When				Description
	----------	---------------	---------------------------
	Alwyn Teh	20-23 June 1995	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static int compare_keyvalues(const void *a, const void *b)
#else
static int compare_keyvalues(a,b)
	void *a;
	void *b;
#endif
{
	Atp_KeywordType *p = (Atp_KeywordType *)a;
	Atp_KeywordType *q = (Atp_KeywordType *)b;

	return ( p->KeyValue - q->KeyValue );
}

#if defined(__STDC__) || defined(__cplusplus)
void Atp_EnumerateProtocolFieldValues(Atp_KeywordType *KeyTabPtr, int indent)
#else
void
Atp_EnumerateProtocolFieldValues(KeyTabPtr, indent)
	Atp_KeywordType	*KeyTabPtr;
	int	indent
#endif
{
	register int x, y;

	unsigned int cur_bitwidth = 0;
	unsigned int max_bitwidth = 0;
	unsigned int max_keyvalue = 0;

	int start_column = 0;
	int extra_indent = 0;
	int final_indent = 0;
	int dec_width = 0;

	char max_keyvalue_str[BINARY_STRING_MAX_BITS+1];
	int curr_keyvalue = 0;

	int sorted = 1; /* assume sorted (true most of the time) */
	int nelem = 0;

	char *cur_desc = 0;
	char *nxt_desc = 0;

	char fmtstr[90], spaces [20];

	typedef int (*PFICS) _PROTO_((char *fmtstr, ... ));
	PFICS myprintf;

	if ( KeyTabPtr == NULL )
	  return;

	for ( x = 0; KeyTabPtr[x].keyword != NULL; x++ )
	{
	   curr_keyvalue = KeyTabPtr[x].KeyValue;

	   max_bitwidth = ( KeyTabPtr[x].internal_use > max_bitwidth ) ?
			   	   	   	   KeyTabPtr[x].internal_use : max_bitwidth;

	   max_keyvalue = ( curr_keyvalue > max_keyvalue )
							? curr_keyvalue
							: max_keyvalue;

	   /* Check if keyword table is sorted or needs sorting. */
	   if ( KeyTabPtr[x+1].keyword != NULL && sorted )
	   {
		 if ( curr_keyvalue <= KeyTabPtr[x+1].KeyValue )
		   sorted = 1;
		 else
		   sorted = 0;
	   }
	}
	nelem = x;

	max_bitwidth = ( max_bitwidth > BINARY_STRING_MAX_BITS ) ?
					 BINARY_STRING_MAX_BITS : max_bitwidth;

	max_keyvalue = ( max_keyvalue > UINT_MAX ) ? UINT_MAX : max_keyvalue;

	(void) sprintf( fmtstr, "%%%dd: %%%ds - %%s",
					dec_width =
					(sprintf(max_keyvalue_str, "%u", max_keyvalue),
								strlen(max_keyvalue_str)),
					max_bitwidth );

	if ( !sorted )
	  qsort(KeyTabPtr, nelem, sizeof(Atp_KeywordType), compare_keyvalues);

	if ( Atp_Used_By_G3O )
	{
	  myprintf = Atp_AdvPrintf;
	  extra_indent = 4;
	}
	else
	  myprintf = (PFICS)printf;

	start_column = indent + extra_indent;
	final_indent = start_column + dec_width + max_bitwidth + 5;

	(void) sprintf(spaces, "%%-%ds", start_column);

	for ( x = 0; KeyTabPtr[x].keyword != NULL; x++ )
	{
	   myprintf ("\n");
	   myprintf(spaces, ""); /* indentation to start_column */

	   /* field internal_use specifies bit width */
	   cur_bitwidth = (KeyTabPtr[x].internal_use > max_bitwidth)
							? max_bitwidth : KeyTabPtr[x].internal_use;

	   /* Detect excessive repeated descriptions not to be displayed. */
	   cur_desc = KeyTabPtr[x].KeywordDescription;
	   cur_desc = (cur_desc == 0 || *cur_desc == '\0') ? "" : cur_desc;
	   for (y = x+1; KeyTabPtr[y].keyword != NULL; y++ )
	   {
		  nxt_desc = KeyTabPtr[y].KeywordDescription;
		  nxt_desc = (nxt_desc == 0 || *nxt_desc == '\0') ? "" : nxt_desc;
		  if ( *cur_desc == *nxt_desc && strcmp(cur_desc,nxt_desc) == 0 )
			continue;
		  else
			break;
	   }

	   /* Print current line */
	   cur_desc = KeyTabPtr[x].KeywordDescription;
	   cur_desc = (cur_desc == 0 || *cur_desc == '\0') ? "?" : cur_desc;
	   Atp_PrintfWordWrap(	myprintf, -1, start_column, final_indent,
							fmtstr,
							KeyTabPtr[x].KeyValue,
							binary_string(KeyTabPtr[x].KeyValue, 0, cur_bitwidth),
							cur_desc );

	   /* Print "to" or ellipses if excessive repetitions detected. */
	   /* Then, jump to last repeated line for the next round. */
	   if ( (y - 1 - x) >= 3 )
	   {
		 char fmt[20];
		 int inner_indent = (max_bitwidth-1)/2;

		 myprintf ("\n");
		 myprintf(spaces, ""); /* indentation to start_column */

		 /* forward to start of bits */
		 (void) sprintf(fmt, "%%%ds", dec_width + 2);
		 myprintf(fmt, "");

		 /* centralize under bits */
		 (void) sprintf(fmt, "%%-%ds%%s", inner_indent);
		 myprintf(fmt, "", "to");

		 x = y - 2;
	   }
	}

	if ( !Atp_Used_By_G3O )
	  myprintf("\n");

	return;
}

/* binary_string() originally by Barry A. Scott - adapted from g3ouifor.cxx */
#if defined (__STDC__) || defined (__cplusplus)
static char *binary_string( int val, int start_pos, int val_size )
#else
static char *binary_string( val, start_pos, val_size )
	int val;
	int start_pos;
	int val_size;
#endif
{
	static char bits[BINARY_STRING_MAX_BITS+1];
	int first_char, bit;
	int err1 = 0, err2 = 0;

	memset( bits, ' ', BINARY_STRING_MAX_BITS );
	bits[BINARY_STRING_MAX_BITS] = '\0';

	if ((err1 = (val_size > BINARY_STRING_MAX_BITS)) ||
		(err2 = (start_pos > (val_size - 1))))
	{
	  fprintf(	stderr, __FILE__
				" : binary_string (val=%d, start_pos=%d,val_size=%d) \n",
				val, start_pos, val_size );
	  if ( err1 )
		fprintf(stderr, "\tval_size must be smaller than %d bits\n",
				BINARY_STRING_MAX_BITS);
	  if ( err2 )
		fprintf(stderr, "\tstart_pos greater than val_size-l !\n");

	  return bits;
	}

	first_char = (start_pos%8);
				 /* the width of this field's bits in this byte */

	for ( bit = 0; bit < val_size; bit++, val >>= 1 )
	   bits[BINARY_STRING_MAX_BITS - 1 - first_char - bit] =
			   (val & 1) != 0 ? '1' : 'O' ;

	return &bits[BINARY_STRING_MAX_BITS-val_size];
}

/*
* I thought I needed this, but I don't anymore, but I'll keep it around anyway.
*/
#if ( USE_DECIMAL_WIDTH == 1 )
#if defined(__STDC__) || defined(__cplusplus)
static int decimal_width( unsigned int no_of_bits }
#else
static int decimal_width( no_of_bits )
	unsigned int no_of_bits;
#endif
{
	int dec_width = 0;

	switch(no_of_bits)
	{
		case 0 : case 1 :
		case 2 : case 3 :	dec_width = 1; break;
		case 4 : case 5 :
		case 6 :			dec_width = 2; break;
		case 7 : case 8 :
		case 9 :			dec_width = 3; break;
		case 10 : case 11 :
		case 12 : case 13 :	dec_width = 4; break;
		case 14 : case 15 :
		case 16 :			dec_width = 5; break;
		case 17 : case 18 :
		case 19 :			dec_width = 6; break;
		case 20 : case 21 :
		case 22 : case 23 :	dec_width = 7; break;
		case 24 : case 25 :
		case 26 :			dec_width = 8; break;
		case 27 : case 28 :
		case 29 :			dec_width = 9; break
		case 30 : case 31 :
		case 32 :			dec_width = 10; break;
		default :			dec_width = 10; break;
	}

	return dec_width;
}
#endif
