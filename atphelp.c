/* EDITION AC07 (REL002), ITD ACST.178 (95/06/21 05:33:00) -- OPEN */

/*+*******************************************************************

	Module Name:		atphelp.c

	Copyright:			BNR Europe Limited,	1992 - 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		Implementation of On-Line HELP system.
						Contains parmdef table dump help, and
						command table display functionalities.

	Modifications:
		Who			When					Description
	----------	----------------	-------------------------------
	Alwyn Teh	9 September 1992	Modified from CLI
	Alwyn Teh	7 June 1993			Global change from
									Atp_PrintfHelpInfo() to
									Atp_AdvPrintf().
	Alwyn Teh	4	July 1993		Separate commands display
									code in Atp_DisplayCmdTable()
									into Atp_DisplayCommands()
									and Atp_DisplayCmdDescs()
									for use in atphelpc.c too.
	Alwyn Teh	28 July 1994		Command "?" discontinued, so
									ditch Atp_DisplayCmdTable().
	Alwyn Teh	1 August 1994		Word-wrap and indent long lines
									(see Atp_PrintfWordWrap())
	Alwyn Teh	8 March 1995		Use stdarg.h instead of
									varargs.h for ANSI compliance.
	Alwyn Teh	28 March 1995		Add BCD digits parameter type.
									Change %ld to %d because of
									change from long to int.
	Alwyn Teh	4 May 1995			Atp_GetUserDefinedProcNames()
									needed in order to separate
									from real built-in commands.
	Alwyn Teh	20 June 1995		Atp_VarargStrlen() renamed
									to Atp_FormatStrlen().
	Alwyn Teh	21 June 1995		Atp_EnumerateProtocolFieldValues
									used by Atp_DisplayParmEntry().
	Alwyn Teh	10 December 2021	Replaced MINDOUBLE with -DBL_MAX,
									and MAXDOUBLE with DBL_MAX. Also
									included float.h

********************************************************************-*/

#include <stdlib.h>
#include <limits.h>
// #include <values.h>
#include <string.h>
#include <float.h>

#if defined(__STDC__) || defined(__cplusplus)
#	include <stdarg.h>
#else
#	include cvarargs.h>
#endif

#include "atph.h"
#include "atpsysex.h"
#include "atpframh.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/* Typedefs */
typedef int (*qsort_compar) _PROTO_((const void *, const void *));

/* Constants */
#define PD_INDENT					3
#define DEFAULT_NAMELIST_NELEM		64
#define DEFAULT_MAX_CHARS_ON_LINE	132

/* Local functions */
static void Atp_DisplayParmEntry
					_PROTO_((int indent,
							 ParmDefEntry *ParmDefPtr,
							 int ParmDeflndex));

/* Initialising any internal global variables and functions. */
void *	(*Atp_GetCmdTabAccessRecord)_PROTO_((void *clientdata)) = NULL;
int		(*Atp_AdaptorUsed)_PROTO_((void * CmdEntryPtr)) = NULL;
void	(*Atp_ReturnDynamicHelpPage)_PROTO_((char *HelpPage, ...)) = NULL;
int		(*Atp_IsLangBuiltInCmd) _PROTO_((char *cmdname)) = NULL;

/*+*******************************************************************

	Function Name:			Atp_GenerateParmDefHelpInfo

	Copyright:				BNR Europe Limited, 1992, 1993
							Bell-Northern Research
							Northern Telecom

	Description:			Top level driver function for generating
							parmdef table dump help information.

	Modifications:
		Who			When				Description
	----------	-----------------	------------------------------
	Alwyn Teh	9 September 1992	Modified from CLI
	Alwyn Teh	7 June 1993			Use Atp_AdvPrintf().
	Alwyn Teh	23 June 1993		No longer need to
									check for paging here.
	Alwyn Teh	2 August 1994		Use Atp_PrintfWordWrap
									for possibly long strings.

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
char * Atp_GenerateParmDefHelpInfo( Atp_CmdRec * __CmdRecPtr )
#else
char *
Atp_GenerateParmDefHelplnfo(__CmdRecPtr)
	Atp_CmdRec *__CmdRecPtr;
#endif
{
	CommandRecord	*CmdRecPtr	= (CommandRecord *) CmdRecPtr;
	register int	indent		= PD_INDENT;
	register int	parm_idx	= 0;
	char			*HelpPage	= NULL;
	char			*error_msg	= NULL;

	error_msg = Atp_VerifyCmdRecParmDef((Atp_CmdRec *)CmdRecPtr);
	if (error_msg != NULL) {
	  return error_msg;
	}

	Atp_PrintfWordWrap(	Atp_AdvPrintf,
						-1, 1, 29,
						"\nParameter table for command: \"%s\" - %s\n\n",
						CmdRecPtr->cmdNameOrig, CmdRecPtr->cmdDesc );

	if ((CmdRecPtr->parmDef == NULL) || (Atp_IsEmptyParmDef(CmdRecPtr)))
	{
	  Atp_DisplayIndent(indent);
	  Atp_AdvPrintf("No parameters\n");
	}
	else
	while (parm_idx < CmdRecPtr->NoOfPDentries)
	{
	  if (isAtpBeginConstruct((CmdRecPtr->parmDef) [parm_idx].parmcode))
	  {
		Atp_DisplayParmEntry(indent, CmdRecPtr->parmDef, parm_idx);
		/*
		 *	EXCEPTION:
		 *	CASE is simulated as BEGIN_CASE and END_CASE. We want to
		 *	display just CASE. Any empty CASE'S will also be affected.
		 */
		if (((CmdRecPtr->parmDef)[parm_idx]).parmcode == ATP_BCS &&
			((CmdRecPtr->parmDef)[parm_idx+1]).parmcode == ATP_ECS)
		  parm_idx++; /* skip to END_CASE */
		else
		  indent += PD_INDENT;
	}
	else
	if (isAtpEndConstruct((CmdRecPtr->parmDef)[parm_idx].parmcode))
	{
	  indent -= PD_INDENT;
	  Atp_DisplayParmEntry(indent, CmdRecPtr->parmDef, parm_idx);
	}
	else
	  Atp_DisplayParmEntry(indent, CmdRecPtr->parmDef, parm_idx);

	  parm_idx++;
	}

	HelpPage = Atp_AdvGets();

	return HelpPage;
}

