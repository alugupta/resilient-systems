/*
 * memory_system.h - header file for sim-dram which holds all function
 * declarations , structure definitions.
 *
 * This is released as part of the sim-dram infrastructure for power/timing
 * simulation of the memory system. Sim-dram can run stand-alone or be
 * integrated with any standard CPU simulator.
 *
 * Copyright (c) 2004 by David Wang Brinda Ganesh Bruce Jacob
 * Brinda Ganesh, David Wang, Bruce Jacob
 * Systems and Computer Architecture Lab
 * Dept of Electrical & Computer Engineering
 * University of Maryland, College Park
 * All Rights Reserved
 *
 * This software is distributed with *ABSOLUTELY NO SUPPORT* and
 * *NO WARRANTY*.  Permission is given to modify this code
 * as long as this notice is not removed.
 *
 * Send feedback to Brinda Ganesh brinda@eng.umd.edu
 * 				 or David Wang davewang@wam.umd.edu
 *               or Bruce Jacob blj@eng.umd.edu

 */
#ifndef _MEM_SYSTEM_H
#define _MEM_SYSTEM_H
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#ifdef MEM_TEST
#include "mem-test.h"
#else
#ifdef GEMS
#include "mem-dram-gems-interface.h"
#else
#ifdef M5
#include "mem/timing/DRAMsim/mem-m5-interface.h"
#else
#include "mem-interface.h"
#endif
#endif
#endif

#define DEBUG 1
#define MEMORY_SYSTEM_H	9763099
//#define DEBUG_POWER

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define	FALSE				0
#define TRUE				1

#define MAX_CPU_FREQUENCY		1000000		/* one teraherz */
#define MIN_CPU_FREQUENCY		10		/* ten MHz */
#define MAX_DRAM_FREQUENCY		1000000		/* one teraherz */
#define MIN_DRAM_FREQUENCY		10		/* ten MHz */
#define MIN_CHANNEL_WIDTH		2		/* narrowest allowable DRAM channel assumed to be 2 bytes */
#define MAX_CHANNEL_COUNT		8		/* max number of logical memory channels */
#define	MAX_RANK_COUNT			32		/* Number of ranks on a channel. A rank is a device-bank */
#define	MAX_BANK_COUNT			32		/* number of banks per rank (chip) */
#define MIN_CACHE_LINE_SIZE		32		/* min cacheline length assumed to be 32 bytes */

#define	MAX_AMB_BUFFER_COUNT	8		/* number of amb buffers */

/* There are 5 states to a memory transaction request
 *  
 *  INVALID   - Entry is not being used
 *  VALID     - ENTRY has valid memory transaction request, but not yet scheduled by DRAM controller
 *  SCHEDULED - request has been accepted by the dram system, broken up into commands, and scheduled for execution in dram system
 *  ISSUED	- 	at least one of the commands associated with the request has been issued to the dram system.
 *  CRITICAL_WORD_RECEIVED - We can allow the callback function to run, since the data has returned.
 *  COMPLETED - request is complete, latency has been computed by dram system.
 */

#define MEM_STATE_INVALID					-1
#define MEM_STATE_VALID						1
#define MEM_STATE_SCHEDULED					2
#define	MEM_STATE_CRITICAL_WORD_RECEIVED	3
#define MEM_STATE_COMPLETED					4
#define MEM_STATE_ISSUED					5
#define MEM_STATE_DATA_ISSUED				6 

#define UNKNOWN_RETRY			-2      	/* can't get a queue, so retry the whole thing */
#define UNKNOWN_PENDING			-1      	/* got a biu slot, just wait for DRAM return. */

#define	MEMORY_ACCESS_TYPES_COUNT	6		/* 5 different ways of accessing memory */
#define	MEMORY_UNKNOWN_COMMAND		0
#define	MEMORY_IFETCH_COMMAND		1
#define	MEMORY_WRITE_COMMAND		2
#define	MEMORY_READ_COMMAND		3
#define	MEMORY_PREFETCH			4

#define	MAX_BUS_QUEUE_DEPTH		1024 /* assume max 256 entry bus queue */   
#define MAX_BUDDY_COUNT         	4
#define	MAX_ACCESS_HISTORY_TABLE_SIZE	4 * 1024 * 1024	/* 256 MB, 64 byte lines = 4 M entries */
#define RECENT_ACCESS_HISTORY_DEPTH 	4              /* housekeeping, keep track of address of last 4 fetched lines */

							/* Transaction prioritization policy */
#define	FCFS				0		/* default, First Come First Served */
#define	RIFF				1		/* Read or Instruction Fetch First */
#define RIFFWWS				2		/* Read or Instruction Fetch First with Write Sweeping */
#define HSTP                            3               /* higest priority first (for prefetching) */
#define OBF				4		/* DRAM tries to service requests to open bank first */
#define WANG				6		/* rank and bank round robin, through each rank and bank */
#define MOST_PENDING		7 		/* Rixners most pending scheme - issue cmds to rows with most transactions */
#define LEAST_PENDING		8 		/* Rixners most pending scheme - issue cmds to rows with most transactions */
#define GREEDY 				9
/* pfcache #def's */

#define	MAX_PREFETCH_CACHE_SIZE		256		/* 256 fully associative entries */
#define	NO_PREFETCH			0
#define	STREAM_PREFETCH			1
#define	MARKOV_MEMORY_PREFETCH		2		/* In memory storage of Morkov prefetch hints */
#define	MARKOV_TABLE_PREFETCH		3		/* on CPU Table of Morkov prefetch hints */

#define	UNKNOWN				-1
#define SDRAM				0
#define	DDRSDRAM			1
#define DDR2				2
#define FBD_DDR2			3			/*** FB-DIMM + DDR2 combination ***/
#define DDR3				4	

/* these are #def's for use that tells the status of commands. INVALID assumed to be -1 */
/* A command is assumed to be IN_QUEUE, or in COMMAND_TRANSMISSION from controller to DRAM, or in
   PROCESSING by the DRAM ranks, or in DATA_TRANSMISSION back to the controller, or COMPLETED */

#define	MAX_TRANSACTION_QUEUE_DEPTH	16
#define	MIN_TRANSACTION_QUEUE_DEPTH	16 /* This is what the base config is */
#define	MAX_REFRESH_QUEUE_DEPTH		8		/* Number of Refreshes that you can hold upto */

