/* EDITION AC05 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atperror.c

	Copyright:			BNR Europe Limited, 1992 - 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module provides error handling routines.

	Modifications:
		Who			When			Description
	----------	--------------	--------------------------------
	Alwyn Teh	7 July 1992		Initial Creation
	Alwyn Teh	7 June 1993		Global change from
								BuiIdErrorMsg{) and
								VPrintfErrorMsgFunc()
								to Atp_AdvPrintf().
	Alwyn Teh	3 January 1994	Code inspection changes -
								error messages style.
	Alwyn Teh	9 August 1994	Wrap and indent long lines
								using Atp_PrintfWordWrap().
	Alwyn Teh	8 March 1995	Use stdarg.h instead of
								varargs.h for ANSI compliance.
	Alwyn Teh	27 March 1995	Implement BCD digits parameter.
	Alwyn Teh	27 March 1995	Convert %d to %d, and %lu to %u
								since we changed long to int,
								Atp_UnsNumType to Atp_NumType.

*******************************************************************-*/

#include <stdio.h>
// #include <malloc.h>
#include <string.h>
#include <setjmp.h>

#if defined(__STDC__) || defined(__cplusplus)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

#include "atph.h"
#include "atpsysex.h"
#include "atpframh.h"

#ifdef DEBUG
static	char	*__Atp_Local_FileName__ = __FILE__;
#endif

/* Forward declaration */
static void VPrintErrorDescRec _PROTO_((Atp_ErrorDescRecord err_rec,
										va_list args ));

/* Global variables */
Atp_BoolType	Atp_PrintErrLocnFlag = FALSE;

#ifndef _ATP_USE_XSTR
char	Atp_User_Error_Str[]	= "USER ERROR";
char	Atp_System_Error_Str[]	= "SYSTEM ERROR";
#endif

/* Local variables */
static char *HyperSpaceMsg	= NULL;

/* Local functions */
static char * Atp_TypeName _PROTO_(( Atp_ParmCode parmcode ));

/*
 *	Static initialization of central error message descriptions. Not
 *	all fields are used currently, they are stubs for the eventual
 *	implementation of interactive prompting in which the end-user is
 *	prompted with aids to enter correct data.
 */
