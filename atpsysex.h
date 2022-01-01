/* EDITION AC06 (REL002), ITD ACST.177 (95/06/21 05:33:00) -- CLOSED */

/*+************************************************************************

	Module Name:			atpsysex.h

	Copyright:				BNR Europe Limited, 1992-1995
							Bell-Northern Research / BNR
							Northern Telecom / Nortel

	Description:			This header	file contains internal
							declarations of	SYStem Exclusive facilities
							used within the Advanced Token Parser (ATP).

	Modifications:
		Who				When					Description
	-------------  -----------------	---------------------------------
	Alwyn Teh		8 July	1992		Initial Creation
	Alwyn Teh		18 May	1993		Convert to REL001 from PROTO
										stream, adding a few extra
										macros (Atp_ParseRecParmDef,
										Atp_ParseRecCurrPDidx) and
										externing Atp_isConstructOfOptParms.
	Alwyn Teh		20 May	1993		Export DataBytesCount to atph.h
	Alwyn Teh		9 June	1993		Add macro Atp_AppendParmName()
	Alwyn Teh		30 June	1993		Add functions Atp_GetHelpInfo(),
										Atp_DisplayHelpInfo(),
										Atp_AppendHelpInfo().
	Alwyn Teh		4 July	1993		Add functions Atp_DisplayCommands()
										and Atp_DisplayCmdDescs()
	Alwyn Teh		12 July	1993		Changed Atp_DisplayHelpInfo() to
										Atp_DisplayCmdHelpInfo () new
										Atp_DisplayHelpInfo() written.
	Alwyn Teh		13 July	1993		Changed Atp_AppendHelpInfo() to
										Atp_AppendCmdHelpInfo(), former
										function rewritten.
	Alwyn Teh		18 July	1993		Use __Atp_Local_FileName__ instead
										of __FILE__ to conserve space
	Alwyn Teh		21 July	1993		Use ERRLOC only in debug mode, it does
										not look good for an application to
										output filename and line number in an
										error message.
	Alwyn Teh		7 December 1993		Export Atp_AddCmdToHelpArea,
										Atp_GetHelpSubSection

	Alwyn Teh		24 December 1993	Change names Atp_isConstructOfOptParms0 to
										Atp_isConstructOfOptOrNullParms() and
										Atp_AllParmsInParmDefAreOptional() to
										Atp_AllParmsInParmDefAreOptionalOrNull()

	Alwyn Teh		17 January 1994		Need to find memory gobbler,
										replace GOB with Tcl for debugging.
										Use cc -DTCL_MEM_DEBUG but Tcl
										must also have been compiled using
										this flag.
	Alwyn Teh		25 January 1994		Comment out ParmDefPtr in struct
										ParmDefEntry since not implemented
										yet. This saves space in parmdefs.
										Mirror change in atph.h too.
	Alwyn Teh		9 March 1995		Atp_CopyCallFrame() and new function
										Atp_CopyCallFrameElems() belong in
										atpframh.h
	Alwyn Teh		20 March 1995		Transfer Atp_AddCmdToHelpArea() and
										Atp_AppendCmdHelpInfo() to atph.h;
										declare Atp_InvokeVproc() and
										Atp_CheckRange().
	Alwyn Teh		24 March 1995		Implement BCD digits parameter type.
	Alwyn Teh		29 March 1995		Declare Atp_ParserStateRec typedef
										in atph.h but leave definition here.
	Alwyn Teh		4 May 1995			Distinguish user defined procs from
										built-in commands.
	Alwyn Teh		20 June 1995		Rename Atp_VarargStrlen () to
										Atp_FormatStrlen() and export in
										atph.h (so remove it here).
	Alwyn Teh		12 December 2021	Add stdlib.h

************************************************************************_*/

#ifndef _ATPINT
#define _ATPINT

/* Include these system headers in individual files where necessary. */
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>
// #include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "atph.h"

#ifdef TCL_MEM_DEBUG
# include <tcl.h>
#endif

#define ATP_IDENTIFIER_CODE	735078 /* an arbitrary number */

/*
-----------------------------------------------------------------------
	INTERNAL data types and definitions
-----------------------------------------------------------------------
*/

/* ATP's definition of boolean */
#ifndef TRUE
#define TRUE	(Atp_BoolType)1
#endif

#ifndef FALSE
#define FALSE	(Atp_BoolType)0
#endif

#ifndef ON
#define ON		(Atp_BoolType)1
#endif

#ifndef OFF
#define OFF		(Atp_BoolType)0
#endif

/* ATP's definition of NULL */
#ifndef NULL
#define NULL	((void *) 0)
#endif

/* Definition of quotes (to get around bug in xstr if used) */
#define ATP_DOUBLE_QUOTE	'\042'
#define ATP_SINGLE_QUOTE	'\047'

/* Error location - __Atp_Local_FileName__ is defined as static in each file. */
#ifdef	DEBUG
#define ERRLOC __Atp_Local_FileName__, __LINE__
#else
#define ERRLOC NULL, 0
#endif

/* Default number of columns and lines/rows on a terminal. */
#define ATP_DEFAULT_COLUMNS		80
#define ATP_DEFAULT_LINES		34

/* Templates for atphelpc.c Help Area functionality, cloned from atph.h */
#define ATP_BEGIN_CASE_TEMPLATE {ATP_BCS, \
								 (Atp_ParserType)Atp_ProcessCaseConstruct}
#define ATP_END_CASE_TEMPLATE {ATP_ECS}

