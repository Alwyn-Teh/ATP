/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+**************************************************»****************

	Module Name:		atpparse.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the top-level	processor
						and parser which drives the parsing process.
						Also provides utilities for use by parameter
						and construct parsers.

********************************************************************-*/

#include <string.h>
#include <setjmp.h>

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

/* Forward declarations */
static Atp_Result Atp_ScanAndParseParmDef _PROTO_((Atp_ParserStateRec *parseRecPtr));

/* Global variables */
char * Atp_CurrParmName = NULL;

/*+*******************************************************************

	Function Name:		Atp_ProcessParameters

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom

	Description:		This is the entry function to ATP for the
						parsing of argc/argv tokens.

	Notes:				Also provides access to parmdef table dump
						help information on command when "<command>
						?" is entered.

	Modifications:
		Who			When				Description
	----------	--------------	---------------------------------
	Alwyn Teh	29 July 1992	Initial Creation
	Alwyn Teh	23 June 1993	Remove call to Atp_PagingMode ()
	Alwyn Teh	24 Dec 1993		Change name of Atp_AllParmsInParmDefAreOptional()
								to Atp_AllParmsInParmDefAreOptionalOrNull()
	Alwyn Teh	4 January 1994	Put parmstore on nesting stack
								to enable any vprocs invoked
								while processing parameters to
								access parameter store.
	Alwyn Teh	18 January 1994 Free parmstore when error found.
								(This is a memory gobbler!)
	Alwyn Teh	28 July 1994	Ditch "<cmd> ?" for getting help
								as it's not neat and consistent.

*********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessParameters
(
	Atp_CmdRec *IncomingCmdRecPtr,
	int argc,
	char *argv[],
	char **return_string_ptr,
	void **parmstore_ptr
)
#else
Atp_Result
Atp_ProcessParameters(IncomingCmdRecPtr, argc, argv,
return_string_ptr, parmstore_ptr)
	Atp_CmdRec	*IncomingCmdRecPtr;
	int			argc;
	char		*argv[];
	char		**return_string_ptr;
	void		**parmstore_ptr;
#endif
{
	printf("atpparse.c: Atp_ProcessParameters() - 1\n");

	ParmStoreMemMgtNode *parmstore = NULL;

	/* Type conversion from external to internal format. */
	CommandRecord *CmdRecPtr = (CommandRecord *)IncomingCmdRecPtr;

	/*
		Normally, for a function using setjmp() to be portable,
		local variables should be made static. However,
		ParseStateRec must NOT be made static because
		Atp_ProcessParameters() is designed to be nestable via
		nested calls of the ATP adaptor (e.g. nested ATP
		commands). In any case, it does not matter here because
		when Atp_HyperSpace() is called, it returns to setjmp()
		from a deeply nested call within Atp_ScanAndParseParmDef(),
		and Atp_ProcessParameters() is made to return with error
		immediately.
	*/
	Atp_ParserStateRec	ParseStateRec;
	Atp_BoolType		ParmsPresent;
	Atp_Result			result;
	char				*errmsg;

	/* Initialisations */
	ParmsPresent	= FALSE;
	result			= ATP_OK;
	errmsg			= NULL;

	if (parmstore_ptr != NULL)
	  *parmstore_ptr = NULL;

	printf("atpparse.c: Atp_ProcessParameters() - 2\n");

	/*
		Caller (e.g. an adaptor) MOST supply return pointer for
		parmstore, if not, program is incorrect.
	 */
	if (parmstore_ptr == NULL)
	{
	  Atp_ShowErrorLocation();
	  errmsg = Atp_MakeErrorMsg(ERRLOC,
								ATP_ERRCODE_RETURN_VALUE_POINTER_ABSENT,
								"parameter store pointer");
	  if (return_string_ptr != NULL)
		*return_string_ptr = errmsg;

	  printf("atpparse.c: Atp_ProcessParameters() - 3\n");
	  return ATP_ERROR;
	}

	printf("atpparse.c: Atp_ProcessParameters() - 4\n");

	/*
		Parmdef should have been verified already at the command
		creation phase. This is just a double-check in case
		someone has written a new ATP adaptor and hasn't verified
		the parmdef.
	 */
	errmsg = Atp_VerifyCmdRecParmDef((Atp_CmdRec *) CmdRecPtr);
	if (errmsg != NULL)
	{
	  if (return_string_ptr != NULL)
		*return_string_ptr = errmsg;

	  printf("atpparse.c: Atp_ProcessParameters() - 5 errmsg = %s\n", errmsg);
	  return ATP_ERROR;
	}

	ParmsPresent = (argc > 1); /* i.e. argv[0] contains command name */

	/*
		Preliminary check on input tokens against parmdef for
		possible errors.

		A	B	C
		ParmDef is empty | Data present |	Error			Comment/Action
		-----------------+--------------+----------	----------------------------------
			  True		 |		True	|	True	Extra data found not wanted.
			  True		 |		False	|	False	No parms to parse, return ATP_OK.
			  False		 |		True	|	False	O.K., carry on.
			  False		 |		False	|	True	Expected data not found.

		Tests:

			IF (A == B) THEN			IF (A != B) THEN
			  C is True			OR		  C is False
			ELSE C is False				ELSE C is True
	 */
	if ((Atp_IsEmptyParmDef(CmdRecPtr)) != (ParmsPresent))
	{
	  if (!ParmsPresent)
	  {
		printf("atpparse.c: Atp_ProcessParameters() - 6\n");
		return ATP_OK;
	  }
	}
	else
	{
	  if (ParmsPresent)
	  {
		if (return_string_ptr != NULL)
		{
		  *return_string_ptr =
				  Atp_MakeErrorMsg(ERRLOC,ATP_ERRCODE_NO_PARMS_REQUIRED,argv[0]);
		}
		printf("atpparse.c: Atp_ProcessParameters() - 7\n");
		return ATP_ERROR;
	  }
	  else
	  if (!Atp_AllParmsInParmDefAreOptionalOrNull(CmdRecPtr))
	  {
		if (return_string_ptr != NULL)
		{
		  *return_string_ptr =
				  Atp_MakeErrorMsg(ERRLOC,ATP_ERRCODE_EXPECTED_PARMS_NOT_FOUND,argv[0]);
		}
		printf("atpparse.c: Atp_ProcessParameters() - 8\n");
		return ATP_ERROR;
	  }
	}

	*(ParmStoreMemMgtNode **)parmstore_ptr = parmstore =
				(ParmStoreMemMgtNode *) Atp_CreateNewParmStore();

	printf("atpparse.c: Atp_ProcessParameters() - 9\n");

	/*
		IMPORTANT: Initialise parser arguments record, clearing
		out all fields to ZERO.
	*/
	(void) memset(&ParseStateRec, 0, sizeof(Atp_ParserStateRec));

	printf("atpparse.c: Atp_ProcessParameters() - 10\n");

	/*
		Fill in required fields, Atp_ScanAndParseParmDef() will
		enter in the rest.
	*/
	ParseStateRec.argc			= argc;
	ParseStateRec.argv			= argv;
	ParseStateRec.ParmDefPtr	= CmdRecPtr->parmDef;
	ParseStateRec.NoOfPDentries	= CmdRecPtr->NoOfPDentries;
	ParseStateRec.CurrArgvIdx	= 1; /* i.e. 1st argument	*/
	ParseStateRec.TermArgvIdx	= argc;
						/* i.e. stop at NULL terminating argv pointer */

	printf("atpparse.c: Atp_ProcessParameters() - 11\n");

	if (setjmp(*Atp_JmpBufEnvPtr) == 0) {
	  Atp_PushParmStorePtrOnStack(parmstore); /* for use by vprocs */
	  result = Atp_ScanAndParseParmDef(&ParseStateRec) ;
	  Atp_PopParmStorePtrFromStack(parmstore);
	  printf("atpparse.c: Atp_ProcessParameters() - 12\n");
	}
	else {
	  /* Atp_ScanAndParseParmDef() has been aborted. */
	  if (return_string_ptr != NULL)
	    *return_string_ptr = Atp_GetHyperSpaceMsg();

	  Atp_PopParmStorePtrFromStack(parmstore);
	  Atp_FreeParmStore(parmstore);

	  printf("atpparse.c: Atp_ProcessParameters() - 13\n");
	  return ATP_ERROR;
	}

	if (result == ATP_OK)
	{
	  printf("atpparse.c: Atp_ProcessParameters() - 14\n");
	  if (ParseStateRec.CurrArgvIdx < ParseStateRec.TermArgvIdx)
	  {
		ParseStateRec.ReturnStr =
				Atp_MakeErrorMsg (ERRLOC,
								  ATP_ERRCODE_EXTRA_PARMS_NOT_WANTED,
								  ParseStateRec.argv[0] ,
								  ParseStateRec.argv[ParseStateRec.CurrArgvIdx-1],
								  ParseStateRec.CurrArgvIdx-1);
		printf("atpparse.c: Atp_ProcessParameters() - 15\n");
		result = ATP_ERROR;
	  }
	}

	if (result == ATP_ERROR)
	{
	  if (return_string_ptr != NULL)
		*return_string_ptr = ParseStateRec.ReturnStr;

	  Atp_FreeParmStore(parmstore);
	  printf("atpparse.c: Atp_ProcessParameters() - 16\n");
	}

	printf("atpparse.c: Atp_ProcessParameters() - 17\n");
	return result;

} /* Atp_ProcessParameters */

