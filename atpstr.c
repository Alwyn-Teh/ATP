/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpstr.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation	of
						the STRING parameter.

*******************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*+*******************************************************************

	Function Name:		Atp_ProcessStrParm

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom

	Description:		This function is the parameter processor
						for the STRING parameter.

	Modifications:
		Who			When					Description
	----------	----------------	-----------------------------
	Alwyn Teh	30 July 1992		Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	9 June 1993			Add Atp_AppendParmName()
	Alwyn Teh	21 January 1994		Use malloced string	always and
									free after use when in error
									condition. (Corresponding free
									in atpmem.c ReleaseStore()).

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessStrParm( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessStrParm(parseRec)
	Atp_ParserStateRec *parseRec;
#endif
{
	Atp_Result	result	= ATP_OK;
	char		*string	= NULL;
	int			length	= 0;
	char		*errmsg	= NULL;

	parseRec ->ReturnStr = NULL;

	result = Atp_SelectInputAndParseParm(parseRec,
										 (Atp_ArgParserType)Atp_ParseStr,
										 "string", &string,
										 &length, NULL, &errmsg);

	if (result == ATP_OK) {
	  if (parseRec->ValueUsedIsDefault) {
		if ((char *) parseRec->defaultPointer == NULL) {
		  /* NOTE: On the SUN, strlen(NULL) core dumps! */
		  string = NULL;
		  length = 0;
		}
		else {
		  string = Atp_Strdup((char *) parseRec->defaultPointer);
		  length = strlen(string);
		}
	  }
	  else
	    parseRec->CurrArgvIdx++;
	}

	if (result == ATP_OK)
	  result = Atp_CheckRange(&Atp_ParseRecParmDefEntry(parseRec),
							  !(parseRec->ValueUsedIsDefault),
							  string, (Atp_NumType)length, &errmsg);

	if (result == ATP_OK)
	result = Atp_InvokeVproc(&Atp_ParseRecParmDefEntry(parseRec),
							 &string,
							 !(parseRec->ValueUsedIsDefault),
							 &errmsg);

	if (result == ATP_OK)
	  Atp_StoreParm(&Atp_ParseRecParmDefEntry(parseRec),
					parseRec->ValueUsedIsDefault,
					string);

	if (result == ATP_ERROR) {
	  if ((parseRec->ReturnStr == NULL) && (errmsg != NULL)) {
		Atp_AppendParmName(parseRec, errmsg);
		parseRec->ReturnStr = errmsg;
	  }
	  if (string != NULL)
		FREE(string);
	}

	parseRec->result = result;

	if (result == ATP_OK) {
	  if (!parseRec->RptBlkTerminatorSize_Known)
	    parseRec->RptBlkTerminatorSize += sizeof(char *);
	}

	return result;
}

/*+*******************************************************************

	Function Name:		Atp_ParseStr

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		This function is the string parameter
						parser. It makes a copy of the input token.
						Optionally returns the string length.

	Modifications:
		Who			When				Description
	----------	---------------	------------------------------
	Alwyn Teh	22 July 1992	Initial Creation
	Alwyn Teh	29 October 1993 Initialize return variables
								properly to prevent core dumps
								caused by misuse such as free.

*****★*************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ParseStr
(
	char *src,
	char **strPtr,
	int  *strLenPtr,
	char *strType,
	char **ErrorMsgPtr
)
#else
Atp_Result
Atp_ParseStr(src, strPtr, strLenPtr, strType, ErrorMsgPtr)
	char *src, **strPtr;
	int  *strLenPtr;
	char *strType; /* specifies what type of string is to be parsed */
	char **ErrorMsgPtr;
#endif
{
	extern char * Atp_CurrParmName;
	int length;

	/*	Initialize	return variables to zeroes.	*/
	if (strPtr != NULL)			*strPtr			= NULL;
	if (strLenPtr	!= NULL)	*strLenPtr		= 0;
	if (ErrorMsgPtr != NULL)	*ErrorMsgPtr	= NULL;

	/*
	The string parameter is allowed to have empty string as a
	value, but not other types of parameter.
	*/
	if (((src == NULL) || (*src == '\0')) && (strType != NULL))
	{
	  if (ErrorMsgPtr != NULL)
	  {
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_EXPECTED_PARM_NOT_FOUND,
										strType, Atp_CurrParmName);
	  }
	  return ATP_ERROR;
	}

	if (strPtr == NULL)
	{
	  if (ErrorMsgPtr != NULL)
	  {
		Atp_ShowErrorLocation();
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_RETURN_VALUE_POINTER_ABSENT,
										(strType == NULL) ? "string" : strType);
	  }
	  return ATP_ERROR;
	}

	/* Make a copy of the token string. */
	length = strlen(src);
	*strPtr = (char *) MALLOC(length + 1, NULL);
	*strPtr = strcpy(*strPtr, src);

	/* Return string length */
	if (strLenPtr != NULL)
	  *strLenPtr = length;

	return ATP_OK;
}

/*+********************************************************************

	Function Name:		Atp_RetrieveStrParm

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Retrieves the string parameter.
						Also supports keyword and choice strings.

	Modifications:
		Who			When					Description
	----------	--------------	-------------------------------------
	Alwyn Teh	27 July 1992	Initial Creation
	Alwyn Teh	13 July 1993	Add CHOICE construct to search list
	Alwyn Teh	23 Dec 1993		Add boolean parameter to search list

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
char *Atp_RetrieveStrParm
(
	char *StrParmName,
	char *filename,
	int  line_number
)
#else
char *
Atp_RetrieveStrParm(StrParmName, filename, line_number)
	char *StrParmName;
	char *filename;
	int	 line_number;
#endif
{
	ParmStoreInfo			*MatchedParmInfoPtr;
	ParmStoreMemMgtNode		*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_ChoiceDescriptor	*ChoiceDescPtr;

	char *str = NULL;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore,
										StrParmName, 4, ATP_STR,
										ATP_KEYS, ATP_BOOL, ATP_BCH);

	if (MatchedParmInfoPtr != NULL)
	{
		switch (Atp_PARMCODE(MatchedParmInfoPtr->parmcode)) {
			case ATP_STR:
				str = (*(char **)MatchedParmInfoPtr->parmValue);
				break;
			case ATP_KEYS:
				str = (char *)MatchedParmInfoPtr->DataPointer;
				break;
			case ATP_BOOL:
				str = (char *)MatchedParmInfoPtr->DataPointer;
				break;
			case ATP_BCH:
				ChoiceDescPtr = (Atp_ChoiceDescriptor *)
				MatchedParmInfoPtr->parmValue;
				str = ChoiceDescPtr->CaseName;
				break;
			default:
				break;
		}
		return str;
	}
	else
	{
		/* This function may not return. */
		Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
							  "Atp_Str()", StrParmName,
							  filename, line_number);

		return NULL;
	}
}
