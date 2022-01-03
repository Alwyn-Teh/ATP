/* EDITION AC16 (REL002), ITD ACST.178 (95/06/21 14:41:30) -- OPEN */

/*+*****************************************************************************

	Module Name:			atph.h

	Copyright:				BNR Europe Limited, 1992-1995
							Bell-Northern Research / BNR
							Northern Telecom / Nortel

	Description:			This header	file contains the externally
							visible facilities of the Advanced Token
							Parser (ATP).

	Notes:					ATP is a library toolkit for enhancing the
							development of command line user interfaces.
							It enables applications using it to provide
							a uniform look-and-feel. Facilities for
							specifying commands, verifying parameters
							and retrieving them are provided. An
							adaptor for use with UCB Tcl is available.
							See associated documentation and example
							program for further information.

	Modifications:
		Who					When				Description
	----------------  ---------------	--------------------------------
	Alwyn Teh			8 July	1992	Initial Creation
	Alwyn Teh			20	May	1993	Atp_DataDescriptor to be made
										available for use by databytes.
										Internally, databytes now gets
										stored as Atp_DataDescriptor
										and NOT (Atp_ByteType *) so that
										repeat blocks of databytes
										can be accessed with ease and
										consistency.
	Alwyn Teh			20	May	1993	Atp_DataBytesCount(databytes_ptr)
	Alwyn Teh			20	May	1993	Introduce Atp_DataBytesDesc and
										AtpRptBlockDesc, both returning
										Atp_DataDescriptor.
	Alwyn Teh			21	May	1993	Atp_DataDescriptor.count and
										Atp_ChoiceDescriptor.Caselndex
										change type to Atp_NumType from
										Atp_UnsNumType. This is because
										if, say, a NULL default is supplied
										for an optional repeat block, a
										count of -1 is used to indicate
										this (see atprpblk.c).
	Alwyn Teh			7 June 1993		Add CASE construct
	Alwyn Teh			10 June 1993	Add	Atp_AddManHeaderInfo().
	Alwyn Teh			23 June	1993	Add	Atp_OutputPager() and
										variable Atp_PagingNeeded.
	Alwyn Teh			27 June	1993	Add	Atp_StrToLower().
	Alwyn Teh			29 June	1993	Changed Atp_AddManHeaderInfo() to
										Atp_AddHelpInfo().
	Alwyn Teh			13 July	1993	Append CaseName to Atp_ChoiceDescriptor
										(MUST be last field to be backwards
										compatible)
	Alwyn Teh			18 July	1993	Use enum parmcode in debug mode
										for display of symbolic names
	Alwyn Teh			21 July	1993	Use ATP_ERRLOC only in debug mode
										so that in production code, static
										literal filenames (__FILE__) do not
										take up space in the compiled objects.
	Alwyn Teh			29 Sept	1993	Add 3rd argument case number value
										to CASE macros. Retrieve using
										Atp_Num(choicename), so add casevalue
										field to end of ChoiceDescriptor.
	Alwyn Teh			30 Sept	1993	Reorder fields in Atp_ChoiceDescriptor,
										also include case description.
	Alwyn Teh			7 December 1993	Export Atp_CreateHelpArea, Atp_HelpCmd
	Alwyn Teh			8 December 1993	Release ATP subsystem AA09
	Alwyn Teh			4 January 1994	Port to HP9Q00/735 running HP-UX 9.01
	Alwyn Teh			13 January 1994	Add Atp_Copyright
	Alwyn Teh			13 January 1994 Do not release unimplemented features
										such as real_def and common_parm_def
										macros.
	Alwyn Teh			17 January 1994 Find memory gobbler bug -
										use cc -DATP_MEM_DEBUG
	Alwyn Teh			25 January 1994 Comment out ParmDefPtr in struct
										Atp_ParmDefEntry since not used yet.
										This saves space in parmdefs.
										Mirror change in atpsysex.h too.
	Alwyn Teh			31 January 1994	Release subsystem ATP.AB01
	Alwyn Teh			18 July	1994	Release subsystem ATP.AC01
	Alwyn Teh			20 July 1994	Unify HELP system to provide single
										point of entry via "help" command.
	Alwyn Teh			27 July 1994	Release subsystem ATP.AC02
	Alwyn Teh			12 August 1994	Release subsystem ATP.AC04
	Alwyn Teh			15 August 1994	Release subsystem ATP.AC05
	Alwyn Teh			17-21 Oct 1994	Incorporate ANSI C and C++ compile
										compatibility for ATP/Tcl applications.
	Alwyn Teh			7 March 1995	ATP_NULL_PTR made acceptable
										to C++ compiler.
	Alwyn Teh			8 March 1995	Changed Atp_NumType (and Atp_UnsNumType)
										from long to int.
	Alwyn Teh			14 March 1995	Remove Atp_FreeParmStore declaration
										since declared in atpsysex.h
	Alwyn Teh			24 March 1995	Release ATP.AC06 subsystem (Rel 2.5)
	Alwyn Teh			24 March 1995	Implement BCD digits parameter type
	Alwyn Teh			12 April 1995	Purify ATP and implement deletion
										routines to remove reported memory
										leak problems:
											Atp_CleanupProc()
											Atp_DeleteCommand()
											Atp_DeleteCommandRecord()
											Atp_DeleteCmdGrp{)
	Alwyn Teh			4 May 1995		Handle window resize.
										Create ATP.AC08 subsystem (Rel 2.7)
	Alwyn Teh			4 May 1995		"help -lang" to distinguish built-in
										commands from user-defined procs.
	Alwyn Teh			11 May 1995		Get ATP to be in sync with SLP.AC07;
										release as ATP.AC09.
	Alwyn Teh			12 May 1995		Fix bug in atpfindp.c AC03 line 107
										Release ATP.AC10
	Alwyn Teh			29 May 1995		Fix bug in atphelpc.c AC09 line 402
	Alwyn Teh			20 June 1995	Rename Atp_VarargStrlen() and export
										as Atp_FormatStrlen() here for use
										by GENIE III.
	Alwyn Teh			20 June 1995	New subsystem ATP.AC12
	Alwyn Teh			21 June 1995	Export Atp_Used_By_G30 and
										Atp_EnumerateProtocolFieldValues().

*****************************************************************************-*/
#ifndef _ATP
#define _ATP

#undef _PROTO_
#undef _VARARGS_
#undef EXTERN

#if defined (__STDC__) || defined (__cplusplus)
#  include <stdarg.h>
#else
#  include <varargs.h>
#endif

#if defined(__STDC__) || defined(__cplusplus)
#  define _PROTO_(s) s
#  ifdef __STDC__
#    define _VARARGS_ , ...
#  else /* #elif not supported by K&R cpp */
#    ifdef cplusplus
#      define _VARARGS_ ...
#    endif
#  endif
#else
#  define _PROTO_(s) ()
#  define _VARARGS_
#endif

