/* EDITION AC04 (REL002), ITD ACST.176 (95/06/19 18:46:08) -- CLOSED */

/*+********************************************************************

	Module Name:		atpnum.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation	of
						the number and unsigned number parameters.
						Retrieval functions support other numeric
						types too.

*********************************************************************-*/

#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#if defined (__STDC__) || defined (__cplusplus)
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
static Atp_Result	CheckNumberOverFlow _PROTO_((char *SrcStart,
												 char *SrcEnd,
												 Atp_ParmCode parmcode,
												 int numBase,
												 char **ErrorMsgPtr));

/*+***************************************************************

	Function Name:		Atp_ProcessNumParm

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Number parameter processor - handles input,
						default, range and vproc checking, and
						parameter storage.

	Modifications:
		Who			When					Description
	----------	----------------	----------------------------
	Alwyn Teh	30 July 1992		Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	9 June 1993			Add Atp_AppendParmName ()

****************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessNumParm( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessNumParm(parseRec)
	Atp_ParserStateRec	*parseRec;
#endif
{
	Atp_Result	result	= ATP_OK;
	Atp_NumType	number	= 0;
	char		*errmsg	= NULL;

	parseRec->ReturnStr = NULL;

	result = Atp_SelectInputAndParseParm(parseRec,
										 (Atp_ArgParserType)Atp_ParseNum,
										 "number", ATP_NUM,
										 &number, &errmsg);

	if (result == ATP_OK) {
	  if (parseRec->ValueUsedIsDefault)
		number = (Atp_NumType) parseRec->defaultValue;
	  else
		parseRec->CurrArgvIdx++; /* next token */
	}

	if (result == ATP_OK)
	  result = Atp_CheckRange(&Atp_ParseRecParmDefEntry(parseRec),
			  	  	  !(parseRec->ValueUsedIsDefault),number,&errmsg);

	if (result == ATP_OK)
	  result = Atp_InvokeVproc(	&Atp_ParseRecParmDefEntry(parseRec),
								&number,
								!(parseRec->ValueUsedIsDefault),
								&errmsg );

	if (result == ATP_OK)
	  Atp_StoreParm(&Atp_ParseRecParmDefEntry(parseRec),
			  	  	parseRec->ValueUsedIsDefault, number);

	if (result == ATP_ERROR) {
	  if ((parseRec->ReturnStr == NULL) && (errmsg != NULL)) {
		Atp_AppendParmName(parseRec, errmsg);
		parseRec->ReturnStr = errmsg;
	  }
	}

	parseRec->result = result;

	if (result == ATP_OK) {
	  if (!parseRec->RptBlkTerminatorSize_Known)
		parseRec->RptBlkTerminatorSize += sizeof(Atp_NumType);
	}

	return result;
}

/*+*******************************************************************

	Function Name:	Atp_ProcessUnsNumParm

	Copyright:	BNR Europe Limited, 1992, 1993
	Bell-Northern Research
	Northern Telecom

	Description:	Unsigned number parameter	processor
	handles input, default, range and vproc
	checking, and parameter storage.

	Modifications:
		Who			When					Description
	----------	----------------	----------------------------
	Alwyn Teh	30 July 1992		Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	9 June 1993			Add Atp_AppendParmName()

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessUnsNumParm( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessUnsNumParm(parseRec)
	Atp_ParserStateRec	*parseRec;
#endif
{
	Atp_Result		result	= ATP_OK;
	Atp_UnsNumType	unumber	= 0;
	char			*errmsg	= NULL;

	parseRec->ReturnStr = NULL;

	result = Atp_SelectInputAndParseParm(parseRec,
										 (Atp_ArgParserType)Atp_ParseNum,
										 "unsigned number", ATP_UNS_NUM,
										 &unumber, &errmsg);

	if (result == ATP_OK) {
	  if (parseRec->ValueUsedIsDefault)
		unumber = (Atp_UnsNumType) parseRec->defaultValue;
	  else
		parseRec->CurrArgvIdx++; /* next token */
	}

	if (result == ATP_OK)
	  result = Atp_CheckRange(&Atp_ParseRecParmDefEntry(parseRec),
							  !(parseRec->ValueUsedIsDefault), unumber,
							  &errmsg);

	if (result == ATP_OK)
	  result = Atp_InvokeVproc(&Atp_ParseRecParmDefEntry(parseRec),
							   &unumber,
							   !(parseRec->ValueUsedIsDefault),
							   &errmsg);

	if (result == ATP_OK)
	  Atp_StoreParm(&Atp_ParseRecParmDefEntry(parseRec),
			  	    parseRec->ValueUsedIsDefault, unumber);

	if (result == ATP_ERROR) {
	  if ((parseRec->ReturnStr == NULL) && (errmsg != NULL)) {
		Atp_AppendParmName(parseRec, errmsg);
		parseRec->ReturnStr = errmsg;
	  }
	}

	parseRec->result = result;

	if (result == ATP_OK) {
	  if (!parseRec->RptBlkTerminatorSize_Known)
		parseRec->RptBlkTerminatorSize += sizeof(Atp_UnsNumType);
	}

	return result;
}

