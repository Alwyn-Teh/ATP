/* EDITION AC09 (REL002), ITD ACST.173 (95/05/28 20:35:20) -- CLOSED */

/*+****************************************************************************

	Module Name:		atphelpc.c

	Copyright:			BNR Europe Limited, 1993-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation	of
						the ATP On-Line HELP system, comprising the
						"help" command and utilities for creating
						and storing help area information, plus
						other useful options for familiarising the
						user with the application.

*****************************************************************************-*/

#include <string.h>
#if defined(__STDC__) || defined(__cplusplus)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

#include "atph.h"
#include "atpsysex.h"
#include "atpframh.h"

#ifdef DEBUG
static char * __Atp_Local_FileName__ = __FILE__;
#endif

/* Typedefs */
typedef int (*qsort_compar) _PROTO_((const void *, const void *));

/* Instructions on how to obtain help information. */
static int	HelpInstrFmtStrsDefined = 0;
static char	*HelpLang = "help -lang";
static char	*HelpMan1 = "help -man";
static char	*HelpMan2 = "man";

static char	FrontEndManPgHelpInstr[160] =
	"For help on %s, type \"%s\", \"%s\" or \"%s\", followed by \"%s\" or \
	command name." ;

static char HowToGetHelpOnCmd[80] =
	"Type \"%s%s\" or \"%s%s\" for help on command name.";

char *Atp_HelpInstructions[] = {
	"Type \"help\" for help information.",
	"Type \"help\" followed by help function or area name for further information.",
	(char *)HowToGetHelpOnCmd,
	(char *)FrontEndManPgHelpInstr,
	NULL
};

/*
 *	Used by ATP adaptor to indicate the version of the frontend used.
 *	e.g. Tcl v7.3
 */
char *VersionOfFrontEndToAtpAdaptor = NULL;

/* Version information has to be constructed at run time. */
static char *Atp_VersionInfo[] = { NULL, NULL, NULL }; /* initial values */

/* Vproc for finding command name. */
static char * FindCmdVproc _PROTO_((char **cmdname, Atp_BoolType isUserValue));

/* Pointer to command record of found command. */
CommandRecord *FoundCmdNameRecPtr = NULL;

/* Define basic HELP command parmdef. */
/*
	Modifications:
		Who			When					Description
	----------	-------------	------------------------------------
	Alwyn Teh	25 June 1993	Initial Creation
	Alwyn Teh	20 July 1994	Unify HELP system by
								means of single entry
								point via "help" command.
	Alwyn Teh	27 July 1994	Move MISC help area to
								bottom of parmdef and
								insert new help areas above it.
								This is because it1s a help
								area and not a help function,
								hence it doesn't have a '-'
								sign preceding it.
*/
char NameOfFrontEndToAtpAdaptor[80] = "Language"; /* moved in from atphelp.c */

#ifdef ATP_HELP_LANG_DEF_IS_FRONTEND_MANPG
#	define ATP_LANG_DEF (&NameOfFrontEndToAtpAdaptor[0] )
#else
#	define ATP_LANG_DEF	"*"
#endif

#define HELP_DEFAULT_CASEVALUE (ATP_IDENTIFIER_CODE * 2)
static Atp_ChoiceDescriptor def_help_choice_desc = { HELP_DEFAULT_CASEVALUE };

#define SHOWALL_SHORT	123
#define SHOWALL_LONG	456

static Atp_KeywordTab HelpShowAllOptions = {
	{"short", SHOWALL_SHORT, "Show application commands in help areas"},
	{"long",  SHOWALL_LONG,  "Show application commands with descriptions"},
	{NULL, NULL}
};

char Atp_HelpLangCaseDesc[120] =
	"List %s language and built-in commands, and\
	any user-defined procedures; or find installed manpage";

char Atp_HelpLangOptStrDesc[80] = "name of language (i.e. %s) or command name";

ATP_DCL_PARMDEF(Atp_BasicHelpParmDef)
	BEGIN_PARMS
		BEGIN_OPT_CHOICE("help areas", "Help information functions and areas",
						 &def_help_choice_desc, NULL)
			CASE(ATP_HELPCMD_OPTION_DEFAULT,
				"List help functions and areas (print this help page)",
				HELP_DEFAULT_CASEVALUE)
			BEGIN_CASE(ATP_HELPCMD_OPTION_SHOWALL,
				"Show commands in help areas (<format> is 'short' or 'long')", 0)
				 opt_keyword_def("format",
								 "display commands in short or long format",
								 SHOWALL_SHORT, HelpShowAllOptions, NULL)
			END_CASE
			BEGIN_CASE(ATP_HELPCMD_OPTION_CMDINFO,
						"Obtain brief information on application command", 0)
				str_def("name", "application command name", 1, 0, FindCmdVproc)
			END_CASE
			BEGIN_CASE(ATP_HELPCMD_OPTION_CMDMANPG,
						"Generate manpage (manual entry) for application command", 0)
				str_def("name", "application command name", 1, 0, NULL)
			END_CASE
			BEGIN_CASE(ATP_HELPCMD_OPTION_CMDPARMS,
						"Display parameter table for application command", 0)
				str_def("name", "application command name", 1, 0, FindCmdVproc)
			END_CASE
			BEGIN_CASE(ATP_HELPCMD_OPTION_LANG, Atp_HelpLangCaseDesc, 0)
				opt_str_def("name", Atp_HelpLangOptStrDesc,
							ATP_LANG_DEF, 1, 0, NULL)
			END_CASE
			BEGIN_CASE(ATP_HELPCMD_OPTION_KEYWORD,
						"Keyword pattern matching on command name and description",
						0)
				str_def("keyword", "keyword pattern", 1, 0, NULL)
			END_CASE
			CASE(ATP_HELPCMD_OPTION_VERSION, "Version information", 0)

			/* Application help areas are inserted here. (ACST.138, 27/7/94) */

			CASE(ATP_HELPCMD_OPTION_MISC, "Miscellaneous facilities", 0)
		END_OPT_CHOICE
	END_PARMS
ATP_END_DCL_PARMDEF

#define DEFAULT		0
#define SHOWALL		1
#define CMDINFO		2
#define CMDMANPG	3
#define CMDPARMS	4
#define LANGUAGE	5
#define KEYWORD		6
#define VERSION		7

#define BASIC_HELP_PD_NELEM \
		(sizeof(Atp_BasicHelpParmDef)/sizeof(Atp_ParmDefEntry))

static Atp_ParmDefEntry *Atp_HelpParmDef = NULL; /* actual one used */

Atp_ParmDefEntry *Atp_HelpCmdParmsPtr = NULL; /* global */

Atp_CmdRec *Atp_HelpCmdRecPtr = NULL;

/* Initialise position where new CASE entry may be inserted. */
#define NO_OF_TRAILING_PD_ENTRIES 4 /* changed 2->4 for MISC (ACST.138) */
#define APPL_HELP_AREA_START_IDX \
		(BASIC_HELP_PD_NELEM - NO_OF_TRAILING_PD_ENTRIES)
static int NextHelpParmDefInsertPosn = APPL_HELP_AREA_START_IDX;

static int				HelpIDcounter = 0;
#define GetNewHelpID()	(ATP_IDENTIFIER_CODE + HelpIDcounter++)

/* Initialise static parmdef entry templates. */
static Atp_ParmDefEntry Begin_Case_Tmpl	= ATP_BEGIN_CASE_TEMPLATE;
static Atp_ParmDefEntry End_Case_Tmpl	= ATP_END_CASE_TEMPLATE;

/* Help	command sub-functions. */
static void	Atp_ListHelpAreas _PROTO_((void));
static void	Atp_ShowAllCommands _PROTO_((void));
static void Atp_FindHelpOnCommand _PROTO_((void));
static void	Atp_SearchByKeyword _PROTO_((char **BuiltInCmds,
										 char **UserProcs,
										 CommandRecord **AtpApplCmds));
static void Atp_GetHelpOnArea _PROTO_((int HelpCaseIndex));
static void	Atp_GetHelpOnVersion _PROTO_((void));
static Atp_HelpInfoType * NewHelpInfoRec _PROTO_((void));

/* Length of longest command name. */
static int	cmdname_width = 0;

/* Simple local functions */
#if defined(__STDC__) || defined(__cplusplus)
static char * GetCommandName(void *ptr) { return (char *)ptr; }
#else
static char * GetCommandName(ptr) void *ptr; { return (char *)ptr; }
#endif

