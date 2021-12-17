/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpcmd.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northem Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains functions	relating to
						ATP commands.

*******************************************************************-*/

#include <string.h>
#include <setjmp.h>

#if defined(__STDC__) || defined (__cplusplus)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

#include "atph.h"
#include "atpsysex.h"
#include "atpframh.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/* Global declarations */
void (*Atp_ReturnDynamicStringToAdaptor) _PROTO_((char *s, ...)) = NULL;
char **Atp_HiddenCommands = NULL;

/*+*******************************************************************

	Function Name:		Atp_ExecuteCallback

	Copyright:			BNR Europe Limited, 1992, 1994
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		This function administers the	execution of
						the callback function of a command. It
						allows the use of nested commands up to a
						certain limit, and maintains the parmstore
						for the current command for parameter
						retrieval.

	Modifications:
		Who			When				Description
	----------	---------------	------------------------------
	Alwyn Teh	13 July 1992	Initial Creation
	Alwyn Teh	18 January 1994	Should pop parmstore before
								freeing. Ensure parmstore is
								always freed after use.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_ExecuteCallback
(
	Atp_CmdCallBackType callBack,
	void *parmstore,
	...
)
#else
int Atp_ExecuteCallback(callBack, parmstore, va_alist)
	Atp_CmdCallBackType callBack;
	void *parmstore;
	va_dcl
#endif
{
	static int CmdNestLevel = -1;

	Atp_CallFrame	interface;
	va_list			ap;

	char	*errmsg	= NULL;
	int		result	= 0;

	/* Obtain variable arguments list from stack frame. */
#if defined(__STDC__) || defined(__cplusplus)
	va_start(ap, parmstore);
#else
	va_start(ap);
#endif
	Atp_CopyCallFrame(&interface, ap);
	va_end(ap);

	if (callBack != NULL) {
		/*
			 Check if command nesting has exceeded limit which could
			 imply infinite loop.
		 */
		if (++CmdNestLevel > ATP_MAX_NESTCMD_DEPTH - 1) {
			errmsg = Atp_MakeErrorMsg(ERRLOC,
									  ATP_ERRCODE_NESTCMD_DEPTH_EXCEEDED,
									  ATP_MAX_NESTCMD_DEPTH);

			result = Atp_Adaptor_FrontEnd_ReturnCode_ERROR;

			Atp_ReturnDynamicStringToAdaptor(errmsg, ATP_FRAME_RELAY(interface));
		}
		else
		if (setjmp(*Atp_JmpBufEnvPtr) == 0) {
			/*
				 NOTE: Parmstore may be NULL if command has NULL or empty parmdef
			 */
			Atp_PushParmStorePtrOnStack(parmstore);

			result = (*callBack)(ATP_FRAME_RELAY(interface));

			Atp_PopParmStorePtrFromStack(parmstore);
		}
		else {
			/*
				 Execution process of command was aborted, longjmp() gets
				 you here eventually.
			 */
			result = Atp_Adaptor_FrontEnd_ReturnCode_ERROR;

			Atp_PopParmStorePtrFromStack(parmstore);

			errmsg = Atp_GetHyperSpaceMsg();

			Atp_ReturnDynamicStringToAdaptor(errmsg,ATP_FRAME_RELAY(interface));
		}
		CmdNestLevel--;
	}

	/* Free parmstore after use. */
	Atp_FreeParmStore(parmstore);
	parmstore = NULL;

	return result;
}

/*+*****************************************************************

	Function Name:		Atp_IsHiddenCommandName

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Checks to see if command is supposed to be
						hidden from the end-user.

	Modifications:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	26 June 1993	Initial	Creation

**********************************************»*******************-*/
#if defined(__STDC__) || defined (__cplusplus)
int Atp_IsHiddenCommandName( char *name )
#else
int Atp_IsHiddenCommandName(name)
	char *name;
#endif
{
	int x;

	if (name == NULL)
	  return 0;

	if (Atp_HiddenCommands == NULL)
	  return 0;

	for (x = 0; Atp_HiddenCommands[x] != NULL; x++)
	  if (Atp_Strcmp(Atp_HiddenCommands[x], name) == 0)
	    return 1;

	return 0;
}
