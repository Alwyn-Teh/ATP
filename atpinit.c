/* EDITION AC04 (REL002), ITD ACST.161 (95/05/04 20:23:54) -- CLOSED but edited */

/*+*******************************************************************

	Module Name:		atpinit.c

	Copyright:			BNR Europe Limited, 1992-1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains functions	used during
						the initialization phase of ATP.

						PLS ITD ACST.158 (950412)
						Deletion functions added:
							Atp_CleanupProc()
							Atp_De1eteCommand()
							Atp_DeleteCmdGrp()
							Atp_DeleteCommandRecord()
							Atp_DeleteCmdRegister()
							Atp_DeleteCmdGrpRegister()

						PLS ITD ACST.161 (950504)
						Detect window resize and update environment
						variables COLUMNS and LINES.

*******************************************************************-*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>

#ifndef TIOCGWINSZ
# include <termios.h>
#endif

#include <setjmp.h>
#include <unistd.h>

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/* Local #defines */
#define DEFAULT_NUM_OF_ATP_CMDGRPS	4
#define DEFAULT_NUM_OF_ATP_COMMANDS	16

/* Local typedefs */
typedef struct {
	void			*GrpIdPtr;		/*	pointer	to external id */
	CommandRecord	**CmdRegister;	/*	Command	register for this id	*/
	int				size;			/*	Size of	command register */
	int				counter;		/*	Command	counter	*/
	int				status;			/*	0 = unused,	1 = inuse, 2 = deleted */
} Atp_CmdGrp;

/* Global variables */
int		Atp_Adaptor_FrontEnd_ReturnCode_OK = ATP_OK;
int		Atp_Adaptor_FrontEnd_ReturnCode_ERROR = ATP_ERROR;
jmp_buf	*Atp_JmpBufEnvPtr = NULL;

/* Local variables */
static	Atp_CmdGrp	*Atp_CmdRegister =	NULL;
static	int	NoOfCmdGrps	= 0;

char *	Atp_Copyright[] = {
	"ATP (Advanced Token Parser) by Alwyn Teh, alteh@bnr.ca",
	" Copyright (C) 1992-1995 BNR Europe Limited, Maidenhead, U.K.",
	" Bell-Northern Research & Northern Telecom / NORTEL",
	NULL
};

/* Functions */
static Atp_Result	Atp_DeleteCmdRegister();
static Atp_Result	Atp_DeleteCmdGrpRegister _PROTO_((CommandRecord **cmdList));
static void			resize_handler _PROTO_((int));

/*+*************************************************************+*****

	Function Name:		Atp_AssembleCmdRecord

	Copyright:			BNR Europe Limited, 1992, 1993, 1995
						Bell-Northern Research
						Northern Telecom

	Description:		This function collects in a structure all
						the essential components of an ATP command.

	Parameters:			name	- name of command visible to user
						desc	- short description of	command
						cb		- callback function for command
						pd		- parmdef for command
						ne		- number of entries in parmdef
						cd		- client data supplied for command

	Global Variables:	None

	Results:			Returns pointer to an Atp_CmdRec structure

	Calls:				malloc() or Atp_Malloc()

	Called by:			Atp2Tcl_CreateCommand()

	Side effects:		None

	Notes:				Record Atp_CmdRec is kept for the duration of
						a command's existence.

	Modifications:
		Who			When					Description
	----------	---------------	---------------------------------------
	Alwyn Teh	30 June	1992	Initial Creation
	Alwyn Teh	10 June	1993	Initialise new manpage
								descriptive text paragraph
	Alwyn Teh	10 June 1993	Manage command register	in
								atpcmd.c instead.
	Alwyn Teh	29 June	1993	Manpage text pointer changed
								to general helpInfo pointer
								in order to refer to manpage
								header, footer and help text.
	Alwyn Teh	1 July 1993		Return NULL if no name.
	Alwyn Teh	17 January 1994	Change strdup to Atp_Strdup
								for memory debugging.
	Alwyn Teh	11 April 1995 	strdup desc as well since some
								applications may use volatile
								strings

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_CmdRec * Atp_AssembleCmdRecord
(
	char				*name,
	char				*desc,
	Atp_CmdCallBackType	cb,
	Atp_ParmDef			pd,
	int					ne,
	void				*cd
)
#else
Atp_CmdRec *
Atp_AssembleCmdRecord(name,desc,cb,pd,ne,cd)
	char				*name;	/* command name */
	char				*desc;	/* command description */
	Atp_CmdCallBackType	cb;		/* callback function pointer	*/
	Atp_ParmDef			pd;		/* parmdef pointer */
	int					ne;		/* number of entries in parmdef */
	void 				*cd;	/* original client data */