/*+*****************************************************************

	Function Name:		Atp_CreateHelpArea

	Copyright:			BNR Europe Limited, 1993 -	1995
						Bell-Northern Research
						Northern Telecom

	Description:		Create help area for HELP system.
						Default help areas are predefined as
						Atp_BasicHelpParmDef. New help areas are
						appended to a runtime dynamic copy of the
						parmdef. If help area already exists, no
						action is taken. When done, returns help
						ID of help area.

	Modifications:
		Who			When				Description
	----------	--------------	------------------------------
	Alwyn Teh	25 June 1993	Initial Creation
	Alwyn Teh	16 July 1993	Redo/update keyword table
								for choice construct to
								add new help area.
								Create new help area info
								record when creating NEW
								help area entry. (Built-in
								help areas do NOT need help
								area records.)
	Alwyn Teh	17 January 1994	Change strdup to Atp_Strdup
								for memory debugging
	Alwyn Teh	27 July 1994	Update to include search of
								trailing MISC help area.
	Alwyn Teh	12 August 1994	Fix bug where help id for "misc"
								becomes invalid when new help
								areas are inserted above "misc"
								so that it gets pushed down the
								parmdef.
	Alwyn Teh	25 May 1995		Use NextHelpParmDeflnsertPosn + 1
								when calling Atp_MakeChoiceKeyTab,
								otherwise will miss new areas
								being added.

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_CreateHelpArea( char *help_area_name, char *help_area_desc )
#else
int
Atp_CreateHelpArea (help_area_name, help_area_desc>
	char *help_area_name;
	char *help_area_desc;
#endif
{
#define SPAREROOM 10

	register int index;

	static int CurrHelpParmDefSize	= BASIC_HELP_PD_NELEM;
	static int HelpEndParmsIdx		= BASIC_HELP_PD_NELEM - 1;

	int		result = 0;
	int		help_id = 0;
	int		term_idx = 0;
	char	*errmsg = NULL;

	if (help_area_name == NULL || *help_area_name == '\0')
	  return 0;

	/* If realtime dynamic HELP parmdef not created, do it. */
	if (Atp_HelpParmDef == NULL) {

	  /* Get some memory... */
	  Atp_HelpParmDef = (Atp_ParmDefEntry *)
							CALLOC( CurrHelpParmDefSize += SPAREROOM,
									sizeof(Atp_ParmDefEntry),
									NULL);

	  /* Make a copy of the basic HELP parmdef. */
	  /* Also, initialize help ids for default help areas. */
	  for (index = 0; index < BASIC_HELP_PD_NELEM; index++) {
		 Atp_HelpParmDef[index] = Atp_BasicHelpParmDef[index];
		 if (Atp_HelpParmDef[index].parmcode == ATP_BCS &&
			 Atp_HelpParmDef[index].Default == 0)
		   Atp_HelpParmDef[index].Default = GetNewHelpID();
	  }

	  Atp_HelpCmdParmsPtr = Atp_HelpParmDef;
	  Atp_HelpParmDef[1].KeyTabPtr = NULL;
	}

	/* If "help" command has been created, update its parmdef. */
	if (Atp_HelpCmdRecPtr != NULL) {
	  Atp_HelpCmdRecPtr->parmDef = Atp_HelpCmdParmsPtr;
	  ((CommandRecord *)Atp_HelpCmdRecPtr)->ParmDefChecked = 0;
	  ((CommandRecord *)Atp_HelpCmdRecPtr)->NoOfPDentries = HelpEndParmsIdx + 1;
	  /*
	   *	IMPORTANT: MUST RESET MATCHINDEX, OTHERWISE "man help" WILL
	   *	GET INTO AN INFINITE LOOP CHEWING UP MEMORY!
	   *
	   *	Reset matchIndex for Atp_ConstructBracketMatcher() to re-check
	   *	later when Atp_VerifyParmDef() is called.
	   */
	  ((ParmDefEntry *)(Atp_HelpCmdRecPtr->parmDef))[0].matchIndex = 0;
	}

	/* Check if help area already exists. */
	for (index = 2;
		 index <= (HelpEndParmsIdx - NO_OF_TRAILING_PD_ENTRIES + 1);
		 index+=2)
	{
	   /* If not empty CASE, jump over any CASE contents to next CASE. */
	   while (Atp_HelpParmDef[index].parmcode != ATP_BCS) index++;
	   if (Atp_Strcmp(Atp_HelpParmDef[index].Name, help_area_name) == 0)
	   {
		   /*
		    *	Return help_id encoded as CASE value in Default field.
		    *	Initialize it if zero (e.g. unused built-in help areas)
		    */
		   if (Atp_HelpParmDef[index].Default == 0)
			 Atp_HelpParmDef[index].Default = GetNewHelpID();
		   return Atp_HelpParmDef[index].Default;
	   }
	}

	/* Reallocate memory if used up. */
	if (HelpEndParmsIdx >= (CurrHelpParmDefSize-1)) {
	  CurrHelpParmDefSize += SPAREROOM;
	  Atp_HelpParmDef = (Atp_ParmDefEntry *)
							REALLOC(Atp_HelpParmDef,
									sizeof(Atp_ParmDefEntry) *
												CurrHelpParmDefSize,
									NULL);
	  Atp_HelpCmdParmsPtr = Atp_HelpParmDef;

	  if (Atp_HelpCmdRecPtr != NULL)
		Atp_HelpCmdRecPtr->parmDef = Atp_HelpCmdParmsPtr;
	}

	/*
	 *	Shift the trailing non-CASE parameters and constructs down two
	 *	positions. This is because CASE is implemented as an empty
	 *	BEGIN_CASE and END_CASE pair.
	 *
	 *	>>>>> BUT BE CAREFUL, MUST BE IN THE RIGHT ORDER <<<<<
	 *
	 */
	term_idx = HelpEndParmsIdx - NO_OF_TRAILING_PD_ENTRIES;
	for (index = HelpEndParmsIdx; index > term_idx; index--) {
	   Atp_HelpParmDef[index+2] = Atp_HelpParmDef[index];
	}
	HelpEndParmsIdx += 2;

	/*
	 *	Checks passed, now create new help area as a CASE entry,
	 *	then assign help area name and description to it.
	 *	Give help_id the case index into the choice construct.
	 *	Create new empty help area info record for eventual use.
	 */
	help_id = GetNewHelpID();
	Atp_HelpParmDef[NextHelpParmDefInsertPosn] = Begin_Case_Tmpl;
	Atp_HelpParmDef[NextHelpParmDefInsertPosn].Default = help_id;
	Atp_HelpParmDef[NextHelpParmDefInsertPosn].Name = Atp_Strdup(help_area_name);
	Atp_HelpParmDef[NextHelpParmDefInsertPosn].DataPointer = NewHelpInfoRec();
	Atp_HelpParmDef[NextHelpParmDefInsertPosn++].Desc =
			(help_area_desc == NULL) ? "" : Atp_Strdup(help_area_desc);
	Atp_HelpParmDef[NextHelpParmDefInsertPosn++] = End_Case_Tmpl;

	/* All done, verify parmdef. */

	result = Atp_VerifyParmDef(Atp_HelpParmDef, HelpEndParmsIdx+1);

	if (result == 0) {
	  Atp_HelpCmdParmsPtr = Atp_HelpParmDef;
	  if (Atp_HelpCmdRecPtr != NULL) {
		Atp_HelpCmdRecPtr->parmDef = Atp_HelpCmdParmsPtr;
		((CommandRecord *)Atp_HelpCmdRecPtr)->ParmDefChecked = 1;
		((CommandRecord *)Atp_HelpCmdRecPtr)->NoOfPDentries = HelpEndParmsIdx + 1;
	  }

	  /* New help area added, redo keyword table if necessary. */
	  if (Atp_HelpParmDef[1].KeyTabPtr != NULL) {
	    /* Get rid of old table. */
	    FREE(Atp_HelpParmDef[1].KeyTabPtr);
	    Atp_HelpParmDef[1].KeyTabPtr = NULL;
	    /* Make a new one. */
	    Atp_HelpParmDef[1].KeyTabPtr =
	    Atp_MakeChoiceKeyTab((ParmDefEntry *)Atp_HelpParmDef,
			  	  	  	  	  	 1, NextHelpParmDefInsertPosn+1);
	  }
	}
	else {
	  Atp_HelpCmdParmsPtr = NULL;

	  Atp_ShowErrorLocation();
	  errmsg = Atp_MakeErrorMsg(ERRLOC, result, "help");

	  (void) fprintf(stderr, "%s\n", errmsg);
	  (void) fflush(stderr);

	  FREE(errmsg);

	  return 0;
	}

	return help_id;

