/* EDITION AC03 (REL002), ITD ACST.158 (95/04/21 20:43:24) -- CLOSED */

/*+********************************************************************

	Module Name:		atpmem.c

	Copyright:			BNR Europe Limited, 1992 - 1995
						Bell-Northern Research / BNR
						Northern Telecom / Nortel

	Description:		This module contains memory management
						functions for the creation, storage and
						release of the parameter store.

*********************************************************************-*/

#include <string.h>
#if defined(__STDC__) || defined(__cplusplus)
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif

#include "atph.h"
#include "atpsysex.h"

#ifdef DEBUG
static char *__Atp_Local_FileName__ = __FILE__;
#endif

/*
	Local definitions
 */
#define EOP_APPEND		'A'
#define EOP_OVERWRITE	'O'

#define DEFAULT_STORE_NELEM		8
#define DEFAULT_DATA_BUFSIZE	32
#define SPAREROOM				1

/*
	Static variables
 */
static ParmStoreMemMgtNode		*ParmStoreBuilder;
static ParmStoreMemMgtNode		**ParmStoreStack = NULL;
static int						CurrParmStoreStackIndex	= -1;
static	ParmStoreInfo			ZeroedParmRec = { 0 };
static int						LargestDataStoreltem = sizeof (Atp_ChoiceDescriptor);

/*
	Local functions
 */
static void WriteFloatingEOPmarker
				_PROTO_((ParmStoreMemMgtNode *parmstore_builder, char writeMode));
static void ReleaseStore _PROTO_((ParmStoreInfo *CtrlStore, void *DataStore));
static void CheckStoreMemory _PROTO_((int MinDataStoreSize));

/*+*******************************************************************

	Function Name:		Atp_CreateNewParmStore

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Create a new parameter store.	Initialise
						memory and returns its address.

	Modifications:
		Who			When				Description
	----------	-------------	---------------------------
	Alwyn Teh	9 July 1992		Initial Creation

********************************************************************-*/
void *
Atp_CreateNewParmStore()
{
	ParmStoreMemMgtNode *parmstore_ptr;

	parmstore_ptr = (ParmStoreMemMgtNode *)
							CALLOC(1, sizeof(ParmStoreMemMgtNode), NULL);

	parmstore_ptr->CtrlStore =
							(ParmStoreInfo *)
							CALLOC(	DEFAULT_STORE_NELEM + SPAREROOM,
									sizeof(ParmStoreInfo), NULL );

	parmstore_ptr->DataStore =
							(Atp_ByteType *)
							CALLOC(	DEFAULT_DATA_BUFSIZE + SPAREROOM,
									sizeof(Atp_ByteType), NULL );

	parmstore_ptr->CurrCtrlPtr = parmstore_ptr->CtrlStore;
	parmstore_ptr->CurrDataPtr = parmstore_ptr->DataStore;

	parmstore_ptr->CurrCtrlIndex = 0;

	parmstore_ptr->EndOfCtrlPtr = &(parmstore_ptr->CtrlStore)
									[DEFAULT_STORE_NELEM - 1];

	parmstore_ptr->EndOfDataPtr =
							(Atp_ByteType *)
							 &((Atp_ByteType *)(parmstore_ptr->DataStore))
							 	 	 	 	 	 [DEFAULT_DATA_BUFSIZE - 1];

	parmstore_ptr->SizeOfCtrlStore =
							sizeof(ParmStoreInfo) * DEFAULT_STORE_NELEM;

	parmstore_ptr->SizeOfDataStore = DEFAULT_DATA_BUFSIZE;

	parmstore_ptr->prevNode = parmstore_ptr->nextNode = NULL;

	ParmStoreBuilder = parmstore_ptr;

	WriteFloatingEOPmarker(ParmStoreBuilder, EOP_OVERWRITE);

	return parmstore_ptr;
}

/*+*******************************************************************

	Function Name:		WriteFloatingEOPmarker

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom

	Description:		Insert floating EOP marker at appropriate
						control parmstore because you don't know if
						and where an operation will fail and at
						which nesting level of which ever branch
						...... and it's stupid trying to go back
						inserting EOPs at all levels upon failure.
						This is necessary because ReleaseStore()
						relies on EOP to know where to stop scanning
						at all nested levels.
						Use EOP_APPEND for repeat blocks ... etc.
						because parsing may fail at a nested level
						and you won't be coming back to write in an
						EOP. (NOTE: pointers NOT incremented for
						repeat blocks ... etc. until the end of
						the block and you know how many instances
						have been found.)
						Use EOP_OVERWRITE in StoreParm() after
						having stored a parameter and incremented
						the pointers because you don't know if the
						next parameter will be read in alright.

	Modifications:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	9 July 1992		Initial Creation
	Alwyn Teh	4 October 1993	Zero out all fields except
								parmcode ATP_EOP.
	Alwyn Teh	21 January 1994	Add new field for any
								parmstore builder to use.

*******************************************************************-*/
static ParmStoreInfo EOP_marker = { "EOP", 0, ATP_EOP };
#if defined(__STDC__) || defined(__cplusplus)
static void WriteFloatingEOPmarker
(
	ParmStoreMemMgtNode *parmstore_builder,
	char writeMode
)
#else
static void
WriteFloatingEOPmarker(parmstore_builder, writeMode)
	ParmStoreMemMgtNode *parmstore_builder;
	char writeMode;
#endif
{
	switch(writeMode) {
		case EOP_OVERWRITE : {
			*parmstore_builder->CurrCtrlPtr = EOP_marker;
			break;
		}
		case EOP_APPEND : {
			*(parmstore_builder->CurrCtrlPtr + 1) = EOP_marker;
			break;
		}
		default : break;
	}
}

