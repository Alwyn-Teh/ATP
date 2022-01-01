/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpcheck.c

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		Provides miscellaneous checking	functions
						for commands and parmdefs.

*******************************************************************_*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*+*******************************************************************

	Function Name:		isAtpCmdRecPtr

	Copyright:			BNR Europe Limited,	1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Determines whether an unknown pointer points
						to an ATP command record.

	Modifications:
		Who			When				Description
	----------	-----------------	-----------------------
	Alwyn Teh	13 November 1992	Initial	Creation

********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
int isAtpCmdRecPtr( void *AnyPointer )
#else
int
isAtpCmdRecPtr(AnyPointer)
	void *AnyPointer;
#endif
{
	if (Atp_FindCommand(AnyPointer,0) == NULL)
	  return 0;
	else
	  return 1;
}

/*+*******************************************************************

	Function Name:		isAtpCommand

	Copyright:			BNR Europe Limited,	1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Determines whether a command record pointer
						references a genuine ATP command.

	Modifications:
		Who			When				Description
	----------	------------------	------------------------
	Alwyn Teh	16 September 1992	Initial	Creation

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int isAtpCommand ( Atp_CmdRec *__Atp_CmdRec_Ptr )
#else
int
isAtpCommand(__Atp_CmdRec_Ptr)
	Atp_CmdRec *__Atp_CmdRec_Ptr;
#endif
{
		CommandRecord *Atp_CmdRec_Ptr = (CommandRecord *)__Atp_CmdRec_Ptr;
		if (Atp_CmdRec_Ptr == NULL)
		  return 0;
		else
		  return (isAtpCmdRecPtr(Atp_CmdRec_Ptr));
}

/*+*******************************************************************

	Function Names:		Atp_VerifyCmdRecParmDef
						Atp_VerifyParmDef

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Verifies that the ParmDef is sane - i.e.
						it's not going to cause the parser to trip
						over things like unmatched and/or mismatched
						BEGIN_REPEATs, BEGIN_CHOICEs ...... etc.
						Notes:	Two interface functions	are provided for
						different access requirements.

	Modifications:
		Who			When				Description
	----------	-------------	----------------------------------
	Alwyn Teh	13 July 1992	Initial Creation
	Alwyn Teh	26 June 1993	Renamed Atp_VerifyParmDef() to
								Atp_VerifyCmdRecParmDef().
								Atp_VerifyParmDef() created to
								accept parmdef pointer argument.

********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
int Atp_VerifyParmDef( Atp_ParmDefEntry *parmdef, int entries )
#else
int
Atp_VerifyParmDef(parmdef, entries)
	Atp_ParmDefEntry *parmdef;
	int entries; /* no of parmdef entries */
#endif
{
		Atp_ParmCode FirstEntryParmCode, LastEntryParmCode;

		printf("atpcheck.c: Atp_VerifyParmDef(parmdef %d, entries %d)\n", parmdef, entries);

		/*
			 Begin by checking if construct brackets are matching
			 properly.
		 */
		FirstEntryParmCode = parmdef[0].parmcode;
		LastEntryParmCode = parmdef[entries - 1].parmcode;

		printf("atpcheck.c: Atp_VerifyParmDef(FirstEntryParmCode = %d, LastEntryParmCode = %d)\n", FirstEntryParmCode, LastEntryParmCode);

		if (!isAtpBeginConstruct(FirstEntryParmCode) ||
			!isAtpEndConstruct(LastEntryParmCode)) {
		  printf("atpcheck.c: Atp_VerifyParmDef() returns ATP_ERRCODE_NO_PARMDEF_OUTER_BEGIN_END\n");
		  return ATP_ERRCODE_NO_PARMDEF_OUTER_BEGIN_END;
		} else {
			/*
			 NOTE: For future enhancement, this could provide more
			 help on where error is located in parmdef .............
			 */
			if (Atp_ConstructBracketMatcher((ParmDefEntry*) parmdef, 0, entries) < 0) {
			  return ATP_ERRCODE_PARMDEF_CONSTRUCT_MATCH_ERROR;
			}
			else {
			  return 0; /* parmdef checked */
			}
		}
}

#if defined(__STDC__) || defined(__cplusplus)
char * Atp_VerifyCmdRecParmDef( Atp_CmdRec *Atp_CmdRecPtr )
#else
char *
Atp_VerifyCmdRecParmDef (Atp_CmdRecPtr)
	Atp_CmdRec *Atp_CmdRecPtr;