/*+*******************************************************************

	Function Name:			Atp_DisplayIndent

	Copyright:				BNR Europe Limited, 1992, 1993,	1994
							Bell-Northern Research
							Northern Telecom

	Description:			Displays indentation required.

	Modifications:
		Who			When				Description
	----------	----------------	-------------------------
	Alwyn Teh	9 September 1992	Initial creation
	Alwyn Teh	7 June 1993			Use Atp_AdvPrintf{).
	Alwyn Teh	2 August 1994		Ensure can handle any
									indent value by using
									printf format field width.

**************************************«*****************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_DisplayIndent( int indent )
#else
void Atp_DisplayIndent(indent)
	int indent; /* Number of spaces to indent by */
#endif
{
	char fmtstr[20];

	if (indent <= 0)
	  return;

	(void) sprintf(fmtstr, "%%-%ds", indent);

	Atp_AdvPrintf(fmtstr, "");

	return;
}

/*+*******************************************************************

	Function Name:			Atp_ParmTypeString

	Copyright:				BNR Europe Limited, 1992, 1993, 1995
							Be11-Northern Research
							Northern Telecom

	Description:			Atp_ParmTypeString() returns a static string
							representing the enumerated value of the ATP
							Parameter type.

	Modifications:
		Who			When				Description
	----------	----------------	------------------------------
	Alwyn Teh	9 September 1992	Modified from CLI
	Alwyn Teh	7 June 1993			Added CASE
	Alwyn Teh	28 March 1995		Add BCD digits

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
char * Atp_ParmTypeString ( int _parmcode_ )
#else
char * Atp_ParmTypeString (_parmcode_)
	int _parmcode_;
#endif
{
	Atp_ParmCode parmcode = (Atp_ParmCode)_parmcode_;

	switch (Atp_PARMCODE(parmcode)) {
		case ATP_BPM:		return("BEGIN_PARMS");
		case ATP_EPM:		return("END_PARMS");
		case ATP_BLS:		return("BEGIN_LIST");
		case ATP_ELS:		return("END_LIST");
		case ATP_BRP:		return("BEGIN_REPEAT");
		case ATP_ERP:		return("END_REPEAT");
		case ATP_BCH:		return("BEGIN_CHOICE");
		case ATP_ECH:		return("END_CHOICE");
		case ATP_BCS:		return("BEGIN_CASE");
		case ATP_ECS:		return("END_CASE");

		case ATP_NUM:		return("number");
		case ATP_UNS_NUM:	return("unsigned number");
		case ATP_REAL:		return("real number");
		case ATP_STR:		return("string");
		case ATP_BOOL:		return("boolean");
		case ATP_DATA:		return("data bytes");
		case ATP_BCD:		return("BCD digits");
		case ATP_KEYS:		return("keyword");
		case ATP_NULL:		return("null");
		case ATP_COM:		return("common parameter");

		default:			return("???");
	}
}

/*+*******************************************************************

	Function Name:		Atp_DisplayParmEntry

	Copyright:			BNR Europe Limited, 1992 - 1995
						Bell-Northern Research
						Northern Telecom

	Description:		Atp_DisplayParmEntry() prints out a ParmDef
						entry, in human-readable form. Handles all
						parameter and construct types. Should
						display all displayable attributes such as
						range.

	Modifications:
		Who			When				Description
	----------	-----------------	----------------------------
	Alwyn Teh	9 September 1992	Modified from CLI
	Alwyn Teh	7 June 1993			Added CASE
	Alwyn Teh	7 June 1993			Use Atp_AdvPrintf().
	Alwyn Teh	30 September 1993	Say which is DEFAULT for
									optional CHOICE.
									Change i/f slightly.
	Alwyn Teh	6 January 1994		Port to SUN workstation -
									suppress SUN's printf habit
									of displaying "(null)" when
									a string is NULL. This causes
									testcases to fail but which
									work on the HP. Fix this for
									the optional default value
									for the string parameter.
	Alwyn Teh	20 January 1994		Output "" when optional default
									choice name is NULL.
									Add 3rd parameter to function
									Atp_GetOptChoiceDefaultCaseName
	Alwyn Teh	3 August 1994		Wrap and indent long lines.
	Alwyn Teh	28 March 1995		Add BCD digits.

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void Atp_DisplayParmEntry
(
	int				indent,
	ParmDefEntry	*ParmDefPtr,
	int				ParmDefIndex
)
#else
static void
Atp_DisplayParmEntry(indent, ParmDefPtr, ParmDefIndex)
	int				indent;	/*	Number of spaces to indent by */
	ParmDefEntry	*ParmDefPtr;
	int				ParmDefIndex;
