/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+******************************************»************************

	Module Name:		atpframe.c

	Copyright:			BNR Europe Limited, 1992, 1993, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module	contains the implementation of
						the Function Arguments frame Relay
						Technique.

	See Also:			atpframh.h

********************************************************************-*/

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

/*+********************************************************************

	Function Name:		Atp_CopyCallFrame
						Atp_CopyCa1lFrameE1ems

	Copyright:			BNR Europe Limited, 1992, 1993, 1995
						Bell-Northern Research
						Northern Telecom

	Description:		Use varargs to copy stack frame. This
						ensures a machine-independent implementation.
						On a . RISC architecture workstation, e.g.
						HP70.Q, indirect addressing is used.

	Parameters:			frame buffer for duplicating stack, and
						va_list pointer to stack frame to read from.

	Global Variables:	None

	Results:			void

	Calls:				va_arg

	Called by:			atpparse.c, atphelp.c, atphelpc.c, atpmanpg.c,
						atpcmd.c, atperror.c, atppage.c

	Side effects:		Only ATP_DEFAULT_MAX_ARGS of int's are
						copied, so may copy rubbish beyond arguments,
						or copy insufficient number of arguments, in
						which case increase the value of
						ATP_DEFAULT_MAX_ARGS.

	Modifications:
		Who				When				Description
	----------	------------------	------------------------------
	Alwyn Teh	2 October 1992		Initial Creation
	Alwyn Teh	9 July 1993			Improve portability by
									using va_arg() to traverse
									parameter list.
									(Ported to HP700)
	Alwyn Teh	9 March 1995		Added Atp_CopyCallFrameElems

*********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_CopyCallFrame( Atp_CallFrame *framePtr, va_list argsPtr )
#else
void
Atp_CopyCallFrame(framePtr, argsPtr)
	Atp_CallFrame	*framePtr;
	va_list			argsPtr;
#endif
{
	register int x;
	va_list ap2;

	va_copy(ap2, argsPtr);

	for (x = 0; x < ATP_DEFAULT_MAX_ARGS; x++)
	{
	   framePtr->stack[x] = va_arg(ap2, ATP_FRAME_ELEM_TYPE);
	}

	va_end(ap2);
}

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_CopyCallFrameElems(	ATP_FRAME_ELEM_TYPE *frameElemPtr,
									va_list argsPtr,
									int position )
#else
Atp_Result
Atp_CopyCallFrameElems(frameElemPtr, argsPtr, position)
	ATP_FRAME_ELEM_TYPE *frameElemPtr;
	va_list argsPtr;
	int position; /* of element supplied in range 0..ATP_DEFAULT_MAX_ARGS-1 */
#endif
{
	register int x;

	if (position > ATP_DEFAULT_MAX_ARGS-1)
	  return ATP_ERROR;

	for (x = 0; x < (ATP_DEFAULT_MAX_ARGS - position); x++)
	{
	   frameElemPtr [x] = va_arg(argsPtr, ATP_FRAME_ELEM_TYPE);
	}
	return ATP_OK;
}