static Atp_ErrorDescRecord CentralErrorDescStore[] = {

{	/* error_code */			ATP_ERRCODE_BAD_INPUT,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			ATP_ERRMSG_BAD_INPUT,
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_COMMAND_FAILED,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			ATP_ERRMSG_CMD_FAILURE,
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggesticm	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_CMD_ABORTED,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			ATP_ERRMSG_CMD_ABORTED,
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_CMD_BAD_RC,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			ATP_ERRMSG_CMD_BAD_RC,
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_NO_PARMDEF_OUTER_BEGIN_END,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"(Command: %s) First parmdef entry not a BEGIN construct and/or last entry not an END construct.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_PARMDEF_CONSTRUCT_MATCH_ERROR,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"(Command: %s) Missing or mismatched or unmatched parmdef BEGIN and END constructs detected.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_EXPECTED_PARM_NOT_FOUND,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location	*/		NULL,
	/* errmsg_fmtstr */			"Expected %s \"%s\" not found.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_RETURN_VALUE_POINTER_ABSENT,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Pointer for return %s value not supplied.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_UNRECOGNISED_PARM_VALUE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location	*/		NULL,
	/* errmsg_fmtstr */			"Unrecognised %s value \"%s\".",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_KEYWD_TAB_ABSENT,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Keyword table unavailable for lookup.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_WRONG_PARMCODE_FOR_PARSER,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Wrong parameter code (%s) supplied for %s parser.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_MINUS_SIGN_FOR_UNSNUM,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Minus sign for expected unsigned number not accepted: \"%s\".",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_NUMSIGNS_CONFLICT,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Ambiguous sign for number: \"%s\".",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_MINUS_SIGN_NOT_ALLOWED_FOR_NUMBASE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Minus sign not allowed for %s number: \"%s\".",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_INVALID_NUMBER,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Invalid %s number \"%s\"",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/*	error_code	*/			ATP_ERRCODE_NUM_OUT_OF_RANGE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"%s \"%s\" out of range.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_NUM_OVERFLOW,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Overflow of %d-bit %s, %s (%s) greater than maximum %s.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_NUM_UNDERFLOW,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errrasg_fmtstr */		"Underflow of %d-bit %s, %s (%s) less than minimum %s.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_INVALID_HEX_DATABYTES,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Invalid hexadecimal digit(s): \"%s\"",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/*	error_code	*/			ATP_ERRCODE_INVALID_BCD_DIGITS,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Invalid 4-bit BCD (Binary Coded Decimal) digit(s): \"%s\".",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			"Use digits only from the range 0123456789ABCDEF."},

{	/* error_code	*/			ATP_ERRCODE_EMPTY_DATABYTES_STRING,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Empty hexadecimal databytes string not allowed.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_EMPTY_BCD_DIGITS,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Empty binary coded decimal (BCD) string not allowed.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_SIGNED_NUM_OUT_OF_RANGE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"%d out of range for number \"%s\" - must be between %d and %d.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_UNSIGNED_NUM_OUT_OF_RANGE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"%u out of range for number \"%s\" - must be between %u and %u.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_REAL_NUM_OUT_OF_RANGE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"%l.l6e out of range for number \"%s\" - must be between %l.l6e and %l.l6e.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_STRING_OUT_OF_RANGE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location	*/		NULL,
	/* errmsg_fmtstr */			"\"%s\" (length %d) for string \"%s\" must be between %d and %d characters long.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_DATABYTES_LENGTH_OUT_OF_RANGE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location	*/		NULL,
	/* errmsg_fmtstr */			"Data for \"%s\" (%d bytes) out of range - must be between %d and %d bytes long.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_BCD_DIGITS_LENGTH_OUT_OF_RANGE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"BCD digits for \"%s\" (%d digits) out of range - must be between %d and %d digits long.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_RPTBLK_INSTANCE_OUT_OF_RANGE,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Number of instances (%d) of repeat block \"%s\" out of range - must be between %d and %d.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_DEFAULT_VALUE_OUT_OF_RANGE,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Default value out of range.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_NON_CONSTRUCT_PARMCODE,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location	*/		NULL,
	/* errmsg_fmtstr */			"Non-construct parameter code encountered in store construct function.",
	/* user_attention	*/		NULL,
	/* system_status */			"EXIT PROGRAM",
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_ILLEGAL_FREE_NULL_CTRLSTORE,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location	*/		NULL,
	/* errmsg_fmtstr */			"Attempt to release NULL parameter control store!",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location	*/		NULL,
	/* errmsg_fmtstr */			"Parameter \"%s\" not found - %s.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_MALLOC_RETURNS_NULL,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Insufficient memory available (%u bytes requested) or memory corruption detected.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_NO_PARMS_REQUIRED,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location	*/		NULL,
	/* errmsg_fmtstr */			"Parameter(s) after command \"%s\" not required.",
	/* user_attention	*/		"Command ignored.",
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_EXPECTED_PARMS_NOT_FOUND,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Expected parameter(s) for command \"%s\" not found.",
	/* user_attention	*/		"Command ignored.",
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_EXTRA_PARMS_NOT_WANTED,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Extra parameter(s) for command \"%s\" following \"%s\" (argument #%d) not wanted.",
	/* user_attention	*/		"Command ignored.",
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_PARSER_MISSING,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"The %s parser is missing!",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_KEYWORD_NOT_RECOGNISED,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Keyword \"%s\" not recognised.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code */			ATP_ERRCODE_DEFAULT_KEYWORD_NOT_FOUND,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Default keyword \"%s\" (value %d) not found in keyword table!",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_RPTBLK_MARKER_MISSING,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Repeat block marker \'%c\' missing.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_RPTBLK_MARKER_OR_NXT_PARM_MISSING,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Repeat block marker \'%c\' or next parameter missing.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL},

{	/* error_code	*/			ATP_ERRCODE_INVALID_RPTBLK_MARKER,
	/* whos_fault */			ATP_ERRMSG_USER_ERROR,
	/* error_location */		NULL,
	/* errmsg_fratstr */		"Invalid repeat block marker \"%s\"",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			"Please separate '(' or ')' from other characters."},

{	/* error_code */			ATP_ERRCODE_NESTCMD_DEPTH_EXCEEDED,
	/* whos_fault */			ATP_ERRMSG_SYS_ERROR,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			"Command aborted - exceeded nesting limit of %d.",
	/* user_attention */		NULL,
	/* system_status */			NULL,
	/* recovery_suggestion	*/	NULL,
	/* user_action */			NULL},

{	/* error_code */			0,
	/* whos_fault */			NULL,
	/* error_location */		NULL,
	/* errmsg_fmtstr */			NULL,
	/* user_attention */		NULL,
	/* systera_status */		NULL,
	/* recovery_suggestion */	NULL,
	/* user_action */			NULL}
};

static int ErrorMsgStoreNelems = (sizeof(CentralErrorDescStore)/sizeof(Atp_ErrorDescRecord));

/*+*******************************************************************

	Function Name:		Atp_MakeErrorMsg

	Copyright:			BNR Europe Limited, 1992,	1993
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Makes an error message using portable
						vsprintf using varargs with format string.

						Caller must free error string after use.

	Parameters:			ERRLOC, error message code and printf arguments

	Modifications:
		Who			When					Description
	-----------	--------------	------------------------------------
	Alwyn Teh	25 July 1992	Initial Creation
	Alwyn Teh	7 June 1993		Use Atp_DvsPrintf & Atp_AdvPrintf().
	Alwyn Teh	21 July 1993	Display filename only in debug mode.
								It does not look good for an application
								to output filename and line number in
								an error message.

*-*******************************************************************/
#if defined(__STDC__) || defined(__cplusplus)
char * Atp_MakeErrorMsg(char *file_name, int line_number, int error_code, ...)
#else
char * Atp_MakeErrorMsg(file_name, line_number, error_code, va_alist)
	char *file_name;
	int line_number;
	int error_code;
	va_dcl
#endif
{
	/*
		filename and line_number used to find error in case of
		program bug (see ERRLOC in atpsysex.h).
	*/
	va_list				args;
	Atp_ErrorDescRecord	error_record;
	Atp_BoolType		err_rec_found = FALSE;
	char				*err_msg = NULL;
	int					x;

#if defined(__STDC__) || defined(__cplusplus)
	va_start(args, error_code);
#else
	va_start(args);
#endif

	/* Find error description record. */
	for (x = 0; x < ErrorMsgStoreNelems; x++) {
	   if (error_code == CentralErrorDescStore[x].error_code) {
	     /* Make local copy of record to use. */
	     error_record = CentralErrorDescStore[x];
	     err_rec_found = TRUE;
	     break;
	   }
	}

	/*
		If the error code is invalid, it's an ATP internal fault
		which must be corrected.
	*/
	if (err_rec_found == FALSE) {
#ifdef DEBUG
	  char *fmtstr = "%s: line %d\n*** PROGRAM ERROR *** Invalid error code %d\n";
	  Atp_DvsPrintf(&err_msg, fmtstr, file_name, line_number, error_code);
#else
	  char *fmtstr = "*** PROGRAM ERROR *** Invalid error code %d\n";
	  Atp_DvsPrintf(&err_msg, fmtstr, error_code);
#endif
	  return err_msg;
	}

	if (Atp_PrintErrLocnFlag && file_name != NULL) {
	  Atp_AdvPrintf("%s: line %d\n", file_name, line_number);
	}

	/*
		Print out the error message description record including
		the formatted error message.
	*/
	VPrintErrorDescRec(error_record, args);

	/* Get error message. */
	err_msg = Atp_AdvGets();

	if (err_msg == NULL) {
	  (void) fprintf(stderr, "Cannot allocate memory for error message.\n");
	  if (Atp_PrintErrLocnFlag && file_name != NULL)
	  (void) fprintf(stderr, "%s: line %d\n", file_name, line_number);
	  (void) fprintf(stderr, error_record.errmsg_fmtstr, args);
	  (void) fprintf(stderr, "\n");
	  (void) fflush(stderr);
	  return NULL;
	}

	va_end(args);

	Atp_PrintErrLocnFlag = FALSE;

	return err_msg;
}

/*+*******************************************************************

	Function Name:		VPrintErrorDescRec

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Prints error message fields used.

	Modifications:
		Who			When			Description
	----------	--------------	--------------------------
	Alwyn Teh	25 July 1992	Initial Creation
	Alwyn Teh	7 June 1993		Use Atp_AdvPrintf().
	Alwyn Teh	9 August 1994	Wrap and indent long lines
								using Atp_PrintfWordWrap().

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void VPrintErrorDescRec( Atp_ErrorDescRecord err_rec, va_list args )
#else
static void
VPrintErrorDescRec(err_rec, args)
	Atp_ErrorDescRecord err_rec;
	va_list	args;
#endif
{
	Atp_CallFrame	frame;
	int				start_column = 7; /* assume after printing "Error:" */

	if (err_rec.whos_fault != NULL) {
	  Atp_AdvPrintf(" [%s] ", err_rec.whos_fault);
	  start_column += (strlen(err_rec.whos_fault) + 4);
	}

	if (err_rec.error_location != NULL) {
	  Atp_AdvPrintf("(%s) ", err_rec.error_location);
	  start_column += (strlen(err_rec.error_location) + 3);
	}

	if (err_rec.errmsg_fmtstr != NULL) {
	  Atp_CopyCallFrame(&frame, args);
	  Atp_PrintfWordWrap(Atp_AdvPrintf,
						 -1, start_column, 8, /* tab position */
						 err_rec.errmsg_fmtstr, ATP_FRAME_RELAY(frame));
	}

	Atp_AdvPrintf("\n");

	if (err_rec.user_attention != NULL)
	  Atp_PrintfWordWrap(Atp_AdvPrintf, -1, 1, 8,
			  	  	  	 "\t%s\n", err_rec.user_attention) ;

	if (err_rec.system_status != NULL)
	  Atp_PrintfWordWrap(Atp_AdvPrintf, -1, 1, 8,
			  	  	  	 "\t%s\n", err_rec.system_status);

	if (err_rec.recovery_suggestion != NULL)
	  Atp_PrintfWordWrap(Atp_AdvPrintf, -1, 1, 8,
			  	  	  	 "\t%s\n", err_rec.recovery_suggestion);

	if (err_rec.user_action != NULL)
	  Atp_PrintfWordWrap(Atp_AdvPrintf, -1, 1, 8,
			  	  	  	 "\t%s\n", err_rec.user_action);

	return;
}

