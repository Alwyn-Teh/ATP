/* EDITION ACO3 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpchois.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation	of
						the CHOICE construct.

*********************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__  = __FILE__;
#endif

/* Forward declarations */
static Atp_Result Atp_ParseChoice
						_PROTO_((char *src,
								 Atp_ParserStateRec *InputParseRec,
								 Atp_ChoiceDescriptor *ChoiceDescPtr,
								 Atp_ChoiceDescriptor **ParmStoreChoiceDescPtr));

/*+*******************************************************************

	Function Name:		Atp_ProcessChoiceConstruct

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Choice construct processor - handles input,
						default, parsing, vproc	checking and
						construct storage.

	Modifications:
		Who			When					Description
	----------	------------------	---------------------------------
	Alwyn Teh	7 September 1992	Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	9 June 1993			Add Atp_AppendParmName()
	Alwyn Teh	13 July 1993		Add CaseName field to
									Atp_ChoiceDescriptor,
									change 4th argument of
									Atp_StoreConstructInfo()
									from choice index to
									ChoiceDescPtr
	Alwyn Teh	1 October 1993		Add CaseDesc field to
									Atp_ChoiceDescriptor
	Alwyn Teh	24 December 1993	Bug found, vproc not
									executed done to parseRec
									CurrPDidx updated to end
									of choice block - fix it.
	Alwyn Teh	24 December 1993	Last fix caused core dump
									to "help" command.
	Alwyn Teh	20 January 1994		Update Atp_ChoiceDescriptor
									fields with strdup copies of
									name and desc in optional
									choice.

*********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessChoiceConstruct(Atp_ParserStateRec *parseRec)
#else
Atp_Result
Atp_ProcessChoiceConstruct(parseRec)
	Atp_ParserStateRec *parseRec;
#endif
{
	Atp_Result 				result = ATP_OK;
	Atp_ChoiceDescriptor	ChoiceDesc, *NewChoiceDescPtr = NULL;
	Atp_ChoiceDescriptor	*ChoiceDescPtr = &ChoiceDesc;
	int						safe;

	parseRec->ReturnStr = NULL;
	safe = Atp_ParseRecCurrPDidx(parseRec);

	/* Initialize all choice descriptor fields. */
	ChoiceDescPtr->CaseValue = 0;
	ChoiceDescPtr->data = NULL;
	ChoiceDescPtr->CaseName = NULL;
	ChoiceDescPtr->CaseDesc = NULL;
	ChoiceDescPtr->CaseIndex = 0;

	result = Atp_SelectInputAndParseParm(parseRec,
										 (Atp_ArgParserType) Atp_ParseChoice,
										 "choice", parseRec,
										 ChoiceDescPtr, &NewChoiceDescPtr);

	if (result == ATP_OK) {
		if (parseRec->ValueUsedIsDefault)
			/* Update ChoiceDescPtr with default value. */
			if (parseRec->defaultPointer != NULL) {
				/* Update Atp_ChoiceDescriptor fields first. */
				Atp_ChoiceDescriptor *Default;
				Atp_GetOptChoiceDefaultCaseName(parseRec->ParmDefPtr, safe, &Default);

				/* defaultPointer now updated with correct choice index...etc.*/
				ChoiceDescPtr = (Atp_ChoiceDescriptor*) parseRec->defaultPointer;

				/* Use copies of name and description just like mandatory parm. */
				ChoiceDescPtr->CaseName = Atp_Strdup(Default->CaseName);
				ChoiceDescPtr->CaseDesc = Atp_Strdup(Default->CaseDesc);

			} else {
				ChoiceDescPtr->CaseValue = -1;
				ChoiceDescPtr->CaseIndex = -1;
			}
	}

	/*
		 It is not necessary to call Atp_CheckRange() because
		 choice keyword table defines range.

		 In order for vproc to be executed, the parmdef entry must
		 be for the BEGIN_CHOICE construct. So, use "safe" index.
	 */

	if (result == ATP_OK) {
		char *errmsg = NULL;

		/*
		 *	Get new location of actual parmstore Atp_ChoiceDescriptor.
		 *	Vproc may need to know and modify it.
		 */
		if (NewChoiceDescPtr != NULL)
		  ChoiceDescPtr = NewChoiceDescPtr;

		result = Atp_InvokeVproc(&((parseRec->ParmDefPtr)[safe]),
								 ChoiceDescPtr,
								 parseRec->ValueUsedIsDefault,
								 &errmsg);

		ChoiceDesc = *ChoiceDescPtr; /* reset for Atp_StoreConstructInfo */

		if (result == ATP_ERROR)
		  parseRec->ReturnStr = errmsg;
	}

	if (result == ATP_OK) {
		/*
			 Current state should only be optional default begin
			 choice or mandatory end choice.
		 */
		Atp_StoreConstructInfo(&Atp_ParseRecParmDefEntry(parseRec),
			/*
				 Parmcode could be ATP_BCH or ATP_ECH.
			 */
			(int)Atp_ParseRecParmDefEntry(parseRec).parmcode,
			/*
				 Depending on what parmcode is, the
				 following fields may or may not be used.
			 */
			parseRec->ValueUsedIsDefault,
			ChoiceDescPtr, NULL,
			/*
				 Dummy number of choice parameters for
				 ATP_ECH or optional default ATP_BCH
			 */
			1
			);

		if (parseRec->ValueUsedIsDefault) {
			/* Jump to end of choice block since default is used. */
			parseRec->CurrPDidx = Atp_ConstructBracketMatcher(
										parseRec->ParmDefPtr,
										parseRec->CurrPDidx,
										parseRec->NoOfPDentries );
		}
	}

	if (result == ATP_ERROR) {
		if (parseRec->ReturnStr != NULL) {
			char *errmsg = parseRec->ReturnStr;
			Atp_ParseRecCurrPDidx(parseRec) = safe;
			Atp_AppendParmName(parseRec, errmsg);
			parseRec->ReturnStr = errmsg;
		}
	}

	parseRec->result = result;

	if (result == ATP_OK) {
		if (!parseRec->RptBlkTerminatorSize_Known)
			parseRec->RptBlkTerminatorSize += sizeof(Atp_ChoiceDescriptor);
	}

	return result;
}