/*+*******************************************************************

	Function Name:		Atp_ParseNum

	Copyright:			BNR Europe Limited, 1992, 1994
						Bell-Northern Research
						Northern Telecom

	Description:		Parses a long number signed or unsigned, and
						in your favourite notations. Gives you the
						value upon successful parse.

						Syntax:

							The '+' sign may preceed any number.

							The '+' sign may preceed the base part
							and/or number-part of a non-decimal number
							(i.e. Bin,B,Oct,0,Hex,H)

							The '-' sign can only preceed a decimal
							number.

							Decimal [+|-]n

							Octal [+] On | Oct[+]n | 0[+]n

							Hexadecimal [+] Oxn | [+] OXn | Hex[+]n | H[+]n

							Binary [+]bin[+]010 (for example)

							where n is a number with digits belonging
							to the corresponding base notation
							system.

							P.S. where letters are used, mixed case
							is supported.

	Modifications:
		Who			When					Description
	----------	--------------	------------------------------------
	Alwyn Teh	26 July 1992	Initial Creation
	Alwyn Teh	6 January 1994	Port to SunOS 4.1.2 on SPARCbook.
								Some unsigned number testcases are
								failing, strtoul does not seem to
								be supported on the SON. It has
								worked so far probably because it
								picked up Tel's implementation of
								strtoul.
	Alwyn Teh	25 January 1994	Remove unused variable argsize.
	Alwyn Teh	19 June 1995	Accept and ignore leading and
								trailing spaces. Makes tabulating
								certain numbers in command files
								easier and look prettier.
								e.g.	  LKJIHGFEDCBA (Msg indicators)
										  ------------
									{ bin			10 }
									{ bin		  01   }
									{ bin		01     }
									{ bin	   0       }
									{ bin 	  0        }
									{ bin	 1         }
									{ bin	1          }
									{ bin  0           }
									{ bin 1            }

********************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
Atp_Result Atp_ParseNum
(
	char			*input_src,
	Atp_ParmCode	parmcode,
	Atp_NumType		*numValPtr,
	char			**ErrorMsgPtr
)
#else
Atp_Result
Atp_ParseNum(input_src, parmcode, numValPtr, ErrorMsgPtr)
	char			*input_src;
	Atp_ParmCode	parmcode;	/* indicates whether number or unsigned number */
	Atp_NumType		*numValPtr; /* pointer to long, signed or unsigned */
	char			**ErrorMsgPtr;