/*
-----------------------------------------------------------------------

ATP parmcodes:
=============

The following parmcodes are #defined in atph.h to be used in parmdef
macros.

Key:	bits 10		indicate	Parameter Category
		bits 432	indicate	Parameter Class
		bits 65		indicate	Parameter Type
		bit 7		indicates	Optional Parameter

Note:	The use of structure bit-fields, though more convenient, is
		avoided due to its machine implementation-dependent nature
		(whether fields are assigned	left-to-right	or
		right-to-left), so as to maintain portability.

Category 0 (00), NULL CLASS:
===========================

65 432 10	<- bit position
---------
00 000 00	(Code: hex00, oct000,	bin00000000)	NULL
00 001 00	(Code: hex04, oct004,	bin00000100)	EOP

Category 1 (01), CONSTRUCT CLASS:
================================

65 432 10	<- bit position
---------
00 000 01	(Code: hex01, oct001,	bin00000001)	PARMS BEGIN
01 000 01	(Code: hex21, oct041,	bin00100001)	PARMS END
00 001 01	(Code: hex05, oct005,	bin00000101)	LIST BEGIN
01 001 01	(Code: hex25, oct045,	bin00100101)	LIST END
00 010 01	(Code: hex09, oct011,	bin00001001)	REPEAT BEGIN
01 010 01	(Code: hex29, oct051,	bin00101001)	REPEAT END
00 011 01	(Code: hex0D, oct015,	bin00001101)	CHOICE BEGIN
01 011 01	(Code: hex2D, oct055,	bin00101101)	CHOICE END
00 100 01	(Code: hex11, oct021,	bin00010001)	CASE BEGIN
01 100 01	(Code: hex31, oct061,	bin00110001)	CASE END

Category 2 (10), REGULAR PARM CLASS:
===================================

65 432 10	<- bit position
---------
00 000 10	(Code: hex02, oct002,	bin00000010)	NUMBER SIGNED
01 000 10	(Code: hex22, oct042,	bin00100010)	NUMBER UNSIGNED
00 001 10	(Code: hex06, oct006,	bin00000110)	OCTET STRING DATABYTES
01 001 10	(Code: hex26, oct046,	bin00100110)	ASCII OCTET CHARACTER STRING
10 001 10	(Code: hex46, oct106,	bin01000110)	BCD DIGITS	(NIBBLES in OCTETS)
00 010 10	(Code: hex0A, oct012,	bin00001010)	ENUMERATED	KEYWORD
01 010 10	(Code: hex2A, oct052,	bin00101010)	ENUMERATED	BOOLEAN
00 011 10	(Code: hex0E, oct016,	bin00001110)	REAL

Category 3 (11), COMPLEX PARM CLASS:
===================================

65 432 10	<- bit position
---------
00 000 11	(Code: hex03, oct003,	bin00000011)	COMMON PARMDEF
-----------------------------------------------------------------------
*/

/* Number of parmcodes defined */
#define NO_OF_ATPPARMCODES				21

/* Category definitions */
#define ATP_PARM_CATEGORY_NULL			00
#define ATP_PARM_CATEGORY_CONSTRUCT		01
#define ATP_PARM_CATEGORY_REGULAR_PARM	02
#define ATP_PARM_CATEGORY_COMPLEX_PARM	03

/* NULL category Class definitions */
#define ATP_NULL_CLASS_NULLPARM			00
#define ATP_NULL_CLASS_EOP				01

/* CONSTRUCT category Class definitions */
#define ATP_CONSTRUCT_CLASS_PARMS		00
#define ATP_CONSTRUCT_CLASS_LIST		01
#define ATP_CONSTRUCT_CLASS_REPEAT		02
#define ATP_CONSTRUCT_CLASS_CHOICE		03
#define ATP_CONSTRUCT_CLASS_CASE		04

/* CONSTRUCT category Type definitions */
#define ATP_CONSTRUCT_TYPE_BEGIN		00
#define ATP_CONSTRUCT_TYPE_END			01

/* REGULAR PARM Class definitions */
#define ATP_REGULAR_PARM_CLASS_NUMBER	00
#define ATP_REGULAR_PARM_CLASS_OCTETS	01
#define ATP_REGULAR_PARM_CLASS_ENUM		02
#define ATP_REGULAR_PARM_CLASS_REAL		03

/* REGULAR PARM Number Class Type definitions */
#define ATP_PARM_TYPE_SIGNED_NUMBER		00
#define ATP_PARM_TYPE_UNSIGNED_NUMBER	01

/* REGULAR PARM Octets Class Type definitions */
#define ATP_PARM_TYPE_DATABYTES			00
#define ATP_PARM_TYPE_ASCII_STRING		01
#define ATP_PARM_TYPE_BCD_DIGITS		02

/* REGULAR PARM Enum Class Type definitions */
#define ATP_PARM_TYPE_KEYWORD			00
#define ATP_PARM_TYPE_BOOLEAN			01

/* COMPLEX PARM Class definitions */
#define ATP_COMPLEX_PARM_CLASS_COMPDF	00

/*
-----------------------------------------------------------------------
	INTERNAL parmcode discrimination macros
-----------------------------------------------------------------------
*/
/* Bit fields sizes */
#define ATP_CATEGORY_SIZE			02
#define ATP_CLASS_SIZE				03
#define ATP_PARMTYPE_SIZE			02
#define ATP_OPTFLAG_SIZE			01

/* Masks for reading parmcode bit fields */
#define ATP_CATEGORY_MASK			003		/* 00000011 */
#define ATP_CLASS_MASK				034		/* 00011100 */
#define ATP_PARMTYPE_MASK			0140	/* 01100000 */

#ifndef ATP_OPTPARM_MASK /* this is used in atph.h */
#define ATP_OPTPARM_MASK			0200	/* 10000000 */
#endif