#ifdef __cplusplus
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif

#define ATP_VERSION "AC12"

EXTERN char *Atp_Copyright[];

/*
--------------------------------------------------------------------
 Definitions of ATP parameters.

 These promote portability of applications by avoiding changes
 required of datatypes due to variations of machine-dependent
 representations.
--------------------------------------------------------------------
 */

/* Atp_NumType and Atp_UnsNumType MUST always be of the same size. */

typedef int				Atp_NumType;	/* whole number type	*/
typedef unsigned int	Atp_UnsNumType;	/* unsigned number type	*/

typedef double			Atp_RealType;	/* real number type		*/

typedef unsigned char	Atp_ByteType;	/* byte type			*/

typedef unsigned char	Atp_BoolType;	/* boolean type			*/

/*
 Strings, NULL-terminated characters, are represented by (char *)
 and do not need to be redefined.
 */

/*
 Keywords are an enumerated type. The term "enum" is avoided for
 user-friendly reasons as it is a computer jargon word.
 */

typedef struct Atp_KeywordType {
	char		*keyword;
	Atp_NumType	KeyValue;
	char		*KeywordDescription; /*	new, optional */
	Atp_NumType	internal_use;
} Atp_KeywordType;

/*
 Atp_KeywordTab is defined ONLY for the use by applications.

 DO NOT use Atp_KeywordTab within ATP itself. This is because of
 warnings caused by strict compilers and CodeCenter due to
 unspecified array dimensions. Use (Atp_KeywordType *) instead.
 */
typedef Atp_KeywordType Atp_KeywordTab[];

/*
---------------------------------------------------------------------
 INTERNAL indicators for ATP control constructs and parameter types.
---------------------------------------------------------------------
 */

/*
 IMPORTANT NOTE:

 Atp_ParmCode is defined as an 8-bit code containing bit
 fields not visible to the external world. The choice of
 using unsigned char rather than a struct containing bit
 fields is for ensuring portability as whether fields are
 assigned left to right or right to left is
 machine-dependent. Internal access of fields are via access
 macros and functions. If more fields are required in
 future, Atp_ParmCode can be increased in size to a short or
 int.

 The following internal parameter codes have been generated
 for internal identification and classification purposes.
 Further details are available in atpsysex.h internal header
 file.

 PLEASE DO NOT CHANGE THEM!
 */

#ifndef ATP_OPTPARM_MASK /* this is used in atpsysex.h too */
#define ATP_OPTPARM_MASK	0200	/* 10000000 */
#endif
#ifndef Atp_SetOptParm
#define Atp_SetOptParm(parmcode)	((parmcode) | ATP_OPTPARM_MASK)
#endif

#ifndef _ATP_INTERNAL_DEBUG

/*
 * Normal compile uses small Atp_ParmCode type to conserve space.
 */

typedef unsigned char Atp_ParmCode;

/*	ParmCodeName	 ParmCode				Purpose			*/
/* ----------------- ------------	------------------------*/
#define ATP_BPM		 '\001'		/*	BEGIN_PARMS	 */
#define ATP_EPM		 '\041'		/*	END_PARMS	 */
#define ATP_BLS		 '\005'		/*	BEGIN_LIST	 */
#define ATP_ELS		 '\045'		/*	END_LIST	 */
#define ATP_BRP		 '\011'		/*	BEGIN_REPEAT */
#define ATP_ERP		 '\051'		/*	END_REPEAT	 */
#define ATP_BCH		 '\015'		/*	BEGIN_CHOICE */
#define ATP_ECH		 '\055'		/*	END_CHOICE	 */
#define ATP_BCS		 '\021'		/*	BEGIN_CASE	 */
#define ATP_ECS		 '\061'		/*	END_CASE	 */
#define ATP_NULL	 '\000'		/*	NULL parameter			*/
#define ATP_NUM		 '\002'		/*	Signed int number		*/
#define ATP_UNS_NUM	 '\042'		/*	Unsigned int number		*/
#define ATP_DATA	 '\006'		/*	Octet databyte string	*/
#define ATP_STR		 '\046'		/*	Octet ASCII string		*/
#define ATP_BCD		 '\106'		/*	BCD digits string		*/
#define ATP_KEYS	 '\012'		/*	Keyword					*/
#define ATP_BOOL	 '\052'		/*	Boolean					*/
#define ATP_REAL	 '\016'		/*	Real number				*/
#define ATP_COM		 '\003'		/*	Common parmdef			*/
#define ATP_EOP		 '\004'		/* End Of Parameter Store Marker */

#else

typedef enum {
		ATP_BPM		=	'\001',
		ATP_EPM		=	'\041',
		ATP_BLS		=	'\005',		ATP_OPT_BLS		= Atp_SetOptParm('\005'),
		ATP_ELS		=	'\045',		ATP_OPT_ELS		= Atp_SetOptParm('\045'),
		ATP_BRP		=	'\011',		ATP_OPT_BRP		= Atp_SetOptParm{'\011'),
		ATP_ERP		=	'\051',		ATP_OPT_ERP		= Atp_SetOptParm{'\051'),
		ATP_BCH		=	'\015',		ATP_OPT_BCH		= Atp_SetOptParm('\015'),
		ATP_ECH		=	'\055',		ATP_OPT_ECH		= Atp_SetOptParm('\055'),
		ATP_BCS		=	'\021',
		ATP_ECS		=	'\061',
		ATP_NULL	=	'\OOO',
		ATP_NUM		=	'\002',		ATP_OPT_NUM		= Atp_SetOptParm{'\002'),
		ATP_UNS_NUM =	'\042',		ATP_OPT_UNS_NUM = Atp_SetOptParm('\042'),
		ATP_DATA	=	'\006',		ATP_OPT_DATA	= Atp_SetOptParm('\006'),
		ATP_BCD		=	'\106',		ATP_OPT_BCD		= Atp_SetOptParm('\106'),
		ATP_STR		=	'\046',		ATP_OPT_STR		= Atp_SetOptParm('\046'),
		ATPJCEYS	=	'\012',		ATP_OPT_KEYS	= Atp_SetOptParm('\012'),
		ATP_BOOL	=	'\052',		ATP_OPT_BOOL	= Atp_SetOptParm('\052'),
		ATP_REAL	=	'\016',		ATP_OPT_REAL	= Atp_SetOptParm('\016'),
		ATP_COM		=	'\003',		ATP_OPT_COM		= Atp_SetOptParm('\003'),
		ATP_EOP		=	'\004'
} Atp_ParmCode;

#endif

/*
-------------------------------------------------------------------
 Definition of ATP parameter definition table (parmdef).

 An ATP command requiring parameter prototyping will capture this
 information in a parameter definition table using parmdef macros
 (see later). A parmdef macro expands to an Atp_ParmDefEntry.

 Atp_ParmDef used to be typedefed as struct Atp_ParmDefEntry
 Atp_ParmDef[]. However, for ATP to be portable, it has been
 changed to a struct pointer to be acceptable by CodeCenter
 (formerly Saber-C) and the GNU CC compiler. The complaint
 received was unspecified array dimensions.
-------------------------------------------------------------------
 */