/*+*******************************************************************

	Function Name:		Atp_StoreParm

	Copyright:			BNR Europe Limited, 1992, 1993, 1995
						Bell-Northern Research
						Northern Telecom

	Description:		Generic function to store parameter values
						in the parameter store.

	Modifications:
		Who			When				Description
	----------	--------------	-----------------------------
	Alwyn Teh	27 July 1992	Initial Creation
	Alwyn Teh	20 May 1993		Store databytes as
								Atp_DataDescriptor and NOT as
								(Atp_ByteType *). However, the
								count field in databytes is
								unchanged.
	Alwyn Teh	7 July 1993		Use AtpIncrPtr(ptr,type) because
								cast is not lvalue required by ++
	Alwyn Teh	4 October 1993	Zero out parm_rec using ZeroedParmRec
								instead of memset() - faster.
	Alwyn Teh	23 Dec 1993		Save boolean string
	Alwyn Teh	27 March 1995	Add BCD digits parameter type.

*******************************************************************-*/
#if defined (__STDC__) || defined(__cplusplus)
void Atp_StoreParm(ParmDefEntry *ParmDefEntryPtr, int isDefault, ...)
#else
void
Atp_StoreParm(ParmDefEntryPtr, isDefault, va_alist)
	ParmDefEntry *ParmDefEntryPtr;
	int isDefault;
	va_dcl
#endif
{
	va_list			argPtr;

	ParmStoreInfo	parm_rec;
	Atp_ParmCode	parmcode;

#if defined (__STDC__) || defined (__cplusplus)
	va_start(argPtr, isDefault);
#else
	va_start(argPtr);
#endif

	/* Zero out parm_rec to get rid of garbage. */
	parm_rec = ZeroedParmRec;

	/* Initialise other parm_rec fields. */
	parm_rec.is_default	= isDefault;
	parm_rec.parmName	= ParmDefEntryPtr->Name;
	parm_rec.parmcode	= parmcode = ParmDefEntryPtr->parmcode;

	parm_rec.ParmDefEntryPtr = ParmDefEntryPtr;

	parm_rec.upLevel	= NULL;
	parm_rec.downLevel	= NULL;

	/* Check amount of memory remaining before writing to it. */
	CheckStoreMemory(0);

	/*
		Point to current location in data store where parameter
		value will be written.
	 */
	parm_rec.parmValue = ParmStoreBuilder->CurrDataPtr;

	/*
		Get the parameter value and write it in the data store.
		Then, advance the data store pointer.
	 */
	switch(Atp_PARMCODE(parmcode)) {
		case ATP_NUM : {
				*(Atp_NumType *)ParmStoreBuilder->CurrDataPtr =
										va_arg(argPtr, Atp_NumType);
				AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_NumType);
				break;
		}
		case ATP_UNS_NUM : {
				*(Atp_UnsNumType *)ParmStoreBuilder->CurrDataPtr =
										va_arg(argPtr, Atp_UnsNumType);
				AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_UnsNumType);
				break;
		}
		case ATP_REAL : {
				*(Atp_RealType *)ParmStoreBuilder->CurrDataPtr =
										va_arg(argPtr, Atp_RealType);
				AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_RealType);
				break;
		}
		case ATP_STR : {
				*((char **)ParmStoreBuilder->CurrDataPtr) =
										va_arg(argPtr, char *);
				AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, char *);
				break;
		}
		case ATP_BOOL : {
				/*
					Boolean argument promotion to int during function
					call. See manpage for stdarg.
				 */
				int  intSizeBool = va_arg(argPtr, int);
				char *boolean_string = va_arg(argPtr, char *);

				*(Atp_BoolType *)ParmStoreBuilder->CurrDataPtr =
										(intSizeBool == 1) ? TRUE : FALSE;
				parm_rec.DataPointer = boolean_string;
				AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_BoolType);

				break;
		}
		case ATP_BCD :
		case ATP_DATA : {
				Atp_DataDescriptor dataBytes; /* or BCD digits */

				dataBytes = va_arg(argPtr, Atp_DataDescriptor);
				parm_rec.TypeDependentInfo.parmSize = dataBytes.count;

				*((Atp_DataDescriptor *)ParmStoreBuilder->CurrDataPtr) = dataBytes;
				AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_DataDescriptor);

				break;
		}
		case ATP_KEYS : {
				Atp_UnsNumType	KeyIndex = 0;
				char			*KeywordString = NULL;

				/* KeyValue */
				*(Atp_NumType *)ParmStoreBuilder->CurrDataPtr =
										va_arg(argPtr, Atp_NumType);
				AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_NumType);

				KeyIndex		= va_arg(argPtr, Atp_UnsNumType);
				KeywordString	= va_arg(argPtr, char *);

				parm_rec.TypeDependentInfo.KeywordCaseIdx = KeyIndex;
				parm_rec.DataPointer = KeywordString;

				break;
		}
		case ATP_NULL : {
				*(Atp_NumType *)ParmStoreBuilder->CurrDataPtr =
										va_arg(argPtr, Atp_NumType);
				AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_NumType);
				break;
		}
		case ATP_COM : {
				/* Not implemented - for Phase 2 development */
				break;
		}
		case ATP_EOP : {
				break;
		}
		default : break;
	}

	/* Write the ParmStoreInfo record to the control structure. */
	*ParmStoreBuilder->CurrCtrlPtr = parm_rec;

	/* Increment control pointer and index. */
	ParmStoreBuilder->CurrCtrlPtr++;
	ParmStoreBuilder->CurrCtrlIndex++;

	/* Terminate here in case no further parameters. */
	WriteFloatingEOPmarker(ParmStoreBuilder, EOP_OVERWRITE);

	va_end(argPtr);
}