/* Define macros and right-justify bits where necessary. */
#define AtpParmCategory(parmcode) \
		((parmcode) & ATP_CATEGORY_MASK)

#define AtpParmClass(parmcode) \
		(((parmcode) & ATP_CLASS_MASK) >> (ATP_CATEGORY_SIZE))

#define AtpParmType(parmcode) \
		(((parmcode) & ATP_PARMTYPE_MASK) >> \
		 (ATP_CATEGORY_SIZE + ATP_CLASS_SIZE))

#define AtpParmOptFlag(parmcode) \
		(((parmcode) & ATP_OPTPARM_MASK) >> \
		 (ATP_CATEGORY_SIZE + ATP_CLASS_SIZE + ATP_PARMTYPE_SIZE))

/*
	The following 3 bitfield setting macros require that parmcode has
	storage.
*/
#define Atp_SetParmCategory(parmcode, setvalue) \
		{(parmcode) &= ~ATP_CATEGORY_MASK; \
		(parmcode) |= setvalue;}

#define Atp_SetParmClass(parmcode, setvalue) \
		{(parmcode) &= ~ATP_CLASS_MASK; \
		(parmcode) |= (setvalue << ATP_CATEGORY_SIZE);}

#define Atp_SetParmType(parmcode, setvalue) \
		{(parmcode) &= ~ATP_PARMTYPE_MASK; \
		(parmcode) |= (setvalue << \
					   (ATP_CATEGORY_SIZE + ATP_CLASS_SIZE));}

/*
	If a parameter definition entry does not care about the range of
	its parameter, its Min value is set to be greater than its Max
	value.
*/
#define ATP_PARMRANGE_DONT_CARE(ParmEntryPtr, ParmType) \
		((ParmType)ParmEntryPtr->Min > (ParmType)ParmEntryPtr->Max)

/*
	Atp_SetOptParm() operates on parmcode and returns a value with
	the optional parameter indicator bit set. It does NOT store the
	result in parmcode, which may have NO storage.
*/
#ifndef Atp_SetOptParm
#  define	Atp_SetOptParm (parmcode) \
					((parmcode) | ATP_OPTPARM_MASK)
#endif

#define AtpParmIsOptional(parmcode) \
			(((parmcode) & ATP_OPTPARM_MASK) >> \
			(ATP_CATEGORY_SIZE + ATP_CLASS_SIZE + ATP_PARMTYPE_SIZE) )

/* Unset the optional flag if set. */
#define Atp_PARMCODE(parmcode) \
			((parmcode) & ~ATP_OPTPARM_MASK)

#define isAtpNull(parmcode) \
			((AtpParmCategory(parmcode) == ATP_PARM_CATEGORY_NULL) && \
			(AtpParmClass(parmcode) == ATP_NULL_CLASS_NULLPARM))

#define isAtpEOP(parmcode) \
			((AtpParmCategory(parmcode) == ATP_PARM_CATEGORY_NULL) && \
			(AtpParmClass(parmcode) -- ATP_NULL_CLASS_EOP))

#define isAtpConstruct(parmcode) \
			(AtpParmCategory(parmcode) == ATP_PARM_CATEGORY_CONSTRUCT)

#define isAtpRegularParm(parmcode) \
			(AtpParmCategory(parmcode) == ATP_PARM_CATEGORY_REGULAR_PARM)

#define isAtpComplexParm(parmcode) \
			(AtpParmCategory(parmcode) == ATP_PARM_CATEGORY_COMPLEX_PARM)

#define isAtpBeginConstruct(parmcode) \
			(isAtpConstruct(parmcode) && \
			(AtpParmType(parmcode) == ATP_CONSTRUCT_TYPE_BEGIN))

#define isAtpEndConstruct(parmcode) \
			(isAtpConstruct(parmcode) && \
			(AtpParmType(parmcode) == ATP_CONSTRUCT_TYPE_END))

/*
-----------------------------------------------------------------------
	INTERNAL general-purpose macros
-----------------------------------------------------------------------
*/
#if 0
#define odd(n)		(((n) %	2)	!=	0)
#define even(n)		(((n) %	2)	==	0)
#else
#define odd(n)		((n) & 1)
#define even(n)		(odd(n)	==	0)
#endif

#define isodigit(d)	(((d) >= 'O') && ((d) <= '7'))

#define AtpIncrPtr(ptr, type) ptr = (unsigned char *)ptr + sizeof(type)

/*
	Checking of returned NULL pointers from malloc routines may be
	done outside the caller code, hence a set of internal Atp_Malloc()
	type routines are used. These accept an additional argument, fn,
	a function for tidy-up tasks,- which if set to NULL, will output
	error messages and perform a longjmp to an appropriate
	environment.
*/

#ifdef USE_OFFICIAL_MALLOC_ROUTINES

#ifndef MALLOC
#define MALLOC(size, fn)			malloc((unsigned)(size))
#endif

#ifndef CALLOC
#define CALLOC(nelem, elsize, fn)	calloc((unsigned)(nelem), (unsigned)(elsize))
#endif

#ifndef REALLOC
#define REALLOC(ptr, size, fn)		realloc(ptr, (unsigned)(size))
#endif

#ifndef FREE
#define FREE(ptr)					free(ptr)
#endif

#else

#define MALLOC(size, fn) \
		Atp_Malloc((unsigned)(size), fn, ERRLOC)

#define CALLOC(nelem, elsize, fn) \
		Atp_Calloc((unsigned)(nelem) , (unsigned) (elsize), fn, ERRLOC)

#define REALLOC(ptr, size, fn) \
		Atp_Realloc(ptr, (unsigned)(size), fn, ERRLOC)

#define FREE(ptr)	free(ptr)