#endif
{
	char		*src, *endPtr, *numStr,	*saveStr, numSign;
	int			base,
				baseIsHex, baseIsOctal,	baseIsBinary,
				OutOfRange;
	Atp_Result	result = ATP_OK;

	/*
		Since this parser accepts number or unsigned number,
		potentially a wrong parmcode could be supplied. Check
		that there is no error here.
	 */
	if ((parmcode != ATP_NUM) && (parmcode != ATP_UNS_NUM))
	{
	  if (ErrorMsgPtr != NULL)
	  {
		char *parm_arg_str = Atp_ParmTypeString(parmcode);

		Atp_ShowErrorLocation();
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_WRONG_PARMCODE_FOR_PARSER,
										parm_arg_str, "number");
	  }
	  // va_end(argPtr);
	  return ATP_ERROR;
	}

	if (ErrorMsgPtr != NULL) *ErrorMsgPtr = NULL;
	numStr = NULL;

	/* Check that return value pointer is supplied. */
	if (numValPtr == NULL)
	{
	  if (ErrorMsgPtr != NULL)
	  {
		Atp_ShowErrorLocation();
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_RETURN_VALUE_POINTER_ABSENT,
										(parmcode == ATP_UNS_NUM) ?
										"unsigned number" : "number");
	  }
	  // va_end(argPtr);
	  return ATP_ERROR;
	}

	/* Essential initialisations */
	base = baseIsHex = baseIsOctal = baseIsBinary = 0;
	src = NULL;

	/*
		Attempt to parse off a number, putting number string in src.
	 */
	result = Atp_ParseStr(input_src, &src, NULL,
						  (parmcode == ATP_UNS_NUM) ?
						  "unsigned number" : "number",
						  ErrorMsgPtr);

	if (result == ATP_ERROR) {
	  if (src != NULL)
		FREE(src);
	  	*(Atp_UnsNumType *)numValPtr = 0;
	  	// va_end(argPtr);
	  	return ATP_ERROR;
	}

	/* Point to beginning of number string. Used for error messages only. */
	saveStr = src;

	/* Trim off any leading and trailing white spaces. */
	{
	char *ptr = &src[strlen(src)-1];
	while (isspace(*src)) src++;
	while (isspace(*ptr)) ptr--;
	*(ptr+1) = '\0';
	}

	/* Point to beginning of number string. Used for error messages only. */
	numStr = src;

	/*
		Is number indicating explicitly whether it is positive or
		negative?
	 */
	numSign = ' '; /* initial value */
	if ((*src =='-') || (*src == '+')) numSign = *src++;

	/* Unsigned numbers can't be negative. */
	if ((parmcode == ATP_UNS_NUM) && (numSign == '-'))
	{
	  if (ErrorMsgPtr != NULL)
	  {
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_MINUS_SIGN_FOR_UNSNUM,
										numStr);
	  }
	  if (saveStr != NULL) FREE(saveStr);
	  *(Atp_UnsNumType *)numValPtr = 0;
	  // va_end(argPtr);
	  return ATP_ERROR;
	}

	/* Find out what base to use. */
	switch (*src) {
		/* Hexadecimal number */
		case 'H' : case 'h' : {
			if (Atp_Strncmp(src,"hex",3) == 0)
			  src += 3;
			else src += 1;
			base = baseIsHex = 16;
			break;
		}
		/* Binary number */
		case 'B' : case 'b' : {
			if (Atp_Strncmp(src,"bin",3) == 0)
			  src += 3;
			else src += 1;
			base = baseIsBinary = 2;
			break;
		}
		/* Octal number */
		case 'O' : case 'o' : {
			if (Atp_Strncmp(src,"oct",3) == 0)
			  src += 3;
			else src += 1;
			base = baseIsOctal = 8;
			break;
		}
		/*
			Default base is determined by the string itself. After
			an optional sign, a leading zero indicates octal
			conversion, and a leading "Ox" or "OX" hexadecimal
			conversion. Otherwise, decimal conversion is used.
		*/
		default : {
			base = 0; /* for indication to strtol() that
						 string itself determines base */
			/*
				For internal use, see if octal or hexadecimal base
				intended
			*/
			if (src[0] == 'O') {
			  baseIsOctal = 8; /* string is octal */
			  if (tolower(src[1]) == 'x')
				/* string is hex if followed by 'x' */
				baseIsHex = 16;
			}
			break;
		}
	} /* switch */

	/* Be strict, check sign again. */
	if ((*src =='-') || (*src == '+'))
	{
	  /*
			Someone may have typed conflicting signs '+' and '-'
			together.
	   */
	  if ((numSign != ' ') && (numSign != *src))
	  {
		if (ErrorMsgPtr != NULL)
		{
		  *ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										  ATP_ERRCODE_NUMSIGNS_CONFLICT,
										  numStr);
		}
		if (saveStr != NULL) FREE(saveStr);
		*(Atp_UnsNumType *)numValPtr = 0;
		// va_end(argPtr);
		return ATP_ERROR;
	  }
	  else
		numSign = *src; /* sign of number determined */
	}

	/*
		Negative sign not allowed for binary, octal and
		hexadecimal numbers.
	 */
	if ((numSign == '-') &&
		((base != 0) || baseIsBinary || baseIsOctal || baseIsHex))
	{
	  if (ErrorMsgPtr != NULL)
	  {
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_MINUS_SIGN_NOT_ALLOWED_FOR_NUMBASE,
										(baseIsHex) ?
										"hexadecimal " :
										(baseIsOctal) ?
										"octal " :
										(baseIsBinary) ?
										"binary " : "",
										numStr);
	  }
	  if (saveStr != NULL) FREE (saveStr);
	  *(Atp_UnsNumType *)numValPtr = 0;
	  // va_end(argPtr);
	  return ATP_ERROR;
	}

	/* Initialise more variables about to be used. */

	/*
		NOTE that errno does not get reset back
		to zero upon successful strtol() call.
	 */
	OutOfRange = errno = 0;

	/* Zero out number buffer, including signed bit. */
	*(Atp_UnsNumType *)numValPtr = 0;

	/* Set endPtr to start of number string. */
	endPtr = src;

	/*
		At long last, do the real thing:
		Read in the number or unsigned number,
		using strtol() or strtoul().
	*/
	if (parmcode == ATP_UNS_NUM) {
#if sun || sun2 || sun3 || sun4
	  *(Atp_UnsNumType *)numValPtr = (Atp_UnsNumType)strtol{src, &endPtr, base);
#else
	  /* strtoul() was introduced in HP-UX v7.05 */
	  /* Don't change this to strtol - it won't work. */
	  *(Atp_UnsNumType *)numValPtr = (Atp_UnsNumType)strtoul(src, &endPtr, base);
#endif
	}
	else {
	  if ((numSign == '-') && (src[-1] == '-')) {
		src--; /* backtrack so that strtol() reads in a negative number */
		endPtr = src;
	  }
	  *(Atp_NumType *)numValPtr = (Atp_NumType)strtol(src, &endPtr, base);
	}

	OutOfRange = errno; /* preserve errno value */

	/*
		Check if strtol() read in number successfully or not.
		Number string may contain illegal characters.
	 */
	if (((endPtr == src) && (*numValPtr == 0)) ||
		((endPtr != src) && (*endPtr != '\0')))
	{
	  if (ErrorMsgPtr != NULL)
	  {
		/*
			Set base properly, syntax could have been Onn or Oxnn
			with base = 0.
		 */
		if (base == 0) {
		  if (baseIsHex)
			base = 16;
		  else
		  if (baseIsOctal)
			base = 8;
		}

		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_INVALID_NUMBER,
										(baseIsHex) ?
										" hexadecimal" :
										(baseIsOctal) ?
										" octal" :
										(baseIsBinary) ?
										" binary" : "",
										numStr);
	  }

	  if (saveStr != NULL) FREE(saveStr);
	  *(Atp_UnsNumType *)numValPtr = 0;
	  // va_end(argPtr);
	  return ATP_ERROR;
	}

	/*
		Check overflow after strtol() has done the syntax
		checking !! You know by now a number has definitely been
		read in but you don’t know if it overflowed or
		underflowed.
	 */
	if (CheckNumberOverFlow(src, endPtr, parmcode, base, ErrorMsgPtr) == ATP_ERROR)
	{
	  /*
			NOTE: CheckNumberOverFlow() returns error message in
			ErrorMsgPtr if error encountered.
	   */
	  if (saveStr != NULL)
		FREE(saveStr);

	  *(Atp_UnsNumType *) numValPtr = 0;

	  // va_end(argPtr);

	  return ATP_ERROR;
	}
	else
	{
	  /* double-check errno (OutOfRange) just in case */
	  if (OutOfRange == ERANGE)
	  {
		if (ErrorMsgPtr != NULL)
		{
		  *ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC, ATP_ERRCODE_NUM_OUT_OF_RANGE,
										  (parmcode == ATP_NUM) ?
										  "Number" : "Unsigned number",
										  numStr);
		}

		if (saveStr != NULL)
		  FREE(saveStr);

		*(Atp_UnsNumType *)numValPtr = 0;

		// va_end(argPtr);

		return ATP_ERROR;
	  }
	}

	/* Everything ok, return. */
	if (saveStr != NULL)
	  FREE(saveStr);

	// va_end(argPtr);

	return ATP_OK;
}