#endif
{
	CommandRecord *xcmd;

	printf("Atp_AssembleCmdRecord - 1\n");

	if (name == NULL || *name == '\0')
	  return NULL;

	xcmd = (CommandRecord *)CALLOC(1, sizeof(CommandRecord), NULL);

	printf("Atp_AssembleCmdRecord - 2\n");

	/*
	 *	Command name may contain mixed upper/lower case letters
	 *	Keep a copy of the original name, then convert the orig
	 *	name to lower case letters. This is because ATP may be
	 *	requested to search for the command in non-case-
	 *	sensitive mode.
	 */
	xcmd->cmdNameOrig	= Atp_Strdup(name);
	printf("Atp_AssembleCmdRecord - 2.1\n");
	xcmd->cmdName		= Atp_StrToLower(name);
	printf("Atp_AssembleCmdRecord - 2.2\n");
	xcmd->cmdDesc		= Atp_Strdup(desc);
	printf("Atp_AssembleCmdRecord - 2.3\n");
	xcmd->helpInfo		= NULL;
	printf("Atp_AssembleCmdRecord - 2.4\n");
	xcmd->callBack		= cb;
	printf("Atp_AssembleCmdRecord - 2.5\n");
	xcmd->parmDef		= (ParmDefEntry *)pd;
	printf("Atp_AssembleCmdRecord - 2.6\n");
	xcmd->NoOfPDentries	= ne;
	printf("Atp_AssembleCmdRecord - 2.7\n");
	xcmd->clientData	= cd;
	printf("Atp_AssembleCmdRecord - 2.8\n");

	xcmd->Atp_ID_Code	= ATP_IDENTIFIER_CODE;

	printf("Atp_AssembleCmdRecord - 3\n");

	return (Atp_CmdRec *)xcmd;
}

/*+*******************************************************************

	Function Name:		Atp_Initialise
						Atp_CleanupProc
						resize_handler

	Copyright:			BNR Europe Limited, 1992, 1995
						Bell-Northern Research
						Northern Telecom

	Description:		Initialises ATP - contains only	ATP-specific
						code. The rest goes in the adaptor.

	Modifications:
		Who			When				Description
	----------	-----------------	-----------------------------
	Alwyn Teh	10 October 1992		Initial Creation
	Alwyn Teh	11 April 1995		Delete left-over commands
									upon program exit...etc.
	Alwyn Teh	24 April 1995		Atp_Initialise() should
									handle window resize signal
									and update window size
									environment variables
									COLUMNS and LINES.

*******************************************************************-*/
#if defined (__STDC__) || defined(__cplusplus)
static void resize_handler(int sigval)
#else
static void resize_handler(sigval)
	int sigval; /* 0 indicates should initialize signal() */
#endif
{
	static void (*prev_resize_handler) _PROTO_((int)) = NULL;

	int Columns, Lines;
	static char columns_env[25];
	static char lines_env[25];
	struct winsize window_size;

	if (sigval == 0)
	{
	  prev_resize_handler = signal(SIGWINCH, resize_handler);
	}
	else
	if (sigval == SIGWINCH)
	{
	  /* Obtain new window size. */
	  ioctl(1, TIOCGWINSZ, &window_size);
	  Columns = (int) window_size.ws_col;
	  Lines = (int) window_size.ws_row;

	  /*
			Send a SIGWINCH to all processes in the terminal's foreground
			process group - just in case.
	   */
	  ioctl(1, TIOCSWINSZ, &window_size);

	  /* Update COLUMNS and LINES environment variables for this process. */
	  (void) sprintf(columns_env, "COLUMNS=%d", Columns);
	  (void) sprintf(lines_env, "LINES=%d", Lines);
	  putenv(columns_env);
	  putenv(lines_env);

	  /* Execute any previous resize signal handler. */
	  if (prev_resize_handler != NULL)
		prev_resize_handler(SIGWINCH);

	  /* Signal processed, reset signal handler. */
	  (void) signal(SIGWINCH, resize_handler);
	}
}

void Atp_CleanupProc()
{
	Atp_HelpCmdDeleteProc();

	Atp_DeleteCmdRegister();

	if (Atp_JmpBufEnvPtr != NULL)
	{
	  FREE(Atp_JmpBufEnvPtr);
	  Atp_JmpBufEnvPtr = NULL;
	}
}