typedef struct Atp_ParmDefEntry *Atp_ParmDef;

/*
-------------------------------------------------------------------
  This is the structure for a parmdef entry. It is a parallel
  overlay structure of an internal ParmDefEntry structure (defined
  in atpsysex.h) . If changes are made to either of these
  structures, the other must be updated also.

  The internal-use and filler fields are used to hide internal or
  system data structures from view by external applications. If
  additional internal fields are introduced, the size of internal
  fields must increase accordingly.

  The sequencing of the fields is such that the most often used
  fields are at the beginning, and the least often used ones are at
  the end. This is so that if fewer initializers are used than
  there are members, the trailing members are initialized with 0
  automatically by the compiler.

  Ensure that whereever possible, bit fields are used to conserve
  memory. Here, double is used for Min, Max and default because the
  largest type used is Atp_RealType. Smaller types can therefore
  fit in by explicit casting during static initialization.
-------------------------------------------------------------------
*/

typedef int		Atp_Result;
typedef double	Atp_LargestType;

struct	Atp_ParserStateRec; /* internal */
typedef	struct Atp_ParserStateRec Atp_ParserStateRec;

typedef Atp_Result	(*Atp_ParserType)
						_PROTO_((Atp_ParserStateRec *_private_));

typedef Atp_Result	(*Atp_ArgParserType)_PROTO_((char *Argv_N, ...));

typedef char *		(*Atp_VprocType)_PROTO_((void * valPtr,
											 Atp_BoolType isUserValue));

typedef struct Atp_ParmDefEntry
{
		Atp_ParmCode	parmcode;		/* Parm ID/classification code	*/
		Atp_ParserType	parser;			/* Parameter parser to use		*/
		char			*Name;			/* Parameter name				*/
		char			*Desc;			/* Description of parameter		*/
		Atp_LargestType	Min;			/* Minimum value of parameter	*/
		Atp_LargestType	Max;			/* Maximum value of parameter	*/
		Atp_VprocType	vproc;			/* Verification procedure		*/
		Atp_LargestType	Default;		/* Default value of parameter	*/
		Atp_KeywordType *KeyTabPtr;		/* Pointer to table of keywords	*/
		void			*DataPointer;	/* e.g. default pointer			*/
		/* Atp_ParmDef	ParmDefPtr;*/	/* Pointer to a common parmdef	*/
		unsigned char	filler[2];		/* Filler for internal use		*/

} Atp_ParmDefEntry;

/*
--------------------------------------------------------------------------
	 Definitions which relate to the formation of ATP commands:

	 Atp_CmdRec contains the essential attributes of an ATP command.

	 Atp_AssembleCmdRecord() gathers the information required by the
	 Atp_CmdRec, puts them in a dynamic structure and returns its
	 address.

	 Atp_NoOfPDentries() returns the number of entries in the ATP
	 command's Parameter Definition table (ParmDef). It attempts to
	 evaluate this first by using sizeof() at compile time. This works
	 only if the name of the parmdef is supplied; otherwise, zero is
	 evaluated in which case Atp_EvalNoOfParmDefEntries() will attempt
	 to count the number of entries.
--------------------------------------------------------------------------
*/

/*
	 Atp_CmdCallBackType is defined for ANSI-C compile compatibility
	 only, the 1st argument given here is arbitrary and is required in
	 order to use "...". In C++, there are less restrictions and
	 "..." may be used in a function prototype on its own.
*/

#ifndef _ATP_EXT_CMD_CALLBACK_INTERFACE_

/* If you change ArgvO's type, change ATP_FRAME_ELEM_TYPE in atpframh.h too. */
/* ArgvO is a void * to be compatible with the 1st argument in the callback. */
/* At least 1 fixed function argument is required for ANSI-C stdarg to work. */

#define _ATP_EXT_CMD_CALLBACK_INTERFACE_ void *ArgvO _VARARGS_

#endif

typedef Atp_Result (*Atp_CmdCallBackType)
								_PROTO_((_ATP_EXT_CMD_CALLBACK_INTERFACE_));

/*
	This is a parallel overlay structure to an internal CommandRecord
	structure in atpsysex.h which contains more trailing internal
	fields; if changes are made to these external fields, update the
	internal structure also. Atp_CmdRec is made available here for use
	by adaptor module(s) such as atp2tcl.c. It is not required by user
	applications.
*/

typedef struct _Atp_CmdRec_ {
		char	*cmdName;		/* Command name					*/
		char	*cmdDesc;		/* Brief command description	*/
		void	*helpInfo;		/* On-line HELP information		*/
		Atp_CmdCallBackType		/* Pointer to callBack function	*/
				callBack;		/* to be invoked for command.	*/
		Atp_ParmDef parmDef;	/* Pointer to parameter table	*/
		void	*clientData;	/* Original client data			*/
} Atp_CmdRec;

EXTERN	Atp_CmdRec *
		Atp_AssembleCmdRecord
				_PROTO_((char *name,
						 char *desc,
						 Atp_CmdCallBackType cb,
						 Atp_ParmDef pd,
						 int ne,
						 void *cd));

EXTERN int			isAtpCommand
							_PROTO_((Atp_CmdRec *Atp_CmdRec_Ptr));

EXTERN Atp_Result	Atp_RegisterCommand _PROTO_((void *CmdGrpId,
												 Atp_CmdRec *CmdRecPtr));

EXTERN void			Atp_CleanupProc();
EXTERN Atp_Result	Atp_DeleteCommand _PROTO_((char *cmdName));
EXTERN Atp_Result	Atp_DeleteCommandRecord _PROTO_ ((Atp_CmdRec *cmdRec));
EXTERN Atp_Result	Atp_DeleteCmdGrp _PROTO_((void *id));
											/* e.g. id = Tcl_Interp* */
/*
 *	At present, only one "help" command is supported per single application.
 *	Therefore, a Tcl application with only one interpreter should be used.
 *
 *	This deleteProc tidies up the one and only "help" command.
 *
 *	DO NOT attempt to use multiple Tcl interpreters in an application;
 *	such as in an object-oriented C++ program, where class constructors
 *	and destructors create and delete interpreters and commands.
 *
 *	Wed Apr 12 18:50:56 BST 1995 (ACST.158)
 */

EXTERN void Atp_HelpCmdDeleteProc();

#define Atp_NoOfPDentries(pd)	Atp_EvalNoOfParmDefEntries(pd, sizeof(pd))

/*
-----------------------------------------------------------------
 Overlay structures for the Repeat Block, Data Bytes and Choice
 parameters returned by ATP.
-----------------------------------------------------------------
 */

