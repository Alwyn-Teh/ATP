/* EDITION AC07 (REL002), ITD ACST.163 (95/05/05 19:15:16) -- CLOSED */

/*+*************★**************************************************************

	Module Name:		atpexmp.c

	Copyright:			BNR Europe Limited, 1993, 1994, 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains an example program for the
						use of ATP with Tcl.

						It may also be used for executing test cases
						according to test plan SVT93005 in PLS ITD.
						These may be found in a file prefixed "atptests"
						in either shell archive (.shar or .text) or tape
						archive (.tar) format.

	Author:				Alwyn Teh,
						BNR Europe Limited,
						Concorde Road,
						Norreys Drive,
						Maidenhead,
						Berkshire SL6 4AG,
						U.K.

	Internet email: 	alteh@bnr.ca

	Notes:				Compatible with Tcl v7.3, ATP.AC08 and SLP.AC06.
						Ported to ANSI-C and C++ compilers (950307)

****************************************************************************-*/

/* Header files */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
// #include <malloc.h>
#include <math.h>
#include <sys/times.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

#include <tcl.h>

#include "slp.h"
#include "atph.h"
#include "atp2tclh.h"

/* Definitions */
#define PROMPT		"atpexmp> "
#define NELEMS(a)	(sizeof (a) / sizeof (a[0]))
#ifndef NULL
#  define NULL (void *)0
#endif

/* Declarations */
char	cmd_result[80];
void 	OutputResult	_PROTO_(( Tcl_Interp *interp, int result ));
int		AutoTest_Loop	_PROTO_(( Tcl_Interp *interp ));

/*
	Thu Aug 4 14:14:00 BST 1994 by alteh (ACST.138)

	New library function Atp_PrintfWordWrap() available for trial. Used for
	wrapping long lines and indenting wrapped lines. Does not right justify
	and space out words. Assumes no newlines or positional characters in
	string to be printed except for trailing newlines only.
*/
#if 0
extern int Atp_PrintfWordWrap
				_PROTO_((
						int (*printf_function)(char *fmtstr, ...),
												/* function to be used */
						int screen_width, /* will check COLUMN env if < 0 */
						int start_column, /* of 1st character, starts at 1 */
						int indent, /* starting column of subsequent lines */
						char *format_string,
						...	/* argO, arg1, arg2, ... argn */
				));
#endif

/* Code for memory debugging - cloned from Tcl v7.0 file tclMain.c */
#ifdef TCL_MEM_DEBUG
static char dumpFile[100];	/* Records where to dump memory allocation
 	 	 	 	 	 	 	 * information. */
static int quitFlag =0;	/* 1 means the "checkmem11 command was
						 * invoked, so the application should quit
						 * and dump memory allocation information. */
static int	CheckmemCmd _ANSI_ARGS_((ClientData clientData,
							Tcl_Interp *interp, int argc, char *argv[]));
#else

#define MALLOC(size,fn)			malloc((size))
#define REALLOC(ptr,size,fn)	realloc((ptr), (size))
#define CALLOC(nelem,elsize,fn)	calloc((nelem),(elsize))
#define FREE(ptr)				free((ptr))

#endif

/* DO NOT USE - TESTING ONLY */
EXTERN Atp_CmdRec * Atp_FindCommand _PROTO_(( void *ptr, int mode ));
						/* mode: 0 = find record, 1 = find command name */

/*
 *	Set the version information for this module.
 */
static char *atpexmp_versionInfo[] = {
#ifndef PLSID_DEFINED
	"ATP Example program \"atpexmp.c\" version ac07",
#else
	"ATP Example program \"atpexmp.c\" version %s\0\0",
#endif
	NULL
};

/*
*	A help information summary for the "number" help area.
*/
char *number_help_area_summary[] = {
		"Number parameters used in this example program can be \
		supplied in decimal, binary, octal or hexadecimal.",
		NULL
};

/*+************************************************************************

	Command:		square <number>

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	An implementation of a "square"	command.
					Accepts 1 number and returns its squared result.
					Contains vproc, parmdef and command callback.

*************************************************************************-*/
/* Square command */
char * SquareVproc _PROTO_(( void *val_ptr, Atp_BoolType isUserValue ));

ATP_DCL_PARMDEF(SquareParms) /* This is the declaration of the parmdef. */
	BEGIN_PARMS /* This marks the beginning of parameters. */
		/*
			num_def() defines a whole-number parameter - reads in a LONG,
			but this is transparent to you, the number is stored as type
			Atp_NumType. The number parameter is a signed number, if you
			wish to use an unsigned number instead, you must use
			unsigned_num_def() instead.
		*/
		num_def("X", "number to square", INT_MIN, INT_MAX, SquareVproc)

		/* "X"		is the parameter name, MUST be unique within the
					context of a ParmDef.
			"number to square" is a description of its purpose.

			INT_MIN	is the minimum input value you will allow.
			INT_MAX	is the maximum input value you will allow.
			*****************************************************************
			* If you do not wish to subject the parameter to range checking,*
			* you can indicate this by making its minimum limit less than   *
			* its maximum limit. (See examples later) e.g. specify 1,0      *
			*****************************************************************
			* ATP performs overflow and underflow detection when a number   *
			* is first read in before range-checking is performed.          *
			*****************************************************************

			An ATP jargon word for you to learn : Vproc, (Verification procedure)

			SquareVproc		is the customized parameter verification function,
							here, we must make sure that we don't end up with
							an overflow when the square is calculated and put
							into a long int. (see below)
		*/

	END_PARMS /* This marks the end of the parameters. */
ATP_END_DCL_PARMDEF /* End declaration of parmdef. */

/* VPROC - Verification procedure */
#if defined(__STDC__) || defined (__cplusplus)
char * SquareVproc(void *val_ptr, Atp_BoolType isUserValue)
#else
char * SquareVproc(val_ptr, isUserValue)	/* checks the value of X, if ok,
											   returns NULL, otherwise, returns
											   an error message
											 */
	void *val_ptr;				/* a pointer to the value X */
	Atp_BoolType isUserValue;	/* this parameter may be omitted because it is
								   used only for optional parameters whereby
								   a default value is provided when the user
								   does not supply a value.
								   (see command "substr")
	 	 	 	 	 	 	 	 */
#endif
{
	Atp_NumType X = *(Atp_NumType *)val_ptr;

	/* errmsg MUST be permanent storage as it gets returned */
	static char errmsg[80];

	if (X >= 0) { /* if positive */
	  if ((double)X > sqrt((double)INT_MAX)) {
	    goto SquareError;
	  }
	}
	else { /* X is negative */
	  if (fabs((double)X) > sqrt(fabs((double)INT_MIN))) {
	    goto SquareError;
	  }
	}
	return NULL;

	SquareError : {
		(void) sprintf(errmsg, "Square of %d causes overflow.", X);
		return(errmsg);
	}
}

/*
	Command callback function -

	Called automatically after parameters have been input correctly. The
	function is called using the Tcl host command interface. If you do
	not need to use the function arguments, they may be omitted. Here,
	interp is used by Tcl_SetResult(). If only one Tcl interpreter is
	used within the application, you may use a global Tcl_lnterp variable
	instead.

	In the command callback, you do not need to do any input, conversion,
	or verification of parameters - just do the actual work of the
	command.

	The parameter value(s) is/are accessible from ATP parameter retrieval
	functions, such as Atp_Num(), in this example. These routines take
	the name of the parameter and return its value, so they are easy to
	use.

	The command name ("square"), the command callback function, and the
	parameter definition may be bound together in the main() function
	when it calls Atp2Tcl_CreateCommand().
	*/

/*
 *	Note:	If you are not going to use the whole of the standard Tcl
 *			callback interface, you can omit the parameters because they
 *			are wasted resources. In fact, if you only use one Tcl_Interp
 *			throughout the application, why not just use a global variable?
 */
#if defined (__STDC__) || defined (__cplusplus)
Atp_Result SquareCmd(ClientData clientData, Tcl_Interp *interp,
					 int argc, char *argv[])
#else
Atp_Result SquareCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	long XSquared;

	/*
		Do the work, using functions to access previously input parm
		values. We limited the input to the range INT_MIN..INT_MAX which
		is perfectly legal, and included the SquareVproc to prevent
		overflow of long. Now, calculate the square.
	*/
	XSquared = Atp_Num("X") * Atp_Num("X");

	/*
		Print the result of the square of X in a buffer. Then, return the
		result buffer to Tcl using

		Tcl_SetResult(interp, string, freeProc); or

		Tcl_AppendResult(interp, string, string, ... , (char *) NULL);

		The buffer may be static, dynamic or volatile. You MUST tell Tcl
		what type of buffer it is, using the freeProc flags TCL_STATIC,
		TCL_DYNAMIC or TCL_VOLATILE. (see manpage Tcl_SetResult(3)).

		In this case, a static string is returned.

		If you need to use a dynamic buffer because you do not know how
		much malloc-ed space is going to be used, ATP provides
		printf-style output routines which manage dynamic buffering. You
		may also use your own routines or Tcl's if provided.

		DO NOT use standard printf() routines...etc. If you do, the
		output goes straight to the screen (e.g. stdout) instead of via
		ATP and Tcl where the result may be used by command substitution.

		The ATP output routines are Atp_DvsPrintf() or Atp_AdvPrintf()
		with Atp_AdvGets() or Atp_AdvGetsn(). [see APPENDIX A below]
	*/

	/* cmd_result is a global static character array buffer */
	(void) sprintf (cmd_result, "%ld", XSquared);

	Tcl_SetResult(interp, cmd_result, TCL_STATIC);

	/*
		Return an ATP return code - ATP_OK or ATP_ERROR.
	*/

	return ATP_OK;
}

/********************************************************************

	Command:		add <number1> <number2>

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	A command which adds 2 numbers and returns
					the result.

*********************************************************************/
/* Parameter definition. */
ATP_DCL_PARMDEF(AddParms)
	BEGIN_PARMS
		num_def("X", "first number to add",  INT_MIN, INT_MAX, NULL)
		num_def("Y", "second number to add", INT_MIN, INT_MAX, NULL)
	END_PARMS
ATP_END_DCL_PARMDEF

/* Command callback function. */
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result AddCmd(ClientData clientData, Tcl_Interp *interp,
				  int argc, char *argv[] )
#else
Atp_Result AddCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	/* Do the work, in a less roundabout way than the previous example. */
	char		AddResultBuffer[100];
	int			return_code;
	Atp_NumType	X = Atp_Num("X");
	Atp_NumType	Y = Atp_Num("Y");

	/*
		You can avoid overflow by casting the addition result as double
		and then using %g in the format string.
	*/
	double Answer = (double) X + (double) Y; /* for detecting overflow */

	if (Answer > (double) INT_MAX) {
	  (void) sprintf(AddResultBuffer,
			  	    "Result overflow: %d + %d = %g", X, Y, Answer);
	  return_code = ATP_ERROR;
	}
	else {
	  (void) sprintf(AddResultBuffer, "%d", X + Y);
	  return_code = ATP_OK;
	}

	Tcl_SetResult(interp, AddResultBuffer, TCL_VOLATILE);

	return return_code;
}

/*+******************************************************************

	Command:		sum ( <num_1> <num_2> ... <num_n> )

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	A command to calculate the summation of
					a sequence of numbers.

*******************************************************************-*/
/* Parameter definition. */
ATP_DCL_PARMDEF(SumParms)
	BEGIN_PARMS
		BEGIN_REPEAT("Numbers","List of numbers to sum",1,INT_MAX,NULL)
		/*
			BEGIN_REPEAT marks the start of a repeat block of one or more
			parameters. The numerical limits specify the minimum and
			maximum number of times the group can be repeated. In certain
			rare cases, you may wish to specify a verification function
			(NULL in this example) to control how many times the block can
			be repeated. There's more on the subject of repeat blocks later.
		*/
			num_def("Num", "a number to sum", INT_MIN, INT_MAX, NULL)
		END_REPEAT
	END_PARMS
ATP_END_DCL_PARMDEF

/* Command callback function. */
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result SumCmd(ClientData clientData, Tcl_Interp *interp,
				  int argc, char *argv[])
#else
Atp_Result SumCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	int	NumberCount, I;
	Atp_NumType	*Numbers, Sum;

	/*
		The numbers have been read into an array of Atp_NumType. Put
		the array pointer in Numbers and update NumberCount to show how
		many items are in it.
	*/
	Numbers = (Atp_NumType *) Atp_RptBlockPtr("Numbers", &NumberCount);

	/* Add the numbers, output the result, and return. */
	for (Sum = 0, I = 0; I < NumberCount; I++)
	   Sum += Numbers[I];

	(void) sprintf(cmd_result, "%d", Sum);

	Tcl_SetResult(interp, cmd_result, TCL_STATIC);

	return ATP_OK;
}

/*+*************************************************************************

	Command:		substr <Str> <Start> [<End>]

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Outputs a substring of the given string (Str),
					from the specified Start to End (both whole
					numbers). End is optional and defaults to the
					end of the string. The first character in the
					string is character index 0.

**************************************************************************-*/

/* Help information for the manpage of the substr command. */
static char *substr_manpg_header[] = {
	"Outputs a substring of the given string (Str), from the specified",
	"Start to End (both whole numbers). End is optional and defaults to",
	"the end of the string. The first character in the string is",
	"character index 0.",
	NULL
};

/* Help information in the form of an example for the manpage. */
static char *substr_manpg_footer[] = {
	"Example:",
	"	atpexmp> substr abcdefg 3 5",
	"	def",
	"	atpexmp> substr abcdefg 2",
	"	cdefg",
	NULL
};

/*
	Vprocs:

	Parameter verification functions - we need to check that both Start
	and End fall within the bounds of the given string and that End is >=
	Start. We also need to default End, if it is not specified.

	Note that if End defaulted to a constant value, we could specify the
	default value in the parameter definition. However, since it
	defaults to something variable, known only at command execution time,
	we must set the default value in the verification function. This is
	demonstrated here to show how it is done - however, you should
	usually avoid this practice: run-time-variable defaults can confuse
	users; constant defaults specified in parmdefs are better.

	Every verification function receives a pointer to the value that has
	been input and a boolean saying whether the value has been typed or
	is [meant to be] defaulted. The vproc returns a character string
	indicating what is wrong with the parameter value - if the parameter
	is OK, it should return NULL.

	By the way, 'C' does not provide the boolean type so ATP has
	typedef-ed its own and it's called Atp_BoolType, so as to avoid
	clashing with other people's typedefs of generic names e.g. boolean.

	IMPORTANT:
	=========
	If the parameter you are going to check is a string, then it will be
	of type char ** because it is a pointer to a string of type char *.

	Also, the string, unlike other value parameters, is of variable
	length, and resides in dynamically allocated memory. It is NOT the
	same memory as that of the string typed in by the user. Therefore,
	if you intend to change the string, you MUST make sure that you DO
	NOT write beyond its limit. Malloc keeps track of how much store it
	provides for the string so you can call realloc () to do this.
*/

/* Vproc */
#if defined(__STDC__) || defined(__cplusplus)
char* VerifyStartValue( void * ValuePtr, Atp_BoolType ValuePresent )
#else
char* VerifyStartValue(ValuePtr, ValuePresent)
	void *	ValuePtr;
	Atp_BoolType ValuePresent;
#endif
{
	Atp_NumType Start = *(Atp_NumType *)ValuePtr;
	if ( Start >= strlen(Atp_Str("Str")) )
	  return("Value must be within bounds of <Str>");
	else
	  return NULL;
}

/* Vproc */
#if defined(__STDC__) || defined(__cplusplus)
char* VerifyEndValue ( void * ValuePtr, Atp_BoolType ValuePresent )
#else
char* VerifyEndValue(ValuePtr, ValuePresent)
	void *	ValuePtr;
	Atp_BoolType ValuePresent;
#endif
{
	Atp_NumType End = *(Atp_NumType *)ValuePtr;
	if (ValuePresent) {
	  if ( End >= strlen(Atp_Str("Str")) )
	    return("Value must be within bounds of <Str>");
	  else
	  if ( End < Atp_Num("Start") )
	    return("Value must be greater than value of <Start>");
	  else
	    return NULL;
	} else {
	  /* Override the hard-coded default value specified in the parmdef. */
	  *(Atp_NumType *)ValuePtr = strlen(Atp_Str("Str")) - 1;
	  return NULL;
	}
}

/* Parameter definition. */
ATP_DCL_PARMDEF(SubStrParms)
	BEGIN_PARMS
		str_def("Str", "string to slice up", 0, INT_MAX, NULL)
		num_def("Start", "start of substring", 0, INT_MAX, VerifyStartValue)
		opt_num_def("End",	"end of substring, defaults to end of string",
					INT_MAX, 0, INT_MAX, VerifyEndValue)
					/*
						We do not know how long the string is going to be so
						the default value is INT_MAX being a maximum. We can
						then use the VerifyEndValue() function to perform
						further checking.
					*/
	END_PARMS
