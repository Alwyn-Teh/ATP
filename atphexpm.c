/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atphexpm.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains the implementation	of
						the hexadecimal DATABYTES parameter.

********************************************************************-*/

#include <ctype.h>
#include <stdlib.h>

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*+*******************************************************************

	Function Name:		Atp_ProcessDatabytesParm

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom

	Description:		Databytes processor - handles	parameter,
						default value, range and vproc checks.

	Modifications:
		Who			When						Description
	----------	----------------	-----------------------------------
	Alwyn Teh	30 July 1992		Initial Creation
	Alwyn Teh	27 November 1992	Adjust CurrArgvIdx
	Alwyn Teh	20 May 1993			Change to Atp_DataDescriptor
	Alwyn Teh	9 June 1993			Add Atp_AppendParmName()
	Alwyn Teh	25 October 1993		Optional databytes default value
									is of type (Atp_DataDescriptor
									*) and not (Atp_ByteType *)
									since the count value is needed
									too. Also, the default data
									desc needs to be duplicated and
									converted to the internal format
									with the byte count in the [-1]
									position.
	Alwyn Teh	20 January 1994		Free databytes when error results
									from range/vproc checking...etc.

*******************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
Atp_Result Atp_ProcessDatabytesParm( Atp_ParserStateRec *parseRec )
#else
Atp_Result
Atp_ProcessDatabytesParm(parseRec)
	Atp_ParserStateRec *parseRec;