EXTERN void * Atp_Malloc _PROTO_((unsigned size,
								  void (*fn)_PROTO_((unsigned size,
													 char *filename,
													 int line_number)),
								  char *filename, int line_number));

EXTERN void * Atp_Calloc _PROTO_((unsigned nelem, unsigned elsize,
								  void (*fn)_PROTO_((unsigned size,
													 char *filename,
													 int line_number)),
								  char *filename, int line_number));

EXTERN void * Atp_Realloc _PROTO_((void *ptr, unsigned size,
								   void (*fn)_PROTO_((unsigned size,
													  char *filename,
													  int line_number)),
								   char *filename, int line_number));

EXTERN void	Atp_OutOfMemoryHandler _PROTO_((unsigned size,
											char *filename,
											int line_number));

#endif

#ifdef TCL_MEM_DEBUG
/*
	Beware the ATP re-definitions - the real Atp_Malloc functions are
	not called at all after these redefinitions:
*/

#ifdef	MALLOC
#	undef		MALLOC
#endif
#define	MALLOC(size, mem_handler)	((void *) ckalloc((size)))

#ifdef	REALLOC
#	undef		REALLOC
#endif
#define REALLOC(ptr, size, mem_handler) \
			   ((void *) ckrealloc((ptr),(size)))

#ifdef CALLOC
#	undef		CALLOC
#endif
#define CALLOC(nelem, elsize, mem_handler)	\
			  ((void *) Atp_MemDebug_Calloc((nelem),(elsize),__FILE__,__LINE__))

#ifdef	FREE
#	undef		FREE
#endif
#define	FREE(ptr)	ckfree(ptr)

EXTERN char *		Atp_MemDebug_Calloc _PROTO_((size_t nelem,
												 size_t elsize,
												 char *filename,
												 int line));

#define Atp_Strdup (s) Atp_MemDebug_Strdup((s),__FILE__,__LINE__)

EXTERN char *	Atp_MemDebug_Strdup _PROTO_((char *s,
											 char *filename,
											 int line));

#endif /* TCL_MEM_DEBUG */

/*
-----------------------------------------------------------------------
	INTERNAL data structures
-----------------------------------------------------------------------
*/

typedef short Atp_PDindexType;

/* Internal ParmDef structure */
typedef struct ParmDefEntry		*ParmDefTable;

/*
	MUST be parallel to Atp_ParmDef Entry in atph.h but hiding
	internal fields.
*/
typedef struct ParmDefEntry
{
	Atp_ParmCode	parmcode;		/*	Parm ID/classification code		*/
	Atp_ParserType	parser;			/*	Parameter parser to use			*/
	char			*Name;			/*	Parameter name					*/
	char			*Desc;			/*	Description of parameter		*/
	Atp_LargestType	Min;			/*	Minimum value of parameter		*/
	Atp_LargestType	Max;			/*	Maximum value of parameter		*/
	char *			(*vproc)		/*	Verification procedure			*/
							_PROTO_((void *valuePtr, Atp_BoolType isUserValue));
	Atp_LargestType	Default;		/*	Default value of parameter		*/
	Atp_KeywordType	*KeyTabPtr;		/*	Pointer to table of keywords	*/
	void			*DataPointer;	/* Generic pointer to data			*/
	/*ParmDefEntry	*ParmDefPtr;*/	/* Pointer to a common parmdef		*/

	/* Internal fields */
	Atp_PDindexType	matchIndex; 	/* for constructs' use				*/

} ParmDefEntry;

/*
	Parameter store control information. Actual parameter data
	values are stored separately, accessed by parmValue.
*/
typedef struct pstoreInfo
{
	char				*parmName;		/*	Parameter name specified
											in Atp_ParmDef table. */
	int					is_default;		/*	1 = parmValue is default value,
											0 = parmValue is user value */
	Atp_ParmCode		parmcode;		/*	Parameter code, including ATP_EOP */

	union {	/* Selection based	on parmcode */
		Atp_UnsNumType	parmSize;		/*	Size of parameter if necessary
											(e.g. data bytes) */
		Atp_UnsNumType	RptBlockCount;	/*	Number of instances of
											a REPEAT block. */
		Atp_UnsNumType	ChoiceCaseIdx;	/*	Index of keyword in
											choice keys */
		Atp_UnsNumType	KeywordCaseIdx;	/*	Index of keyword in
											Atp_KeywordTab table */
		int				TokenCount;		/*	Number of tokens in
											token list */
	} TypeDependentInfo;

	void				*parmValue;		/*	pointer to parameter value */

	ParmDefEntry		*ParmDefEntryPtr;/*	pointer to parmdef entry
											for access to info */
	void				*DataPointer; 	/*	general-purpose pointer
											e.g. pointer for keyword string */

	struct pstoreInfo	*upLevel,		/* Pointer to upper level parmstore, if any. */
						*downLevel;		/* Pointer to lower level parmstore, if any. */

} ParmStoreInfo;

/*
	Transient dynamic structure for keeping track of construction of
	parameter store.
*/
typedef struct psMemMgtNode {
	ParmStoreInfo		*CtrlStore;			/*	Points to start of control store.	*/
	void				*DataStore;			/*	Points to beginning of data store.	*/
	ParmStoreInfo		*CurrCtrlPtr;		/*	Current control store	pointer.	*/
	void				*CurrDataPtr;		/*	Current data store pointer.			*/
	ParmStoreInfo		*EndOfCtrlPtr;		/*	End of control store pointer.		*/
	void				*EndOfDataPtr;		/*	End of data store pointer.			*/
	unsigned int		SizeOfCtrlStore;	/*	Size of control store in bytes.		*/
	unsigned int		SizeOfDataStore;	/*	Size of data store in bytes.		*/
	int					CurrCtrlIndex;		/*	Current control index.				*/
	struct psMemMgtNode	*prevNode;			/*	Points to previous struct.			*/
	struct psMemMgtNode	*nextNode;			/*	Points to next struct.				*/
} ParmStoreMemMgtNode;

