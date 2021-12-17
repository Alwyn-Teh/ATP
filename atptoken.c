/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atptoken.c

	Copyright:			BNR Europe Limited, 1992, 1994, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains a suite of routines
						for tokenising a piece of string.

*******************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/* Definitions */
#define DEFAULT_NUM_TOKENS	16
#define SPAREROOM			1

/*+*******************************************************************

	Function Name:		Atp_Tokeniser

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		A basic tokeniser for testing purposes.
						Makes a token list from the source string.

						Two pieces of storage are dynamically
						allocated:
						a)	the array of pointers to the tokens,
						b)	the contiguous store containing the
							tokens.

						It is the caller's responsibility to free
						them after use using Atp_FreeTokenList().

	Modifications:
		Who			When				Description
	----------	-------------	----------------------------------
	Alwyn Teh	28 July 1992	Initial Creation
	Alwyn Teh	29 July 1994	Handle NULL or empty input string

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
char **Atp_Tokeniser(char *Source, int *tokencount)
#else
char **
Atp_Tokeniser(Source, tokencount)
	char *Source;
	int * tokencount;
#endif
{
	register int	count;
	char			**TokenPtrs;
	char			*src, *token, *TokenString, *contSrc;
	unsigned int	NumOfTokenPtrs = DEFAULT_NUM_TOKENS;
	Atp_UnsNumType	tokenLength;

	/* Go no further if Source is empty. */
	if ((Source == NULL) || (*Source == '\0')) {
	  if (tokencount != NULL)
	    *tokencount = 0;
	  return NULL;
	}

	/* Miscellaneous initializations */
	count = 0;
	while (isspace(*Source)) Source++;
	src = Source;
	token = TokenString = contSrc = NULL;

	/* Preallocate array of token pointers. */
	TokenPtrs = (char **)CALLOC((NumOfTokenPtrs + SPAREROOM), sizeof(char *), NULL);

	/*
		Allocate store to contain tokens. (Get maximum possible -
		worst case is that every character is a token requiring a
		NULL terminator !)
	 */
	TokenString = (char *)CALLOC((strlen(src) * 2) +1, sizeof(char), NULL);

	tokenLength = 0;
	TokenPtrs[count] = TokenString;

	/* Tokenize the whole input source string up to the end of line. */
	while (*src != '\0')
	{
		token = Atp_GetToken(src, &contSrc, &tokenLength) ;
		if (token == NULL) break;

		/* Copy token across to contiguous string containing tokens. */
		TokenPtrs[count] = strcpy(TokenPtrs[count], token);
		FREE(token);

		count++;

		if (count >= NumOfTokenPtrs) {
		  /* Enlarge array of token pointers. */
		  TokenPtrs = (char **)REALLOC(TokenPtrs,
				  	  	  	  	  	   ((NumOfTokenPtrs += DEFAULT_NUM_TOKENS) + SPAREROOM)
									    * sizeof(char *),
										NULL);
		}

		/* Tell next token where to go. */
		TokenPtrs[count] = TokenPtrs[count-1] + tokenLength + 1;

		/* Carry on from next source character. */
		src = contSrc;

	} /* while */

	TokenPtrs[count] = NULL;

	if (tokencount != NULL) *tokencount = count;

	return TokenPtrs;
}

/*+*******************************************************************

	Function Name:		Atp_FreeTokenList

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Frees token list created by Atp_Tokeniser.

	Modifications:
		Who			When				Description
	----------	-------------	-----------------------------
	Alwyn Teh	28 July 1992	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_FreeTokenList(char **tokenlist)
#else
void
Atp_FreeTokenList(token!ist)
	char **tokenlist;
#endif
{
	if (tokenlist != NULL) {
	  FREE(tokenlist [0]);
	  FREE(tokenlist);
	}
}

/*+********************************************************************

	Function Name:		Atp_GetToken

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Gets a token which is either delimited by
						white space or double/single quotes.

						White space delimited tokens can also be
						terminated by a NULL character.

						If	TokenLengthPtr is supplied,	returns
						length of returned token.

						Results: Returns pointer to token.
								 Returns NULL if no token is found.

						If NextSrc is supplied, returns pointer to
						char in src where parsing may continue after
						calling Atp_GetToken().

						If	TokenLengthPtr is supplied,	returns
						length of returned token.

	Modifications:
		Who			When				Description
	----------	-------------	------------------------------
	Alwyn Teh	28 July 1992	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
char *Atp_GetToken(char *src, char **NextSrc, Atp_UnsNumType *TokenLengthPtr)
#else
char *
Atp_GetToken(src, NextSrc, TokenLengthPtr)
	char			*src;				/*	input source */
	char			**NextSrc;			/*	and continuation source */
	Atp_UnsNumType	*TokenLengthPtr;	/*	pointer to unsigned number
											for returning token length */
#endif
{
	register char	*ptr = src;
	char			*tokenPtr, delimiter, *ReturnToken;
	unsigned int	n;

	if (NextSrc != NULL) *NextSrc = src;

	while (isspace(*ptr)) ptr++;
	if (*ptr == '\0') {
	  if (NextSrc != NULL) *NextSrc = ptr;
	  return NULL;
	}

	/*
	 * Quoted token.
	 */
	if ((*ptr = ATP_DOUBLE_QUOTE) || (*ptr == ATP_SINGLE_QUOTE)) {

	  delimiter = *ptr;
	  ptr += 1;
	  tokenPtr = ptr;

	  while (*ptr != '\0') {
		  if (*ptr == delimiter) break;
		  if (*ptr != '\0') ptr++;
	  }

	  if (NextSrc != NULL) *NextSrc = (*ptr == delimiter) ? ptr+1 : ptr;

	}
	else {
	  /*
	   * White space delimited token.
	   */
	  tokenPtr = ptr;
	  while ((*ptr != '\0') && !isspace(*ptr)) {
		  if (*ptr != '\0') ptr++;
	  }
	  if (NextSrc != NULL) *NextSrc = ptr;
	}

	n = ptr - tokenPtr;
	ReturnToken = (char *)MALLOC(n+1, NULL);
	(void) strncpy(ReturnToken, tokenPtr, n) ;
	ReturnToken[n] = '\0';

	if (TokenLengthPtr != NULL) *TokenLengthPtr = (Atp_UnsNumType) n;

	/* Return pointer to token. Caller has to free it after use. */
	return ReturnToken;
}