/*+******************************************************************

	Function Name:		Atp_ShowErrorLocation

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Function to indicate that	error location
						is to be shown in the error message.

	Modifications:
		Who			When			Description
	----------	-------------	----------------------
	Alwyn Teh	25 July 1992	Initial Creation

*******************************************************************-*/
void
Atp_ShowErrorLocation()
{
	Atp_PrintErrLocnFlag = TRUE;
}

/*+********************************************************************

	Function Name:		Atp_RetrieveParmError

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Handles parameter retrieval	error, primarily
						application bugs.

						The application code does not have to call
						this function, it's "automatic".

						e.g. undefined parameter name, parameter
							 not supplied/found.

						Performs longjmp to last saved jmpbuf by
						calling Atp_HyperSpace.

	Modifications:
		Who			When				Description
	----------	--------------	--------------------------
	Alwyn Teh	27 July 1992 Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_RetrieveParmError
(
	int		error_code,
	char	*parm_retrieval_func_name,
	char	*parm_name,
	char	*filename,
	int		line_number
)
#else
void
Atp_RetrieveParmError(error_code,
					  parm_retrieval_func_name, parm_name,
					  filename, line_number)
	int		error_code;
	char	*parm_retrieval_func_name, *parm_name;
	char	*filename;
	int		line_number;
#endif
{
	char	*errmsg;

	Atp_ShowErrorLocation();

	errmsg = Atp_MakeErrorMsg(filename, line_number, error_code,
							  parm_name, parm_retrieval_func_name);

	Atp_HyperSpace(errmsg);
}

/*+*******************************************************************

	Function Name:		Atp_HyperSpace

	Copyright:			BNR Europe Limited,	1992, 1993
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Performs longjmp to current	last saved
						jmpbuf whilst securing the error message.

	Modifications:
		Who			When			Description
	----------	---------------	--------------------------
	Alwyn Teh	9 October 1992	Initial Creation
	Alwyn Teh	28 June 1993	Check error_msg != NULL

********************************************************************-*/
#if defined (__STDC__) || defined (_cplusplus)
void Atp_HyperSpace( char *error_msg )
#else
void
Atp_HyperSpace(error_msg)
	char * error_msg;