/*+*******************************************************************

	Function Name:		Atp_StoreConstructInfo

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Store and update construct information in
						the parmstore.

	Notes:				Sensitive and delicate;	well balanced;
						doesn't like being mucked about if you don't
						know what you're doing; independent of
						multiple parmstores for nested commands; the
						memory map is in my head; could do with a
						rewrite to simplify its operations but
						haven’t got time.

	Modifications:
		Who			When				Description
	-----------	--------------	-----------------------------------
	Alwyn Teh	27 July 1992	Initial Creation
	Alwyn Teh	20 May 1993		Atp_DataDescriptor field RptCount
								changes to count
	Alwyn Teh	27	June 1993	Protect DataPointer field of
								ParmDefEntry for BEGIN_CASE
								as it's not used normally
								except in the built-in "help"
								command where each CASE'S
								DataPointer points to table
								of command names/descs. However,
								this is just precautionary as
								this function doesn't get called
								for the CASE construct anyway!
	Alwyn Teh	7 July 1993		Use AtpIncrPtr(ptr,type) because
								cast is not lvalue required by ++
	Alwyn Teh	13 July 1993	Changed 4th argument for CHOICE
								from index to ChoiceDescPtr
	Alwyn Teh	29 Sept 1993	Add new CaseValue field to Choice
								Descriptor
	Alwyn Teh	4 October 1993	Zero out parm_rec using ZeroedParmRec
	Alwyn Teh	5 October 1993	Fix core dump bug due to increased
								size of Atp_ChoiceDescriptor, causing
								insufficient data store and hence
								store trampling. Also, should call
								CheckStoreMemory before updating
								parm_rec and writing to store.
	Alwyn Teh	24 Dec 1993		Return location of actual parmstore
								Atp_ChoiceDescriptor by means of
								an extra field &Atp_ChoiceDescriptor
								for the CHOICE construct.
	Alwyn Teh	21 January 1994	Eliminate a memory gobbler whereby
								the downlevel parmstore is not freed
								when no parameter values are written,
								such as in a nested empty CASE within a
								CHOICE. Fix is to ensure the DataStore
								address is written in the CtrlStore's
								parmValue field so that ReleaseStore
								will free the malloced space.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_StoreConstructInfo(ParmDefEntry * ParmDefEntryPtr,
							int _parmcode,
							int isDefault, ...)
#else
void
Atp_StoreConstructInfo( ParmDefEntryPtr, _parmcode, isDefault, va_alist )
	ParmDefEntry * ParmDefEntryPtr;
	int _parmcode;
	int isDefault;
	va_dcl
#endif
{
	va_list					argPtr;
	Atp_ParmCode			parmcode = (Atp_ParmCode)_parmcode;

	/* Repeat block attributes. */
	Atp_UnsNumType			RptTimes = 0;
	Atp_BoolType			isUserZeroInst = FALSE;
	int						terminatorSize = 0;
	Atp_DataDescriptor		*RptBlkDescPtr;	/* to be filled in and returned to caller */

	/* Choice attribute(s) */
	Atp_UnsNumType			ChoiceIdx = 0;
	Atp_UnsNumType			NoOfChoiceParms = 0;

	/* Parmstore info stuff */
	ParmStoreInfo			parm_rec;
	Atp_DataDescriptor		tmpDataDesc;
	Atp_ChoiceDescriptor	tmpChoiceDesc, *ChoiceDescPtr, **ChoiceDescPtrPtr;
	void					*tmpListPtr	= NULL;
	ParmStoreMemMgtNode		*newBuilder	= NULL;

#if defined(__STDC__) || defined(__cplusplus)
	va_start(argPtr, isDefault);
#else
	va_start(argPtr);