/*+*********************************************************************

	Function Name:		CheckNumberOverFlow

	Copyright:			BNR Europe Limited,	1992
						Bell-Northern Research
						Northern Telecom

	Description:		Checks the number parameter string to see if
						there is any overflow or underflow.

						Can only cope with decimal, octal and
						hexadecimal notation.

						Both signed and unsigned numbers accepted.

						If over/under-flow, returns ATP_ERROR and
						error message, else returns ATP_OK.

	Dependencies:		Depends on strtol() having been called
						beforehand. Therefore, must ONLY be used in
						conjunction with Atp_ParseNum().

	Notes:				This function was developed for CLI running
						under HP-UX v6.5. Then, HP-UX v7.05
						introduced overflow checking. The system
						variable errno will return ERANGE if a
						number is out of range but there is no
						indication of the sign. So best thing is to
						preserve this function as it is much more
						helpful and user-friendly in its error
						messages. Hence, retained for use in ATP.

	Modifications:
		Who			When				Description
	----------	------------	-----------------------------
	Alwyn Teh	26 July 1992	Initial Creation

*********************************************************************-*/

/* Definitions and data structures used by CheckNumberOverFlow(). */
#define BINARY_NOTATION		0
#define OCTAL_NOTATION		1
#define DECIMAL_NOTATION	2
#define HEX_NOTATION		3