#undef SPAREROOM
}

/*+*****************************************************************

	Function Name:		NewHelpInfoRec

	Copyright:			BNR Europe Limited,	1993
						Bell-Northern Research
						Northern Telecom

	Description:		Internal routine for issuing new dynamic
						help information record for a new help
						area.

	Modifications:
		Who			When				Description
	----------	--------------	----------------------------
	Alwyn Teh	16 July 1993	Initial	Creation

*******************************************************************-*/
static Atp_HelpInfoType * NewHelpInfoRec ()
{
	Atp_HelpInfoType *helpInfoRecPtr = NULL;

	/* Create Help Info Record */
	helpInfoRecPtr = (Atp_HelpInfoType *)
							CALLOC(1, sizeof(Atp_HelpInfoType), NULL);

	/* Initialise the record. */
	helpInfoRecPtr->cmdRecs = NULL;
	helpInfoRecPtr->HelpAreaDesc = NULL;
	helpInfoRecPtr->id = ATP_IDENTIFIER_CODE;

	return helpInfoRecPtr;
}

/*+*****************************************************************

	Function Name:			Atp_AddCmdToHelpArea

	Copyright:				BNR Europe Limited, 1993 - 1995
							Bell-Northern Research
							Northern Telecom

	Description:			Internal function for registering a
							command with a particular help area.
							This is	done normally during command
							creation time. The help area must have
							been created already as the help ID is
							required and the command record is also
							available.

	Modifications:
		Who			When				Description
	----------	---------------	----------------------------------
	Alwyn Teh	25 June 1993	Initial Creation
	Alwyn Teh	29 June 1993	Change parmDef DataPointer
								to point to Atp_HelpInfoType
	Alwyn Teh	16 July 1993	Rearrange code to allow call
								to Atp_AddHelpInfo() for info
								type ATP_HELP_AREA_SUMMARY
								before the other types.
	Alwyn Teh	24 July 1993	Keep track of longest command
								name width
	Alwyn Teh	12 August 1994	Validate help area id to make
								it work for misc help area which
								gets pushed down parmdef when
								new help areas are inserted.
	Alwyn Teh	12 April 1995	End of parmdef is ATP_EPM not
								ATP_EOP!

★★******************************************************************-*/
#if defined(_STDC___) || defined(__cplusplus)
int Atp_AddCmdToHelpArea( int help_area_id, Atp_CmdRec *CmdRecPtr )
#else
int Atp_AddCmdToHelpArea(help_area_id, CmdRecPtr)
	int help_area_id;
	Atp_CmdRec *CmdRecPtr;
#endif
{
#define DEFAULT_NO_OF_CMDRECS 5

	int				index = 0;
	register int	x;
	int				result = 0;
	int				len = 0;
	Atp_CmdRec		**ptr;

	Atp_HelpInfoType	*helpInfoRecPtr;

	/* If "help" command not initialised, don't bother. */
	if (Atp_HelpParmDef == NULL)
	  return 0;

	if (CmdRecPtr == NULL)
	  return 0;

	/* Find help area. */
	for (x = 0; Atp_HelpParmDef[x].parmcode != ATP_EPM; x++)
	   if (Atp_HelpParmDef[x].parmcode == ATP_BCS &&
		   Atp_HelpParmDef[x].Default == help_area_id)
	   {
		 index = x;
		 break;
	   }

	if (Atp_HelpParmDef[index].parmcode == ATP_BCS)
	{
	  /* If no help area, start one. */
	  if (Atp_HelpParmDef[index].DataPointer == NULL)
	  {
		/* Create Help Info Record */
		Atp_HelpParmDef[index].DataPointer =
				helpInfoRecPtr = NewHelpInfoRec();
	  }
	  else {
		/* Get Help Info Record pointer. */
		helpInfoRecPtr = (Atp_HelpInfoType *)
		Atp_HelpParmDef[index].DataPointer;

		/* If wrong pointer, return error. */
		if (helpInfoRecPtr->id != ATP_IDENTIFIER_CODE)
		  return 0;
	  }

	/*
	 *	Get pointer to command record pointer array.
	 *	If none, start a new one.
	 */
	ptr = helpInfoRecPtr->cmdRecs;
	if (ptr == NULL) {
	  /* Create array of command record pointers. */
	  ptr =
	  helpInfoRecPtr->cmdRecs =
			  	  (Atp_CmdRec **) CALLOC(DEFAULT_NO_OF_CMDRECS,
			  			  	  	  	  	 sizeof(Atp_CmdRec *), NULL);

	  helpInfoRecPtr->cmdRecs[0] = NULL; /* always null-terminated */

	  /* Reuse the Max field to store size. */
	  Atp_HelpParmDef[index].Max = DEFAULT_NO_OF_CMDRECS;
	}

	/* Add new command to help area. Expand help area if necessary. */
	for (x = 0; ptr[x] != NULL; x++) ; /* zoom to the end */
	   if (x >= (Atp_HelpParmDef[index].Max -1)) {
	     int newsize;

	     newsize = (int)(Atp_HelpParmDef[index].Max+=DEFAULT_NO_OF_CMDRECS);

	     ptr =
	     helpInfoRecPtr->cmdRecs =
			  (Atp_CmdRec **)REALLOC( ptr,
									  sizeof(Atp_CmdRec *) * newsize,
									  NULL );

	     /* Null terminate array. */
	     ptr[x] = NULL;
	     ptr[newsize-1] = NULL;
	   }

	   ptr[x] = CmdRecPtr; /* assign new command record to help area */
	   ptr[x+1] = NULL; /* null terminator */

	   result = help_area_id;
	}

	/* Find width of longest command name. */
	if (cmdname_width < (len = strlen(CmdRecPtr->cmdName)))
	  cmdname_width = len;

	/* Return decoded help_area_id if successful. */
	return result;

#undef DEFAULT_NO_OF_CMDRECS
}
/*+*****************************************************************

	Function Name:		Atp_GetHelpAreaInfoRec

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Internal function for returning	the help
						area information record for a particular
						help area attached to the parmdef entry.
						Access is via either the help area name,
						with the help area index less than 0; or
						the help area index with the name being
						NULL in order to access the choice
						keyword table as the key to the parmdef
						entry index.

	Notes:				Interface not very neat but will do since
						internal. Rewrite may be required if more
						maintenance is necessary.

	Modifications:
		Who			When					Description
	----------	-----------------		-------------------------------
	Alwyn Teh	13 July 1993		Initial Creation
	Alwyn Teh	27 July 1993		Create help info record
									if not present.
	Alwyn Teh	30 September 1993	Use internal_use field
									instead of KeyValue in
									Atp_KeywordType to get
									parmdef index.

*******************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
static Atp_HelpInfoType *
Atp_GetHelpAreaInfoRec( char *HelpAreaName, int Helplndex )
#else
static Atp_HelpInfoType *
Atp_GetHelpAreaInfoRec(HelpAreaName, Helplndex)
	char	*HelpAreaName;
	int		Helplndex;
#endif
{
	Atp_KeywordType		*HelpKeyTab = NULL;
	Atp_HelpInfoType	*HelpInfoRecPtr = NULL;
	int					EndChoiceldx = 0;
	Atp_NumType			k = 0;
	int					i = 0;
	Atp_Result			result = ATP_OK;

	/* If "help" command not initialised, don't bother. */
	if (Atp_HelpParmDef == NULL)
	  return NULL;

	/* Initialise EndChoiceldx for use below. */
	EndChoiceldx = Atp_ConstructBracketMatcher(
					(ParmDefEntry *)Atp_HelpParmDef,
					1, /* index of choice construct */
					((CommandRecord *)Atp_HelpCmdRecPtr)->NoOfPDentries);
	/*
	 *	Get keyword table generated when "help" choice construct was parsed.
	 *	If it's not available, make one.
	 */
	if (Atp_HelpParmDef[1].KeyTabPtr == NULL) {
	  HelpKeyTab =
	  Atp_HelpParmDef[1].KeyTabPtr =
			Atp_MakeChoiceKeyTab((ParmDefEntry *)Atp_HelpParmDef,
			1, EndChoiceldx);
	}
	else {
	  HelpKeyTab = Atp_HelpParmDef[1].KeyTabPtr;
	}

	/* Access help information. */
	if (HelpAreaName != NULL && Helplndex < 0) {
	  /* Access by name... (k is a mandatory dummy) */
	  result = Atp_ParseKeyword(HelpAreaName, HelpKeyTab, &k, &i,
			  	  	  	  	  	NULL, NULL);
	  if (result == ATP_OK) {
	    /* Convert i = keyword index into i = parmdef index */
		i = HelpKeyTab[i].internal_use;
		HelpInfoRecPtr = (Atp_HelpInfoType *)Atp_HelpParmDef[i].DataPointer;
		/*
		 *	Create new help info record if not present, perhaps this
		 *	help area has no commands (e.g. version info)
		 */
		if (HelpInfoRecPtr == NULL) {
		  Atp_HelpParmDef[i].DataPointer =
		    HelpInfoRecPtr = NewHelpInfoRec();
		}
	  }
	}
	else {
	  /* Access by index... */
	  i = HelpKeyTab[Helplndex].internal_use;
	  HelpInfoRecPtr = (Atp_HelpInfoType *) Atp_HelpParmDef[i].DataPointer;
	}

	return HelpInfoRecPtr;
}