#endif
{
	Atp_Result			result = ATP_OK;
	Atp_DataDescriptor	databytes;
	register int		count;
	char				*errmsg = NULL;

	parseRec->ReturnStr = NULL;

	/* Initialize to zeroes. */
	databytes.count	= 0;
	databytes.data	= NULL;

	result = Atp_SelectInputAndParseParm(parseRec,
										 (Atp_ArgParserType)Atp_ParseDataBytes,
										 "hexadecimal data bytes",
										 &databytes, &errmsg) ;

	if (result == ATP_OK) {
	  /* Handle default databytes value. */
	  if (parseRec->ValueUsedIsDefault) {
		Atp_DataDescriptor *databytes_ptr = NULL;

		databytes_ptr = (Atp_DataDescriptor *)(parseRec->defaultPointer);

		if (databytes_ptr != NULL) {
		  /* Copy databytes data in internal format. */
		  /* new_data will be released in the normal manner after use. */
		  Atp_ByteType *new_data = (Atp_ByteType *)
									CALLOC(databytes_ptr->count
											+sizeof(Atp_UnsNumType),
									sizeof(Atp_ByteType), NULL);

		  new_data += sizeof(Atp_UnsNumType);

		  for (count = 0; count < databytes_ptr->count; count++)
			 new_data[count] = ((Atp_ByteType *)databytes_ptr->data)[count];

		  Atp_DataBytesCount(new_data) = (Atp_UnsNumType)databytes_ptr->count;

		  /* Assign to real databytes. */
		  databytes = *databytes_ptr; /* assign all fields */
		  databytes.data = new_data; /* but change data field only */
		}
	  }
	  else
		parseRec->CurrArgvIdx++;
	}

	if (result == ATP_OK)
	  result = Atp_CheckRange(&Atp_ParseRecParmDefEntry(parseRec),
							  !(parseRec->ValueUsedIsDefault),
							  databytes, &errmsg);

	if (result == ATP_OK)
	result = Atp_InvokeVproc(&Atp_ParseRecParmDefEntry(parseRec),
							 &databytes,
							 !(parseRec->ValueUsedIsDefault),
							 &errmsg);

	if (result == ATP_OK)
	  Atp_StoreParm(&Atp_ParseRecParmDefEntry(parseRec),
				    parseRec->ValueUsedIsDefault, databytes);

	if (result == ATP_ERROR) {
	  /* Free up databytes after use since error encountered. */
	  void *ptr = (databytes.data == NULL) ? NULL :
			  	  	  	  &Atp_DataBytesCount(databytes.data);
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

/*+*******************************************************************

	Function Name:		Atp_ParseDataBytes

	Copyright:			BNR Europe Limited,	1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Databytes parameter	parser.
						Reads in a string of hexadecmial digit pairs.
						Returns parameter value in Atp_DataDescriptor.

	Modifications:
		Who			When					Description
	-----------	--------------	-------------------------------------
	Alwyn Teh	26 July 1992	Initial Creation
	Alwyn Teh	20 May 1993		Change to Atp_DataDescriptor
	Alwyn Teh	7 July 1993		Change ((Atp_UnsNumType *)byte_string)++
								to byte_string += sizeof(Atp_UnsNumType)

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_ParseDataBytes
(
	char				*hex_src,
	Atp_DataDescriptor	*dataBytes,
	char				**ErrorMsgPtr
)
#else
Atp_Result
Atp_ParseDataBytes(hex_src, dataBytes, ErrorMsgPtr)
	char				*hex_src; /* source of ascii hex representation */
	Atp_DataDescriptor	*dataBytes;
	char				**ErrorMsgPtr;
#endif
{
	Atp_ByteType			hex_byte[3];
	register Atp_ByteType	*byte_string;
	register int			hex_str_index, byte_index, hex_digits_count;
	register char			*scanPtr;

	char *ptr, *hexstr_start;
	char *hex_str = NULL;
	int	hex_str_length;
	Atp_Result result;

	if (ErrorMsgPtr != NULL) *ErrorMsgPtr = NULL;

	if (dataBytes == NULL) {
	  if (ErrorMsgPtr != NULL) {
		Atp_ShowErrorLocation();
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_RETURN_VALUE_POINTER_ABSENT,
										"hexadecimal data bytes");
	  }
	  return ATP_ERROR;
	}

	/*
		Parse off the hexadecimal string, making a duplicate copy
		so that there is no chance of corrupting the original
		data.
	*/
	result = Atp_ParseStr(hex_src, &hex_str, &hex_str_length,
						  "hexadecimal data bytes", ErrorMsgPtr);

	if (result == ATP_ERROR) {
	  if (hex_str != NULL)
	    FREE(hex_str);
	  return ATP_ERROR;
	}

	hexstr_start = hex_str; /* save pointer to beginning of hex string */

	/* Preliminary check, scan hex string for contents. */
	for (scanPtr = hex_str, hex_digits_count = 0; *scanPtr != '\0'; scanPtr++)
	{
	   /* Expect to find hex digits. */
	   if (isxdigit(*scanPtr)) {
		 hex_digits_count++;
	   }
	   else {
		 /* If not a hex digit, should be space but nothing else. */
		 if (!isspace(*scanPtr)) {
		   if (ErrorMsgPtr != NULL) {
		     char *tmp;

		     /*
				For error message, terminate illegal hex-string
				at next space or end of hex string.
			  */
			 tmp = scanPtr;
			 while ((*tmp != '\0') && !isspace(*tmp)) tmp++;
			 *tmp = '\0';

			 *ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
											 ATP_ERRCODE_INVALID_HEX_DATABYTES,
											 scanPtr);
		   }
		   if (hex_str != NULL) FREE(hex_str);
		   return ATP_ERROR;
		 }
	   }
	}

	if (hex_digits_count == 0) {
	  if (ErrorMsgPtr != NULL) {
		*ErrorMsgPtr = Atp_MakeErrorMsg(ERRLOC,
										ATP_ERRCODE_EMPTY_DATABYTES_STRING);
	  }
	  if (hex_str != NULL) FREE(hex_str);
	    return ATP_ERROR;
	}

	/* Remove any leading and trailing spaces */
	while (isspace(*hex_str)) {
		 hex_str++;
		 hex_str_length--;
	}

	while ((hex_str_length != 0) &&
			isspace(hex_str[hex_str_length - 1])) {
		 hex_str[--hex_str_length] = '\0';
	}

	/* Allocate maximum storage for data bytes and length count. */
	byte_string = (Atp_ByteType *)
					CALLOC( ((hex_str_length/2) + 1) +
							sizeof(Atp_UnsNumType),
							sizeof(Atp_ByteType), NULL);

	/* -1th field is length of byte string */
	byte_string += sizeof(Atp_UnsNumType);

	/*
		Initialize hex byte - this is working space for
		converting one hex byte.
	*/
	hex_byte[2] = '\0'; /* terminates 2 hex digits */

	/* Convert ASCII hex data to binary. */
	for (hex_str_index = 0, byte_index = 0, ptr = NULL;
		 hex_str_index < hex_str_length;
		 hex_str_index += 2, byte_index++) {


	   /* Skip any leading spaces - essential */
	   while (isspace(hex_str[hex_str_index]))
	   hex_str_index++;

	   /* Extract 2 hex digits for a byte */
	   hex_byte[0] = hex_str[hex_str_index];
	   hex_byte[1] = hex_str[hex_str_index + 1];

	   /* Get the hex value and write to store */
	   byte_string[byte_index] =
			   (Atp_ByteType)strtol((char *)hex_byte, &ptr, 16);

	   /* This will only catch error in 1st hex digit */
	   if ((char *)hex_byte == ptr) {
	     if (ErrorMsgPtr != NULL) {
	       *ErrorMsgPtr = Atp_MakeErrorMsg(	ERRLOC,
											ATP_ERRCODE_INVALID_HEX_DATABYTES,
											hex_byte );
	     }
	     if (hexstr_start != NULL) FREE(hexstr_start);
	     FREE(&Atp_DataBytesCount(byte_string));

	     return ATP_ERROR;
	   }
	}

	/* How many bytes did we read in ? */
	Atp_DataBytesCount(byte_string) = (Atp_UnsNumType)byte_index;
	dataBytes->count = (Atp_UnsNumType)byte_index;

	/* Return result to caller. */
	dataBytes->data = byte_string;

	if (hexstr_start != NULL) FREE(hexstr_start);

	return ATP_OK;
}

