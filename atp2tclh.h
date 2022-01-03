/* EDITION AC04 (REL002), ITD ACST.179 (95/06/22 15:00:00) -- OPEN */
/*+*******************************************************************

  Module Name:		atp2tclh.h

  Copyright:		BNR Europe Limited, 1992-1994
					Bell-Northern Research
					Northern Telecom

  Description:		This header file contains the ATP-to-Tcl
					Adaptor Interface facilities which are
					externally visible.

*******************************************************************-*/

#ifndef _ATP2TCL
#define _ATP2TCL

#define ATP2TCL_VERSION "1.2"

#include <tcl.h>

#ifdef ATP_MEM_DEBUG
#include "atpsysex.h"
#endif

#define _TCL_CMD_CALLBACK_ARGS_ (ClientData clientData, \
								 Tcl_Interp *interp, \
								 int argc, char *argv[])

#ifdef		_ATP_EXT_CMD_CALLBACK_INTERFACE_
#	undef	_ATP_EXT_CMD_CALLBACK_INTERFACE_
#	define	_ATP_EXT_CMD_CALLBACK_INTERFACE_ _TCL_CMD_CALLBACK_ARGS_
#endif

/*
	Include atph.h here to get typedef of Atp_Result...etc. and for
	atph.h to pick up definition of _ATP_EXT_CMD_CALLBACK_INTERFACE_
	from this header file.
*/

#include "atph.h"

#undef _PROTO_
#undef _VARARGS_
#undef EXTERN

#if defined(__STDC__) || defined(__cplusplus)
#	define _PROTO_(s) s
#	ifdef __STDC__
#		define _VARARGS_ , ...
#	else /* #elif not supported by K&R cpp */
#		ifdef __cplusplus
#			define _VARARGS_ ...
#		endif
#	endif
#else
#	define _PROTO_(s) ()
#	define _VARARGS_ ()
#endif

#ifdef __cplusplus
#	define EXTERN extern "C"
#else
#	define EXTERN extern
#endif

/*
----------------------------------------------------------------
  KEY
		interp:		Tcl interpreter
		name:		Name of command
		desc:		Description of command
		help_id:	Help area ID
		cb:			Command Callback
		pd:			Parameter Definition table
		cd:			Client Data
		delProc:	Delete command tidy-up Procedure

----------------------------------------------------------------
 */

#ifdef DEBUG
#define Atp2Tcl_CreateCommand(interp,name,desc,help_id,cb,pd,cd,delProc) \
				Atp2Tcl_InternalCreateCommand(interp, name, desc,	\
												help_id,cb,pd,Atp_NoOfPDentries(pd),\
												cd,delProc, \
												__FILE__, __LINE__, 0)
#else
#define Atp2Tcl_CreateCommand(interp,name,desc,help_id,cb,pd,cd,delProc) \
				Atp2Tcl_InternalCreateCommand(interp,name,desc,	\
												help_id,cb,pd,Atp_NoOfPDentries(pd),\
												cd,delProc, \
												(char *) 0, 0, 0)
#endif

EXTERN Atp_Result	Atp2Tcl_Init _PROTO_((Tcl_Interp *interp));
EXTERN Atp_Result	Atp2Tcl_AddHelpInfo _PROTO_((Tcl_Interp *interp,
												 int text_type,
												 char *HelpName,
												 char **DescText));

/* FOR ATP INTERNAL USE ONLY */
EXTERN int			Atp2Tcl_Adaptor _PROTO_(_TCL_CMD_CALLBACK_ARGS_);
EXTERN Atp_Result	Atp2Tcl_InternalCreateCommand
						_PROTO_((Tcl_Interp *interp,
								 char *name, char *desc,
								 int help_id,
								 Atp_Result (*callback)
										 _PROTO_((ClientData clientdata,
												  Tcl_Interp *interp,
												  int argc,
												  char **argv)),
								 Atp_ParmDef parmdef,
								 int sizeof_parmdef,
								 ClientData clientdata,
								 Tcl_CmdDeleteProc *deleteproc,
								 char *filename, int linenumber,
								 Atp_CmdRec **retum_cmd_rec));

typedef int			(*PFI)_PROTO_((char *format_string _VARARGS_));
EXTERN PFI			Atp2Tcl_GetPager _PROTO_((Tcl_Interp *interp));
EXTERN int			Atp2Tcl_UnknownCmd _PROTO_((ClientData clientdata,
												Tcl_Interp *interp,
												int argc, char **argv));
EXTERN char **		Tcl_Copyright;

#endif /* _ATP2TCL */