ATP_END_DCL_PARMDEF

/* Command callback function. */
#if defined (__STDC__) || defined(__cplusplus)
Atp_Result SubstrCmd(ClientData clientData, Tcl_Interp *interp,
					 int argc, char *argv[])
#else
Atp_Result SubstrCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	char *		Str		= Atp_Str("Str");
	Atp_NumType	Start	= Atp_Num("Start");
	Atp_NumType	End		= Atp_Num ("End");
	char *		substr	= NULL;

	/*
		Parameters are all verified, including verification functions
		called, so now we just do the work.

		We can update the values that have been input - in this case,
		that will save us the trouble of copying a substring of Str
		to another char array.
	 */

	Str[End+1] = '\0';	/* end marker for character strings */

	/* Use dynamic buffer because maximum length is INT_MAX. */
	(void) Atp_DvsPrintf(&substr, "%s", Str+Start);

	Tcl_SetResult(interp, substr, free);

	return ATP_OK;
}

/*+*****************************************************************

	Command:		matchpaint <colour> {
							"brand"		<brandname>
							"volume"	<quantity> <units>
							"texture"	<texture_type>
					}

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Select paint by matching colour, then
					brand, volume and texture attributes.

					This command illustrates the use of
					keywords as a standalone parameter (e.g.
					colour), and as a selector field for a
					CHOICE parameter (e.g. brand, ... etc.).

******************************************************************-*/
/*
	Keyword tables:

	These keyword tables are used in the parmdef (below) to specify a
	list of allowable strings for a keyword parameter. They can also be
	used for output (as seen in the command callback below).

	Atp_KeywordTab is an array of Atp_KeywordType structures. The first
	field of this structure is of type char * and the second of type
	Atp_NumType. We call this number the KeyValue of the keyword you
	have defined. You can use #defined numbers instead and in any order,
	they can be negative too. (Basically, it's NOT the index of the
	element in the table.)

	An optional extra field follows, the keyword description. This is
	used for help information purposes. The "man <command>" and
	"<command> ?" commands will display them if defined.

	Lastly, but not least, the Atp_KeywordTab MUST be terminated by a
	NULL keyword (Doesn't matter what the KeyValue is, can even be
	omitted in which case the C compiler initializes any trailing fields
	to zero.)

	(Note: Use Atp_KeywordType [] instead of Atp_KeywordTab.)
*/
#define RED		0
#define ORANGE	1
#define YELLOW	2
#define GREEN	3
#define BLUE	4
#define INDIGO	5
#define VIOLET	6

Atp_KeywordType RainbowColour[] = {
		{"Red",		RED},
		{"Orange",	ORANGE},
		{"Yellow",	YELLOW},
		{"Green",	GREEN},
		{"Blue",	BLUE},
		{"Indigo",	INDIGO},
		{"Violet",	VIOLET},
		{NULL}
};

Atp_KeywordType BrandNameList [] = {
		{"Ace",		0,	"The Best"},
		{"Lux",		1,	"Luxurious"},
		{"Glare",	2,	"Flashy"},
		{"Buddy",	3,	"Friendly"},
		{"Sparkle",	4,	"Shiny"},
		{NULL,		5}
};

#define L		0
#define PT		1
#define BBL		2
#define OINK	3

Atp_KeywordType UnitList [] = {
		{"l",		L,		"litres"},
		{"pt",		PT,		"pints"},
		{"bbl",		BBL,	"balloons"},
		{"oink",	OINK,	"oinks"},
		{NULL}
};

#define GLOSSY	123
#define MATT	567

Atp_KeywordType TextureAppearance[] = {
		{"glossy",	GLOSSY},
		{"matt",	MATT},
		{NULL}
};

/*
 *	Parameter definition table for matchpaint
 */
typedef struct {
		Atp_NumType quantity, units;
} MatchpaintVolumeOvly;

MatchpaintVolumeOvly DefaultVolume = {5, L};

#define BRAND	707
#define VOLUME	737
#define TEXTURE	747

/*
 *	Vproc to check the matchpaint attributes.
 *
 *	The CHOICE value is represented by the structure Atp_ChoiceDescriptor.
 *	(see atph.h for its field definitions)
 */
#if defined(__STDC__) || defined(__cplusplus)
char * CheckAttributesVproc( void *valPtr, Atp_BoolType isUserValue )
#else
char * CheckAttributesVproc(valPtr, isUserValue)
	void			*valPtr;
	Atp_BoolType	isUserValue;
#endif
{
	Atp_ChoiceDescriptor *ChoiceDescPtr = (Atp_ChoiceDescriptor *)valPtr;

	if (ChoiceDescPtr != NULL) {
	  switch(ChoiceDescPtr->CaseIndex) {
	  	case 0: ChoiceDescPtr->CaseValue = BRAND;	break;
		case 1: ChoiceDescPtr->CaseValue = VOLUME;	break;
		case 2: ChoiceDescPtr->CaseValue = TEXTURE;	break;
		default:break;
	  }
	  return NULL; /* MUST return NULL - don’t forget!!! */
	}
	else
	  return "matchpaint vproc cannot modify CaseValue";
}

ATP_DCL_PARMDEF(MatchPaintParms)
	BEGIN_PARMS
		keyword_def("colour","colour of paint",RainbowColour,NULL)
		BEGIN_CHOICE("paint_attributes","brand, volume or texture",
					 CheckAttributesVproc)
		/*
			BEGIN_CHOICE marks the start of a choice of parameters. The
			user types the name of the parameter (s)he wants to specify,
			("brand", "volume" or "texture" in this example) followed by
			the parameter value(s). If you wish to have the choice
			repeated (e.g. user can specify any number of the keyword
			parameters) place the choice within a repeat block
			(BEGIN_REPEAT).
		*/
			keyword_def("brand","brand name",BrandNameList,NULL)
			BEGIN_OPT_LIST("volume","desired amount", &DefaultVolume)
			/*
				BEGIN_LIST is for a choice that contains more than one
				sub-parameter value. Valid input for this example could
				be "volume 2 1" or "volume 5 bbl". Here, an optional
				list is used instead using BEGIN_OPT_LIST and
				END_OPT_LIST. The default list value is simply a
				structure containing the default values of the enclosed
				parameters.

				Note that BEGIN_LIST does NOT have a Vproc argument
				because you can check the list of parameters using the
				vproc of the last parameter in the list. We would
				confuse you to put it in!

				See also use of BEGIN_CASE()/END_CASE and CASE(). With
				CASE, you can specify CaseValues just like KeyValues.
			*/
				num_def("quantity","number of units",1,5,NULL)
				keyword_def("units","litres, pints, barrels, hogsheads",
							UnitList,NULL)
			END_OPT_LIST
			keyword_def("texture", "glossy or matt",TextureAppearance,NULL)
		END_CHOICE
	END_PARMS
ATP_END_DCL_PARMDEF

/* Command callback function */
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result MatchPaintCmd(ClientData clientData, Tcl_Interp *interp,
						 int argc, char *argv[] )
#else
Atp_Result MatchPaintCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	char * string = NULL; /* for result string */

	/*
		Since we don't actually have access to any paint-matching
		software, all we do in this example is echo the parameters that
		were typed by the user.

		Here, output is accumulated using dynamic buffering because it
		is verbose and contains several lines of text.
	*/
	(void) Atp_AdvPrintf("Paint matched with the following attributes:\n");

	/*
		Note that keyword and choice parameters can be seen as numbers
		(using Atp_Num() for KeyValue or Atp_Index() for index), or as
		strings (using Atp_Str()), whichever is more convenient to the
		application programmer. Both methods are demonstrated here.
	*/
	(void) Atp_AdvPrintf("Colour = %s\n",
						 RainbowColour[Atp_Num("colour")].keyword);

	/*
		If you are not using CASE or BEGIN_CASE/END_CASE, use
		Atp_Index() to recall selected choice branch. If the choice
		parameters were specified using CASE, Atp_Num() can be used to
		recall the CaseValue. This is useful if you have many choice
		branches and positions change often under code maintenance.
		However, in this example, the vproc cheats by modifying the
		CaseValue in the Atp_ChoiceDescriptor.
	*/
	switch (Atp_Num("paint_attributes")) {
		case BRAND:
				(void) Atp_AdvPrintf("Brand = %s",
									 BrandNameList[Atp_Num("brand")].keyword);
				break;
		case VOLUME: {
				/*
				 *	Unfortunately, you cannot use Atp_Num() and Atp_Str()
				 *	or any parameter retrieval functions on parameters or
				 *	constructs enclosed within the optional LIST
				 *	construct when the optional default LIST value is
				 *	used. This is because no index referencing is kept
				 *	in the internal parmstore and hence no searching by
				 *	name can be done. However, you can detect when the
				 *	default value is used as shown below!
				 *
				 *	Atp_ParmPtr is used to obtain a pointer to any
				 *	parameter or construct. You can traverse a complex
				 *	parmstore using pointers and overlay structures that
				 *	are parallel to the parmdef structure. However, in
				 *	complex nested parmdefs, the parmstore may look more
				 *	fragmented due to the various possible paths through
				 *	the parmdef during command processing. If you get
				 *	lost, use Atp_ResetParmPtr to get back to the
				 *	beginning. (Sorry I have not had time to construct a
				 *	complicated parmdef to demonstrate this.)
				 */
				MatchpaintVolumeOvly *default_ptr;
				default_ptr = *(MatchpaintVolumeOvly **)Atp_ParmPtr("volume");

				if (default_ptr == &DefaultVolume) {
				  (void) Atp_AdvPrintf("Volume = %d %s",
										DefaultVolume.quantity,
										UnitList[DefaultVolume.units].KeywordDescription);
				}
				else {
				  /* Cross check with Atp_ParmPtr/overlay and Atp_Num/Atp_Str.*/
				  if (default_ptr->quantity == Atp_Num("quantity") &&
						  default_ptr->units == Atp_Num("units")) {
					(void) Atp_AdvPrintf("Volume = %d %s",
							Atp_Num("quantity"), Atp_Str("units"));
				  }
				}
				break;
		}
		case TEXTURE:
				(void) Atp_AdvPrintf("Texture = %s",Atp_Str("texture"));
				break;
		default: break;
	}

	/*
		Obtain the Atp_AdvPrintf output and assign to a pointer.

		There's no need to append a newline at the end because the
		result string is printed to the screen by the frontend of the
		application with a newline character.
	*/
	string = Atp_AdvGets();

	Tcl_SetResult(interp, string, free);

	return ATP_OK;
}

/*+****************************************************************************

	Command:		echohex <databytes>

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Demo of databytes for input of hexadecimal strings.
					Echoes databytes as ASCII characters.

*****************************************************************************-*/

/* Encode "DEFAULT" in the default value. */
Atp_ByteType Default_DataBytes[] = {0x44,0x45,0x46,0x41,0x55,0x4c,0x54};

/*
 *	The databytes parameter is stored as a Atp_DataDescriptor structure.
 *	(see atph.h for the field definitions)
 *
 *	Assign the default to a Atp_DataDescriptor.
 */
Atp_DataDescriptor Default_DataBytesVar = {7, Default_DataBytes};

#if defined(__STDC__) || defined(__cplusplus)
static char * CheckPrintable( void *valPtr, Atp_BoolType isUserValue )
#else
static char * CheckPrintable(valPtr, isUserValue)
	Atp_DataDescriptor *valPtr;
	Atp_BoolType isUserValue
#endif
{
	Atp_DataDescriptor *dataPtr = (Atp_DataDescriptor *)valPtr;
	Atp_ByteType *databytes = NULL;
	int x;

	if (dataPtr != NULL) {
	  databytes = (Atp_ByteType *)dataPtr->data;
	  for (x = 0; x < dataPtr->count; x++) {
	     if (!isprint(databytes[x]))
	       return "Hexadecimal string contains non-printable ASCII data.";
	  }
	}

	return NULL;
}

/* Parmdef */
ATP_DCL_PARMDEF(DataBytesParms)
	BEGIN_PARMS
		opt_data_bytes_def("DataBytesVar","hexadecimal string",
						   &Default_DataBytesVar,1,0,CheckPrintable)
		/*
			If the range for the databytes parameter is 1 and 0,
			ATP will not restrict the maximum number of bytes
			entered.
		 */
	END_PARMS
ATP_END_DCL_PARMDEF

/* Routine for printing databytes in ASCII. */
#if defined(__STDC__) || defined(__cplusplus)
char * PrintAsciiDatabytes( Atp_ByteType *dataBytes, Atp_UnsNumType len )
#else
char * PrintAsciiDatabytes(dataBytes, len)
	Atp_ByteType	*dataBytes;
	Atp_UnsNumType	len;
#endif
{
	char *string = NULL;
	register int x, y;
	char fmtStr[20];

	/* If databytes contain any NULL bytes, remove them. */
	for (x=0, y=0; x < len; x++) {
	   if (dataBytes[x] != 0)
		 dataBytes[y++] = dataBytes[x];
	}
	(void) sprintf (fmtStr, "%%.%ds", y); /* make format string */
	(void) Atp_DvsPrintf(&string, fmtStr, (char *)dataBytes);
	return string;
}

/* Command callback */
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result EchoHexToAsciiCmd(ClientData clientData, Tcl_Interp *interp,
							 int argc, char *argv[])
#else
Atp_Result EchoHexToAsciiCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	Atp_UnsNumType len;
		/*	Unsigned number because it will never be negative,
			you can use anything else provided that it is the
			same size as a long. */

	Atp_ByteType *dataBytes;
		/*	Use this type to mean a byte, nevertheless, a byte
			is always a byte and that's 8 bits. */

	Atp_DataDescriptor	desc;
	char *string;

	/* Method 1 */
	dataBytes = Atp_DataBytes("DataBytesVar",&len);

	/* Method 2 */
	desc = Atp_DataBytesDesc ("DataBytesVar");
	dataBytes = (dataBytes == desc.data) ? dataBytes : NULL;

	/*
		This contains 2 implementations. The first displays in
		verbose hex and ASCII formats, and the other in straight
		ASCII. Choose whichever one you like.
	 */
#define DISPLAY_HEX_AND_ASCII
#ifdef DISPLAY_HEX_AND_ASCII
	{
	unsigned char unitByte;
	int x;
	(void) Atp_AdvPrintf("Hex : ");
	for (x=0; x < len; x++) {
	   unitByte = dataBytes [x];
	   (void) Atp_AdvPrintf("%2.2x ", unitByte);
	}
	(void) Atp_AdvPrintf("\n");
	(void) Atp_AdvPrintf("Ascii: ");
	for (x=0; x < len; x++) {
	   unitByte = dataBytes[x];
	   (void) Atp_AdvPrintf("%2.2c ", unitByte);
	}
	string = Atp_AdvGets();
	}
#else
	string = PrintAsciiDatabytes(dataBytes, len);
#endif

	Tcl_SetResult(interp, string, free);

	return ATP_OK;
}

/*+****************************************************************************

	Command:		echobcd <bcd_digits_1> [ <bcd_digits_2> ]

	Copyright:		BNR Europe Limited, 1995
					Bell-Northern Research / BNR
					Northern Telecom / NORTEL

	Description:	Demo of bcd_digits for input of BCD digit strings.
					Echoes BCD digits as typed.

*****************************************************************************-*/
Atp_ByteType def_bcd_digs[] = {0x10,0x32,0x54,0x76,0x98};

#if defined(__STDC__) || defined(__cplusplus)
static char * bcd_vproc( void *valPtr, Atp_BoolType isUserValue )
#else
static char * bcd_vproc(valPtr, isUserValue)
	Atp_DataDescriptor *valPtr;
	Atp_BoolType isUserValue;
#endif
{
	/* Check if bcd_digits_1 and bcd_digits_2 are the same. */
	Atp_DataDescriptor bcd1Desc;
	Atp_DataDescriptor *bcd2Ptr = (Atp_DataDescriptor *)valPtr;
	Atp_ByteType *bcd_digits1 = NULL;
	Atp_ByteType *bcd_digits2 = NULL;
	int x, digit1, digit2;

	if (bcd2Ptr != NULL)
	{
	  bcd1Desc = Atp_BcdDigitsDesc("bcd1");
	  if (bcd1Desc.count == bcd2Ptr->count)
	  {
		bcd_digits1 = (Atp_ByteType *)bcd1Desc.data;
		bcd_digits2 = (Atp_ByteType *)bcd2Ptr->data;
		for (x = 0; x < bcd2Ptr->count; x++)
		{
		   digit1 = (x & 1) ? bcd_digits1[x/2] >> 4 :
		   bcd_digits1[x/2] & 0x0F;
		   digit2 = (x & 1) ? bcd_digits2[x/2] >> 4 :
		   bcd_digits2[x/2] & 0x0F;
		   if (digit1 != digit2)
		     break;
		}
		if (x == bcd2Ptr->count)
		  return "<bcd1> and <bcd2> are the same";
	  }
	}

	return NULL;
}