#define IN_QUEUE			0 
#define	LINK_COMMAND_TRANSMISSION	1  // COMMAND_TRANSMISSION
#define	DRAM_PROCESSING				2  // PROCESSING
#define LINK_DATA_TRANSMISSION	    3  // DATA_TRANSMISSION
#define	AMB_PROCESSING				5  // Left it as is, it technicall is AMB UP processing (except for DRIVE)
#define	DIMM_COMMAND_TRANSMISSION	6  // AMB COMMAND TRANSMISSION
#define DIMM_DATA_TRANSMISSION		7  // AMB_DATA_TRANSMISSION
#define AMB_DOWN_PROCESSING         8 
#define DIMM_PRECHARGING			9 // Used only for CAS_WITH_PREC commands 
 
/*  Already defined above
#define	COMPLETED			4
 */

/* These are #def's for DRAM "action" messages */
/* SCHEDULED 				2 */
/* COMPLETED 				4 */
#define XMIT2PROC			5
#define PROC2DATA			6
#define XMIT2AMBPROC			7
#define AMBPROC2AMBCT			8
#define AMBCT2PROC			9
#define AMBCT2DATA			10
#define AMBPROC2DATA			11
#define AMBPROC2PROC			12
#define DATA2AMBDOWN			13
#define AMBDOWN2DATA			14
#define LINKDATA2PREC			15

#define MAX_DEBUG_STRING_SIZE 	24000
#define MAX_TRANSACTION_LATENCY	10000

/* These are the bundle types */
#define THREE_COMMAND_BUNDLE		0
#define COMMAND_PLUS_DATA_BUNDLE	1

/*** Bundle stuff ***/
#define DATA_BYTES_PER_READ_BUNDLE		16		/** Number of data bytes in a read bundle **/
#define DATA_BYTES_PER_WRITE_BUNDLE		8		/** Number of data bytes in a write bundle **/
/****************** DRIVE DATA positions  ***************
 * *****************************************************/
#define DATA_FIRST			0
#define DATA_MIDDLE			1
#define DATA_LAST			2

#define BUNDLE_SIZE					3
#define DATA_CMD_SIZE				2		/** Occupies two slots in a bundle **/
/* these are #def's for use with commands and transactions INVALID assumed to be -1 */


/* these are #def's for use with commands and transactions INVALID assumed to be -1 */
#define NUM_COMMANDS		14
#define	RAS					1
#define	CAS					2
#define	CAS_AND_PRECHARGE	3	
#define	CAS_WRITE				4		/* Write Command */
#define	CAS_WRITE_AND_PRECHARGE	5
#define RETIRE				6		/* Not currenly used */
#define	PRECHARGE			7
#define	PRECHARGE_ALL		8		/* precharge all, used as part of refresh */
#define	RAS_ALL				9		/* row access all.  Used as part of refresh */
#define	DRIVE				10		/* FB-DIMM -> data driven out of amb */
#define	DATA				11		/* FB-DIMM -> data being sent following a CAS to the DRAM */
#define CAS_WITH_DRIVE      12
#define	REFRESH				13		/* Refresh command  */
#define	REFRESH_ALL			14		/* Refresh command  */

/* Assumed state transitions of banks for each command*/

/*     PRECHARGE : (ACTIVE) -> PRECHARGING -> IDLE
 *     RAS  : (IDLE) -> ACTIVATING -> ACTIVE
 */

#define	BUSY				1

#define	IDLE 				-1
#define	PRECHARGING			1
#define	ACTIVATING			2
#define	ACTIVE				4

#define	BUS_BUSY			1
#define	BUS_TURNAROUND			2

/* This section defines the virtual address mapping policy */
#define VA_EQUATE			0			/* physical == virtual */
#define VA_SEQUENTIAL			1			/* sequential physical page assignment */
#define VA_RANDOM			2
#define VA_PER_BANK			3
#define VA_PER_RANK			4

/* This section defines the physical address mapping policy */

#define	BURGER_BASE_MAP			0
#define	BURGER_ALT_MAP			1
#define	INTEL845G_MAP			2
#define	SDRAM_BASE_MAP			3
#define SDRAM_HIPERF_MAP		4	
#define SDRAM_CLOSE_PAGE_MAP		5


#define ADDRESS_MAPPING_FAILURE		0
#define ADDRESS_MAPPING_SUCCESS		1

/* This section defines the row buffer management policy */

#define OPEN_PAGE               	0
#define CLOSE_PAGE              	1
#define	PERFECT_PAGE			2

/* Refresh policies that are supported and should be supported **/
#define REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK		0		/** Initial policy supported by ras_all/pre_all -> whole system goes down for refresh*/
#define REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK	1		/* Only one bank in all ranks goes down for refresh  Currently not supported*/
#define REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK	2		/* Only one bank in one rank at a time goes down for refresh */
#define REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK	3		/* Standard Refresh policy available*/

/* When do we issue a refresh -> highest priority opportunistic */
#define	REFRESH_HIGHEST_PRIORITY	0
#define REFRESH_OPPORTUNISTIC		1

// Update the latency stat names structure in mem-stat.c when updating latency
// types
#define LATENCY_OVERHEAD_COUNT  13
#define REFRESH_CMD_CONFLICT_OVERHEAD   0
#define LINK_UP_BUS_BUSY_OVERHEAD  1
#define DIMM_BUS_BUSY_OVERHEAD  2
#define PROCESSING_OVERHEAD     3
#define BANK_CONFLICT_OVERHEAD  4
#define AMB_BUSY_OVERHEAD       5
#define LINK_DOWN_BUS_BUSY_OVERHEAD  6
#define STRICT_ORDERING_OVERHEAD	7
#define RAW_OVERHEAD		8
#define	MISC_X_OVERHEAD		9
#define BUNDLE_CONFLICT_OVERHEAD 10
#define PREV_TRAN_SCHEDULED_OVERHEAD 11
#define BIU_RETURN_OVERHEAD 12
/* This section used for statistics gathering */