/*+*******************************************************************

	Function Name:			Atp_AddHelpInfo

	Copyright:				BNR Europe Limited, 1993-1994
							Bell-Northern Research
							Northern Telecom

	Description:			Main function for adding help information
							to the HELP system. The text type must
							first be specifed before the name: for
							command name -	valid text types are
							ATP_HELP_SUMMARY (for use with the "help
							-n" option), ATP_MANPAGE_HEADER or
							ATP_MANPAGE_FOOTER (for use with the
							"man" command) and for help area name -
							text type of ATP_HELP_AREA_SUMMARY only.
							The actual textual paragraph is specified
							as a NULL terminated char *[] structure.
							Additional text paragraphs may be added
							but cannot be deleted.

	Modifications:
		Who			When					Description
	----------	---------------		-------------------------------
	Alwyn Teh	29 June 1993		Initial	Creation
	Alwyn Teh	13 July 1993		Support	help area info,
									modified interface to
									Atp_AppendCmdHelpInfo().
	Alwyn Teh	27 July 1993		Allow help areas without
									commands to have help info,
									(e.g. version info)
	Alwyn Teh	25 January 1994		Removed	text_type argument
									to Atp_AppendHelpInfo ()

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_AddHelpInfo( int text_type, char *name, char **paradesc )
#else
Atp_Result
Atp_AddHelpInfo(text_type, name, paradesc)
	int  text_type;
	char *name;
	char **paradesc;
#endif
{
	Atp_Result				result = ATP_OK;
	CommandRecord			*CmdRecPtr = NULL;
	Atp_HelpInfoType		*HelpInfoRecPtr = NULL;
	Atp_HelpSubSectionsType	*HelpSubSectionPtr = NULL;

	/* If "help" command not initialised, don't bother. */
	if (Atp_HelpParmDef == NULL)
	  return ATP_ERROR;

	if (name == NULL || paradesc == NULL)
	  return ATP_ERROR;

	/*
	 *	Help area summary information correspond to
	 *	the help area only and is stored separately.
	 */
	if (text_type == ATP_HELP_AREA_SUMMARY) {
	  HelpInfoRecPtr = Atp_GetHelpAreaInfoRec(name, -1);
	  if (HelpInfoRecPtr == NULL)
		return ATP_ERROR;
	  HelpSubSectionPtr = Atp_AppendHelpInfo(HelpInfoRecPtr->HelpAreaDesc,
			  	  	  	  	  	  	  	  	 paradesc);
	  if (HelpSubSectionPtr != NULL) {
	    HelpInfoRecPtr->HelpAreaDesc = HelpSubSectionPtr;
	    return ATP_OK;
	  }
	}
	else
	  /*
	   *	Other types of help info are for commands only.
	   */
	  if ((0 <= text_type) && (text_type < ATP_NO_OF_HELP_TYPES))
	  {
		CmdRecPtr = (CommandRecord *) Atp_FindCommand(name,1);

		if (CmdRecPtr != NULL) {
		  if (CmdRecPtr->Atp_ID_Code != ATP_IDENTIFIER_CODE)
		    return ATP_ERROR;
		  result = Atp_AppendCmdHelpInfo((Atp_CmdRec *)CmdRecPtr, text_type, paradesc);
		  return result;
		}
		return ATP_ERROR; /* name not found */
	  }

	return ATP_ERROR;
}

/*+*****************************************************************

	Function Name:		Atp_AppendHelpInfo

	Copyright:			BNR Europe Limited, 1993-1994
						Bell-Northern Research
						Northern Telecom

	Description:		Internal function for appending help
						information to a help area section
						specified by text type.

	Modifications:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	13 July 1993	Derived from
								Atp_AppendCmdHelpInfo()
	Alwyn Teh	25 January 1994	Removed text_type argument

*******************************************************************-*/
#if defined (__STDC__) || defined(__cplusplus)
Atp_HelpSubSectionsType *
Atp_AppendHelpInfo( Atp_HelpSubSectionsType *helpInfo, char **text )
#else
Atp_HelpSubSectionsType *
Atp_AppendHelpInfo(helpInfo, text)
	Atp_HelpSubSectionsType * helpInfo;
	char **text;
#endif
{
	register int slot = 0;

	/*
	 *	Initialise help info if necessary.
	 *	Obtain pointer to variable length pointer array
	 *	of char *[] help text pointers.
	 *	Write at the beginning the number of subsections or slots.
	 */
	if (helpInfo == NULL) {
	  helpInfo = (Atp_HelpSubSectionsType *)
							CALLOC(1, sizeof(Atp_HelpSubSectionsType), NULL);
	  helpInfo->SubSectionSlots = ATP_DEFAULT_NO_OF_HELP_SUBSECTIONS;
	  (helpInfo->HelpSubSections)[0] =
	  (helpInfo->HelpSubSections)[ATP_DEFAULT_NO_OF_HELP_SUBSECTIONS]
			= NULL; /* null terminate */
	}

	/*
	 *	Fast forward to the last slot to append new help text section.
	 *	Add more slots if no more room left.
	 */
	for (slot = 0; (helpInfo->HelpSubSections) [slot] != NULL; slot++) ;
	if (slot >= helpInfo->SubSectionSlots) {
	  int size;
	  helpInfo->SubSectionSlots += ATP_DEFAULT_NO_OF_HELP_SUBSECTIONS+1;
	  size = sizeof(Atp_HelpSubSectionsType) +
					(sizeof(helpInfo->HelpSubSections) *
							helpInfo->SubSectionSlots);
	  helpInfo = (Atp_HelpSubSectionsType *) REALLOC(helpInfo,size,NULL);
	}

	/* Go straight to the last slot and assign the help text. */
	(helpInfo->HelpSubSections)[slot]	= text;
	(helpInfo->HelpSubSections)[slot+1]	= NULL;

	return helpInfo;
}

/*+*****************************************************************

	Function Name:		Atp_AppendCmdHelpInfo

	Copyright:			BNR Europe Limited,	1993-1994
						Bell-Northern Research
						Northern Telecom

	Description:		Secondary support function for	appending
						help information to a command according
						to text type.

	Modifications:
		Who			When				Description
	----------	--------------	--------------------------------
	Alwyn Teh	30 June 1993	Initial	Creation
	Alwyn Teh	13 July 1993	Renamed	Atp_AppendHelpInfo()
								to Atp_AppendCmdHelpInfo(),
								put last section of code into
								Atp_AppendHelpInfo() above.
	Alwyn Teh	25 January 1994	Removed	text_type argument
								to Atp_AppendHelpInfo()

*******************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
Atp_Result Atp_AppendCmdHelpInfo
(
	Atp_CmdRec	*CmdRecPtr,
	int			text_type,
	char		**text
)
#else
Atp_Result
Atp_AppendCmdHelpInfo(CmdRecPtr, text_type, text)
	Atp_CmdRec	*CmdRecPtr;
	int			text_type;
	char		**text;
#endif
{
	Atp_HelpSectionsType *helpPtr = NULL;

	/* Reject at earliest opportunity if nothing supplied. */
	if (CmdRecPtr == NULL || text == NULL)
	  return ATP_ERROR;

	/* Ensure valid ATP command record, we don't want core dumps. */
	if (((CommandRecord *)CmdRecPtr)->Atp_ID_Code != ATP_IDENTIFIER_CODE)
	  return ATP_ERROR;

	/* Check for valid text_type, also used for array indexing. */
	if (text_type != ATP_HELP_SUMMARY &&
		text_type != ATP_MANPAGE_HEADER &&
		text_type != ATP_MANPAGE_FOOTER)
	return ATP_ERROR;

	/*
	 *	Initialise pointer to array of pointers to help text sections
	 *	under the different help types such as general help info summary,
	 *	manpage header and footer paragraphs.
	 */
	if ((helpPtr = (Atp_HelpSectionsType *) CmdRecPtr->helpInfo) == NULL) {
	  CmdRecPtr->helpInfo = helpPtr =
			  	  	  (Atp_HelpSectionsType *)
					  CALLOC(1, sizeof(Atp_HelpSectionsType), NULL);
	  /* Ensure 1st pointer set to null */
	  (helpPtr->HelpSectionsPtr)[0] = NULL;
	  /* Null terminator */
	  (helpPtr->HelpSectionsPtr)[ATP_NO_OF_HELP_TYPES] = NULL;
	}

	/*
	 *	Append new text to help information.
	 *	Update pointer to help area always.
	 */
	(helpPtr->HelpSectionsPtr)[text_type] =
			Atp_AppendHelpInfo(((helpPtr->HelpSectionsPtr)[text_type]),
								text);

	/* That's it! */
	return ATP_OK;
}