#endif
{
/*
-------------------------------
	Local MACRO Definitions
-------------------------------
*/
#define make_vproc_string(vproc) \
		((vproc == NULL) ? "" :	" {CUSTOM}")

#define make_presence_string(code) \
		(AtpParmIsOptional(code) ? " OPTIONAL" : "")

/*
-------------------------------
	Local Variables
-------------------------------
*/

	/*
	Reusable ASCII buffers,- faster to use static strings.
	*/
	static char DefNum[12]; /* shared by signed & unsigned numbers */
	static char DefReal[25];
	static char Def[24];

	Atp_ParmCode	parmcode;
	ParmDefEntry	*ParmEntryPtr; /* Points to the parmdef entry */

	register int index = 0; /* index counter */

	char *tmpname = NULL;
	char *parmTypeStr = NULL;

	ParmEntryPtr = &ParmDefPtr[ParmDefIndex];

	(void) strcpy(Def, " DEFAULT: ");
					/* this gets inserted before the default value */

	Atp_DisplayIndent(indent);

	parmcode = Atp_PARMCODE(ParmEntryPtr->parmcode);
	parmTypeStr = Atp_ParmTypeString(ParmEntryPtr->parmcode);

	switch(parmcode)
	{
		case ATP_BPM:
		case ATP_EPM:
		case ATP_ELS:
		case ATP_ERP:
		case ATP_ECH:
		case ATP_ECS:
			Atp_AdvPrintf("%s\n", parmTypeStr);
			break;

		case ATP_BLS:
		case ATP_BCS:
		case ATP_NULL: {
			char *tmp = (parmcode == ATP_BCS &&
						 (ParmEntryPtr+1)->parmcode == ATP_ECS) ? "CASE" :
						 parmTypeStr;

			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(tmp)+2,
				"%s (\"%s\" - %s)%s%s\n",
				tmp,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				make_presence_string(ParmEntryPtr->parmcode),
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
					(ParmEntryPtr->DataPointer != NULL) ?
					 " (DEFAULT NOT DISPLAYED)" : " (NO DEFAULT)"
				);
			break;
		}

		case ATP_UNS_NUM:
			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent, indent+5,
				"%s (\"%s\" - %s) [%lu to %lu]%s%s%s\n",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_UnsNumType) ?
						(Atp_UnsNumType)0 :
						(Atp_UnsNumType)ParmEntryPtr->Min,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_UnsNumType) ?
						(Atp_UnsNumType)UINT_MAX :
						(Atp_UnsNumType)ParmEntryPtr->Max,
				make_vproc_string(ParmEntryPtr->vproc),
				make_presence_string(ParmEntryPtr->parmcode),
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
						strcat(Def, ((void) sprintf(DefNum,"%lu",
								(Atp_UnsNumType)ParmEntryPtr->Default),
								DefNum))
				);

			if ( Atp_Used_By_G3O && ParmEntryPtr->KeyTabPtr != NULL )
			{
				Atp_EnumerateProtocolFieldValues(ParmEntryPtr->KeyTabPtr,
												 indent);
				Atp_AdvPrintf("\n\n");
			}
			break;

		case ATP_BRP:
			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(parmTypeStr)+2,
				"%s (\"%s\" - %s)	[%d to %d instances]%s%s%s\n",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						/* you're allowed zero instances */
						(Atp_NumType)0 :
						/* Min specified, but is it illegally
						   negative ? */
						((Atp_NumType)ParmEntryPtr->Min < 0) ?
						/* min is -ve, force it to be zero */
						(Atp_NumType)0 :
						(Atp_NumType)ParmEntryPtr->Min, /* min is ok */
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						(Atp_NumType)INT_MAX :
						(Atp_NumType)ParmEntryPtr->Max,
				make_vproc_string(ParmEntryPtr->vproc),
				make_presence_string(ParmEntryPtr->parmcode),
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
					(ParmEntryPtr->DataPointer != NULL) ?
					 " (DEFAULT NOT DISPLAYED)" : " (NO DEFAULT)"
				);
			break;

		case ATP_DATA: {
			Atp_DataDescriptor *def_hex_desc = (Atp_DataDescriptor *)
													ParmEntryPtr->DataPointer;
			char *def_hex = (def_hex_desc != NULL) ?
								Atp_DisplayHexBytes(*def_hex_desc, " ") : NULL;
			char *default_hex = " (NULL)";

			if (def_hex != NULL)
			  Atp_DvsPrintf(&default_hex, " DEFAULT: {%s}", def_hex);

			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(parmTypeStr)+2,
				"%s (\"%s\" - %s)	[%d to %d bytes]%s%s%s\n",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						/* must have at least one byte of data */
						(Atp_NumType)1 :
						/* min specified, but is it illegally
						   negative ? */
						((Atp_NumType)ParmEntryPtr->Min < 0) ?
						/* min is -ve, force it to be zero */
						(Atp_NumType)0 :
						(Atp_NumType)ParmEntryPtr->Min, /* min is ok */
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						(Atp_NumType)INT_MAX :
						(Atp_NumType)ParmEntryPtr->Max,
				make_vproc_string(ParmEntryPtr->vproc),
				make_presence_string(ParmEntryPtr->parmcode),
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
					(ParmEntryPtr->DataPointer != NULL) ?
					 default_hex : " (NO DEFAULT)"
				);

			if (def_hex != NULL)
			{
			  FREE(def_hex);
			  if (default_hex != NULL)
				FREE(default_hex);
			}
			break;
		}

		case ATP_BCD: {
			Atp_DataDescriptor *def_bcd_desc = (Atp_DataDescriptor *)
													ParmEntryPtr->DataPointer;
			char *def_bcd = (def_bcd_desc != NULL) ?
								Atp_DisplayBcdDigits(*def_bcd_desc) : NULL;
			char *default_bcd = " (NULL)";

			if (def_bcd != NULL)
			  Atp_DvsPrintf(&default_bcd, " DEFAULT: {%s}", def_bcd);

			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(parmTypeStr)+2,
				"%s (\"%s\" - %s)	[%d to %d digits]%s%s%s\n",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						/* must have at least one BCD digit */
						(Atp_NumType)1 :
						/* min specified, but is it illegally negative ? */
						((Atp_NumType)ParmEntryPtr->Min < 0) ?
						/* min is -ve, force it to be zero */
						(Atp_NumType)0 :
						(Atp_NumType)ParmEntryPtr->Min, /* min is ok */
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						(Atp_NumType)INT_MAX :
						(Atp_NumType)ParmEntryPtr->Max,
				make_vproc_string(ParmEntryPtr->vproc),
				make_presence_string(ParmEntryPtr->parmcode),
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
					(ParmEntryPtr->DataPointer != NULL) ?
					 default_bcd : " (NO DEFAULT)"
				);

			if (def_bcd != NULL)
			{
			  FREE(def_bcd);
			  if (default_bcd != NULL)
			    FREE(default_bcd);
			}
			break;
		}

		case ATP_NUM:
			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(parmTypeStr)+2,
				"%s (\"%s\" - %s)	[%d to %d] %s%s%s\n",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						(Atp_NumType)INT_MIN :
						(Atp_NumType)ParmEntryPtr->Min,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						(Atp_NumType)INT_MAX :
						(Atp_NumType)ParmEntryPtr->Max,
				make_vproc_string(ParmEntryPtr->vproc) ,
				make_presence_string(ParmEntryPtr->parmcode),
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
						strcat(Def, ((void) sprintf(DefNum,"%d",
								(Atp_NumType)ParmEntryPtr->Default),
								DefNum))
				);

			if ( Atp_Used_By_G3O && ParmEntryPtr->KeyTabPtr != NULL )
			{
			  Atp_EnumerateProtocolFieldValues(ParmEntryPtr->KeyTabPtr,
					  	  	  	  	  	  	   indent);
			  Atp_AdvPrintf("\n\n");
			}
			break;

		case ATP_STR: {
			char *defstr = (char *)ParmEntryPtr->DataPointer;

			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(parmTypeStr)+2,
				"%s (\"%s\n - %s)	[%d to %d characters]%s%s%s%s%s%s\n",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						(Atp_NumType)0 : /* i.e. empty string: "" */
						/* min specified, but is it illegally
						negative ? */
						(Atp_NumType)ParmEntryPtr->Min < 0 ?
						(Atp_NumType)0 : (Atp_NumType)ParmEntryPtr->Min,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_NumType) ?
						(Atp_NumType)INT_MAX :
						(Atp_NumType)ParmEntryPtr->Max,
				make_vproc_string(ParmEntryPtr->vproc),
				make_presence_string(ParmEntryPtr->parmcode),
						(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ?
						"" : Def,
						(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ?
						"" : "\"",
						(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ?
						"" : (defstr == NULL) ?	: defstr,
						(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ?
						"" : "\""
				);
			break;
		}

		case ATP_REAL:
			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(parmTypeStr)+2,
				"%s (\"%s\" - %s)	[%g to %g] %s%s%s\n",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_RealType) ?
						(Atp_RealType)-DBL_MAX : ParmEntryPtr->Min,
				ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, Atp_RealType) ?
						(Atp_RealType)DBL_MAX : ParmEntryPtr->Max,
				make_vproc_string(ParmEntryPtr->vproc),
				make_presence_string(ParmEntryPtr->parmcode),
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
						strcat(Def, ((void)sprintf(DefReal,"%g",
								(Atp_RealType)ParmEntryPtr->Default),
								DefReal))
				);
			break;

		case ATP_BCH:
			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(parmTypeStr)+2,
				"%s {\"%s\" - %s)%s%s%s%s\n",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				make_vproc_string(ParmEntryPtr->vproc),
				make_presence_string(ParmEntryPtr->parmcode),
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
						" DEFAULT: ",
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
				((tmpname = Atp_GetOptChoiceDefaultCaseName(
								ParmDefPtr, ParmDefIndex, NULL)) == NULL) ?
						"\"\"" : tmpname
				);
			break;

		case ATP_BOOL:
			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(parmTypeStr)+2,
				"%s (\"%s\" - %s)%s%s%s\n",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				make_vproc_string(ParmEntryPtr->vproc),
				make_presence_string(ParmEntryPtr->parmcode),
				(!AtpParmIsOptional(ParmEntryPtr->parmcode)) ? "" :
				((Atp_BoolType)ParmEntryPtr->Default == TRUE) ?
						" DEFAULT: TRUE" : " DEFAULT: FALSE"
				);
			break;

		case ATP_KEYS:
			Atp_PrintfWordWrap(Atp_AdvPrintf, -1, indent,
				indent+strlen(parmTypeStr)+2,
				"%s (\"%s\" - %s)%s%s",
				parmTypeStr,
				ParmEntryPtr->Name,
				ParmEntryPtr->Desc,
				make_vproc_string(ParmEntryPtr->vproc),
				make_presence_string(ParmEntryPtr->parmcode)
				);
			Atp_AdvPrintf("\n");
			Atp_DisplayIndent(indent);
			Atp_AdvPrintf("{");
			for (index = 0;
				 ((ParmEntryPtr->KeyTabPtr)[index].keyword != NULL);
				 index++)
			{
				char *Keyword = NULL;
				char *Keydesc = NULL;
				char *Default = NULL;
				int column = 0;

				Keyword = (ParmEntryPtr->KeyTabPtr)[index].keyword;
				Keydesc = (ParmEntryPtr->KeyTabPtr)[index].KeywordDescription;
				Default = (!AtpParmIsOptional(ParmEntryPtr->parmcode)) ?
								"" :
							((Atp_NumType)ParmEntryPtr->Default ==
							 (ParmEntryPtr->KeyTabPtr)[index].KeyValue) ?
								" (DEFAULT)" : "",

				Atp_AdvPrintf("\n");
				Atp_DisplayIndent(column = indent + PD_INDENT);
				column =
				Atp_PrintfWordWrap(
					Atp_AdvPrintf, -1, column,
					column+strlen(Keyword)+strlen(Default)+5,
					"\"%s\"%s%s%s",
					(Keyword) ? Keyword : "",
					(Default) ? Default : "",
					(Keydesc) ? " - " : "",
					(Keydesc) ? Keydesc : "");
			}
			Atp_AdvPrintf("\n");
			Atp_DisplayIndent(indent);
			Atp_AdvPrintf("}\n");
			break;

		case ATP_COM:
			Atp_AdvPrintf("COMMON PARMDEF TYPE, NOT IMPLEMENTED.\n");
			break;
		default:
			Atp_AdvPrintf("(SYSTEM ERROR: *** INTERNAL ERROR *** Unknown parameter type encountered.)\n");
			break;
		}

		return;

