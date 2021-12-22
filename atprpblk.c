/* EDITION ACO3 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+******************************************************************

	Module Name:		atprpblk.c

	Copyright:			BNR Europe Limited, 1992, 1993, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation of
						the REPEAT BLOCK construct.

*******************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char * __Atp_Local_FileName__ =  __FILE__;
#endif

/* Forward declarations. */
static Atp_Result	Atp_ParseRepeatBlock
							_PROTO_((char *src,
									 Atp_ParserStateRec *InputParseRec,
									 Atp_DataDescriptor *RptBlkDescPtr,
									 Atp_BoolType *UserZeroInstRptBlkIndicator,
									 int *TerminatorSizePtr));

/*+**************************»****************************************

	Function Name:		Atp_ProcessRepeatBlockConstruct

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Repeat block processor - handles input,
						default value, range and vproc checking, and
						construct storage.

	Modifications:
		Who			When					Description
	----------	----------------	-----------------------------
	Alwyn Teh	2 August 1992		Initial Creation
	Alwyn Teh	27 November 1992	Special case for optional
									repeat block default value
									being defined as NULL.
									Indicate this by making
									number of instance less than
									zero. This will be returned
									by Atp_RptBlockPtr (i.e.
									Atp_RetrieveRptBlk).
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	20 May 1993			Atp_DataDesriptor's	RptCount
									changes to count so it can be
									shared by databytes
	Alwyn Teh	9 June 1993			Add Atp_AppendParmName()

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessRepeatBlockConstruct(Atp_ParserStateRec *parseRec)
#else
Atp_Result
Atp_ProcessRepeatBlockConstruct(parseRec)
	Atp_ParserStateRec *parseRec;
#endif
{
	Atp_Result			result = ATP_OK;
	ParmDefEntry		RptBlkParmDefEntry;
	Atp_DataDescriptor	RptBlkDesc;
	Atp_DataDescriptor	*RptBlkDescPtr = &RptBlkDesc;
	int					TerminatorSize = 0;
	Atp_BoolType		isUserZeroInstRptBlk = FALSE;
	char				*errmsg	= NULL;
	int					safe;

	parseRec->ReturnStr = NULL;
	safe = Atp_ParseRecCurrPDidx(parseRec);

	RptBlkParmDefEntry = Atp_ParseRecParmDefEntry(parseRec);
	RptBlkDesc.count = 0;
	RptBlkDesc.data = NULL;

	result = Atp_SelectInputAndParseParm(parseRec,
										 (Atp_ArgParserType)Atp_ParseRepeatBlock,
										 "repeat block",
										 parseRec, RptBlkDescPtr,
										 &isUserZeroInstRptBlk, &TerminatorSize);

	if (result == ATP_OK)
	{
	  if (parseRec->ValueUsedIsDefault)
	  {
		if (parseRec->defaultPointer != NULL)
		{
		  RptBlkDescPtr = (Atp_DataDescriptor *)parseRec->defaultPointer;
		}
		else
		{
		  /*
				Specify -1 to indicate that nothing has been
				supplied as the optional default value. This is
				different from a user supplied zero instance
				mandatory repeat block.
		   */
		  RptBlkDescPtr->count = -1;
		  RptBlkDescPtr->data = NULL;
		}

		isUserZeroInstRptBlk = FALSE;
	  }
	}

	if (result == ATP_OK) {
	  result = Atp_CheckRange(&RptBlkParmDefEntry,
							  !(parseRec->ValueUsedIsDefault),
							  RptBlkDescPtr->count, &errmsg);
	  if (result == ATP_ERROR)
	    parseRec->ReturnStr = errmsg;
	}

	if (result == ATP_OK) {
	  /*
			If default is used, then only the BEGIN Repeat Block
			Atp_DataDescriptor is stored here.

			However, if the Repeat Block is supplied by the
			user, then the BEGIN Repeat Block Atp_DataDescriptor
			is stored by Atp_ParseRepeatBlock(). So, here,
			Atp_StoreConstructInfo() stores the END Repeat Block
			Atp_DataDescriptor and wraps back up a level in the
			parmstore.
	   */
	  Atp_StoreConstructInfo(&Atp_ParseRecParmDefEntry(parseRec),
							 (int)Atp_ParseRecParmDefEntry(parseRec).parmcode,
							 parseRec->ValueUsedIsDefault,
							 RptBlkDescPtr->count,
							 isUserZeroInstRptBlk,
							 TerminatorSize,
							 RptBlkDescPtr);

	  if (parseRec->ValueUsedIsDefault) {
		/* Jump to end of repeat block since default is used. */
		parseRec->CurrPDidx = Atp_ConstructBracketMatcher(parseRec->ParmDefPtr,
														  parseRec->CurrPDidx,
														  parseRec->NoOfPDentries);
	  }
	}

	/*
		Vproc can only be called when the repeat block is
		completed and stored in the parmstore.
	 */
	if (result == ATP_OK) {
	  result = Atp_InvokeVproc(&RptBlkParmDefEntry,
								RptBlkDescPtr, /* may be updated by vproc */
								!(parseRec->ValueUsedIsDefault),
								&errmsg);
	  if (result == ATP_ERROR)
		parseRec->ReturnStr = errmsg;
	}

	if (result == ATP_ERROR) {
	  if (parseRec->ReturnStr != NULL) {
		char *ErrMsg = parseRec->ReturnStr;
		Atp_ParseRecCurrPDidx(parseRec) = safe;
		Atp_AppendParmName(parseRec, ErrMsg);
		parseRec->ReturnStr = ErrMsg;
	  }
	}

	parseRec->result = result;

	if (result == ATP_OK) {
	  if (!parseRec->RptBlkTerminatorSize_Known)
		parseRec->RptBlkTerminatorSize += sizeof(Atp_DataDescriptor);
	}

	return result;
}