#endif

	/* Initialize parm_rec. */
	parm_rec = ZeroedParmRec;

	switch(Atp_PARMCODE(parmcode)) {
		case ATP_BRP :
		case ATP_ERP : {
			RptTimes		= va_arg(argPtr, Atp_UnsNumType);
			isUserZeroInst	= (Atp_BoolType)va_arg(argPtr, int);
			terminatorSize	= va_arg(argPtr, int);
			RptBlkDescPtr	= va_arg(argPtr, Atp_DataDescriptor *);
			break;
		}
		case ATP_BCH : {
			ChoiceDescPtr	= va_arg(argPtr, Atp_ChoiceDescriptor *);
			ChoiceDescPtrPtr= va_arg(argPtr, Atp_ChoiceDescriptor **);
			ChoiceIdx		= ChoiceDescPtr->CaseIndex;
			NoOfChoiceParms	= va_arg (argPtr, Atp_UnsNumType);
			terminatorSize	= sizeof(Atp_ChoiceDescriptor);
			break;
		}
		case ATP_ECH : break;
		case ATP_BLS : {
			terminatorSize = sizeof(void *);
			break;
		}
		case ATP_ELS : break;
		default : return;
	}

	if (isAtpBeginConstruct(parmcode)) {
	  /*
	   *	Prepare ParmStoreInfo record for initial level control.
	   */
	  /* Write in the name and type of the repeat block. */
	  parm_rec.parmName = ParmDefEntryPtr->Name;
	  parm_rec.parmcode = parmcode;

	  /* Miscellaneous bits and pieces. */
	  parm_rec.is_default			= isDefault;
	  parm_rec.ParmDefEntryPtr	= ParmDefEntryPtr;

	  /*
		Initialize type-dependent info to whatever it happens
		to be.
	   */
	  switch (Atp_PARMCODE(parmcode)) {
		case ATP_BRP:
			parm_rec.TypeDependentInfo.RptBlockCount = RptTimes;
			break;
		case ATP_BCH:
			parm_rec.TypeDependentInfo.ChoiceCaseIdx = ChoiceIdx;
			break;
		case ATP_BLS:
			break;
		default:
			break;
	  }

	  /* Check amount of memory remaining before writing to it. */
	  CheckStoreMemory(terminatorSize);

	  /*
		Point to current location in data store where construct
		value will be written.
		e.g. Atp_DataDescriptor or Atp_ChoiceDescriptor or void *
	  */
	  parm_rec.parmValue = ParmStoreBuilder->CurrDataPtr;

	  /* Initialize level pointers for nesting. */
	  parm_rec.upLevel	= NULL; /* not used at present */
	  parm_rec.downLevel	= NULL; /* NOTE: Optional construct has
										 no downLevel pointer */

	  /* Set initial value for data or choice descriptor. */
	  switch (Atp_PARMCODE(parmcode)) {
		case ATP_BRP : {
			tmpDataDesc.count	= RptTimes;
			tmpDataDesc.data	= NULL;
			break;
		}
		case ATP_BCH : {
			tmpChoiceDesc = *ChoiceDescPtr;
			tmpChoiceDesc.data = NULL;
			break;
		}
		case ATP_BLS : {
			tmpListPtr = NULL;
			break;
		}
		default : break;
	  }

	  /*
		If use of default value indicated by the omission of
		user value, then get the default from the parmdef
		entry.

		REPEAT BLOCKS:
			If user typed in "command (	then no input is
			available for parsing, so we shouldn't waste time
			and memory resources going into the last else branch
			here.
	   */
	  if (isDefault) {
	    /*
			Extract default if there really is one available in
			the parmdef entry.
	    */
	    if (ParmDefEntryPtr->DataPointer != NULL) {
		  switch (Atp_PARMCODE(parmcode)) {
			case ATP_BRP:
				tmpDataDesc = *((Atp_DataDescriptor *)
								(ParmDefEntryPtr->DataPointer));
				break;
			case ATP_BCH:
				tmpChoiceDesc = *((Atp_ChoiceDescriptor *)
								  (ParmDefEntryPtr->DataPointer));
				break;
			case ATP_BLS:
				tmpListPtr = (void *)(ParmDefEntryPtr->DataPointer);
				break;
			default : break;
		  }
	    }
	  }
	  else
	  if ((Atp_PARMCODE(parmcode) == ATP_BRP) && (isUserZeroInst)) {
		tmpDataDesc.count = 0;
		tmpDataDesc.data = NULL; /* Don't even bother supplying
								    room for a terminator, it's
								    too much hassle! */
	  }
	  else {
	    int NelemsUsed = 0;
	    int DataBufSizeUsed = 0;

	    /*
	     * Extend linked list of ParmStoreBuilders.
	     */
	    newBuilder = (ParmStoreMemMgtNode *)
							CALLOC(1, sizeof(ParmStoreMemMgtNode), NULL);

	    ParmStoreBuilder->nextNode = newBuilder;

	    newBuilder->prevNode = ParmStoreBuilder;
	    newBuilder->nextNode = NULL;

	    /*
			Get some room for the repeat/choice block
			ParmStoreInfo structures to go in. To economize,
			choice only gets as much room as it needs.
			Initialize ParmStoreBuilder fields for next level.
	     */
	    NelemsUsed = (Atp_PARMCODE(parmcode) == ATP_BCH) ?
						NoOfChoiceParms : DEFAULT_STORE_NELEM;
	    parm_rec.downLevel =
	    newBuilder->CtrlStore =
	    newBuilder->CurrCtrlPtr =
				(ParmStoreInfo *)CALLOC(NelemsUsed + SPAREROOM,
										sizeof(ParmStoreInfo),
										NULL);

	    newBuilder->CurrCtrlIndex = 0;

	    newBuilder->EndOfCtrlPtr = &((newBuilder->CtrlStore)
								[NelemsUsed - SPAREROOM]);

	    newBuilder->SizeOfCtrlStore =
						NelemsUsed * sizeof(ParmStoreInfo);
						/* not counting spareroom */

	    /*
			Get some room for the repeat/choice block parameters
			(the actual data) to go in. To economize, choice
			only gets as much room as it needs. Initialize
			ParmStoreBuilder fields for next level.
	     */
	    DataBufSizeUsed = ((Atp_PARMCODE(parmcode) == ATP_BCH) &&
						   (NoOfChoiceParms == 1) ) ?
						   LargestDataStoreltem : DEFAULT_DATA_BUFSIZE;

	    tmpDataDesc.data =
	    tmpChoiceDesc.data =
	    tmpListPtr =
	    newBuilder->DataStore =
	    newBuilder->CurrDataPtr =
					(void *)CALLOC(	DataBufSizeUsed + SPAREROOM,
									sizeof(Atp_ByteType), NULL );

	    /*
			Stick it in the parmdef entry for possible later use,
			but ONLY if it's not an optional.
	     */
	    if (ParmDefEntryPtr->parmcode != ATP_BCS &&
		    !AtpParmIsOptional(ParmDefEntryPtr->parmcode) &&
		    (ParmDefEntryPtr->DataPointer == NULL)) {
	      ParmDefEntryPtr->DataPointer = newBuilder->DataStore;
	    }

	    newBuilder->EndOfDataPtr =
						&(((Atp_ByteType *)(newBuilder->DataStore))
						[DataBufSizeUsed - SPAREROOM]);

	    newBuilder->SizeOfDataStore = DataBufSizeUsed;
									  /* not counting spareroom */

	    /* Initialize 1st CtrlStore field to point to its DataStore. */
	    WriteFloatingEOPmarker(newBuilder, EOP_OVERWRITE); /* do this 1st */
	    newBuilder->CtrlStore[0].parmValue = newBuilder->DataStore;

	  } /* else */

	  /*
	   * Write the temporary records into the respective structures.
	   */
	  /* Write the ParmStoreInfo in. */
	  *ParmStoreBuilder->CurrCtrlPtr = parm_rec;

	  /* Write the descriptor in the data store. */
	  switch (Atp_PARMCODE(parmcode)) {
		case ATP_BRP:
			*(Atp_DataDescriptor *)
					ParmStoreBuilder->CurrDataPtr = tmpDataDesc;
			break;
		case ATP_BCH:
			*(Atp_ChoiceDescriptor *)
					ParmStoreBuilder->CurrDataPtr = tmpChoiceDesc;

			/* Return location of actual parmstore Atp_ChoiceDescriptor */
			if (ChoiceDescPtrPtr != NULL)
			  *ChoiceDescPtrPtr = (Atp_ChoiceDescriptor *)
			  	  	  	  	  	  	  	  ParmStoreBuilder->CurrDataPtr;
			break;
		case ATP_BLS:
			*(void **) ParmStoreBuilder->CurrDataPtr = tmpListPtr;
			break;
		default:
		break;
	  }

	  WriteFloatingEOPmarker(ParmStoreBuilder, EOP_APPEND);

	  if (isDefault || ((Atp_PARMCODE(parmcode) == ATP_BRP) && isUserZeroInst))
	  {
	    /*
			Increment current pointers ’cause finished with them.
			Won't be returning in a closing END_REPEAT call.
	    */
	    ParmStoreBuilder->CurrCtrlPtr++;
	    ParmStoreBuilder->CurrCtrlIndex++;

	    switch (Atp_PARMCODE(parmcode)) {
		  case ATP_BRP:
			AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_DataDescriptor);
			break;
		  case ATP_BCH:
			AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_ChoiceDescriptor);
			break;
		  case ATP_BLS:
			AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, void *);
			break;
		  default: break;
	    }
	  }
	  else {
	    /*
			After all the relevant pointers .... and so on have
			been set up, we are ready to go into the next level.
	    */
		  ParmStoreBuilder = ParmStoreBuilder->nextNode;
	  }
	}
	else
	if (isAtpEndConstruct(parmcode)) {
	  if (isDefault || ((Atp_PARMCODE(parmcode) == ATP_ERP) && isUserZeroInst))
	  {
		/* Shouldn't have been here in the first place. */
		return;
	  }

	  /*
	   *	Check amount of memory remaining and make sure that
	   *	there's at least terminatorSize bytes at the end of
	   *	data store.
	   */
	  CheckStoreMemory(terminatorSize);

	  /*
			Go back to previous builder and get rid of the present
			one.
	   */
	  ParmStoreBuilder = ParmStoreBuilder->prevNode;

	  if (ParmStoreBuilder->nextNode != NULL) {
		FREE(ParmStoreBuilder->nextNode);
		ParmStoreBuilder->nextNode = NULL;
	  }

	  /* Write in number of instances of repeat block found. */
	  if (Atp_PARMCODE(parmcode) == ATP_ERP) {
		(ParmStoreBuilder->CurrCtrlPtr)->TypeDependentInfo.RptBlockCount =
				((Atp_DataDescriptor *)ParmStoreBuilder->CurrDataPtr)->count = RptTimes;

		/*
			Read in the parmstore repeat block descriptor, and
			return value to caller.
		 */
		*RptBlkDescPtr = *(Atp_DataDescriptor *)ParmStoreBuilder->CurrDataPtr;
	  }

	  /* Increment current pointers 'cause finished with them. */
	  ParmStoreBuilder->CurrCtrlPtr++;
	  ParmStoreBuilder->CurrCtrlIndex++;
	  switch (Atp_PARMCODE(parmcode)) {
		case ATP_ERP:
			AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_DataDescriptor);
			break;
		case ATP_ECH:
			AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, Atp_ChoiceDescriptor);
			break;
		case ATP_ELS:
			AtpIncrPtr(ParmStoreBuilder->CurrDataPtr, void *);
			break;
		default: break;
	  }

	  WriteFloatingEOPmarker(ParmStoreBuilder, EOP_OVERWRITE);
	  	  	  /* Note: not really necessary but being cautious here */
	}
	else {
	  /* ATP program bug encountered - wrong parmcode! */
	  char *errmsg;
	  Atp_ShowErrorLocation();
	  errmsg = Atp_MakeErrorMsg(ERRLOC, ATP_ERRCODE_NON_CONSTRUCT_PARMCODE);
	  Atp_HyperSpace(errmsg);
	}

	return;
}

