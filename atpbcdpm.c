/* EDITION AA02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+****★**************************************************************

	Module Name:		atpbcdpm.c

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation of
						the BCD (Binary Coded Decimal) digits parameter.

*******************************************************************-*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char	*__Atp_Local_FileName__ = __FILE__;
#endif

static char	*bcd_long_desc = "4-bit BCD (Binary Coded Decimal) digits";

/*+*******************************************************************

	Function Name:		Atp_ProcessBcdDigitsParm

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		BCD digits processor - handles parameter,
						default value, range and vproc checks.

	Modifications:
		Who			When				Description
	----------	-------------	--------------------------
	Alwyn Teh	24 March 1995	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ProcessBcdDigitsParm( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessBcdDigitsParm(parseRec)
	Atp_ParserStateRec *parseRec;
#endif
{
		Atp_Result			result = ATP_OK;
		Atp_DataDescriptor	bcd_digits;
		register int		count;
		char				*errmsg = NULL;

		parseRec->ReturnStr = NULL;

		/* Initialize to zeroes. */
		bcd_digits.count = 0;
		bcd_digits.data = NULL;

		result = Atp_SelectInputAndParseParm(parseRec,
								(Atp_ArgParserType)Atp_ParseBcdDigits,
								bcd_long_desc,
								&bcd_digits, &errmsg);

		if (result == ATP_OK) {
		  /* Handle default bcd_digits value. */
		  if (parseRec->ValueUsedIsDefault) {
			Atp_DataDescriptor *BcdDigits_ptr = NULL;
			BcdDigits_ptr = (Atp_DataDescriptor*) (parseRec->defaultPointer);
			if (BcdDigits_ptr != NULL) {
				/* Copy BCD digits data in internal format. */
				/* new_data will be released in the normal manner after use. */
				int no_of_nibbles = BcdDigits_ptr->count;
				int no_of_bytes = (no_of_nibbles + 1) / 2;
				Atp_ByteType *new_data = (Atp_ByteType*) CALLOC(no_of_bytes +
																sizeof(Atp_UnsNumType),
																sizeof(Atp_ByteType), NULL);
				new_data += sizeof(Atp_UnsNumType);
				for (count = 0; count < no_of_bytes; count++)
				new_data[count] = ((Atp_ByteType *)BcdDigits_ptr->data)[count];
					Atp_BcdDigitsCount(new_data)=(Atp_UnsNumType)BcdDigits_ptr->count;
					/* Assign to real bcd_digits. */
					bcd_digits = *BcdDigits_ptr; /* assign all fields */
					bcd_digits.data = new_data; /* but change data field only */
			}
		  } else
			  parseRec->CurrArgvIdx++;
		}

		if (result == ATP_OK)
		  result = Atp_CheckRange(&Atp_ParseRecParmDefEntry(parseRec),
								  !(parseRec->ValueUsedIsDefault),
								  bcd_digits, &errmsg);
		if (result == ATP_OK)
		  result = Atp_InvokeVproc(&Atp_ParseRecParmDefEntry(parseRec),
								   &bcd_digits, !(parseRec->ValueUsedIsDefault),
								   &errmsg);

		if (result == ATP_OK)
		  Atp_StoreParm(&Atp_ParseRecParmDefEntry(parseRec),
						parseRec->ValueUsedIsDefault, bcd_digits);

		if (result == ATP_ERROR) {
		  /* Free up databytes after use since error encountered. */
		  void *ptr = (bcd_digits.data == NULL) ? NULL :
				  	   &Atp_BcdDigitsCount(bcd_digits.data);
		  if (ptr != NULL)
			FREE(ptr);

		  if ((parseRec->ReturnStr == NULL) && (errmsg != NULL)) {
			Atp_AppendParmName(parseRec, errmsg);
			parseRec->ReturnStr = errmsg;
		  }
		}

		parseRec->result = result;

		if (result == ATP_OK) {
		  if (!parseRec->RptBlkTerminatorSize_Known)
			parseRec->RptBlkTerminatorSize += sizeof(Atp_DataDescriptor);
		}

		return result;
}

