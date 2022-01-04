/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+********************************************************************

	Module Name:		atppmchk.c

	Copyright:			BNR Europe Limited, 1992, 1993, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module	contains parameter checking
						functions for ATP. It includes range
						checking and verification procedure (vproc)
						invocation.

*********************************************************************-*/

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

/*+*******************************************************************

	Function Name:		Atp_CheckRange

	Copyright:			BNR Europe Limited, 1992, 1993, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		Checks the ranges of parameters if range
						checking is appropriate to the type of the
						current parameter. Otherwise, checks other
						aspects of the parameter such as the number
						of repeat block instances.

						If parameter has a value, checks to see if
						it lies within the specified range, unless a
						"don't care" request is made by specifying a
						minimum value greater than the maximum
						value.

	Parameters:			variable arguments list by arrangement with
						caller type

	Global Variables:	none

	Results:			Atp_Result

	Calls:				Atp_MakeErrorMsg

	Called by:			parameter and construct processors

	Side effects:		see branch handling repeat	blocks

	Notes:				some parameters don't need	checking
						e.g. keyword, boolean, null, list,...

	Modifications:
		Who			When				Description
	----------	----------------	---------------------------------
	Alwyn Teh	26 July 1992		Initial Creation
	Alwyn Teh	27 November 1992	Check for optional repeat
									block default value being
									NULL which is indicated by
									number of instances less
									than zero. In this special
									case, do not check the
									range.
	Alwyn Teh	20 May 1993			Atp_DataDescriptor now used
									for databytes instead of
									(Atp_ByteType *)
	Alwyn Teh	27 March 1995		Add BCD digits parameter type.

*******************************************************************-*/
#if defined (__STDC__) || defined(__cplusplus)
Atp_Result Atp_CheckRange(ParmDefEntry *PDE_ptr, int isUserValue, ...)
#else
Atp_Result
Atp_CheckRange(PDE_ptr, isUserValue, va_alist)
	ParmDef Entry *PDE_ptr;
	int isUserValue;
	va_dcl