/*+*******************************************************************

	Function Name:		CheckStoreMemory

	Copyright:			BNR Europe Limited, 1992, 1993
						Bell-Northern Research
						Northern Telecom

	Description:		Manages storage for	the current	level
						parameter stores.

						If the current buffer is full, a new and
						bigger piece of dynamic memory is allocated.

						Notes:	Don't touch! I spent ages on this. If you
						upset it, it could core dump on you when you
						least expect it.

	Modifications:
		Who			When				Description
	----------	--------------	------------------------------
	Alwyn Teh	27 July 1992	Initial Creation
	Alwyn Teh	5 October 1993	Increase terminatorSize	from
								Atp_RealType to LargestDataStoreltem,
								this should prevent store tramplers.

********************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void CheckStoreMemory(int MinDataStoreSize )
#else
static void
CheckStoreMemory(MinDataStoreSize)
	int MinDataStoreSize;
#endif
{
#define MARGIN 2

	void	*oldDataStore, *newDataStore;
	int		delta,
			oldSize, newSize,
			oldNelem, newNelem,
			terminatorSize;

	register int idx;

	/*
	 * ----------------------------------------
	 *		Check parameter control store.
	 * ----------------------------------------
	 */
	oldSize = ParmStoreBuilder->SizeOfCtrlStore;
	oldNelem = oldSize / sizeof(ParmStoreInfo);

	/*
		MARGIN must always be at least 2 to be able to write in
		the parameter and the floating EOP marker.
	 */
	if (ParmStoreBuilder->CurrCtrlIndex >= (oldNelem - MARGIN)) {

	  delta = ParmStoreBuilder->CurrCtrlPtr -
			  ParmStoreBuilder->CtrlStore; /* same value as CurrCtrlIndex */

	  newNelem = oldNelem * 2;
	  newSize = newNelem * sizeof(ParmStoreInfo);

	  /*
	  ---------> Increase control store size <--------------
		NOTE that extra memory won't be zeroed for you by
		realloc.

		e.g. Don't depend on the end of ParmStoreInfo to be
		zero.
	  */
#ifdef DEBUG
	  if (Atp_DebugMode) {
		(void) fprintf(stderr,
						"realloc Ctrl store from %d to %d+1 elements.\n",
						oldNelem, newNelem);
		(void) fflush(stderr);
	  }
#endif

	  ParmStoreBuilder->CtrlStore = (ParmStoreInfo *)
									REALLOC(ParmStoreBuilder->CtrlStore,
									((newNelem+SPAREROOM) * sizeof(ParmStoreInfo)),
									NULL);

	  /* Update downLevel pointer of control store upstairs. */
	  if (ParmStoreBuilder->prevNode != NULL) {
		ParmStoreBuilder->prevNode->CurrCtrlPtr->downLevel = ParmStoreBuilder->CtrlStore;
	  }

	  ParmStoreBuilder->CurrCtrlPtr		= ParmStoreBuilder->CtrlStore + delta;
	  ParmStoreBuilder->EndOfCtrlPtr	= &ParmStoreBuilder->CtrlStore[newNelem];
	  ParmStoreBuilder->SizeOfCtrlStore	= newSize;
	}

	/*
	 * ----------------------------------------
	 *		Check parameter data store.
	 * ----------------------------------------
	 */

	terminatorSize = (MinDataStoreSize != 0) ? MinDataStoreSize : LargestDataStoreltem;

	if (((Atp_ByteType *)ParmStoreBuilder->EndOfDataPtr -
		 (Atp_ByteType *)ParmStoreBuilder->CurrDataPtr)
							<=
					  terminatorSize)
	{
	  delta = (Atp_ByteType *)(ParmStoreBuilder->CurrDataPtr) -
			  (Atp_ByteType *)(ParmStoreBuilder->DataStore);

	  oldSize = ParmStoreBuilder->SizeOfDataStore;
	  newSize = (oldSize * 2) * sizeof(Atp_ByteType);

	  oldDataStore = ParmStoreBuilder->DataStore;

	  /*
		----------> Increase data store size <----------
		NOTE that extra memory won't be zeroed for you by
		realloc.

		e.g. Don't depend on the end of ParmStoreInfo to be
		zero.
	   */
#ifdef DEBUG
	  if (Atp_DebugMode) {
		(void) fprintf(	stderr,
						"realloc data store from %d to %d+%d bytes.\n",
						oldSize, newSize, terminatorsize );
		(void) fflush(stderr);
	  }
#endif

	  newDataStore =
	  ParmStoreBuilder->DataStore =
					(void *)REALLOC(ParmStoreBuilder->DataStore,
									((newSize + terminatorSize) *
									sizeof(Atp_ByteType)),
									NULL);

	  /*
			Update down-level pointer to data store if store
			relocated by realloc.
	   */
	  if ((oldDataStore != newDataStore) &&
		  (ParmStoreBuilder->prevNode != NULL))
	  {
		switch(Atp_PARMCODE(ParmStoreBuilder->prevNode->CurrCtrlPtr->parmcode))
		{
			case ATP_BRP:
			{
				((Atp_DataDescriptor *)
				(ParmStoreBuilder->prevNode->CurrDataPtr))->data =
						ParmStoreBuilder->DataStore;

				if (!AtpParmIsOptional(ParmStoreBuilder->prevNode->CurrCtrlPtr->parmcode))
				{
				  ParmStoreBuilder->prevNode->CurrCtrlPtr->ParmDefEntryPtr->DataPointer =
						  ParmStoreBuilder->DataStore;
				}
				break;
			}
			case ATP_BCH:
			{
				((Atp_ChoiceDescriptor *)
				(ParmStoreBuilder->prevNode->CurrDataPtr))->data =
						ParmStoreBuilder->DataStore;

				if (!AtpParmIsOptional(ParmStoreBuilder->prevNode->CurrCtrlPtr->parmcode))
				{
				  ParmStoreBuilder->prevNode->CurrCtrlPtr->ParmDefEntryPtr->DataPointer =
						  ParmStoreBuilder->DataStore;
				}
				break;
			}
			case ATP_BLS:
			{
				*(void **)ParmStoreBuilder->prevNode->CurrDataPtr =
								ParmStoreBuilder->DataStore;

				if (!AtpParmIsOptional(ParmStoreBuilder->prevNode->CurrCtrlPtr->parmcode))
				{
				  ParmStoreBuilder->prevNode->CurrCtrlPtr->ParmDefEntryPtr->DataPointer =
						  ParmStoreBuilder->DataStore;
				}
				break;
			}
			default : break;
		} /* switch */
	  } /* if */

	  /*
	   * -----------------------------------------------------------
	   *	Update ALL parmstore pointers to parameters from the
	   *	control store !!!
	   * -----------------------------------------------------------
	   */
	  if (oldDataStore != newDataStore)
	  {
		for (idx = 0; idx <= ParmStoreBuilder->CurrCtrlIndex; idx++)
		{
		   if (ParmStoreBuilder->CtrlStore[idx].parmValue != NULL)
		   {
			 ParmStoreBuilder->CtrlStore[idx].parmValue =
									((Atp_ByteType *)
									ParmStoreBuilder->CtrlStore[idx].parmValue -
									(Atp_ByteType *) oldDataStore)
									+ (Atp_ByteType *) newDataStore;
		   }
		}
	  }

	  /* Update other control fields. */
	  ParmStoreBuilder->CurrDataPtr =
			  (Atp_ByteType *)(ParmStoreBuilder->DataStore) + delta;
	  ParmStoreBuilder->EndOfDataPtr =
			  &((Atp_ByteType *)
			    (ParmStoreBuilder->DataStore))[newSize-1];
	  ParmStoreBuilder->SizeOfDataStore = newSize;
	}