/*
  Atp_DataDescriptor is used for databytes and repeat blocks, where
  count is the number of bytes. When used for BCD digits, then count
  is the number of nibbles per BCD digit, although data points to
  storage of an integral number of bytes. Any trailing unused BCD
  nibble is set to zero.
*/
typedef struct _Atp_DataDescriptor_
{
	Atp_NumType	count;	/* number of instances of data block		*/
	void		*data;	/* pointer to 1st parameter of data block	*/

} Atp_DataDescriptor;

typedef struct _Atp_ChoiceDescriptor_
{
	Atp_NumType	CaseValue;	/* enumerated value for case entry			*/
	void		*data;		/* pointer to beginning of case parameter	*/

	/* Following fields are for internal use only. */
	char		*CaseName;	/* name of case keyword used for choice		*/
	char		*CaseDesc;	/* description of case used for choice		*/
	Atp_NumType	CaseIndex;	/* keyword list index starting at zero		*/

} Atp_ChoiceDescriptor;

/*
-----------------------------------------------------------------
  DataBytes and BCD digits count field extraction macros.
-----------------------------------------------------------------
*/

#define Atp_DataBytesCount(ptr) (((Atp_UnsNumType *)(ptr))[-1])
#define Atp_BcdDigitsCount(ptr) Atp_DataBytesCount(ptr)

/*
-----------------------------------------------------------------
 ATP parser syntax definitions
-----------------------------------------------------------------
*/
#define ATP_OPEN_REPEAT_BLOCK	'('
#define ATP_CLOSE_REPEAT_BLOCK	')'
#define ATP_OMITTED_OPTPARM		'.'

/*
-----------------------------------------------------------------
 ATP Conversion ParraDef Macros
-----------------------------------------------------------------
*/

#define BEGIN_PARMS				ATP_BEGIN_PARMS
#define END_PARMS				ATP_END_PARMS
#define EMPTY_PARMDEF			ATP_EMPTY_PARMDEF

#define BEGIN_LIST				ATP_BEGIN_LIST
#define END_LIST				ATP_END_LIST

#define BEGIN_OPT_LIST			ATP_BEGIN_OPT_LIST
#define END_OPT_LIST			ATP_END_OPT_LIST

#define BEGIN_REPEAT			ATP_BEGIN_REPEAT
#define END_REPEAT				ATP_END_REPEAT

#define BEGIN_OPT_REPEAT		ATP_BEGIN_OPT_REPEAT
#define END_OPT_REPEAT			ATP_END_OPT_REPEAT

#define BEGIN_CHOICE			ATP_BEGIN_CHOICE
#define END_CHOICE				ATP_END_CHOICE

#define BEGIN_OPT_CHOICE		ATP_BEGIN_OPT_CHOICE
#define END_OPT_CHOICE			ATP_END_OPT_CHOICE

#define BEGIN_CASE				ATP_BEGIN_CASE
#define END_CASE 				ATP_END_CASE

#define CASE 					ATP_CASE

#define num_def					atp_num_def
#define unsigned_num_def		atp_unsigned_num_def
#define opt_num_def				atp_opt_num_def
#define opt_unsigned_num_def	atp_opt_unsigned_num_def

#ifdef ATP_REALNUM_IMPLEMENTED
#define real_def				atp_real_def
#define opt_real_def			atp_opt_real_def
#endif

#define str_def					atp_str_def
#define opt_str_def				atp_opt_str_def
#define	bool_def				atp_bool_def
#define	opt_bool_def			atp_opt_bool_def
#define	keyword_def				atp_keyword_def
#define	opt_keyword_def			atp_opt_keyword_def
#define	data_bytes_def			atp_data_bytes_def
#define	opt_data_bytes_def		atp_opt_data_bytes_def
#define	bcd_digits_def			atp_bcd_digits_def
#define	opt_bcd_digits_def		atp_opt_bcd_digits_def
#ifdef ATP_COMMON_PARMDEF_IMPLEMENTED
#define	common_parm_def			atp_common_parm_def
#define	opt_common_parm_def		atp_opt_coramon_parm_def
#endif
#define null_def				atp_null_def

/*
-----------------------------------------------------------------
 ATP ParmDef Declaration Macro
-----------------------------------------------------------------
 */