#define	GATHER_BUS_STAT			0
#define	GATHER_BANK_HIT_STAT		1
#define	GATHER_BANK_CONFLICT_STAT	2
#define	GATHER_CAS_PER_RAS_STAT		3
#define GATHER_REQUEST_LOCALITY_STAT	4
#define GATHER_BUNDLE_STAT		5
#define GATHER_TRAN_QUEUE_VALID_STAT		6
#define GATHER_BIU_SLOT_VALID_STAT	7
#define GATHER_BIU_ACCESS_DIST_STAT	8

#define CMS_MAX_LOCALITY_RANGE		257             /* 256 + 1 */

/*
 * DEfine which trace this is.
 */

#define         NO_TRACE                0
#define         K6_TRACE                1
#define         TM_TRACE                2
#define         SS_TRACE                3
#define         MASE_TRACE              4

/***********************************************************************
 Here are the data structures for the prefetch cache
 ***********************************************************************/


typedef struct RECENT_ACCESS_HISTORY{
	uint64_t    address[RECENT_ACCESS_HISTORY_DEPTH];      /* 0 is most recently accessed, MAX is oldest */
} RECENT_ACCESS_HISTORY;

typedef struct GLOBAL_ACCESS_HISTORY_ENTRY {
	uint64_t    address[MAX_BUDDY_COUNT];
} GLOBAL_ACCESS_HISTORY_ENTRY;

typedef struct GLOBAL_ACCESS_HISTORY {
	GLOBAL_ACCESS_HISTORY_ENTRY      table_entry[MAX_ACCESS_HISTORY_TABLE_SIZE];
} GLOBAL_ACCESS_HISTORY;

typedef struct pfcache_t {

	int		mechanism;		/* No prefetch, or streaming prefetch, or markov */

	int		debug_flag;
	int		depth;			/* prefetch ahead 1,2,3, or 4? */
	int		cache_hit_count;	/* How many were eventually used? */
	int		biu_hit_count;		/* How many were eventually used? */
	int		prefetch_attempt_count;	/* how many prefetches were done? */
	int		prefetch_entry_count;	/* how many prefetches were done? */
	int		dirty_evict;		/* special dirty evict mode */
	int		hit_latency;		/* # of CPU cycles for a hit */

	uint64_t	address[MAX_PREFETCH_CACHE_SIZE];
	RECENT_ACCESS_HISTORY	recent_access_history[MEMORY_ACCESS_TYPES_COUNT];
	GLOBAL_ACCESS_HISTORY	*global_access_history_ptr;
} pfcache_t;

/***********************************************************************
 Here are the data structures for the memory system.
 ***********************************************************************/

typedef struct addresses_t {
	uint64_t	virtual_address;	
	int		thread_id;
	uint64_t	physical_address;
	int		chan_id;		/* logical channel id */
	int		rank_id;		/* device id */
	int		bank_id;
	int		row_id;
	int		col_id;			/* column address */
} addresses_t;

typedef struct bank_t {
	int		status;			/* IDLE, ACTIVATING, ACTIVE, PRECHARGING */
	int		row_id;			/* if the bank is open to a certain row, what is the row id? */
	tick_t		ras_done_time;		/* precharge command can be accepted when this counter expires */
	tick_t		rcd_done_time;		/* cas command can be accepted when this counter expires */
	tick_t		cas_done_time;		/* When this command completes, data appears on the data bus */
	tick_t		rp_done_time;		/* another ras command can be accepted when this counter expires */
	tick_t		rfc_done_time;		/* another refresh/ras command can be accepted when this counter expires */
	tick_t		rtr_done_time;		/* for architecture with write buffer, this command means that a retirement command can be issued */
	tick_t		twr_done_time;		/* twr_done_time */
	int		last_command;		/* used to gather stats */
	int		cas_count_since_ras;	/* how many CAS have we done since the RAS? */
	uint64_t		bank_conflict;	/* how many CAS have we done since the RAS? */
	uint64_t		bank_hit;	/* how many CAS have we done since the RAS? */
	int 		paging_policy;		/* open or closed, per bank, used in dynamic policy */
} bank_t;

typedef struct amb_buffer_t {
	int value;
	uint64_t tran_id; // the id of the transaction currently using the buffer
	int	occupied;	/** Set to TRUE if occupied else set to FALSE **/
} amb_buffer_t;

typedef struct power_counter_t {
  uint64_t             bnk_pre;    /* pecentage of time all banks are precharged */
  uint64_t             cke_hi_pre;
  uint64_t             cke_hi_act;
  tick_t          last_RAS_time; /* last time we issue RAS command  */
  tick_t          last_CAS_time; /* last time we issue CAS command  */
  tick_t          last_CAW_time; /* last time we issue CAW command  */
  uint64_t             RAS_count; /* How many times we isssue RAS, CAS, and CAW */
  uint64_t             RAS_sum;   /* summation of all the RAS command cycles */
  tick_t             current_CAS_cycle, current_CAW_cycle; /* cycles of current CAS/CAW command in current RAS period */
  tick_t           sum_per_RD, sum_per_WR;    /* summation of the CAS/CAW cycles in current print period  */
  uint64_t             dram_access;
  uint64_t             refresh_count;
  tick_t			  amb_busy;
  tick_t			  amb_busy_spill_over;
  tick_t			  amb_data_pass_through;
  tick_t			  amb_data_pass_through_spill_over;
  tick_t 		  busy_till;
  tick_t		  busy_last;
}power_counter_t;

typedef struct power_meas_t {
	float p_ACT;
	float CKE_LO_PRE;
	float CKE_LO_ACT;
	float t_ACT;
	float BNK_PRE;
	float p_PRE_PDN;
	float p_PRE_STBY;
	float p_PRE_MAX;
	float p_ACT_PDN;
	float p_ACT_STBY;
	float p_ACT_MAX;
	float WR_percent;
	float RD_percent;
	float p_WR;
	float p_RD;
	float p_DQ;
	float p_REF;
	float p_TOT;
	float p_TOT_MAX;
	float p_AMB_ACT;
	float p_AMB_PASS_THROUGH;
	float p_AMB_IDLE;
	float p_AMB;
	
}power_meas_t;