/*+*******************************************************************

	Function Name:		Atp_ScanAndParseParmDef

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Data-driven parser for parmdef and
						construct. Provides the outer control loop
						for invoking other parameter and construct
						parsers.

	Modifications:
		Who			When				Description
	----------	--------------	--------------------------------
	Alwyn Teh	29 July 1992	Initial	Creation
	Alwyn Teh	18 May 1993		Make function work for any
								construct not just BEGIN_PARMS

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static Atp_Result Atp_ScanAndParseParmDef(Atp_ParserStateRec *parseRecPtr)
#else
static Atp_Result
Atp_ScanAndParseParmDef(parseRecPtr)
	Atp_ParserStateRec	*parseRecPtr;
#endif
{
	printf("atpparse.c: Atp_ScanAndParseParmDef() - 1\n");

	int	ExitParmDefIdx;
	Atp_BoolType	finish;
	Atp_Result		result;
	Atp_ParmCode	parmcode;
	Atp_ParserType	parser = NULL;

	/* Initialisations */
	finish			= FALSE;
	result			= ATP_OK;
	ExitParmDefIdx	= Atp_ConstructBracketMatcher(parseRecPtr->ParmDefPtr,
												  parseRecPtr->CurrPDidx,
												  parseRecPtr->NoOfPDentries);

	printf("atpparse.c: Atp_ScanAndParseParmDef() - 2\n");

	/*
		If at beginning of parmdef, or beginning of a construct, move
		on to the next entry.
	 */
	parmcode = Atp_PARMCODE(Atp_ParseRecParmDefEntry(parseRecPtr).parmcode);
	if (((parseRecPtr->CurrPDidx == 0) && (parmcode == ATP_BPM)) ||
	    (isAtpBeginConstruct(parmcode)))
	  parseRecPtr->CurrPDidx++;

	printf("atpparse.c: Atp_ScanAndParseParmDef() - 3\n");

	/*
		Check to see if it's end of parmdef already, i.e. an empty
		parmdef.
	 */
	finish = (Atp_BoolType)
					(parseRecPtr->CurrPDidx == ExitParmDefIdx);

	printf("atpparse.c: Atp_ScanAndParseParmDef() - 4\n");

	/*
	* Descend parmdef and invoke parsers.
	*/
	while (!finish)
	{
		/* Initialisations */
		parseRecPtr->result				= ATP_OK;
		parseRecPtr->defaultValue		= 0;
		parseRecPtr->defaultPointer		= NULL;
		parseRecPtr->ValueUsedIsDefault	= FALSE;

		/* Get parser to call. */
		parser = Atp_ParseRecParmDefEntry(parseRecPtr).parser;

		if (parser != NULL)
		{
		  /* Call parser. */
		  printf("atpparse.c: Atp_ScanAndParseParmDef() - 5\n");
		  result = (*parser)(parseRecPtr);
		  printf("atpparse.c: Atp_ScanAndParseParmDef() - 6\n");
		}
		else {
		  char *parm_type_name = Atp_ParmTypeString(
				  	  	  	  	  	  	  Atp_ParseRecParmDefEntry(parseRecPtr).parmcode);
		  Atp_ShowErrorLocation();
		  parseRecPtr->ReturnStr =
				  	  	  Atp_MakeErrorMsg(ERRLOC,
				  	  			  	  	   ATP_ERRCODE_PARSER_MISSING,
										   parm_type_name);
		  printf("atpparse.c: Atp_ScanAndParseParmDef() - 7\n");
		  result = ATP_ERROR;
		}

		/* Update parseRecPtr result. */
		parseRecPtr->result = result;

		printf("atpparse.c: Atp_ScanAndParseParmDef() - 8\n");
		/*
			Check for error condition and move down parmdef,
			checking for end of parmdef.
		*/
		finish = (Atp_BoolType)
					((result != ATP_OK) ||
					(++parseRecPtr->CurrPDidx >= ExitParmDefIdx));
		printf("atpparse.c: Atp_ScanAndParseParmDef() - 9\n");
	} /* while */

	printf("atpparse.c: Atp_ScanAndParseParmDef() - 10\n");
	return result;

} /* Atp_ScanAndParseParmDef */