Atp_DataDescriptor default_bcd_digits = {sizeof(def_bcd_digs)*2,def_bcd_digs};

ATP_DCL_PARMDEF(EchoBcdParms)
	BEGIN_PARMS
		bcd_digits_def("bcd1","First set of BCD digits",1,20,NULL)
		opt_bcd_digits_def("bcd2","Second set of BCD digits with default value",
							&default_bcd_digits,1,0,bcd_vproc)
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result EchoBcdCmd(ClientData clientData, Tcl_Interp *interp,
					  int argc, char *argv[])
#else
Atp_Result EchoBcdCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	char *string1 = NULL;
	char *string2 = NULL;
	char *string = NULL;

	int nibbles, digit;

	Atp_DataDescriptor bcd_1_desc;

	Atp_ByteType *bcd_digits2 = NULL;
	Atp_UnsNumType no_of_digits2 = 0;

	/* Accessing stored BCD digits by name. */
	bcd_1_desc = Atp_BcdDigitsDesc("bcd1");					/* Method 1 */
	bcd_digits2 = Atp_BcdDigits("bcd2", &no_of_digits2);	/* Method 2 */

	/* (1) Print BCD digits in ASCII format using built-in function. */
	string1 = Atp_DisplayBcdDigits(bcd_1_desc);

	/* (2) Print BCD digits in ASCII format, own way. */
	string2 = (char *)CALLOC(no_of_digits2 + 1, sizeof(char), NULL);

	for (nibbles = 0; nibbles < no_of_digits2; nibbles++)
	{
	   digit = (nibbles & 1) ? bcd_digits2[nibbles/2] >> 4 :
			   	   	   	   	   bcd_digits2[nibbles/2] & 0x0F;
	   string2[nibbles] = (digit >9) ? digit-10+'a' : digit+'0';
	}
	string2[nibbles] = '\0';

	/* Print both BCD strings. */
	Atp_DvsPrintf(&string, "BCD1 = %s\nBCD2 = %s", string1, string2);

	if (string1 != NULL) FREE(string1);
	if (string2 != NULL) FREE(string2);

	Tcl_SetResult(interp, string, free);

	return ATP_OK;
}

/*+*******************************************************************

	Command:		weekday <weekday_narae>

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	A command to test the keyword	feature.
					Also used is a vproc.

********************************************************************-*/
/* KeyValues */
#define MON 111
#define TUE 222
#define WED 333
#define THU 444
#define FRI 555
#define SAT 666
#define SUN 777

Atp_KeywordType WeekDaysNames[] = {
		{"Monday",		MON},
		{"Tuesday",		TUE},
		{"Wednesday",	WED},
		{"Thursday",	THU},
		{"Friday",		FRI},
		{"Saturday",	SAT},
		{"SUNDAY",		SUN},
		{"Beginning of the working week",	MON},
		{"End of the working week",			FRI},
		{"Beginning of the weekend",		SAT},
		{"End of the weekend",				SUN},
		{NULL,			-999}
};

char * WeekdayVproc _PROTO_(( void *val_ptr, Atp_BoolType isUserValue ));

/* Parmdef */
ATP_DCL_PARMDEF(WkDayPD)
	BEGIN_PARMS
		opt_keyword_def("weekdays","names of weekdays",SUN,
						WeekDaysNames,WeekdayVproc)
	END_PARMS
ATP_END_DCL_PARMDEF

/* vproc */
#if defined(__STDC__) || defined(__cplusplus)
char * WeekdayVproc( void *valPtr, Atp_BoolType isUserValue )
#else
char * WeekdayVproc(valPtr, isUserValue)
	void			*valPtr;
	Atp_BoolType	isUserValue;
#endif
{
	Atp_NumType pWkDay = *(Atp_NumType *)valPtr;

	if (isUserValue) {
	/*
	 * If you want to change the KeyValue, here's how you do it.
	 */
	  if ((pWkDay == SAT) || (pWkDay == SUN)) {
		pWkDay = MON;
	    return ("Closed at weekend, come back on Monday.");
	  }
	}
	return NULL;
}

/* Command callback */
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result WeekdayCmd(ClientData clientData, Tcl_Interp *interp,
					  int argc, char *argv[])
#else
Atp_Result WeekdayCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	char * string = NULL;

	(void) Atp_AdvPrintf("Keyword Index  = %d\n",	Atp_Index("weekdays"));
	(void) Atp_AdvPrintf("Keyword Value  = %d\n",	Atp_Num("weekdays"));
	(void) Atp_AdvPrintf("Keyword string = \"%s\"", Atp_Str("weekdays"));

	string = Atp_AdvGets();

	Tcl_SetResult(interp, string, free);

	return ATP_OK;
}

/*+*******************************************************************

	Command:		bool <boolean_keyword>

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command to test the boolean parameter.

********************************************************************-*/
struct {
	char *boolean_string;
	int tested;
} checklist [] = {
	{"0", 0},		{"1", 0},
	{"F", 0},		{"T", 0},
	{"N", 0},		{"Y", 0},
	{"False", 0},	{"True", 0},
	{"No", 0},		{"Yes", 0},
	{"Off", 0},		{"On", 0},
	{"Bad", 0},		{"Good", 0}
};

#if defined(__STDC__) || defined(__cplusplus)
char * CheckBool( void *valPtr, Atp_BoolType isUserValue )
#else
char * CheckBool{valPtr, isUserValue)
	void			*valPtr;
	Atp_BoolType	isUserValue;
#endif
{
	Atp_BoolType boolValue = * (Atp_BoolType *)valPtr;
	int x;
	if ((boolValue == 6 || boolValue == 1) && isUserValue) {
	  for (x = 0; x < NELEMS (checklist); x++)
	     if (!checklist[x].tested) /* if 0 found => not all tested */
	       return NULL;
	  return "All boolean keywords tested. Stop using \"bool\" command.";
	}
	return NULL;
}

#if defined(__STDC__) || defined (__cplusplus)
Atp_Result ResetBooleanVprocChecklist(ClientData clientData, Tcl_Interp *interp,
									  int argc, char *argv [])
#else
Atp_Result ResetBooleanVprocChecklist(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int	argc;
	char *argv[];
#endif
{
	int x;
	for (x = 0; x < NELEMS(checklist); x++)
	   checklist[x].tested = 0;
	return ATP_OK;
}

ATP_DCL_PARMDEF(BOOL_PD)
	BEGIN_PARMS
		opt_bool_def("BooleanParm", "Boolean Parameter", 1, CheckBool)
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined(__STDC__) || defined (__cplusplus)
Atp_Result TestBooleanCmd(ClientData clientData, Tcl_Interp *interp,
						  int argc, char *argv[])
#else
Atp_Result TestBooleanCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	int		boolVal = (int) Atp_Bool("BooleanParm");
	char *	string;
	int		x;

	(void) Atp_DvsPrintf(&string, "Boolean value = %d (%s)",
						 boolVal,
						 ((boolVal == 1) ? "TRUE" : "FALSE"));

	/* Log boolean string tested. */
	for (x = 0; x < NELEMS(checklist); x++) {
	   if (Atp_Strcmp(checklist[x].boolean_string,
			   	   	  Atp_Str("BooleanParm")) == 0)
		 checklist[x].tested = 1;
	}

	Tcl_SetResult(interp, string, free);

	return ATP_OK;
}

/*+**************************************************************************

	Command:		num <number>

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command to test the overflow detection of numbers.

****************************************************************************-*/
ATP_DCL_PARMDEF(CHK_NUM_PD)
	BEGIN_PARMS
		opt_num_def("number","whole number",0,1,0,NULL)
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result CheckNumCmd(ClientData clientData, Tcl_Interp *interp,
					   int argc, char *argv[])
#else
AtpJResult CheckNumCmd(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp * interp;
	int argc;
	char *argv[];
#endif
{
	Atp_NumType	num = Atp_Num("number");
	char		*string;

	(void) Atp_DvsPrintf(&string,
						 "number = (decimal)%d, (octal)%o, (hex)%x",
						 num, num, num);

	Tcl_SetResult(interp, string, free);

	return ATP_OK;
}

/*+**************************************************************************

	Command:		unum <unsigned_number>

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command to check out overflow	detection of
					unsigned numbers.

****************************************************************************-*/
#if defined(__STDC__) || defined (__cplusplus)
char * UnumVproc( void *valPtr, Atp_BoolType isDserValue )
#else
char * UnumVproc(valPtr, isUserValue)
	void			*valPtr;
	Atp_BoolType	isUserValue;
#endif
{
	Atp_UnsNumType *unumPtr = (Atp_UnsNumType *)valPtr;

	if (unumPtr != NULL) {
	  if (*unumPtr == 65904358)
	    return "Hey, that's my ESN number! Try another number.";
	}

	return NULL;
}

ATP_DCL_PARMDEF(CHK_UNUM_PD)
	BEGIN_PARMS
		opt_unsigned_num_def("unumber","whole unsigned number",0,1,0,UnumVproc)
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined (__STDC__) || defined (__cplusplus)
Atp_Result CheckUnsNumCmd(ClientData clientData, Tcl_Interp *interp,
						  int argc, char *argv[])
#else
Atp_Result CheckUnsNumCmd(clientData, interp, argc, argv)
	ClientData clientData
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	Atp_UnsNumType	unum = Atp_UnsignedNum("unumber");
	char			*string;

	(void)Atp_DvsPrintf(&string,
						"unsigned num = (decimal)%u, (octal)%o, (hex)%x",
						unum, unum, unum);

	Tcl_SetResult(interp, string, free);

	return ATP_OK;
}

/*+***************************************************************************

	Command:		plot [ ( <nl> <n2> <n3> ... <nn> ) ]

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command to test the repeat block feature.
					Checks that the repeat block terminator is
					guaranteed to be the size of the parameters
					enclosed within BEGIN_REPEAT and END_REPEAT at
					the next nested level.

****************************************************************************-*/

#define	MAX_COORDS			INT_MAX
#define	X_WIDTH				80
#define	Y_DEPTH				24
#define	ARRAY_TERMINATOR	-999
#define	DynamicArray(x,y)	((Max_X + 1) * y + x)

typedef struct xy_coord {Atp_NumType x, y;} XY_CoordsStruct;

/* Initialize default repeat block values with digitized images. */
XY_CoordsStruct DemoData1 [] = {
	14, 1,	16, 1,	18, 1,	24, 1,	26, 1,	28,	1,	12, 2, 20, 2, 22,	2, 30,
	2, 11,	3, 21,	3, 31,	3, 11,	4, 31,	4,	11,	5, 31, 5, 12,	6,	30, 6,
	13, 7,	29, 7,	15, 8,	27, 8,	17, 9,	25,	9,	19, 10, 23, 10, 21, 11,
	0, 0 /* plus room for terminator pair */
};

XY_CoordsStruct DemoData2[] = {
	{39, 0}, {40, 0}, {42, 0}, {43, 0}, {44, 0}, {45, 0}, {46, 0},
	{47, 0}, {48, 0}, {49, 0}, {50, 0}, {51, 0}, {37, 1}, {38, 1},
	{39, 1}, {40, 1}, {42, 1}, {43, 1}, {44, 1}, {45, 1}, {46, 1},
	{47, 1}, {48, 1}, {49, 1}, {52, 1}, {53, 1}, {36, 2}, {37, 2},
	{38, 2}, {39, 2}, {40, 2}, {42, 2}, {43, 2}, {44, 2}, {45, 2},
	{46, 2}, {47, 2}, {50, 2}, {51, 2}, {52, 2}, {53, 2}, {8, 3},
	{9, 3}, {10, 3}, {11, 3}, {12, 3}, {13, 3}, {16, 3}, {17, 3},
	{22, 3}, {23, 3}, {25, 3}, {26, 3}, {27, 3}, {28, 3}, {29, 3},
	{30, 3}, {34, 3}, {35, 3}, {38, 3}, {39, 3}, {40, 3}, {42, 3},
	{43, 3}, {44, 3}, {45, 3}, {48, 3}, {49, 3}, {50, 3}, {51, 3},
	{52, 3}, {53, 3}, {8, 4}, {9, 4}, {10, 4}, {11, 4}, {12, 4},
	{13, 4}, {14, 4}, {16, 4}, {17, 4}, {22, 4}, {23, 4}, {25, 4},
	{26, 4}, {27, 4}, {28, 4}, {29, 4}, {30, 4}, {31, 4}, {34, 4},
	{35, 4}, {36, 4}, {37, 4}, {40, 4}, {42, 4}, {43, 4}, {46, 4},
	{47, 4}, {48, 4}, {49, 4}, {50, 4}, {51, 4}, {52, 4}, {53, 4},
	{8, 5}, {9, 5}, {13, 5}, {14, 5}, {16, 5}, {17, 5}, {18, 5},
	{22, 5}, {23, 5}, {25, 5}, {26, 5}, {30, 5}, {31, 5}, {33, 5},
	{34, 5}, {35, 5}, {36, 5}, {37, 5}, {38, 5}, {39, 5}, {44, 5},
	{45, 5}, {46, 5}, {47, 5}, {48, 5}, {49, 5}, {50, 5}, {51, 5},
	{52, 5}, {53, 5}, {8, 6}, {9, 6}, {10, 6}, {11, 6}, {12, 6},
	{13, 6}, {16, 6}, {17, 6}, {18, 6}, {19, 6}, {22, 6}, {23, 6},
	{25, 6}, {26, 6}, {27, 6}, {28, 6}, {29, 6}, {30, 6}, {33, 6},
	{34, 6}, {35, 6}, {36, 6}, {37, 6}, {38, 6}, {39, 6}, {40, 6},
	{42, 6}, {43, 6}, {44, 6}, {45, 6}, {46, 6}, {47, 6}, {48, 6},
	{49, 6}, {50, 6}, {51, 6}, {52, 6}, {53, 6}, {8, 7}, {9, 7},
	{10, 7}, {11, 7}, {12, 7}, {13, 7}, {14, 7}, {16, 7}, {17, 7},
	{19, 7}, {20, 7}, {22, 7}, {23, 7}, {25, 7}, {26, 7}, {27, 7},
	{28, 7}, {29, 7}, {30, 7}, {31, 7}, {33, 7}, {34, 7}, {35, 7},
	{36, 7}, {37, 7}, {38, 7}, {39, 7}, {40, 7}, {42, 7}, {43, 7},
	{44, 7}, {45, 7}, {46, 7}, {47, 7}, {48, 7}, {49, 7}, {50, 7},
	{51, 7}, {52, 7}, {53, 7}, {8, 8}, {9, 8}, {13, 8}, {14, 8},
	{16, 8}, {17, 8}, {20, 8}, {21, 8}, {22, 8}, {23, 8}, {25, 8},
	{26, 8}, {30, 8}, {31, 8}, {33, 8}, {34, 8}, {35, 8}, {36, 8},
	{37, 8}, {38, 8}, {39, 8}, {40, 8}, {42, 8}, {43, 8}, {44, 8},
	{45, 8}, {46, 8}, {47, 8}, {48, 8}, {49, 8}, {50, 8}, {51, 8},
	{52, 8}, {53, 8}, {8, 9}, {9, 9}, {13, 9}, {14, 9}, {16, 9},
	{17, 9}, {21, 9}, {22, 9}, {23, 9}, {25, 9}, {26, 9}, {30, 9},
	{31, 9}, {33, 9}, {34, 9}, {35, 9}, {36, 9}, {37, 9}, {38, 9},
	{39, 9}, {40, 9}, {42, 9}, {43, 9}, {44, 9}, {45, 9}, {46, 9},
	{47, 9}, {48, 9}, {49, 9}, {50, 9}, {51, 9}, {52, 9}, {53, 9},
	{8, 10}, {9, 10}, {10, 10}, {11, 10}, {12, 10}, {13, 10}, {14,
	10}, {16, 10}, {17, 10}, {22, 10}, {23, 10}, {25, 10}, {26, 10},
	{30, 10}, {31, 10}, {34, 10}, {35, 10}, {36, 10}, {37, 10}, {38,
	10}, {39, 10}, {40, 10}, {42, 10}, {43, 10}, {44, 10}, {45, 10},
	{46, 10}, {47, 10}, {48, 10}, {49, 10}, {50, 10}, {51, 10}, {52,
	10}, {8, 11}, {9, 11}, {10, 11}, {11, 11}, {12, 11}, {13, 11},
	{16, 11}, {17, 11}, {23, 11}, {25, 11}, {26, 11}, {30, 11}, {31,
	11}, {35, 11}, {36, 11}, {37, 11}, {38, 11}, {39, 11}, {40, 11},
	{42, 11}, {43, 11}, {44, 11}, {45, 11}, {46, 11}, {47, 11}, {48,
	11}, {49, 11}, {50, 11}, {51, 11}, {36, 12}, {37, 12}, {38, 12},
	{39, 12}, {40, 12}, {42, 12}, {43, 12}, {44, 12}, {45, 12}, {46,
	12}, {47, 12}, {48, 12}, {49, 12}, {50, 12}, {38, 13}, {39, 13},
	{40, 13}, {42, 13}, {43, 13}, {44, 13}, {45, 13}, {46, 13}, {47,
	13}, {48, 13}, {49, 13}, {40, 14}, {42, 14}, {43, 14}, {44, 14},
	{45, 14}, {46, 14},
	{0, 0}
};