#undef MARGIN
}

/*+*******************************************************************

	Function Name:		Atp_FreeParmStore

	Copyright:			BNR Europe Limited, 1992-1994
						Bell-Northern Research
						Northern Telecom

	Description:		Top level controller of the parmstore
						liberation movement.

	Modifications:
		Who			When				Description
	----------	---------------	-----------------------------
	Alwyn Teh	9 July 1992		Initial Creation
	Alwyn Teh	20 January 1994	Free linked list of
								ParmStoreBuilders after use.

********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
void Atp_FreeParmStore(void *_parmStore)
#else
void
Atp_FreeParmStore(_parmStore)
	void *_parmStore;
#endif
{
	ParmStoreMemMgtNode *safecopy = NULL;
	ParmStoreMemMgtNode *parmStore = (ParmStoreMemMgtNode *)_parmStore;

	if (parmStore == NULL)
	  return;

	ReleaseStore(parmStore->CtrlStore, parmStore->DataStore);

	/*
	 *	After the parmstore, ParmStoreBuilders also need to be freed.
	 *	Start from the top. An error condition could have left lots of
	 *	ParmStoreBuilders lying around, especially in a nested parmdef.
	 */
	ParmStoreBuilder = parmStore;

	/* Find the last ParmStoreBuilder. */
	while (ParmStoreBuilder->nextNode != NULL)
		ParmStoreBuilder = ParmStoreBuilder->nextNode;

	/* Now delete backwards. */
	while (ParmStoreBuilder != NULL) {
		safecopy = ParmStoreBuilder; /* save copy of current one */

		ParmStoreBuilder = ParmStoreBuilder->prevNode; /* move backwards */

		/* Delete saved ParmStoreBuilder */
		FREE(safecopy);
		safecopy = NULL;
	}

	/* Free top-level node */
	if (ParmStoreBuilder != NULL)
	  FREE(ParmStoreBuilder);

	return;
}