/*
	This is a parallel structure to Atp_CmdRec in atph.h, so if
	externally-visible fields are changed, or additional ones
	introduced, update the other also.
*/
typedef struct {
	/* Externally-visible fields: */
	char				*cmdName;		/*	Command name in lower case		*/
	char				*cmdDesc;		/*	Brief command description		*/
	void				*helpInfo;		/*	On-line HELP information		*/
	Atp_CmdCallBackType	callBack;		/*	Pointer to callBack function	*/
	ParmDefEntry *		parmDef;		/*	Pointer to parameter table		*/
	void				*clientData;	/*	Original client data			*/

	/* System exclusive internal fields: */
	char				*cmdNameOrig;	/*	Copy of orig command name		*/
	int					Atp_ID_Code;	/*	ATP discriminator ID code		*/
	int					ParmDefChecked; /* Has parmdef been checked? 		*/
	int					NoOfPDentries;	/*	Number of entries in parmDef	*/
	int					NoOfParameters;	/*	Number of parameters   "		*/
	int					count;			/*	Number of times cmd executed	*/
} CommandRecord;

/*
	INTERNAL ATP COMMAND REGISTER (assuming front end language is Tcl) :
		A command register of ATP commands is kept to distinguish ATP
		commands from Tcl commands. However, since multiple interpreters
		may be found in an application, this register is organised into
		command groups, if more than one group (Tcl_Interp) is used.
		Atp_FindCommand() searches the register to locate and verify the
		command record pointer or command name given. (i.e. 2 search
		modes).

		NOTE: Support of multiple Tcl interpreters is not guaranteed although
		certain ATP facilities may work under such scenarios.
		No such tests have been conducted to observe this condition.
		If this is a requirement, then a special design must be added.
*/
EXTERN Atp_CmdRec * Atp_FindCommand _PROTO_(( void *ptr,
											  int mode ));
									/* 0 = find record, 1 = find command name */

/*
	An extendible structure containing arguments passed between internal
	parser functions during the data-driven parsing process as the parmdef
	is being scanned. Thus, the values of these fields represent the state
	of the parser at any instance.
*/
struct Atp_ParserStateRec {
	/* Input token list. */
	int				argc;
	char			**argv;

	/* Parmdef details. */
	ParmDefEntry *	ParmDefPtr;
	int				CurrPDidx;
	int				NoOfPDentries;

	/* Control information. */
	int				CurrArgvIdx;
	int				TermArgvIdx;
	int				NestCount;
	int				RptBlkTerminatorSize_Known;
	int				RptBlkTerminatorSize;

	/* Parameter value information. */
	Atp_LargestType	defaultValue;
	void			*defaultPointer;
	int				ValueUsedIsDefault;

	/* Parsing result, error string, if any. */
	Atp_Result		result;
	char			*ReturnStr;
};

/* Transparent layer of parser selection function */
EXTERN Atp_Result Atp_SelectInputAndParseParm
						_PROTO_((Atp_ParserStateRec *parseRec,
								 Atp_ArgParserType parser,
								 char *ascii_parmtype, ...));

/*
	INTERACTIVE PROMPTING - NOT IMPLEMENTED / STUB ONLY
	A structure containing descriptions of the attributes of a parameter
	which may be used in interactive prompting to enable user to input
	valid data, especially after repeated attempts in doing so.

	All prompt fields are strings for ease of output.
*/
typedef struct {
	char	*ParmName;
	char	*ParmDesc;
	char	*MinValueStr;
	char	*MaxValueStr;
	char	*DefaultValueStr;
	char	*ReadyMadePrompt;
	int		NoOfAttempts;
} Atp_PromptParmRec;

/*
 *	Hierarchical structures for on-line help information.
 *	Used for command summary, manpage header and footer
 *	pointers to subsection paragraphs.
 */
#define ATP_DEFAULT_NO_OF_HELP_SUBSECTIONS	3

typedef struct {
	int	SubSectionSlots;
	char	* *HelpSubSections[ATP_DEFAULT_NO_OF_HELP_SUBSECTIONS+1];
} Atp_HelpSubSectionsType; /* used as an overlay */

typedef struct {
	Atp_HelpSubSectionsType *HelpSectionsPtr[ATP_NO_OF_HELP_TYPES+1];
} Atp_HelpSectionsType;

/*
 *	Structure for referencing help information.
 *	The ParmDef for the "help" command uses it exclusively
 *	for the parmDef DataPointer field.
 */
typedef struct {
	Atp_CmdRec				**cmdRecs;
	Atp_HelpSubSectionsType	*HelpAreaDesc;
	int						id;
} Atp_HelpInfoType;

EXTERN	void	Atp_DisplayHelpInfo	_PROTO_((Atp_HelpSubSectionsType *helpPtr, int indent));
EXTERN	void	Atp_DisplayCmdHelpInfo _PROTO_((Atp_CmdRec *CmdRecPtr,
												int text_type, int indent));
EXTERN	int		Atp_DisplayCommands	_PROTO_((void **voidptrs,
											 char *(*cmdnamefunc)
													 _PROTO_((void *)),
											 int (*comparator)
													 _PROTO_((const void *,
											 const void *)),
											 int count, int name_width));

EXTERN	int	Atp_DisplayCmdDescs	_PROTO_((void *cmdrecs,
										 int count, int name_width));

#define ATP_MANPG_INDENT	5