typedef struct dimm_t{
  int			up_buffer_cnt; /* Buffer size : amt of data sent per bundle default 8 butes*/
  int			down_buffer_cnt; /* Buffer size : amt of data sent out per bundle default 16 butes*/
  amb_buffer_t	*up_buffer;
  amb_buffer_t	*down_buffer;	
  int 		num_up_buffer_free;
  int 		num_down_buffer_free;
  tick_t		fbdimm_cmd_bus_completion_time;/* Used by fb-dimm: time when the bus will be free */
  tick_t		data_bus_completion_time;/* Used by fb-dimm: time when the bus will be free */
  int	rank_members[2];	
}dimm_t;
typedef struct rank_t {
  bank_t		bank[MAX_BANK_COUNT];
  tick_t		activation_history[4];	/* keep track of time of last four activations for tFAW and tRRD*/
  //Ohm--Adding for power calculation
  power_counter_t	r_p_info; /* Global Access Snapshot */
  power_counter_t	r_p_gblinfo; /* Global Access Snapshot */
  bool			cke_bit;		/* Set to 1 if the rank CKE is enabled */
  uint64_t	cke_tid;  	/* Transaction which caused it to be set high */
  uint64_t	cke_cmd;  	/* Transaction which caused it to be set high */
  tick_t		cke_done;
  dimm_t		*my_dimm;  
} rank_t;


/* this controls the row buffers to see which one is open, which one may be opened soon, so we don't send two 
   ACT commands to the same bank on the same rank.  The bonus here is that we can also keep track of the 
   number of open rows we allow 

   Currently Disabled.
*/

typedef struct row_buffer_t {
  	int status; 			/* Three choices. IDLE, OPEN, PENDING */  
	int completion_time;		/* this is used with counters to decide how long to keep a row buffer open */
  	int rank_id;
	int row_id;
  	int bank_id;
} row_buffer_t;

 
typedef struct command_t {
	int		status;
	tick_t		start_time;			/* In DRAM ticks */
    int             command;                        /* which command is this? */
	int		chan_id;
	int 	rank_id;
	int		bank_id;
	int		row_id;
	int		col_id;	/* column address */
	tick_t	completion_time;
	/** Added list of completion times in order to clean up code */
	tick_t	start_transmission_time;
	tick_t	link_comm_tran_comp_time;
	tick_t	amb_proc_comp_time;
	tick_t	dimm_comm_tran_comp_time;
	tick_t	dram_proc_comp_time;
	tick_t	dimm_data_tran_comp_time;
	tick_t	amb_down_proc_comp_time;
	tick_t	link_data_tran_comp_time;
	tick_t	rank_done_time;
	/* Variables added for the FB-DIMM */
	int		bundle_id;			/* Bundle into which command is being sent - Do we need this ?? */
	uint64_t 	tran_id;  			/*	The transaction id number */
	int		data_word;		/* Indicates which portion of data is returned i.e. entire cacheline or fragment thereof
									and which portions are being sent*/	
	int 	data_word_position;	/** This is used to determine which part of the data transmission we are doing : postions include 
								  FIRST , MIDDLE, LAST **/
	int		refresh;		/** This is used to determine if the ras/prec are part of refresh **/
	struct command_t *rq_next_c; /* This is used to set up the refresh queue */
	int		posted_cas;		/** This is used to determine if the ras + cas were in the same bundle **/
	 int     cmd_chked_for_scheduling;   /** This is used to keep track of the latency - set if the command has been checked to be scheudled*/
	int     cmd_updated;    /** This is set if the update_command was called and reqd **/
	int     cmd_id; /* Id added to keep track of which command you have already issued in a list of commands**/
			 
	struct command_t *next_c;
} command_t;

/* type used for FB-DIMM implementation */
typedef struct bundle_t {
	int bundle_id;
	int bundle_type; /* specifies whether this is a bundle with 3 commands or 1 command and write data */
	tick_t start_time;
}bundle_t;
typedef struct transaction_t {
	int		status;
	tick_t		arrival_time;		/* time when first seen by the memory controller in DRAM clock ticks */
	tick_t		completion_time;	/* time when entire transaction has completed in DRAM clock ticks */
	uint64_t	transaction_id;			/** Gives the Transaction id **/
	int		transaction_type;	/* command type */
	int		slot_id;		/* Id of the slot that requested this transaction */
	addresses_t address;
	int	tindex; // To keep track of which slot you are in 
	//int		chan_id;		/* id of the memory controller */
    //uint64_t    physical_address; 
	int		critical_word_available;	/* critical word flag used in the FBDIMM */
	int		critical_word_ready;	/* critical word ready flag */
	tick_t		critical_word_ready_time;	/* time when the critical word first filters back in DRAM clock ticks */
	command_t	*next_c;

	bool issued_command;
	bool issued_data;
	bool issued_cas;	
	bool issued_ras; // helps keep track of bank conflicts in case of open page	
	struct transaction_t * next_ptr; /* Used by the pending queue only invalid in the actual transaction queue */	
} transaction_t;

typedef struct global_tq_info {
	bool debug_flag;
	uint64_t	debug_tran_id_threshold;	/** tran id at which to start debug **/ 
	int     num_ifetch_tran;
    int     num_read_tran;
    int     num_prefetch_tran;
    int     num_write_tran;
	uint64_t		tran_watch_id;
	bool		tran_watch_flag;	/* Set to true if you are indeed watching a transaction **/
	uint64_t		transaction_id_ctr;		/* Transaction id for new transaction **/
	command_t	*free_command_pool;
	tick_t last_transaction_retired_time;
}global_tq_info_t;
typedef struct transaction_queue_t {
	int		transaction_count;		/** Tail at which we add **/
	transaction_t 	entry[MAX_TRANSACTION_QUEUE_DEPTH];	/* FIFO queue inside of controller */
} transaction_queue_t;

/* Used to keep track of info for most pending and least pending Transaction
 Selection policyes */
typedef struct pending_info_t{
  int rank_id;
  int bank_id;
  int row_id;
  int transaction_count;
  int transaction_list[MAX_TRANSACTION_QUEUE_DEPTH];
  int head_location;
  int tail_location;
}pending_info_t;

typedef struct pending_queue_t{
  pending_info_t pq_entry[MAX_TRANSACTION_QUEUE_DEPTH];
  int queue_size; /* Number of valid entries */
}pending_queue_t;
/* All this thing does is tell if someone is using it now */

typedef struct bus_t {
	int		status;		/* IDLE, BUSY */
	tick_t		completion_time;/* time when the bus will be free */
	int		current_rank_id;
	int		command;	/* if this is a command bus, the command on this bus now */
} bus_t;