/*+*****************************************************************

	Function Name:		Atp_HelpCmdDeleteProc

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research
						Northern Telecom

	Description:		Function to free up memory malloced by
						Atp_CreateHelpArea(), NewHelpInfoRec(),
						...

	Modifications:
		Who			When					Description
	----------	-----------------	-----------------------------
	Alwyn Teh	12-21 April 1995	Initial	Creation

******************************************************************-*/
void Atp_HelpCmdDeleteProc()
{
	register int i;

	if (Atp_HelpParmDef != NULL)
	{
	  if (Atp_HelpParmDef[1].KeyTabPtr != NULL)
		FREE(Atp_HelpParmDef[1].KeyTabPtr);

	  for (i = 0; Atp_HelpParmDef[i].parmcode != ATP_EPM; i++)
	  {
		 if (Atp_HelpParmDef[i].parmcode == ATP_BCS)
		 {
		   if (i >= APPL_HELP_AREA_START_IDX)
		   {
			 /* Free help area name and desc except help area "misc". */
			 if (Atp_HelpParmDef[i+NO_OF_TRAILING_PD_ENTRIES-1].parmcode != ATP_EPM)
			 {
			   if (Atp_HelpParmDef[i].Name != NULL)
			   {
				 FREE(Atp_HelpParmDef[i].Name);
				 Atp_HelpParmDef[i].Name = NULL;
			   }
			   if (Atp_HelpParmDef[i].Desc != NULL &&
				   *(Atp_HelpParmDef[i].Desc) != '\0')
			   {
				 FREE(Atp_HelpParmDef[i].Desc);
				 Atp_HelpParmDef[i].Desc = NULL;
			   }
			 }
		   }

		   /*
			*	Free command list associated with help area; then...
			*	Free help area info record (Atp_HelpInfoType) and contents
			*	allocated by NewHelpInfoRec().
			*/
		   if (Atp_HelpParmDef[i].DataPointer != NULL)
		   {
			 Atp_HelpInfoType *infoRec;
			 infoRec = (Atp_HelpInfoType *)Atp_HelpParmDef[i].DataPointer;

			 if (infoRec->cmdRecs != NULL)
			 {
			   register int j;
			   for (j = 0; infoRec->cmdRecs[j] != NULL; j++)
				    Atp_DeleteCommandRecord(infoRec->cmdRecs[j]);
			   FREE(infoRec->cmdRecs);
			   infoRec->cmdRecs = NULL;
			 }

			 if (infoRec->HelpAreaDesc != NULL)
			 {
			   FREE(infoRec->HelpAreaDesc);
			   infoRec->HelpAreaDesc = NULL;
			 }

			 FREE(Atp_HelpParmDef[i].DataPointer);
			 Atp_HelpParmDef[i].DataPointer = NULL;
		   }
		 }
	  }

	  FREE(Atp_HelpParmDef);
	  Atp_HelpParmDef = NULL;
	  Atp_HelpCmdParmsPtr = NULL;
	}
}

/*+*****************************************************************

	Function Name:		Atp_GetHelpSubSection

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Internal function for retrieving the help
						information section of a command
						according to text type.

	Modifications:
		Who			When					Description
	----------	-------------	------------------------------------
	Alwyn Teh	30 June 1993	Initial	Creation
	Alwyn Teh	13 July 1993	Renamed	from Atp_GetHelpInfo()
								to Atp_GetHelpSubSection()

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_HelpSubSectionsType * Atp_GetHelpSubSection
(
	Atp_CmdRec	*CmdRecPtr,
	int			text_type
)
#else
Atp_HelpSubSectionsType *
Atp_GetHelpSubSection(CmdRecPtr, text_type)
	Atp_CmdRec	*CmdRecPtr;
	int			text_type;
#endif
{
	Atp_HelpSectionsType *helpPtr = NULL;

	if (CmdRecPtr == NULL)
	  return NULL;

	if (((CommandRecord *)CmdRecPtr)->Atp_ID_Code != ATP_IDENTIFIER_CODE)
	  return NULL;

	if (text_type != ATP_HELP_SUMMARY &&
		text_type != ATP_MANPAGE_HEADER &&
		text_type != ATP_MANPAGE_FOOTER)
	  return NULL;

	if (CmdRecPtr->helpInfo == NULL)
	  return NULL;

	/* Get help section */
	helpPtr = (Atp_HelpSectionsType *)(CmdRecPtr->helpInfo);

	if (helpPtr == NULL)
	  return NULL;

	return (helpPtr->HelpSectionsPtr)[text_type];
}

/*+*****************************************************************

	Function Name:		Atp_DisplayHelpInfo

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Display function responsible for printing
						help information to the internal buffer.
						Textual paragraphs are displayed chunk by
						chunk separated by newlines.

	Modifications:
		Who			When				Description
	----------	--------------	------------------------------
	Alwyn Teh	13 July 1993	From Atp_DisplayCmdHelpInfo
	Alwyn Teh	15 July 1993	Insert trailing newline
	Alwyn Teh	3 August 1994	Wrap and indent long lines.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_DisplayHelpInfo ( Atp_HelpSubSectionsType *helpPtr, int indent )
#else
void Atp_DisplayHelpInfo(helpPtr, indent)
	Atp_HelpSubSectionsType *helpPtr;
	int indent;
#endif
{
	register int	x, y;
	char			**textPtr;

	if (helpPtr != NULL) {
	  indent = (indent >= 0) ? indent : ATP_MANPG_INDENT;
	  x = y = 0;
	  for (x=0; (textPtr = (helpPtr->HelpSubSections)[x]) != NULL; x++) {
		 /* insert leading newline for subsequent paragraphs */
		 if (x > 0) Atp_AdvPrintf ("\n");
		 for (y=0; textPtr[y] != NULL; y++) {
			Atp_DisplayIndent(indent);
			Atp_PrintfWordwrap(Atp_AdvPrintf,
								-1, indent, indent,
								"%s\n", textPtr[y]);
		 }
	  }

	  /* insert trailing newline after printing all paragraphs */
	  if (y > 0) Atp_AdvPrintf("\n");
	}
}

/*+*****************************************************************

	Function Name:		Atp_DisplayCmdHelpInfo

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Function for retrieving help	information
						for a command and displaying it.

	Modifications:
		Who			When				Description
	----------	-------------	--------------------------------
	Alwyn Teh	30 June 1993	Initial Creation
	Alwyn Teh	12 July 1993	Change from Atp_DisplayHelpInfo
								to Atp_DisplayCmdHelpInfo

******************************************************************-*/
#if defined (__STDC_) || defined (__cplusplus)
void Atp_DisplayCmdHelpInfo
(
	Atp_CmdRec	*CmdRec Ptr,
	int			text_type,
	int			indent
)
#else
void Atp_DisplayCmdHelpInfo(CmdRecPtr, text_type, indent)
	Atp_CmdRec	*CmdRecPtr;
	int			text_type, indent;
#endif
{
	Atp_HelpSubSectionsType *helpPtr = NULL;

	helpPtr = Atp_GetHelpSubSection(CmdRecPtr, text_type);

	Atp_DisplayHelpInfo(helpPtr, indent);
}