/*+*********************************************************************

	Function Name:		Atp_SelectInputAndParseParm

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Provides middle layer between parameter or
						construct processor and parser for
						implementing input selection. Contains stub
						for interactive prompting currently switched
						off.

	Modifications:
		Who			When					Description
	----------	----------------	-------------------------------
	Alwyn Teh	30 July 1992		Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	18 May 1993			Enable trailing omitted optional
									parameters to be processed
	Alwyn Teh	26 October 1993		Use global variable
									Atp_CurrParmName to indicate to
									the parser called the name of
									the parameter to be parsed,
									without extending the argument
									lists.
	Alwyn Teh	24 December 1993	Change name Atp_isConstructOfOptParms()
									to Atp_isConstructOfOptOrNullParms 0

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_SelectInputAndParseParm
(
	Atp_ParserStateRec	*parseRec,
	Atp_ArgParserType	parser,
	char				*ascii_parmtype,
	...
)
#else
Atp_Result
Atp_SelectInputAndParseParm(parseRec, parser, ascii_parmtype, va_alist)
	Atp_ParserStateRec	*parseRec;
	Atp_ArgParserType	parser;
	char				*ascii_parmtype;
	va_dcl
#endif
{
	va_list	ap;

	printf("atpparse.c: Atp_SelectInputAndParseParm() - 1\n");
	Atp_CallFrame parser_interface;

	char *parmname = Atp_CurrParmName = Atp_ParseRecParmDefEntry(parseRec).Name;

	Atp_Result result = ATP_OK;
	Atp_BoolType is_optional = FALSE;
	Atp_LargestType defaultValue = 0;

	/* for number, unsigned number, ... etc. */
	void *defaultPointer = NULL;

	/* for strings and databytes ... etc. */
	char *ascii_parm = NULL;
	char *errmsg = NULL;

	/* Extract stack frame from variable arguments list. */
