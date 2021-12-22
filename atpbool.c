/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpbool.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation	of
						the BOOLEAN parameter.

*******************************************************************_*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/* Default boolean keywords. */
static char * Atp_BooleanWords[] = {
		"0",		"1",
		"F",		"T",
		"N",		"Y",
		"False",	"True",
		"No",		"Yes",
		"Off",		"On",
		"Bad",		"Good",
		NULL
};

/*+********************************************************************

	Function Name:		Atp_ProcessBoolParm

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		This function is the boolean parameter
						processor.

	Modifications:
		Who			When			Description
	----------	----------------	-------------------------------
	Alwyn Teh	30 July 1992		Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	9 June 1993			Add Atp_AppendParmName()
	Alwyn Teh	23 December 1993	Save boolean string
	Alwyn Teh	21 January 1994		Free malloced boolean_string
									when in error condition.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessBoolParm( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessBoolParm(parseRec)
	Atp_ParserStateRec	*parseRec;
#endif
{
		Atp_Result		result			= ATP_OK;
		Atp_BoolType	boolean			= TRUE;
		char			*boolean_string	= NULL;
		char			*errmsg			= NULL;

		parseRec->ReturnStr = NULL;

		result = Atp_SelectInputAndParseParm(parseRec,
								(Atp_ArgParserType)Atp_ParseBoolean,
								"boolean", &boolean,
								&boolean_string, &errmsg);

		if (result == ATP_OK) {
		  if (parseRec->ValueUsedIsDefault)
		    boolean = (Atp_BoolType) parseRec->defaultValue;
		  else
		    parseRec->CurrArgvIdx++; /* next token */
		}

		/*
			There is no need to check the range of a boolean value,
			it's either TRUE or FALSE.
		*/

		if (result == ATP_OK)
		  result = Atp_InvokeVproc(&Atp_ParseRecParmDefEntry(parseRec),
								   &boolean,
								   !(parseRec->ValueUsedIsDefault),
								   &errmsg);

		if (result == ATP_OK)
			Atp_StoreParm(&Atp_ParseRecParmDefEntry(parseRec),
						  parseRec->ValueUsedIsDefault, boolean, boolean_string);

		if (result == ATP_ERROR) {
			if ((parseRec->ReturnStr == NULL) && (errmsg != NULL)) {
				Atp_AppendParmName(parseRec, errmsg);
				parseRec->ReturnStr = errmsg;
			}
			if (boolean_string != NULL)
				FREE(boolean_string);
		}

		parseRec->result = result;

		if (result == ATP_OK) {
		  if (!parseRec->RptBlkTerminatorSize_Known)
		    parseRec->RptBlkTerminatorSize += sizeof(Atp_BoolType);
		}

		return result;
}

/*+*******************************************************************

	Function Name:		Atp_ParseBoolean

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Parses the boolean parameter.

	Modifications:
		Who			When			Description
	-----------	--------------	-------------------
	Alwyn Teh	22 July 1992	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ParseBoolean
(
	char			*src,
	Atp_BoolType	*boolValPtr,
	char			**boolStrPtr,
	char			**ErrorMsgPtr
)
#else
Atp_Result
Atp_ParseBoolean(src, boolValPtr, boolStrPtr, ErrorMsgPtr)
	char			*src;
	Atp_BoolType	*boolValPtr;
	char			**boolStrPtr, **ErrorMsgPtr;
#endif
{
		char		*boolean_string;
		int			match_index;
		Atp_Result	result;

		/* Initial value */
		if (ErrorMsgPtr != NULL) *ErrorMsgPtr = NULL;

		if (boolValPtr == NULL) {
		  if (ErrorMsgPtr != NULL) {
			Atp_ShowErrorLocation();
			*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
											ATP_ERRCODE_RETURN_VALUE_POINTER_ABSENT,
											"boolean");
		  }
		  return ATP_ERROR;
		}

		result = Atp_ParseStr(src, &boolean_string, NULL, "boolean string", ErrorMsgPtr);

		if (boolStrPtr != NULL)
			*boolStrPtr = boolean_string;

		if (result == ATP_ERROR) {
		  if ((boolStrPtr == NULL) && (boolean_string != NULL))
			FREE(boolean_string);
		  return ATP_ERROR;
		}

		/* Try to match a boolean value. */
		match_index = Atp_MatchStrings(boolean_string, Atp_BooleanWords);

		if (match_index == ATP_ERROR) {
		  if (ErrorMsgPtr != NULL) {
			  *ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
											  ATP_ERRCODE_UNRECOGNISED_PARM_VALUE,
											  "boolean", boolean_string);
		  }
		  if ((boolStrPtr == NULL) && (boolean_string != NULL))
			FREE(boolean_string);
		  return ATP_ERROR;
		} else {
			/* Successful match, get the value. */
			if odd(match_index)
			  *boolValPtr = TRUE;
			else *boolValPtr = FALSE;
		}
		if ((boolStrPtr == NULL) && (boolean_string != NULL))
		  FREE(boolean_string);
		return ATP_OK;
}

/*+*******************************************************************

	Function Name:		Atp_RetrieveBoolParm

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Retrieves the boolean parameter.

	Modifications:
		Who			When			Description
	----------	-------------	--------------------
	Alwyn Teh	27 July 1992	Initial Creation

********************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
Atp_BoolType Atp_RetrieveBoolParm
(
	char	*BooleanParmName,
	char	*filename,
	int		line_number
)
#else
Atp_BoolType
Atp_RetrieveBoolParm(BooleanParmName, filename, line_number)
	char	*BooleanParmName;
	char	*filename;
	int		line_number;
#endif
{
		ParmStoreInfo		*MatchedParmInfoPtr;
		ParmStoreMemMgtNode	*CurrParmStore = (ParmStoreMemMgtNode*) Atp_CurrParmStore();
		Atp_BoolType		boolVal = TRUE;

		MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore,
											BooleanParmName, 1, ATP_BOOL);

		if (MatchedParmInfoPtr != NULL) {
			boolVal = *(Atp_BoolType*) MatchedParmInfoPtr->parmValue;
			return boolVal;
		} else {
			Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
								  "Atp_Bool()", BooleanParmName,
								  filename, line_number);

			return FALSE ; /* above function may not return here */
		}
}
