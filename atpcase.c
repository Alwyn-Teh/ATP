/* EDITION ABO3 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+***************************»***************************************

	Module Name:		atpcase.c

	Copyright:			BNR Europe Limited, 1993, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation of
						the CASE construct. It must be used within
						the CHOICE construct to contain a selection
						branch.

*******************************************************************_*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*+*********★*********************************************************

	Function Name:		Atp_ProcessCaseConstruct

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom / NORTEL

	Description:		This function is called	when a successful
						CHOICE selection is made and a jump to a
						CASE branch is made. It then traverses the
						CASE contents and invokes the parsers.

	Modifications:
		Who			When			Description
	-----------	-------------	------------------------
	Alwyn Teh	7 June 1993		Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessCaseConstruct( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_Proc ess CaseCons truet(parseRec)
	Atp_ParserStateRec	*parseRec;
#endif
{
		Atp_BoolType	finish					= FALSE;
		Atp_Result		result					= ATP_OK;
		int				EndCaseBlkParmDefIdx	= 0;
		Atp_ParserType	parser					= NULL;

		/* Find end of case block in parmdef. */
		EndCaseBlkParmDefIdx = Atp_ConstructBracketMatcher(
										parseRec->ParmDefPtr,
										parseRec->CurrPDidx,
										parseRec->NoOfPDentries);

		/* At beginning of case block, move on to the next entry. */
		if (Atp_ParseRecParmDefEntry(parseRec).parmcode == ATP_BCS)
		  parseRec->CurrPDidx++;

		/*
			Check to see if it's end of case already, i.e. an empty
			case construct.
		*/
		finish = (Atp_BoolType) (parseRec->CurrPDidx == EndCaseBlkParmDefIdx);

		/*
		 * Descend parmdef and invoke parsers.
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
			} else {
				char *parm_type_name =
						Atp_ParmTypeString(
							Atp_ParseRecParmDefEntry(parseRec).parmcode);

				Atp_ShowErrorLocation();
				parseRec->ReturnStr = Atp_MakeErrorMsg(ERRLOC,
											ATP_ERRCODE_PARSER_MISSING,
											parm_type_name);
				result = ATP_ERROR;
			}

			/* Update parseRec result. */
			parseRec->result = result;

			/*
				 Check for error condition and move down parmdef,
				 checking for end of case block.
			 */
			finish = (Atp_BoolType) ((result != ATP_OK) ||
									 (++parseRec->CurrPDidx >= EndCaseBlkParmDefIdx));
		} /* while */

		return result;
}