typedef	struct refresh_queue_t {
  	command_t	*rq_head;				/* Pointer to head of refresh command queue */
	int			refresh_rank_index;		/* the rank index to be refreshed - dont care for some policies */
	int			refresh_bank_index;		/* the bank index to be refreshed - dont care for some policies */
	int			refresh_row_index;		/* the row index to be refreshed */
	tick_t		last_refresh_cycle;		/* tells me when last refresh was done */
	uint64_t refresh_counter;
	int 		refresh_pending;		/* Set to TRUE if we have to refresh and transaction queue was full */
	int 		size; 					/* Tells you how many refresh commands are currently active */
}refresh_queue_t;

typedef struct rbrr_t {
  int last_cmd; /* refers to the last CAS/RAS/PRE executed */
  int last_ref; /* If we executed a refresh we dont want to disturb the sequence */

  /* Stuff we need to execute now */
  int current_cmd; /* What we should execute now */
  /* RAS -> 0 CAS -> 1 PRE -> 2 REF -> 3 */
  int current_bank[4][MAX_RANK_COUNT];
  int current_rank[4];
  int cmd2indmapping[4]; 
}rbrr_t;

typedef struct dram_controller_t {
	int		id;
    transaction_queue_t     	transaction_queue;
	rank_t 		rank[MAX_RANK_COUNT];  		/*  device-bank in SDRAM system */
	dimm_t 		dimm[MAX_RANK_COUNT];  		/* assume 1 per rank initially: used in fb-dimm system */
	bus_t		command_bus;			/* used for DDR/SDRAM */
	bus_t		data_bus;			/* used for every one with bi-directional bus */
	bus_t		row_command_bus;		/* used for systems that use separate row and col command busses */
	bus_t		col_command_bus;

	/* Bus Descriptions added for FB-DIMM	*/
	bus_t		up_bus;					/* point-to-point link to the DRAMs -> carries cmd and write data */
	bus_t		down_bus;				/* point-to-point link from the DRAMs -> carries read data  */
	uint64_t bundle_id_ctr;			/* Keeps track of how manyth bundle in the channel is to be issued*/			
	int     last_rank_id;
	int     last_bank_id;
	int     last_command;
	int     next_command;
	int 	t_faw_violated;
	int		t_rrd_violated;
	int		t_dqs_violated;	
	/* We need some stuff for the RBRR */
	struct rbrr_t  sched_cmd_info;
	refresh_queue_t		refresh_queue;
	pending_queue_t		pending_queue;
	int 	active_write_trans;
	bool	active_write_flag;
	int 	active_write_burst;
}dram_controller_t;
	
typedef struct	power_config_t{
	
  float max_VDD;
  float min_VDD;
  float	VDD;
  float P_per_DQ;
  int density;
  int DQ;
  int DQS;
  int IDD0;
  int IDD2P;
  int IDD2F;
  int IDD3P;
  int IDD3N;
  int IDD4R;
  int IDD4W;
  int IDD5;
  float t_CK;
  float t_RC;
  float t_RFC_min;
  float t_REFI;
  float VDD_scale;
  float freq_scale; /* Note this needs to be fixed everytime you change the dram frequency*/
  int	print_period;
  int 	chip_count;

  float ICC_Idle_0;
  float ICC_Idle_1;
  float ICC_Idle_2;
  float ICC_Active_1;
  float ICC_Active_2;
  float VCC;

  /* Pre-calcualted power values */
  float p_PRE_PDN;
  float p_PRE_STBY;
  float p_ACT_PDN;
  float p_ACT_STBY;
  float p_ACT;
  float p_WR;
  float p_RD;
  float p_REF;
  float p_DQ;
}power_config_t; 	/* Holds all the power variables */


typedef struct power_info_t{
  tick_t	last_print_time;
  FILE *	power_file_ptr;
}power_info_t;
/* 
 *	Contains all of the configuration information.  
 */