#define SIGNED_NUMBER		0
#define UNSIGNED_NUMBER		1

#define POSITIVE			1
#define NEGATIVE			-1

#define OVERFLOW_ERROR		123
#define UNDERFLOW_ERROR		321

/*
                         0        1         2         3         4
                         1234567890123456789012345678901234567890
 */
#define NUMBER_BUFFER12	"              "
#define NUMBER_BUFFER32	"                                  "

/*
	These are printf conversion strings used by CheckNutnberOverFlow()
	- do not change.
*/
/*
	notation \ sign | SIGNED | UNSIGNED
	          \     |   0    |    1
	----------------+--------+--------------
	BINARY  0       | NULL   |   NULL
	OCTAL   1       | "%o"   |   "%o"	(no leading 0)
	DECIMAL	2       | "%d"   |   "%u"
	HEX     3       | "%x"   |   "%x"	(no leading Ox)
*/

static char * FmtConv[4][2] = {
	{"",	""},
	{"%o",	"%o"},
	{"%d",	"%u"},
	{"%x",	"%x"}
};

/* Allocate space for maximum and minimum limits strings for numbers. */
static char * MinLimits[4][2] = {
	{NUMBER_BUFFER32, NUMBER_BUFFER32},
	{NUMBER_BUFFER12, NUMBER_BUFFER12},
	{NUMBER_BUFFER12, NUMBER_BUFFER12},
	{NUMBER_BUFFER12, NUMBER_BUFFER12}
};

static char * MaxLimits[4][2] = {
	{NUMBER_BUFFER32, NUMBER_BUFFER32},
	{NUMBER_BUFFER12, NUMBER_BUFFER12},
	{NUMBER_BUFFER12, NUMBER_BUFFER12},
	{NUMBER_BUFFER12, NUMBER_BUFFER12}
};

/*
Function CheckNumberOverFlow()
*/
#if defined(__STDC__) || defined(__cplusplus)
static Atp_Result CheckNumberOverFlow
(
	char			*SrcStart,
	char			*SrcEnd,
	Atp_ParmCode	parmcode,
	int				numBase,
	char			**ErrorMsgPtr
)
#else
static Atp_Result
CheckNumberOverFlow(SrcStart, SrcEnd, parmcode, numBase, ErrorMsgPtr)
	char			*SrcStart, *SrcEnd;
	Atp_ParmCode	parmcode;
	int				numBase;
	char			**ErrorMsgPtr;