/*+******************************************************************

	Function Name:		ReleaseStore

	Copyright:			BNR Europe Limited,	1992-1995
						Bell-Northern Research
						Northern Telecom

	Description:		Release any dynamic	store for	strings, data
						bytes and repeat blocks. ReleaseStore()
						gets called recursively for repeat blocks,
						choice and list branches.

	Notes:				If you're not careful, you could get into an
						infinite loop and then crash in the
						wilderness.

	Modifications:
		Who			When				Description
	-----------	---------------	--------------------------------
	Alwyn Teh	27 July 1992	Initial Creation
	Alwyn Teh	18 May 1993		Don't free string if it's a
								default value because it's
								not my property and is
								probably static too.
	Alwyn Teh	20 May 1993		DataBytesCount changes to
								Atp_DataBytesCount
	Alwyn Teh	21 May 1993		Databytes stored as
								Atp_DataDescriptor, so release
								appropriately.
	Alwyn Teh	13 July 1993	New CaseName field for
								Atp_ChoiceDescriptor needs to
								be freed after use.
	Alwyn Teh	23 Dec 1993		Free boolean string if stored
	Alwyn Teh	21 January 1994	Use malloced string always, so
								needs freeing everytime.
	Alwyn Teh	27 March 1995	Add BCD digits parameter type.

*******************************************************************-*/
#if defined(__STDC__) || defined(__cplusplus)
static void ReleaseStore(ParmStoreInfo *CtrlStore, void *DataStore)
#else
static void
ReleaseStore(CtrlStore, DataStore)
	ParmStoreInfo	*CtrlStore;
	void			*DataStore;