/*+*****************************************************************

	Function Names:			Atp_HelpCmd()
							Atp_ListHelpAreas()
							Atp_ShowAllCommands()
							Atp_FindHelpOnCommand()
							Atp_SearchByKeyword()
							Atp_GetHelpOnVersion()
							Atp_GetHelpOnArea()
							FindCmdVproc()

	Copyright:				BNR Europe Limited, 1993, 1994
							Bell-Northern Research
							Northern Telecom

	Description:			On-Line built-in HELP command system with
							a range	of default options and
							application help areas.
	Modifications:
		Who			When						Description
	----------	-------------------	------------------------------------
	Alwyn Teh	1-25 July 1993		Initial Creation
	Alwyn Teh	27 July 1993		Search by keyword to
									look within actual command
									table instead of within
									help system.
	Alwyn Teh	27	July 1993		Ability to show version
									information
	Alwyn Teh	30	September 1993	Use internal_use field
									instead of KeyValue in
									Atp_KeywordType to get
									parmdef index in function
									Atp_GetHelpOnArea().
	Alwyn Teh	1 October 1993		Print man/? msg after -k
									and help_area output.
	Alwyn Teh	17	January 1994	Change strdup to Atp_Strdup
									for memory debugging
	Alwyn Teh	21	July 1994		Change Atp_List_CommandClasses()
									to Atp_ListHelpAreas()
	Alwyn Teh	21	July 1994		Unify HELP system by single
									point access via "help" command
	Alwyn Teh	27 July 1994		Move MISC help area to bottom
									of help parmdef
	Alwyn Teh	27 July 1994		Display help functions and
									areas separately.
	Alwyn Teh	2 August 1994		Use Atp_PrintfWordwrap () to
									wrap possibly long strings.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_HelpCmd
(
	void *clientData,
	Atp_Result (*callback)(char *),
	char **HelpPageReturnPtr
)
#else
Atp_Result Atp_HelpCmd(clientData, callback, HelpPageReturnPtr)
	void *clientData;
	Atp_Result (*callback)();
	char **HelpPageReturnPtr;
#endif
{
	Atp_Result		rc = ATP_OK;

	char			*HelpPage = NULL;
	int				Helplndex = 0;
	int				BuiltInCmdsCount = 0, BuiltInCmdNameWidth = 0;
	int				UserProcsCount = 0, UserProcsNameWidth = 0;
	int				GetHelpPageFlag = 1; /* default */

	char			**BuiltInCmds =	NULL;
	char			**UserProcs	  =	NULL;
	CommandRecord	**AtpApplCmds =	NULL;

	static int HelpInstructionsAdded = 0;

	if (!HelpInstructionsAdded) {
	  Atp_AddHelpInfo(ATP_HELP_AREA_SUMMARY,
					  ATP_HELPCMD_OPTION_MISC,
					  Atp_HelpInstructions);
	  HelpInstructionsAdded = 1;
	}

	/* Initialise help strings. */
	if (!HelpInstrFmtStrsDefined) {
	  char *bnfName = " <name>";
	  char *fmtstr = strdup(FrontEndManPgHelpInstr);
	  sprintf(FrontEndManPgHelpInstr, fmtstr,
			  NameOfFrontEndToAtpAdaptor,
			  HelpLang, HelpMan1, HelpMan2,
			  NameOfFrontEndToAtpAdaptor);
	  free(fmtstr);

	  fmtstr = strdup(HowToGetHelpOnCmd);
	  sprintf(HowToGetHelpOnCmd, fmtstr,
			  HelpMan1, bnfName, HelpMan2, bnfName);
	  free(fmtstr);

	  HelpInstrFmtStrsDefined = 1;
	}

	Atp_AdvResetBuffer();

	switch(Helplndex = (int)Atp_Index("help areas")) {
		case DEFAULT:	Atp_ListHelpAreas();
						break;
		case SHOWALL:	Atp_ShowAllCommands();
						break;
		case CMDINFO:	Atp_FindHelpOnCommand();
						break;
		case CMDMANPG:	/*
						 *	Reuse Atp_DisplayManPage() indirectly.
						 *	Glue routine in adaptor module takes care of it.
						 */
						rc = callback(ATP_HELPCMD_OPTION_CMDMANPG);
						GetHelpPageFlag = 0;
						break;
		case CMDPARMS:	HelpPage = Atp_GenerateParmDefHelpInfo
										((Atp_CmdRec *)FoundCmdNameRecPtr);
		break;
		case LANGUAGE:
		{
						if (Atp_Strcmp(Atp_Str("name"), ATP_LANG_DEF) == 0) {
						  BuiltInCmds = Atp_GetBuiltInCmdNames(
													clientData,
													&BuiltInCmdsCount,
													&BuiltInCmdNameWidth);
						  Atp_AdvPrintf("\n%s Language Commands:\n\n",
										NameOfFrontEndToAtpAdaptor);
						  Atp_DisplayCommands((void **)BuiltInCmds,
													GetCommandName,
													(qsort_compar)Atp_CompareNames,
													BuiltInCmdsCount,
													BuiltInCmdNameWidth);
						  Atp_AdvPrintf("\n");

						  UserProcs = Atp_GetUserDefinedProcNames(
													clientData,
													&UserProcsCount,
													&UserProcsNameWidth);
						  if (UserProcs != NULL)
						  {
						    Atp_AdvPrintf("Other %s Commands and ",
								  	  	  NameOfFrontEndToAtpAdaptor);
						    Atp_AdvPrintf("User-Defined Procedures :\n\n");
						    Atp_DisplayCommands((void **) UserProcs,
													GetCommandName,
													(qsort_compar)
														Atp_CompareNames,
													UserProcsCount,
													UserProcsNameWidth);
						    Atp_AdvPrintf("\n");
						  }

						  Atp_PrintfWordwrap(Atp_AdvPrintf,
											 -1, 1, 0, "%s",
											 FrontEndManPgHelpInstr);
						  Atp_AdvPrintf("\n");

						  if (BuiltInCmds != NULL) FREE(BuiltInCmds);
						  if (UserProcs != NULL) FREE(UserProcs);
						}
						else
						{
						  rc = callback(ATP_HELPCMD_OPTION_LANG);
						  GetHelpPageFlag = 0;
						}
						break;
		}
		case KEYWORD:
		{
						Atp_GetCmdTables(clientData,
										 &BuiltInCmds, NULL, NULL,
										 &UserProcs, NULL, NULL,
										 &AtpApplCmds, NULL, NULL);

						Atp_SearchByKeyword(BuiltInCmds, UserProcs, AtpApplCmds);

						if (BuiltInCmds	!= NULL) FREE(BuiltInCmds);
						if (UserProcs	!= NULL) FREE(UserProcs);
						if (AtpApplCmds	!= NULL) FREE(AtpApplCmds);

						break;
		}
		case VERSION:	Atp_GetHelpOnVersion();			break;
		default:		Atp_GetHelpOnArea(Helplndex);	break;
	}

	if (GetHelpPageFlag) {
	  if (HelpPage == NULL)
		HelpPage = Atp_AdvGets();
	  if (HelpPageReturnPtr != NULL)
		*HelpPageReturnPtr = HelpPage;
	  else
		FREE(HelpPage);
	}

	return rc;
}

static char * PrintHelpFunctionSyntax
				_PROTO_(( Atp_ParmDefEntry *pde, int idx ));