/*********************************************************************

	Function Name:		Atp_ParseBcdDigits

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		BCD Digits parameter parser.
						Reads in a string of BCD digits.
						Returns parameter value in Atp_DataDescriptor.

	Modifications:
		Who			When				Description
	----------	-------------	----------------------------
	Alwyn Teh	27 March 1995	Initial Creation

********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
Atp_Result Atp_ParseBcdDigits
(
	char				*bcd_src,
	Atp_DataDescriptor	*bcdDigits,
	char				**ErrorMsgPtr
)
#else
Atp_Result
Atp_ParseBcdDigits(bcd_src, bcdDigits, ErrorMsgPtr)
	char				*bcd_src;
	Atp_DataDescriptor	*bcdDigits;
	char				**ErrorMsgPtr;
#endif
{
		Atp_ByteType	*bcd_digits, *bcd_ptr, digit;
		register int	bcd_digits_count, index, Size;
		register char	*scanPtr;

		char			*ptr, *bcdstr_start;
		char			*bcd_str = NULL;
		int				bcd_str_length;
		Atp_Result		result;

		if (ErrorMsgPtr != NULL) *ErrorMsgPtr = NULL;

		if (bcdDigits == NULL) {
		  if (ErrorMsgPtr != NULL) {
			Atp_ShowErrorLocation();
			*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
								ATP_ERRCODE_RETURN_VALUE_POINTER_ABSENT,
								bcd_long_desc);
		  }
		  return ATP_ERROR;
		}

		/*
			Parse off the BCD digits string, making a duplicate copy
			so that there is no chance of corrupting the original
			data.
		*/
		result = Atp_ParseStr(bcd_src, &bcd_str, &bcd_str_length,
							  bcd_long_desc, ErrorMsgPtr);

		if (result == ATP_ERROR) {
		  if (bcd_str != NULL)
			FREE(bcd_str);
		  return ATP_ERROR;
		}

		bcdstr_start = bcd_str; /* save pointer to beginning
								   of bcd string */

		/* Preliminary check, scan bcd string for contents. */
		for (scanPtr = bcd_str, bcd_digits_count = 0; *scanPtr != '\01'; scanPtr++)
		{
			/* Expect to find bcd digits (0..9 and also A..F, a..f too) */
			if (isxdigit(*scanPtr)) {
				bcd_digits_count++;
			} else {
				/* If not a bcd digit, should be space but nothing else. */
				if (!isspace(*scanPtr)) {
					if (ErrorMsgPtr != NULL) {
						char *tmp;
						/*
							 For error message, terminate illegal bcd-string
							 at next space or end of bcd string.
						 */
						tmp = scanPtr;
						while ((*tmp != '\0') && !isspace(*tmp)) tmp++;
						*tmp = '\0';

						ErrorMsgPtr = Atp_MakeErrorMsg (ERRLOC,
														ATP_ERRCODE_INVALID_BCD_DIGITS,
														scanPtr);
					}
					if (bcd_str != NULL) FREE(bcd_str);
					return ATP_ERROR;
				}
			}
		}
		if (bcd_digits_count == 0) {
			if (ErrorMsgPtr != NULL) {
				*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
												ATP_ERRCODE_EMPTY_BCD_DIGITS);
			}
			if (bcd_str != NULL) FREE(bcd_str);
			return ATP_ERROR;
		}

		/* Remove any spaces in string and convert to lower case. */
		for (ptr = scanPtr = bcd_str; *scanPtr != '\0'; scanPtr++)
		{
			if (!isspace(*scanPtr))
			*ptr++ = tolower(*scanPtr);
		}
		*ptr = '\0';
		bcd_str_length = strlen(bcd_str);

		/* Allocate maximum storage for BCD digits and length count. */
		Size = (bcd_str_length/2 + 1) + sizeof(Atp_UnsNumType) + 1;
		bcd_digits = (Atp_ByteType*) CALLOC(Size, sizeof(Atp_ByteType),	NULL);

		memset(bcd_digits, 0, Size); /* ensure zeroed, don't trust calloc() */

		/* -1th field indicates number of BCD digits */
		bcd_digits += sizeof(Atp_UnsNumType);

		/* Convert BCD digits to binary. */
		scanPtr = bcd_str;		/* ASCII source */
		bcd_ptr = bcd_digits;	/* BCD destination */
		index = 0;				/* index of ASCII source */
		while (scanPtr[index] != '\0')
		{
			if (isalpha(scanPtr [index]))
			  digit = scanPtr[index] - 'a' + 10;
			else
			  digit = scanPtr[index] - 'O';

			if (index & 1)
			  bcd_ptr[index/2] |= (digit << 4);
			else
			  bcd_ptr[index/2] |= digit;

			index++;
		}

		/* How many BCD digit nibbles did we read in ? */
		bcd_digits_count = index;
		Atp_BcdDigitsCount(bcd_digits) = (Atp_UnsNumType) bcd_digits_count;
		bcdDigits->count = (Atp_UnsNumType) bcd_digits_count;

		/* Return result to caller. */
		bcdDigits->data = bcd_digits;

		if (bcdstr_start != NULL) FREE(bcdstr_start);
		return ATP_OK;
}