#endif
{
	va_list	argPtr;

	Atp_ParmCode	parmcode;
	Atp_BoolType	OutOfRange		= FALSE;
	Atp_BoolType	DontCareRange	= FALSE;
	char			*error_msg		= NULL;
	char			**ErrorMsgPtr	= NULL;

#if defined (__STDC__) || defined (__cplusplus)
	va_start(argPtr, isUserValue);
#else
	va_start(argPtr);
#endif

	/* Do not look at the optional flag, mask it out. */
	parmcode = Atp_PARMCODE(PDE_ptr->parmcode);

	/*
		Check to see if the ParmDefEntry doesn't care about the
		parameter's range.
	 */
	switch(Atp_PARMCODE(parmcode)) {
		case ATP_NUM:
		case ATP_STR:
		case ATP_DATA:
		case ATP_BCD:
		case ATP_BRP: case ATP_ERP:
		{
			if ((Atp_NumType)PDE_ptr->Min > (Atp_NumType)PDE_ptr->Max)
			  DontCareRange = TRUE;
			break;
		}
		case ATP_UNS_NUM:
		{
			if ((Atp_UnsNumType)PDE_ptr->Min > (Atp_UnsNumType)PDE_ptr->Max)
			  DontCareRange = TRUE;
			break;
		}
		case ATP_REAL:
		{
			if ((Atp_RealType)PDE_ptr->Min > (Atp_RealType)PDE_ptr->Max)
			  DontCareRange = TRUE;
			break;
		}
		default: break;
	}

	if (DontCareRange == TRUE) {
	  va_end(argPtr);
	  return ATP_OK;
	}

	/*
	 * Get the parameter value and check its range.
	 */
	switch(Atp_PARMCODE(parmcode)) {
		case ATP_NUM:
		{
			Atp_NumType num = va_arg(argPtr, Atp_NumType);

			if ((num < (Atp_NumType)PDE_ptr->Min) || (num > (Atp_NumType)PDE_ptr->Max))
			{
			  OutOfRange = TRUE;
			  error_msg = Atp_MakeErrorMsg(	ERRLOC,
											ATP_ERRCODE_SIGNED_NUM_OUT_OF_RANGE,
											num, PDE_ptr->Name,
											(Atp_NumType)PDE_ptr->Min,
											(Atp_NumType)PDE_ptr->Max );
			}
			break;
		}
		case ATP_UNS_NUM:
		{
			Atp_UnsNumType unum = va_arg(argPtr, Atp_UnsNumType);

			if ((unum < (Atp_UnsNumType)PDE_ptr->Min) || (unum > (Atp_UnsNumType)PDE_ptr->Max))
			{
			  OutOfRange = TRUE;
			  error_msg = Atp_MakeErrorMsg(	ERRLOC,
											ATP_ERRCODE_UNSIGNED_NUM_OUT_OF_RANGE,
											unum, PDE_ptr->Name,
											(Atp_UnsNumType)PDE_ptr->Min,
											(Atp_UnsNumType)PDE_ptr->Max );
			}
			break;
		}
		case ATP_REAL:
		{
			Atp_RealType realNum = va_arg(argPtr, Atp_RealType);

			if ((realNum < (Atp_RealType)PDE_ptr->Min) || (realNum > (Atp_RealType)PDE_ptr->Max))
			{
			  OutOfRange = TRUE;
			  error_msg = Atp_MakeErrorMsg(	ERRLOC,
											ATP_ERRCODE_REAL_NUM_OUT_OF_RANGE,
											realNum, PDE_ptr->Name,
											(Atp_RealType)PDE_ptr->Min,
											(Atp_RealType)PDE_ptr->Max );
			}
			break;
		}
		case ATP_STR:
		{
			char *str = va_arg(argPtr, char *);
			Atp_NumType strLength = va_arg(argPtr, Atp_NumType);

			/*
				Assuming that the range has been specified as
				positive numbers, it would be ok. Otherwise, if
				this error is printed, then the application
				programmer should go and fix the range.
			 */
			if ((strLength < (Atp_NumType)PDE_ptr->Min) || (strLength > (Atp_NumType)PDE_ptr->Max))
			{
			  OutOfRange = TRUE;
			  error_msg = Atp_MakeErrorMsg(	ERRLOC,
											ATP_ERRCODE_STRING_OUT_OF_RANGE,
											str, strLength, PDE_ptr->Name,
											(Atp_NumType)PDE_ptr->Min,
											(Atp_NumType)PDE_ptr->Max );
			}
			break;
		}
		case ATP_DATA:
		{
			Atp_DataDescriptor dataBytes;
			dataBytes = va_arg (argPtr, Atp_DataDescriptor);

			/*
				Assuming that the range has been specified as
				positive numbers.
			 */
			if ((dataBytes.count < (Atp_NumType)PDE_ptr->Min) ||
				(dataBytes.count > (Atp_NumType)PDE_ptr->Max))
			{
			  OutOfRange = TRUE;
			  error_msg = Atp_MakeErrorMsg(	ERRLOC,
											ATP_ERRCODE_DATABYTES_LENGTH_OUT_OF_RANGE,
											PDE_ptr->Name, dataBytes.count,
											(Atp_NumType)PDE_ptr->Min,
											(Atp_NumType)PDE_ptr->Max );
			}
			break;
		}
		case ATP_BCD:
		{
			Atp_DataDescriptor BcdDigits;
			BcdDigits = va_arg(argPtr, Atp_DataDescriptor);

			/*
				Assuming that the range has been specified as
				positive numbers.
			 */
			if ((BcdDigits.count < (Atp_NumType)PDE_ptr->Min) ||
				(BcdDigits.count > (Atp_NumType)PDE_ptr->Max))
			{
			  OutOfRange = TRUE;
			  error_msg = Atp_MakeErrorMsg(	ERRLOC,
											ATP_ERRCODE_BCD_DIGITS_LENGTH_OUT_OF_RANGE,
											PDE_ptr->Name, BcdDigits.count,
											(Atp_NumType)PDE_ptr->Min,
											(Atp_NumType)PDE_ptr->Max );
			}
			break;
		}
		case ATP_BRP:
		case ATP_ERP:
		{
			Atp_NumType NoOfRptBlklnstances = (Atp_NumType) va_arg(argPtr, Atp_UnsNumType);

			/*
				If optional default is used and it's NULL,
				NoOfRptBlklnstances is -1. So, no need to check
				range because command callback will detect this.
			 */
			if (NoOfRptBlklnstances < 0)
			  break;

			if (((Atp_NumType)PDE_ptr->Min >= 0) && ((Atp_NumType)PDE_ptr->Max >= 0))
			{
			  if ((NoOfRptBlklnstances < (Atp_NumType)PDE_ptr->Min) ||
				  (NoOfRptBlklnstances > (Atp_NumType)PDE_ptr->Max))
			  {
				OutOfRange = TRUE;
				error_msg = Atp_MakeErrorMsg(ERRLOC,
											 ATP_ERRCODE_RPTBLK_INSTANCE_OUT_OF_RANGE,
											 NoOfRptBlklnstances, PDE_ptr->Name,
											 (Atp_NumType)PDE_ptr->Min,
											 (Atp_NumType)PDE_ptr->Max);
			  }
			}
			break;
		}

		default: break;

	} /* switch */

	/* Get return error message pointer. */
	ErrorMsgPtr = va_arg(argPtr, char **);

	if (ErrorMsgPtr != NULL)
	  *ErrorMsgPtr = NULL;

	va_end(argPtr);

	if (OutOfRange == TRUE)
	{
	  if (ErrorMsgPtr != NULL) {
	    *ErrorMsgPtr = error_msg;
	  }

	  /*
			If parameter is out of range and it is a default value,
			then there is a bug in the application.
	   */
	  if (isUserValue == FALSE)
	  {
	    char *bug_str;

	    /*
			Make another error message to explain that default
			value is out of range because it's a bug.
	     */
	    bug_str = Atp_MakeErrorMsg(ERRLOC, ATP_ERRCODE_DEFAULT_VALUE_OUT_OF_RANGE);

	    if (ErrorMsgPtr != NULL)
	    {
		  *ErrorMsgPtr = (char *)REALLOC(error_msg,
										 strlen(error_msg) +
										 strlen(bug_str) + 5,
										 NULL);

		  (void) strcat(*ErrorMsgPtr, "\n");
		  (void) strcat(*ErrorMsgPtr, bug_str);
		  (void) strcat(*ErrorMsgPtr, "\n");

		  FREE(bug_str);
	    }
	    else
	    {
		  (void) fprintf(stderr, "%s\n", error_msg);
		  (void) fprintf(stderr, "%s\n", bug_str);
		  (void) fflush(stderr);

		  FREE(error_msg);
		  FREE(bug_str);
	    }
	  }

	  return ATP_ERROR;
	}

	return ATP_OK;
}