#undef make_presence_string
#undef make_vproc_string
}

/*+*****************************************************************

	Function Name:			Atp_GetCmdTables
	Atp_GetBuiltInCmdNames
	Atp_GetUserDefinedProcNames
	Atp_GetApplCmdRecs

	Copyright:	BNR Europe Limited, 1993, 1994, 1995
	Bell-Northern Research
	Northern Telecom

	Description:	Generates command	table in a suitable
	display format. Handles built-in commands
	and ATP commands.

	Modifications:
		Who			When				Description
	----------	-------------	----------------------------
	Alwyn Teh	27 July 1993	Created from function
								Atp_DisplayCmdTable()
	Alwyn Teh	15 July 1994	Fix memory fault core dump
								bug when doing keyword search
								by checking indices properly
								and NULL terminating lists.
	Alwyn Teh	22 July 1994	Don't alloc memory if
								pointers aren't supplied.
	Alwyn Teh	22 July 1994	Test and fix unused functions
								Atp_GetBuiItInCmdNames() and
								Atp_GetApp1CmdRecs().
	Alwyn Teh	17 March 1995	Support ANSI C and simplify
								interface, passing clientData
								straight in.
	Alwyn Teh	4 May 1995		Atp_GetUserDefinedProcNames()
								added. Atp_GetCmdTables()
								changed.

******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_GetCmdTables
(
	void				*clientData,
	char **				*BuiltInCmdTabPtr,
	int					*BuiltInCmdsCountPtr,
	int					*BuiltInCmdNameWidthPtr,
	char **				*UserProcsTabPtr,
	int					*UserProcsCountPtr,
	int					*UserProcsNameWidthPtr,
	CommandRecord **	*AtpApplCmdTabPtr,
	int					*AtpApplCmdsCountPtr,
	int					*AtpApplCmdNameWidthPtr
)
#else
void Atp_GetCmdTables(
				clientData,
				BuiltInCmdTabPtr, BuiltInCmdsCountPtr, BuiltInCmdNameWidthPtr,
				UserProcsTabPtr, UserProcsCountPtr, UserProcsNameWidthPtr,
				AtpApplCmdTabPtr, AtpApp1CmdsCountPtr, AtpApplCmdNameWidthPtr)
	void				*clientData;
	char **				*BuiltInCmdTabPtr;
	int					*BuiltInCmdsCountPtr, *BuiltInCmdNameWidthPtr;
	char **				*UserProcsTabPtr;
	int					*UserProcsCountPtr, *UserProcsNameWidthPtr;
	CommandRecord **	*AtpApplCmdTabPtr;
	int					*AtpApplCmdsCountPtr, *AtpApplCmdNameWidthPtr;
#endif
{
	char					**BuiltInCmds=0, **UserProcs=0, *tmpName=0;
	CommandRecord			**AtpApplCmds=0, *CmdRecPtr=0;
	void					*CmdTablePtr=0, *CmdEntryPtr=0;
	int						BuiltInCmdsCount=0, AtpApplCmdsCount=0;
	int						BuiltInCmdNameWidth=0, AtpApplCmdNameWidth=0;
	int						len=0;
	int						UserProcsCount=0, UserProcsNameWidth=0;
	Atp_CmdTabAccessType	*CmdTableAccessDesc=0;
	int						CurrBuiltInCmdsListSize=0,
							CurrUserProcsListSize=0,
							CurrAtpApplCmdsListSize=0;

	/*
		Get command table access descriptor record pointer from
		the call stack.
	 */
	CmdTableAccessDesc =
		(Atp_CmdTabAccessType *)
		(*Atp_GetCmdTabAccessRecord)(clientData);

	/* Get pointer to external command table. */
	CmdTablePtr = CmdTableAccessDesc->CommandTablePtr;

	/* Initialisations */
	CurrBuiltInCmdsListSize =
		CurrUserProcsListSize =
			CurrAtpApplCmdsListSize = DEFAULT_NAMELIST_NELEM;

	/* Create temporary command name lists. */
	if (BuiltInCmdTabPtr != NULL)
	  BuiltInCmds = (char **) CALLOC(CurrBuiltInCmdsListSize,
			  	  	  	  	  	  	  sizeof(char *), NULL);
	if (UserProcsTabPtr != NULL)
	  UserProcs = (char **) CALLOC(CurrUserProcsListSize,
			  	  	  	  	  	  	  sizeof(char *), NULL);
	if (AtpApplCmdTabPtr != NULL)
	  AtpApplCmds = (CommandRecord **) CALLOC(CurrAtpApplCmdsListSize,
			  	  	  	  	  	  	  	  	  sizeof(CommandRecord *),NULL);

	/* Scan command table and generate name lists. */
	for (/* Get pointer to first entry in command table. */
			CmdEntryPtr = (*CmdTableAccessDesc->FirstCmdTabEntry)(CmdTableAccessDesc);
		 /* Test if end of table. */
			(CmdEntryPtr != NULL);
		 /* Get next command table entry. */
			CmdEntryPtr = (*CmdTableAccessDesc->NextCmdTabEntry)(CmdTableAccessDesc)
	)
	{
		/* Get command entry name. */
		tmpName = (*CmdTableAccessDesc->CmdName)(CmdTablePtr, CmdEntryPtr);

		if (tmpName != NULL && *tmpName != '\0' && !Atp_IsHiddenCommandName(tmpName))
		  /* Decide to which name list it belongs. */
		  if ((*Atp_AdaptorUsed)(CmdEntryPtr))
		  {
			if (AtpApplCmdTabPtr == NULL)
			  continue;

			/* Get entry's original command name and description. */
			CmdRecPtr = (CommandRecord *)(*CmdTableAccessDesc->CmdRec)(CmdEntryPtr);

			AtpApplCmds[AtpApplCmdsCount++] = CmdRecPtr;

			/* Resize application command name list if necessary. */
			if (AtpApplCmdsCount >= CurrAtpApplCmdsListSize) {
			  register int i;
			  CurrAtpApplCmdsListSize += DEFAULT_NAMELIST_NELEM;
			  AtpApplCmds = (CommandRecord **)
							REALLOC(AtpApplCmds,
									(CurrAtpApplCmdsListSize *
									sizeof(CommandRecord *)), NULL);

			  /* NULL-terminate list by zeroing ALL unused slots */
			  for (i = AtpApplCmdsCount; i < CurrAtpApplCmdsListSize; i++)
				 AtpApplCmds[i] = NULL;
			}

			/* Determine field width for command name. */
			if (AtpApplCmdNameWidth < (len = strlen(CmdRecPtr->cmdName)))
			  AtpApplCmdNameWidth = len;
		  }
		  else
		  if (Atp_IsLangBuiltInCmd(tmpName))
		  {
			if (BuiltInCmdTabPtr == NULL)
			  continue;

			BuiltInCmds[BuiltInCmdsCount++] = tmpName;

			/* Resize build-in command name list if necessary. */
			if (BuiltInCmdsCount >= CurrBuiltInCmdsListSize) {
			  register int i;
			  CurrBuiltInCmdsListSize += DEFAULT_NAMELIST_NELEM;
			  BuiltInCmds = (char **)
							REALLOC(BuiltInCmds,
									(CurrBuiltInCmdsListSize *
									sizeof(char *)), NULL);

			  /* NULL-terminate list by zeroing ALL unused slots */
			  for (i = BuiltInCmdsCount; i < CurrBuiltInCmdsListSize; i++)
			     BuiltInCmds[i] = NULL;
			}

			/* Determine field width for command name. */
			if (BuiltInCmdNameWidth < (len = strlen(tmpName)))
			  BuiltInCmdNameWidth = len;
		  }
		  else /* must be user-defined procs */
		  {
			  if (UserProcsTabPtr == NULL)
				continue;

			  UserProcs[UserProcsCount++] = tmpName;

			  /* Resize build-in command name list if necessary. */
			  if (UserProcsCount >= CurrUserProcsListSize) {
				register int i;
				CurrUserProcsListSize += DEFAULT_NAMELIST_NELEM;
				UserProcs = (char **)
									REALLOC(UserProcs,
											(CurrUserProcsListSize *
											sizeof(char *)), NULL);
				/* NULL-terminate list by zeroing ALL unused slots */
				for (i = UserProcsCount; i < CurrUserProcsListSize; i++)
				   UserProcs[i] = NULL;
			  }

			  /* Determine field width for command name. */
			  if (UserProcsNameWidth < (len = strlen(tmpName)))
				UserProcsNameWidth = len;
		  }
	}

	/* Return built-in command table and attributes. */
	if (BuiltInCmdTabPtr != NULL) {
	  *BuiltInCmdTabPtr = BuiltInCmds;
	  if (BuiltInCmdsCountPtr != NULL)
	    *BuiltInCmdsCountPtr = BuiltInCmdsCount;
	  if (BuiltInCmdNameWidthPtr != NULL)
	    *BuiltInCmdNameWidthPtr = BuiltInCmdNameWidth;
	}

	/* Return user-defined procs table and attributes. */
	if (UserProcsTabPtr != NULL) {
	  *UserProcsTabPtr = UserProcs;
	  if (UserProcsCountPtr != NULL)
		*UserProcsCountPtr = UserProcsCount;
	  if (UserProcsNameWidthPtr != NULL)
		*UserProcsNameWidthPtr = UserProcsNameWidth;
	}

	/* Return ATP application command table and attributes. */
	if (AtpApplCmdTabPtr != NULL) {
	  *AtpApplCmdTabPtr = AtpApplCmds;
	  if (AtpApplCmdsCountPtr != NULL)
	    *AtpApplCmdsCountPtr = AtpApplCmdsCount;
	  if (AtpApplCmdNameWidthPtr != NULL)
	    *AtpApplCmdNameWidthPtr = AtpApplCmdNameWidth;
	}
}