#endif
{
	int				notationIdx,
					numberTypeIdx,
					number_sign,
					numLen,
					LimitLen,
					lexcmp_result;

	Atp_NumType		minLimit, maxLimit;
					/* for use with signed and unsigned number */

	char			*LimitString, *limitCmpStr;

	register char	*src = SrcStart;

	/* Initialisations */
	LimitString = limitCmpStr = "";
	if (ErrorMsgPtr != NULL)
	  *ErrorMsgPtr = NULL;

	if (*src == '-') {
	  number_sign = NEGATIVE;
	  src += 1; /* skip minus sign */
	}
	else {
	  /* Positive sign is implicit but may have been supplied. */
	  if (*src == '+')
	    src += 1; /* skip plus sign */
	  number_sign = POSITIVE;
	}

	/*
		Skip any notational prefixes if base 0 was used with
		strtol(). With other bases denotated by "HEX", "H",
		"OCT" or "O", these have been skipped over in
		Atp_ParseNum().
	*/
	if (numBase == 0)
	{
	  /* Skip 'Ox' or 'OX' or 'O' if any. */
	  if ((src[0] == '0') && ((src[1] == 'x') || (src[1] == 'X')))
	  {
		src += 2;
		numBase = 16;
	  }
	  else
	  if ((src[0] == '0') && (src[1] != '\0'))
	  {
		src += 1;
		numBase = 8;
	  }
	}

	/* MUST skip leading zeros after optional sign. */
	while (*src == '0') src++;

	/* If zero number, no point going any further. */
	if (*src == '\0') return ATP_OK;

	/* Get array index for notation. */
	switch(numBase)
	{
		 case 0 : notationIdx = DECIMAL_NOTATION;
		 	 	  break;
		 case 2 : notationIdx = BINARY_NOTATION;
		 	 	  break;
		 case 8 : notationIdx = OCTAL_NOTATION;
		 	 	  break;
		case 16 : notationIdx = HEX_NOTATION;
				  break;
		default : return ATP_OK; /* no checking available */
	}

	/*
		Get number type (signed or unsigned) index, and the
		minimum and maximum limits.
	*/
	switch(parmcode)
	{
		case ATP_NUM:
		{
			numberTypeIdx =	SIGNED_NUMBER;
			minLimit	  =	INT_MIN;
			maxLimit	  =	INT_MAX;
			break;
		}
		case ATP_UNS_NUM:
		{
			numberTypeIdx = UNSIGNED_NUMBER;
			minLimit	  =	(Atp_UnsNumType)0;
			maxLimit	  =	(Atp_UnsNumType)UINT_MAX;
			break;
		}
		default:
			return ATP_OK; /* no checking available */
	}

	/*
		Check minimum or maximum limit. Then sprintf the limit
		string into the buffer if not already done so.
	*/
	if (number_sign == POSITIVE)
	{
	  if (MaxLimits[notationIdx][numberTypeIdx] == NULL)
	  {
		return ATP_OK; /* conversion not available */
	  }
	  else
	  if (MaxLimits[notationIdx][numberTypeIdx][0] == ' ')
	  {
		if (numBase == 2)
		{
		  int dig, numOfBits;

		  /*
				If signed, minus 1 bit. e.g. 32-bit signed integer
				uses values represented by only 31 bits
		   */
		  numOfBits = (parmcode == ATP_NUM) ?
							(sizeof(Atp_NumType) * 8) - 1 :
							 sizeof(Atp_UnsNumType) * 8;

		  for (dig = 0; (dig < numOfBits); dig++)
			 MaxLimits[notationIdx][numberTypeIdx][dig] = '1';

		  MaxLimits[notationIdx][numberTypeIdx][numOfBits] = '\0';
		}
		else
		  (void) sprintf(MaxLimits[notationIdx][numberTypeIdx],
						 FmtConv[notationIdx][numberTypeIdx],
						 maxLimit);
	  }

	  LimitLen = strlen(MaxLimits[notationIdx][numberTypeIdx]);

	  limitCmpStr =
	    LimitString =
	    		MaxLimits[notationIdx][numberTypeIdx];
	}
	else
	{
	  /* Number_sign is NEGATIVE */

	  if (MinLimits[notationIdx][numberTypeIdx] == NULL)
	  {
		return ATP_OK; /* conversion not available */
	  }
	  else
	  if (MinLimits[notationIdx][numberTypeIdx][0] == ' ')
	  {
		(void) sprintf(	MinLimits[notationIdx][numberTypeIdx],
						FmtConv[notationIdx][numberTypeIdx],
						minLimit );
	  }

	  LimitLen = strlen(MinLimits[notationIdx][numberTypeIdx]);

	  limitCmpStr =
	    LimitString =
	    		MinLimits[notationIdx][numberTypeIdx];

	  if (LimitString[0] == '-')
	  {
		limitCmpStr += 1; /* skip the minus sign */
		LimitLen -= 1; /* less the '-' sign */
	  }
	}

	/* Compare numbers lexicographically. */
	numLen = SrcEnd - src;

	lexcmp_result = 0;

	if (numLen == LimitLen)
	{
	  lexcmp_result = Atp_Strcmp(src, limitCmpStr);

	  if (lexcmp_result <= 0)
	  {
		return ATP_OK; /* number <= limit */
	  }
	  else
	  if (lexcmp_result > 0)
	  {
		if (number_sign == POSITIVE)
		{
		  lexcmp_result = OVERFLOW_ERROR;
		}
		else
		{
		  lexcmp_result = UNDERFLOW_ERROR;
		}
	  }
	}
	else
	if (numLen > LimitLen)
	{
	  if (number_sign == POSITIVE)
	  {
		lexcmp_result = OVERFLOW_ERROR;
	  }
	  else
	  {
		/* Number_sign is NEGATIVE */
		lexcmp_result = UNDERFLOW_ERROR;
	  }
	}

	/*
		else condition here would be (numLen < LimitLen).
		Everything's fine here, within limit,
		so continue to leave function.
	 */

	if ((lexcmp_result == OVERFLOW_ERROR) ||
		(lexcmp_result == UNDERFLOW_ERROR))
	{
	  if (ErrorMsgPtr != NULL)
	  {
		int error_code;

		error_code = (lexcmp_result == OVERFLOW_ERROR) ?
									   ATP_ERRCODE_NUM_OVERFLOW :
									   ATP_ERRCODE_NUM_UNDERFLOW;

		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC, error_code,
										(parmcode == ATP_NUM) ?
												sizeof(Atp_NumType) * 8 :
												sizeof(Atp_UnsNumType) * 8,
										(parmcode == ATP_NUM) ?
												"signed integer" : "unsigned integer",
										SrcStart,
										(numBase == 16) ? "hex" :
												(numBase == 8) ? "octal":
														(numBase == 2) ? "binary" :
																				"decimal",
										LimitString);
	  }
	  return ATP_ERROR;
	}

	return ATP_OK;
}

