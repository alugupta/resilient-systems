/*
 * mem-biu.h - header file for mem-biu.c
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

#ifndef MEMORY_SYSTEM_H

#ifndef MEM_TEST
#include "memory.h"
#endif

#include "mem-system.h"
#endif

#define BIU_H 1010987

/***********************************************************************
 Here are the data structures for the BIU
 ***********************************************************************/
#define MAX_PROCESSOR 32	
typedef struct biu_slot_t {
	int		status;		/* INVALID, VALID, SCHEDULED, CRITICAL_WORD_RECEIVED or COMPLETED */
	int		thread_id;
	int		rid;		/* request id, or transaction id given by CPU */
	tick_t		start_time;	/* start time of transaction. unit is given in CPU ticks*/
	int		critical_word_ready;	/* critical word ready flag */
	int		callback_done;	/* make sure that callback is only performed once */
	addresses_t address;	/* Entire Address structure */
	int		access_type;	/* read/write.. etc */
	int		priority;	/* -1 = most important, BIG_NUM = least important */
#ifdef ALPHA_SIM 
	RELEASE_FN_TYPE callback_fn;    /* callback function to simulator */
	cache_access_packet *mp;	/* for use with alpha-sim */
#endif
#ifdef SIM_MASE
	void 		(*callback_fn)(int rid, int lat); /* callback function to simulator */
	void            *mp;
#endif
#ifdef MEM_TEST 
	RELEASE_FN_TYPE callback_fn;    /* callback function to simulator */
	void            *mp;
#endif
#ifdef GEMS
	RELEASE_FN_TYPE callback_fn;    /* callback function to simulator */
	void            *mp;
#endif
} biu_slot_t;

typedef struct biu_t {
	tick_t		current_cpu_time;		
	int		bus_queue_depth;
	int     active_slots;
	int     critical_word_rdy_slots;
	biu_slot_t	slot[MAX_BUS_QUEUE_DEPTH];
	int		fixed_latency_flag;
	tick_t		delay;		/* How many CPU delay cycles should the bus queue add? */
							/* debug flags turns on debug printf's */
	int		debug_flag;
	int		access_count[MEMORY_ACCESS_TYPES_COUNT];
	int		proc_request_count[MAX_PROCESSOR];
							/* where to dump out the stats */
	FILE 		*biu_stats_fileptr;		
	FILE        *biu_trace_fileptr;

	int		prefetch_biu_hit;		/* How many were eventually used? */
	int		prefetch_entry_count;		/* how many prefetches were done? */
	int		cpu_frequency;			/* cpu frequency now kept here */
	int             mem_frequency;               	/* a "copy" of memory frequency is kept here so sim-cms can do a bit of an optimization */
	tick_t          current_dram_time;              /* this is what biu expects the DRAM time to be based on its own computations, not the
							real DRAM time. The real DRAM time should be the same as this one, but it's kept inthe dram system */
	int             transaction_selection_policy;
	int             thread_count;                   /* how many threads are there on the cpu? */
	double 		mem2cpu_clock_ratio;
	double		cpu2mem_clock_ratio;
	int		max_write_burst_depth;		/* If we're doing write sweeping, limit to this number of writes*/
	int		write_burst_count;		/* How many have we done so far?*/
	dram_system_configuration_t  *dram_system_config; /* A pointer to the DRAM system configuration is kept here */
	int     last_bank_id;
	int     last_rank_id;
	int     last_transaction_type;
	tick_t  last_transaction_time;
} biu_t;

/***********************************************************************
 Here are the functiona prototypes for the BIU
 ***********************************************************************/

void init_biu(biu_t *);
biu_t *get_biu_address();
void set_biu_address(biu_t *);
void set_dram_chan_count_in_biu(biu_t *, int);
void set_dram_chan_width_in_biu(biu_t *, int);
void set_dram_cacheline_size_in_biu(biu_t *, int);
void set_dram_rank_count_in_biu(biu_t *, int);
void set_dram_bank_count_in_biu(biu_t *, int);
void set_dram_row_count_in_biu(biu_t *, int);
void set_dram_col_count_in_biu(biu_t *, int);
void set_dram_address_mapping_in_biu(biu_t *, int);
void set_biu_depth(biu_t *, int);
int  get_biu_depth(biu_t *);
void set_biu_delay(biu_t *, int);
int  get_biu_delay(biu_t *);
void set_cpu_dram_frequency_ratio(biu_t *, int);
int  biu_hit_check(biu_t *, int, unsigned int, int, int, tick_t);	/* see if request is already in biu. If so, then convert transparently */
int  find_free_biu_slot(biu_t *, int);
#ifdef ALPHA_SIM
void fill_biu_slot(biu_t *, int, int, tick_t, int, unsigned int, int, int, struct _cache_access_packet *,RELEASE_FN_TYPE);	/* slot_id, thread_id, now, rid, block address */
#endif // SIM_MASE use this