/*+*******************************************************************

	Function Name:		Atp_ParseRepeatBlock

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Repeat block parser - handles input tokens
						and performs syntax checking. Parses
						parameters and constructs contained with
						repeat block.

	Modifications:
		Who			When					Description
	----------	----------------	--------------------------------
	Alwyn Teh	3 August 1992		Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	20 May 1993			Atp_DataDesriptor's	RptCount
									changes to count so it can be
									shared by databytes
	Alwyn Teh	24 May 1993			Fix bug to allow zero instance
									optional repeat blocks.

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static Atp_Result Atp_ParseRepeatBlock
(
	char *src,
	Atp_ParserStateRec *InputParseRec,
	Atp_DataDescriptor *RptBlkDescPtr,
	Atp_BoolType *UserZeroInstRptBlkIndicator,
	int *TerminatorSizePtr
)
#else
static Atp_Result
Atp_ParseRepeatBlock(src,
					 InputParseRec,
					 RptBlkDescPtr,
					 UserZeroInstRptBlkIndicator,
					 TerminatorSizePtr)
	char				*src; /* pointer to beginning of repeat block source */
	Atp_ParserStateRec	*InputParseRec;
	Atp_DataDescriptor	*RptBlkDescPtr;
	Atp_BoolType		*UserZeroInstRptBlkIndicator;
	int					*TerminatorSizePtr;