/* Help instruction indices for indexing *Atp_HelpInstructions[] */
extern char *Atp_HelpInstructions [];

/* Pointer to command record for "help" command. */
extern Atp_CmdRec *Atp_HelpCmdRecPtr;

/* Shared atphelp[c].c functions */
EXTERN int Atp_CompareNames _PROTO_(( char **spl, char **sp2 ));
EXTERN int Atp_CompareCmdRecNames _PROTO_((CommandRecord **reclp,
										   CommandRecord **rec2p ));
EXTERN void Atp_GetCmdTab1es
				_PROTO_((void				*clientData,
						 char **			*BuiltInCmdTabPtr,
						 int				*BuiltInCmdsCountPtr,
						 int				*BuiltInCmdNameWidthPtr,
						 char **			*UserProcsTabPtr,
						 int				*UserProcsCountPtr,
						 int				*UserProcsNameWidthPtr,
						 CommandRecord **	*AtpApplCmdTabPtr,
						 int				*AtpApplCmdsCountPtr,
						 int				*AtpApplCmdNameWidthPtr));

EXTERN char ** Atp_GetBuiltInCmdNames _PROTO_(( void *clientData,
												int *BuiltInCmdsCountPtr,
												int *BuiltInCmdNameWidthPtr));

EXTERN char ** Atp_GetUserDefinedProcNames _PROTO_((void *clientData,
													int *UserProcsCountPtr,
													int *UserProcsNameWidthPtr));

#define ATP_HELP_INSTR_HELPINFO		0
#define ATP_HELP_INSTR_HELPAREA		1
#define ATP_HELP_INSTR_CMD			2

/*
-----------------------------------------------------------------------
	INTERNAL parser-associated macros
-----------------------------------------------------------------------
*/
#define Atp_ParseRecParmDef(prec)	(prec->ParmDefPtr)

#define Atp_ParseRecParmDefEntry(prec) \
			((prec->ParmDefPtr)[prec->CurrPDidx])

#define Atp_ParseRecCurrPDidx(prec)	(prec->CurrPDidx)

#define Atp_ParseRecDefaultParmValue(prec) \
			((prec->ParmDefPtr)[prec->CurrPDidx].Default)

#define Atp_ParseRecDefaultParmPointer(prec) \
			((prec->ParmDefPtr)[prec->CurrPDidx].DataPointer)

#define Atp_OptParmOmitted(src) \
			((*src == ATP_OMITTED_OPTPARM) && (src[1] == '\0'))

/*
-----------------------------------------------------------------------
	INTERNAL global variables
-----------------------------------------------------------------------
*/
/* INTERACTIVE PROMPTING - NOT IMPLEMENTED / STUB ONLY */
extern	Atp_BoolType	Atp_InteractivePrompting;
EXTERN	Atp_Result		(*Atp_InputParmFunc)
								_PROTO_((Atp_PromptParmRec *, char **));
EXTERN	Atp_Result		(*Atp_OutputToUserFunc)_PROTO_((char *fmtstr,...));

/*
-----------------------------------------------------------------------
	INTERNAL global functions
-----------------------------------------------------------------------
*/
/* Parameter store manipulation functions. */
EXTERN void Atp_StoreParm _PROTO_((ParmDefEntry *ParmDefEntryPtr,
								   int isDefault, ...));
EXTERN void Atp_StoreConstructInfo _PROTO_((ParmDefEntry *ParmDefEntryPtr,
											int _parmcode,
											int isDefault, ...));
EXTERN void Atp_FreeParmStore _PROTO_((void *parmStore));
EXTERN void Atp_PushParmStorePtrOnStack _PROTO_((void *parmStore));
EXTERN void Atp_PopParmStorePtrFromStack _PROTO_( (void *parmStore));
EXTERN void *Atp_CurrParmStore _PROTO_((void));
EXTERN void *Atp_CreateNewParmStore _PROTO_((void));

/* INTERACTIVE PROMPTING - NOT IMPLEMENTED / STUB ONLY */
/* Interactive command prompting related functions. */
EXTERN	void	Atp_SetPromptFunc
					_PROTO_((Atp_Result (*func)(Atp_PromptParmRec *, char **)));
EXTERN Atp_PromptParmRec * Atp_CreatePromptParmRec
					_PROTO_((Atp_ParserStateRec *parseRec));
EXTERN	void	Atp_FreePromptParmRec _PROTO_((Atp_PromptParmRec *recPtr));

/* Parmdef checking functions. */
EXTERN char *	Atp_VerifyCmdRecParmDef _PROTO_(( Atp_CmdRec *Atp_CmdRecPtr ));
EXTERN int		Atp_AllParmsInParmDefAreOptionalOrNull
					_PROTO_((CommandRecord *CmdRecPtr));
EXTERN int		Atp_isConstructOfOptOrNullParms _PROTO_((ParmDefEntry * parmDef,
														 int index));

/* Check range function */
EXTERN Atp_Result	Atp_CheckRange _PROTO_((ParmDefEntry *PDE_ptr,
											int isUserValue, ...));

/* Vproc execution function */
EXTERN Atp_Result	Atp_InvokeVproc _PROTO_((ParmDefEntry *PDE_ptr,
											 void *valuePtr,
											 int isUserValue,
											 char **ErrorMsgPtr));
/* Error handling functions. */
EXTERN void	Atp_RetrieveParmError
					_PROTO_((int error_code,
							 char *parm_retrieval_func_name,
							 char *parm_name, char *filename,
							 int line_number));

/* Newline counter given string to see if output exceeds screen size (lines). */
EXTERN int Atp_PageLongerThanScreen _PROTO_((char *output_string));