#endif
{
		CommandRecord	*CmdRecPtr = (CommandRecord*) Atp_CmdRecPtr;
		int				result = 0;
		char			*result_str = NULL; /* NULL indicates parmdef OK */

		printf("atpcheck.c: Atp_VerifyCmdRecParmDef() - 1\n");
		if (CmdRecPtr == NULL)
		  return NULL;

		printf("atpcheck.c: Atp_VerifyCmdRecParmDef() - 2\n");
		if (CmdRecPtr->parmDef == NULL)
			return NULL;

		printf("atpcheck.c: Atp_VerifyCmdRecParmDef() - 3\n");
		if (CmdRecPtr->ParmDefChecked)
			return NULL;

		printf("atpcheck.c: Atp_VerifyCmdRecParmDef() - 4 calling Atp_VerifyParmDef() - CmdRecPtr->parmDef = %d, CmdRecPtr->NoOfPDentries = %d\n", CmdRecPtr->parmDef, CmdRecPtr->NoOfPDentries);

		result = Atp_VerifyParmDef((Atp_ParmDefEntry*) CmdRecPtr->parmDef,
													   CmdRecPtr->NoOfPDentries);

		printf("atpcheck.c: Atp_VerifyCmdRecParmDef() - 5\n");
		if (result != 0) {
		  printf("Atp_VerifyParmDef() returned %d\n", result);
		  Atp_ShowErrorLocation();
		  result_str = Atp_MakeErrorMsg(ERRLOC, result, CmdRecPtr->cmdName);
		  printf("result_str = %s\n", result_str);
		  printf("atpcheck.c: Atp_VerifyCmdRecParmDef() - 6\n");
		}

		printf("Atp_VerifyParmDef() returned %d\n", result);
		printf("atpcheck.c: Atp_VerifyCmdRecParmDef() - 7\n");

		return result_str;

		/*
			 NOTE: More checks could be done on the fields of the
			 parmdef ...
		 */
}

/*+*******************************************************************

	Function Name:		Atp_AllParmsInParmDefAreOptionalOrNull

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		See if all	parameters contained in
						parmdef are optional or null. If so, you
						only need to type the command name and
						then all the default values will be
						deployed. Otherwise, ATP will expect
						more input than just the command name.

	Notes:				Assumes that parmdef has already been
						verified by Atp_VerifyParmDef() and is
						sane.

	Modifications:
		Who			When			Description
	----------	-------------	------------------------
	Alwyn Teh	13 July 1992	Initial Creation
				17 May 1993		Put scan loop in new function
								Atp_isConstructOfOptParms().
				24 Dec 1993		Change name from Atp_AllParmsInParmDefAreOptional
								to Atp_AllParmsInParmDefAreOptionalOrNull;
								and Atp_isConstructOfOptParms to
								Atp_isConstruetofOptOrNul1Parms

*******************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
int Atp_AllParmsInParmDefAreOptionalOrNull( CommandRecord *CmdRecPtr )
#else
int
Atp_AllParmsInParmDefAreOptionalOrNull(CmdRecPtr)
	CommandRecord *CmdRecPtr;
#endif
{
		if (CmdRecPtr == NULL)
		  return FALSE;

		if (CmdRecPtr->parmDef == NULL)
		  return FALSE;

		return Atp_isConstructOfOptOrNullParms(CmdRecPtr->parmDef, 0);
}

/*+*****************************************************************

	Function Name:		Atp_isConstructOfOptOrNullParms

	Copyright:			BNR Europe Limited,	1993
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Checks to see if construct contains
						parameters and constructs that are all
						optional.

	Modifications:
		Who			When					Description
	----------	-----------------	--------------------------
	Alwyn Teh	17 May 1993			Initial	Creation
	Alwyn Teh	24 December 1993	Change name from Atp_isConstructOfOptParms
									to Atp_isConstructOfOptOrNullParms; and test
									for NULL parameter as well.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_isConstructOfOptOrNullParms( ParmDefTable parmDef, int index )
#else
int
Atp_isConstructOfOptOrNullParms(parmDef, index)
	ParmDefTable parmDef;
	int	index;
#endif
{
		Atp_ParmCode	CurrParmCode;
		int				TermIndex;

		CurrParmCode = parmDef[index].parmcode;

		if (!isAtpBeginConstruct(CurrParmCode))
		  return FALSE;

		TermIndex = parmDef[index].matchIndex;

		index++;

		while (index < TermIndex) {
			CurrParmCode = parmDef [index].parmcode;

			if (isAtpBeginConstruct(CurrParmCode)) {
				/* If mandatory construct found, return false. */
				if (!AtpParmIsOptional(CurrParmCode)) {
				  return FALSE;
				}
				else {
					/* Jump to end of construct */
					index = parmDef[index].matchIndex;
				}
			}
			else {
				/*
					 If mandatory regular or complex parm found, return false.
					 However, exclude NOLL parameter.
				 */
				if (!AtpParmIsOptional(CurrParmCode) &&
						Atp_PARMCODE(CurrParmCode) != ATP_NULL)
				  return FALSE;
			}
			index++;
		}

		return (int) TRUE ;
}