/*+********************************************************************

	Function Name:		Atp_ParseChoice

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Performs parsing of the CHOICE construct.
						When a recognised choice keyword is parsed,
						control jumps to the selected choice branch
						for further processing.

	Modifications:
		Who			When				Description
	----------	-----------------	---------------------------
	Alwyn Teh	7 September 1992	Initial Creation
	Alwyn Teh	13	July 1993		Store results in ChoiceDescPtr
	Alwyn Teh	29	September 1993	Add CaseValue to Choice Descriptor
	Alwyn Teh	24	December 1993	Add return pointer for
									actual parmstore Atp_ChoiceDescriptor
	Alwyn Teh	17	January 1994	Change strdup to Atp_Strdup
									for memory debugging

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static Atp_Result
Atp_ParseChoice
(
	char					*src,
	Atp_ParserStateRec		*InputParseRec,
	Atp_ChoiceDescriptor	*ChoiceDescPtr,
	Atp_ChoiceDescriptor	**ParmStoreChoiceDescPtr
)
#else
static Atp_Result
Atp_ParseChoice(src, InputParseRec, ChoiceDescPtr, ParmStoreChoiceDescPtr)
	char					*src;
	Atp_ParserStateRec		*InputParseRec;
	Atp_ChoiceDescriptor	*ChoiceDescPtr;
	Atp_ChoiceDescriptor	**ParmStoreChoiceDescPtr;
#endif
{
	Atp_ParserStateRec		LocalCopyOfInputParseRec;
	Atp_ParserStateRec		*parseRec;

	Atp_ParmCode	begin_parmcode, end_parmcode;

	Atp_Result		result						= ATP_OK;
	int				StartChoiceBlkParmDefIdx	= 0;
	int				EndChoiceBlkParmDefIdx		= 0;
	Atp_KeywordType	*ChoiceKeyTabPtr			= NULL;
	int				ChoiceIdx					= 0;
	Atp_NumType		ChoiceKeyValue				= 0;
	char			*ChoiceKeyWd				= NULL;
	Atp_ParserType	parser						= NULL;
	char			*errmsg						= NULL;

	/* Initialise local parseRec */
	LocalCopyOfInputParseRec	= *InputParseRec;
	parseRec					= &LocalCopyOfInputParseRec;

	/* Initialise other variables. */
	begin_parmcode = end_parmcode =
					 Atp_ParseRecParmDefEntry(parseRec).parmcode;
	Atp_SetParmType(end_parmcode, ATP_CONSTRUCT_TYPE_END);
	StartChoiceBlkParmDefIdx = parseRec->CurrPDidx;

	/* Find end of choice block in parmdef. */
	EndChoiceBlkParmDefIdx = Atp_ConstructBracketMatcher(
									parseRec->ParmDefPtr,
									parseRec->CurrPDidx,
									parseRec->NoOfPDentries);
	/*
		 If choice keyword table hasn't been created, make one and
		 attach to parmdef entry.
	 */
	ChoiceKeyTabPtr = Atp_MakeChoiceKeyTab(	parseRec->ParmDefPtr,
											StartChoiceBlkParmDefIdx,
											EndChoiceBlkParmDefIdx);

	/*
		 Now parse the choice keyword whilst looking up the choice
		 keyword table.
	 */
	result = Atp_ParseKeyword(src, ChoiceKeyTabPtr, &ChoiceKeyValue,
							  &ChoiceIdx, &ChoiceKeyWd, &errmsg);

	/*
	 If parsing of choice keyword was not successful, return
	 error.
	 */
	if (result != ATP_OK) {
		InputParseRec->result			= result;
		InputParseRec->ReturnStr		= errmsg;

		if (ChoiceDescPtr != NULL) {
			ChoiceDescPtr->CaseIndex	= 0;
			ChoiceDescPtr->data			= NULL;
		}

		if (ChoiceKeyWd != NULL)
			FREE(ChoiceKeyWd);

		return result;
	}

	/* Choice keyword parsed OK, register results in ChoiceDescPtr. */
	if (ChoiceDescPtr != NULL) {
		ChoiceDescPtr->CaseValue	= ChoiceKeyValue;
		ChoiceDescPtr->data			= NULL;
		ChoiceDescPtr->CaseName		= ChoiceKeyWd;
		ChoiceDescPtr->CaseDesc		= Atp_Strdup(
				parseRec->ParmDefPtr[ChoiceKeyTabPtr[ChoiceIdx].internal_use].Desc);
		ChoiceDescPtr->CaseIndex	= ChoiceIdx;
	}

	/* Forward to next token after choice selector keyword. */
	parseRec->CurrArgvIdx++;

	/*
		 Choice keyword selection OK, register in parmstore.
		 prepare for next level of parameters.
	 */
	Atp_StoreConstructInfo(&Atp_ParseRecParmDefEntry(parseRec),
			(int) Atp_ParseRecParmDefEntry(parseRec).parmcode,
			parseRec->ValueUsedIsDefault, ChoiceDescPtr, ParmStoreChoiceDescPtr,
			(EndChoiceBlkParmDefIdx - StartChoiceBlkParmDefIdx + 1));

	/*
	 	 At beginning of choice block, go to selected parmdef entry.
	 */
	if (Atp_PARMCODE(begin_parmcode) == ATP_BCH) {
		/* Get the parmdef idx of the selected case of the choice construct */
		parseRec->CurrPDidx = ChoiceKeyTabPtr[ChoiceIdx].internal_use;
	}

	/* Initialisations */
	parseRec->result				= ATP_OK;
	parseRec->defaultValue			= 0;
	parseRec->defaultPointer		= NULL;
	parseRec->ValueUsedIsDefault	= FALSE;

	/* Get parser to call for selected choice. */
	parser = Atp_ParseRecParmDefEntry(parseRec).parser;

	if (parser != NULL) {
		/* Call parser. (Note that CurrArgvIdx will be updated by parser.) */
		result = (*parser)(parseRec);
	} else {
		char *parm_type_name = Atp_ParmTypeString(
									Atp_ParseRecParmDefEntry(parseRec).parmcode);
		Atp_ShowErrorLocation();
		parseRec->ReturnStr = Atp_MakeErrorMsg(ERRLOC,
									ATP_ERRCODE_PARSER_MISSING, parm_type_name);
		result = ATP_ERROR;
	}

	/* Update parseRec result. */
	parseRec->result = result;
	if (result == ATP_OK) {
		/*
			 Parsing of selected choice succeeded, go to end of
			 choice block.
		 */
		parseRec->CurrPDidx = EndChoiceBlkParmDefIdx;
	}

	/* Return to caller position of next token to pick up from. */
	InputParseRec->CurrArgvIdx	= parseRec->CurrArgvIdx;
	InputParseRec->CurrPDidx	= parseRec->CurrPDidx;
	InputParseRec->result		= parseRec->result;
	InputParseRec->ReturnStr	= parseRec->ReturnStr;

	return result;
}