typedef struct dram_system_configuration_t {
	int		dram_type;					/* SDRAM, DDRSDRAM,  etc. */
	int		memory_frequency;			/* clock freq of memory controller system, in MHz */ 
	int		dram_frequency;				/* FB-DIMM only clock freq of the dram system, in MHz */ 
	float 	memory2dram_freq_ratio;		/* FBD-DIMM multiplier for timing */
	uint64_t		dram_clock_granularity;		/* 1 for SDRAM, 2 for DDR and etc */
	int		critical_word_first_flag;	/* SDRAM && DDR SDRAM should have this flag set, */

	int		physical_address_mapping_policy;/* which address mapping policy for Physical to memory? */
    int     row_buffer_management_policy;   /* which row buffer management policy? OPEN PAGE, CLOSE PAGE, etc */
    int     refresh_policy;   				/* What refresh policies are applied */
    int     refresh_issue_policy;   				/* Should we issue at once or when opportunity arises*/

	int		cacheline_size;			/* size of a cacheline burst, in bytes */
	int		channel_count;			/* How many logical channels are there ? */
	int		channel_width;			/* How many bytes width per logical channel? */
	int		up_channel_width;	/* FB-DIMM : Up channel width in bits */
	int		down_channel_width;	/* FB-DIMM : Down channel width in bits */
	int 	rank_count;			/* How many ranks are there on the channel ? */
	int		bank_count;			/* How many banks per device? */
	int		row_count;			/* how many rows per bank? */
	int		col_count;			/* Hwo many columns per row? */
      int     packet_count;           /*   cacheline_size / (channel_width * 8) */
      
	int max_tq_size;
							/* we also ignore t_pp (min prec to prec of any bank) and t_rr (min ras to ras).   */
	int		data_cmd_count;		/** For FBDIMM : Reqd to determine how many data "commands" need to be sent **/
	int		drive_cmd_count;		/** For FBDIMM : Reqd to determine how many drive "commands" need to be sent currently set to 1**/
	int     	t_burst;          		/* number of cycles utilized per cacheline burst */
	int		t_cas;				/* delay between start of CAS command and start of data burst */
	int		t_cmd;				/* command bus duration... */
	int		t_cwd;				/* delay between end of CAS Write command and start of data packet */
	int		t_cac;				/* delay between end of CAS command and start of data burst*/
	int		t_dqs;				/* rank hand off penalty. 0 for SDRAM, 2 for DDR, and */
	uint64_t		t_faw;				/* four bank activation */
	int		t_ras;				/* interval between ACT and PRECHARGE to same bank */
	int		t_rc;				/* t_rc is simply t_ras + t_rp */
	int		t_rcd;				/* RAS to CAS delay of same bank */
	uint64_t	t_rrd;				/* Row to row activation delay */
	int		t_rp;				/* interval between PRECHARGE and ACT to same bank */
	int		t_rfc;				/* t_rfc -> refresh timing : for sdram = t_rc for ddr onwards its t_rfc (refer datasheetss)*/
	int		t_wr;				/* write recovery time latency, time to restore data o cells */
	int		t_rtp;				/* Interal READ-to-PRECHARGE delay - reqd for CAS with PREC support prec delayed by this amount */

	int		posted_cas_flag;		/* special for DDR2 */
	int		t_al;				/* additive latency = t_rcd - 2 (ddr half cycles) */
	
	int		t_rl;				/* read latency  = t_al + t_cas */
	int		t_rtr;				/* delay between start of CAS Write command and start of write retirement command*/

	int		t_bus;					/* FBDIMM - bus delay */
	int		t_amb_up;				/* FBDIMM - Amb up delay  - also the write buffer delay*/
	int		t_amb_down;				/* FBDIMM - Amb down delay - also the read buffer delay  Note that this implies the first 2 bursts are packaged and sent down in this time later -> overlap of dimm and link burst is seen*/
	int		t_amb_forward;				/* FBDIMM - Amb to AMB forwarding delay */
    uint64_t     t_bundle;          		/* FBDIMM number of cycles utilized to transmit a bundle */
	int		up_buffer_cnt;			/* FBDIMM	number of buffers available at the AMB for write cmds  1 buffer => 1 cache line*/ 
	int		down_buffer_cnt;		/* FBDIMM	number of buffers available at the AMB for read cmds */ 
	int		var_latency_flag;		/* FBDIMM	Set when the bus latency is to not rank dependent */
	int		row_command_duration;
	int		col_command_duration;
    int     auto_refresh_enabled;       /* interleaved autorefresh, or no refresh at all */
  
	double		refresh_time;			/* given in microseconds. Should be 64000. 1 us is 1 MHz, makes the math easier */
	int		refresh_cycle_count;		/* Every # of DRAM clocks, send refresh command */
	int 	watch_refresh;
	uint64_t ref_tran_id;
 
	tick_t arrival_threshold;
	int		strict_ordering_flag;		/* if this flag is set, we cannot opportunistically allow later */
							/* CAS command to by pass an earlier one. (earlier one may be waiting for PREC+RAS) */
							/* debug flags turns on debug printf's */
	int		dram_debug;
	int		addr_debug;			/* debug the address mapping schemes */
	int		wave_debug;			/* draw cheesy waveforms */
	int		issue_debug;			/* Prints out when things got issued - DDRX */
	int		wave_cas_debug;			/* draw cheesy waveforms of cas activity only*/
	int		bundle_debug;			/* for FBDIMM */
	int		amb_buffer_debug;		/* for FBDIMM */
	int		tq_delay;				/* How many DRAM delay cycles should the memory controller add for delay through the transaction queue? */

	bool	single_rank; // true - Number of ranks per dimm=1 else false for dual rank systems 
	int 	num_thread_sets;
	int		thread_sets[32]; // need to make this some const
	int 	thread_set_map[32];

	bool    cas_with_prec; // For close page systems this will be used 

  	power_config_t dram_power_config;	/* Power Configuration */

} dram_system_configuration_t;

/*
 *  	DRAM System now has the configuration component, the transaction queue and the dram controller
 */

typedef struct dram_system_t {
  tick_t				current_dram_time;
  tick_t				last_dram_time;			
  tick_t				current_mc_time;
  tick_t				last_mc_time;		
  dram_system_configuration_t 	config;
  dram_controller_t       	dram_controller[MAX_CHANNEL_COUNT];
  global_tq_info_t		tq_info;
  power_info_t		dram_power;
} dram_system_t;

typedef struct STAT_BUCKET {
	int                     count;
	int                  	delta_stat;		/* latency for bus stat, time between events for DRAM stat */
	struct  STAT_BUCKET 	*next_b;
} STAT_BUCKET;

typedef struct DRAM_STATS {
  uint64_t* value; // Keeps everything in a large array
  uint64_t size; // size of array
	STAT_BUCKET 	*next_b; // if value overflows from array you place it in the stat bucket!
} DRAM_STATS;

typedef struct aux_stat_t {
	/* statistical gathering stuff */
	/* where to dump out the stats */
	FILE		*stats_fileptr;

	DRAM_STATS	*bus_stats;
	DRAM_STATS	*bank_hit_stats;
	DRAM_STATS	*bank_conflict_stats;
	DRAM_STATS	*cas_per_ras_stats;
	DRAM_STATS	*biu_access_dist_stats;
	
	
	// You need an array for these stats
	int	*tran_queue_valid_stats;
	int *biu_slot_valid_stats;
	int*bundle_stats;

	int		page_size;		/* how big is a page? */
    int             locality_range;
	int		valid_entries;
    int             locality_hit[CMS_MAX_LOCALITY_RANGE];
	uint64_t	previous_address_page[CMS_MAX_LOCALITY_RANGE];

	/** additional bundle stats for FBD **/
    int num_cas[3];
    int num_cas_w_drive[3];
    int num_cas_w_prec[3];
    int num_cas_write[3];
    int num_cas_write_w_prec[3];
    int num_ras[3];
    int num_ras_refresh[3];
    int num_ras_all[3];
    int num_prec[3];
    int num_prec_refresh[3];
    int num_prec_all[3];
    int num_refresh[3];
    int num_drive[3];
    int num_data[3];
    int num_refresh_all[3];
    /** end additional bundle stats **/

} aux_stat_t;

typedef struct BUS_event_t {   /* 4 DWord per event */
    bool 	already_run;
	int     attributes;
	uint64_t     address;        /* assume to be < 32 bits for now */
	int 	trace_id;
	tick_t  timestamp;      /* time stamp will now have a large dynamic range, but only 53 bit precision */
	addresses_t addr; // Added only to support sending addresses of banks/ranks/chans/rows pre-decided
	uint8_t length; // Used to set the burst length of  request
} BUS_event_t;