void Atp_Initialise()
{
	int Columns = 0, Lines = 0;
	char *columns_value, *lines_value;
	struct winsize initial_window_size;
	static char initial_columns_env[25];
	static char initial_lines_env[25];

	/* Ensure COLUMNS and LINES are set, if not, find out and set them. */
	columns_value = getenv("COLUMNS");
	lines_value = getenv("LINES");

	if (columns_value == NULL || *columns_value == '\0')
	{
	  /* $COLUMNS not set, use ioctl to find out. */
	  ioctl(1, TIOCGWINSZ, &initial_window_size);
	  Columns = (int) initial_window_size.ws_col;
	  (void) sprintf(initial_columns_env, "COLUMNS=%d", Columns);
	  putenv(initial_columns_env);
	}
	else
	  Columns = atoi(columns_value);

	if (lines_value == NULL || *lines_value == '\0')
	{
	  /* $LINES not set, use ioctl to find out. */
	  ioctl(1, TIOCGWINSZ, &initial_window_size);
	  Lines = (int) initial_window_size.ws_row;
	  (void) sprintf(initial_lines_env, "LINES=%d", Lines);
	  putenv(initial_lines_env);
	}
	else
	  Lines = atoi(lines_value);

	/* Set handler to detect window resize. */
	resize_handler(0);

	/*
		Jump environment buffer is usually fairly big, get space
		for it at run-time.
	*/
	Atp_JmpBufEnvPtr = (jmp_buf *)CALLOC(1, sizeof(jmp_buf), NULL);

	/*
		Register cleanup procedure with atexitO or on_exit() .
		The cleanup procedure is called when exit() is invoked.
	*/
#if __STDC__ || hpux
		atexit(Atp_CleanupProc);
#else
#  if sun || sun2 || sun3 || sun4
		on_exit(Atp_CleanupProc);
#  endif
#endif
}

/*+*******************************************************************

	Function Name:		Atp_GetCmdGrp

	Copyright:			BNR Europe Limited, 1993, 1995
						Bell-Northern Research
						Northern Telecom

	Description:		Creates and maintains ATP command groups.

	Modifications:
		Who			When				Description
	----------	--------------	-------------------------------
	Alwyn Teh	10 June 1993	Initial Creation
	Alwyn Teh	11 April 1995	Support Atp_CmdGrp deletion
								hence check status in loop

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static Atp_CmdGrp * Atp_GetCmdGrp( void *id )
#else
static Atp_CmdGrp *
Atp_GetCmdGrp(id)
	void *id;
#endif
{
	Atp_CmdGrp *cmdgrp = NULL;
	int idx = 0;

	/* Create new register first time round. */
	if (Atp_CmdRegister == NULL) {
	  Atp_CmdRegister = (Atp_CmdGrp *) CALLOC(DEFAULT_NUM_OF_ATP_CMDGRPS,
			  	  	  	  	  	  	  	  	  sizeof(Atp_CmdGrp), NULL);
	  NoOfCmdGrps = DEFAULT_NUM_OF_ATP_CMDGRPS;
	}
	else
	/* Locate command group using id. If found, return immediately. */
	  for (idx= 0; Atp_CmdRegister[idx].status != 0; idx++)
		 if (Atp_CmdRegister[idx].status == 1 && /* 1 indicates in-use */
			 Atp_CmdRegister[idx].GrpIdPtr == id)
		   return &Atp_CmdRegister[idx];

	/* New group or can't find it. Check if need more room. */
	if (idx >= NoOfCmdGrps-1) {
	  NoOfCmdGrps += DEFAULT_NUM_OF_ATP_CMDGRPS;
	  Atp_CmdRegister = (Atp_CmdGrp *) REALLOC(	Atp_CmdRegister,
												NoOfCmdGrps *
												sizeof(Atp_CmdGrp),
												NULL );
	}

	/* Initialise new command group. */
	Atp_CmdRegister[idx].GrpIdPtr = id;
	Atp_CmdRegister[idx].CmdRegister = (CommandRecord **)
										CALLOC(DEFAULT_NUM_OF_ATP_COMMANDS,
										sizeof (CommandRecord *), NULL);
	Atp_CmdRegister[idx].size = DEFAULT_NUM_OF_ATP_COMMANDS;
	Atp_CmdRegister[idx].counter = 0;
	Atp_CmdRegister[idx].status = 1;
	Atp_CmdRegister[idx+1].status = 0; /* ensure null-terminated */

	cmdgrp = &Atp_CmdRegister[idx];

	return cmdgrp;
}