/* Hidden commands may be declared and queried. */
extern char	**Atp_HiddenCommands;
EXTERN int	Atp_IsHiddenCommandName _PROTO_((char *name));

/* "help -lang" parmdef description strings (atphelpc.c) */
extern char Atp_HelpLangCaseDesc[120];
extern char Atp_HelpLangOptStrDesc[80];

/* Pointer to "help" command parameter table. */
extern Atp_ParmDefEntry *Atp_HelpCmdParmsPtr;

/* On-line help information functions. */
EXTERN Atp_HelpSubSectionsType *
				Atp_AppendHelpInfo _PROTO_((Atp_HelpSubSectionsType *helpInfo,
											char **text));

EXTERN Atp_HelpSubSectionsType *
				Atp_GetHelpSubSection _PROTO_((Atp_CmdRec *CmdRecPtr,
											   int text_type));
EXTERN Atp_Result (*Atp_GetFrontEndManpage) _PROTO_((void *, ...));
EXTERN int	Atp_DisplayManPage _PROTO_((void *, ...));
EXTERN void	Atp_DisplayManPageSynopsis _PROTO_((CommandRecord *CmdRecPtr));
EXTERN void	Atp_PrintListInNotationFormat _PROTO_((ParmDefEntry * parmdef,
												   Atp_PDindexType start_index,
												   int indent));
EXTERN void Atp_ResetManPgColumn _PROTO_((void));
extern int	Atp_ManPgLineWrap_Flag; /* defaults to on (1) */

/*
-----------------------------------------------------------------------
	INTERNAL constants
-----------------------------------------------------------------------
*/
#define ATP_MAX_NESTCMD_DEPTH			50
#define ATP_MAX_PARSE_NESTING_DEPTH		100
				/* Nesting limit for nested constructs in Atp_ParmDef */

/*
	ERROR HANDLING and ERROR MESSAGING
	Some of these fields are not yet used but are stubs for possible use
	when interactive prompting is implemented.
*/
/* Central store for error message format strings. */
typedef struct {
	/* Error code for identification / indexing. */
	int		error_code;

	/* Indicate who was at fault, user or system. */
	char	*whos_fault;	/* i.e. user error or system error */

	/* Where did error occur? */
	char	*error_location;

	/* What went wrong? Describe error. */
	char	*errmsg_fmtstr; /* format string for printf() */

	/* User attention: STOP, CAUTION or NOTE, indicates severity. */
	char	*user_attention;

	/*
		Indicate current system status as a result of error.
		What is the system's next action or is user in control.
	*/
	char	*system_status;

	/*
		Suggestion on how to recover from error
		or how to find out how to recover from it?
	*/
	char	*recovery_suggestion;

	/* State user action required in order to proceed any further. */
	char	*user_action;

} Atp_ErrorDescRecord;

/* Table of error description records for internal use. */
typedef Atp_ErrorDescRecord Atp_ErrorDescTable[];

/* Atp_MakeErrorMsg: ERRLOC must be first argument */
EXTERN char * Atp_MakeErrorMsg _PROTO_((char *file_name, int line_number,
										int error_code, ...));

EXTERN void Atp_ShowErrorLocation _PROTO_((void));

extern Atp_BoolType Atp_PrintErrLocnFlag;

/* Use this to append parm name and desc to end of error message. */
#define Atp_AppendParmName(parseRec,errmsg) \
		Atp_INTERNAL_AppendParmName(parseRec,&errmsg)

EXTERN void Atp_INTERNAL_AppendParmName
					_PROTO_((Atp_ParserStateRec *parseRec,
							char **errmsg_ptr));

/*
-----------------------------------------------------------------------
	Long jumping facilities for error handling
-----------------------------------------------------------------------
*/
extern jmp_buf *	Atp_JmpBufEnvPtr;

/*
-----------------------------------------------------------------------
	Error codes and messages
-----------------------------------------------------------------------
*/

/* Error types */
#ifndef _ATP_USE_XSTR
		extern	char					Atp_User_Error_Str[];
		extern	char					Atp_System_Error_Str[];
#		define	ATP_ERRMSG_USER_ERROR	((char *)Atp_User_Error_Str)
#		define	ATP_ERRMSG_SYS_ERROR	((char *)Atp_System_Error_Str)
#else
#		define	ATP_ERRMSG_USER_ERROR	"USER ERROR"
#		define	ATP_ERRMSG_SYS_ERROR	"SYSTEM ERROR"
#endif

/* Error indicating bad user input (see atperror.c) */
#define ATP_ERRMSG_BAD_INPUT			"BAD INPUT"

/* Errors to do with executed command (see atperror.c) */
#define ATP_ERRMSG_CMD_FAILURE			"COMMAND FAILED"
#define ATP_ERRMSG_CMD_ABORTED			"COMMAND ABORTED"
#define ATP_ERRMSG_CMD_BAD_RC			"BAD RETURN CODE FROM COMMAND"

/* Hyperspace error code for use with longjmp. */
#define ATP_HYPERSPACE_CODE	999

/* Error codes for indicating type of error */
#define ATP_ERRCODE_USER_ERROR		-1
#define ATP_ERRCODE_SYS_ERROR		-2

#define ATP_ERRCODE_BAD_INPUT		-3

#define ATP_ERRCODE_COMMAND_FAILED	-4
#define ATP_ERRCODE_CMD_ABORTED		-5
#define ATP_ERRCODE_CMD_BAD_RC		-6

