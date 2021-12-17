/* EDITION AC03 (REL002), ITD ACST.169 (95/05/12 19:19:40) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpfindp.c

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the common	shared
						utility for finding a parameter in the
						parmstore.

********************************************************************-*/

#if defined(__STDC__) || defined(__cplusplus)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/* Forward declarations */
static Atp_BoolType MatchParmType _PROTO_((	Atp_ParmCode ParmTypeToMatch,
											int HowManyTypesToMatch,
											Atp_ParmCode *ParmTypeArray ) );

/*+******************************************************************

	Function Name:		Atp_SearchParm

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		A common function	used	by	parameter
						retrieval functions.

						SearchParm() finds the parameter given its
						type and name, and returns a pointer to the
						control store from which any information can
						be extracted by its caller.

						This function has been	made	self-recursive
						so that nested levels also get searched.

	Notes:				For internal use only

	Modifications:
		Who			When				Description
	----------	--------------	-------------------------------
	Alwyn Teh	27 July 1992	Initial Creation
	Alwyn Teh	13 July 1993	Robustify - cannot go on searching
								if parmName in CtrlStore is NULL.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
ParmStoreInfo * Atp_SearchParm( ParmStoreInfo *CtrlStorePtr, ...)
#else
ParmStoreInfo *
Atp_SearchParm(CtrlStorePtr, va_alist)
	ParmStoreInfo *CtrlStorePtr;
	va_dcl
#endif
{
	va_list					argPtr;
	register int			x;
	static	char			*SearchParmName = NULL;
	static	int				NoOfParmTypesToMatch = 0;
	static	Atp_ParmCode	SearchParmTypeArray[NO_OF_ATPPARMCODES];
	static	int				SearchParmLevel = -1;

	ParmStoreInfo			*resultPtr;

#if defined (__STDC__) || defined(__cplusplus)
	va_start(argPtr, CtrlStorePtr);
#else
	va_start(argPtr);
#endif

	if (CtrlStorePtr == NULL) {
	  va_end(argPtr);
	  return NULL;
	}

	SearchParmLevel++;

	if (SearchParmLevel == 0) {
	  SearchParmName = va_arg(argPtr, char *);
	  NoOfParmTypesToMatch = va_arg(argPtr, int);

	  if (NoOfParmTypesToMatch != 0) {
		for (x = 0; x < NoOfParmTypesToMatch; x++) {
#if sun || __hp9000s700 || __hp9000s800
		   SearchParmTypeArray[x] = va_arg(argPtr, Atp_ParmCode);
#else
		   SearchParmTypeArray[x] = va_arg(argPtr, int);
#endif
		}
	  }
	}

	/* Search the present level, ground level first. */
	for (x = 0, resultPtr = NULL;
	    CtrlStorePtr[x].parmcode != ATP_EOP;
	    x++)
	{
		/* parmName not allowed to be NULL, likely error here if so. */
		if (CtrlStorePtr[x].parmName == NULL)
		  return NULL;

		if ( (MatchParmType(CtrlStorePtr[x].parmcode,
			  NoOfParmTypesToMatch,
			  SearchParmTypeArray))
			&&
			  (Atp_Strcmp(CtrlStorePtr[x].parmName, SearchParmName) == 0) )
		{
			resultPtr = &CtrlStorePtr[x];
			break; /* Found it! */
		}
	}

	if (resultPtr != NULL) {
	  va_end(argPtr);
	  SearchParmLevel--;
	  return(resultPtr);
	}

	/*
		If parameter wasn't found on present level, go down
		further to search.
	*/
	for (x = 0, resultPtr = NULL;
		 (CtrlStorePtr[x].parmcode != ATP_EOP);
		 x++)
	{
		if (CtrlStorePtr[x].downLevel != NULL) {
		  resultPtr = Atp_SearchParm(CtrlStorePtr[x].downLevel);
		  	  /* note: recursive call uses downLevel pointer ONLY */
		  if (resultPtr != NULL) break;
		}
	}

	va_end(argPtr);

	SearchParmLevel--;

	return resultPtr;
}

/*+*******************************************************************

	Function Name:		MatchParmType

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Searches the given Atp_ParraCode array to see
						if a given parmcode can be matched.
						Notes:	Array size must be known.

	Modifications:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	27 July 1992	Initial Creation

*******************************************************************_*/
#if defined(__STDC__) || defined(__cplusplus)
static Atp_BoolType MatchParmType
(
	Atp_ParmCode	ParmTypeToMatch,
	int				HowManyTypesToMatch,
	Atp_ParmCode	*ParmTypeArray
)
#else
static Atp_BoolType
MatchParmType(ParmTypeToMatch, HowManyTypesToMatch, ParmTypeArray)
	Atp_ParmCode	ParmTypeToMatch;
	int				HowManyTypesToMatch;
	Atp_ParmCode	*ParmTypeArray;
#endif
{
	register int beepbeep;

	if (HowManyTypesToMatch == 0) {
	  return TRUE;	/* i.e. don't care what type */
	}

	for (beepbeep = 0; beepbeep < HowManyTypesToMatch; beepbeep++)
	{
	   if (ParmTypeArray[beepbeep] == Atp_PARMCODE(ParmTypeToMatch))
	   {
		 return TRUE;
	   }
	}

	return FALSE;
}