#endif
{
	HyperSpaceMsg = error_msg;

	if (Atp_JmpBufEnvPtr != NULL) {
	  longjmp(*Atp_JmpBufEnvPtr, ATP_HYPERSPACE_CODE);
	}
	else {
	  if (error_msg != NULL) {
		(void) fprintf(stderr, "%s\n", error_msg);
		(void) fflush(stderr);
		FREE(error_msg);
	  }
	}
}

/*+*******************************************************************

	Function Name:			Atp_GetHyperSpaceMsg

	Copyright:				BNR Europe Limited, 1992
							Bell-Northern Research
							Northern Telecom / Nortel

	Description:			Retrieves error message after call to
							Atp_HyperSpace.

	Modifications:
		Who			When			Description
	----------	---------------	-----------------------
	Alwyn Teh	9 October 1992	Initial	Creation

*******************************************************************-*/
char * Atp_GetHyperSpaceMsg()
{
	return HyperSpaceMsg;
}

/*+*******************************************************************

	Function Name:		Atp_INTERNAL_AppendParmName

	Copyright:			BNR Europe Limited,	1993
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Provides mechanism to append parameter or
						construct name to an error message as nested
						parser functions unwrap. Helps locate
						faulty parameter or construct should it be
						deeply nested.

	Modifications:
		Who			When			Description
	----------	-------------	------------------------
	Alwyn Teh	9 June 1993		Initial Creation

★it*****************************************************************.*/
#if defined(__STDC__) || defined (__cplusplus)
void Atp_INTERNAL_AppendParmName
(
	Atp_ParserStateRec *parseRec,
	char **errmsg_ptr
)
#else
void
Atp_INTERNAL_AppendParmName(parseRec, errmsg_ptr)
	Atp_ParserStateRec *parseRec;
	char **errmsg_ptr;