/* Function to get built-in command names ONLY. */
#if defined(__STDC__) || defined(__cplusplus)
char ** Atp_GetBuiltInCmdNames
(
	void	*clientData,
	int		*BuiltInCmdsCountPtr,
	int		*BuiltInCmdNameWidthPtr
)
#else
char ** Atp_GetBuiltInCmdNames( clientData,
								Bui1tInCmdsCountPtr,
								BuiltInCmdNameWidthPtr )
	void	*clientData;
	int		*BuiltInCmdsCountPtr;
	int		*BuiltInCmdNameWidthPtr;
#endif
{
	char **BuiltInCmds = NULL;

	Atp_GetCmdTables(clientData,
					 &BuiltInCmds, BuiltInCmdsCountPtr, BuiltInCmdNameWidthPtr,
					 NULL, NULL, NULL,
					 NULL, NULL, NULL);

	return BuiltInCmds;
}

/* Function to get built-in command names ONLY. */
#if defined(__STDC__) || defined(__cplusplus)
char ** Atp_GetUserDefinedProcNames
(
	void *clientData,
	int *UserProcsCountPtr,
	int *UserProcsNameWidthPtr
)
#else
char ** Atp_GetUserDefinedProcNames(clientData,
									UserProcsCountPtr,
									UserProcsNameWidthPtr )
	void	*clientData;
	int		*UserProcsCountPtr;
	int		*UserProcsNameWidthPtr;