#ifdef SIM_MASE
void fill_biu_slot(biu_t *, int, int, tick_t, int, unsigned int, int, int, void *, void (*callback_fn)(int, int));	/* slot_id, thread_id, now, rid, block address */
#endif

#ifdef GEMS// This is true for the GEMS and mem-test versions
void fill_biu_slot(biu_t *, int, int, tick_t, int, unsigned int, int, int, void *, RELEASE_FN_TYPE);	/* slot_id, thread_id, now, rid, block address */
#endif
#ifdef MEM_TEST// This is true for the GEMS and mem-test versions
void fill_biu_slot(biu_t *, int, int, tick_t, int, unsigned int, int, int, void *, RELEASE_FN_TYPE);	/* slot_id, thread_id, now, rid, block address */
#endif

RELEASE_FN_TYPE biu_get_callback_fn(biu_t *this_b,int slot_id) ;

void *biu_get_access_sim_info(biu_t *this_b,int slot_id) ;
void release_biu_slot(biu_t *, int);
int  get_next_request_from_biu(biu_t *);				/* find next occupied bus slot */
int  get_next_request_from_biu_chan(biu_t *,int);				/* find next occupied bus slot */
int  bus_queue_status_check(biu_t *, int);
int  dram_update_required(biu_t *, tick_t);
int  find_critical_word_ready_slot(biu_t *, int);
int  find_completed_slot(biu_t *, int, tick_t);
int  num_free_biu_slot(biu_t *);
void squash_biu_entry_with_rid(biu_t *, int);

int  get_rid(biu_t *, int);
int  get_access_type(biu_t *, int);
tick_t get_start_time(biu_t *, int);
unsigned int get_virtual_address(biu_t *, int);
unsigned int get_physical_address(biu_t *, int);
void set_critical_word_ready(biu_t *, int);
int  critical_word_ready(biu_t *, int);
int  callback_done(biu_t *, int);
void biu_invoke_callback_fn(biu_t *, int);

void set_biu_slot_status(biu_t *, int, int);
int  get_biu_slot_status(biu_t *, int);

void set_biu_debug(biu_t *, int);
int  biu_debug(biu_t *);
void set_biu_fixed_latency(biu_t *, int);
int  biu_fixed_latency(biu_t *);

void print_access_type(int,FILE *);
void print_biu(biu_t *);
void print_biu_access_count(biu_t *,FILE *);
void set_current_cpu_time(biu_t *, tick_t);				/* for use with testing harness */
void set_cpu_frequency(biu_t *,int);
int  get_cpu_frequency(biu_t *);
tick_t get_current_cpu_time(biu_t *);

void set_transaction_selection_policy(biu_t *,int);
int  get_transaction_selection_policy(biu_t *);

void set_last_transaction_type(biu_t *,int);
int  get_previous_transaction_type(biu_t *);
int get_biu_queue_depth (biu_t *this_b);

int next_RBRR_RAS_in_biu(biu_t *this_b, int rank_id, int bank_id);
/* cms specific stuff */

int  get_thread_id(biu_t *, int);
void set_thread_count(biu_t *, int);
int  get_thread_count(biu_t *);

void scrub_biu(biu_t *,int);
void biu_set_mem_cfg(biu_t*this_b,dram_system_configuration_t * cfg_ptr);
#ifdef ALPHA_SIM
int is_biu_busy(biu_t* this_b);
#else
bool is_biu_busy(biu_t* this_b);
#endif
void gather_biu_slot_stats(biu_t*);

extern int shmid;
extern int semid;
extern int thread_id;

extern biu_t *global_biu;