/*+********************************************************************

	Function Name:		Atp_MakeChoiceKeyTab

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Makes a Atp_KeywordTab table for the
						CHOICE parameter to match	the	choice
						keyword against. The KeyValues are the
						CASE values for the CASE and BEGIN_CASE
						constructs, and equal zero otherwise.

	Modifications:
		Who			When				Description
	----------	----------------	----------------------------
	Alwyn Teh	7 September 1992	Initial Creation
	Alwyn Teh	27 November 1992	Include NULL parameter
									type as valid CHOICE
									selector.
	Alwyn Teh	12 July 1993		Export function.
	Alwyn Teh	29 September 1993	Don't return parmdef
									index as keyvalue, use
									new CASE keyvalue instead.
									Put parmdef index in
									new Atp_KeywordType field
									"internal_use".
	Alwyn Teh	20 January 1994		Always realloc ChoiceKeyTabPtr
									to conserve space.

*********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_KeywordType * Atp_MakeChoiceKeyTab
(
	ParmDefEntry	*PDptr,
	int				PDidx,
	int				EndChoiceldx
)
#else
Atp_KeywordType *
Atp_MakeChoiceKeyTab(PDptr, PDidx, EndChoiceldx)
	ParmDefEntry	*PDptr;
	int				PDidx, EndChoiceldx;
#endif
{
	int				TabIdx, pd_idx;
	int				ChoiceKeyTabNelem;
	Atp_KeywordType	*ChoiceKeyTabPtr = NULL;

	/*
		 If choice keyword table has been created in a previous
		 call, then use it.
	 */
	if (PDptr[PDidx].KeyTabPtr != NULL) {
		return PDptr[PDidx].KeyTabPtr;
	}

	/*
		 Calculate maximum size of keyword table from number of
		 enclosed parmdef entries within choice block.
	 */
	ChoiceKeyTabNelem = EndChoiceldx - PDidx;
	ChoiceKeyTabPtr = (Atp_KeywordType *)
							CALLOC( ChoiceKeyTabNelem,
									sizeof(Atp_KeywordType), NULL );

		for (TabIdx = 0, pd_idx = PDidx + 1;
			(pd_idx < EndChoiceldx); TabIdx++,
			 pd_idx++)
		{
			if (isAtpBeginConstruct(PDptr[pd_idx].parmcode))
			{
				ChoiceKeyTabPtr[TabIdx].keyword		= PDptr[pd_idx].Name;
				ChoiceKeyTabPtr[TabIdx].KeyValue	= (Atp_NumType)PDptr[pd_idx].Default;
				ChoiceKeyTabPtr[TabIdx].KeywordDescription = PDptr[pd_idx].Desc;
				ChoiceKeyTabPtr[TabIdx].internal_use = pd_idx;
				pd_idx = PDptr[pd_idx].matchIndex;
			}
			else
			if ( isAtpRegularParm(PDptr[pd_idx].parmcode) ||
				 isAtpNull(PDptr[pd_idx].parmcode) )
			{
				ChoiceKeyTabPtr[TabIdx].keyword		= PDptr[pd_idx].Name;
				ChoiceKeyTabPtr[TabIdx].KeyValue	= (Atp_NumType)PDptr[pd_idx].Default;
				ChoiceKeyTabPtr[TabIdx].KeywordDescription = PDptr[pd_idx].Desc;
				ChoiceKeyTabPtr[TabIdx].internal_use = pd_idx;
			}
	}

	/* Terminate choice key table properly. */
	ChoiceKeyTabPtr[TabIdx].keyword		= (char *)NULL;
	ChoiceKeyTabPtr[TabIdx].KeyValue	= -1;

	/* Conserve space - don't be greedy. */
	ChoiceKeyTabNelem = TabIdx + 1;
	ChoiceKeyTabPtr = (Atp_KeywordType *) REALLOC(ChoiceKeyTabPtr,
												  ChoiceKeyTabNelem *
												  sizeof(Atp_KeywordType),
												  NULL);

	/*
		 Write the choice key table pointer into the CHOICE
		 parmdef entry.
	 */
	if (PDptr[PDidx].KeyTabPtr == NULL) {
		PDptr[PDidx].KeyTabPtr = ChoiceKeyTabPtr;
	}

	return ChoiceKeyTabPtr;
}