/*
 *	Repeat blocks are also stored as Atp_DataDescriptor structures,
 *	same as databytes.
 */
Atp_DataDescriptor DemoDesc [] = {
	{NELEMS(DemoData1) - 1, DemoData1},
	{NELEMS(DemoData2) - 1, DemoData2}
};

/*
 *	Our compiler does not support aggregate initialization of structures,
 *	otherwise, I would have typed Atp_DataDescriptor DataDescBuf = DemoDesc[0];
 */
Atp_DataDescriptor DataDescBuf = {NELEMS(DemoData2) - 1, DemoData2};

/*
 * Vproc
 */
#if defined (__STDC__) || defined (__cplusplus)
char * PlotVproc( void *valPtr, Atp_BoolType UserSuppliedRptBlk )
#else
char * PlotVproc(valPtr, UserSuppliedRptBlk)
	void			*valPtr;
	Atp_BoolType	UserSuppliedRptBlk;
#endif
{
	Atp_DataDescriptor *RptBlkDescPtr = (Atp_DataDescriptor *)valPtr;
	if ((RptBlkDescPtr->count == 0) && (RptBlkDescPtr->data == NULL) &&
		 UserSuppliedRptBlk)
	  return("PlotVproc: \"Don't you wanna plot anything?\"");

	return NULL;
}

/* Parmdef */
ATP_DCL_PARMDEF(GraphCoordinates_PD)
	BEGIN_PARMS
		/*
			Optional repeat block default :-

			Specify the default as a pointer VALUE to a static
			Atp_DataDescriptor structure. You CANNOT use a pointer
			instead because it has no value at compile time !!
		*/
		BEGIN_OPT_REPEAT("coords","Group of 3D coordinates",&DataDescBuf,
						 0,MAX_COORDS,PlotVproc)
			num_def("X","X-coordinate",0,X_WIDTH,NULL)
			num_def("Y","Y-coordinate",0,Y_DEPTH,NULL)
		END_OPT_REPEAT
	END_PARMS
ATP_END_DCL_PARMDEF

/* Command callback */
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result PlotGraph(ClientData clientData, Tcl_Interp *interp,
					 int argc, char *argv[])
#else
Atp_Result PlotGraph(clientData, interp, argc, argv)
	ClientData clientData;
	Tcl_Interp *interp;
	int argc;
	char *argv[];
#endif
{
	register int	n, x_coord, y_coord;

	int				coordCount, Max_X, Max_Y, GraphSize;
	char			*Graph, *FastGraph, *cPtr;
	XY_CoordsStruct	*CoordsPtr = NULL;

	static int	toggle = 1;

	/* Get pointer to beginning of repeat block of coordinates. */
	CoordsPtr = (struct xy_coord *) Atp_RptBlockPtr("coords", &coordCount);

	/* If zero instance repeat block, then there's nothing to do. */
	if (CoordsPtr == NULL && coordCount == 0) {
	  Tcl_SetResult(interp, NULL, TCL_STATIC);
	  return ATP_OK;
	}

	/*
	 *	This is where you toggle between the two demo pictures
	 *	if the default is used.
	 */
	if ((CoordsPtr == DemoDesc[0].data) || (CoordsPtr == DemoDesc[1].data))
	{
	  toggle = (toggle == 0) ? 1 : 0;
	  DataDescBuf = DemoDesc[toggle];
	}

	/* Write in the terminator coordinates (first pair being the zeroth). */
	CoordsPtr[coordCount].x = ARRAY_TERMINATOR;
	CoordsPtr[coordCount].y = ARRAY_TERMINATOR;

	/* What are the boundaries of the graph ? */
	Max_X = Max_Y = 0;
	for (n = 0; n < coordCount; n++) {
	   if (CoordsPtr[n].x > Max_X) Max_X = CoordsPtr[n].x;
	   if (CoordsPtr[n].y > Max_Y) Max_Y = CoordsPtr[n].y;
	}

	Graph = (char *)MALLOC((size_t)(GraphSize = (Max_X + 1) * (Max_Y +1)), NULL);

	/* Blank out the graph. */
	for (n = 0; n < GraphSize; n++) Graph[n] = ' ';

	/* Now plot in points in graph. */
	for (n = 0; CoordsPtr[n].x != ARRAY_TERMINATOR; n++) {
	   Graph[DynamicArray(CoordsPtr[n].x,CoordsPtr[n].y) ] = '*';
	}

	/* Make a faster graph for firing at the screen. */
	cPtr =
	FastGraph = (char *)MALLOC((size_t)(GraphSize=((Max_X+2)*(Max_Y+1)+1)), NULL);

	/* Print graph onto fast graph. */
	for (y_coord = 0; y_coord <= Max_Y; y_coord++) {
	   for (x_coord = 0; x_coord <= Max_X; x_coord++) {
		  cPtr [x_coord] = Graph[DynamicArray (x_coord, y_coord)];
	   }
	   cPtr[Max_X +1] = '\n';
	   cPtr += Max_X + 2;
	}
	FastGraph[GraphSize - 1] = '\0';

	FREE((void *)Graph);

	/* Return fast-graph as result string. */
	Tcl_SetResult(interp, FastGraph, free);

	return ATP_OK;
}

/*+***************************************************************************

	Command:		check

	Copyright:		BNR Europe Limited, 1993
					Be11-Northern Research
					Northern Telecom / Nortel

	Description:	This function checks	that	the  command
					callback interface is	being	maintained when
					relayed via Tcl, atp2tcl and ATP (function
					Atp_ExecuteCallback).

****************************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result CheckTclInterfaceCmd(ClientData clientData, Tcl_Interp *interp,
								int argc, char *argv [])
#else
Atp_Result CheckTclInterfaceCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	char	*return_string = NULL;
	int		i;

	(void) Atp_AdvPrintf("clientData = \"%s\"\n", (char *)clientData);
	(void) Atp_AdvPrintf("argc = %d", argc);
	for (i = 0; i < argc; i++)
	   (void) Atp_AdvPrintf("\nargv[%d] = \"%s\"", i, argv[i]);

	return_string = Atp_AdvGets();

	Tcl_SetResult(interp, return_string, free);

	return ATP_OK;
}

/*+*********************************************************************

	Command:		recur <string> <command>

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command to test nested commands. This causes
					recursive calls to ATP's internal functions.

***********************************************************************-*/
ATP_DCL_PARMDEF(RecurParms)
	BEGIN_PARMS
		str_def("marker", "marker to indicate nesting level", 0, INT_MAX, NULL)
		str_def("command", "nested command to execute", 0, INT_MAX, NULL)
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result RecurCmd(ClientData clientData, Tcl_Interp *interp,
					int argc, char *argv[])
#else
Atp_Result RecurCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	static int	level = -1;
	char *		return_string = NULL;
	int			result = 0;

	level++;

	(void) Atp_AdvPrintf("[%d]%s->\n", level, Atp_Str("marker"));

	result = Tcl_Eval(interp, Atp_Str("command"));

	if (Tcl_GetStringResult(interp) != NULL) {
	  (void) Atp_AdvPrintf("%s\n", Tcl_GetStringResult(interp));

	  Tcl_SetResult(interp, "", TCL_STATIC);
	}

	(void) Atp_AdvPrintf("<-[%d]%s", level, Atp_Str("marker"));

	if (level == 0) {
	  return_string = Atp_AdvGets();
	  Tcl_SetResult(interp, return_string, free);
	}

	level--;

	return (result == TCL_OK) ? ATP_OK : ATP_ERROR;
}

/*+**********************************************************************

	Command:		more <command>

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Executes command and pipes	output of command
					through the more output pager.

***************+*******************************************************-*/
ATP_DCL_PARMDEF(more_parms)
	BEGIN_PARMS
		str_def("command", "command whose output to pipe through more",
				0, INT_MAX, NULL)
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result MoreCmd(ClientData clientData, Tcl_Interp *interp,
				   int argc, char *argv[])
#else
Atp_Result MoreCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	int result = TCL_OK;

	result = Tcl_Eval(interp, Atp_Str("command"));

	OutputResult(interp, result);

	Tcl_SetResult(interp, NULL, TCL_STATIC);

	return result;
}

/*
* Output paging routine for "more" command.
*/
#if defined(__STDC__) || defined(__cplusplus)
void OutputResult( Tcl_Interp *interp, int result )
#else
void OutputResult(interp, result)
	Tcl_Interp	*interp;
	int			result;
#endif
{
	if (result == TCL_OK)
	{
	  if (*Tcl_GetStringResult(interp) != 0)
	  {
		FILE *cmdout_fp = stdout;

#if sun || sun2 || sun3 || sun4
		cmdout_fp = (cmdout_fp = popen("/usr/ucb/more -", "w")) ?
					 cmdout_fp : stdout;
#else
		cmdout_fp = (cmdout_fp = popen("/usr/bin/more -", "w")) ?
					 cmdout_fp : stdout;
#endif

		(void) fprintf(cmdout_fp, "%s\n", Tcl_GetStringResult(interp));

		(void) fflush(cmdout_fp); /* flush output */
		(void) pclose(cmdout_fp); /* close pipe */
	  }
	}
	else
	{
	  if (result == TCL_ERROR)
		(void) fprintf(stderr, "Error");
	  else
		(void) fprintf(stderr, "Error %d", result);

	  if (*Tcl_GetStringResult(interp) != 0)
		(void) fprintf(stderr, ": %s\n", Tcl_GetStringResult(interp));
	  else
		(void) fprintf(stderr, "\n");

	  (void) fflush(stderr);
	}
}

/*+**********************************************************************

	Command:		llc

	Copyright:		BNR	Europe	Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command from the GENIE PRI Prototype application
					Low	Level Compatibility parameter definition
					command. This is used to test the ATP "man"
					command and the parsing of deeply nested
					constructs and parameters using CHOICE and
					CASE...etc.

***************+*******************************************************-*/
#define OCTET_PRESENT	"Y"
#define OCTET_ABSENT	"N"
#define GPP_EXT_CASE	BEGIN_CASE(OCTET_PRESENT, "Extension domain", 0)
#define GPP_NO_EXT		CASE(OCTET_ABSENT, "No extension", 0)

ATP_DCL_PARMDEF(IE_llc_Parms)
	BEGIN_PARMS
		num_def("itc", "Information Transfer Capability", 0, 31, NULL)
		num_def("cs", "Coding Standard", 0, 3, NULL)
		BEGIN_CHOICE("ext_3", "Extend Octet 3", NULL)
			GPP_EXT_CASE
				num_def("ni", "Negotiation Indicator", 0, 1, NULL)
			END_CASE
			GPP_NO_EXT
		END_CHOICE

		num_def("itr", "Information Transfer Rate", 0, 31, NULL)
		num_def("tm", "Transfer Mode", 0, 3, NULL)
		BEGIN_CHOICE("ext_4" , "Extend Octet 4", NULL)
			GPP_EXT_CASE
				num_def("estab", "Establishment", 0, 3, NULL)
				num_def("config", "Configuration", 0, 3, NULL)
				num_def("struc", "Structure", 0, 7, NULL)
				BEGIN_CHOICE("ext_4a", "Extend bit", NULL)
					GPP_EXT_CASE
						num_def("itrdo", "Info Transfer Rate (Dest - Orig)", 0, 31, NULL)
						num_def("sym", "Symmetry", 0, 3, NULL)
					END_CASE
					GPP_NO_EXT
				END_CHOICE
			END_CASE
			GPP_NO_EXT
		END_CHOICE

		BEGIN_CHOICE("oct5", "Octet 5", NULL)
			BEGIN_CASE(OCTET_PRESENT, "Octet 5 Present", 0)
				num_def("l1p", "Layer 1 Protocol", 0, 31, NULL)
				BEGIN_CHOICE("ext_5", "Extend Octet 5", NULL)
					GPP_EXT_CASE
						num_def("rate", "User Rate", 0, 31, NULL)
						num_def("neg", "Negotiation", 0, 1, NULL)
						num_def("sync", "Sync/Async", 0, 1, NULL)
						BEGIN_CHOICE("ext_5a", "Extend Octet 5a", NULL)
							GPP_EXT_CASE
								num_def("fcr", "Flow Control on Rx", 0, 1, NULL)
								num_def("fct", "Flow Control on Tx", 0, 1, NULL)
								num_def("nicr", "NIC on Rx", 0, 1, NULL)
								num_def("nict", "NIC on Tx", 0, 1, NULL)
								num_def("ir", "Intermediate Rate", 0, 3, NULL)
								BEGIN_CHOICE("ext_5b", "Extend Octet 5b", NULL)
									GPP_EXT_CASE
										num_def("p", "Parity", 0, 7, NULL)
										num_def("ndb", "Number of Data bits", 0, 3, NULL)
										num_def("nsb", "Number of Stop bits", 0, 3, NULL)
										BEGIN_CHOICE("ext_5c", "Extend Octet 5c", NULL)
											GPP_EXT_CASE
												num_def("tom", "Type of Modem", 0, 63, NULL)
												num_def("dt", "Duplex Mode", 0, 1, NULL)
											END_CASE
											GPP_NO_EXT
										END_CHOICE
									END_CASE
									GPP_NO_EXT
								END_CHOICE
							END_CASE
							GPP_NO_EXT
						END_CHOICE
					END_CASE
					GPP_NO_EXT
				END_CHOICE
				BEGIN_CHOICE("oct6", "Octet 6", NULL)
					BEGIN_CASE(OCTET_PRESENT, "Octet 6 Present", 0)
						num_def("l2p", "Layer 2 Protocol", 0, 31, NULL)
						BEGIN_CHOICE("ext_6", "Extend Octet 6", NULL)
							GPP_EXT_CASE
								num_def("ol2pi", "Optl Layer 2 Protocol Info", 0, 127, NULL)
							END_CASE
							GPP_NO_EXT
						END_CHOICE
						BEGIN_CHOICE("oct7" , "Octet 7", NULL)
							BEGIN_CASE(OCTET_PRESENT, "Octet 7 Present", 0)
								num_def("l3p", "Layer 3 Protocol", 0, 31, NULL)
								BEGIN_CHOICE("ext_7", "Extend Octet 7", NULL)
									GPP_EXT_CASE
										num_def("ol3pi","Optl Layer 3 Protocol Info", 0, 127, NULL)
									END_CASE
									GPP_NO_EXT
								END_CHOICE
							END_CASE
							CASE(OCTET_ABSENT, "No Octet 7", 0)
						END_CHOICE
					END_CASE
					CASE(OCTET_ABSENT, "No Octet 6", 0)
				END_CHOICE
			END_CASE
			CASE(OCTET_ABSENT, "No Octet 5", 0)
		END_CHOICE
	END_PARMS
ATP_END_DCL_PARMDEF

/* Callback */
#if defined (__STDC__) || defined(__cplusplus)
Atp_Result LLC_Cmd(ClientData clientData, Tcl_Interp *interp,
				   int argc, char *argv[])