#define ATP_DCL_PARMDEF(ParmDefName)	\
		Atp_ParmDefEntry ParmDefName[] = {

#define ATP_END_DCL_PARMDEF				};

/*
-----------------------------------------------------------------
 ATP Constructs ParmDef Macros
-----------------------------------------------------------------
 */
#if defined (__STDC__) || defined (__cplusplus)
#	define	ATP_NULL_PTR	0
#else
#	define	ATP_NULL_PTR	((void *) 0)
#endif

#define ATP_BEGIN_PARMS		{ATP_BPM},
#define ATP_END_PARMS		{ATP_EPM}

#define ATP_EMPTY_PARMDEF	ATP_BEGIN_PARMS	ATP_END_PARMS

#define ATP_BEGIN_LIST(name, desc)	\
		{ATP_BLS, (Atp_ParserType)Atp_ProcessListConstruct, name, desc},

#define ATP_END_LIST		{ATP_ELS},

#define ATP_BEGIN_OPT_LIST(name, desc, default) \
		{Atp_SetOptParm(ATP_BLS), (Atp_ParserType)Atp_ProcessListConstruct, \
		 name, desc, 0, 0, 0, \
		 0, ATP_NULL_PTR, ((void *)(default))},

#define ATP_END_OPT_LIST	{Atp_SetOptParm(ATP_ELS)},

#define ATP_BEGIN_REPEAT(name,desc,min,max,vproc)	\
		{ATP_BRP, (Atp_ParserType)Atp_ProcessRepeatBlockConstruct,	\
		 name, desc, min, max, (Atp_VprocType)vproc},

#define ATP_END_REPEAT {ATP_ERP,(Atp_ParserType)Atp_ParseRptBlkMarker},

#define ATP_BEGIN_OPT_REPEAT(name,desc,default,min,max,vproc)	\
		{Atp_SetOptParm(ATP_BRP), \
		 (Atp_ParserType)Atp_ProcessRepeatBlockConstruct, \
		 name, desc, min, max, (Atp_VprocType)vproc, \
		 0, ATP_NULL_PTR, ((void *)(default))},
		/* Pointer to default value is of type (Atp_DataDescriptor *). */

#define ATP_END_OPT_REPEAT	\
		{Atp_SetOptParm(ATP_ERP), (Atp_ParserType)Atp_ParseRptBlkMarker},

#define ATP_BEGIN_CHOICE(name,desc,vproc)	\
		{ATP_BCH, (Atp_ParserType)Atp_ProcessChoiceConstruct, \
		 name, desc, 0, 0, (Atp_VprocType)vproc},

#define ATP_END_CHOICE		{ATP_ECH},

#define ATP_BEGIN_OPT_CHOICE(name,desc,default,vproc) \
		{Atp_SetOptParm(ATP_BCH), \
		 (Atp_ParserType)Atp_ProcessChoiceConstruct, \
		 name, desc, 0, 0, (Atp_VprocType)vproc, \
		 0, ATP_NULL_PTR, ((void *)(default))},
		 /* Pointer to default value is of type (Atp_ChoiceDescriptor *). */

#define ATP_END_OPT_CHOICE	{Atp_SetOptParm(ATP_ECH)},

#define ATP_BEGIN_CASE(label, desc, casevalue) \
		{ATP_BCS, (Atp_ParserType)Atp_ProcessCaseConstruct, label, desc, \
		 0, 0, 0, (Atp_NumType)casevalue},

#define ATP_END_CASE		{ATP_ECS},

#define ATP_CASE(label, desc, casevalue)	\
		{ATP_BCS, (Atp_ParserType)Atp_ProcessCaseConstruct, label, desc, \
		 0, 0, 0, (Atp_NumType)casevalue}, \
		 {ATP_ECS},

/*
-----------------------------------------------------------------
 ATP Parameters ParmDef Macros
-----------------------------------------------------------------
 */

#define atp_num_def(name,desc,min,max,vproc)	\
		{ATP_NUM, (Atp_ParserType)Atp_ProcessNumParm, name, desc, \
		 (Atp_NumType)min, (Atp_NumType)max, (Atp_VprocType)vproc},

#define atp_unsigned_num_def(name,desc,min,max,vproc)	\
		{ATP_UNS_NUM, (Atp_ParserType)Atp_ProcessUnsNumParm, name, desc, \
		 (Atp_UnsNumType)min, (Atp_UnsNumType)max, (Atp_VprocType)vproc},

#define atp_opt_num_def(name,desc,default,min,max,vproc) \
		{Atp_SetOptParm(ATP_NUM), (Atp_ParserType)Atp_ProcessNumParm, \
		 name, desc, (Atp_NumType)min, (Atp_NumType)max, \
		 (Atp_VprocType) vproc, (Atp_NumType)default},

#define atp_opt_unsigned_num_def(name,desc,default,min,max,vproc) \
		{Atp_SetOptParm(ATP_UNS_NUM), (Atp_ParserType)Atp_ProcessUnsNumParm, \
		 name, desc, \
		 (Atp_UnsNumType)min, (Atp_UnsNumType)max, (Atp_VprocType)vproc, \
		 (Atp_UnsNumType)default},

#ifdef ATP_REALNUM_IMPLEMENTED
#define atp_real_def(name,desc,min,max,vproc) \
		{ATP_REAL, (Atp_ParserType)Atp_ProcessRealNumParm, name, desc, \
		 (Atp_RealType)min, (Atp_RealType)max, (Atp_VprocType)vproc},

#define atp_opt_real_def(name,desc,default,min,max,vproc) \
		{Atp_SetOptParm(ATP_REAL), (Atp_ParserType)Atp_ProcessRealNumParm, \
		 name, desc, \
		 (Atp_RealType)min, (Atp_RealType)max, (Atp_VprocType)vproc, \
		 (Atp_RealType)default},
#endif

#define atp_str_def(name,desc,min,max,vproc) \
		{ATP_STR, (Atp_ParserType)Atp_ProcessStrParm, name, desc, \
		 (Atp_NumType)min, (Atp_NumType)max, (Atp_VprocType)vproc},

#define atp_opt_str_def(name,desc,default,min,max,vproc) \
		{Atp_SetOptParm(ATP_STR), (Atp_ParserType)Atp_ProcessStrParm, \
		 name, desc, (Atp_NumType)min, (Atp_NumType)max, \
		 (Atp_VprocType)vproc, 0, ATP_NULL_PTR, (void *)default},
		 /* Pointer to default value is of type (char *) */

#define atp_bool_def(name,desc,vproc) \
		{ATP_BOOL, (Atp_ParserType)Atp_ProcessBoolParm, \
		 name, desc, 0, 1, (Atp_VprocType)vproc},

#define atp_opt_bool_def(name,desc,default,vproc) \
		{Atp_SetOptParm(ATP_BOOL), (Atp_ParserType)Atp_ProcessBoolParm, \
		 name, desc, 0, 1, (Atp_VprocType)vproc, (Atp_BoolType)default},

#define atp_keyword_def(name,desc,keys,vproc) \
		{ATP_KEYS, (Atp_ParserType)Atp_ProcessKeywordParm, name, desc, 0, 0, \
		 (Atp_VprocType)vproc, 0, (Atp_KeywordType *)keys},

#define atp_opt_keyword_def(name,desc,DefaultKeyValue,keys,vproc) \
		{Atp_SetOptParm(ATP_KEYS), (Atp_ParserType)Atp_ProcessKeywordParm, \
		 name, desc, 0, 0, (Atp_VprocType)vproc, \
		 ((int)(DefaultKeyValue)), (Atp_KeywordType *)keys},

#define atp_data_bytes_def(name,desc,min,max,vproc)	\
		{ATP_DATA, (Atp_ParserType)Atp_ProcessDatabytesParm, name, desc, \
		 (Atp_NumType)min, (Atp_NumType)max, (Atp_VprocType)vproc},

#define atp_opt_data_bytes_def(name,desc,default,min,max,vproc) \
		{Atp_SetOptParm(ATP_DATA), (Atp_ParserType)Atp_ProcessDatabytesParm, \
		 name, desc, (Atp_NumType)min, (Atp_NumType)max, \
		 (Atp_VprocType)vproc, 0, ATP_NULL_PTR, \
		 ((void *)(default))},
		 /* Pointer to default value is of type (Atp_DataDescriptor *). */

#define atp_bcd_digits_def(name,desc,min,max,vproc)	\
		{ATP_BCD, (Atp_ParserType)Atp_ProcessBcdDigitsParm, name, desc, \
		 (Atp_NumType)min, (Atp_NumType)max, (Atp_VprocType)vproc},

#define atp_opt_bcd_digits_def(name,desc,default,min,max,vproc) \
		{Atp_SetOptParm(ATP_BCD), (Atp_ParserType)Atp_ProcessBcdDigitsParm, \
		 name, desc, (Atp_NumType)min, (Atp_NumType)max, \
		 (Atp_VprocType)vproc, 0, ATP_NULL_PTR, \
		 ((void *)(default))},
		 /* Pointer to default value is of type (Atp_DataDescriptor *). */

#ifdef ATP_COMMON_PARMDEF_IMPLEMENTED
/*
 *	When implementing common parmdef, remember to remove commented out
 *	field ParmDefPtr in struct _Atp_ParmDefEntry_ above.
 */
#define atp_common_parm_def(name,desc,parmDefPtr)	\
		{ATP_COM, (Atp_ParserType)0, name, desc, ATP_NULL_PTR, 0, 0, 0, \
		 ATP_NULL_PTR, ATP_NULL_PTR, parmDefPtr},

#define atp_opt_common_parm_def(name,desc,default,parmDefPtr)	\
		{Atp_SetOptParm(ATP_COM), (Atp_ParserType)0, name, desc, 0, 0, \
		 0, 0, ATP_NULL_PTR, ((void *)(default)), parmDefPtr},
#endif

#define atp_null_def(name,desc) {ATP_NULL, \
								 (Atp_ParserType)Atp_ProcessNullParm, \
								 name, desc},