/*+*******************************************************************

	Function Name:		Atp_ConstructBracketMatcher

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		A generic ATP parmdef construct	matcher.
						Matches construct brackets according to
						construct type and presence (i.e.
						optionality) bidirectionally. Whenever
						construct brackets are matched, their
						corresponding indices are recorded in the
						parmdef entries so that matching only takes
						place once. All enclosed matched pairs also
						have their indices recorded.

						Returns index of matched construct. Returns
						index of parameter if not a construct.
						Returns negative result if unsuccessful or
						error encountered.

	Modifications:
		Who			When			Description
	----------	-------------	------------------------
	Alwyn Teh	16 July 1992	Initial Creation

********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
int Atp_ConstructBracketMatcher
(
	ParmDefEntry	*ParmDefPtr,
	Atp_PDindexType	InputPDidx,
	int				NoOfPDEs
)
#else
int
Atp_ConstructBracketMatcher(ParmDefPtr, InputPDidx, NoOfPDEs)
	ParmDefEntry	*ParmDefPtr;
	Atp_PDindexType	InputPDidx;
	int				NoOfPDEs;
#endif
{
		struct {
			Atp_ParmCode	parmcode;
			Atp_PDindexType	index;
		} PDstack[ATP_MAX_PARSE_NESTING_DEPTH + 6];

		register Atp_PDindexType CurrPDidx = InputPDidx;
		register Atp_PDindexType stackIdx;

		Atp_ParmCode InputPDcode, CurrPDcode, MatchPDcode;

		int direction = 1;
			/* default = 1 for searching downwards (-1 for up) */

		int TermPDEidx; /* index of terminating parmdef entry */
		int rc; /* return code */

		/* Empty parmdef or no parmdef supplied */
		if (ParmDefPtr == NULL)
		  return -111; /* arbitrary negative return code */

		/* Index must be positive, if not, return error. */
		if (InputPDidx < 0)
		  return -222; /* arbitrary negative return code */

		/*
			 We MUST know how big the parmdef is, if we don't, we must
			 count the entries.
		 */
		if (NoOfPDEs < 2)
		  NoOfPDEs = Atp_EvalNoOfParmDefEntries((Atp_ParmDefEntry*) ParmDefPtr, sizeof(*ParmDefPtr));

		/* If input index is out of range, return error. */
		if (InputPDidx > (NoOfPDEs - 1))
		  return -333; /* arbitrary negative return code */

		/* Figure out where the end of parmdef is. */
		TermPDEidx = NoOfPDEs - 1; /* first parmdef entry has index 0 */

		/* What is the parmcode of the input parmdef entry? */
		InputPDcode = ParmDefPtr[InputPDidx].parmcode;

		/*
			 If a BEGIN_ construct, define matching END_ construct to
			 search for, and vice versa.
		 */
		MatchPDcode = InputPDcode; /* initialise MatchPDcode */
		if (isAtpConstruct(InputPDcode)) {
			if (isAtpBeginConstruct(InputPDcode)) {
				Atp_SetParmType(MatchPDcode, ATP_CONSTRUCT_TYPE_END);
			}
			else
			if (isAtpEndConstruct(InputPDcode)) {
				Atp_SetParmType(MatchPDcode, ATP_CONSTRUCT_TYPE_BEGIN);
				direction = -1; /* scan direction */
			}
		} else {
			/*
				 If a regular parm, simply assign its own index to its
				 matchIndex.
			 */
			if (isAtpRegularParm(InputPDcode)) {
				ParmDefPtr[InputPDidx].matchIndex = InputPDidx;
			}

			/*
				 Then, return input index since parmdef entry is not one
				 of a paired construct.
			 */
			return (InputPDidx);
		}

		/*
			 The initial value of a construct's matchIndex is zero.
			 This means that this function hasn't been called.
			 However, it could also be the last parmdef entry matching
			 up with the first entry with index 0. Check this before
			 moving on. If matching has been done before, simply
			 return the matched index as this is what's required.
		 */
		if ((ParmDefPtr[InputPDidx].matchIndex) == 0) {
		  if (InputPDidx == (NoOfPDEs - 1))
			return 0;
		}
		else {
		  return (int)(ParmDefPtr[InputPDidx].matchIndex);
		}

		rc = -2; /* initial return code, indicating error */
		stackIdx = -1; /* stack empty condition */

		while (1) {
			/* Let's see what we've got here. */
			CurrPDcode = ParmDefPtr[CurrPDidx].parmcode;

			/*
				 (1)	Descending parmdef :

							On encountering an open construct bracket, put
							it on the stack; if it's a close construct
							bracket, the corresponding open construct
							bracket is removed from the stack.

				 (2)	Ascending parmdef :

							On encountering a close construct bracket, put
							it on the stack; if it's an open construct
							bracket, the corresponding close construct
							bracket is removed from the stack.

				 NOTE: The bracket sequence is legal UNLESS ........

				 (i)	On reading a closing/opening bracket, it
						does NOT corresponding to the
						opening/closing bracket on top of the
						stack, or the stack is empty.

				 (ii)	The stack is non-empty on completion.
			 */
			if (isAtpConstruct(CurrPDcode)) {

			  if (((direction == 1) &&
					  isAtpBeginConstruct(CurrPDcode)) ||
						((direction == -1) &&
								isAtpEndConstruct(CurrPDcode))) { /* PUSH */

					/* Putting on the stack. */
					stackIdx++;
					PDstack[stackIdx].parmcode	= CurrPDcode;
					PDstack[stackIdx].index		= CurrPDidx;
				}

			  else

			  if (((direction == 1) &&
							isAtpEndConstruct(CurrPDcode)) ||
				  ((direction == -1) &&
							isAtpBeginConstruct(CurrPDcode))) { /* POP */
				/* If stack is empty, error. */
				if (stackIdx == -1) {
				  rc = -1;
				  break;
				}
				else

				/*
					 Removing off top of the stack ONLY if brackets
					 correspond.
				 */
				if (
					(AtpParmClass(CurrPDcode) ==
					 AtpParmClass(PDstack[stackIdx].parmcode))
											&&
					(AtpParmOptFlag(CurrPDcode) ==
					 AtpParmOptFlag(PDstack[stackIdx].parmcode))
											&&
					( (isAtpBeginConstruct(CurrPDcode) &&
					   isAtpEndConstruct(PDstack[stackIdx].parmcode))
								||
					  (isAtpEndConstruct(CurrPDcode) &&
					   isAtpBeginConstruct(PDstack[stackIdx].parmcode)))
					)
				{
					/*
						 Write the matching indices in each other's parmdef
						 entry.
					 */
					ParmDefPtr[(PDstack[stackIdx].index)].matchIndex = CurrPDidx;
					ParmDefPtr[CurrPDidx].matchIndex = PDstack[stackIdx].index;

					/*
						 Corrupt the values deliberately so as to prevent
						 mistakes.
					 */
					PDstack[stackIdx].parmcode = ~0;
					PDstack[stackIdx].index = ~0;

					/* Take it off the stack. */
					stackIdx--;

					/*
						 Check to see if we've found the matching bracket
						 construct (must be nothing left on stack).
					 */
					if ((stackIdx == -1) && (CurrPDcode == MatchPDcode))
					{
						/* Found it, let's get out of here. */
						rc = CurrPDidx;
						break;
					}
				}
				else {
					/*
						 Bracket construct did not match that on top of
						 stack => i.e. error.
					 */
					rc = -1;
					break;
				}
			} /* POP */

		}

		/* Go to next PD entry, whether it's up or down. */
		CurrPDidx += direction;

		if (((direction == 1) && (CurrPDidx > TermPDEidx)) ||
			((direction == -1) && (CurrPDidx < 0))) {
			/* On termination, if stack is non-empty, error. */
			if (stackIdx != -1)
			  rc = -1;
			break; /* we’ve exhausted the parmdef */
		}

		/*
			 If there's nothing on the stack, we have an error,
			 shouldn't have got here at all.
		 */
		if (stackIdx < 0) {
		  rc = -1;
		  break;
		}
	} /* end of while */

	return (rc);

}