#endif
{
	char **UserProcs = NULL;

	Atp_GetCmdTables(clientData,
			NULL, NULL, NULL,
			&UserProcs, UserProcsCountPtr, UserProcsNameWidthPtr,
			NULL, NULL, NULL);

	return UserProcs;
}

/* Function to get ATP application command records ONLY. */
#if defined(__STDC__) || defined(__cplusplus)
CommandRecord ** Atp_GetApplCmdRecs
(
	void	*clientData,
	int		*AtpApplCmdsCountPtr,
	int		*AtpApplCmdNameWidthPtr
)
#else
CommandRecord ** Atp_GetApplCmdRecs ( clientData,
									  AtpApplCmdsCountPtr,
									  AtpApplCmdNameWidthPtr )
	void	*clientData;
	int		*AtpApplCmdsCountPtr;
	int		*AtpApplCmdNameWidthPtr;
#endif
{
	CommandRecord **AtpApplCmds = NULL;

	Atp_GetCmdTables(clientData,
					 NULL, NULL, NULL,
					 NULL, NULL, NULL,
					 &AtpApplCmds, AtpApplCmdsCountPtr, AtpApplCmdNameWidthPtr);

	return AtpApplCmds;
}

/*+*****************************************************************

	Function Name:		Atp_DisplayCommands

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Displays command names in table format.

	Modifications:
		Who			When			Description
	----------	--------------	-------------------------
	Alwyn Teh	4 July 1993		Initial Creation
	Alwyn Teh	24 July 1993	Enable function to
								handle any command
								array pointer type.
	Alwyn Teh	20 March 1995	Replaced ceil() with
								integer calculation of
								number of rows.

******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_DisplayCommands
(
	void *	*voidptrs,
	char *	(*cmdnamefunc)(void *),
	int		(*comparator)(const void *, const void *),
	int		count,
	int		name_width
)
#else
int Atp_DisplayCommands(voidptrs, cmdnamefunc, comparator, count, name_width)
	void *	*voidptrs;			/* pointer to array of unknown type pointers */
	char *	(*cmdnamefunc)();	/* function for extracting command name */
	int		(*comparator)();	/* function for comparing array members */
	int		count;
	int		name_width;