/*+***************♦***************************************************

	Function Name:		Atp_RetrieveNumParm

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Atp_Num() primarily allows the application
						to retrieve the value for the number
						parameter defined using the num_def() macro,
						but is also usable for other numeric
						parameters and constructs such as unsigned
						number, keyword, boolean and choice.

	Modifications:
		Who			When				Description
	----------	-------------	-------------------------------
	Alwyn Teh	27 July 1992	Initial Creation
	Alwyn Teh	29 Sept 1992	CHOICE to return casevalue
								instead of caseindex

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_NumType Atp_RetrieveNumParm
(
	char	*NumericParmName,
	char	*filename,
	int		line_number
)
#else
Atp_NumType
Atp_RetrieveNumParm(NumericParmName, filename, line_number)
	char	*NumericParmNatne
	char	*filename;
	int		line_number;
#endif
{
	ParmStoreInfo			*MatchedParmInfoPtr;
	ParmStoreMemMgtNode		*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_NumType				number = 0;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore,
										NumericParmName,
										5, ATP_NUM, ATP_UNS_NUM, ATP_KEYS,
										ATP_BOOL, ATP_BCH);

	if (MatchedParmInfoPtr != NULL)
	{
		switch (Atp_PARMCODE(MatchedParmInfoPtr->parmcode))
		{
			case ATP_UNS_NUM:
			{
				Atp_UnsNumType unum = (*(Atp_UnsNumType *)MatchedParmInfoPtr->parmValue);
				return (Atp_NumType)unum;
			}
			case ATP_BCH:
			{
				Atp_ChoiceDescriptor *choice =
						(Atp_ChoiceDescriptor *)MatchedParmInfoPtr->parmValue;

				return (Atp_NumType)(choice->CaseValue);
			}
			case ATP_BOOL:
			{
				Atp_BoolType boolVal =
						(*(Atp_BoolType *)MatchedParmInfoPtr->parmValue);

				return (Atp_NumType)boolVal;
			}
			default:
				number = (*(Atp_NumType *)MatchedParmInfoPtr->parmValue);
				break;
		}
		return number;
	}
	else
	{
		/* This function may not return. */
		Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
							  "Atp_Num()", NumericParmName,
							  filename, line_number);
		return (Atp_NumType)0;
	}
}