/*
-----------------------------------------------------------------
 If ATP interfaces to a front-end tokeniser by means of an
 adaptor such as Atp2Tcl, then the adaptor should specify the
 name and version of the front-end, i.e. "Tcl" in this case.
-----------------------------------------------------------------
 */
EXTERN char NameOfFrontEndToAtpAdaptor[80];
EXTERN char *VersionOfFrontEndToAtpAdaptor;

/*
-----------------------------------------------------------------
 Exported ATP On-Line HELP facilities
-----------------------------------------------------------------
 */

/* Default names for ATP built-in on-line help commands. */
#define ATP_MANPAGE_CMDNAME		"man"
#define ATP_HELP_CMDNAME		"help"

/*
 *	Text types for on-line help information.
 *	Used as 1st argument to Atp_AddHelpInfo ()
 *	followed by command or help area name and
 *	help text.
 *	(Do not change - used for array indexing.)
 */
#define ATP_HELP_SUMMARY		0	/*	If used, name must be command name. */
#define ATP_MANPAGE_HEADER		1	/*	ditto */
#define ATP_MANPAGE_FOOTER		2	/*	ditto */
#define ATP_HELP_AREA_SUMMARY	3	/*	If used, name must be help area name. */

#define ATP_NO_OF_HELP_TYPES	4

/* Help command default areas */
#define ATP_HELPCMD_OPTION_DEFAULT		"-default"
#define ATP_HELPCMD_OPTION_SHOWALL		"-cmds"
#define ATP_HELPCMD_OPTION_CMDINFO		"-info"
#define ATP_HELPCMD_OPTION_CMDMANPG		"-man"
#define ATP_HELPCMD_OPTION_CMDPARMS		"-parms"
#define ATP_HELPCMD_OPTION_LANG			"-lang"
#define ATP_HELPCMD_OPTION_KEYWORD		"-key"
#define ATP_HELPCMD_OPTION_VERSION		"-version"
#define ATP_HELPCMD_OPTION_MISC			"misc"

/* Paging Mode Flags */
#define ATP_PAGING_MODE_OFF		0
#define ATP_PAGING_MODE_ON		1
#define ATP_PAGING_MODE_AUTO	2
#define ATP_QUERY_PAGING_MODE	3

/*
--------------------------------------------------------------------
 ATP does not provide a command register for lookup and callback
 execution purposes as it is designed to provide a parsing service
 to an external frontend language. (However, if an adaptor for a
 new language needs to be written and it does not provide a
 clientdata field "backdoor" which gets passed around, then ATP
 will have to provide a command callback lookup table.)

 Therefore, ATP has to be informed of mechanisms for accessing the
 external command table maintained by the frontend language (e.g.
 UCB Tcl "tickle"). This is for the purpose of providing HELP
 facilities to the end-user.
--------------------------------------------------------------------
 */
typedef struct _Atp_CmdTabAccessType_ {
	void * CommandTablePtr;		/* pointer to command table */
	void * (*FirstCmdTabEntry)	/* function returning 1st entry */
				_PROTO_ ((struct _Atp_CmdTabAccessType_ *ptr));
	void * (*NextCmdTabEntry)	/* function returning next entry */
				_PROTO_((struct _Atp_CmdTabAccessType_ *ptr));
	void * (*FindCmdTabEntry)	/* function returning entry being searched */
					_PROTO_( (struct _Atp_CmdTabAccessType_ *ptr, void *key));
	void * TableSearch;			/* pointer to table search structure,
								   if any */
	char * (*CmdName)_PROTO_((void *tablePtr, void *entryPtr));
								/* function returning command name
	 	 	 	 	 	 	 	   given command entry */
	char * (*CmdDesc)_PROTO_((void *entryPtr));
								/* function returning command
								   description given command entry */
	void * (*CmdRec)_PROTO_((void *entryPtr));
								/* function returning ATP command
								   record given command entry */
} Atp_CmdTabAccessType;

/*
--------------------------------------------------------------------
 EXTERNs for On-Line HELP facilities. Paging and Debug Modes
--------------------------------------------------------------------
 */
EXTERN char * Atp_GenerateParmDefHelpInfo _PROTO_((Atp_CmdRec *CmdRecPtr));
EXTERN char * Atp_ParmTypeString _PROTO_((int parmcode));

EXTERN Atp_ParmDefEntry *Atp_ManPage_PD_ptr;
EXTERN char * Atp_ManPageHeader[];
EXTERN Atp_Result (*Atp_ManPageCmd)_PROTO_((_ATP_EXT_CMD_CALLBACK_INTERFACE_));
EXTERN int Atp_DisplayManPage _PROTO_((void *, ...));
EXTERN int (*Atp_IsLangBuiltInCmd) _PROTO_((char *cmdname));

EXTERN int Atp_CreateHelpArea _PROTO_((char *help_area_name,
									   char *help_area_desc));

EXTERN Atp_Result Atp_AddHelpInfo _PROTO_((int text_type,
										  char *name,
										  char **paradesc));

EXTERN int Atp_AddCmdToHelpArea _PROTO_((int help_area_id,
										 Atp_CmdRec *CmdRecPtr));

EXTERN Atp_Result Atp_AppendCmdHelpInfo _PROTO_((Atp_CmdRec *CmdRecPtr,
												 int text_type,
												 char **text));

/* Atp_HelpCmd changed to be called by glue routine from frontend. */
EXTERN Atp_Result Atp_HelpCmd _PROTO_((void *clientData,
									  Atp_Result (*callback)(char *),
									  char **HelpPageReturnPtr));

EXTERN int					Atp_PagingMode _PROTO_((int flag));
EXTERN int					Atp_OutputPager _PROTO_ ((char *fmtstr _VARARGS_));
EXTERN int					Atp_PagingNeeded;
EXTERN Atp_Result			Atp_PagingCmd _PROTO_((_ATP_EXT_CMD_CALLBACK_INTERFACE_));
EXTERN Atp_ParmDefEntry		*Atp_PagingParmsPtr;
EXTERN Atp_Result			Atp_PagerCmd _PROTO_((_ATP_EXT_CMD_CALLBACK_INTERFACE_));
EXTERN Atp_ParmDefEntry		*Atp_PagerParmsPtr;

