/* EDITION AC02 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+*******************************************************************

	Module Name:		atpstcmp.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains miscellaneous	string
						operations, mainly for string comparisons.

*******************************************************************-*/

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*+*******************************************************************

	Function Name:		Atp_Strcmp

	Copyright:			BNR Europe Limited, 1992, 1994
						Bell-Northern Research
						Northern Telecom

	Description:		Similar to strcmp except that letter case is
						ignored.

						Returns	<0	if	s1<s2,
								 0	if	s1=s2,
								>0	if	s1>s2.

						NOTE: Value of return code is always (*s1 - *s2)
							  whereas strcmp seems to return -1, 0,
							  or 1 only.

	Modifications:
		Who			When				Description
	-----------	--------------	--------------------------------
	Alwyn Teh	25 July 1992	Initial Creation
	Alwyn Teh	5 January 1994	Port to SUN workstation where
								NULL strings cause core dumps

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_Strcmp(char *s1, char *s2)
#else
int
Atp_Strcmp(s1,s2)
	char *s1, *s2;
#endif
{
	register char	*rs1 = s1;
	register char	*rs2 = s2;

	int	rc = 0;

	if (s1 == 0 || s2 == 0) {
	  if (s1 == 0 && s2 == 0)
	    return 0;
	    else
	      if (s1 == 0 && s2 != 0)
	        return -1;
	      else
	        if (s1 != 0 && s2 == 0)
	          return 1;
	}

	while (	(*rs1 != 0) &&
			(*rs2 != 0) &&
			(tolower(*rs1) == tolower(*rs2))) {
		rs1++;
		rs2++;
	}

	rc = tolower(*rs1) - tolower(*rs2);

	return rc;
}

/*+*******************************************************************

	Function Name:		Atp_Strncmp

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Similar to strncmp except that letter case
						is ignored.

						Returns	<0 if s1<s2,
								 0 if s1=s2,
								>0 if s1>s2.

	NOTE:				Value of return code is always (*s1 - *s2)

	Modifications:
		Who			When				Description
	----------	---------------	-------------------------------
	Alwyn Teh	25 July 1992	Initial	Creation
	Alwyn Teh	5 January 1994	Port to	SUN	workstation	where
								NULL strings cause core dumps

*********************************************»*********************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_Strncmp(char *s1, char *s2, int n)
#else
int
Atp_Strncmp (s1, s2, n)
	char *s1, *s2;
	int n;
#endif
{
	register char	*rs1 = s1;
	register char	*rs2 = s2;
	register int	x = 0;

	int	rc = 0;

	if (n <= 0) {
	  return 0;	/* i.e. s1 and s2 considered equal */
	}

	if (s1 == 0 || s2 == 0) {
	  if (s1 == 0 && s2 == 0)
	    return 0;
	  else
	    if (s1 == 0 && s2 != 0)
	      return -1;
	    else
	      if (s1 != 0 && s2 == 0)
	        return 1;
	}

	/* Examine a maximum of n characters. */
	x = 1;
	while ((x <= n) && (*rs1 != 0) && (*rs2 != 0) && (tolower(*rs1) == tolower(*rs2))) {
		rs1++;
		rs2++;
		x++ ;
	}

	if ((x-1) == n)
	  rc = 0;
	else
	  rc = tolower(*rs1) - tolower(*rs2);

	return rc;
}

/*+*******************************************************************

	Function Name:		Atp_MatchStrings

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		This function matches a string against a
						NULL-terminated string table.

	Modifications:
		Who			When				Description
	----------	--------------	------------------------------
	Alwyn Teh	25 July 1992	Initial Creation

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
int Atp_MatchStrings(char *src, char *strTab[])
#else
int
Atp_MatchStrings(src, strTab)
	char *src;
	char *strTab[];
#endif
{
	register int index;
	char c;

	c = tolower(*src);

	for (index = 0; strTab[index] != NULL; index++) {
	   if (tolower(strTab[index][0]) == c) {
	     if (Atp_Strcmp(src, strTab[index]) == 0) {
	       return index;
	     }
	   }
	}

	return ATP_ERROR;
}

/*+********************************************************************

	Function Name:		Atp_StrToLower

	Copyright:			BNR Europe Limited, 1993-1994
						Bell-Northern Research
						Northern Telecom

	Description:		This function converts all the	alphabetical
						characters in a string to lower case.

	Modifications:
		Who			When					Description
	----------	----------------	--------------------------
	Alwyn Teh	27 June 1993		Initial Creation
	Alwyn Teh	25 January 1994		Redo while loop as for
									loop to fix lint complaint

*********************************************************************-*/
#if defined(__STDC__) || defined(___cplusplus)
char *Atp_StrToLower(char *string)
#else
char *
Atp_StrToLower(string)
	char *string;
#endif
{
	register char *s;

	if (string == NULL)
	  return NULL;

	for (s = string; (*s != NULL); s++) {
	   *s = tolower(*s);
	}

	return string;
}