/*+*****************+*************************************************

	Function Name:		Atp_RetrieveUNumParm

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Retrieves the unsigned number parameter.

	Modifications:
		Who			When				Description
	----------	-------------	----------------------------
	Alwyn Teh	27 July 1992	Initial Creation

********************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
Atp_UnsNumType Atp_RetrieveUNumParm
(
	char *UNumParmName,
	char *filename,
	int  line_number
)
#else
Atp_UnsNumType
Atp_RetrieveUNumParm(UNumParmName, filename, line_number)
	char *UNumParmName;
	char *filename;
	int  line_number;
#endif
{
	ParmStoreInfo		*MatchedParmInfoPtr;
	ParmStoreMemMgtNode	*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_UnsNumType		unumber =0;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore,
										UNumParmName,1,ATP_UNS_NUM);

	if (MatchedParmInfoPtr != NULL)
	{
	  unumber = (*(Atp_UnsNumType *)MatchedParmInfoPtr->parmValue);
	  return unumber;
	}
	else
	{
	  /* This function may not return. */
	  Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
							"Atp_UnsignedNum()", UNumParmName,
							filename, line_number);

	  return (Atp_UnsNumType)0;
	}
}

/*+********************************************************************

	Function Name:		Atp_RetrieveIndex

	Copyright:			BNR Europe Limited,	1992
						Bell-Northern Research
						Northern Telecom

	Description:		Atp_RetrieveIndex{)	gets the INDEX of the
						keyword and choice parameters.

						i.e. keyword - index to Atp_KeywordTab table
						choice - index of choice case branch

	Modifications:
		Who			When				Description
	----------	-------------	-----------------------------
	Alwyn Teh	27 July 1992	Initial Creation

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_UnsNumType Atp_RetrieveIndex
(
	char *ParmName,
	char *filename,
	int  line_number
)
#else
Atp_UnsNumType
Atp_RetrieveIndex(ParmName, filename, line_number)
	char *ParmName;
	char *filename;
	int	 line_number;
#endif
{
	ParmStoreInfo		*MatchedParmInfoPtr;
	ParmStoreMemMgtNode	*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_UnsNumType		index = 0;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore,
										ParmName,
										2, ATP_KEYS, ATP_BCH);

	if (MatchedParmInfoPtr != NULL)
	{
		switch (Atp_PARMCODE(MatchedParmInfoPtr->parmcode))
		{
			case ATP_BCH:
				index = MatchedParmInfoPtr->TypeDependentInfo.ChoiceCaseIdx;
				break;
			case ATP_KEYS:
				index = MatchedParmInfoPtr->TypeDependentInfo.KeywordCaseIdx;
				break;
			default:
				break;
		}
		return index;
	}
	else
	{
		/* This function may not return. */
		Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
							  "Atp_Index()", ParmName,
							  filename, line_number);

		return (Atp_UnsNumType)0;
	}
}
