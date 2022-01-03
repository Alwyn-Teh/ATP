/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpkeywd.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation	of
						the KEYWORD parameter. Some procedures are
						reusable for general purposes.

*******************************************************************_*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBOG
static char * __Atp_Local_FileName__ = __FILE__;
#endif

/* Forward declarations */
Atp_Result Atp_GetKeywordIndex
				_PROTO_((Atp_KeywordType	*KeyTable,
						 Atp_NumType		KeyValue,
						 char				**KeywordStringPtr,
						 Atp_UnsNumType		*KeyIndexPtr,
						 char				**ErrorMsgPtr));

static int Atp_MatchKeyword _PROTO_((char *keyword, Atp_KeywordType *KeyTable));

/*+*******************************************************************

	Function Name:		Atp_ProcessKeywordParm

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom

	Description:		Keyword parameter processor - handles input,
						parsing, default value, keyword index, value,
						string, vproc and storage.

	Parameters:			Atp_ParserStateRec *

	Global Variables:	none

	Results:			Atp_Result

	Calls:				Atp_SelectInputAndParseParm,
						Atp_GetKeywordIndex...

	Called by:			application via parmdef macro and ATP

	Side effects:		generates and stores keyword value, index
						and string

	Notes:				N/A

	Modifications:
		Who			When					Description
	----------	-----------------	--------------------------------
	Alwyn Teh	30 July 1992		Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	9 June 1993			Add Atp_AppendParmName ()
	Alwyn Teh	21 January 1994		Use malloced keyword_string and
									free always after use.

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessKeywordParm( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessKeywordParm(parseRec)
	Atp_ParserStateRec	*parseRec;
#endif
{
	Atp_Result		result		= ATP_OK;
	Atp_NumType		keyvalue	= 0;
	Atp_UnsNumType	keyindex	= 0;
	char			*keystring	= NULL;
	Atp_KeywordType	*KeyTable	= Atp_ParseRecParmDefEntry(parseRec).KeyTabPtr;
	char			*errmsg		= NULL;

	parseRec->ReturnStr = NULL;

	result = Atp_SelectInputAndParseParm(parseRec,
					(Atp_ArgParserType)Atp_ParseKeyword, "keyword",
					KeyTable, &keyvalue, &keyindex,
					&keystring, &errmsg);

	if (result == ATP_OK) {
	  /*
		See if optional parameter requiring use of default
		value.
	   */
	  if (parseRec->ValueUsedIsDefault) {
	    /*
		  Default keyvalue is supplied by opt_keyword_def{)
	     */
	    keyvalue = (Atp_NumType)parseRec->defaultValue;

	    /*
		  Default keystring may have been obtained by a
		  previous call to Atp_GetKeywordIndex() below.
		 */
	    keystring = (char *) parseRec->defaultPointer;

	    /*
		  Default keystring, however, has to be obtained by
		  Atp_GetKeywordIndex().
		 */
	    result = Atp_GetKeywordIndex(KeyTable, keyvalue, &keystring,
	    							 &keyindex, &errmsg);
	    if (result == ATP_OK)
	    {
	      parseRec->defaultPointer = keystring;

	      /*
			Write the default keyword string into the parmdef
			entry for future use.
		   */
	      Atp_ParseRecDefaultParmPointer(parseRec) = keystring;

	      /*
			Make a copy of the default keystring to be
			stored/read/freed in the parmstore.
		   */
		  keystring = Atp_Strdup(keystring);
	    }
	  }
	  else
		parseRec->CurrArgvIdx++; /* next token */
	}

	/*
		There is no need to check the range of a keyword value,
		it does not make sense.
	 */
	if (result == ATP_OK)
	  result = Atp_InvokeVproc(	&Atp_ParseRecParmDefEntry(parseRec),
								&keyvalue,
								!(parseRec->ValueUsedIsDefault),
								&errmsg );

	if (result == ATP_OK)
	  Atp_StoreParm(&Atp_ParseRecParmDefEntry(parseRec),
								parseRec->ValueUsedIsDefault,
								keyvalue, keyindex, keystring);

	if (result == ATP_ERROR) {
	  if ((parseRec->ReturnStr == NULL) && (errmsg != NULL)) {
		Atp_AppendParmName(parseRec, errmsg);
		parseRec->ReturnStr = errmsg;
	  }
	  if (keystring != NULL)
		FREE(keystring);
	}

	parseRec->result = result;

	if (result == ATP_OK) {
	  if (!parseRec->RptBlkTerminatorSize_Known)
	    parseRec->RptBlkTerminatorSize += sizeof(Atp_NumType);
	}

	return result;
}