/*+*******************************************************************

	Function Name:		Atp_RegisterCommand

	Copyright:			BNR Europe Limited,	1993
						Bell-Northern Research
						Northern Telecom

	Description:		Registers a command	with a command group.

	Modifications:
		Who			When				Description
	----------	---------------	----------------------------
	Alwyn Teh	10 June 1993	Initial Creation
	Alwyn Teh	25 January 1994	Return ATP_OK at end

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_RegisterCommand(void *CmdGrpId, Atp_CmdRec *CmdRecPtr )
#else
Atp_Result
Atp_RegisterCommand(CmdGrpId, CmdRecPtr)
	void		*CmdGrpId;
	Atp_CmdRec	*CmdRecPtr;
#endif
{
		Atp_CmdGrp	*CmdGrpPtr = NULL;

		if (CmdGrpId == NULL || CmdRecPtr == NULL)
		  return ATP_ERROR;

		CmdGrpPtr = Atp_GetCmdGrp(CmdGrpId);

		/* Increase size of register if necessary. */
		if (CmdGrpPtr->counter >= CmdGrpPtr->size-1)
		{
		  CmdGrpPtr->size += DEFAULT_NUM_OF_ATP_COMMANDS;
		  CmdGrpPtr->CmdRegister =
				(CommandRecord **) REALLOC(	CmdGrpPtr->CmdRegister,
											CmdGrpPtr->size *
													sizeof(CommandRecord *),
											NULL );
		}

		/* Append command record to end of register. */
		CmdGrpPtr->CmdRegister[CmdGrpPtr->counter++] = (CommandRecord *) CmdRecPtr;
		CmdGrpPtr->CmdRegister[CmdGrpPtr->counter] = NULL;

		return ATP_OK;
}

/*+*******************************************************************

	Function Name:		Atp_FindCommand

	Copyright:			BNR Europe Limited,	1993
						Bell-Northern Research
						Northern Telecom

	Description:		Finds a command in the command register of
						command groups.

	Modifications:
		Who			When				Description
	----------	-------------	----------------------------
	Alwyn Teh	10 June 1993	Initial	Creation

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_CmdRec * Atp_FindCommand( void *ptr, int mode )
#else
Atp_CmdRec *
Atp_FindCommand(ptr, mode)
	void	*ptr;
	int		mode; /* 0 = find record, 1 = find command name */
#endif
{
	register int x, y;

	if (ptr == NULL || (mode != 0 && mode != 1) || Atp_CmdRegister == NULL)
	  return NULL;

	for (x = 0; Atp_CmdRegister[x].status != 0; x++)
	   for (y = 0; Atp_CmdRegister[x].CmdRegister[y] != NULL; y++)
		  if ((mode == 0 && Atp_CmdRegister[x].CmdRegister[y] == ptr) ||
			  (mode == 1 &&	Atp_Strcmp((Atp_CmdRegister[x].CmdRegister[y])->cmdName,
					  	  	  	  	   (char *)ptr) == 0))
			if ((Atp_CmdRegister[x].CmdRegister[y])->Atp_ID_Code ==
										ATP_IDENTIFIER_CODE) /* double-check */
			  return (Atp_CmdRec *)(Atp_CmdRegister[x].CmdRegister[y]);

	return NULL;
}