static void Atp_ListHelpAreas()
{
	int x, save, i, j, len, width, tablesize;
	char fmtstr[32], *s = NULL;
	struct Table {
		char *helpName; /* dynamic */
		char *helpDesc; /* static */
	} *table = NULL;

	if (Atp_HelpCmdRecPtr == NULL || Atp_HelpCmdParmsPtr == NULL)
	  return;

	/* Calculate size of table. */
	tablesize = ((CommandRecord *)Atp_HelpCmdRecPtr)->NoOfPDentries;
	tablesize -= (2 + 3); /* less non-CASE parmdef entries */
	tablesize = tablesize/2; /* get number of CASE entries */

	/* Make room for table entries. */
	table = (struct Table *)CALLOC(tablesize, sizeof(struct Table), NULL);

	/* Measure name width and append table entries. */
	width - len = i = j = 0;
	for (x = 2; Atp_PARMCODE(Atp_HelpCmdParmsPtr[x].parmcode) != ATP_ECH;
		 x += 2)
	{
	   while (Atp_HelpParmDef[x].parmcode != ATP_BCS) x++;
	   if (Atp_HelpCmdParmsPtr[x].Name != NULL) {
		 s = PrintHelpFunctionSyntax(Atp_HelpCmdParmsPtr, x); /* dynamic */
		 len = strlen(s);
		 width = (len > width) ? len : width;
	   }
	   table[i].helpName = s; /* to be freed */
	   table[i].helpDesc = Atp_HelpCmdParmsPtr[x].Desc;
	   i++;
	   if (*s == '-') j++;
	   s = NULL;
	}

	/*
	 *	Make format string and print table entries.
	 *	No need to sort list, should be in order of appearance/importance.
	 */
	(void) sprintf(fmtstr, "	%%-%ds	-	%%s\n", width);
	Atp_AdvPrintf("\nHelp functions:\n\n");
	for (x = 0; x < j; x++) {
	   Atp_PrintfWordwrap(Atp_AdvPrintf,
						  -1 /* unknown screen width */, 1, width+7,
						  fmtstr, table[x].helpName, table[x].helpDesc);
	  FREE(table[x].helpName);
	}
	Atp_AdvPrintf ("\nHelp areas :\n\n");
	save = x;
	for (width = 0; x < i; x++) {
	   len = strlen(table[x].helpName);
	   width = (len > width) ? len : width;
	}
	(void) sprintf(fmtstr, "	%%-%ds	-	%%s\n", width);
	for (x = save; x < i; x++) {
	   Atp_PrintfWordwrap(Atp_AdvPrintf, -1, 1, width+7,
			   	   	   	  fmtstr, table[x].helpName, table[x].helpDesc);
	   FREE(table[x].helpName);
	}

	Atp_AdvPrintf("\n%s\n", Atp_HelpInstructions[ATP_HELP_INSTR_HELPAREA]);

	if (table != NULL) FREE(table);

	return;
}

#if defined(__STDC__) || defined(__cplusplus)
static char * PrintHelpFunctionSyntax( Atp_ParmDefEntry *pde, int idx )
#else
static char * PrintHelpFunctionSyntax(pde, idx)
	Atp_ParmDefEntry *pde;
	int idx;
#endif
{
	char *tmp1, *tmp2;
	register int i, j;

	if (Atp_PARMCODE(pde[idx].parmcode) != ATP_BCS)
	  return NULL;

	Atp_AdvPrintf("%s", pde[idx].Name); /* help function/area name */

	/* Print syntax for help functions only. */
	if (pde[idx].Name != NULL && pde[idx].Name[0] == '-') {
	  Atp_ManPgLineWrap_Flag = 0;
	  Atp_PrintListInNotationFormat((ParmDefEntry *)pde, idx, 0);
	}

	tmp1 = Atp_AdvGets();
	tmp2 = Atp_Strdup(tmp1);
	FREE(tmp1);

	/*
	 *	Shrink string to save space if possible.
	 *	e.g. Change "-cmds [ <format> ]" to "-cmds [<format>]"
	 */
	if (tmp2 != NULL && tmp2[0] == '-')
	  for (i = 0; tmp2[i] != '\0'; i++) {
	     if (isspace(tmp2[i]) && ((tmp2[i-1] =='[') || (tmp2[i+1] == ']'))) {
	       for (j = i+1; tmp2[j] != '\0'; j++)
	    	  tmp2[j-1] = tmp2[j];
	       tmp2[j -1] = '\0';
	     }
	  }

	return tmp2;
}

#if defined(__STDC__) || defined(__cplusplus)
static char * ExtractCmdName( void *ptr )
#else
static char * ExtractCmdName(ptr)
	void *ptr;
#endif
{
	return ((CommandRecord *)ptr)->cmdNameOrig;
}

static void Atp_ShowAllCommands()
{
	register int i;
	Atp_HelpInfoType *helpRec;

	Atp_AdvPrintf("\nList of commands in help areas:\n\n");

	for (i = 2; Atp_PARMCODE(Atp_HelpCmdParmsPtr[i].parmcode) != ATP_ECH;
		 i += 2)
	{
		while (Atp_HelpParmDef[i].parmcode != ATP_BCS) i++;
		helpRec = (Atp_HelpInfoType *) Atp_HelpCmdParmsPtr[i].DataPointer;
		if (helpRec != NULL && helpRec->cmdRecs != NULL) {
		  Atp_PrintfWordwrap(Atp_AdvPrintf,
				  	  	  	  -1, 1, strlen(Atp_HelpCmdParmsPtr[i].Name) + 3,
							  "%s - %s:\n",
							  Atp_HelpCmdParmsPtr[i].Name,
							  Atp_HelpCmdParmsPtr[i].Desc);

		  if (Atp_Num("format") == SHOWALL_SHORT) {
			Atp_DisplayCommands((void **)helpRec->cmdRecs,
								ExtractCmdName,
								(qsort_compar)Atp_CompareCmdRecNames,
								0,
								cmdname_width);
		  }
		  else {
			Atp_DisplayCmdDescs(helpRec->cmdRecs, 0, cmdname_width);
		  }

		  Atp_AdvPrintf("\n");
		}
	}

	Atp_AdvPrintf("%s\n", Atp_HelpInstructions[ATP_HELP_INSTR_CMD]);

	return;
}

static void Atp_FindHelpOnCommand()
{
	if (FoundCmdNameRecPtr != NULL) {
	  Atp_AdvPrintf("\nHelp information for command \"%s\":\n\n",
			  	  	FoundCmdNameRecPtr->cmdNameOrig);

	  Atp_AdvPrintf("Description -\n");
	  Atp_DisplayIndent(ATP_MANPG_INDENT); /* align with synopsis below */
	  Atp_PrintfWordwrap(Atp_AdvPrintf,
						 -1, ATP_MANPG_INDENT, ATP_MANPG_INDENT,
						 "%s\n\n", FoundCmdNameRecPtr->cmdDesc);
	  Atp_DisplayCmdHelpInfo((Atp_CmdRec *)FoundCmdNameRecPtr,
							 ATP_HELP_SUMMARY, 2);

	  Atp_AdvPrintf("Synopsis -\n");
	  Atp_ResetManPgColumn();
	  Atp_DisplayManPageSynopsis(FoundCmdNameRecPtr);
	  Atp_AdvPrintf ("\n");

	  Atp_AdvPrintf(
			"Type \"help -man %s\" or \"man %s\" for further help on command.\n",
			FoundCmdNameRecPtr->cmdName,
			FoundCmdNameRecPtr->cmdName);
	}
}

#if defined(__STDC__) || defined(__cplusplus)
static void Atp_SearchByKeyword(char **BuiltInCmds,
								char **UserProcs,
								CommandRecord **AtpApplCmds )
#else
static void Atp_SearchByKeyword(BuiltInCmds, UserProcs, AtpApplCmds)
	char			**BuiltInCmds;
	char			**UserProcs;
	CommandRecord	**AtpApplCmds;