/*+********************************************************************

	Function Name: 		Atp_GetOptChoiceDefaultCaseName

	Copyright:			BNR Europe Limited, 1993-1994
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Looks in the parmdef entry default value of
						Atp_ChoiceDescriptor and returns the name of
						the case branch name.

	Called by:			Atp_DisplayParmEntry() in atphelp.c

	Side effects:		Checks to ensure that the case name,
						description, and parmdef index are correct.
						If not already initialized, fills them in.

	Modifications:
		Who 		When					Description
	-----------	-----------------	---------------------------------
	Alwyn Teh	30 September 1993	Initial Creation
	Alwyn Teh	17 January 1994		Change strdup to Atp_Strdup
									for memory debugging
	Alwyn Teh	20 January 1994		Return NULL when keyvalue
									of default not matched in
									choice keytable.
	Alwyn Teh	20 January 1994		No need to strdup name and
									desc, just use static values.
	Alwyn Teh	20 January 1994		Return updated default if
									required.

*********************************************************************-*/
static Atp_ChoiceDescriptor NullChoiceDesc = {0};
#if defined(__STDC__) || defined(__cplusplus)
char * Atp_GetOptChoiceDefaultCaseName
(
	ParmDefEntry			*ParmDefPtr,
	int						ParmDefIndex,
	Atp_ChoiceDescriptor	**DefaultPtr
)
#else
char * Atp_GetOptChoiceDefaultCaseName(ParmDefPtr, ParmDefIndex, DefaultPtr)
	ParmDefEntry			*ParmDefPtr;
	int						ParmDefIndex;
	Atp_ChoiceDescriptor	**DefaultPtr;