/*+********************************************************************

	Function Name:		Atp_DeleteCommand
						Atp_De1eteCommandRecord
						Atp_DeleteCmdRegister

	Copyright:			BNR Europe Limited,	1995
						Bell-Northern Research
						Northern Telecom

	Description:		Deletes an ATP command

	Modifications:
		Who			When				Description
	----------	--------------	------------------------------
	Alwyn Teh	11 April 1995	Initial	Creation

*********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_DeleteCommand(char *cmdName)
#else
Atp_Result Atp_DeleteCommand(cmdName)
char *cmdName;
#endif
{
	Atp_CmdRec *cmdRec = NULL;

	cmdRec = Atp_FindCommand(cmdName, 1);
	if (cmdRec != NULL)
	  return Atp_DeleteCommandRecord(cmdRec);

	return ATP_ERROR;
}

#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_DeleteCommandRecord(Atp_CmdRec *cmdRec)
#else
Atp_Result Atp_DeleteCommandRecord(cmdRec)
	Atp_CmdRec *cmdRec;
#endif
{
	CommandRecord *CmdRecord = (CommandRecord *)cmdRec;
	register int x, y, z;

	if (cmdRec == NULL || Atp_CmdRegister == NULL)
	  return ATP_ERROR;
	else
	{
	  /* Delete malloced contents of cmdRec */
	  if (CmdRecord->cmdNameOrig != NULL)
	    FREE(CmdRecord->cmdNameOrig);
	  if (CmdRecord->cmdDesc != NULL)
	    FREE(CmdRecord->cmdDesc);
	}

	/* Delete cmdRec from Atp_CmdRegister */
	for (x = 0; Atp_CmdRegister[x].status != 0; x++)
	   if (Atp_CmdRegister[x].status == 1)
		 for (y = 0; Atp_CmdRegister[x].CmdRegister[y] != NULL; y++)
			if ((Atp_Strcmp((Atp_CmdRegister[x].CmdRegister[y])->cmdName,
							 cmdRec->cmdName) == 0) &&
			   ((Atp_CmdRegister[x].CmdRegister[y])->Atp_ID_Code ==
					   	   	 ATP_IDENTIFIER_CODE) && /* double-check */
				(Atp_CmdRegister[x].CmdRegister[y] == CmdRecord))
			{
				CommandRecord **ptr = NULL;

				/* Dispose of CmdRecord properly. */
				CmdRecord = Atp_CmdRegister[x].CmdRegister [y];
				if (CmdRecord->helpInfo != NULL)
				{
				  Atp_HelpSectionsType *HelpInfoPtr;
				  HelpInfoPtr = (Atp_HelpSectionsType *)(CmdRecord->helpInfo);

				  for (z = 0; z < ATP_NO_OF_HELP_TYPES; z++)
				  {
				     if ((HelpInfoPtr->HelpSectionsPtr)[z] != NULL)
					   /* Free Atp_HelpSubSectionsType */
					   FREE((HelpInfoPtr->HelpSectionsPtr)[z]);
				  }

				  FREE(CmdRecord->helpInfo);
				}
				FREE(CmdRecord);

				/* Remove cmdRec by shrinking NULL-terminated list */
				ptr = &(Atp_CmdRegister[x].CmdRegister[y]);
				while (*ptr != NULL)
				{
					 *ptr = *(ptr+1);
					 ptr++;
				}
				return ATP_OK;
			}
	return ATP_ERROR;
}

/*
	Atp_DeleteCmdGrp() removes a command group from the command register,
	taking care to delete the command list contained inside.
 */
#if defined(__STDC__) || defined(__cplusplus)
Atp_Result Atp_DeleteCmdGrp(void *id)
#else
Atp_Result Atp_DeleteCmdGrp(id)
	void *id;
#endif
{
	register int x;

	if (Atp_CmdRegister == NULL)
	  return ATP_ERROR;

	for (x = 0; Atp_CmdRegister[x].status != 0; x++)
	   if (Atp_CmdRegister[x].status == 1 && /* 1 indicates in-use */
		   Atp_CmdRegister[x].GrpIdPtr == id)
	   {
		 Atp_CmdGrp *ptr;
		 Atp_CmdRegister[x].status = 2; /* deleted */
		 Atp_DeleteCmdGrpRegister(Atp_CmdRegister[x].CmdRegister);
		 ptr = &Atp_CmdRegister[x];
		 while (ptr->status != 0)
		 {
			 *ptr = *(ptr+1);
			 ptr++;
		 }
		 return ATP_OK;
	   }

	return ATP_ERROR;
}

/*
	Atp_DeleteCmdGrpRegister() deletes a specified command list
	for a group of commands.
 */
#if defined(__STDC__) || defined(__cplusplus)
static Atp_Result Atp_DeleteCmdGrpRegister(CommandRecord **cmdList)
#else
static Atp_Result Atp_DeleteCmdGrpRegister(cmdList)
	CommandRecord **cmdList ;
#endif
{
	register int x;

	if (cmdList == NULL)
	  return ATP_ERROR;

	for (x=0; cmdList[x] != NULL; x++)
	   Atp_DeleteCommandRecord((Atp_CmdRec *)cmdList[x]);

	FREE(cmdList);

	return ATP_OK;
}

/*
	Atp_DeleteCmdRegister() deletes the whole Atp_CmdRegister and
	everything inside it.
 */
static Atp_Result Atp_DeleteCmdRegister()
{
	register int x;

	if (Atp_CmdRegister == NULL)
	  return ATP_ERROR;

	for (x=0; Atp_CmdRegister[x].status != 0; x++)
	   if (Atp_CmdRegister[x].CmdRegister != NULL)
		 Atp_DeleteCmdGrpRegister(Atp_CmdRegister[x].CmdRegister);

	FREE(Atp_CmdRegister);
	Atp_CmdRegister = NULL;

	return ATP_OK;
}
