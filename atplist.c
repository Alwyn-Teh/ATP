/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atplist.c

	Copyright:			BNR Europe Limited, 1992,	1993
						Bell-Northern Research
						Northern Telecom

	Description:		This module contains the	implementation	of
						the LIST construct, used to group together a
						list of parameters and constructs under one
						name.

********************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/* Forward declarations */
static Atp_Result Atp_ParseList _PROTO_((char *dummy_src,
										 Atp_ParserStateRec *InputParseRec));

/*+*******************************************************************

	Function Name:		Atp_ProcessListConstruct

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		This function processes the	LIST construct.

	Modifications:
		Who			When				Description
	----------	----------------	-------------------------
	Alwyn Teh	2 August 1992		Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	9 June 1993			Add Atp_AppendParmName()

********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
Atp_Result Atp_ProcessListConstruct( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessListConstruct(parseRec)
	Atp_ParserStateRec *parseRec;
#endif
{
	Atp_Result	result = ATP_OK;
	int			safe;

	parseRec->ReturnStr = NULL;
	safe = Atp_ParseRecCurrPDidx(parseRec);

	result = Atp_SelectInputAndParseParm(parseRec,
										 (Atp_ArgParserType)Atp_ParseList,
										 "list", parseRec);

	/*
		If optional LIST default required,
		Atp_SelectInputAndParseParm() will not call
		Atp_ParseList(). It will then extract the supplied
		LIST pointer and return it in the parseRec1s
		defaultPointer field.

		However, we do not need to extract it here because
		Atp_StoreConstructInfo() will do that itself when we
		tell it whether the default should be used or not.
	*/
	if (result == ATP_OK) {
	  Atp_StoreConstructInfo(&Atp_ParseRecParmDefEntry(parseRec),
							 (int)Atp_ParseRecParmDefEntry(parseRec).parmcode,
							 parseRec->ValueUsedIsDefault);

	  if (parseRec->ValueUsedIsDefault) {
		/* Jump to end of list block since default is used. */
		parseRec->CurrPDidx = Atp_ConstructBracketMatcher (	parseRec->ParmDefPtr,
															parseRec->CurrPDidx,
															parseRec->NoOfPDentries );
	  }
	}

	if (result == ATP_ERROR) {
	  char *errmsg = parseRec->ReturnStr;
	  if (errmsg != NULL) {
		Atp_ParseRecCurrPDidx(parseRec) = safe;
		Atp_AppendParmName(parseRec, errmsg);
		parseRec->ReturnStr = errmsg;
	  }
	}

	parseRec->result = result;

	return result;
}

/*+*********************************************************************

	Function Name:		Atp_ParseList

	Copyright:			BNR Europe Limited,	1992
						Bell-Northern Research
						Northern Telecom

	Description:		This function parses the LIST construct and
						its contents.

	Modifications:
		Who			When				Description
	----------	-----------------	------------------------
	Alwyn Teh	8 September 1992	Initial	Creation

**********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static Atp_Result Atp_ParseList
(
	char				*dummy_src,
	Atp_ParserStateRec	*InputParseRec
)
#else
static Atp_Result
Atp_ParseList(dummy_src, InputParseRec)
	char				*dummy_src;
							/* declared only to comply with interface */
	Atp_ParserStateRec	*InputParseRec;
#endif
{
	Atp_ParserStateRec	LocalCopyOfInputParseRec;
	Atp_ParserStateRec	*parseRec;
	Atp_ParmCode		end_parmcode;
	Atp_BoolType		finish = FALSE;
	Atp_Result			result = ATP_OK;
	int					EndListBlkParmDefIdx = 0;
	Atp_ParserType		parser = NULL;

	/* Initialise local parseRec */
	LocalCopyOfInputParseRec	= *InputParseRec;
	parseRec					= &LocalCopyOfInputParseRec;

	/* Initialise other variables. */
	end_parmcode = Atp_ParseRecParmDefEntry(parseRec).parmcode;
	Atp_SetParmType(end_parmcode, ATP_CONSTRUCT_TYPE_END);

	/* Find end of list block in parmdef. */
	EndListBlkParmDefIdx = Atp_ConstructBracketMatcher(
											parseRec->ParmDefPtr,
											parseRec->CurrPDidx,
											parseRec->NoOfPDentries);

	/*
		Beginning a list, register in parmstore, prepare for next
		level of parameters.
	*/
	Atp_StoreConstructInfo(	&Atp_ParseRecParmDefEntry(parseRec),
							(int)Atp_ParseRecParmDefEntry(parseRec).parmcode,
							parseRec->ValueUsedIsDefault );

	/* At beginning of list block, move on to the next entry. */
	if (Atp_PARMCODE(Atp_ParseRecParmDefEntry(parseRec).parmcode)==ATP_BLS)
	  parseRec->CurrPDidx++;

	/*
		Check to see if it's end of list already, i.e. an empty
		list construct.
	*/
	finish = (Atp_BoolType) (parseRec->CurrPDidx == EndListBlkParmDefIdx);

	/*
	 * Descend parmdef and invoke parsers.
	 */
	while (!finish) {
	  /* Initialisations */
	  parseRec->result				= ATP_OK;
	  parseRec->defaultValue		= 0;
	  parseRec->defaultPointer		= NULL;
	  parseRec->ValueUsedIsDefault	= FALSE;

	  /* Get parser to call. */
	  parser = Atp_ParseRecParmDefEntry(parseRec).parser;

	  if (parser != NULL) {
		/* Call parser. */
		result = (*parser)(parseRec);
	  }
	  else {
		char *parm_type_name = Atp_ParmTypeString(Atp_ParseRecParmDefEntry(parseRec).parmcode);
		Atp_ShowErrorLocation();
		parseRec->ReturnStr = Atp_MakeErrorMsg(ERRLOC, ATP_ERRCODE_PARSER_MISSING, parm_type_name);
		result = ATP_ERROR;
	  }

	  /* Update parseRec result. */
	  parseRec->result = result;

	  /*
		Check for error condition and move down parmdef,
		checking for end of list block.
	   */
	  finish = (Atp_BoolType) ((result != ATP_OK) || (++parseRec->CurrPDidx >= EndListBlkParmDefIdx));
	} /* while */

	/* Return to caller position of next token to pick up from. */
	InputParseRec->CurrArgvIdx	= parseRec->CurrArgvIdx;
	InputParseRec->CurrPDidx	= parseRec->CurrPDidx;
	InputParseRec->result		= parseRec->result;
	InputParseRec->ReturnStr	= parseRec->ReturnStr;

	return result;
}