#endif
{
	Atp_ParserStateRec	LocalCopyOfInputParseRec;
	Atp_ParserStateRec	*parseRec;

	Atp_ParmCode	begin_parmcode, end_parmcode;

	Atp_BoolType	finish					= FALSE;
	Atp_Result		result					= ATP_OK;
	int				StartRptBlkParmDefIdx	= 0;
	int				EndRptBlkParmDefIdx		= 0;
	Atp_ParserType	parser					= NULL;
	Atp_UnsNumType	RptBlkInstances			= 0;
	Atp_BoolType	isUserZeroInstRptBlk	= FALSE;
	char			*errmsg					= NULL;

	/* Initialise local parseRec */
	LocalCopyOfInputParseRec	= *InputParseRec;
	parseRec					= &LocalCopyOfInputParseRec;

	/* Initialise other variables. */
	begin_parmcode = end_parmcode =	Atp_ParseRecParmDefEntry(parseRec).parmcode;
	Atp_SetParmType(end_parmcode, ATP_CONSTRUCT_TYPE_END);
	StartRptBlkParmDefIdx	= parseRec->CurrPDidx;
	RptBlkDescPtr->count	= RptBlkInstances;
	RptBlkDescPtr->data		= NULL;

	/* First, see if open repeat block marker present. */
	if (Atp_ParseRptBlkMarker(src, begin_parmcode, &errmsg) == ATP_ERROR)
	{
	  InputParseRec->ReturnStr = errmsg;
	  return ATP_ERROR;
	}

	/*
		Next, see if end repeat block marker present, indicating
		zero instance repeat block.
	 */
	if ((src[0] == ATP_OPEN_REPEAT_BLOCK) &&
		(src[1] == ATP_CLOSE_REPEAT_BLOCK) &&
		(src[2] == '\0'))
	  isUserZeroInstRptBlk = TRUE;
	else {
	  /*
			If ATP_CLOSE_REPEAT_BLOCK is not in the same token, it
			could be in the next.
	   */
	  if (Atp_ParseRptBlkMarker(parseRec->argv[parseRec->CurrArgvIdx+1],
			  	  	  	  	  	end_parmcode, NULL) == ATP_OK)
	  {
		isUserZeroInstRptBlk = TRUE;
		/* Advance beyond close repeat block marker. */
		parseRec->CurrArgvIdx += 2;
		InputParseRec->CurrArgvIdx = parseRec->CurrArgvIdx;
	  }
	}

	/* Find end of repeat block in parmdef. */
	EndRptBlkParmDefIdx = Atp_ConstructBracketMatcher(parseRec->ParmDefPtr,
													  parseRec->CurrPDidx,
													  parseRec->NoOfPDentries);

	/*
		NOTE that the case of a default repeat block is handled
		elsewhere so this function does not need to worry about
		this.

		If isUserZeroInstRptBlk is TRUE, then an empty repeat
		block is written into the parmstore immediately.

		Otherwise, repeat block is about to commence, so this
		sets parmstore up for new level of storage for enclosed
		parameters.
	*/
	Atp_StoreConstructInfo(	&Atp_ParseRecParmDefEntry(parseRec),
							(int) Atp_ParseRecParmDefEntry(parseRec).parmcode,
							parseRec->ValueUsedIsDefault,
							RptBlkInstances,
							isUserZeroInstRptBlk,
							parseRec->RptBlkTerminatorSize,
							RptBlkDescPtr );

	if (isUserZeroInstRptBlk) {
	  if (UserZeroInstRptBlkIndicator != NULL)
		*UserZeroInstRptBlkIndicator = TRUE;

	  /* Done with repeat block, let's get out of it. */
	  InputParseRec->CurrPDidx = EndRptBlkParmDefIdx;

	  return ATP_OK;
	}

	/* At beginning of repeat block, move on to the next entry. */
	if (Atp_PARMCODE(begin_parmcode) == ATP_BRP) {
	  parseRec->CurrPDidx++;
	  parseRec->CurrArgvIdx++;
	}

	/* Initialize finish condition. */
	finish = (Atp_BoolType)(parseRec->CurrPDidx == EndRptBlkParmDefIdx);

	/*
	Scan repeat block parmdef and parse parameters enclosed
	within.
	*/
	while (!finish) {
		/* Initialisations */
		parseRec->result				= ATP_OK;
		parseRec->defaultValue			= 0;
		parseRec->defaultPointer		= NULL;
		parseRec->ValueUsedIsDefault	= FALSE;

		/* Get parser to call. */
		parser = Atp_ParseRecParmDefEntry(parseRec).parser;

		if (parser != NULL) {
		  /* Call parser. */
		  result = (*parser)(parseRec);
		}
		else {
		  char *parm_type_name = Atp_ParmTypeString(
				  	  	  	  	  	  Atp_ParseRecParmDefEntry(parseRec).parmcode);
		  Atp_ShowErrorLocation();
		  parseRec->ReturnStr = Atp_MakeErrorMsg(ERRLOC, ATP_ERRCODE_PARSER_MISSING,
				  	  	  	  	  	  	  	  	 parm_type_name);
		  result = ATP_ERROR;
		}

		/* Update parseRec result. */
		parseRec->result = result;

		/* Check for error condition */
		if (result == ATP_ERROR) {
		  finish = (Atp_BoolType) TRUE;
		}
		else
		{
		  /* Move down parmdef, checking for end of repeat block. */
		  if (++parseRec->CurrPDidx == EndRptBlkParmDefIdx)
		  {
			char *source = (parseRec->argv)[parseRec->CurrArgvIdx];
			end_parmcode = Atp_ParseRecParmDefEntry(parseRec).parmcode;
			/*
				NOTE:	Error message not required, address set to
						NULL; otherwise memory gobbling would
						result.
			 */
			if (Atp_ParseRptBlkMarker(source, end_parmcode, NULL) == ATP_OK)
			{
			  RptBlkInstances++; /* the last instance */
			  parseRec->CurrArgvIdx++; /* move on to next token */
			  finish = TRUE;
			}
			else
			{
			  /*
					Not end of repeat block data yet, repeat for
					another instance.
			   */
			  parseRec->CurrPDidx = StartRptBlkParmDefIdx + 1;
			  RptBlkInstances++;

			  if (!parseRec->RptBlkTerminatorSize_Known)
			  {
				parseRec->RptBlkTerminatorSize_Known = 1;
				if (TerminatorSizePtr != NULL)
				  *TerminatorSizePtr = parseRec->RptBlkTerminatorSize;
			  }
			}
		  }
		}
	} /* while */

	/* Return to caller position of next token to pick up from. */
	InputParseRec->CurrArgvIdx	= parseRec->CurrArgvIdx;
	InputParseRec->CurrPDidx	= parseRec->CurrPDidx;

	InputParseRec->result		= parseRec->result;
	InputParseRec->ReturnStr	= parseRec->ReturnStr;

	RptBlkDescPtr->count		= RptBlkInstances;
	RptBlkDescPtr->data			= NULL;

	InputParseRec->RptBlkTerminatorSize_Known	= parseRec->RptBlkTerminatorSize_Known;
	InputParseRec->RptBlkTerminatorSize			= parseRec->RptBlkTerminatorSize;

	return result;
}

