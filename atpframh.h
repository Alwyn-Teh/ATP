/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+***********************************************************************************

	Module Name:		atpframh.h

	Copyright:			BNR Europe Limited, 1992, 1993, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains definitions for the
						function arguments frame relay mechanism
						used in ATP.

						This enables a caller function A calling
						function B to specify a trailing second list
						of arguments to be relayed to function C.

						By agreement, this list may contain a
						pointer to function C to be used by function B.
						Thus, the first called function B does
						not know what it is calling next, and what
						arguments it is relaying.

						The purpose of this method is so that a
						common	general-purpose	layer of functionality,
						such as	interactive prompting, may be inserted
						between a parameter processor and its syntax parser.
						The advantage is that the respective parser modules
						remain independent and isolated from this middle layer,
						which decides whether input is coming from the
						command line or the user interactively.
						Also, maintenance to this layer does not disturb
						the parsers' code.

	Usage:				When receiving arguments in called function, use
						varargs to obtain pointer to arguments. Then,
						duplicate stack frame using Atp_CopyCallFrame().
						Call the next function using ATP_FRAME_RELAY().
						The called function receives the arguments intact in
						the usual manner, without the need to use
						varargs to extract them.

	See Also:			atpframe.c

***********************************************************************************-*/

#ifndef _ATP_FRAME_RELAY_HEADER_INCLUDED
#define _ATP_FRAME_RELAY_HEADER_INCLUDED

#if defined(__STDC__) || defined(__cplusplus)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

/*
---------------------------------------------------------------------

	INTERNAL "FLexible Interface for Portability-Programming" (FLIP)

		FLIP uses "Function Arguments Relay Technique" (FART) to
		create transparent function layers which relay function
		pointers AND variable argument lists.

---------------------------------------------------------------------
*/

/*
	NOTE: If the maximum number of arguments being passed is increased
		  beyond the limit ATP_DEFAOLT_MAX_ARGS, just simply change
		  its value to suit the requirements and then alter
		  ATP_FRAME_RELAY() appropriately.
*/

#define ATP_DEFAULT_MAX_ARGS	16

/*
	FRAME RELAY macro:

		Use ATP_FRAME_RELAY(frame) as an argument when calling a function.

		Systems tested for portability are:
			HP9000 Series 300,400,700; HP-UX v7.05, v8.00, v8.07, v9.01
			SPARC w/s; SunOS Release 4.1.1 and 4.1.2
*/

#define ATP_FRAME_RELAY(frame)	frame.stack[0], frame.stack[1], \
								frame.stack[2], frame.stack[3], \
								frame.stack[4], frame.stack[5], \
								frame.stack[6], frame.stack[7], \
								frame.stack[8], frame.stack[9], \
								frame.stack[10], frame.stack[11], \
								frame.stack[12], frame.stack[13], \
								frame.stack[14], frame.stack[15]

#ifdef _ATP_CMD_CALLBACK_FIRST_FIXED_ARG_IS_INT_
#define ATP_FRAME_ELEM_TYPE int
#else
#define ATP_FRAME_ELEM_TYPE void *
#endif

typedef struct
{
		ATP_FRAME_ELEM_TYPE stack[ATP_DEFAULT_MAX_ARGS+1];
} Atp_CallFrame;

#define ATP_FRAME_ELEM_CALL_ARGS \
				ATP_FRAME_ELEM_TYPE,ATP_FRAME_ELEM_TYPE, \
				ATP_FRAME_ELEM_TYPE,ATP_FRAME_ELEM_TYPE, \
				ATP_FRAME_ELEM_TYPE,ATP_FRAME_ELEM_TYPE, \
				ATP_FRAME_ELEM_TYPE,ATP_FRAME_ELEM_TYPE, \
				ATP_FRAME_ELEM_TYPE,ATP_FRAME_ELEM_TYPE, \
				ATP_FRAME_ELEM_TYPE,ATP_FRAME_ELEM_TYPE, \
				ATP_FRAME_ELEM_TYPE,ATP_FRAME_ELEM_TYPE, \
				ATP_FRAME_ELEM_TYPE,ATP_FRAME_ELEM_TYPE

typedef int (*Atp_ProcUsingFrameRelayReturningInt)
				_PROTO_((ATP_FRAME_ELEM_CALL_ARGS));

EXTERN void Atp_CopyCallFrame
				_PROTO_((Atp_CallFrame *framePtr, va_list argsPtr));

EXTERN Atp_Result Atp_CopyCallFrameElems
				_PROTO_((ATP_FRAME_ELEM_TYPE *frameElemPtr,
						va_list argsPtr, int position));

#endif /* ATP FRAME RELAY HEADER INCLUDED */
