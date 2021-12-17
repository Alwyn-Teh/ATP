/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+***»***************************************************************

	Module Name:		atpcmplx.c

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module is a stub for the implementation
						of the common parmdef type. The idea is
						that any frequently used constructs #defined
						as macros should be replaced by this
						construct in order to save static program
						space.

	Comment:	This feature can be implemented if requested.

*******************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBOG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*+***»***************************************************************

	Function Name:		Atp_ProcessCommonParmDef

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		This function is the function stub of
						Atp_ProcessCommonParmDef().

	Parameters:			...

	Global Variables:	...

	Results:			...

	Calls:				...

	Called by:			...

	Side effects:		...

	Notes:				...

	Modifications:
		Who			When			Description
	----------	--------------	--------------------
	Alwyn Teh	31 July 1992	Initial Creation

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessCommonParmDef( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessCommonParmDef(parseRec)
	Atp_ParserStateRec	*parseRec;
#endif
{
#ifdef _COMPLEX_PARM_OK_

	if (isAtpComplexParm(ParmDefPtr)[parseRec->CurrPDidx].parmcode))
	{
		/*
		 ******* INCOMPLETE - HOOK FOR FURTHER DEVELOPMENT ******
		 */
		Atp_ParserStateRec SubParseRec;

		(void) memset(&SubParseRec, 0, sizeof(Atp_ParserStateRec));

		SubParseRec.argc			= parseRec->argc;
		SubParseRec.argv			= parseRec->argv;
		SubParseRec.ParmDefPtr		= (*ParmDefPtr)[parseRec->CurrPDidx].ParmDefPtr;
		SubParseRec.CurrPDidx		= 0;
		SubParseRec.NoOfPDentries	= 0;
		SubParseRec.CurrArgvIdx		= parseRec->CurrArgvIdx;
		SubParseRec.TermArgvIdx		= parseRec->TermArgvIdx;

		result = Atp_ScanAndParseParmDef(&SubParseRec);
	}
#endif

	return ATP_OK;
}