/*+*******************************************************************

	Function Name:		Atp_ParseRptBlkMarker

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Parses ATP_OPEN_REPEAT_BLOCK	and
						ATP_CLOSE_REPEAT_BLOCK markers.

	Modifications:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	2 August 1992	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ParseRptBlkMarker(char *src, int parmcode, char **errmsg)
#else
Atp_Result
Atp_ParseRptBlkMarker(src, parmcode, errmsg)
	char			*src;
	Atp_ParmCode	parmcode;
	char			**errmsg;
#endif
{
	char marker;
	int errcode = 0;

	if (src == NULL)
	  return ATP_ERROR;

	parmcode = Atp_PARMCODE(parmcode);

	marker = (parmcode == ATP_BRP) ? ATP_OPEN_REPEAT_BLOCK :
			 (parmcode == ATP_ERP) ? ATP_CLOSE_REPEAT_BLOCK : ' ';

	if ((src[0] == marker) && (src[1] == '\0')) {
	  return ATP_OK;
	}
	else
	if ((parmcode == ATP_BRP) &&
		(src[0] == ATP_OPEN_REPEAT_BLOCK) &&
		(src[1] == ATP_CLOSE_REPEAT_BLOCK) &&
		(src[2] == '\0'))
	{
	  /*
			i.e. user typed in "()" to indicate a zero instance
			repeat block
	   */
	  return ATP_OK;
	}
	else
	if ((src[0] == marker) && (src[1] != '\0')) {
	  if (errmsg != NULL)
	    *errmsg = Atp_MakeErrorMsg(ERRLOC, ATP_ERRCODE_INVALID_RPTBLK_MARKER, src) ;
	  return ATP_ERROR;
	}
	else {
	  if (errmsg != NULL) {
	    errcode = (parmcode == ATP_BRP) ? ATP_ERRCODE_RPTBLK_MARKER_MISSING :
			      (parmcode == ATP_ERP) ? ATP_ERRCODE_RPTBLK_MARKER_OR_NXT_PARM_MISSING : 0;
	    *errmsg = Atp_MakeErrorMsg(ERRLOC, errcode, marker);
	  }
	  return ATP_ERROR;
	}
}

