/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+****************************************************************************

	Module Name:		atpnullp.c

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research
						Northern Telecom

	Description:		This module contains the implementation of the
						NULL parameter processor.

****************************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*+*****************************************************************

	Function Name:		Atp_ProcessNullParm

	Copyright:			BNR Europe Limited,	1992
						Bell-Northern Research
						Northern Telecom

	Description:		There is no syntax parsing associated with
						the NULL parameter. Therefore, it is used
						as a filler, so a NULL value is stored.

	Parameters:			Atp_ParserStateRec *

	Global Variables:	None

	Results:			Atp_Result

	Calls:				Atp_StoreParm

	Called by:			parmdef scanner via	atp_null_def macro
						in application

	Side effects:		N/A

	Notes:				N/A

	Modifications:
		Who			When				Description
	----------	-----------------	----------------------
	Alwyn Teh	27 November	1992	Initial	Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessNullParm( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessNullParm(parseRec)
	Atp_ParserStateRec	*parseRec;
#endif
{
	/*
		A NULL parameter has no parameters to parse, just store
		the value NULL in the parmstore.
	 */
	Atp_StoreParm(	&Atp_ParseRecParmDefEntry(parseRec),
					(parseRec->ValueUsedIsDefault = FALSE),
					0 ) ;

	parseRec->result = ATP_OK;

	return ATP_OK;
}