#else
Atp_Result LLC_Cmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tel_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	Atp_NumType	i = 0;
	char		*return_string = NULL;

	Atp_AdvPrintf("Decode llc:\n");

	/* Octet 3 */
	Atp_AdvPrintf("Octet 3:	");
	Atp_AdvPrintf("itc = %d, ", Atp_Num("itc"));
	Atp_AdvPrintf("cs = %d, ", Atp_Num("cs"));
	Atp_AdvPrintf("ext_3 = %u\n", i = Atp_Index("ext_3"));

	if (i == 0)
	{
		/* Octet 3a */
		Atp_AdvPrintf("Octet 3a: ");
		Atp_AdvPrintf("ni = %d\n", Atp_Num("ni"));
	}

	/* Octet 4 */
	Atp_AdvPrintf("Octet 4:  ");
	Atp_AdvPrintf("itr = %d, ", Atp_Num("itr"));
	Atp_AdvPrintf("tm = %d, ", Atp_Num("tm"));
	Atp_AdvPrintf("ext_4 = %u\n", i = Atp_Index("ext_4"));

	if (i == 0)
	{
		/* Octet 4a */
		Atp_AdvPrintf("Octet 4a: ");
		Atp_AdvPrintf("estab = %d, ", Atp_Num("estab"));
		Atp_AdvPrintf("config = %d, ", Atp_Num("config"));
		Atp_AdvPrintf("struc = %d, ", Atp_Num("struc")) ;
		Atp_AdvPrintf("ext_4a = %u\n", i = Atp_Index("ext_4a"));

		if (i == 0)
		{
			/* Octet 4b */
			Atp_AdvPrintf("Octet 4b: ");
			Atp_AdvPrintf("itrdo = %d, ", Atp_Num("itrdo"));
			Atp_AdvPrintf("sym = %d\n", Atp_Num ("sym"));
		}
	}

	if (Atp_Index("oct5") == 0)
	{
		/* Octet 5 */
		Atp_AdvPrintf("Octet 5:	") ;
		Atp_AdvPrintf("l1p = %d, ", Atp_Num("l1p"));
		Atp_AdvPrintf("ext_5 = %u\n", i = Atp_Index("ext_5"));

		if (i == 0)
		{
			/* Octet 5a */
			Atp_AdvPrintf ("Octet 5a: ");
			Atp_AdvPrintf("rate = %d, ", Atp_Num("rate"));
			Atp_AdvPrintf("neg = %d, ", Atp_Num("neg"));
			Atp_AdvPrintf("sync = %d, ", Atp_Num("sync"));
			Atp_AdvPrintf("ext_5a = %u\n", i = Atp_Index("ext_5a"));

			if (i == 0)
			{
				/* Octet 5b */
				Atp_AdvPrintf("Octet 5b: ");
				Atp_AdvPrintf("fcr = %d, ", Atp_Num("fcr"));
				Atp_AdvPrintf("fct = %d, ", Atp_Num("fct"));
				Atp_AdvPrintf("nicr = %d, ", Atp_Num("nicr"));
				Atp_AdvPrintf("nict = %d, ", Atp_Num("nict"));
				Atp_AdvPrintf("ir = %d, ", Atp_Num("ir"));
				Atp_AdvPrintf ("ext_5b = %u\n", i = Atp_Index("ext_5b"));

				if (i == 0)
				{
					/* Octet 5c */
					Atp_AdvPrintf("Octet 5c: ");
					Atp_AdvPrintf("p = %d, ", Atp_Num("p"));
					Atp_AdvPrintf("ndb = %d, ", Atp_Num("ndb"));
					Atp_AdvPrintf("nsb = %d, ", Atp_Num("nsb"));
					Atp_AdvPrintf("ext_5c = %u\n", i = Atp_Index("ext_5c"));

					if (i == 0)
					{
						/* Octet 5d */
						Atp_AdvPrintf("Octet 5d: ");
						Atp_AdvPrintf("tom = %d, ", Atp_Num("tom"));
						Atp_AdvPrintf("dt = %d\n", Atp_Num("dt"));
					}
				}
			}
		}

		if (Atp_Index("oct6") == 0)
		{
			/* Octet 6 */
			Atp_AdvPrintf("Octet 6:	");
			Atp_AdvPrintf("l2p = %d, ", Atp_Num("l2p"));
			Atp_AdvPrintf("ext_6 = %u\n", i = Atp_Index("ext_6"));

			if (i == 0)
			{
				/* Octet 6a */
				Atp_AdvPrintf("Octet 6a: ");
				Atp_AdvPrintf("ol2pi = %d\n", Atp_Num("ol2pi"));
			}

			if (Atp_Index("oct7") == 0)
			{
				/* Octet 7 */
				Atp_AdvPrintf("Octet 7:	");
				Atp_AdvPrintf("l3p = %d, ", Atp_Num("l3p"));
				Atp_AdvPrintf("ext_7 = %u\n", i = Atp_Index("ext_7"));

				if (i == 0)
				{
					/* Octet 7a */
					Atp_AdvPrintf("Octet 7a: ");
					Atp_AdvPrintf("ol3pi = %d\n", Atp_Num("ol3pi"));
				}
			}
		}
	}

	return_string = Atp_AdvGets();

	Tcl_SetResult(interp, return_string, free);

	return ATP_OK;
}

/*
 * The "llc_test" command tests the "llc" command.
 */
static char llc_cmd[] = "llc bin10101 2 y 1 bin11011 3 y bin01 bin01 bin101 y \
bin01010 bin11 y bin01110 y bin01111 1 0 y 1 0 1 1 0 y 0 3 0 y bin110011 0 y \
bin11100 y bin1111111 y bin00011 y bin0000000";

#if defined (__STDC__) || defined(__cplusplus)
Atp_Result LLC_TEST_Cmd(ClientData clientData, Tcl_Interp *interp,
						int arge, char *argv[])
#else
Atp_Result LLC_TEST_Cmd(clientData, interp, arge, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	int result;

	Atp_AdvPrintf("Executing command; %s\n\n", llc_cmd);

	result = Tcl_Eval(interp, llc_cmd);

	Atp_AdvPrintf("%s\n", Tcl_GetStringResult(interp));

	Atp_AdvPrintf("\"llc\" command test OK.");

	Tcl_SetResult(interp, Atp_AdvGets(), free);

	return result;
}

/*+*****************************************************************

	Command:		estimate

	Copyright:		BNR Europe	Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	A command to estimate the manpower
					required to produce an arbitrary quantity
					of software in several broad categories
					according to Boehm's equations derived
					from historical data.

					Also, shown here is an example of how to
					define manpage headers and footers for a
					command. See Atp_AddHelpInfo() in the
					main() program.

*****************************•**************************************-*/
ATP_DCL_PARMDEF(EstimateParms)
	BEGIN_PARMS
		num_def("LOC", "Lines of Code", 1, INT_MAX, NULL)
	END_PARMS
ATP_END_DCL_PARMDEF

/*
	The header and footer have been changed to contain single lines per
	paragraph so that the help command will wrap them automatically using
	Atp_PrintfWordWrap(). This provides a simple sanity check for the
	wrap function.
*/
char *estimate_desc_header[] = {
	"Boehm's 3 levels of product complexity are organic, \
	semidetached, and embedded programs.\n",

	"These levels roughly correspond to applications programs, \
	utility programs and systems programs.",

	NULL
};

char *estimate_desc_footer[] = {
	"The Boehm set of equations were derived by examining \
	historical data from a large number of actual projects.",
	NULL
};

char *estimate_output_template [] = {
	"Program category	PM		TDEV	Staffing Level",
	"-----------------------  --------------------------",
	"PM   = Programmer-month",
	"TDEV = Development Time (months)\n",

	"TDEV is an average, and you would need the given number of \
	people.\n",

	"The Boehm set of equations were derived by examining \
	historical data from a large number of actual projects.\n",

	"Boehm's 3 levels of product complexity are organic, \
	semidetached, and embedded programs.\n",

	"These levels roughly correspond to applications programs, \
	utility programs and systems programs.",

	NULL
};

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result EstimateCmd(ClientData clientData, Tcl_Interp *interp,
					   int argc, char *argv[])
#else
Atp_Result EstimateCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
#define App		0
#define Util	1
#define Sys		2

	double PM[3], TDEV[3], STAFF [3] ;
	int x;
	char *return_string;
	double KDSI = (double)Atp_Num("LOC") / 1000;

	PM[App]  = 2.4 * pow(KDSI,1.05);
	PM[Util] = 3.0 * pow(KDSI,1.12);
	PM[Sys]  = 3.6 * pow(KDSI,1.20);

	TDEV[App]  = 2.5 * pow(PM[App],0.38);
	TDEV[Util] = 2.5 * pow(PM[Util],0.35);
	TDEV[Sys]  = 2.5 * pow(PM[Sys],0.32);

	STAFF[App]  = PM[App]/TDEV[App];
	STAFF[Util] = PM[Util]/TDEV[Util];
	STAFF[Sys]  = PM[Sys]/TDEV[Sys];

	Atp_AdvPrintf("%s\n", estimate_output_template[0]);
	Atp_AdvPrintf("%s\n", estimate_output_template[1]);

	for (x = 0; x < 3; x++) {
	   Atp_AdvPrintf((x==App)  ? "Applications		" :
					 (x==Util) ? "Utility			" :
					 "Systems				");
	   Atp_AdvPrintf("%6.If	%6.If	%14.If\n",
			   	     PM[x], TDEV[x], STAFF[x]);
	}

	Atp_AdvPrintf("\n");

	for (x = 2; estimate_output_template [x] != NULL; x++)
	   Atp_PrintfWordWrap(Atp_AdvPrintf, 80-4, 1, 1,
					      "%s\n", estimate_output_template[x]);

	return_string = Atp_AdvGets();

	Tcl_SetResult(interp, return_string, free);

	return ATP_OK;

#undef App
#undef Util
#undef Sys
}

/*+*****************************************************************

	Command:		optrpt

	Copyright:		BNR Europe	Limited, 1993
					Bell-Northern Research
					Northern Telecom

	Description:	Command to	test	optional	repeat	blocks.
					If no value is	supplied	for	an	optional
					repeat block, then the number of
					instances obtained from calls to
					Atp_RptBlockPtr() and Atp_RptBlockDesc()
					will be less than 0 or -1.

*******************************************************************-*/
ATP_DCL_PARMDEF(optrpt_parms)
	BEGIN_PARMS
		BEGIN_OPT_REPEAT("repeat strings","strings",NULL,1,255,NULL)
			str_def("string", "string within repeat block", 1, INT_MAX, NULL)
		END_OPT_REPEAT
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result optrpt_cmd(ClientData clientData, Tcl_Interp *interp,
					  int argc, char *argv[])
#else
Atp_Result optrpt_cmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[] ;
#endif
{
	char	**strings, *return_string;
	int		x, count;

	strings = (char **) Atp_RptBlockPtr("repeat strings", &count);

	if (strings == NULL && count < 0)
	  Atp_AdvPrintf("Optional repeat block default value is NOLL.");

	for (x = 0; x < count; x++)
	   Atp_AdvPrintf("strings[%d] = %s%s", x, strings[x],
			   	   	 (x < count-1) ? "\n" : "");

	return_string = Atp_AdvGets();

	Tcl_SetResult(interp, return_string, free);

	return ATP_OK;
}

/*+*****************************************************************

	Command:		query

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command to test the use of CHOICE and CASE
					constructs. In particular, define an optional
					CHOICE default value.

*******************************************************************-*/
#define QUERY_ENUM_CAP		707
#define QUERY_ENUM_USAGE	717
#define QUERY_ENUM_USER		727
#define QUERY_ENUM_SYSTEM	737
#define QUERY_ENUM_LINK		747
#define QUERY_ENUM_SUT		757
#define QUERY_ENUM_MSG		767

/*
 *	Choice default as Atp_ChoiceDescriptor, here 1st field is assigned.
 *	other fields are defaulted to zero by the compiler.
 */
static Atp_ChoiceDescriptor query_default_choice = { QUERY_ENUM_USAGE };

#define MAX_LINKS	100

#if defined(__STDC__) || defined (__cplusplus)
char * NoSpacesVproc( void *valPtr, Atp_BoolType dummy )
#else
char * NoSpacesVproc(valPtr, dummy)
	void			*valPtr;
	Atp_BoolType	dummy;
#endif
{
	char **string = (char **)valPtr;
	char *str = *string;

	if (str != NULL)
	while (*str != '\0') {
		if (isspace(*str))
		  return "No spaces allowed in SUT name.";
		str++;
	}

	return NULL;
}

ATP_DCL_PARMDEF(QueryParms)
	BEGIN_PARMS
		BEGIN_OPT_CHOICE("type", "Report type", &query_default_choice, NULL)
			CASE("CAP", "Capabilities", QUERY_ENUM_CAP)
			CASE("USAGE", "Links of User [DEFAULT]", QUERY_ENUM_USAGE)
			CASE("USER", "Identified Users", QUERY_ENUM_USER)
			CASE("SYSTEM", "System resources", QUERY_ENUM_SYSTEM)

			BEGIN_CASE("LINK", "Link Information", QUERY_ENUM_LINK)
				num_def("link", "link number", 1, MAX_LINKS - 1, NULL)
				str_def("name", "specified link name", 0, 80, NULL)
			END_CASE

			BEGIN_CASE("SUT" , "Systems Under Test", QUERY_ENUM_SUT)
				opt_str_def("sut", "specified sut", "DMSIOOi", 0, 80, NoSpacesVproc)
			END_CASE

			BEGIN_CASE("MSG", "Protocol Messages", QUERY_ENUM_MSG)
				opt_str_def("msg", "message mnemonic", "*", 1, 80, NULL)
			END_CASE

			END_OPT_CHOICE
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined (__STDC__) || defined (__cplusplus)
Atp_Result QueryCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
#else
Atp_Result QueryCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[] ;
#endif
{
	char *return_string;
	int query_type = Atp_Num("type");

	switch(query_type) {
		case QUERY_ENUM_CAP:
			Atp_AdvPrintf("Capabilities");
			break;
		case QUERY_ENUM_USAGE:
			Atp_AdvPrintf("Links of User");
			break;
		case QUERY_ENUM_USER:
			Atp_AdvPrintf("Identified Users");
		break;
		case QUERY_ENUM_SYSTEM:
			Atp_AdvPrintf("System resources");
			break;
		case QUERY_ENUM_LINK:
			Atp_AdvPrintf("Link Information: link %d (%s)",
			Atp_Num("link"), Atp_Str("name"));
			break;
		case QUERY_ENUM_SUT: {
			char *sut = Atp_Str("sut");
			Atp_AdvPrintf("Systems Under Test: \"%s\"",
						  (sut == NULL) ? "" : sut);
			break;
			}
		case QUERY_ENUM_MSG:
			Atp_AdvPrintf("Protocol Messages: \"%s\"", Atp_Str("msg"));
			break;
		default:Atp_AdvPrintf("Unknown query type %d", query_type);
			break;
	}

	return_string = Atp_AdvGets();

	Tcl_SetResult(interp, return_string, free);

	return ATP_OK;
}

/*+*****************************************************************

	Command:		send

	Copyright:		BNR Europe	Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command to test an optional repeat block
					of databytes.

*******************************************************************-*/
/* You may also define a parmdef simply as Atp_ParmDefEntry[]. */
Atp_ParmDefEntry SendParms[] =
{
	BEGIN_PARMS
		num_def("link", "Logical link number", 0, 255, NULL)
		str_def("msg", "Mnemonic of message to send", 1, 255, NULL)
		BEGIN_OPT_REPEAT("ie_list", "IE values to send explicitly",
						 NULL, 0, 255, NULL)
			data_bytes_def("ie_data", "Hex data bytes", 1, 0, NULL)
		END_OPT_REPEAT
	END_PARMS
};

#if defined (__STDC__) || defined (__cplusplus)
Atp_Result SendCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
#else
Atp_Result SendCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	char *return_string;
	unsigned char unitByte;
	Atp_DataDescriptor rptblk_desc, *ie_list;
	int x, y;

	Atp_AdvPrintf("Logical link number         = %d\n", Atp_Num("link"));
	Atp_AdvPrintf("Mnemonic of message to send = %s\n", Atp_Str("msg"));

	rptblk_desc = Atp_RptBlockDesc("ie_list");

	ie_list = (Atp_DataDescriptor *)rptblk_desc.data;

	if (rptblk_desc.count == -1)
	  Atp_AdvPrintf("Optional default repeat block for ie_list is NULL.");
	else
	if (rptblk_desc.count == 0)
	  Atp_AdvPrintf("No IB data to send.");
	else
	  Atp_AdvPrintf("IE data to send:\n");

	for (x=0; x < rptblk_desc.count; x++) {
	   Atp_AdvPrintf("IE[%d] = ", x);
	   for (y=0; y < ie_list[x].count; y++) {
	      unitByte = ((unsigned char *)ie_list[x].data)[y];
	      Atp_AdvPrintf("%x ", unitByte);
	   }
	   if (x < rptblk_desc.count-1) Atp_AdvPrintf("\n");
	}

	return_string = Atp_AdvGets();

	Tcl_SetResult(interp, return_string, free);

	return ATP_OK;
}

/*+*****************************************************************

	Command:		null

	Copyright:		BNR Europe Limited,	1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command for testing	the NULL parameter.

*******************************************************************-*/
ATP_DCL_PARMDEF(NullParms)
	BEGIN_PARMS
		null_def("null", "NULL parameter")
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result NullCmd(ClientData clientData, Tcl_Interp *interp,
		int argc, char *argv[])
#else
Atp_Result NullCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	Atp_NumType	*parmptr = NULL;
	Atp_NumType	parmvalue = -1;

	parmptr = (Atp_NumType *) Atp_ParmPtr("null");
	if (parmptr != NULL) {
	  parmvalue = *parmptr;
	  (void) sprintf(cmd_result, "null = %d (%s)", parmvalue,
			  	  	 (parmvalue == 0) ? "PASS" : "FAIL");
	  Tcl_SetResult(interp, cmd_result, TCL_STATIC);
	  return ATP_OK;
	}
	else {
	  Tcl_SetResult(interp, "NULL parameter test failed.", TCL_STATIC);
	  return ATP_ERROR;
	}
}

