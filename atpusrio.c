/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpusrio.c

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains user I/O functions
						for the interactive prompting feature.

	Comments:			MODULE INCOMPLETE - FEATURE NOT IMPLEMENTED

*******************************************************************-*/

#include <stdio.h>

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char	*__Atp_Local_FileName__ = __FILE__;
#endif

/* Definitions */
#define DEFAULT_NUM_BUFSIZE 32

/* Global declarations */
Atp_BoolType	Atp_InteractivePrompting = FALSE;
Atp_Result		(*Atp_InputParmFunc)_PROTO_((Atp_PromptParmRec *, char **)) = NULL;
Atp_Result		(*Atp_OutputToUserFunc)_PROTO_((char *fmtstr,...)) = NULL;

/* Forward declarations */
void	Atp_SetPromptFunc _PROTO_((Atp_Result (*func)(Atp_PromptParmRec *, char **)));
Atp_PromptParmRec * Atp_CreatePromptParmRec _PROTO_((Atp_ParserStateRec *parseRec));
void	Atp_FreePromptParmRec _PROTO_((Atp_PromptParmRec *recPtr));

/*+*******************************************************************

	Function Name:		Atp_SetPromptFunc

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		This function sets the prompt string.

	Parameters:

	Global Variables:	...

	Results:

	Calls:

	Called by:			...

	Side effects:		...

	Notes:				INCOMPLETE

	Modifications:
		Who			When				Description
	----------	-------------	-----------------------------
	Alwyn Teh	30 July	1992	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_SetPromptFunc (Atp_Result (*func)(Atp_PromptParmRec *, char **))
#else
void
Atp_SetPromptFunc(func)
	Atp_Result	(* func)();
#endif
{
	Atp_InputParmFunc = func;
}

/*+*******************************************************************

	Function Name:		Atp_CreatePromptParmRec

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		This function creates a record of prompt strings.

	Parameters:			...

	Global Variables:	...

	Results:			...

	Calls:

	Called by:			...

	Side effects:		...

	Notes:				INCOMPLETE

	Modifications:
		Who			When				Description
	----------	-------------	------------------------------
	Alwyn Teh	30 July	1992	Initial Creation

********************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
Atp_PromptParmRec *Atp_CreatePromptParmRec(Atp_ParserStateRec *parseRec)
#else
Atp_PromptParmRec *
Atp_CreatePromptParmRec(parseRec)
	Atp_ParserStateRec	*parseRec;
#endif
{
	Atp_PromptParmRec	*rec = NULL;
	char				*name, *desc, *min, *max, *default_string, *prompt, *prompt_format;
	int					prompt_length = 0;
	Atp_ParmCode		parmcode;

	rec = (Atp_PromptParmRec *)MALLOC(sizeof(Atp_PromptParmRec), NULL);

	name = (char *) MALLOC(strlen((parseRec->ParmDefPtr)[parseRec->CurrPDidx].Name) + 1, NULL);
	desc = (char *) MALLOC(strlen((parseRec->ParmDefPtr)[parseRec->CurrPDidx].Desc) + 1, NULL);

	rec->ParmName = strcpy(name, (parseRec->ParmDefPtr)[parseRec->CurrPDidx].Name);
	rec->ParmDesc = strcpy(desc, (parseRec->ParmDefPtr)[parseRec->CurrPDidx].Desc);
	parmcode = (parseRec->ParmDefPtr)[parseRec->CurrPDidx].parmcode;

	min = max = NULL;

	switch(Atp_PARMCODE(parmcode)) {
		case ATP_BRP:
		case ATP_NUM:
		case ATP_STR: {
			min = (char *) MALLOC(DEFAULT_NUM_BUFSIZE, NULL);
			max = (char *) MALLOC(DEFAULT_NUM_BUFSIZE, NULL);
			(void) sprintf(min, "%d",
							(Atp_NumType)((parseRec->ParmDefPtr)[parseRec->CurrPDidx].Min));
			(void) sprintf(max, "%d",
							(Atp_NumType)((parseRec->ParmDefPtr)[parseRec->CurrPDidx].Max));
			break;
		}
		case ATP_UNS_NUM:
		case ATP_DATA: {
			min = (char *) MALLOC(DEFAULT_NUM_BUFSIZE, NULL);
			max = (char *) MALLOC(DEFAULT_NUM_BUFSIZE, NULL);
			(void) sprintf(min, "%u",
							(Atp_UnsNumType)((parseRec->ParmDefPtr)[parseRec->CurrPDidx].Min));
			(void) sprintf(max, "%u",
							(Atp_UnsNumType)((parseRec->ParmDefPtr)[parseRec->CurrPDidx].Max));
			break;
		}
		case ATP_REAL: {
			min = (char *) MALLOC(DEFAULT_NUM_BUFSIZE, NULL);
			max = (char *) MALLOC(DEFAULT_NUM_BUFSIZE, NULL);
			(void) sprintf(min, "%g",
							(Atp_RealType)((parseRec->ParmDefPtr)[parseRec->CurrPDidx].Min));
			(void) sprintf(max, "%g",
							(Atp_RealType)((parseRec->ParmDefPtr)[parseRec->CurrPDidx].Max));
			break;
		}
		default: break;
	}

	rec->MinValueStr = min;
	rec->MaxValueStr = max;

	default_string = NULL;

	if (AtpParmIsOptional(parmcode)) {
	  switch(Atp_PARMCODE(parmcode)) {
		case ATP_BLS:
		case ATP_BRP:
		case ATP_BCH:
		case ATP_DATA:
		case ATP_COM: {
			if (Atp_ParseRecDefaultParmPointer(parseRec) != NULL) {
			  default_string = (char *) MALLOC(18, NULL);
			  default_string = strcpy(default_string, "default available");
			}
			break;
		}
		case ATP_NUM: {
			default_string = (char *) MALLOC(DEFAULT_NUM_BUFSIZE, NULL);
			(void)sprintf(default_string, "%d",
							(Atp_NumType)(Atp_ParseRecDefaultParmValue(parseRec)));
			break;
		}
		case ATP_UNS_NUM: {
			default_string = (char *) MALLOC(DEFAULT_NUM_BUFSIZE, NULL);
			(void)sprintf(default_string, "%u",
							(Atp_UnsNumType)(Atp_ParseRecDefaultParmValue(parseRec)));
			break;
		}
		case ATP_REAL: {
			default_string = (char *) MALLOC(DEFAULT_NUM_BUFSIZE, NULL);
			(void)sprintf(default_string, "%g",
							(Atp_RealType)(Atp_ParseRecDefaultParmValue(parseRec)));
			break;
		}
		case ATP_STR:
		case ATP_KEYS: {
			char *str = (char *)Atp_ParseRecDefaultParmPointer(parseRec);
			if (str != NULL) {
			  default_string = (char *) MALLOC(strlen(str)+1, NULL);
			  default_string = strcpy(default_string, str);
			}
			break;
		}
		default: break;
	  } /* switch */
	} /* if */

	rec->DefaultValueStr = default_string;
	prompt_format = "%s = ";
	prompt_length = strlen(name) + strlen(prompt_format) + 1;
	prompt = (char *) MALLOC(prompt_length, NULL);

	(void) sprintf(prompt, prompt_format, name);
	rec->ReadyMadePrompt = prompt;

	rec->NoOfAttempts = 0;

	return rec;
}