#endif
{
#define GAP 3
	register int x, y;
	int cols, rows;
	int index = 0, colenv = ATP_DEFAULT_COLUMNS;
	char *column_env_str = getenv("COLUMNS");
	char fmtstr[32];

	/* If no command names supplied, return error. */
	if (voidptrs == NULL)
	  return 0;

	/* If number of command names not supplied, try counting. */
	if (count <= 0) {
	  for (x=0; voidptrs[x] != NULL; x++) ;
	  count = x;
	}

	/* If name field width not supplied, find out. */
	if (name_width <= 0) {
	  int len = 0;
	  char *cmdname = NULL;

	  name_width = 0;
	  for (name_width = 0, x = 0; x < count; x++)
	     if (voidptrs [x] != NULL &&
	    	 (cmdname = cmdnamefunc(voidptrs[x])) != NULL &&
			 *cmdname != '\0')
	       if (name_width < (len = strlen(cmdname)))
	    	 name_width = len;
	}

	/* Quick sort command list into alphabetical order. */
	qsort(voidptrs, count, sizeof(char *), comparator);

	/*
	 *	Calculate number of columns to use (incl. 2 char left-hand margin).
	 *	colenv is the terminal screen width, and cols is no. of columns of
	 *	commands to display.
	 */
	if (column_env_str != NULL && *column_env_str != '\0')
	  colenv = ((colenv = atoi(column_env_str)) > 0) ?
			     colenv : ATP_DEFAULT_COLUMNS;
	  cols = (colenv - 2) / (name_width + GAP);

	/* Must be at least 1 column. */
	if (cols <= 0)
	  cols = 1;

	/* Calculate number of rows to use. */
	rows = (count + (cols - 1)) / cols;

	/* Make format strings (incl. GAPs between names). */
	(void) sprintf(fmtstr, "%%-%ds	", name_width);
					/* includes trailing space of size GAP */

	/* Print matrix of command names (incl. 2 char left-hand margin). */
	Atp_AdvPrintf("  ") ;
	for (y = 0; y < rows; y++) {
	   for (x = 0; x < cols; x++) {
		  index = x * rows + y;
		  if (index < count && voidptrs[index] != NULL)
			Atp_AdvPrintf(fmtstr, cmdnamefunc(voidptrs[index]));
	   }
	   Atp_AdvPrintf("\n ");
	}

	return 1;

#undef GAP
}

/*******************************************************************

	Function Name:			Atp_DisplayCmdDescs

	Copyright:				BNR Europe Limited, 1993
							Bell-Northern Research
							Northern Telecom

	Description:			Displays command names and	descriptions
							in list format.

	Modifications:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	4 July 1993		Initial Creation
	Alwyn Teh	3 August 1994	Wrap and indent long lines.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_DisplayCmdDescs( void *cmdrecs, int count, int name_width )
#else
int Atp_DisplayCmdDescs(cmdrecs, count, name_width)
	void	*cmdrecs;
	int		count;
	int		name_width;
#endif
{
	register int x;
	char fmtstr[32];
	CommandRecord **CmdRecs = (CommandRecord **) cmdrecs;

	/* If no command names supplied, return error. */
	if (cmdrecs == NULL)
	  return 0;

	/* If number of command names not supplied, try counting. */
	if (count <= 0) {
	  for (x=0; CmdRecs[x] != NULL; x++) ;
	  	 count = x;
	}

	/* If name field width not supplied, find out. */
	if (name_width <= 0) {
	  int len = 0;
	  name_width = 0;
	  for (name_width = 0, x = 0; x < count; x++)
		 if (CmdRecs[x]->cmdNameOrig != NULL &&
		   *CmdRecs[x]->cmdNameOrig != '\0')
		   if (name_width < (len = strlen(CmdRecs[x]->cmdNameOrig)))
			 name_width = len;
	}

	/* Sort command lists into alphabetical order. */
	qsort(CmdRecs, count, sizeof(CommandRecord *),
		  (qsort_compar)Atp_CompareCmdRecNames);

	/* Make format string. */
	(void) sprintf(fmtstr, "  %%-%ds  -  %%s\n", name_width);

	/* Display names and descriptions of commands. */
	for (x = 0; x < count; x++) {
	   Atp_PrintfWordWrap(Atp_AdvPrintf,
						  -1, 1, name_width + 7,
						  fmtstr,
						  CmdRecs[x]->cmdNameOrig, CmdRecs[x]->cmdDesc);
	}

	return 1;
}

