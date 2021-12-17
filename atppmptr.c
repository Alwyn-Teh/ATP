/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atppmptr.c

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains a common
						general-purpose parameter retrieval function
						and a reset function which returns the
						address of the parameter store.

*******************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*+*******************************************************************

	Function Name:		Atp_RetrieveParm

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Retrieves pointer to any parameter.

	Called by:			Atp_ParmPtr() macro in application

	Note:				gets first occurence of named parameter

	Modifications:
		Who			When				Description
	----------	--------------	------------------------------
	Alwyn Teh	27 July 1992	Initial Creation

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_ByteType *Atp_RetrieveParm
(
	char *AnyParmName,
	char *filename,
	int  line_number
)
#else
Atp_ByteType *
Atp_RetrieveParm(AnyParmName, filename, line_number)
	char *AnyParmName;
	char *filename;
	int  line_number;
#endif
{
	ParmStoreInfo		*MatchedParmInfoPtr;
	ParmStoreMemMgtNode	*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_ByteType		*anyParm = NULL;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore, AnyParmName, 0);

	if (MatchedParmInfoPtr != NULL)
	{
	  anyParm = ((Atp_ByteType *)MatchedParmInfoPtr->parmValue);
	  return anyParm;
	}
	else
	{
	  /* This function may not return. */
	  Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
							"Atp_ParmPtr()", AnyParmName,
							filename, line_number);
	  return NULL;
	}
}

/*+**************************»****************************************

	Function Name:		Atp_ResetParmPtr

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Returns a pointer to the beginning of the
						parameter store.

	Note:				If nested commands used, innermost parmstore
						is referenced. Function can only be used in
						a callback function. This explains the use
						of the word "reset" as no real internal
						reset is actually done. Function inherited
						from CLI.

	Modifications:
		Who			When				Description
	----------	-------------	------------------------------
	Alwyn Teh	27 July 1992	Initial Creation

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_ByteType * Atp_ResetParmPtr(void)
#else
Atp_ByteType * Atp_ResetParmPtr()
#endif
{
	ParmStoreMemMgtNode	*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_ByteType *Parms = NULL;

	Parms = (Atp_ByteType *)CurrParmStore->DataStore;

	return Parms;
}