/*
	Internal error codes for indicating errors
*/
/* User Errors such as sensible input but not accepted */
#define ATP_ERRCODE_DATABYTES_LENGTH_OUT_OF_RANGE		-100
#define ATP_ERRCODE_BCD_DIGITS_LENGTH_OUT_OF_RANGE		-101
#define ATP_ERRCODE_EXPECTED_PARM_NOT_FOUND				-102
#define ATP_ERRCODE_NO_PARMS_REQUIRED					-103
#define ATP_ERRCODE_EXTRA_PARMS_NOT_WANTED				-104
#define ATP_ERRCODE_EXPECTED_PARMS_NOT_FOUND			-105
#define ATP_ERRCODE_NUM_OUT_OF_RANGE					-106
#define ATP_ERRCODE_PARSE_RPT_MARKER_ERROR				-107
#define ATP_ERRCODE_REAL_NUM_OUT_OF_RANGE				-108
#define ATP_ERRCODE_RPTBLK_INSTANCE_OUT_OF_RANGE		-109
#define ATP_ERRCODE_SIGNED_NUM_OUT_OF_RANGE				-110
#define ATP_ERRCODE_STRING_OUT_OF_RANGE					-111
#define ATP_ERRCODE_UNSIGNED_NUM_OUT_OF_RANGE			-112
#define ATP_ERRCODE_KEYWORD_NOT_RECOGNISED				-113

/* System Errors */
#define ATP_ERRCODE_CANNOT_RETRIEVE_PARM				-200
#define ATP_ERRCODE_DEFAULT_VALUE_OUT_OF_RANGE			-201
#define ATP_ERRCODE_ILLEGAL_FREE_NULL_CTRLSTORE			-202
#define ATP_ERRCODE_KEYWD_TAB_ABSENT					-203
#define ATP_ERRCODE_NEGATIVE_PARMDEF_INDEX				-204
#define ATP_ERRCODE_NON_CONSTRUCT_PARMCODE				-205
#define ATP_ERRCODE_NO_PARMDEF_OUTER_BEGIN_END			-206
#define ATP_ERRCODE_PARMDEF_CONSTRUCT_MATCH_ERROR		-207
#define ATP_ERRCODE_RETURN_VALUE_POINTER_ABSENT			-208
#define ATP_ERRCODE_WRONG_PARMCODE_FOR_PARSER			-209
#define ATP_ERRCODE_MALLOC_RETURNS_NULL					-210
#define ATP_ERRCODE_PARSER_MISSING						-211
#define ATP_ERRCODE_DEFAULT_KEYWORD_NOT_FOUND			-212
#define ATP_ERRCODE_NESTCMD_DEPTH_EXCEEDED				-213

/* User Errors due to parsing e.g. syntax error, invalid input */
#define ATP_ERRCODE_EMPTY_DATABYTES_STRING				-300
#define ATP_ERRCODE_EMPTY_BCD_DIGITS					-301
#define ATP_ERRCODE_INVALID_HEX_DATABYTES				-302
#define ATP_ERRCODE_INVALID_BCD_DIGITS					-303
#define ATP_ERRCODE_INVALID_NUMBER						-304
#define ATP_ERRCODE_MINUS_SIGN_FOR_UNSNUM				-305
#define ATP_ERRCODE_MINUS_SIGN_NOT_ALLOWED_FOR_NUMBASE	-306
#define ATP_ERRCODE_NUMSIGNS_CONFLICT					-307
#define ATP_ERRCODE_NUM_OVERFLOW						-308
#define ATP_ERRCODE_NUM_UNDERFLOW						-309
#define ATP_ERRCODE_UNRECOGNISED_PARM_VALUE				-310
#define ATP_ERRCODE_INVALID_RPTBLK_MARKER				-311
#define ATP_ERRCODE_RPTBLK_MARKER_MISSING				-312
#define ATP_ERRCODE_RPTBLK_MARKER_OR_NXT_PARM_MISSING	-313

/*
----------------------------------------------------------------------------
	Internal variable externs (note that some variables may be referenced
	by local externs in modules and not made public here)
----------------------------------------------------------------------------
*/
extern Atp_UnsNumType Atp_CurrentKeywordIndex;
extern	char		*Atp_HelpInfoBuffer;

/* Internal externed functions */
EXTERN	Atp_Result	Atp_GetKeywordIndex
							_PROTO_((Atp_KeywordType *KeyTable,
									 Atp_NumType KeyValue,
									 char **KeywordStringPtr,
									 Atp_UnsNumType *KeyIndexPtr,
									 char **ErrorMsgPtr));
EXTERN	char *		Atp_GetOptChoiceDefaultCaseName
									_PROTO_((ParmDefEntry * ParmDefPtr,
											 int ParmDefIndex,
											 Atp_ChoiceDescriptor **DefaultPtr));

/*
----------------------------------------------------------------------------
	Internal function externs (note that some functions may be referenced
	by local externs in modules and not made public here)
----------------------------------------------------------------------------
*/
EXTERN	ParmStoreInfo	*Atp_SearchParm _PROTO_((ParmStoreInfo *CtrlStorePtr, ...));
EXTERN	int				Atp_ConstructBracketMatcher
								_PROTO_((ParmDefEntry * ParmDefPtr,
										 Atp_PDindexType InputPDidx,
										 int NoOfPDEs));
EXTERN	Atp_KeywordType * Atp_MakeChoiceKeyTab
								_PROTO_((ParmDefEntry * PDptr,
										 int PDidx,
										 int EndChoiceIdx));
EXTERN	void			Atp_HyperSpace _PROTO_((char *error_msg));
EXTERN	char *			Atp_GetHyperSpaceMsg _PROTO_((void));
EXTERN	Atp_BoolType	Atp_IsEmptyParmDef _PROTO_((CommandRecord *CmdRecPtr));
EXTERN	void			Atp_DisplayIndent _PROTO_((int indent));

#endif /* _ATPINT */