#endif
{
	ParmDefEntry *ParmDefEntry_Ptr = &ParmDefPtr[ParmDefIndex];
	Atp_ChoiceDescriptor *choice_desc;
	int idx;

	if (DefaultPtr != NULL)
	  *DefaultPtr = &NullChoiceDesc;

	if (Atp_PARMCODE(ParmDefEntry_Ptr->parmcode) == ATP_BCH &&
		AtpParmIsOptional(ParmDefEntry_Ptr->parmcode) &&
		ParmDefEntry_Ptr->DataPointer != NULL) {

	  choice_desc = (Atp_ChoiceDescriptor *)(ParmDefEntry_Ptr->DataPointer);

	  if (ParmDefEntry_Ptr->KeyTabPtr == NULL) {
		ParmDefEntry_Ptr->KeyTabPtr =
		Atp_MakeChoiceKeyTab(ParmDefPtr, ParmDefIndex,
		Atp_ConstructBracketMatcher(ParmDefPtr,ParmDefIndex, 0));
	  }

	  for (idx = 0; ParmDefEntry_Ptr->KeyTabPtr[idx].keyword != NULL; idx++) {
		 if (ParmDefEntry_Ptr->KeyTabPtr[idx].KeyValue == choice_desc->CaseValue) {
			choice_desc->CaseName = ParmDefEntry_Ptr->KeyTabPtr[idx].keyword;
			choice_desc->CaseDesc = ParmDefEntry_Ptr->KeyTabPtr[idx].KeywordDescription;
			choice_desc->CaseIndex = idx;
			if (DefaultPtr != NULL)
			  *DefaultPtr = choice_desc;
			return choice_desc->CaseName;
			}
		}
		return NULL; /* no matching keyvalue error condition */
	}
	else
	  return NULL;

}