/*+*******************************************************************

	Function Name:		Atp_RetrieveRptBlk

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Returns a pointer to the beginning of a
						repeat block and how many times the block
						has been repeated.

	Modifications:
		Who			When				Description
	----------	--------------	-------------------------------
	Alwyn Teh	4 August 1992	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_ByteType *Atp_RetrieveRptBlk
(
	char *RptBlkName,
	int  *RptBlkTimes,
	char *filename,
	int  line_number
)
#else
Atp_ByteType *
Atp_RetrieveRptBlk(RptBlkName, RptBlkTimes, filename, line_number)
	char *RptBlkName;
	int	 *RptBlkTimes;
	char *filename;
	int  line_number;
#endif
{
	ParmStoreInfo		*MatchedParmInfoPtr;
	ParmStoreMemMgtNode	*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_ByteType		*RptBlk = NULL;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore, RptBlkName, 1, ATP_BRP);

	if (MatchedParmInfoPtr != NULL)
	{
	  if (RptBlkTimes != NULL)
	  {
		*RptBlkTimes = (int)(MatchedParmInfoPtr->TypeDependentInfo.RptBlockCount);
	  }

	  RptBlk = (Atp_ByteType *)((Atp_DataDescriptor *)MatchedParmInfoPtr->parmValue)->data;

	  return RptBlk;
	}
	else
	{
	  /* This function may not return. */
	  Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
							"Atp_RptBlockPtr()", RptBlkName,
							filename, line_number);
	  return NULL;
	}
}

/*+*******************************************************************

	Function Name:		Atp_RetrieveRptBlkDescriptor

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Retrieves the repeat block as a descriptor
						Atp_DataDescriptor structure.

	Modifications:
		Who			When				Description
	-----------	-------------	-----------------------------
	Alwyn Teh	20 May 1993		Initial Creation

*«********************************«*************★******************_*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_DataDescriptor Atp_RetrieveRptBlkDescriptor
(
	char *RptBlkName,
	char *filename,
	int  line_number
)
#else
Atp_DataDescriptor
Atp_RetrieveRptBlkDescriptor(RptBlkName, filename, line_number)
	char *RptBlkName;
	char *filename;
	int  line_number;
#endif
{
	ParmStoreInfo		*MatchedParmInfoPtr;
	ParmStoreMemMgtNode	*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_DataDescriptor	RptBlkDesc;

	/* Initialise */
	RptBlkDesc.count = 0;
	RptBlkDesc.data  = NULL;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore, RptBlkName, 1, ATP_BRP);

	if (MatchedParmInfoPtr != NULL)
	{
	  RptBlkDesc = *((Atp_DataDescriptor *)(MatchedParmInfoPtr->parmValue));
	  return RptBlkDesc;
	}
	else
	{
	  /* This function may not return. */
	  Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
							"Atp_RptBlockPtr()", RptBlkName,
							filename, line_number);
	  return RptBlkDesc;
	}
}