#endif
{
	char *tmp, *str, *fmtstr;

	if (errmsg_ptr == NULL)
	  return;

	/* Ensure newline at end of error message. */
	if ((*errmsg_ptr)[strlen(*errmsg_ptr)-1] == '\n')
	  fmtstr = "%s\t%s: %s - %s\n";
	else
	  fmtstr = "%s\n\t%s: %s - %s\n";

	/* Append parameter name and description to error message. */
	/* Also, say whether it is a construct or a parameter. */
	(void)
	Atp_DvsPrintf(&str, fmtstr, *errmsg_ptr,
				  Atp_TypeName(Atp_ParseRecParmDefEntry(parseRec).parmcode),
				  Atp_ParseRecParmDefEntry(parseRec).Name,
				  Atp_ParseRecParmDefEntry(parseRec).Desc);

	/* Free the original error message since new one has been made. */
	tmp = *errmsg_ptr;
	if (tmp != NULL) FREE(tmp);

	/* Return new error string. */
	*errmsg_ptr = str;
}

/*+********************************************************************

	Function Name:		Atp_TypeName

	Copyright:			BNR Europe Limited,	1993
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Simple function for providing English
						description of parameter and construct types
						for error messages.

	Modifications:
		Who			When			Description
	----------	------------	-----------------------
	Alwyn Teh	9 June 1993		Initial Creation

*********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static char * Atp_TypeName( Atp_ParmCode parmcode )
#else
static char *
Atp_TypeName(parmcode)
	Atp_ParmCode parmcode;
#endif
{
	switch (Atp_PARMCODE(parmcode)) {
		case ATP_BPM:		return("Parameter definition table");
		case ATP_EPM:		return("End of	ParmDef Table");
		case ATP_BLS:		return("List");
		case ATP_ELS:		return("End of	List");
		case ATP_BRP:		return("Repeat	Block");
		case ATP_ERP:		return("End of Repeat Block");
		case ATP_BCH:		return("Choice");
		case ATP_ECH:		return("End of Choice");
		case ATP_BCS:		return("Case Label");
		case ATP_ECS:		return("End of Case Block");
		case ATP_NUM:		return("Number");
		case ATP_UNS_NUM:	return("Unsigned Number");
		case ATP_REAL:		return("Real Number");
		case ATP_STR:		return("String");
		case ATP_BOOL:		return("Boolean");
		case ATP_DATA:		return("Data Bytes");
		case ATP_KEYS:		return("Keyword");
		case ATP_NULL:		return("Null");
		case ATP_COM:		return("Common Parameter");
		default:			return("Parameter");
	}
}