/*+★******************************************************************

	Function Name:		Atp_RetrieveDataBytesParm

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Retrieves the databytes parameter	as a
						pointer to the data.

	Modifications:
		Who			When				Description
	----------	-------------	-----------------------------
	Alwyn Teh	27 July 1992	Initial Creation
	Alwyn Teh	20 May 1993		Change to Atp_DataDescriptor

*******************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
Atp_ByteType *
Atp_RetrieveDataBytesParm
(
	char			*dataParmName,
	Atp_UnsNumType	*NumOfBytes,
	char			*filename,
	int				line_number
)
#else
Atp_ByteType *
Atp_RetrieveDataBytesParm(dataParmName, NumOfBytes, filename, line_number)
	char			*dataParmName;
	Atp_UnsNumType	*NumOfBytes; /* Number of bytes returned */
	char			*filename;
	int				line_number;
#endif
{
	ParmStoreInfo		*MatchedParmInfoPtr;
	ParmStoreMemMgtNode	*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_DataDescriptor	dataBytes;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore,
										dataParmName, 1, ATP_DATA);

	if (MatchedParmInfoPtr != NULL) {
	  dataBytes = (*(Atp_DataDescriptor *)MatchedParmInfoPtr->parmValue);
	  if (NumOfBytes != NULL) {
		*NumOfBytes = Atp_DataBytesCount(dataBytes.data);
	  }
	  return (Atp_ByteType *) dataBytes.data;
	}
	else {
	  Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
			  	  	  	    "Atp_DataBytes()", dataParmName,
							filename, line_number);

	  return NULL; /* above function may not return here */
	}
}

/*+*******************************************************************

	Function Name:		Atp_RetrieveDataBytesDescriptor

	Copyright:			BNR Europe Limited, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Retrieves the databytes parameter as
						a Atp_DataDescriptor structure.

	Modifications:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	20 May 1993		Initial Creation
	Alwyn Teh	27 March 1995	Correct error function ID,
								should be Atp_DataBytesDesc()
								and not Atp_DataBytes()

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_DataDescriptor
Atp_RetrieveDataBytesDescriptor(char *dataParmName,
								char *filename, int line_number)
#else
Atp_DataDescriptor
Atp_RetrieveDataBytesDescriptor(dataParmName, filename, line_number)
	char	*dataParmName;
	char	*filename;
	int		line_number;
#endif
{
	ParmStoreInfo		*MatchedParmInfoPtr;
	ParmStoreMemMgtNode	*CurrParmStore = (ParmStoreMemMgtNode *)Atp_CurrParmStore();
	Atp_DataDescriptor	dataBytes;

	/* Initialize */
	dataBytes.count = 0;
	dataBytes.data = NULL;

	MatchedParmInfoPtr = Atp_SearchParm(CurrParmStore->CtrlStore,
										dataParmName, 1, ATP_DATA);

	if (MatchedParmInfoPtr != NULL) {
	  dataBytes = (*(Atp_DataDescriptor *)MatchedParmInfoPtr->parmValue);
	  return dataBytes;
	}
	else {
	  Atp_RetrieveParmError(ATP_ERRCODE_CANNOT_RETRIEVE_PARM,
			  	  	  	    "Atp_DataBytesDesc()", dataParmName,
							filename, line_number);

	  return dataBytes; /* above function may not return here */
	}
}

/*+*******************************************************************

	Function Name:		Atp_DisplayHexBytes

	Copyright:			BNR Europe Limited, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		Display Atp_DataDescriptor as a hex string.

	Modifications:
		Who			When			Description
	---------	--------------	-----------------------
	Alwyn Teh	28 March 1995	Initial Creation

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
char * Atp_DisplayHexBytes(Atp_DataDescriptor databytes_desc,
						   char *hexbyte_separator)
#else
char * Atp_DisplayHexBytes{databytes_desc, hexbyte_separator)
	Atp_DataDescriptor databytes_desc;
	char *hexbyte_separator;
#endif
{
	register int Byte;
	Atp_NumType no_of_bytes = databytes_desc.count;
	Atp_ByteType *databytes = (Atp_ByteType *)databytes_desc.data;
	int field_width = 2 + strlen(hexbyte_separator);
	char *string = (char *)CALLOC(no_of_bytes * field_width + 1,
								  sizeof(char), NULL);

	register char *ptr = string;

	for (Byte = 0; Byte < no_of_bytes; Byte++)
	{
	   (void)sprintf (ptr, "%2.2X%s", databytes [Byte] , hexbyte_separator);
	   ptr += field_width; /* forward field_width characters */
	}
	ptr -= strlen(hexbyte_separator);
	*ptr = '\0';
	return string;
}
