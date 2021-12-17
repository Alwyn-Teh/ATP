/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+********************************************************************

	Module Name:		atppmdef.c

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:	This module contains miscellaneous	functions
	associated with operations on parmdefs.

*********************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*+*******************************************************************

	Function Name:		Atp_IsEmptyParmDef

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		If a ParmDef contains no parameters, returns
						TRUE, else FALSE. This is a pretty rough
						check for the time being.

	Parameters:			CommandRecord * or Atp_CmdRec	*

	Results:			boolean true or FALSE (Atp_BoolType)

	Called by:			atphelp.c, atpmanpg.c, atpparse.c

	Notes:				Relies on initialized field	NoOfPDentries
						within CommandRecord.

	Modifications:
		Who			When				Description
	----------	-------------	-------------------------------
	Alwyn Teh	26 July 1992 	Initial Creation

*******************************************************************.*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_BoolType Atp_IsEmptyParmDef(CommandRecord *CmdRecPtr)
#else
Atp_BoolType
Atp_IsEmptyParmDef(CmdRecPtr)
	CommandRecord * CmdRecPtr;
#endif
{
	if (CmdRecPtr == NULL)
	{
	  return TRUE;
	}

	/* No parmdef for command. */
	if (CmdRecPtr->parmDef == NULL)
	{
	  return TRUE;
	}

	/* Checks if ParmDef defined as EMPTY_PARMDEF. */
	if ((CmdRecPtr->NoOfPDentries == 2) &&
	    ((CmdRecPtr->parmDef)[0].parmcode == ATP_BPM) &&
	    ((CmdRecPtr->parmDef)[1].parmcode == ATP_EPM))
	{
	  return TRUE;
	}

	if (CmdRecPtr->NoOfPDentries > 2)
	{
	  return FALSE;
	}

	/*
		Default, say yes just in case (shouldn't reach here anyway)
	 */
	return TRUE;
}

/*+*******************************************************************

	Function Name:		Atp_EvalNoOfParmDefEntries

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Evaluates the number of parmdef entries.

	Parameters:			parmdef, compile time value	of number of
						parmdef entries

	Results:			Number of parameter entries.

						Returns the number of entries in a parameter
						definition table if Atp_2Tcl_CreateCommand()
						was NOT called from within a loop. (using
						sizeof(pd)/sizeof(Atp_ParmDefEntry))

						If Atp_2Tcl_CreateCommand() was called from
						within a loop, loading parameters from a
						static	table,	then
						sizeof(pd)/sizeof(Atp_ParmDefEntry) would be
						wrong because sizeof(pd) != sizeof(object)
						but is sizeof(pointer to pd) instead !!
						Therefore, if this happens, then
						Atp_EvalNoOfParmDefEntries counts the number
						of entries in the parameter definition table
						beginning with the first until an END_PARMS
						is encountered. If END_PARMS is missing
						from the table, this procedure will crash!!

	Notes:	N/A

	Modifications:
		Who			When				Description
	----------	-------------	----------------------------
	Alwyn Teh	30 June 1992 	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_EvalNoOfParmDefEntries (Atp_ParmDefEntry *ParmDefPtr, int NoOfPDentries)
#else
int
Atp_EvalNoOfParmDefEntries(ParmDefPtr, NoOfPDentries)
	Atp_ParmDefEntry	*ParmDefPtr;
	int					NoOfPDentries;
#endif
{
	register int i;
	register ParmDefEntry *ParmDefEntryPtr;

	if (ParmDefPtr == NULL)
	{
	  return 0;
	}
	else
	{
	  /*
			If Atp_2Tcl_CreateCommand() is used inside a loop,
			NoOfPDentries will be sizeof(pointer to PD
			table)/sizeof(one PD entry) i.e. a fraction, and
			would get truncated to a 0.
	   */
	  if (NoOfPDentries == 0)
	  {
		/* Count no of entries in table. */
		for (i = 1, ParmDefEntryPtr = (ParmDefEntry *)ParmDefPtr;
			 ParmDefEntryPtr->parmcode != ATP_EPM;
			 i++, ParmDefEntryPtr++)
		{
		   continue;
		}

		return i;
	  }
	  else
	  {
		i = NoOfPDentries;
		return i;
	  }
	}
}