#endif
{
	register int	x;
	register void	*voidPtr;
	Atp_ParmCode	parmcode;

	if (CtrlStore == NULL) {
	  char *errmsg;

	  Atp_ShowErrorLocation();
	  errmsg = Atp_MakeErrorMsg(ERRLOC, ATP_ERRCODE_ILLEGAL_FREE_NULL_CTRLSTORE);
	  Atp_HyperSpace(errmsg);
	}

	if (DataStore == NULL) {
	  /*
			DataStore is NULL when called self-recursively and
			when a user input parameter didn't get read in.
			Therefore, there's no point continuing. No error
			messages necessary here.
	   */
	  return;
	}

	/*
	 *	Scan control structure searching for memory to deallocate.
	 *	Each control record's parmstore points to data in data store.
	 */

	/*
	 *	Since termination of scan relies on finding a ATP_EOP,
	 *	therefore, must make sure there is always a ATP_EOP
	 *	marker under all circumstances.
	 */
	for (x = 0; (CtrlStore[x].parmcode != ATP_EOP); x++)
	{
		switch (parmcode = Atp_PARMCODE(CtrlStore[x].parmcode))
		{
			case ATP_STR:
			{
				if ((voidPtr = *(char **)CtrlStore[x].parmValue) != NULL)
				{
				  FREE(voidPtr);
				  *(char **)CtrlStore[x].parmValue = NULL;
				  	  	  	  /* this prevents accidental reuse */
				}
				break;
			}
			case ATP_BCD:
			case ATP_DATA:
			{
				Atp_DataDescriptor desc; /* Databytes or BCD digits */

				desc = *(Atp_DataDescriptor *)CtrlStore[x].parmValue;

				/*
					NOTE: First field is number of bytes or BCD
					digits(nibbles), but parmStore actually points to
					first byte of data.
				 */
				voidPtr = (desc.data == NULL) ? NULL : &Atp_DataBytesCount(desc.data);

				if (voidPtr != NULL)
				{
				  FREE(voidPtr);
				  ((Atp_DataDescriptor *)CtrlStore[x].parmValue)->data = NULL;
				  	  	  	  	  	  	  	  /* this prevents accidental reuse */
				}
				break;
			}
			case ATP_KEYS:
			case ATP_BOOL:
			{
				/* Free spare copy of keyword string or boolean string. */
				if ((voidPtr = (char *)CtrlStore[x].DataPointer) != NULL)
				{
				  FREE(voidPtr);
				}
				break;
			}
			case ATP_BLS:
			case ATP_BRP:
			case ATP_BCH:
			{
				/*
				 *	If CHOICE construct, free CaseName and CaseDesc
				 *	in Atp_ChoiceDescriptor after use.
				 */
				if (parmcode == ATP_BCH) {
				  Atp_ChoiceDescriptor *ChoiceDescPtr;
				  ChoiceDescPtr = (Atp_ChoiceDescriptor *) CtrlStore[x].parmValue;
				  if (ChoiceDescPtr != NULL) {
					if (ChoiceDescPtr->CaseName != NULL) {
					  FREE(ChoiceDescPtr->CaseName);
					  ChoiceDescPtr->CaseName = NULL;
					}
					if (ChoiceDescPtr->CaseDesc != NULL) {
					  FREE(ChoiceDescPtr->CaseDesc);
					  ChoiceDescPtr->CaseDesc = NULL;
					}
				  }
				}

				/*
					NOTE: No downLevel ParmStoreInfo control
					structure exists for default and zero
					instance repeat blocks.
				 */
				if (CtrlStore[x].downLevel != NULL) {
				  ReleaseStore(CtrlStore[x].downLevel,
						  	   CtrlStore[x].downLevel->parmValue);
				  CtrlStore[x].downLevel = NULL; /* this prevents accidental reuse */
				}
				break;
			}
			default:
				break;
		} /* switch */
	} /* for */

	/*
	 *	Release DataStore first, then CtrlStore.
	 */

	/*
	 *	Release parameter store for data if it's not the static
	 *	buffer.
	 */
	if (DataStore != NULL) {
	  FREE(DataStore);
	}

	/* Release store for control if it's not the static buffer. */
	if (CtrlStore != NULL) {
	  FREE(CtrlStore);
	}

	return;
}

/*+*******************************************************************

	Function Name:		Atp_PushParmStorePtrOnStack

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Pushes the parameter store	pointer onto a
						stack just to support nested commands!

	Modifications:
		Who			When					Description
	----------	----------------	------------------------------
	Alwyn Teh	13 July 1992		Initial Creation
	Alwyn Teh	31 December 2021	Make ParmStoreStack dynamic
									to avoid runtime error

********************************************************************-*/
#if defined (__STDC__) || defined (__cplusplus)
void Atp_PushParmStorePtrOnStack(void *_parmStore )
#else
void
Atp_PushParmStorePtrOnStack (_parmStore)
	void *_parmStore;
#endif
{
	ParmStoreMemMgtNode *parmStore = (ParmStoreMemMgtNode *)_parmStore;

	if (ParmStoreStack == NULL)
	  ParmStoreStack = calloc(ATP_MAX_NESTCMD_DEPTH+1, sizeof(ParmStoreMemMgtNode *));

	ParmStoreStack[++CurrParmStoreStackIndex] = (ParmStoreMemMgtNode *) parmStore;
}

/*+*******************************************************************

	Function Name:		Atp_PopParmStorePtrFromStack

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Pops the parameter store pointer	off the
						stack just to support nested commands!

	Modifications:
		Who			When				Description
	----------	-------------	-------------------------------
	Alwyn Teh	13 July 1992	Initial Creation

*******************************************************************_*/
#if defined(__STDC__) || defined(__cplusplus)
void Atp_PopParmStorePtrFromStack(void *_parmStore)
#else
void
Atp_PopParmStorePtrFromStack (_parmStore)
	void *_parmStore;
#endif
{
	ParmStoreMemMgtNode * parmStore = (ParmStoreMemMgtNode *)_parmStore;

	if (parmStore == ParmStoreStack[CurrParmStoreStackIndex])
	  ParmStoreStack[CurrParmStoreStackIndex--] = NULL;
}

/*+*******************************************************************

	Function Name:		Atp_CurrParmStore

	Copyright:			BNR Europe Limited, 1992
						Bell-Northern Research
						Northern Telecom

	Description:		Lets you have the current parameter store to
						play with in case nested commands are used.

	Modifications:
		Who			When				Description
	----------	-------------	--------------------------------
	Alwyn Teh	13 July 1992	Initial Creation

********************************************************************-*/
void * Atp_CurrParmStore()
{
	return (void *) (ParmStoreStack[CurrParmStoreStackIndex]);
}