/*+******************************************************************

	Command:		choice

	Copyright:		BNR Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Command to test nested	parameters and
					constructs within the CHOICE construct.

*******************************************************************-*/
#define BELL		931224
#define NORTHERN	931225
#define RESEARCH	931226

Atp_KeywordType TestChoiceKeyTab[] = {
	{"Bell",		BELL},
	{"Northern",	NORTHERN},
	{"Research",	RESEARCH},
	{NULL}
};

ATP_DCL_PARMDEF(TestChoiceParms)
	BEGIN_PARMS
		BEGIN_CHOICE("choice", "choice block", NULL)
			num_def("num", "number", 1, 0, NULL)
			unsigned_num_def("unsigned number", "unsigned number" ,1, 0, NULL)
			data_bytes_def("databytes", "hexadecimal data bytes", 1, 0, NULL)
			str_def("str", "string", 1, 0, NULL)
			keyword_def("key","keyword",TestChoiceKeyTab,NULL)
			bool_def("bool", "boolean", NULL)
			BEGIN_LIST("list", "list of parameters")
				num_def("num2", "number 2", 1, 0, NULL)
			END_LIST
			BEGIN_REPEAT("rptblk", "repeat block", 1, 0, NULL)
				str_def("str2", "string 2", 1, 0, NULL)
			END_REPEAT
			BEGIN_CHOICE("choice2", "choice block 2", NULL)
				CASE("case0", "Zeroth case within a choice", 0)
				BEGIN_CASE("case1", "First case within a choice", 1)
					num_def("num3", "number 3", 1, 0, NULL)
				END_CASE
			END_CHOICE
		END_CHOICE
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined (__STDC__) || defined (__cplusplus)
Atp_Result TestChoiceCmd(ClientData clientData, Tcl_Interp *interp,
						 int argc, char *argv[])
#else
Atp_Result TestChoiceCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	Atp_UnsNumType Index = Atp_Index("choice");
	char *return_string = NULL;

	switch (Index) {
		case 0:
			(void) Atp_AdvPrintf ("num = %d", Atp_Num("num")) ;
			break;
		case 1:
			(void) Atp_AdvPrintf("unsigned number = %u",
								 Atp_UnsignedNum("unsigned number"));
			break;
		case 2: {
			Atp_DataDescriptor desc;
			char *str;
			desc = Atp_DataBytesDesc("databytes");
			str = PrintAsciiDatabytes((Atp_ByteType *)desc.data,desc.count);
			(void) Atp_AdvPrintf("databytes = %s", str);
			if (str != NULL) FREE(str);
			break;
		}
		case 3:
			(void) Atp_AdvPrintf("string = %s", Atp_Str("str"));
			break;
		case 4:
			(void) Atp_AdvPrintf("keyword = %s, ", Atp_Str("key"));
			(void) Atp_AdvPrintf("keyvalue = %d, ", Atp_Num("key"));
			(void) Atp_AdvPrintf("keyindex = %u", Atp_Index("key"));
			break;
		case 5:
			(void) Atp_AdvPrintf("boolean = %d", (int) Atp_Bool("bool"));
			break;
		case 6:
			(void) Atp_AdvPrintf("number 2 = %d", Atp_Num("num2"));
			break;
		case 7: {
			int x, count = 0;
			char **strings = NULL;
			strings = (char **) Atp_RptBlockPtr("rptblk", &count);
			(void) Atp_AdvPrintf("Repeat block of strings =");
			for (x = 0; x < count; x++)
			   (void) Atp_AdvPrintf (" %s", strings[x]);
			break;
		}
		case 8:
			switch(Atp_Num("choice2")) {
			case 0:
				(void) Atp_AdvPrintf("case 0 within choice2");
				/* Atp_Str() can be used on CHOICE construct. */
				(void) Atp_AdvPrintf(" - you typed \"%s\".",
									 Atp_Str("choice2"));
				break;
			case 1:
				(void) Atp_AdvPrintf ("case 1 within choice2, ");
				(void) Atp_AdvPrintf("num3 = %d", Atp_Num("num3"));
				break;
		default:
		break;
		}
		break;
			default:
		break;
	}

	return_string = Atp_AdvGets () ;

	Tcl_SetResult(interp, return_string, free);

	return ATP_OK;
}

/*+*******************************************************************

	Command:		logtest

	Copyright:		BNR	Europe Limited, 1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	The	"logtest" feature captures	commands
					and	their results to test	files	which	can
					be "source"d for replay.
					Test files logged can be played back
					using the  "test" procedure in the
					"defs" and "all" files in the  "tests"
					directory.

*******************************************************************-*/
#define LOGTEST_SETMODE			0
#define LOGTEST_SETFILENAME		1
#define LOGTEST_SETPREFIX		2
#define LOGTEST_SETTESTDESC		3
#define LOGTEST_SETTESTCOUNT	4

#define LOGTEST_FILENAME_MAXLEN	30
#define LOGTEST_PREFIX_MAXLEN	20

#define LOGTEST_TESTCASE_PREFIX \
		((*Atp_Logtest_Prefix == '\0') ? "test" : Atp_Logtest_Prefix)

FILE			*Atp_Logtest_File = NULL;
Atp_BoolType	Atp_Logtest_Mode = 0;
char			Atp_Logtest_Filename[LOGTEST_FILENAME_MAXLEN+10] = "";
char			Atp_Logtest_Prefix[LOGTEST_PREFIX_MAXLEN] = "";
char			*Atp_Logtest_Description = NULL;
int				Atp_Logtest_Count = 0;
Atp_BoolType	Atp_Logtest_CmdCalled = 0;

char *Atp_Logtest_Manpage_Header[] = {
	"The \"logtest\" feature captures commands and their results to test",
	"files which can be \"source\"d for replay.\n",

	"Commands and expected results are packaged in a test script format",
	"acceptable to the Tcl \"test\" procedure. Use the various \"logtest\"",
	"command options to adjust the settings desired.\n",

	"Test files logged can be played back using the Tcl \"test\" procedure",
	"defined in the Tcl \"defs\" file. The test files are suffixed with",
	"\".test\". They can either be executed by sourcing the \"all\" file",
	"(i.e. command \"source all\"), or run individually (e.g. \"source",
	"num-1.1.test\"). If all goes well, no output will appear. If errors",
	"occur, then error messages will be displayed. The VERBOSE variable",
	"may be set to 1, in which case, all testcases run will be displayed.\n",

	"Optionally, the TESTS variable can be set with a match pattern to",
	"select test files. Any test whose identifier matches TESTS will be",
	"run. TESTS defaults to *.\n",

	NULL
};

ATP_DCL_PARMDEF(LogtestParms)
	BEGIN_PARMS
		BEGIN_OPT_REPEAT("option_list","list of options",NULL,1,0,NULL)
			BEGIN_CHOICE("option", "logtest command option", NULL)
				BEGIN_CASE("-o", "set logtest mode to on or off", LOGTEST_SETMODE)
					bool_def("mode", "logtest mode", NULL)
				END_CASE
			BEGIN_CASE("-f", "set log filename of test suite",
						LOGTEST_SETFILENAME)
				str_def("filename", "logtest filename",
						1, LOGTEST_FILENAME_MAXLEN-10, NULL)
			END_CASE
			BEGIN_CASE("-p", "set testcase prefix", LOGTEST_SETPREFIX)
				str_def("prefix", "testcase prefix",
						1, LOGTEST_PREFIX_MAXLEN-1, NULL)
			END_CASE
			BEGIN_CASE("-d", "set testcase description", LOGTEST_SETTESTDESC)
				str_def("description", "testcase description", 1, 132, NULL)
			END_CASE
			BEGIN_CASE("-n", "set testcase counter", LOGTEST_SETTESTCOUNT)
				num_def("count", "testcase count", 0, 100, NULL)
			END_CASE
			END_CHOICE
		END_OPT_REPEAT
	END_PARMS
ATP_END_DCL_PARMDEF

/* logtest command callback */
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result LogtestCmd(ClientData clientData, Tcl_Interp *interp,
					  int argc, char *argv[])
#else
Atp_Result LogtestCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	Atp_DataDescriptor		options_desc;
	Atp_ChoiceDescriptor	*choice_ptr;
	char					*result;
	int						i;

	/* Initialize */
	options_desc	= Atp_RptBlockDesc("option_list");
	choice_ptr		= (Atp_ChoiceDescriptor *)options_desc.data;

	for (i = 0; i < options_desc.count; i++) {
	   switch(choice_ptr[i].CaseValue) {
			case LOGTEST_SETMODE:
				Atp_Logtest_Mode = Atp_Bool("mode");
				break;
			case LOGTEST_SETFILENAME: {
				char *new_filename = Atp_Str("filename");
				if (strcmp(Atp_Logtest_Filename,new_filename) != 0) {
				  (void) strncpy(Atp_Logtest_Filename, new_filename,
						  	  	 LOGTEST_FILENAME_MAXLEN);
				  Atp_Logtest_Filename[LOGTEST_FILENAME_MAXLEN-1] = '\0';

				  /* Change of filename, close old file. */
				  if (Atp_Logtest_Filename[0] != '\0') {
				    (void) fclose (Atp_Logtest_File) ;
				    Atp_Logtest_File = NULL;
				    Atp_Logtest_Count = 0;
				  }
				}
				break;
			}
			case LOGTEST_SETPREFIX:
				(void) strcpy(Atp_Logtest_Prefix, Atp_Str("prefix"));
				Atp_Logtest_Count = 0; /* prefix change => reset */
				break;
			case LOGTEST_SETTESTDESC:
				if (Atp_Logtest_Description != NULL)
				  FREE((void *)Atp_Logtest_Description);
				Atp_Logtest_Description =
				  Atp_Strdup(Atp_Str("description")) ;
				break;
			case LOGTEST_SETTESTCOUNT:
				Atp_Logtest_Count = (int) Atp_Num("count");
				break;
			default:break;
		}
	}

	Atp_Logtest_CmdCalled = 1;

	Atp_AdvPrintf("Log test:\n");
	Atp_AdvPrintf(" Mode ..................... %s\n",
				  (Atp_Logtest_Mode) ? "ON" : "OFF");
	Atp_AdvPrintf(" Test Suite Filename .... \"%s\"\n",
				  Atp_Logtest_Filename);
	Atp_AdvPrintf(" Testcase Prefix .......... \"%s\"\n",
				  Atp_Logtest_Prefix);
	Atp_AdvPrintf(" Testcase Description ... \"%s\"\n",
				  Atp_Logtest_Description);
	Atp_AdvPrintf(" Testcase Counter ......... %d\n",
				  Atp_Logtest_Count);
	Atp_AdvPrintf(" Next Testcase ............ %s.%d",
				  LOGTEST_TESTCASE_PREFIX, Atp_Logtest_Count+1);

	result = Atp_AdvGets();

	Tcl_SetResult(interp, result, free);

	return ATP_OK;
}

/*
 * Procedure for logging a command and its result in a test script format.
 */
#if defined(__STDC__) || defined(__cplusplus)
void Atp_LogTestScript( Tcl_Interp *interp, char *cmd, char *result )
#else
void Atp_LogTestScript(interp, cmd, result)
	Tcl_Interp *interp;
	char *cmd, *result;
#endif
{
	char logtest_filename[LOGTEST_FILENAME_MAXLEN+10];

	if (Atp_Logtest_Mode == 1) {
	  if (Atp_Logtest_File == NULL) {
		(void) strcpy(logtest_filename, Atp_Logtest_Filename);
		(void) strcat(logtest_filename, ".test");
		if ((Atp_Logtest_File = fopen(logtest_filename, "a+")) == NULL)
		{
		  (void) fprintf(stderr, "[ERRNO %d]:\nCannot create file %s\n",
				  	  	 errno, logtest_filename);
		  exit(errno) ;
		}
	  }

	  /*
	   * Log last command and expected result as a testcase.
	   *
	   * In order to allow errors to occur and not abort a testsuite,
	   * the "catch" command is used, "set" of variable "result"
	   * then returns the result for "test" to compare against
	   * the expected output.
	   *
	   * Do NOT use "return" to return result. This literally returns
	   * immediately from the procedure.
	   */
	  (void) fprintf(Atp_Logtest_File, "test %s.%d {%s} {\n",
			  	  	 LOGTEST_TESTCASE_PREFIX, ++Atp_Logtest_Count,
					 Atp_Logtest_Description);
	  (void) fprintf (Atp_Logtest_File, " catch {\n") ;
	  (void) fprintf(Atp_Logtest_File, "	%s\n", cmd);
	  (void) fprintf(Atp_Logtest_File, "  } result\n");
	  (void) fprintf(Atp_Logtest_File, " set result\n");
	  (void) fprintf(Atp_Logtest_File, "} {%s}\n\n", result);
	}
}

/*
 * Log interactive command and result output to test file.
 */
#if defined (__STDC__) || defined (__cplusplus)
void Atp_LogTestFile( Tcl_Interp *interp )
#else
void Atp_LogTestFile(interp)
	Tcl_Interp *interp;
#endif
{
	char *last_command = NULL;
	char *last_command_result = NULL;

	if (Atp_Logtest_Mode == 1) {
	  if (Atp_Logtest_CmdCalled == 1)
		Atp_Logtest_CmdCalled = 0;
	  else {
		/* Save the last command's result. */
		last_command_result = Atp_Strdup(Tcl_GetStringResult(interp));
		/*
		 *	We have no direct access to the command buffer,
		 *	so use "history" to get the last command entered.
		 */
		Tcl_Eval(interp, "history event [expr {[history nextid]-1}]");
		last_command = Tcl_GetStringResult(interp);

		Atp_LogTestScript(interp, last_command, last_command_result);

		if (last_command_result != NULL)
		  FREE((void *)last_command_result);
	  }
	}
	return;
}

/*+*******************************************************************

	Command:		change

	Copyright:		BNR Europe Limited,	1993
					Be11-Northern Research
					Northern Telecom / Nortel

	Description:	A command to set or	change the parameter
					attributes (minimum/maximum range values,
					default value) for a command.
					NOTE: USED ONLY FOR TESTING

********************************************************************-*/
#define CHANGE_RANGE			0
#define CHANGE_DEFAULT_NUMBER	1
#define CHANGE_DEFAULT_STRING	2

/* Cloned from atpsysex.h */
#define Atp_PARMCODE(parmcode)	((parmcode) & ~ATP_OPTPARM_MASK)

static Atp_CmdRec *CmdRecPtr = NULL;
static Atp_ParmDefEntry *parmdef_entry_ptr = NULL;

#if defined(__STDC__) || defined(__cplusplus)
char * FindCmdVproc( void *valPtr, Atp_BoolType dummy )
#else
char * FindCmdVproc(valPtr, dummy)
	void			*valPtr;
	Atp_BoolType	dummy;
#endif
{
	char **CmdNamePtr = (char **)valPtr;
	static char *return_string = NULL; /* initial value is null */

	/* Free string from last use. */
	if (return_string != NULL) {
	  FREE(return_string);
	  return_string = NULL;
	}

	CmdRecPtr = Atp_FindCommand(*CmdNamePtr,1);

	if (CmdRecPtr == NULL)
	  (void) Atp_DvsPrintf(&return_string, "Cannot find command \"%s\".", *CmdNamePtr);

	return return_string;
}

#if defined(__STDC__) || defined(__cplusplus)
char * FindParmVproc( void *valPtr, Atp_BoolType dummy )
#else
char * FindParmVproc(valPtr, dummy)
	void			*valPtr;
	Atp_BoolType	dummy;
#endif
{
	char **ParmNamePtr = (char **)valPtr;
	int x;
	static char *return_string = NULL;

	/* Free string from last use. */
	if (return_string != NULL) {
	  FREE(return_string);
	  return_string = NULL;
	}

	/* Assuming CmdRecPtr is not NULL, proceed with parameter search. */
	for (x = 0; ((CmdRecPtr->parmDef)[x].parmcode != ATP_EPM); x++) {
		if ((CmdRecPtr->parmDef)[x].Name != NULL)
		  /*
		   * Note:	strcmp(s1,s2) where s1 = nil
		   *		results in segmentation violation on
		   *		Sun w/s (SunOS 4.1.2) and hence core dumps.
		   */
		  if (strcmp((CmdRecPtr->parmDef)[x].Name , *ParmNamePtr) == 0) {
			parmdef_entry_ptr = &(CmdRecPtr->parmDef)[x];
			return NULL;
		  }
	}

	/* Search again, this time ignoring case. */
	for (x = 0; ((CmdRecPtr->parmDef)[x].parmcode != ATP_EPM); x++) {
	   if ((CmdRecPtr->parmDef)[x].Name != NULL)
		 if (Atp_Strcmp((CmdRecPtr->parmDef)[x].Name, *ParmNamePtr) == 0) {
		   parmdef_entry_ptr = &(CmdRecPtr->parmDef)[x];
		   return NULL;
		 }
	}

	(void) Atp_DvsPrintf(&return_string,
						 "Cannot find parameter \"%s\".",
						 *ParmNamePtr);

	return return_string;
}