#ifdef DEBUG
  EXTERN Atp_BoolType		Atp_DebugMode;
  EXTERN Atp_ParmDefEntry	*Atp_DebugModeParmsPtr;
  EXTERN Atp_Result			Atp_DebugModeCmd
  	  	  	  	  	  	  	  	  	  _PROTO_((_ATP_EXT_CMD_CALLBACK_INTERFACE_));
#endif

EXTERN void *	(*Atp_GetExternalClientData)
										_PROTO_((_ATP_EXT_CMD_CALLBACK_INTERFACE_));
EXTERN void *	(*Atp_GetCmdTabAccessRecord)_PROTO_((void *clientdata));
EXTERN int		(*Atp_AdaptorUsed)_PROTO_((void * CmdEntryPtr));
EXTERN void		(*Atp_ReturnDynamicHelpPage)_PROTO_((char *HelpPage _VARARGS_));
EXTERN void		(*Atp_ReturnDynamicStringToAdaptor)_PROTO_((char *s _VARARGS_));

/*
 Exported ATP token parser functions (see also return codes below)
 */
EXTERN Atp_Result Atp_ProcessParameters
							_PROTO_((Atp_CmdRec *CmdRecPtr,
									int argc, char *argv[] ,
									char **return_string_ptr,
									void **parmstore_ptr));
EXTERN int Atp_ExecuteCallback _PROTO_((
									Atp_CmdCallBackType callBack,
									void *parmstore _VARARGS_));
EXTERN Atp_Result Atp_ProcessListConstruct _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ProcessRepeatBlockConstruct _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ParseRptBlkMarker _PROTO_((char *src,
												 int parmcode,
												 char **errmsg));


EXTERN Atp_Result Atp_ProcessChoiceConstruct _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ProcessCaseConstruct _PROTO_((Atp_ParserStateRec *));

EXTERN Atp_Result Atp_ProcessNumParm _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ProcessUnsNumParm _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ParseNum _PROTO_((char *input_src,
										Atp_ParmCode parmcode,
										/* ATP_NUM or ATP_UNS_NUM */
										Atp_NumType *numValPtr,
										/* Atp_NumType * or
										 Atp_UnsNumType *
										 based on parmcode */
										char **ErrorMsgPtr));

EXTERN Atp_Result Atp_ProcessDatabytesParm _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ParseDataBytes
							_PROTO_((char *hex_src,
									 Atp_DataDescriptor *dataBytes,
									 char **ErrorMsgPtr));

EXTERN Atp_Result Atp_ProcessBcdDigitsParm _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ParseBcdDigits
							_PROTO_((char *bcd_src,
									Atp_DataDescriptor *bcdDigits,
									char **ErrorMsgPtr));

EXTERN Atp_Result Atp_ProcessStrParm _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ParseStr _PROTO_((char *src,
										char **strPtr,
										int *strLenPtr,
										char *strType,
										/* ASCII type indicator */
										char **ErrorMsgPtr));

EXTERN Atp_Result Atp_ProcessKeywordParm _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ParseKeyword _PROTO_((char *src,
											Atp_KeywordType * KeyTable,
											Atp_NumType *KeyValuePtr,
											int *KeyIndexPtr,
											char **KeywordString,
											char **ErrorMsgPtr));

EXTERN Atp_Result Atp_ProcessBoolParm _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ParseBoolean _PROTO_((char *src,
											Atp_BoolType *boolValPtr,
											char **boolStrPtr,
											char **ErrorMsgPtr));

#ifdef ATP_REALNUM_IMPLEMENTED
EXTERN Atp_Result Atp_ProcessRealNumParm _PROTO_((Atp_ParserStateRec *));
EXTERN Atp_Result Atp_ParseRealNum _PROTO_(_VARARGS_);
#endif

EXTERN Atp_Result Atp_ProcessNullParm _PROTO_((Atp_ParserStateRec *));

/*
--------------------------------------------------------------------
  ATP token parser return codes
--------------------------------------------------------------------
*/
/*
  Explanation of return codes:

	ATP_OK		Parsing completed, parmstore ready,
				execute command

	ATP_ERROR	Parsing not completed, error message returned,
				do not execute command; other errors include
				malloc returning NULL

	ATP_RETURN	Command cannot proceed any further, return,
				do not execute command
				e.g. Help on parmdef service request done,
				help info returned
 */
#define ATP_OK		0
#define ATP_ERROR	-1
#define ATP_RETURN	1

/*
  These return codes for the frontend are for the adaptor interface
  to tell ATP what they are.
 */
EXTERN int Atp_Adaptor_FrontEnd_ReturnCode_OK;
EXTERN int Atp_Adaptor_FrontEnd_ReturnCode_ERROR;

/*
  Exported ATP parameter retrieval functions
 */
#ifdef DEBUG
#  define ATP_ERRLOC	__FILE__, __LINE__
#else
#  define ATP_ERRLOC	((char*)0), 0
#endif /* DEBUG */

#define Atp_Num(parmname)				Atp_RetrieveNumParm(parmname, ATP_ERRLOC)
#define Atp_UnsignedNum(parmname)		Atp_RetrieveUNumParm(parmname, ATP_ERRLOC)
#define Atp_Index(parmname)				Atp_RetrieveIndex(parmname, ATP_ERRLOC)
#define Atp_Bool(parmname)				Atp_RetrieveBoolParm(parmname, ATP_ERRLOC)
#define Atp_Str(parmname)				Atp_RetrieveStrParm(parmname, ATP_ERRLOC)

#define Atp_DataBytes(parmname, count)	Atp_RetrieveDataBytesParm(parmname, count, ATP_ERRLOC)
#define Atp_DataBytesDesc(parmname)		Atp_RetrieveDataBytesDescriptor(parmname, ATP_ERRLOC)
#define Atp_BcdDigits(parmname, count)	Atp_RetrieveBcdDigitsParm(parmname, count, ATP_ERRLOC)
#define Atp_BcdDigitsDesc(parmname)		Atp_RetrieveBcdDigitsDescriptor(parmname, ATP_ERRLOC)
#define Atp_ParmPtr(parmname)			Atp_RetrieveParm(parmname, ATP_ERRLOC)
#define Atp_RptBlockPtr(name, count)	Atp_RetrieveRptBlk(name, count, ATP_ERRLOC)
#define Atp_RptBlockDesc(name)			Atp_RetrieveRptBlkDescriptor(name, ATP_ERRLOC)

EXTERN Atp_NumType		Atp_RetrieveNumParm _PROTO_((char *NumericParmName,
													 char *filename,
													 int line_number));

EXTERN Atp_UnsNumType	Atp_RetrieveUNumParm _PROTO_((char *UNumParmName,
													  char *filename,
													  int line_number));

EXTERN Atp_UnsNumType	Atp_RetrieveIndex _PROTO_((char *ParmName,
												   char *filename,
												   int line_number));

EXTERN Atp_BoolType		Atp_RetrieveBoolParm _PROTO_((char *BooleanParmName,
													  char *filename,
													  int line_number));