/*+*******************************************************************

	Function Name:		Atp_FreePromptParmRec

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		This function frees the record of prompt strings.

	Parameters:			...

	Global Variables:	...

	Results:			...

	Calls:				...

	Called by:			...

	Side effects:		...

	Notes:				INCOMPLETE

	Modifications:
		Who			When				Description
	----------	-------------	-----------------------------
	Alwyn Teh	30 July 1992	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_FreePromptParmRec(Atp_PromptParmRec *recPtr)
#else
void
Atp_FreePromptParmRec(recPtr)
Atp_PromptParmRec	*recPtr;
#endif
{
	if (recPtr != NULL) {

	  if (recPtr->ParmName != NULL)
	    FREE(recPtr->ParmName);

	  if (recPtr->ParmDesc != NULL)
		FREE(recPtr->ParmDesc);

	  if (recPtr->MinValueStr != NULL)
		FREE(recPtr->MinValueStr);

	  if (recPtr->MaxValueStr != NULL)
		FREE(recPtr->MaxValueStr);

	  if (recPtr->DefaultValueStr != NULL)
		FREE(recPtr->DefaultValueStr);

	  if (recPtr->ReadyMadePrompt != NULL)
		FREE(recPtr->ReadyMadePrompt);

	  FREE(recPtr);

	}
}