/*
 *	It is not possible to call Atp_Num{"attribute") from a vproc
 *	nested within the CHOICE construct because its parsing process
 *	has not completed and the CHOICE result has not been stored.
 *	Therefore, we use a workaround solution.
 */
#if defined(__STDC__) || defined(__cplusplus)
char * CheckType( int attribute )
#else
char * CheckType(attribute)
	int attribute;
#endif
{
	Atp_ParmCode parmcode = Atp_PARMCODE(parmdef_entry_ptr->parmcode);

	switch (parmcode) {
		case ATP_NUM:
		case ATP_UNS_NUM:
		case ATP_REAL:
			if (attribute != CHANGE_RANGE &&
				attribute != CHANGE_DEFAULT_NUMBER)
			  return "Can only change default number and range values.";
			break;

		case ATP_STR:
			if (attribute != CHANGE_DEFAULT_STRING &&
				attribute != CHANGE_RANGE)
			  return "Can only change default string and range values.";
			break;

		case ATP_KEYS:
		case ATP_BOOL:
			if (attribute != CHANGE_DEFAULT_NUMBER)
			  return "Can only change default number value.";
			break;

		case ATP_DATA:
		case ATP_BRP:
			if (attribute != CHANGE_RANGE)
			  return "Can only change range values.";
			break;

		case ATP_BPM: case ATP_EPM:
		case ATP_BLS: case ATP_ELS:
		case ATP_BCH: case ATP_ECH:
		case ATP_ERP:
		case ATP_BCS: case ATP_ECS:
		case ATP_NULL:
		case ATP_COM:
			return "Cannot change attributes for this parameter type.";
		default:
			break;
	}

	return NULL;
}

/* Vprocs */
char * CheckRangeMin()	{return CheckType(CHANGE_RANGE);}
char * CheckRangeMax()	{return CheckType(CHANGE_RANGE);}
char * CheckDefNum()	{return CheckType(CHANGE_DEFAULT_NUMBER);}
char * CheckDefStr()	{return CheckType(CHANGE_DEFAULT_STRING);}

/* Parmdef */
ATP_DCL_PARMDEF(ChangeParms)
	BEGIN_PARMS
		str_def("cmd", "command name", 1,INT_MAX, FindCmdVproc)
		str_def("parm","parameter name",1,INT_MAX, FindParmVproc)
		BEGIN_CHOICE("attribute", "parameter attribute", NULL)
			BEGIN_CASE("range", "change range values", CHANGE_RANGE)
				num_def("min", "minimum value", INT_MIN, INT_MAX, CheckRangeMin)
				num_def("max", "maximum value", INT_MIN, INT_MAX, CheckRangeMax)
			END_CASE
			BEGIN_CASE("defnum", "change default number", CHANGE_DEFAULT_NUMBER)
				num_def("defnum","default number",INT_MIN,INT_MAX,CheckDefNum)
			END_CASE
			BEGIN_CASE("defstr", "change default string", CHANGE_DEFAULT_STRING)
				str_def("defstr", "default string",0,INT_MAX,CheckDefStr)
			END_CASE
		END_CHOICE
	END_PARMS
ATP_END_DCL_PARMDEF

/* Callback */
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result ChangeCmd(ClientData clientData, Tcl_Interp *interp,
					 int argc, char *argv [])
#else
Atp_Result ChangeCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	switch(Atp_Num("attribute")) {
		case CHANGE_RANGE:
			/* Change minimum and maximum range values. */
			parmdef_entry_ptr->Min = Atp_Num("min");
			parmdef_entry_ptr->Max = Atp_Num("max");
			break;
		case CHANGE_DEFAULT_NUMBER:
			parmdef_entry_ptr->Default = Atp_Num("defnum");
			break;
		case CHANGE_DEFAULT_STRING:
			parmdef_entry_ptr->DataPointer = Atp_Strdup(Atp_Str("defstr"));
			break;
	}

	/* Reset */
	CmdRecPtr = NULL;
	parmdef_entry_ptr = NULL;

	Tcl_SetResult(interp, "", TCL_STATIC);

	return ATP_OK;
}

/*+*****************************************************************

	Command:		setcase

	Copyright:		BNR Europe Limited,	1993
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Set case sensitivity of command name.
					This is	achieved by using the Tcl
					"unknown" command which is called when a
					command is not recognised.

	Notes:			If you are using the Tcl "defs" file, it
					defines "unknown" to "".

******************************************************************-*/
ATP_DCL_PARMDEF(SetcaseParms)
	BEGIN_PARMS
		bool_def("mode", "case sensitivity", NULL)
	END_PARMS
ATP_END_DCL_PARMDEF

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result SetcaseCmd(ClientData clientData, Tcl_Interp *interp,
					  int argc, char *argv[])
#else
Atp_Result SetcaseCmd(clientData, interp, argc, argv)
	ClientData	clientData;
	Tcl_Interp	*interp;
	int			argc;
	char		*argv[];