/*+******************************************************************

	Function Name:		Atp_InvokeVproc

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Invoke vproc to check parameter value.

	Parameters:			parmdef entry, pointer to parameter value,
						boolean is-user-value indicator, error
						message return pointer.

	Global Variables:	none

	Results:			Atp_Result

	Calls:				vproc (if not NULL)

	Called by:			parameter and construct processors

	Side effects:		vproc error	message	is duplicated
						dynamically and should be freed before the
						next command

	Notes:				be aware that it's a pointer to the
						parameter value, so it could be a pointer to
						a pointer that the vproc is receiving

	Modifications:
		Who			When				Description
	----------	--------------	---------------------------
	Alwyn Teh	26 July 1992	Initial	Creation

*******************************************************************_*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_InvokeVproc
(
	ParmDefEntry *PDE_ptr,
	void *valuePtr,
	int isUserValue,
	char **ErrorMsgPtr
)
#else
Atp_Result
Atp_InvokeVproc(PDE_ptr, valuePtr, isUserValue, ErrorMsgPtr)
	ParmDefEntry *PDE_ptr;
	void *valuePtr;
	int isUserValue;
	char **ErrorMsgPtr;
#endif
{
	char *vprocRtnStr = NULL;

	/* If verification procedure is supplied, invoke it. */
	if (PDE_ptr->vproc != NULL) {
	  /*
			Vproc expects pointer to parameter and Atp_BoolType
			default flag.
	   */
	  vprocRtnStr = (*PDE_ptr->vproc)(valuePtr, (Atp_BoolType)isUserValue);
	}

	if (ErrorMsgPtr != NULL)
	  *ErrorMsgPtr = NULL;

	/*
		If vproc didn't like the value, it would have returned an
		error message.
	 */
	if ((vprocRtnStr != NULL) && (ErrorMsgPtr != NULL)) {
	  char *error_msg;

	  error_msg = (char *)MALLOC(strlen(vprocRtnStr) + 1, NULL);
	  *ErrorMsgPtr = strcpy(error_msg, vprocRtnStr);
	}

	if (vprocRtnStr != NULL) {
	  return ATP_ERROR;
	}

	return ATP_OK;
}