/*+******************************************************************

	Function Name:		Atp_ParseKeyword

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom

	Description:		Gets a keyword from the input source and
						matches it against the Atp_KeywordTab
						provided. Returns the keyword value, index
						and string.

	Modifications:
		Who			When				Description
	----------	---------------	----------------------------------
	Alwyn Teh	22 July 1992	Initial Creation
	Alwyn Teh	21 January 1994 Initialize keyword_string to 0.
								Free malloced keyword_string
								where necessary.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ParseKeyword
(
	char			*src,
	Atp_KeywordType	*KeyTable,
	Atp_NumType		*KeyValuePtr,
	int				*KeyIndexPtr,
	char			**KeywordString,
	char			**ErrorMsgPtr
)
#else
Atp_Result
Atp_ParseKeyword(src, KeyTable, KeyValuePtr,
				 KeyIndexPtr, Keywordstring, ErrorMsgPtr)
	char			*src;
	Atp_KeywordType	*KeyTable;
	Atp_NumType		*KeyValuePtr;
	int				*KeyIndexPtr;
	char			**Keywordstring;
	char			**ErrorMsgPtr;
#endif
{
	int			match_idx;
	char		*keyword_string = NULL;
	Atp_Result	result;

	if (ErrorMsgPtr != NULL) *ErrorMsgPtr = NULL;

	if (KeyTable == NULL) {
	  if (ErrorMsgPtr != NULL) {
		Atp_ShowErrorLocation();
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC, ATP_ERRCODE_KEYWD_TAB_ABSENT);
	  }
	  return ATP_ERROR;
	}

	if (KeyValuePtr == NULL) {
	  if (ErrorMsgPtr != NULL) {
		Atp_ShowErrorLocation();
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC, ATP_ERRCODE_RETURN_VALUE_POINTER_ABSENT,
										"keyword");
	  }
	  return ATP_ERROR;
	}
	else
	  *KeyValuePtr = -1; /* initial value, keyword not matched yet */

	if (KeyIndexPtr != NULL)
	  *KeyIndexPtr = -1;

	/* Parse off a keyword string */
	result = Atp_ParseStr(src, &keyword_string, NULL, "keyword",
						  ErrorMsgPtr);

	/* Fill in return value of user entered keyword string. */
	if (result == ATP_OK) {
	  if (KeywordString != NULL)
		*KeywordString = keyword_string;
	}
	else {
	  if (keyword_string != NULL)
		FREE(keyword_string);
	  if (KeywordString != NULL)
		*KeywordString = NULL;
	  return ATP_ERROR;
	}

	match_idx = Atp_MatchKeyword(keyword_string, KeyTable);

	if (match_idx < 0) {
	  if (ErrorMsgPtr != NULL) {
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_KEYWORD_NOT_RECOGNISED,
										keyword_string);
	  }
	  if (keyword_string != NULL)
		FREE(keyword_string);
	  if (KeywordString != NULL)
		*KeywordString = NULL;
	  return ATP_ERROR;
	}
	else {
	  /* Successful match */
	  *KeyValuePtr = KeyTable[match_idx].KeyValue;

	  if (KeyIndexPtr != NULL)
		*KeyIndexPtr = (Atp_UnsNumType)match_idx;

	  if ((KeywordString == NULL) && (keyword_string != NULL))
		FREE(keyword_string);

	  return ATP_OK;
	}
}