EXTERN char *			Atp_RetrieveStrParm _PROTO_((char *StrParmName,
													 char *filename,
													 int line_number));

EXTERN Atp_ByteType *	Atp_RetrieveDataBytesParm
								_PROTO_((char *dataParmName,
										 Atp_UnsNumType *NumOfBytes,
										 char *filename,
										 int line_number));

EXTERN Atp_DataDescriptor
						Atp_RetrieveDataBytesDescriptor
								_PROTO_((char *dataParmName,
										 char *filename,
										 int line_number));

EXTERN Atp_ByteType *	Atp_RetrieveBcdDigitsParm
								_PROTO_((char *BcdDigitsParmName,
										 Atp_UnsNumType *NumOfNibbles,
										 char *filename,
										 int line_number));
EXTERN Atp_DataDescriptor
						Atp_RetrieveBcdDigitsDescriptor
								_PROTO_((char *BcdDigitsParmName,
										 char *filename,
										 int line_number));

EXTERN char *			Atp_DisplayBcdDigits
								_PROTO_((Atp_DataDescriptor bcd_digits_desc));

EXTERN char *			Atp_DisplayHexBytes
								_PROTO_((Atp_DataDescriptor databytes_desc,
										 char *hexbyte_separator));

EXTERN Atp_ByteType *	Atp_RetrieveParm _PROTO_ ((char *AnyParmName,
												   char *filename,
												   int line_number));

EXTERN Atp_ByteType *	Atp_RetrieveRptBlk _PROTO_((char *RptBlkName,
													int *RptBlkTimes,
													char *filename,
													int Iine_number));

EXTERN Atp_DataDescriptor
						Atp_RetrieveRptBlkDescriptor
											_PROTO_((char *RptBlkName,
													 char *filename,
													 int line_number));

EXTERN Atp_ByteType *	Atp_ResetParmPtr _PROTO_((void));

/*
--------------------------------------------------------------------
 Exported Miscellaneous ATP functions
--------------------------------------------------------------------
*/

#define Atp_Initialize		Atp_Tnitialise

EXTERN void		Atp_Initialise _PROTO_((void));

EXTERN int		Atp_EvalNoOfParmDefEntries
						_PROTO_((Atp_ParmDefEntry *ParmDefPtr,
								 int sizeOfParmdef));

EXTERN int		Atp_VerifyParmDef _PROTO_((Atp_ParmDefEntry *parmdef,
										   int entries));

EXTERN void		Atp_SetDefaultMallocErrorHandler
						_PROTO_((void (*fn)(unsigned size,
								 char *filename,
								 int line_number)));

EXTERN int		Atp_Strcmp _PROTO_((char *sl, char *s2));
EXTERN int		Atp_Strncmp _PROTO_((char *sl, char *s2, int n));
EXTERN char *	Atp_StrToLower _PROTO_((char *string));
EXTERN int		Atp_MatchStrings _PROTO_((char *src, char *strTab[]));
EXTERN char **	Atp_Tokeniser _PROTO_((char *Source, int *tokencount));
EXTERN void		Atp_FreeTokenList _PROTO_((char **tokenlist));
EXTERN char *	Atp_GetToken _PROTO_((char *src,
									  char **NextSrc,
									  Atp_UnsNumType *TokenLengthPtr));

/* Dynamic string printf declarations, (atpdynam.c) */
#define ATP_MAX_DYNAM_STRING_SIZE	1000000

/* Memory debug */
#ifdef ATP_MEM_DEBUG
EXTERN int		Atp_MemDebugDvsPrintf _PROTO_ ((char **outstr,
												char *fmtstr
												_VARARGS_));
EXTERN int		Atp_MemDebugAdvPrintf _PROTO_((char *fmtstr _VARARGS_));
EXTERN char *	Atp_MemDebugAdvGets _PROTO_((void));
EXTERN char *	Atp_MemDebugAdvGetsn _PROTO_((int *count));
#else
EXTERN int		Atp_DvsPrintf _PROTO_((char **outstr, char *fmtstr _VARARGS_));
EXTERN int		Atp_AdvPrintf _PROTO_((char *fmtstr _VARARGS_));
EXTERN char *	Atp_AdvGets _PROTO_((void));
EXTERN char *	Atp_AdvGetsn _PROTO_((int *count));
#endif
EXTERN int		Atp_AdvSetDefBufsize _PROTO_( (int size));
EXTERN int		Atp_AdvGetBufsize _PROTO_((void));
EXTERN int		Atp_AdvGetDefBufsize _PROTO_((void));
EXTERN int		Atp_AdvResetDefBufsize _PROTO_((void));
EXTERN int		Atp_PrintfWordWrap _PROTO_(
									( int (*printf_function)(char *fmtstr, ...),
									  int screen_width,
									  int start_column,
									  int indent,
									  char *format_string, ...));

EXTERN int	(*Atp_FormatStrlen)_PROTO_(( char *fmtstr, va_list ap ));

EXTERN int	Atp_Used_By_G3O;
EXTERN void Atp_EnumerateProtocolFieldValues
					_PROTO_((Atp_KeywordType *KeyTabPtr, int indent));
			/* if Atp_Used_By_G30 is 1, Atp_EnumerateProtocolFieldValues()
			 uses AtpAdvPrintf() to display output; otherwise it uses
			 printf() */

#ifdef ATP_MEM_DEBUG
EXTERN int	Atp_MemDebugAdvResetBuffer _PROTO_((void));
#else
EXTERN int	Atp_AdvResetBuffer _PROTO_((void));
#endif

/* Memory debug */
#ifdef ATP_MEM_DEBUG

#define ATP_MEMDEBUG_CMD	"checkmem"

#define Atp_Strdup(s)		Atp_MemDebug_Strdup((s), __FILE__, __LINE__)
EXTERN char *				Atp_MemDebug_Strdup _PROTO_((char *s,
														char *filename,
														int line_number));

EXTERN void					Atp_MemDebugLocn _PROTO_((char *filename,
													 int line_number));

#define Atp_DvsPrintf		(Atp_MemDebugLocn(__FILE_,__LINE__),\
							 Atp_MemDebugDvsPrintf)

#define Atp_AdvPrintf		(Atp_MemDebugLocn(__FILE_,__LINE__),\
							 Atp_MemDebugAdvPrintf)

#define Atp_AdvGets()		(Atp_MemDebugLocn(__FILE_,__LINE__),\
							 Atp_MemDebugAdvGets())

#define Atp_AdvGetsn(n)		(Atp_MemDebugLocn(__FILE_,__LINE__),\
							 Atp_MemDebugAdvGetsn(n))

#define Atp_AdvResetBuffer	(Atp_MemDebugLocn (__FILE__,__LINE__) , \
							 Atp_MemDebugAdvResetBuffer)

#else

#define Atp_Strdup		strdup

#endif

#endif /* _ATP */