#endif
{
	register int	i, j;
	char			*keyword = Atp_Str("keyword");
	char			*s1, *s2;
	int				match, found, nothing_at_all;

	/* Prepare user-supplied keyword to match (non-case sensitive). */
	s1 = NULL;
	s2 = Atp_Strdup(keyword);
	s2 = Atp_StrToLower(s2);

	/* Initialise integers */
	found = match = 0;
	nothing_at_all = 1;

	/*
	 *	Go through built-in command table.
	 *	If matched name, keep it, if not ditch it.
	 */
	if (BuiltInCmds != NULL) {
	  for (i = j = 0; BuiltInCmds[i] != NULL; i++) {
	     /* Try to match command name. */
	     match = 0;
	     s1 = Atp_StrToLower(Atp_Strdup(BuiltInCmds[i]));/* all lower case */
	     if (strstr(s1, s2) != NULL) {
		   match = 1;
		   if (found == 0) found = 1;
		   BuiltInCmds[j++] = BuiltInCmds[i]; /* matched, keep it */
	     }
	     else {
		   BuiltInCmds[i] = NULL; /* not matched, delete from list */
	     }
	     if (s1 != NULL) {
		   FREE(s1);
		   s1 = NULL;
	     }
	  }
	  BuiltInCmds[j] = NULL; /* NULL terminate */

	  if (found) {
	    Atp_PrintfWordwrap(Atp_AdvPrintf, -1, 1, 0,
						   "\n%s commands matching keyword \"%s\":\n\n",
						   NameOfFrontEndToAtpAdaptor, s2);
	    Atp_DisplayCommands((void **)BuiltInCmds,
						    GetCommandName,
						    (qsort_compar)Atp_CompareNames,
						    0, 0);
	    nothing_at_all = 0;
	  }
	}

	/*
	 *	Go through user-defined procs command table.
	 *	If matched name, keep it, if not ditch it.
	 */
	if (UserProcs != NULL) {
	  found = 0;
	  for (i = j = 0; UserProcs[i] != NULL; i++) {
		 /* Try to match command name. */
		  match = 0;
		  s1 = Atp_StrToLower(Atp_Strdup(UserProcs[i])); /* all lower case */
		  if (strstr(s1, s2) != NULL) {
			match = 1;
			if (found == 0) found = 1;
			UserProcs[j++] = UserProcs[i]; /* matched, keep it */
		  }
		  else {
			UserProcs[i] = NULL; /* not matched, delete from list */
		  }
		  if (s1 != NULL) {
			FREE(s1);
			s1 = NULL;
		  }
	  }
	  UserProcs[j] = NULL; /* NULL terminate */

	  if (found) {
		Atp_PrintfWordwrap( Atp_AdvPrintf, -1, 1, 0,
							"\n%s application commands and %s \"%s\":\n\n",
							NameOfFrontEndToAtpAdaptor,
							"user-defined procedures matching keyword", s2);
		Atp_DisplayCommands((void **)UserProcs,
							GetCommandName,
							(qsort_compar)Atp_CompareNames,
							0, 0);
		nothing_at_all = 0;
	  }
	}

	/*
	 * Now match ATP application commands.
	 */
	if (AtpApplCmds != NULL)
	{
	  found = 0; /* i.e. indicate if found at least one matching command */
	  for (i = j = 0; AtpApplCmds[i] != NULL; i++)
	  {
		 s1 = Atp_StrToLower(Atp_Strdup(AtpApplCmds[i]->cmdName));
		 match = 0;

		 /* Try to match command name. */
		 if (strstr(s1, s2) != NULL)
		 {
		   match = 1;
		   if (s1 != NULL) {
			 FREE(s1);
			 s1 = NULL;
		   }
		 }
		 else
		 {
		   /* Reuse s1, free and null it first. */
		   if (s1 != NULL) {
			 FREE(s1);
			 s1 = NULL;
		   }

		   /* Command name not matched, try to match description. */
		   s1 = Atp_StrToLower(Atp_Strdup(AtpApplCmds[i]->cmdDesc));
		   if (strstr(s1, s2) != NULL)
		   {
			 match = 1;
		   }
		 }
		 if (s1 != NULL) {
		   FREE(s1);
		   s1 = NULL;
		 }
		 if (match)
		 {
		   if (found == 0) found = 1;
		   AtpApplCmds[j++] = AtpApplCmds[i]; /* keep matched command */
		 }
		 else
		 {
		   AtpApplCmds[i] = NULL; /* ditch unmatched command */
		 }
	  }
	  AtpApplCmds[j] = NULL; /* NULL terminate list */

	  if (found) {
		Atp_PrintfWordwrap( Atp_AdvPrintf, -1, 1, 0,
							"\nATP Commands / Descriptions %s \"%s\”:\n\n",
							"matching keyword", s2);
		Atp_DisplayCmdDescs(AtpApplCmds, 0, 0);
		nothing_at_all = 0;
	  }
	}

	if (nothing_at_all)
	{
	  Atp_AdvPrintf("\nNothing appropriate for keyword \"%s\".\n", s2);
	}
	else
	{
	  Atp_AdvPrintf("\n%s\n", Atp_HelpInstructions[ATP_HELP_INSTR_CMD]);
	}

	if (s2 != NULL) FREE(s2);

	return;
}

static void Atp_GetHelpOnVersion()
{
	/* If Atp_VersionInfo[0-1] not initialised, do it. */
	if (Atp_VersionInfo[0] == NULL && Atp_VersionInfo[1] == NULL)
	{
	  char *fmtstr, *atpname, *atpversion;
	  fmtstr = "%s version %s";
	  atpname = "ATP";
	  atpversion = ATP_VERSION;

	  if (&NameOfFrontEndToAtpAdaptor[0] != NULL &&
		  VersionOfFrontEndToAtpAdaptor != NULL)
	  {
		Atp_VersionInfo[0] =
			(char *) MALLOC(strlen(fmtstr) +
							strlen(NameOfFrontEndToAtpAdaptor) +
							strlen(VersionOfFrontEndToAtpAdaptor) + 1,
							NULL);
		(void) sprintf(	Atp_VersionInfo[0], fmtstr,
						NameOfFrontEndToAtpAdaptor,
						VersionOfFrontEndToAtpAdaptor );
	  }

	  Atp_VersionInfo[1] =
			(char *) MALLOC(strlen(fmtstr) +
							strlen(atpname) +
							strlen(atpversion) + 1,
							NULL);

	(void) sprintf(Atp_VersionInfo[1], fmtstr, atpname, atpversion);

	/*
	 *	Add version information to help area.
	 *	Application using ATP may have its own help info to add here.
	 */
	Atp_AddHeIpInfo(ATP_HELP_AREA_SUMMARY,
					ATP_HELPCMD_OPTION_VERSION,
					Atp_VersionInfo);
	}

	Atp_GetHelpOnArea(VERSION);
}

#if defined (__STDC__) || defined (__cplusplus)
static void Atp_GetHelpOnArea( int HelpCaseIndex )
#else
static void Atp_GetHelpOnArea(HelpCaseIndex)
	int HelpCaseIndex;
#endif
{
	Atp_HelpInfoType *HelpInfoRecPtr;
	Atp_KeywordType *HelpKeyTab = Atp_HelpParmDef[1].KeyTabPtr;
	int PD_idx = HelpKeyTab [HelpCaseIndex].internal_use; /* parmdef idx */
	int DescPrinted = 0;

	/* Get help area info record by index. */
	HelpInfoRecPtr = Atp_GetHelpAreaInfoRec(0, HelpCaseIndex);

	/* Display help information on help area. */
	if (HelpInfoRecPtr != NULL) {
	  Atp_AdvPrintf("\n"); /* opening newline */

	  /* Display help summary for help area if available. */
	  if (HelpInfoRecPtr->HelpAreaDesc != NULL) {
		Atp_PrintfWordwrap(	Atp_AdvPrintf,
							-1, 1, 2,
							"Help information on \"%s\" (%s):\n\n",
							HelpKeyTab[HelpCaseIndex].keyword,
							Atp_HelpParmDef[PD_idx].Desc );
		DescPrinted = 1;
		Atp_DisplayHelpInfo(HelpInfoRecPtr->HelpAreaDesc, 2);
	  }

	  /* If help area contains commands, list them. */
	  if (HelpInfoRecPtr->cmdRecs != NULL) {
		Atp_PrintfWordwrap(	Atp_AdvPrintf,
							-1, 1, 2,
							"List of commands for \"%s\"%s%s%s:\n\n",
							HelpKeyTab[HelpCaseIndex].keyword,
							(DescPrinted)? "" : " (",
							(DescPrinted)? "" : Atp_HelpParmDef[PD_idx].Desc,
							(DescPrinted)? "" : ")");
		Atp_DisplayCmdDeses(HelpInfoRecPtr->cmdRecs, 0, 0);
		Atp_AdvPrintf("\n%s\n", Atp_HelpInstructions[ATP_HELP_INSTR_CMD]);
	  }
	}
}

/* Vproc for finding command name. */
#if defined(__STDC__) || defined(__cplusplus)
static char * FindCmdVproc( char **cmdname, Atp_BoolType isUserValue )
#else
static char * FindCmdVproc(cmdname /* , isUserValue */)
	char **cmdname;
	/* Atp_BoolType isUserValue; */
#endif
{
	char			*name;
	CommandRecord	*CmdRecPtr = NULL;
	static char		*errmsg = NULL;

	/* Free errmsg if it has been used before. */
	if (errmsg != NULL) {
	  FREE(errmsg);
	  errmsg = NULL;
	}

	if (cmdname == NULL || *cmdname == NULL || **cmdname == '\0')
	  return " Command name not supplied";

	name = Atp_StrToLower(*cmdname);
	CmdRecPtr = (CommandRecord *) Atp_FindCommand(name,1);

	FoundCmdNameRecPtr = CmdRecPtr;
	if (CmdRecPtr == NULL) {
	  (void) Atp_DvsPrintf(&errmsg,
			  	  	  	   " Application command \"%s\" not found.", *cmdname);
	  return errmsg;
	}

	return NULL;
}