#if defined(__STDC__) || defined(__cplusplus)
	va_start(ap, ascii_parmtype);
#else
	va_start(ap);
#endif

	printf("atpparse.c: Atp_SelectInputAndParseParm() - 2\n");
	Atp_CopyCallFrame(&parser_interface, ap);
	va_end(ap);

	is_optional = (Atp_BoolType)AtpParmIsOptional(
						Atp_ParseRecParmDefEntry(parseRec).parmcode);

	printf("atpparse.c: Atp_SelectInputAndParseParm() - 3\n");
	/* If no tokens left to parse ... */
	if (parseRec->CurrArgvIdx >= parseRec->TermArgvIdx)
	{
	  printf("atpparse.c: Atp_SelectInputAndParseParm() - 4\n");
	  if (Atp_InteractivePrompting)
	  {
		/*
		 * ****** INCOMPLETE - FOR FUTURE DEVELOPMENT ******
		 */
		Atp_PromptParmRec *PromptParmRecPtr = NULL;

		PromptParmRecPtr = Atp_CreatePromptParmRec(parseRec);

		do
		{
			result = (*Atp_InputParmFunc)(PromptParmRecPtr, &ascii_parm);
			if (result == ATP_OK)
			{
			  /*
					User indicates default to be used, if available.
			   */
			  if (is_optional && (ascii_parm == NULL))
			  {
				/*
					Get default from parmdef entry and put it in
					the parseRec to be returned.
				*/
				defaultValue   = Atp_ParseRecDefaultParmValue(parseRec);
				defaultPointer = Atp_ParseRecDefaultParmPointer(parseRec);

				parseRec->defaultValue   = defaultValue;
				parseRec->defaultPointer = defaultPointer;

				parseRec->ValueUsedIsDefault = TRUE;
			  }
			  else
				result = ATP_RETURN; /* user gives up */
			}
			else
			{
				result = parser (ascii_parm, ATP_FRAME_RELAY(parser_interface));

				if (result == ATP_ERROR)
				{
				  (*Atp_OutputToUserFunc)("%s\n", errmsg); /* to be revisited */
				  PromptParmRecPtr->NoOfAttempts++;
				}
			}
		} while (result == ATP_ERROR);

		Atp_FreePromptParmRec(PromptParmRecPtr);
	  }
	  else
	  {
		/*
			No tokens left and interactive input not required.

			If parameter is optional, obtain its default value,
			otherwise it is an error.

			Usually this situation is permitted only when all
			remaining parameters in the parmdef are optional
			parameters. No checking of such circumstances need
			be done because if a mandatory parameter is
			encountered eventually, it will fail the parsing
			task.
		*/
		printf("atpparse.c: Atp_SelectInputAndParseParm() - 5\n");
		if (is_optional) {
		  /*
				Get default from parmdef entry and put it in the
				parseRec to be returned.
		   */
		  defaultValue   = Atp_ParseRecDefaultParmValue(parseRec);
		  defaultPointer = Atp_ParseRecDefaultParmPointer(parseRec);

		  parseRec->defaultValue		= defaultValue;
		  parseRec->defaultPointer		= defaultPointer;
		  parseRec->ValueUsedIsDefault	= TRUE;
		}
		else
		if (Atp_isConstructOfOptOrNullParms(Atp_ParseRecParmDef(parseRec),
											Atp_ParseRecCurrPDidx(parseRec)))
		{
		  /*
				Encountered construct containing optional parameters.
				Go and process these parameters inside the construct.
		   */
		  Atp_ParserStateRec tmp_parseRec;

		  tmp_parseRec = *parseRec;

		  Atp_ScanAndParseParmDef(&tmp_parseRec);

		  parseRec->result    = result = tmp_parseRec.result;
		  parseRec->ReturnStr =	         tmp_parseRec.ReturnStr;
		}
		else {
		  errmsg = Atp_MakeErrorMsg(ERRLOC,
									ATP_ERRCODE_EXPECTED_PARM_NOT_FOUND,
									ascii_parmtype, parmname);
		  parseRec->ReturnStr = errmsg;
		  result = ATP_ERROR;
		}
	  }
	}
	else {
	  printf("atpparse.c: Atp_SelectInputAndParseParm() - 6\n");
	  ascii_parm = (parseRec->argv)[parseRec->CurrArgvIdx];
	  if ((ascii_parm != NULL) && (is_optional) && Atp_OptParmOmitted(ascii_parm))
	  {
		/*
			Get default from parmdef entry and put it in the
			parseRec to be returned.
		 */
		defaultValue   = Atp_ParseRecDefaultParmValue(parseRec);
		defaultPointer = Atp_ParseRecDefaultParmPointer(parseRec);

		parseRec->defaultValue   = defaultValue;
		parseRec->defaultPointer = defaultPointer;

		parseRec->ValueUsedIsDefault = TRUE;
		if (parseRec->CurrArgvIdx < parseRec->TermArgvIdx)
		  parseRec->CurrArgvIdx++;
	  }
	  else {
		printf("atpparse.c: Atp_SelectInputAndParseParm() - 7\n");
		result = parser(ascii_parm, ATP_FRAME_RELAY(parser_interface));
	  }
	}

	printf("atpparse.c: Atp_SelectInputAndParseParm() - 8\n");
	return result;
}