// Stuff for the random distribution stolen from DRAMSIMII
enum distribution_type_t
{
  DISTRIBUTION_UNIFORM,
  DISTRIBUTION_GAUSSIAN,
  DISTRIBUTION_POISSON,
  DISTRIBUTION_FIXED,
  DISTRIBUTION_NORMAL
};

typedef struct input_rand_t {
  double chan_locality;
  double rank_locality;
  double bank_locality;
  tick_t time;                /* time reported by trace or recorded by random number generator */
  tick_t last_issue_time;
  double row_locality;
  double read_percentage;     /* the percentage of accesses that are reads. should replace with access_distribution[] */
  double short_burst_ratio;       /* long burst or short burst? */
  double arrival_thresh_hold;
  tick_t average_interarrival_cycle_count;   /* used by random number generator */
  enum distribution_type_t interarrival_distribution_model;
  int last_chan_id;
  int * last_rank;
  int * last_bank;
  int * last_row;
}input_rand_t;
//#define rand_s(a) *a=lrand48()*2

#define rand_s(a) *a=rand()


/***********************************************************************
 Here are the functiona prototypes for the prefetch cache
 ***********************************************************************/

void init_prefetch_cache(int);
int  get_prefetch_mechanism();
int  active_prefetch();
void set_prefetch_debug(int);
int  prefetch_debug();
void set_prefetch_depth(int);
int  get_prefetch_depth();
void set_dirty_evict();
int  dirty_evict();

void init_global_access_history();
void init_recent_access_history();
void update_recent_access_history(int, uint64_t);
int  get_prefetch_attempt_count();
int  get_prefetch_hit_count();

void check_prefetch_algorithm(int, uint64_t, tick_t);
void update_prefetch_cache(uint64_t);
void issue_prefetch(uint64_t, int, tick_t);

int  pfcache_hit_check(uint64_t);	/* given address, see if there's a hit in the pfetch cache hit */
int  pfcache_hit_latency();
void remove_pfcache_entry(uint64_t);

/***********************************************************************
 * Here are the functional prototypes for the Configuration settings of the Memory System
 ***********************************************************************/

void init_dram_system_configuration();
void convert_config_dram_cycles_to_mem_cycles();
void set_dram_type(int);
void set_dram_frequency(int);
int  get_dram_frequency();
int  get_dram_type();
void set_memory_frequency(int);
int  get_memory_frequency();
void set_dram_rank_count(int rank_count);
void set_dram_bank_count(int);
void set_dram_row_count(int);
void set_dram_col_count(int);
void set_dram_channel_count(int);
int get_dram_channel_count();
int get_dram_rank_count();
int get_dram_bank_count();
int get_dram_row_count();
int get_dram_row_buffer_management_policy();
void set_dram_buffer_count(int);
void set_dram_channel_width(int);
void set_dram_up_channel_width(int); /*** FB-DIMM ***/
void set_dram_down_channel_width(int); /*** FB-DIMM ***/
void set_dram_transaction_granularity(int);
void set_pa_mapping_policy(int);
void set_dram_row_buffer_management_policy(int);
void set_dram_refresh_policy(int);
void set_dram_refresh_issue_policy(int);
void set_dram_refresh_time(int);
void set_fbd_var_latency_flag(int);
void set_posted_cas(bool);
void set_dram_debug(int);
void set_addr_debug(int);
void set_wave_debug(int);
void set_issue_debug(int);
void set_wave_cas_debug(int);
void set_bundle_debug(int);
void set_amb_buffer_debug(int);
void set_tran_watch(uint64_t tran_id);
int get_tran_watch(uint64_t tran_id);
int get_ref_tran_watch(uint64_t tran_id);
void set_ref_tran_watch(uint64_t tran_id);
void enable_auto_refresh(int, int);
 int  dram_debug();
 int  addr_debug();
int  wave_debug();
 int  cas_wave_debug();
 int  bundle_debug();
 int  amb_buffer_debug();
void set_strict_ordering(int);
void set_t_bundle(int);
void set_t_bus(int);
void set_independent_threads(int *thread_sets,int num_threads);
dram_system_configuration_t *get_dram_system_config();
power_info_t *get_dram_power_info();
dram_system_t *get_dram_system();
tick_t get_dram_current_time();
void read_dram_config_from_file(FILE *, dram_system_configuration_t *);
BUS_event_t *get_next_bus_event( BUS_event_t *,int);
int trace_file_io_token(char *);

/***********************************************************************
 Here are the functional prototypes for the Memory controller
 ***********************************************************************/

void init_dram_controller(int);
bool can_issue_command(int, command_t *);
bool can_issue_cas(int, command_t *);
bool can_issue_ras(int, command_t *);
bool can_issue_prec(int, command_t *);
bool can_issue_prec_all(int, command_t *);
bool can_issue_ras_all(int, command_t *);
bool can_issue_drive(int, command_t *);
bool can_issue_data(int, command_t *);
bool can_issue_cas_with_precharge(int, command_t *);
bool fbd_can_issue_ras_with_posted_cas (int , command_t *);
bool check_for_refresh(command_t * this_c,int tran_index,tick_t cmd_bus_reqd_time);
bool check_cmd_bus_required_time (command_t * this_c,  tick_t cmd_bus_reqd_time);


/* the following might need tran_ids in the future */
void init_amb_buffer();
int is_down_buffer_free(int, int, int, uint64_t);
int is_up_buffer_free(int, int, int, uint64_t);
int has_up_buffer(int chan_id, int rank_id, int bank_id, uint64_t tid);
int has_down_buffer (int chan_id, int rank_id, int bank_id, uint64_t tid);
int get_open_up_buffer(int, int, int, uint64_t);
int get_open_down_buffer(int, int, int, uint64_t);

void print_buffer_contents(int, int);

void release_up_buffer(int, int, int, uint64_t);
void release_down_buffer(int, int, int, uint64_t);

void lock_buffer(command_t*,int);

int  row_command_bus_idle(int);
int  col_command_bus_idle(int);
int is_cmd_bus_idle(int chan_id,command_t *this_c);
int  up_bus_idle(int);
int  down_bus_idle(int);
void set_row_command_bus(int, int);
void set_col_command_bus(int, int);
void update_bus_link_status(int);