/*+******************************************************************

	Function Name:		Atp__GetKeywordIndex

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Searches the Atp_KeywordTab table to match
						the KeyValue and keyword string.
						Returns index to table where KeyValue has
						been matched.

	Modifications:
		Who			When				Description
	----------	---------------	---------------------------------
	Alwyn Teh	31 July 1992	Initial Creation
	Alwyn Teh	5 October 1993	Prefix with Atp_ and remove
								static declaration to export it.
	Alwyn Teh	21 January 1994	If keyvalue not matched, and if
								KeywordstringPtr present, return
								a NULL value in it.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_GetKeywordIndex
(
	Atp_KeywordType	*KeyTable,
	Atp_NumType		KeyValue,
	char			**KeywordStringPtr,
	Atp_UnsNumType	*KeyIndexPtr,
	char			**ErrorMsgPtr
)
#else
Atp_Result
Atp_GetKeywordIndex(KeyTable, KeyValue, KeywordStringPtr, KeyIndexPtr, ErrorMsgPtr)
	Atp_KeywordType	*KeyTable;
	Atp_NumType		KeyValue;
	char			**KeywordStringPtr;
	Atp_UnsNumType	*KeyIndexPtr;
	char			**ErrorMsgPtr;
#endif
{
	register Atp_UnsNumType	x;
	Atp_UnsNumType			rc = 0;
	Atp_Result				result = ATP_OK;
	static char				*dummy = "";

	if (KeywordStringPtr ==	NULL)
	  KeywordStringPtr = &dummy; /* temporary, for error message use */
	/* KeywordStringPtr may have address of static default keyword */

	/* To find the index, match the keyword value. */
	for (x = 0; KeyTable[x].keyword != NULL; x++) {
	   if (KeyTable[x].KeyValue == KeyValue) {
		 rc = x;
		 break;
	   }
	}

	/*
	See if it got to the end of the table, if so, it didn't
	find keyword.
	*/
	if (KeyTable[x].keyword == NULL) {
	  if (ErrorMsgPtr != NULL) {
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
							ATP_ERRCODE_DEFAULT_KEYWORD_NOT_FOUND,
							*KeywordStringPtr, KeyValue);
	  }
	result = ATP_ERROR;
	}

	if (result == ATP_OK) {
	  /* Return keyword index. */
	if (KeyIndexPtr != NULL)
	  *KeyIndexPtr = rc;

	  /* Return default keyword string. */
	  if (KeywordStringPtr != NULL) {
		*KeywordStringPtr = KeyTable[rc].keyword;
	  }
	}
	else {
	if (KeyIndexPtr != NULL)
	  *KeyIndexPtr = x; /* last index containing NULL */
	if (KeywordStringPtr != NULL)
	  *KeywordStringPtr = NULL;
	}

	return result;
}

/*+*******************************************************************

	Function Name:		Atp_MatchKeyword

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Matches given keyword against those in a
						keyword table and returns the index if
						found, otherwise returns -1.

	Modifications:
		Who			When				Description
	----------	--------------	------------------------------
	Alwyn Teh	22 July 1992	Initial Creation

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static int Atp_MatchKeyword(char *keyword, Atp_KeywordType *KeyTable )
#else
static int
Atp_MatchKeyword(keyword, KeyTable)
	char			*keyword;
	Atp_KeywordType	*KeyTable;
#endif
{
	register int	index;
	char			c;

	c = tolower(*keyword);

	for (index = 0; KeyTable[index].keyword != NULL; index++) {
	   if (tolower(KeyTable[index].keyword[0]) == c) {
		 if (Atp_Strcmp(keyword, KeyTable[index].keyword) == 0) {
		   return index;
		 }
	   }
	}

	return -1;
}