#endif
{
	int result = ATP_OK;

	Tcl_DeleteCommand(interp, "unknown");

	switch(Atp_Bool("mode")) {
		case 0: { /* set non case-sensitive mode */
			Tcl_CreateCommand(interp, "unknown", Atp2Tcl_UnknownCmd,
							  (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
			break;
		}
		case 1: { /* set case-sensitive mode */
			result = Tcl_Eval(interp, "proc unknown {} {}; rename unknown \"\"");
			break;
		}
		default: break;
	}
	return result;
}

/*
 * Functions to create and delete miscellaneous commands.
 */
struct {
	char		*cmdName, *cmdDesc;
	Atp_Result	(*cmdProc) _PROTO_((ClientData clientdata,
									Tcl_Interp *interp,
									int argc, char **argv));
	Atp_ParmDef	parmdef;
	ClientData	clientdata;
} MiscCmds [] = {
	{"check", "verify Tcl command interface relayed OK by ATP",
			CheckTclInterfaceCmd, NULL, (ClientData) "check"},
	{"recur", "invokes nested command", RecurCmd, RecurParms,
			(ClientData) 0},
	{"llc_test", "11c command self-test", LLC_TEST_Cmd, NULL,
			(ClientData) 0},
	{"logtest", "Log commands and results to test file",
			LogtestCmd, LogtestParms, (ClientData) 0},
	{"change", "Sets/changes the attributes of a parameter (FOR TESTING ONLY)",
			ChangeCmd, ChangeParms, (ClientData) 0},
	{"setcase", "Sets command name case sensitivity using 's unknown command",
			SetcaseCmd, SetcaseParms, (ClientData) 0},
	{"resetbool", "Resets bool command vproc checklist to zero",
			ResetBooleanVprocChecklist, 0, (ClientData) 0},
	{NULL}
};

#if defined(__STDC__) || defined(__cplusplus)
int CreateMiscCmds( Tcl_Interp *interp )
#else
int CreateMiscCmds(interp)
	Tcl_Interp *interp;
#endif
{
	int x = 0, help_id = 0;

	/* Stick commands in existing miscellaneous area. */
	help_id = Atp_CreateHelpArea(ATP_HELPCMD_OPTION_MISC, NULL);

	for (x = 0; MiscCmds [x] . cmdName != NULL; x++) {
	   Atp2Tcl_CreateCommand(interp,
							 MiscCmds[x].cmdName, MiscCmds[x].cmdDesc,
							 help_id,
							 MiscCmds[x].cmdProc, MiscCmds[x].parmdef,
							 MiscCmds[x].clientdata,
							 (Tcl_CmdDeleteProc *) NULL);
	}

	return x;
}

/* Cleanup routines to be called by SLP when exiting program. */
int Cleanup_Atpexmp()
{
	printf("Goodbye!\n");
	return 1;
}

/*
 * Additional Help Information for the SLP command line editor.
 */
char * NoteOnHistoryMechanism [] = {
	"NOTE that the history mechanism provided by the command line editor",
	"is different from and independent of that which is provided by Tcl.",
	NULL
};

/*+**************************************************************************

	Function:		main()

	Copyright:		BNR Europe Limited, 1993, 1994
					Bell-Northern Research
					Northern Telecom / Nortel

	Description:	Main control program.
					Initializes SLP, Tcl, and ATP.
					Creates help areas for on-line help system.
					Creates commands.
					Initializes output paging facility.
					Adds English textual help information.
					Manages I/O.
					Logs commands and results to test files.

**************************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
int main( int argc, char *argv[] )
#else
int main(argc, argv)
	int argc;
	char *argv[];
#endif
{
	Tcl_Interp	*interp = NULL;
	int			init_result = ATP_OK;
	char		*manpath = NULL;
	int			AutoTestMode = 0;

	/* IDs for various help areas of the help system. */
	int n_id, s_id, d_id, k_id, b_id, nu_id, rb_id, ch_id, cp_id;

#ifdef PLSID_DEFINED
	/* Stick in the real PLS ID in the version information. */
	{
	char *fmtstr;
	(void) sprintf(atpexmp_versionInfo[0],
				   (fmtstr = Atp_Strdup(atpexmp_versionInfo[0])),
				   pisid);
	FREE((void *)fmtstr);
	}
#endif

	/*
	 *	Create a Tcl Interpreter.
	 *
	 *	Some aspects of ATP may accommodate multiple interpreters,
	 *	and some don't. ATP2TCL has NOT been designed and tested for
	 *	the use of multiple interpreters. If there is a requirement,
	 *	please submit your request.
	 */
	interp = Tcl_CreateInterp();

	/* See if atpexmp invoked with -oAutoTest option. */
	if ((argc == 2) && (strcmp(argv[1] , "-oAutoTest") == 0))
	  AutoTestMode = 1;

#ifdef TCL_MEM_DEBUG
	Tcl_InitMemory(interp);
	Tcl_CreateCommand(interp, ATP_MEMDEBUG_CMD, CheckmemCmd,
					  (ClientData) ATP_MEMDEBUG_CMD,
					  (Tcl_CmdDeleteProc *) NULL);
#endif

	/*
	 *	Initialize ATP and the ATP2TCL adaptor.
	 *	Initialization includes the setting up of the atp2tcl interface,
	 *	such as command table access and return codes.
	 *	Other duties include the creation of ATP built-in commands
	 *	such as "help" and "man".
	 */
	init_result = Atp2Tcl_Init(interp);
	if (init_result != ATP_OK) {
	  (void) fprintf(stderr, "ATP initialisation error.\n");
	  exit(1);
	}

	/*
	 *	Initialize frontend editor SLP (pronounced "slap").
	 *
	 *	Redefines Tcl's "source" and "exit" commands.
	 *	Creates "echo" command ( uses Tcl_Concat() ).
	 *
	 *	Calls Slp_SetTclInterp() to register Tcl interp with SLP.
	 *	See also Slp_GetTclInterp() in Cleanup_Atpexmp().
	 *
	 *	SLP incorporates getline.c by Chris Thewalt of UCB.
	 *	We decided to use getline rather than GNU readline because
	 *	of the strong terms and conditions of the GNU General Public
	 *	License as published by the Free Software Foundation.
	 */
	Slp_InitTclInterp(interp);

	/*
	 *	Create Help Areas for the built-in "help" command.
	 *	Default help areas are created when Atp2Tcl_Init()
	 *	is invoked.
	 */
	n_id = Atp_CreateHelpArea("number", "whole numbers");
	s_id = Atp_CreateHelpArea("string", "C null-terminated strings");
	d_id = Atp_CreateHelpArea("databytes", "hexadecimial data");
	k_id = Atp_CreateHelpArea("keyword", "enumerated type");
	b_id = Atp_CreateHelpArea("boolean", "true or false values");
	nu_id = Atp_CreateHelpArea("null", "null filler or empty type");
	rb_id = Atp_CreateHelpArea("repeat_block", "repeated parameters");
	ch_id = Atp_CreateHelpArea("choice", "selectable parameters");
	cp_id = Atp_CreateHelpArea("copyright", "copyright information");

	/*
	 *	Create commands in ATP and Tcl.
	 *
	 *	Also, let the on-line help system know what type of command
	 *	it is by using the help ID obtained from Atp_CreateHelpArea.
	 *	If a command does not belong anywhere, specify a help ID of
	 *	zero.
	 *
	 *	If you like, you do not have to use ATP, just use
	 *	Tcl_CreateCommand on its own.
	 *
	 *	You may use Tcl_DeleteCommand at the end of the program, but
	 *	the resources within ATP will not be freed automatically.
	 *	There is no Atp2Tcl_DeleteCommand or Atp_DeleteCommand. This
	 *	is because under most circumstances, the program makes an
	 *	exit and all memory resources disappear. If there is a
	 *	specific need or requirement, please submit a request.
	 */

	Atp2Tcl_CreateCommand(interp, "square", "Squares a number",
						  n_id, SquareCmd, SquareParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "add", "Adds two numbers",
						  n_id, AddCmd, AddParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "sum", "Adds a series of numbers",
						  rb_id, SumCmd, SumParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "substr",
						  "Makes a substring of a given string",
						  s_id, SubstrCmd, SubStrParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "matchpaint", "Matches paint",
						  ch_id, MatchPaintCmd, MatchPaintParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "echohex","Prints hex data in ASCII",
						  d_id, EchoHexToAsciiCmd, DataBytesParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "echobcd","Echoes BCD digits",
						  d_id, EchoBcdCmd, EchoBcdParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "weekday","weekday keywords",
						  k_id, WeekdayCmd, WkDayPD,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "bool", "boolean command",
						  b_id, TestBooleanCmd, BOOL_PD,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "num", "Check number overflow",
						  n_id, CheckNumCmd, CHK_NUM_PD,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "unum", "Check unsigned number overflow",
						  n_id, CheckUnsNumCmd, CHK_UNUM_PD,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "plot", "plot graph",
						  rb_id, PlotGraph, GraphCoordinates_PD,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "llc", "PRI Low Level Compatibility",
						  ch_id, LLC_Cmd, IE_llc_Parms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "Estimate",
						  "Boehm Software Development Cost Estimation",
						  n_id, EstimateCmd, EstimateParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "optrpt",
						  "command to test optional repeat block",
						  rb_id, optrpt_cmd, optrpt_parms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp,"query","command to test optional choice",
						  ch_id, QueryCmd, QueryParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "more", "output paging using more",
						  nu_id, MoreCmd, more_parms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "null",
						  "Test command containing a NULL parameter",
						  nu_id, NullCmd, NullParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "send",
						  "command to test optional rpt block of databytes",
						  d_id, SendCmd, SendParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
	Atp2Tcl_CreateCommand(interp, "choice", "Nested CHOICE construct tests",
						  ch_id, TestChoiceCmd, TestChoiceParms,
						  (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

	CreateMiscCmds(interp);

	/*
	 *	Enable output paging feature by calling Atp2Tcl_GetPager.
	 *
	 *	Provides output paging capability by creating the built-in
	 *	"paging" and "pager" commands, then returning a pointer to
	 *	the pager function to the application caller to use instead
	 *	of a printf function.
	 *
	 *	Atp2Tcl_GetPager returns a pointer to a function returning
	 *	int. Here, it is assigned to Slp_Printf since SLP invokes
	 *	Tcl_Eval and displays Tcl_GetStringResult(interp).
	 */
	Slp_Printf = Atp2Tcl_GetPager(interp);

	/*
	 *	Above, we created some help areas using Atp_CreateHelpArea.
	 *	All help areas can be displayed using the "help" command.
	 *	(In fact, a dynamic parmdef table is created containing
	 *	built-in help areas and new ones we've just added. Thus,
	 *	"help ?" and "man help" also show the "help" command
	 *	definition.) We then created some commands whilst also
	 *	assigning them to appropriate help areas.
	 *
	 *	Now, we use Atp_AddHelpInfo to add some more help information
	 *	in the form of English textual paragraphs. These are in the
	 *	form of char * [] terminated by NULL.
	 *
	 *	Here, we add a header section and a footer section to the
	 *	"substr" command. Then, we add a summary section to the
	 *	"number" help area.
	 *
	 *	Also shown is an ATP2TCL version of Atp_AddHelpInfo -
	 *	Atp2Tcl_AddHelpInfo, which takes an Tcl_Interp argument too.
	 *
	 *	The Atp_AddHelpInfo call to add information under
	 *	ATP_HELPCMD_OPTION_VERSION	is additional. Version
	 *	information on ATP and Tcl are automatically added when you
	 *	first run the "help -v" command. Other default help areas
	 *	alongside ATP_HELPCMD_OPTION_VERSION are listed in atph.h but
	 *	you do not need to use them.
	 *
	 *	Notice that we created a "copyright" help area with no
	 *	commands attached - this is O.K. as we only wanted to add
	 *	textual information on copyright to the help system.
	 *
	 *	It is always good practice to include complete, accurate and
	 *	concise English help information in your application to guide
	 *	the user through the program, especially when it is the first
	 *	time (or when someone is in a hurry).
	 */
	/* Add help information (manpage headers and footers) to commands. */
	Atp_AddHelpInfo(ATP_MANPAGE_HEADER, "substr", substr_manpg_header);
	Atp_AddHelpInfo(ATP_MANPAGE_FOOTER, "substr", substr_manpg_footer);

	/* Add help information summary to help area. */
	Atp_AddHelpInfo(ATP_HELP_AREA_SUMMARY, "number",
					number_help_area_summary);

	/* Add manpage header and footer to estimate command. */
	Atp2Tcl_AddHelpInfo(interp, ATP_MANPAGE_HEADER,
						"estimate", estimate_desc_header);
	Atp_AddHelpInfo(ATP_MANPAGE_FOOTER, "estimate", estimate_desc_footer);

	/* Add version information to help system. */
	Atp_AddHelpInfo(ATP_HELP_AREA_SUMMARY,
					ATP_HELPCMD_OPTION_VERSION,
					atpexmp_versionInfo);
	Atp_AddHelpInfo(ATP_HELP_AREA_SUMMARY,
					ATP_HELPCMD_OPTION_VERSION,
					Slp_VersionInfo);

	/* Add copyright information to help system (this is up to you). */
	Atp_AddHelpInfo(ATP_HELP_AREA_SUMMARY, "copyright", Atp_Copyright);
	Atp_AddHelpInfo(ATP_HELP_AREA_SUMMARY, "copyright", Tcl_Copyright);
	Atp_AddHelpInfo(ATP_HELP_AREA_SUMMARY, "copyright", Slp_Copyright);

	/* Add help information to logtest manpage. */
	Atp_AddHelpInfo(ATP_MANPAGE_HEADER, "logtest",
					Atp_Logtest_Manpage_Header);

	/* Add help information on SLP command line editing keystrokes. */
	Atp_AddHelpInfo(ATP_HELP_AREA_SUMMARY,
					ATP_HELPCMD_OPTION_MISC,
					Slp_KeystrokesHelpText);
	Atp_AddHelpInfo(ATP_HELP_AREA_SUMMARY,
					ATP_HELPCMD_OPTION_MISC,
					NoteOnHistoryMechanism);

	/*
	 *	Force search path $MANPATH to include directories used by
	 *	this application first. Search standard directories last as
	 *	some Tcl commands have the same name as system commands (e.g.
	 *	cd).
	 */
	Atp_AdvPrintf("MANPATH=/Users/alwynteh/dev/Tcl-Tk/tcl8.6.12/doc");
	Atp_AdvPrintf(":%s/man", getenv("HOME"));
	Atp_AdvPrintf(":~/man");
	Atp_AdvPrintf(":/users/guest/man");
	char *_manpath = getenv("MANPATH");
	if (_manpath != NULL)
	  Atp_AdvPrintf(":%s", _manpath); /* do this last */
	putenv(manpath = Atp_AdvGets());

	/* Defaults prompt to PROMPT. */
	Slp_SetPrompt(PROMPT);

	/*
	 *	Initialize Tcl I/O routines. Command line screen width is
	 *	initialized to 80 columns. If the prompt has not yet been
	 *	defined, it is set to "Tcl-> ".
	 */
	if (!AutoTestMode)
	  Slp_InitStdio();

	/*
	 *	Display currently active prompt on screen (i.e. to stdout).
	 *	Slp_OutputPrompt() calls Slp_GetPrompt() to obtain the prompt.
	 */
	if (!AutoTestMode)
	  Slp_OutputPrompt();

	/*
	 *	If you have something to cleanup before exiting the program,
	 *	declare a cleanup routine to be called by SLP's modified
	 *	"exit" command. Alternatively, redefine your own "exit"
	 *	command. See also note on the use of Tcl_DeleteInterp ()
	 *	above.
	 */
	Slp_SetCleanupProc(Cleanup_Atpexmp);

	/*
	 *	Loop to accept keyboard input and log command/resuit.
	 *	See also tclTest.c in the official  distribution.
	 *
	 *	Alternatively, Slp_StdinHandler() may be registered with an
	 *	asynchronous input handler which accepts input from other sources.
	 *
	 *	Slp_StdinHandler() obtains the current active  interp from
	 *	Slp_GetTclInterp() in which to execute commands by calling
	 *	Tcl_Eval() (in the "source" command) or Tcl_RecordAndEval().
	 *
	 * Atp_LogTestFile() is defined here in atpexmp.c (see above).
	 */

	if (!AutoTestMode) {
	  for (;;) {
		 Slp_StdinHandler();
		 Atp_LogTestFile(interp);
#ifdef TCL_MEM_DEBUG
		 if (quitFlag) {
		   Tcl_DeleteInterp(interp);
		   Tcl_DumpActiveMemory(dumpFile);
		   system("reset");
		   exit(0);
		 }
#endif
	  }
	}
	else {
		AutoTest_Loop(interp);
	}

	return 1;
}

/*
	APPENDIX A - ATP Dynamic Printf functions
	-----------------------------------------

	NAME
		Atp_DvsPrintf() - Dynamic Varargs String PRINTF

		Atp_AdvPrintf() - Accumulating Dynamic Varargs PRINTF

		Atp_AdvGets()	- Accumulating Dynamic Varargs GET String
		Atp_AdvGetsn()	- Accumulating Dynamic Varargs GET String + Number

		Atp_AdvSetDefBufsize()	- Sets the default minimum buffer size

		Atp_AdvGetBufsize()		- Gets the actual current buffer size
		Atp_AdvGetDefBufsize()	- Gets the default minimum buffer size

		Atp_AdvResetDefBufsize() - Resets the default buffer size value
								   to its default value
		Atp_AdvResetBuffer()	 - Resets the buffer

	SYNOPSIS
		#include <atph.h>

		int Atp_DvsPrintf(char **return_str, char *fmtstr, [arg, ...])

		int Atp_AdvPrintf(char *fmtstr, [arg, ...]);
		char * Atp_AdvGets();
		char * Atp_AdvGetsn(int *count);

		int Atp_AdvSetDefBufsize(int size);
		int Atp_AdvGetBufsize();
		int	Atp_AdvGetDefBufsize();

		int	Atp_AdvResetDefBufsize();
		int	Atp_AdvResetBuffer();

	DESCRIPTION
		This suite of routines is used to format a string into a
		dynamically allocated piece of memory, incrementally if
		required, and returns the result.

		The advantage of these routines over the use of malloc() and
		sprintf() is that it is not necessary to pre-determine the
		maximum size of the resultant string.

		Atp_DvsPrintf works like sprintf, where fmtstr is a printf-style
		format string, but the resultant string is dynamically allocated
		using malloc. If return_str is supplied, it returns the length
		of the resultant string (excluding the ending NULL byte). It
		returns -1 if retum_str is not supplied or if an error occurs.

		Atp_AdvPrintf accumulates the results of the initial and
		subsequent calls, in dynamic memory. In the event of an error,
		the buffer is destroyed and -1 is returned. If successful, it
		returns the length of the resultant string built. When the
		accumulated string is ready for use, it is then returned by
		Atp_AdvGets or Atp_AdvGetsn (if count is non-NULL, the string
		length is returned).

		Atp_AdvSetDefBufsize sets the default minimum buffer size for
		Atp_AdvPrintf to use, but only if the specified size is more
		than zero, otherwise the buffer size remains unchanged. The
		default buffer size used is 4096 bytes. This does not mean a
		single printf statement must not exceed this size, it is just a
		way of allocating initial storage and subsequently reallocating
		memory when more is needed.

		Atp_AdvGetBufsize returns the actual current buffer size used.
		Atp_AdvGetDefBufsize returns the default buffer size used.

		Atp_AdvResetDefBufsize resets the default buffer size to 4096.
		Atp_AdvResetBuffer resets the buffer to its initial default
		state.

		The buffer size specifies the amount of storage to allocate for
		a sequence of calls to Atp_AdvPrintf. When storage needs to be
		increased, the specified buffer size is used as a minimum.

		All dynamic strings returned become the property of the caller
		and must be freed after use.
*/
/*
	REMARKS:	Certain ATP functionalities have not been implemented
				because there has been no formal request.

				Interactive parameter prompting is one of them, but
				hooks within ATP have already been laid. If you can
				realistically benefit from such a feature, especially if
				your ATP application involves the use of a lot of
				complex and long commands, then submit a request
				describing exactly how you would wish to see such a
				feature designed.

				Other features include the real number parameter, and
				common parmdef construct type. The latter is supposed
				to save static program data storage when common parmdefs
				may be defined for multiple use within a parmdef. This
				is preferrable to #define-ing parmdef constructs for
				repeated use.

				The EMPTY_PARMDEF macro has not been used. It is a bit
				out-of-date now because you can simply specify an empty
				parmdef as NULL in Atp2Tcl_CreateCommand. If you want
				to use it, here's how:

				Atp_ParmDefEntry null_parmdef[] = { EMPTY_PARMDEF };

									or

				ATP_DCL_PARMDEF(null_parmdef)
					EMPTY_PARMDEF
				ATP_END_DCL_PARMDEF

				...the latter may be a bit more useful as a place
				holder.

				In ATP's predecessor CLI, there used to be a
				"rest_of_command" parmdef macro. However, CLI is a
				character-oriented parser and did not perform
				pre-tokenizing. In ATP, rest_of_command has been
				removed because it is not possible then to specify the
				types of the trailing parameters, even though Tcl is
				able to generate an argc and argv token list of any size
				without knowing the command specification (i.e. the
				parmdef). Therefore, you cannot write a command with a
				variable parameter list. Hence, such commands as printf
				cannot be written using ATP:

					atpexmp> set STRING abcdefg
					abcdefg
					atpexmp> printf "There are %d chars in \"%s\".\n" \
					>		 [string length $STRING] $STRING

				If you have any bright ideas on how to achieve this
				using ATP, let me know. Alternatively, implement the
				"printf" command using Tcl alone.

				Parameter retrieval functions are adequate for searching
				for single parameters. However, when complex constructs
				are to be accessed, the current practice is to define an
				overlay structure to be used with a pointer to the
				construct in the parameter store. Some find this rather
				tedious so there is an opportunity for improvement,
				perhaps by designing an advanced parameter access
				function to simplify the process. Let us know your
				experience and ideas!

				Error messaging in ATP has been enhanced by appending
				details of the parameter or construct to the error
				message, plus all constructs within which it is nested.
				Thus it may seem rather verbose than the first error
				message line. However, it does help users quickly
				locate and correct input commands.

				If you have any suggestions on improvements on any
				topic, please forward them to me.
*/
/*
	PARMDEF MACRO TEMPLATES (for you to cut & paste):
					Constructs										Type
		-------------------------------------------------	----------------------
		ATP_DCL_PARMDE F(parmdef_name)							Atp_ParmDef
		ATP_END_DCL_PARMDEF

		BEGIN_PARMS
		END_PARMS

		EMPTY_PARMDEF

		BEGIN_LIST(name,desc)									void *
		END_LIST

		BEGIN_OPT_LIST(name,desc,default)						void *
		END_OPT_LIST

		BEGIN_REPEAT(name,desc,min,max,vproc)					Atp_DataDescriptor
		END_REPEAT

		BEGIN_OPT_REPEAT(name,desc,default,min,max,vproc)		Atp_DataDescriptor
		END_OPT_REPEAT

		BEGIN_CHOICE(name,desc,vproc)							Atp_ChoiceDescriptor
		END_CHOICE

		BEGIN_OPT_CHOICE(name,desc,default,vproc)				Atp_ChoiceDescriptor
		END_OPT_CHOICE

		BEGIN_CASE(label,desc,casevalue)
		END_CASE

		CASE(label,desc,casevalue)

					Parameters										Type
		-------------------------------------------------	----------------------
		num_def(name,desc,min,max,vproc)						Atp_NumType
		unsigned_num_def(name,desc,min,max,vproc)				Atp_UnsNumType

		opt_num_def(name,desc,default,min,max,vproc)			Atp_NumType
		opt_unsigned_num_def(name,desc,default,min,max,vproc)	Atp_UnsNumType

		str_def(name,desc,min,max,vproc)						char *
		opt_str_def(name,desc,default,min,max,vproc)			char *

		bool_def(name,desc,vproc)								Atp_BoolType
		opt_bool_def(name,desc,default,vproc)					Atp_BoolType

		keyword_def(name,desc,keys,vproc)
		opt_keyword_def(name,desc,DefaultKeyValue,keys,vproc)
																char * (keyword string)
																Atp_NumType (keyvalue)
																Atp_UnsNumType (index)

		data_bytes_def(name,desc,min,max,vproc)					Atp_DataDescriptor
		opt_data_bytes_def(name,desc,default,min,max,vproc)		Atp_DataDescriptor

		null_def(name,desc)										Atp_NumType
*/

/*
	PARAMETER RETRIEVAL FUNCTIONS:

	Atp_NumType			Atp_Num(parmname);
	Atp_UnsNumType		Atp_UnsignedNum(parmname);
	Atp_UnsNumType		Atp_Index(parmname);
	Atp_BoolType		Atp_Bool(parmname);
	char *				Atp_Str(parmname);
	Atp_ByteType *		Atp_DataBytes(parmname, &count);
	Atp_DataDescriptor	Atp_DataBytesDesc (parmname);
	Atp_ByteType *		Atp_ParmPtr (parmname);
	Atp_ByteType *		Atp_ResetParmPtr();
	Atp_ByteType *		Atp_RptBlockPtr (parmname,	&count) ;
	Atp_DataDescriptor	Atp_RptBlockDesc(parmname);

*/

/* Code cloned from  v6.7 follows: */
/*
 *	Copyright 1987-1991 Regents of the University of California
 *	All rights reserved.
 *
 *	Permission to use, copy, modify, and distribute this
 *	software and its documentation for any purpose and without
 *	fee is hereby granted, provided that the above copyright
 *	notice appears in all copies. The University of California
 *	makes no representations about the suitability of this
 *	software for any purpose. It is provided "as is" without
 *	express or implied warranty.
 */
/*
 *----------------------------------------------------------------------
 ★
 *	CheckmemCmd --
 *
 *		This is the command procedure for the "checkmem" command, which
 *		causes the application to exit after printing information about
 *		memory usage to the file passed to this command as its first
 *		argument.
 *
 *	Results:
 *		Returns a standard Tcl completion code.
 *
 *	Side effects:
 *		None.
 *
 *----------------------------------------------------------------------
 */
#ifdef TCL_MEM_DEBUG
/* ARGSUSED */
#if defined(__STDC__) || defined(__cplusplus)
static
Atp_Result CheckmemCmd(ClientData clientData, Tcl_Interp *interp,
					   int argc, char *argv[])
#else
static int
CheckmemCmd(clientData, interp, argc, argv)
	ClientData	clientData;	/* Not used. */
	Tcl_Interp	*interp;	/*	Interpreter for evaluation.	*/
	int			argc;		/*	Number of arguments. */
	char		*argv[];	/*	String values of arguments.	*/
#endif
{
	if (argc != 2) {
	  Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
			  	  	   " fileName\"", (char *) NULL);
	  return TCL_ERROR;
	}
	strcpy(dumpFile, argv[l]);
	quitFlag = 1;
	return TCL_OK;
}
#endif

#if defined(__STDC__) || defined(__cplusplus)
int AutoTest_Loop( Tcl_Interp *interp )
#else
int AutoTest_Loop(interp)
	Tcl_Interp *interp;
#endif
{
	static Tcl_DString buffer;
	char line[1024], *cmd;
	int result, gotPartial;
	FILE *pager_fp = NULL;

	Tcl_DStringInit(&buffer);

	gotPartial = 0;
	while (1) {
		clearerr(stdin);
		fflush(stdout);
		if (fgets(line, 1000, stdin) == NULL) {
		  if (!gotPartial) {
			/* Call exit procedure to leave program. */
			sprintf(line, "exit 0") ;
			Tcl_Eval(interp, line);
			return 1; /* shouldn't get here */
		  }
		  line[0] = 0;
		}
		cmd = Tcl_DStringAppend(&buffer, line, -1);
		if ((line[0] != 0) && !Tcl_CommandComplete(cmd)) {
		  gotPartial = 1;
		  continue;
		}

		gotPartial = 0;
		result = Tcl_RecordAndEval(interp, cmd, 0);
		Tcl_DStringFree(&buffer);
		if (result == TCL_OK) {
		  if (*Tcl_GetStringResult(interp) != 0) {
			printf ("%s\n" , Tcl_GetStringResult(interp));
		  }
		} else {
			if (result == TCL_ERROR) {
			  fprintf(stderr, "Error");
			} else {
			  fprintf(stderr, "Error %d", result);
			}
			if (*Tcl_GetStringResult(interp) != 0) {
			  printf(": %s\n", Tcl_GetStringResult(interp));
			} else {
			  printf("\n");
			}
		}
	}
}