/*+********************************************************************

	Function Name:		Atp_CompareNames

	Copyright:			BNR Europe Limited,	1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Name comparison routine for use with qsort.

	Modifications:
		Who			When					Description
	----------	------------------	-----------------------------
	Alwyn Teh	16 September 1992	Initial	Creation

*********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_CompareNames( char **sp1, char **sp2 )
#else
int Atp_CompareNames(sp1, sp2)
	char **sp1, **sp2;
#endif
{
	int result = 0;

	result = Atp_Strcmp(*sp1, *sp2);

	return result;
}

/*+*******************************************************************

	Function Name:			Atp_CompareCmdRecNames

	Copyright:				BNR Europe Limited, 1992, 1993
							Bell-Northern Research
							Northern Telecom

	Description:			Command record name comparison	routine for
							use by qsort.

	Modifications:
		Who			When				Description
	-----------	-----------------	-----------------------------
	Alwyn Teh	16 September 1992	Initial Creation
	Alwyn Teh	1 July 1993			Changed argument types
	Alwyn Teh	24 July 1993		Export function, renamed from
									CompareNameDescRecords()

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_CompareCmdRecNames( CommandRecord **rec1p, CommandRecord **rec2p )
#else
int Atp_CompareCmdRecNames(rec1p, rec2p)
	CommandRecord **rec1p, **rec2p;
#endif
{
	int result = 0;

	result = Atp_Strcmp ( (*rec1p)->cmdNameOrig, (*rec2p)->cmdNameOrig);

	return result;
}

/*+*******************************************************************

	Function Name:		Atp_PrintfWordWrap

	Copyright:			BNR Europe Limited, 1994
						Bell-Northern Research
						Northern Telecom

	Description:		Check if line needs wrapping, if so wrap and
						then indent where necessary. Returns column
						number.

						Assumes resultant string contains no newline
						or cursor positional characters except for a
						trailing newline.

	Modifications:
		Who			When			Description
	----------	--------------	-----------------------------
	Alwyn Teh	1 August 1994	Initial Creation
	Alwyn Teh	29 March 1995	Fix infinite loop bug
								when no spaces in string.
								Can break line up at some
								punctuation marks where it
								reads better.
	Alwyn Teh	20 June 1995	Atp_VarargStrlen() renamed
								to Atp_FormatStrlen().

*******************************************************************_*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_PrintfWordWrap
(
	int (*printf_function)(char *fmtstr, ...),
	int screen_width,
	int start_column,
	int indent,
	char *format_string,
	...
)
#else
int Atp_PrintfWordWrap( printf_function,
						screen_width, start_colutnn, indent.
						format_string, va_alist )
	int		(*printf_function)();
	int		screen_width, start_column, indent;
	char	*format_string;
	va_dcl
#endif
{
#define RIGHT_MARGIN 2

	va_list ap;
	Atp_CallFrame callframe;
	char *string, *wrapPtr, *endlinePtr, *line, *endPtr, *stop;
	char spaces[20];
	int column = start_column;
	int len = 0, remain = 0, wrappedLine = 0;

	if (format_string == NULL || *format_string == '\0')
	  return column;

	if (printf_function == NULL)
	  return column;

#if defined(__STDC__) || defined (__cplusplus)
	va_start(ap, format_string);
#else
	va_start(ap);
#endif

	Atp_CopyCallFrame(&callframe, ap);

	if (start_column <= 0)
	  start_column = 1;

	/* Find screen width if not supplied. */
	if (screen_width <= 0) {
	  char *column_env_str = getenv("COLUMNS");
	  int colenv;
	  if (column_env_str != NULL && *column_env_str != '\0')
		screen_width = ((colenv = atoi(column_env_str)) > 0) ?
											colenv : ATP_DEFAULT_COLUMNS;
	  else
		screen_width = ATP_DEFAULT_COLUMNS;
	}

	if (indent < 0)
	  indent = 0;

	if (start_column > screen_width) {
	  int len = Atp_FormatStrlen(format_string, ap);
	  column = screen_width - column;
	  (*printf_function)(format_string, ATP_FRAME_RELAY(callframe));
	  column += len;
	  return column;
	}
	va_end(ap);

	Atp_DvsPrintf(&string, format_string, ATP_FRAME_RELAY(callframe));

	line = string;
	column = start_column;
	(void) sprintf(spaces, "%%-%ds", indent);
	screen_width -= RIGHT_MARGIN;
	remain = screen_width - column; /* room remaining on row */
	len = strlen(line);
	endPtr = string + len;

	while ((line != NULL) && (*line != '\0') && (len > remain))
	{
	  if (len > remain)
		wrapPtr = endlinePtr = line + remain;
	  else
		wrapPtr = endlinePtr = line + len;

	  /*
		  Find word break to wrap line in last 25 characters, if any.
		  If can't find space, try to break on punctuation mark.
	   */
	  stop = wrapPtr - 25;
	  stop = (stop < line) ? line : stop;
	  while ((wrapPtr > stop) && (*wrapPtr != '\0'))
	  {
		   if ((isspace(*wrapPtr)) ||
			   ((*wrapPtr == '.' || *wrapPtr == ',' ||
			     *wrapPtr == '-' || *wrapPtr == '/') &&
				 isalpha(wrapPtr[-1]) && isalpha(wrapPtr[1])) )
			 break;
		   wrapPtr--;
	  }
	  if (wrapPtr == stop)
	  {
		/*
			Note: Don't break on periods and commas in numbers such as
				  9.80665 and 4,095 but ok in sentences.
		*/
		wrapPtr = endlinePtr; /* start again */
		while (wrapPtr > stop && *wrapPtr != '\0')
		{
			if ((*wrapPtr == '/') || /* e.g. long pathnames */
				((*wrapPtr == '.' || *wrapPtr == ',') &&
					!(isdigit(wrapPtr[-1]) && isdigit(wrapPtr[1]))) ||
				(*wrapPtr == ':') || (*wrapPtr == ';') ||
				(*wrapPtr == '-') || (*wrapPtr == '+')	||
				(*wrapPtr == '=') || (*wrapPtr == '*')	||
				(*wrapPtr == '&') || (*wrapPtr == '#')	)
			  break;
			wrapPtr--;
		}
	  }

	  /*
			Wrap line where space is found.
			If punctuation instead, wrap after it.
	   */
	  if (wrapPtr != line && (isspace(*wrapPtr) || ispunct(*wrapPtr)))
	  {
	    char saveChar = *wrapPtr;
	    if (isspace(saveChar))
	      *wrapPtr = '\0';
	    else { /* is punctuation */
	      saveChar = *++wrapPtr;
	      *wrapPtr = '\0';
	    }
	    (*printf_function)("%s\n", line);
	    *wrapPtr = saveChar;
	    wrappedLine = 1;
	    column = 1;
	    while (isspace(*wrapPtr)) wrapPtr++;
	    line = (wrapPtr >= endPtr) ? endPtr : wrapPtr;
	    if (*line != '\0') {
	      (*printf_function)(spaces, ""); /* print indent */
	      column += indent;
	    }
	  }
	  else /* wrap line anyway where space or punctuation delimiter
			  is not found, otherwise could get into infinite loop */
	  {
	    char saveChar = *endlinePtr;
	    *endlinePtr = '\0';
	    (*printf_function)("%s\n", line);
	    *endlinePtr = saveChar;
	    wrappedLine = 1;
	    column = 1;

	    line = (endlinePtr >= endPtr) ? endPtr : endlinePtr;
	    if (*line != '\0') {
		  (*printf_function)(spaces, ""); /* print indent */
		  column += indent;
	    }
	  }

	  remain = screen_width - column;
	  len = strlen(line);
	  endPtr = line + len;
	} /* while loop */

	/* Print whatever is left on line. */
	(*printf_function)("%s", line);

	if (wrappedLine) {
	  if (len > 0)
		column = (line[len-1] == '\n') ? 1 : indent + strlen(line);
	  else
		column = 1;
	}
	else {
	  if (len > 0)
		column = (line[len-1] == '\n') ? 1 : (start_column + len);
	  else
		column = start_column;
	}

	FREE(string);

	return column;
}