void print_dram_system_state();
void print_status(int);
void print_bus_status();
void build_wave(char *, int, command_t *, int);

/***********************************************************************
 Here are the functional prototypes for the Transaction Queue
 ***********************************************************************/

void init_transaction_queue();
void set_chipset_delay(int);
int  get_chipset_delay();
int  virtual_address_translation(int, int, addresses_t *);		/* take virtual address, map to physical address */
int  physical_address_translation(int, addresses_t *);		/* take physical address, convert to channel, rank, bank, row, col addr */
void print_addresses(addresses_t *);
int convert_address(int , dram_system_configuration_t *, addresses_t *);
int get_transaction_index(int chan_id,uint64_t);

int  add_transaction(tick_t, int, int,int *);
void remove_transaction(int chan_id,int tindex);
command_t *acquire_command();
void release_command(command_t *);
command_t *add_command(tick_t, command_t *, int, addresses_t *, uint64_t,int,int,int);
command_t *transaction2commands(tick_t, int, int, addresses_t *);
void commands2bundle(tick_t, int,char *);
bool is_bank_open(int chan_id,int rank_id,int bank_id, int row_id) ;
void  collapse_cmd_queue( transaction_t * this_t, command_t *cmd_issued);

void set_transaction_debug(int);
void set_debug_tran_id_threshold(uint64_t);
int get_transaction_debug(void);
int  transaction_debug();
void print_command(tick_t,command_t *);
void print_command_detail(tick_t,command_t *);
void print_bundle(tick_t,command_t *[],int);
void print_transaction(tick_t,int,uint64_t);
void print_transaction_index(tick_t now, int chan_id, uint64_t tindex);
  
void print_transaction_queue(tick_t);
int get_num_read();
int get_num_write();
int get_num_ifetch();
int get_num_prefetch();
/*Redefined to take care of actual log2 math.h definitions */
int  sim_dram_log2(int input);

void dram_dump_config(FILE *);

/***********************************************************************
 * Here are the functional prototypes for init and update of system as a whole
 ***********************************************************************/

void init_dram_system();				/* set DRAM to system defaults */
void update_dram_system(tick_t);
void update_base_dram(tick_t);
#ifdef ALPHA_SIM
int is_dram_busy();
#else
bool is_dram_busy();
#endif
void update_refresh_status(tick_t,int,char *);
int  can_issue_refresh_command(command_t *, command_t *);
command_t * get_next_refresh_command_to_issue(
	int chan_id,int transaction_selection_policy,int command );
int is_refresh_queue_full(int chan_id);
int is_refresh_queue_half_full(int chan_id);
int is_refresh_queue_empty(int chan_id);
void update_tran_for_refresh(int chan_id);

command_t *next_RBRR_RAS_in_transaction_queue(int);
command_t *next_RBRR_CAS_in_transaction_queue(int);
command_t *next_PRECHARGE_in_transaction_queue(int);

command_t *init_RBRR_command_chain();
int update_command_states(tick_t,int, int,char *);
void cas_transition(command_t *, int, int, int, int, int, char *);
void cas_write_transition(command_t *, int, int, int, int, int, char *);
void ras_transition(command_t *, int, int, int, int, int, char *);
void precharge_transition(command_t *, int, int, int, int, int, char *);
void precharge_all_transition(command_t *, int, int, int, int, int, char *);
void ras_all_transition(command_t *, int, int, int, int, int, char *);
void drive_transition(command_t *, int, int, int, int, int, char *);
void data_transition(command_t *, int, int, int, int, int, char *);
void refresh_transition(command_t *, int, int, int, int, int, char *);
void refresh_all_transition(command_t *, int, int, int, int, int, char *);

void retire_transactions();
void issue_new_commands(tick_t,int, char *);
void insert_refresh_transaction(int);
void schedule_new_transaction(int chan_id);

void gather_tran_stats();
void issue_command(tick_t,command_t *, int);
void issue_cas(tick_t,command_t *,int);
void issue_cas_write(tick_t,command_t *,int);
void issue_ras_or_prec(tick_t,command_t *,int); /** OBSOLETE FUNCTION**/
void issue_ras(tick_t,command_t *,int);
void issue_prec(tick_t,command_t *,int);
void issue_data(tick_t,command_t *,int);
void issue_drive(tick_t,command_t *,int);
void issue_refresh(tick_t ,command_t *, int);
void issue_fbd_command(tick_t,command_t *, int,transaction_t*,char *);

void issue_bundle(tick_t, int, command_t *[]);

void update_refresh_missing_cycles (tick_t now,int chan_id,char * debug_string) ;
#ifdef ALPHA_SIM
int is_refresh_pending (int chan_id) ;
#else
bool is_refresh_pending (int chan_id) ;
#endif
/***********************************************************************
 * Here are the functional prototypes for statistic gathering
 ***********************************************************************/

void mem_gather_stat_init(int, int, int);
void mem_gather_stat(int, int);
#ifdef ALPHA_SIM
void mem_print_stat(int,int);
#else
void mem_print_stat(int,bool);
#endif
void mem_close_stat_fileptrs();
void init_extra_bundle_stat_collector();
void gather_extra_bundle_stats(command_t *[], int);
void print_extra_bundle_stats(bool);
void mem_stat_init_common_file(char *);
FILE *get_common_stat_file();
void print_rbrr_stat(int);
void print_bank_conflict_stats();
/***********************************************************************
 * Here are the functional prototypes for power data gathering
 ***********************************************************************/
void read_power_config_from_file(FILE *, power_config_t *);
void initialize_power_config(power_config_t *);
void mem_initialize_power_collection(power_info_t *dram_power, char *filename);
void power_update_freq_based_values(int freq,power_config_t *power_config_ptr);
void print_power_stats(tick_t now, dram_system_t *dram_system);
void print_global_power_stats(FILE *fileout);
void update_amb_power_stats(command_t* bundle[], int command_count , tick_t now);
int check_cke_hi_pre(int chan_id, int rank_id);
int check_cke_hi_act(int chan_id, int rank_id);
void update_cke_bit(tick_t now,
		int chan_id,
			int max_ranks);
void update_power_stats(tick_t now, command_t *this_c);

/**************************************
 * Extern common declarations
 * *********************************/
extern dram_system_t dram_system;
#endif