/*******************************************************************

	Function Name:		Atp_RetrieveBcdDigitsParm

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		Retrieves the BCD digits parameter as a
						pointer to the first BCD digit.

	Modifications:
		Who			When			Description
	----------	-------------	-----------------------
	Alwyn Teh	27 March 1995	Initial Creation

********★********★**************************************************/
#if defined (__STDC__) || defined (__cplusplus)
Atp_ByteType *
Atp_RetrieveBcdDigitsParm
(
	char			*BcdParmName,
	Atp_UnsNumType	*NumOfBcdDigitNibbles,
	char			*filename,
	int				line_number
)
#else
Atp_Byt eType *
Atp_RetrieveBcdDigitsParm(BcdParmName, NumOfBcdDigitNibbles,
						  filename, line_number)
	char			*BcdParmName;
	Atp_UnsNumType	*NumOfBcdDigitNibbles
	char			*filename;
	int				line_number;
#endif
{
	ParmStoreInfo		*MatchedParmInfoPtr;
	ParmStoreMemMgtNode	*CurrParmStore =
								(ParmStoreMemMgtNode *)
								Atp_CurrParmStore();
	Atp_DataDescriptor	BcdDigits;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore,
										BcdParmName, 1, ATP_BCD);

	if (MatchedParmInfoPtr != NULL) {
		BcdDigits = (*(Atp_DataDescriptor *) MatchedParmInfoPtr->parmValue);
		if (NumOfBcdDigitNibbles != NULL) {
			*NumOfBcdDigitNibbles = Atp_BcdDigitsCount(BcdDigits.data);
		}
		return (Atp_ByteType *) BcdDigits.data;
	} else {
		Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
							  "Atp_BcdDigits()", BcdParmName,
							  filename, line_number);

		return NULL; /* above function may not return here */
	}
}

/*+******************************************************************

	Function Name:		Atp_RetrieveBcdDigitsDescriptor

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research
						Northern Telecom / Nortel

	Description:		Retrieves the BCD digits parameter as
						a Atp_DataDescriptor structure.

	Modifications:
		Who			When				Description
	-----------	--------------	--------------------------
	Alwyn Teh	27 March 1995	Initial Creation

******************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
Atp_DataDescriptor
Atp_RetrieveBcdDigitsDescriptor(char *BcdParmName,
								char *filename, int line_number)
#else
Atp_DataDescriptor
Atp_RetrieveBcdDigitsDescriptor(BcdParmName, filename, line_number)
	char	*BcdParmName;
	char	*filename;
	int		line_number;
#endif
{
	ParmStoreInfo		*MatchedParmlnfoPtr;
	ParmStoreMemMgtNode	*CurrParmStore =
								(ParmStoreMemMgtNode *)
								Atp_CurrParmStore();
	Atp_DataDescriptor	BcdDigits;

	/* Initialize */
	BcdDigits.count = 0;
	BcdDigits.data = NULL;

	MatchedParmlnfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore,
										BcdParmName, 1, ATP_BCD);

	if (MatchedParmlnfoPtr != NULL) {
		BcdDigits = (*(Atp_DataDescriptor *)MatchedParmlnfoPtr->parmValue);
		return BcdDigits;
	}
	else {
		Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
							  "Atp_BcdDigitsDesc()", BcdParmName,
							  filename, line_number);

		return BcdDigits; /* above function may not return here */
	}
}

/*+*******************************************************************

	Function Name:		Atp_DisplayBcdDigits

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		Display Atp_DataDescriptor	as BCD digits.

	Modifications:
		Who			When			Description
	----------	--------------	-----------------------
	Alwyn Teh	27 March 1995	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
char * Atp_DisplayBcdDigits(Atp_DataDescriptor bcd_digits_desc)
#else
char * Atp_DisplayBcdDigits(bcd_digits_desc)
	Atp_DataDescriptor bcd_digits_desc;
#endif
{
	int nibbles, digit;
	Atp_NumType no_of_digits = bcd_digits_desc.count;
	Atp_ByteType *bcd_digits = (Atp_ByteType*) bcd_digits_desc.data;
	char *string = (char*) CALLOC(no_of_digits + 1, sizeof(char), NULL);

	for (nibbles = 0; nibbles < no_of_digits; nibbles++)
	{
		digit = (nibbles & 1) ? bcd_digits[nibbles/2] >> 4 :
								bcd_digits[nibbles/2] & 0x0F;
		string[nibbles] = (digit > 9) ? digit-10+'A' : digit+'0';
	}
	string[nibbles] = '\0';
	return string;
}
