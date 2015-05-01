/*
 * mem-biu.c - simulates bus interface unit to memory controller/DRAM system
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

#include "mem-biu.h"

/* 
 * BIU initialization.
 */

biu_t *global_biu;

void init_biu(biu_t *this_b){
  int i;
  if(this_b == NULL) {
	this_b = (biu_t *)calloc(1,sizeof(biu_t));
  }
  global_biu = this_b;
  this_b->current_cpu_time = 0;
  this_b->bus_queue_depth = 0;
  for(i=0;i<MAX_BUS_QUEUE_DEPTH;i++){
	this_b->slot[i].status = MEM_STATE_INVALID;
  }
  this_b->delay = 1;
  this_b->debug_flag = FALSE;
  for(i=0;i<MEMORY_ACCESS_TYPES_COUNT;i++){
	this_b->access_count[i] = 0;
  }
  this_b->prefetch_biu_hit = 0;
  this_b->current_dram_time = 0;
  this_b->last_transaction_type = MEM_STATE_INVALID;
  this_b->max_write_burst_depth = 8;			/* sweep at most 8 writes at a time */
  this_b->write_burst_count = 0;
  this_b->biu_trace_fileptr = NULL;
  this_b->active_slots = 0;
  this_b->critical_word_rdy_slots = 0;
}

void biu_set_mem_cfg(biu_t*this_b,dram_system_configuration_t * cfg_ptr) {
  this_b->dram_system_config = cfg_ptr;
}
biu_t *get_biu_address(){
  return global_biu;
}

void set_biu_address(biu_t *this_b){		/* for use by cms-dram */
  global_biu = this_b;
}

void set_biu_depth(biu_t *this_b, int depth){
  this_b->bus_queue_depth = MAX(1,depth);
}

int  get_biu_depth(biu_t *this_b){
  return this_b->bus_queue_depth;
}

void set_biu_delay(biu_t *this_b, int delay_value){
  this_b->delay = MAX(0,delay_value);
}

int  get_biu_delay(biu_t *this_b){
  return this_b->delay;
}   

void  set_dram_chan_count_in_biu(biu_t *this_b, int count){
  //this_b->dram_system_config.channel_count = count;
}

void  set_dram_cacheline_size_in_biu(biu_t *this_b, int size){
  //this_b->dram_system_config.cacheline_size = size;
}

void  set_dram_chan_width_in_biu(biu_t *this_b, int width){
  //this_b->dram_system_config.channel_width = width;
}

void  set_dram_rank_count_in_biu(biu_t *this_b, int count){
  //this_b->dram_system_config.rank_count = count;
  this_b->last_rank_id = count - 1;
}

void  set_dram_bank_count_in_biu(biu_t *this_b, int count){
  //this_b->dram_system_config.bank_count = count;
  this_b->last_bank_id = count - 1;
}

void  set_dram_row_count_in_biu(biu_t *this_b, int count){
  //this_b->dram_system_config.row_count = count;
}

void  set_dram_col_count_in_biu(biu_t *this_b, int count){
  //this_b->dram_system_config.col_count = count;
}

void  set_dram_address_mapping_in_biu(biu_t *this_b, int policy){
  //this_b->dram_system_config.physical_address_mapping_policy = policy;
}

/***
 * This function sets the raito of the cpu 2 memory controller frequency
 * Note in FBDIMM only is the memory controller freq diff from the dram freq
 * Old function in Daves original code set_cpu_dram_frequency_ratio
 * **/
void set_cpu_memory_frequency_ratio(biu_t *this_b, int mem_frequency){
  int cpu_frequency;
  cpu_frequency = this_b->cpu_frequency;
  this_b->mem2cpu_clock_ratio  = (double) mem_frequency / (double) cpu_frequency;
  this_b->cpu2mem_clock_ratio  = (double) cpu_frequency / (double) mem_frequency;
}

/* 
 * If there is a prefetch mechanism, the prefetch request could be in the biu
 * slot already.  If there is an entry in the BIU already, then return the sid.
 * If the incoming request is a prefetch, and its priority is lower than the 
 * existing entry, do nothing, but still return sid.  If its priority is higher
 * than the existing entry, update and go on.
 */

int biu_hit_check(biu_t *this_b,
	int access_type, 
	unsigned int baddr, 
	int priority,
	int rid,
	tick_t now){

  int i, match_found;
  int bus_queue_depth;

  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  match_found = FALSE;
  for(i=0;i<bus_queue_depth && (match_found == FALSE);i++){
	if((this_b->slot[i].status != MEM_STATE_INVALID) && 
		(this_b->slot[i].address.physical_address == baddr) && 
		(this_b->slot[i].access_type == MEMORY_PREFETCH)){
	  if((access_type == MEMORY_IFETCH_COMMAND) || 
		  (access_type == MEMORY_READ_COMMAND)){

		this_b->slot[i].rid = rid;
		this_b->prefetch_biu_hit++;
		this_b->slot[i].access_type = access_type;
		this_b->slot[i].callback_done = FALSE;
		match_found = TRUE;
	  } else if (access_type == MEMORY_PREFETCH){
		if(priority < this_b->slot[i].priority){
		  this_b->slot[i].priority = priority;
		}
		match_found = TRUE;
	  }
	}
  }
  return match_found;
}

/* 
 *  Low Priority number == High priority.   Priority of 0 means non-speculative.
 *  This function finds a free slot in the BIU.
 */

int find_free_biu_slot(biu_t *this_b, int priority){
  int i,victim_id = MEM_STATE_INVALID,victim_found = false;
  int bus_queue_depth;

  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
 
  if (this_b->active_slots != bus_queue_depth) {
	
	for(i=0;i<bus_queue_depth;i++){
	  if(this_b->slot[i].status == MEM_STATE_INVALID){
		return i;
	  }
	}
  }
  /* Nothing found, now look for speculative prefetch entry to sacrifice */
  victim_found = FALSE;
  for(i=0;i<bus_queue_depth;i++){
	if((this_b->slot[i].access_type == MEMORY_PREFETCH) && 
		(this_b->slot[i].status == MEM_STATE_VALID)){
	  if(victim_found){
		if(this_b->slot[i].priority > this_b->slot[victim_id].priority){	/* higher priority number has less priority */
		  victim_id = i;
		}
	  } else {
		victim_found = TRUE;
		victim_id = i;
	  }
	}
  } 
  if((victim_found == TRUE) && 
	  ((priority == MEM_STATE_INVALID) || (priority < this_b->slot[victim_id].priority))){
	this_b->active_slots--;
	return victim_id;	/* kill the speculative prefetch */
  } else {
	return MEM_STATE_INVALID;		/* didn't find anything */
  }
}

/* 
 *  Note Put the entry into BIU slot with slot id 
 *  Note that the fill_biu_slot definition has references to the driver code.
 *  To interface to the simulator you will no longer need to modify this
 *  function definition 
 */

void fill_biu_slot(biu_t *this_b,
	int slot_id,
	int thread_id,
	tick_t now, 
	int rid, 
	unsigned int baddr, 
	int access_type,
	int priority,
#ifdef ALPHA_SIM 
	// Sim-alpha 
	struct _cache_access_packet *mp,
	RELEASE_FN_TYPE callback_fn 
#endif
#ifdef SIM_MASE
	// Used by Dave's DRAM MEM-TEST file
	// And MASE!
	void *mp,
	void (*callback_fn)(int rid, int lat)
#endif
#ifdef MEM_TEST
	// Used by Dave's DRAM MEM-TEST file
	// And MASE!
	void *mp,
	RELEASE_FN_TYPE callback_fn 
#endif
#ifdef GEMS
	// Used by Dave's DRAM MEM-TEST file
	// And MASE!
	void *mp,
	RELEASE_FN_TYPE callback_fn 
#endif
	){
 int now_high;
 int now_low;
#ifdef SIM_MASE
 int i;
#endif

  if( biu_debug(this_b)){ 
	fprintf(stdout,"BIU: acquire sid [%2d] rid [%3d] tid[%3d] access_type[", slot_id, rid,thread_id);
	print_access_type(access_type,stdout);
	fprintf(stdout,"] addr [0x%8X] Now[%d]\n", baddr, (int)now);
  }
  if(biu_fixed_latency(this_b)){	/* fixed latency, no dram */
	this_b->slot[slot_id].status = MEM_STATE_COMPLETED;
	this_b->slot[slot_id].start_time = now + get_biu_delay(this_b);
  } else {	/* normal mode */
	this_b->slot[slot_id].status = MEM_STATE_VALID;
	this_b->slot[slot_id].start_time = now;
  }
  this_b->slot[slot_id].thread_id = thread_id;
  this_b->slot[slot_id].rid = rid;
#ifdef SIM_MASE
  for(i=0;i<this_b->bus_queue_depth;i++){   /* MASE BUG. Spits out same address, so we're going to offset it */
	if((i != slot_id) &&
		(this_b->slot[slot_id].address.physical_address == this_b->slot[i].address.physical_address) &&
		(this_b->slot[slot_id].access_type) == (this_b->slot[i].access_type)){
	  this_b->slot[slot_id].address.physical_address += this_b->dram_system_config->cacheline_size;
	}
  }
#endif
  this_b->slot[slot_id].address.physical_address = (unsigned int)baddr;
  convert_address(this_b->dram_system_config->physical_address_mapping_policy,
	  this_b->dram_system_config,
	  &(this_b->slot[slot_id].address));
  if(thread_id != -1){ // Fake Multi-processor workloads
	this_b->slot[slot_id].address.bank_id = (this_b->slot[slot_id].address.bank_id + thread_id) %
	  this_b->dram_system_config->bank_count;
  }
  this_b->slot[slot_id].access_type = access_type;
  this_b->slot[slot_id].critical_word_ready = FALSE;
  this_b->slot[slot_id].callback_done = FALSE;
  this_b->slot[slot_id].priority = priority;
  this_b->slot[slot_id].mp = mp;
  this_b->slot[slot_id].callback_fn = callback_fn;
  // Update the stats
  this_b->active_slots++;
  // Update the time for the last access and gather stats
  mem_gather_stat(GATHER_BIU_ACCESS_DIST_STAT,now - this_b->last_transaction_time);
  if(this_b->biu_trace_fileptr != NULL){
	fprintf(this_b->biu_trace_fileptr,"0x%8X ",baddr);
	switch(access_type){
	  case MEMORY_UNKNOWN_COMMAND:
		fprintf(this_b->biu_trace_fileptr,"UNKNOWN");
		break;
	  case MEMORY_IFETCH_COMMAND:
		fprintf(this_b->biu_trace_fileptr,"IFETCH ");
		break;
	  case MEMORY_WRITE_COMMAND:
		fprintf(this_b->biu_trace_fileptr,"WRITE  ");
		break;
	  case MEMORY_READ_COMMAND:
		fprintf(this_b->biu_trace_fileptr,"READ   ");
		break;
	  case MEMORY_PREFETCH:
		fprintf(this_b->biu_trace_fileptr,"P_FETCH");
		break;
	  default:
		fprintf(this_b->biu_trace_fileptr,"UNKNOWN");
		break;
	}
	now_high = (int)(now / 1000000000);
	now_low = (int)(now - (now_high * 1000000000));
	if(now_high != 0){
	  fprintf(this_b->biu_trace_fileptr," %d%09d\n",now_high,now_low);
	} else {
	  fprintf(this_b->biu_trace_fileptr," %d\n",now_low);
	}
  }
}
/*
 * When a BIU slot is to be released, we know that the data has fully returned. 
 */

void release_biu_slot(biu_t *this_b,  int sid){
  if(biu_debug(this_b)){
	fprintf(stdout,"BIU: release sid [%2d] rid [%3d] access_type[", sid, this_b->slot[sid].rid);
	print_access_type(this_b->slot[sid].access_type,stdout);
	fprintf(stdout,"] addr [0x%8X]\n", this_b->slot[sid].address.physical_address);
  }

  this_b->slot[sid].status = MEM_STATE_INVALID; 
  this_b->slot[sid].thread_id = MEM_STATE_INVALID;
  this_b->slot[sid].rid = MEM_STATE_INVALID; 
  this_b->access_count[this_b->slot[sid].access_type]++; 
  this_b->slot[sid].access_type = MEM_STATE_INVALID; 
  this_b->slot[sid].critical_word_ready = FALSE;
  this_b->slot[sid].callback_done = FALSE;
  this_b->slot[sid].callback_fn = NULL;
  this_b->active_slots--;
}

/* look in the bus queue to see if there are any requests that needs to be serviced
 * with a specific rank, bank and request type, if yes, return TRUE, it not, return FALSE
 */

int next_RBRR_RAS_in_biu(biu_t *this_b, int rank_id, int bank_id){
  int i,bus_queue_depth;
  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  for(i=0;i<bus_queue_depth;i++){
	if((this_b->slot[i].status == MEM_STATE_VALID) &&
		((this_b->slot[i].access_type == MEMORY_READ_COMMAND) ||
		 (this_b->slot[i].access_type == MEMORY_IFETCH_COMMAND)) &&
		(this_b->slot[i].address.bank_id == bank_id) &&
		(this_b->slot[i].address.rank_id == rank_id) &&
		((this_b->current_cpu_time - this_b->slot[i].start_time) > this_b->delay)){
	  return i;
	}
  }
  return MEM_STATE_INVALID;
}


/* look in the bus queue to see if there are any requests that needs to be serviced 
 * Part of the logic exists to ensure that the "slot" only becomes active to the 
 * DRAM transaction scheduling mechanism after this_b->delay has occured.
 */

int get_next_request_from_biu(biu_t *this_b){
  int 	i,found,candidate_id = MEM_STATE_INVALID;
  tick_t	candidate_time = 0;
  int	candidate_priority;
  int	bus_queue_depth;
  int 	priority_scheme;
  int	last_transaction_type;

  priority_scheme = this_b->transaction_selection_policy;
  last_transaction_type = this_b->last_transaction_type;

  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  found = FALSE;
  if(priority_scheme == HSTP){
	for(i=0;i<bus_queue_depth;i++){
	  if((this_b->slot[i].status == MEM_STATE_VALID) && 
		  ((this_b->current_cpu_time - this_b->slot[i].start_time) > this_b->delay)){
		if(found == FALSE){
		  found = TRUE;
		  candidate_priority = this_b->slot[i].priority;
		  candidate_time = this_b->slot[i].start_time;
		  candidate_id = i;
		} else if(this_b->slot[i].priority < candidate_priority){
		  candidate_priority = this_b->slot[i].priority;
		  candidate_time = this_b->slot[i].start_time;
		  candidate_id = i;
		} else if(this_b->slot[i].priority == candidate_priority){
		  if(this_b->slot[i].start_time < candidate_time){
			candidate_priority = this_b->slot[i].priority;
			candidate_time = this_b->slot[i].start_time;
			candidate_id = i;				    
		  }
		}
	  }
	}
	if(found == TRUE){
	  return candidate_id;
	} else {
	  return MEM_STATE_INVALID;
	}
  }else {
	  for(i=0;i<bus_queue_depth;i++){
		if((this_b->slot[i].status == MEM_STATE_VALID) && 
			((this_b->current_cpu_time - this_b->slot[i].start_time) > this_b->delay)){
		  if(found == FALSE){
			found = TRUE;
			candidate_time = this_b->slot[i].start_time;
			candidate_id = i;
		  } else if(this_b->slot[i].start_time < candidate_time){
			candidate_time = this_b->slot[i].start_time;
			candidate_id = i;
		  }
		}
	  }
	  if(found == TRUE){
		return candidate_id;
	  } else {
		return MEM_STATE_INVALID;
	  }
	}
  return MEM_STATE_INVALID;
}

/* look in the bus queue to see if there are any requests that needs to be serviced 
 * Part of the logic exists to ensure that the "slot" only becomes active to the 
 * DRAM transaction scheduling mechanism after this_b->delay has occured.
 * This mechanism ensures that the biu can move transactions based on commands
 * - address translation is done in the biu. Assuming biu and tq are on the
 *   smae structure - 
 */

int get_next_request_from_biu_chan(biu_t *this_b,int chan_id){
  int 	i,found,candidate_id = MEM_STATE_INVALID;
  tick_t	candidate_time = 0;
  int	candidate_priority;
  int	bus_queue_depth;
  int 	priority_scheme;
  int	last_transaction_type;

  priority_scheme = this_b->transaction_selection_policy;
  last_transaction_type = this_b->last_transaction_type;

  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  found = FALSE;
  if(priority_scheme == HSTP){
	for(i=0;i<bus_queue_depth;i++){
	  if((this_b->slot[i].status == MEM_STATE_VALID) &&
         (this_b->slot[i].address.chan_id == chan_id) && 
		  ((this_b->current_cpu_time - this_b->slot[i].start_time) > this_b->delay)){
		if(found == FALSE){
		  found = TRUE;
		  candidate_priority = this_b->slot[i].priority;
		  candidate_time = this_b->slot[i].start_time;
		  candidate_id = i;
		} else if(this_b->slot[i].priority < candidate_priority &&
         this_b->slot[i].address.chan_id == chan_id){  
		  candidate_priority = this_b->slot[i].priority;
		  candidate_time = this_b->slot[i].start_time;
		  candidate_id = i;
		} else if(this_b->slot[i].priority == candidate_priority &&
         this_b->slot[i].address.chan_id == chan_id){  
		  if(this_b->slot[i].start_time < candidate_time){
			candidate_priority = this_b->slot[i].priority;
			candidate_time = this_b->slot[i].start_time;
			candidate_id = i;				    
		  }
		}
	  }
	}
	if(found == TRUE){
	  return candidate_id;
	} else {
	  return MEM_STATE_INVALID;
	}
  }else {
	  for(i=0;i<bus_queue_depth;i++){
		if((this_b->slot[i].status == MEM_STATE_VALID) && 
           (this_b->slot[i].address.chan_id == chan_id) &&
			((this_b->current_cpu_time - this_b->slot[i].start_time) > this_b->delay)){
		  if(found == FALSE){
			found = TRUE;
			candidate_time = this_b->slot[i].start_time;
			candidate_id = i;
		  } else if(this_b->slot[i].start_time < candidate_time &&
           (this_b->slot[i].address.chan_id == chan_id)){ 
			candidate_time = this_b->slot[i].start_time;
			candidate_id = i;
		  }
		}
	  }
	  if(found == TRUE){
		return candidate_id;
	  } else {
		return MEM_STATE_INVALID;
	  }
	}
  return MEM_STATE_INVALID;
}


int bus_queue_status_check(biu_t *this_b, int thread_id){	
  int i;
  int bus_queue_depth;

  bus_queue_depth = this_b->bus_queue_depth;
  if(thread_id == MEM_STATE_INVALID){ 	/* for mase code */
	for(i=0;i<bus_queue_depth;i++){		/* Anything in biu, DRAM must service */
	  if((this_b->slot[i].status == MEM_STATE_VALID) || (this_b->slot[i].status == MEM_STATE_SCHEDULED)){
		return BUSY;
	  }
	}
  } else { /* for cms code */
	for(i=0;i<bus_queue_depth;i++){
	  if((this_b->slot[i].thread_id == thread_id) &&
		  ((this_b->slot[i].status == MEM_STATE_VALID) || (this_b->slot[i].status == MEM_STATE_SCHEDULED))){
		return BUSY;
	  }
	}
	if(num_free_biu_slot(this_b) <= 5){      /* hard coded... */
	  return BUSY;            /* even if I have no requests outstanding, if the bus queue is filled up, it needs to be serviced */
	}
  }
  return IDLE;
}

int dram_update_required(biu_t *this_b, tick_t current_cpu_time){
  tick_t expected_dram_time;
  double mem2cpu_clock_ratio;

  mem2cpu_clock_ratio = this_b->mem2cpu_clock_ratio;

  expected_dram_time = (tick_t) ((double)current_cpu_time * mem2cpu_clock_ratio);
  /*
   *                 fprintf(stdout,"cpu[%d] dram[%d] expected [%d]\n",
   *                 		(int)this_b->current_cpu_time,
   *                 		(int)this_b->current_dram_time,
   *                 		(int)expected_dram_time);
   *                 
   */            
  if((expected_dram_time - this_b->current_dram_time) > 1){
	return TRUE;
  } else {
	return FALSE;
  }
}

int find_critical_word_ready_slot(biu_t *this_b, int thread_id){
  int i;
  int latency;
  int bus_queue_depth;

  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);

  if (this_b->active_slots == 0)
	return MEM_STATE_INVALID; 
  if (this_b->critical_word_rdy_slots== 0)
	return MEM_STATE_INVALID; 

  for(i=0;i<bus_queue_depth;i++){
	if((this_b->slot[i].thread_id == thread_id) &&
		(this_b->slot[i].critical_word_ready == TRUE) && 
		(this_b->slot[i].callback_done == FALSE)) {
      this_b->critical_word_rdy_slots--; // Keeps track of those whose callback you want to do
	  this_b->slot[i].callback_done = TRUE;
	  latency = (int) (this_b->current_cpu_time - this_b->slot[i].start_time);
	  if((this_b->slot[i].access_type == MEMORY_IFETCH_COMMAND) || 
		  (this_b->slot[i].access_type == MEMORY_READ_COMMAND)){	/* gather stat for critical reads only */
		mem_gather_stat(GATHER_BUS_STAT, latency);
		if(biu_debug(this_b)){
		  fprintf(stdout,"BIU: Critical Word Received sid [%d] rid [%3d] access_type[",
			  i,
			  this_b->slot[i].rid); 
		  print_access_type(this_b->slot[i].access_type,stdout);
		  fprintf(stdout,"] addr [0x%8X] Now[%d] Latency[%4d]\n",
			  this_b->slot[i].address.physical_address,
			  (int)this_b->current_cpu_time,
			  (int)(this_b->current_cpu_time-this_b->slot[i].start_time));
		}
	  }
	  return i;
	}
  }
  return MEM_STATE_INVALID;
}
int find_completed_slot(biu_t *this_b, int thread_id, tick_t now){
  int i;
  int access_type,latency;
  int bus_queue_depth;

  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  
  if (this_b->active_slots == 0)
	return MEM_STATE_INVALID; 
  for(i=0;i<bus_queue_depth;i++){
	if((this_b->slot[i].status == MEM_STATE_COMPLETED) && (this_b->slot[i].thread_id == thread_id)){
	  access_type = get_access_type(this_b,i);
	  latency = (int) (this_b->current_cpu_time - this_b->slot[i].start_time);
	  if((callback_done(this_b,i) == FALSE) && ((access_type == MEMORY_IFETCH_COMMAND) || (access_type == MEMORY_READ_COMMAND))){
		mem_gather_stat(GATHER_BUS_STAT, latency);
	  }
	  return i;
	}
  }
  return MEM_STATE_INVALID;
}
/*
 * Determine the Number of free biu slots currently available 
 * Used by simulators to probe for MEM_RETRY situations
 */
int num_free_biu_slot(biu_t *this_b){
  int i,j;
  int bus_queue_depth;

  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  for(i=0,j=0;i<bus_queue_depth;i++){
	if(this_b->slot[i].status == MEM_STATE_INVALID){
	  j++;
	}
  }
  return j;  
}

int get_rid(biu_t *this_b, int slot_id){
  return this_b->slot[slot_id].rid;
}

int get_access_type(biu_t *this_b, int slot_id){
  return this_b->slot[slot_id].access_type;
}

tick_t get_start_time(biu_t *this_b, int slot_id){
  return this_b->slot[slot_id].start_time;
}

unsigned int get_virtual_address(biu_t *this_b, int slot_id){
  return this_b->slot[slot_id].address.virtual_address;
}

unsigned int get_physical_address(biu_t *this_b, int slot_id){
  return this_b->slot[slot_id].address.physical_address;
}

void set_critical_word_ready(biu_t *this_b, int slot_id){
  this_b->critical_word_rdy_slots++;
  this_b->slot[slot_id].critical_word_ready = TRUE;
}

int critical_word_ready(biu_t *this_b, int slot_id){
  return this_b->slot[slot_id].critical_word_ready;
}

void set_biu_slot_status(biu_t *this_b, int slot_id, int status){
  this_b->slot[slot_id].status = status;
}

int get_biu_slot_status(biu_t *this_b, int slot_id){
  return this_b->slot[slot_id].status;
}

int callback_done(biu_t *this_b, int sid){
  return this_b->slot[sid].callback_done;
}
/**
 * Needs to go : retained for legacy interfaces 
 */
void biu_invoke_callback_fn(biu_t *this_b, int slot_id) {

#ifdef ALPHA_SIM 
  if (!this_b->slot[slot_id].callback_fn) { 
	if( this_b->slot[slot_id].mp ) {
	  cache_free_access_packet( this_b->slot[slot_id].mp );
	}
	return;
  }
  this_b->slot[slot_id].callback_fn( sim_cycle, this_b->slot[slot_id].mp->obj, this_b->slot[slot_id].mp->stamp);
  cache_free_access_packet( this_b->slot[slot_id].mp );
#endif
#ifdef SIM_MASE
  // Used by Dave's MEM Test file
  // And MASE
   if (!this_b->slot[slot_id].callback_fn) return;
 	 this_b->slot[slot_id].callback_fn(this_b->slot[slot_id].rid, 1);
#endif
#ifdef GEMS

	 this_b->slot[slot_id].callback_fn(this_b->slot[slot_id].rid);
#endif
}

RELEASE_FN_TYPE biu_get_callback_fn(biu_t *this_b,int slot_id) {
 return this_b->slot[slot_id].callback_fn;
}

void *biu_get_access_sim_info(biu_t *this_b,int slot_id) {
//  return this_b->slot[slot_id].
  return NULL;
}
void set_biu_fixed_latency(biu_t *this_b, int flag_status){
  this_b->fixed_latency_flag = flag_status;
}

int  biu_fixed_latency(biu_t *this_b){
  return this_b->fixed_latency_flag;
}

void set_biu_debug(biu_t *this_b, int debug_status){
  this_b->debug_flag = debug_status;
}

int biu_debug(biu_t *this_b){
  return this_b->debug_flag;
}

void print_access_type(int type,FILE *fileout){
  switch(type){
	case MEMORY_UNKNOWN_COMMAND:
	  fprintf(fileout,"UNKNOWN");
	  break;
	case MEMORY_IFETCH_COMMAND:
	  fprintf(fileout,"IFETCH ");
	  break;
	case MEMORY_WRITE_COMMAND:
	  fprintf(fileout,"WRITE  ");
	  break;
	case MEMORY_READ_COMMAND:
	  fprintf(fileout,"READ   ");
	  break;
	case MEMORY_PREFETCH:
	  fprintf(fileout,"P_FETCH");
	  break;
	default:
	  fprintf(fileout,"UNKNOWN");
	  break;
  }
}

void print_biu(biu_t *this_b){
  int i;
  int bus_queue_depth;

  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  for(i=0;i<bus_queue_depth;i++){
	if(this_b->slot[i].status != MEM_STATE_INVALID){
	  fprintf(stdout,"Entry[%2d] Status[%2d] Rid[%8d] Start_time[%8d] Addr[0x%8X] ",
		  i, this_b->slot[i].status, this_b->slot[i].rid, 
		  (int)this_b->slot[i].start_time, 
		  this_b->slot[i].address.physical_address);
	  print_access_type(this_b->slot[i].access_type,stdout);
	  fprintf(stdout,"\n");
	}
  }
}

void print_biu_access_count(biu_t *this_b,FILE *fileout){
  int i;
  int total_count;
  total_count = 1;
  if (fileout == NULL)
	fileout =stdout;
  for(i=0;i<MEMORY_ACCESS_TYPES_COUNT;i++){
	total_count += this_b->access_count[i];
  }
  fprintf(fileout,"TOTAL   Count = [%8d]  percentage = [100.00]\n",total_count);
  for(i=0;i<MEMORY_ACCESS_TYPES_COUNT;i++){
	print_access_type(i,fileout);
	fprintf(fileout," Count = [%8d]  percentage = [%5.2lf]\n",
		this_b->access_count[i],
		100.0 * this_b->access_count[i]/ (double) total_count);
  }
  fprintf(fileout,"\n");
}

tick_t get_current_cpu_time(biu_t *this_b){
  return this_b->current_cpu_time;
}

void set_current_cpu_time(biu_t *this_b, tick_t current_cpu_time){
  this_b->current_cpu_time = current_cpu_time;
}

void set_cpu_frequency(biu_t *this_b, int freq){
  if(freq < MIN_CPU_FREQUENCY){
	this_b->cpu_frequency	= MIN_CPU_FREQUENCY;
	fprintf(stdout,"\n\n\n\n\n\n\n\nWARNING: CPU frequency set to minimum allowed frequency [%d] MHz\n\n\n\n\n\n",this_b->cpu_frequency);
  } else if (freq > MAX_CPU_FREQUENCY){
	this_b->cpu_frequency	= MAX_CPU_FREQUENCY;
	fprintf(stdout,"\n\n\n\n\n\n\n\nWARNING: CPU frequency set to maximum allowed frequency [%d] MHz\n\n\n\n\n\n",this_b->cpu_frequency);
  } else {
	this_b->cpu_frequency        = freq; 
  }
}

int get_cpu_frequency(biu_t *this_b){
  return this_b->cpu_frequency;
}

void set_last_transaction_type(biu_t *this_b, int type){
  this_b->last_transaction_type = type;
}
int  get_last_transaction_type(biu_t *this_b){
  return this_b->last_transaction_type;
}

void set_transaction_selection_policy(biu_t *this_b, int policy){
  this_b->transaction_selection_policy = policy;
}
int  get_transaction_selection_policy(biu_t *this_b){
  return this_b->transaction_selection_policy;
}

/* As a thread exits, last thing it should do is to come here and clean of the biu with stuff that belongs to it */
/* CMS specific */

void scrub_biu(biu_t *this_b, int thread_id){
  int i;
  int bus_queue_depth;
  bus_queue_depth = this_b->bus_queue_depth;
  assert (bus_queue_depth <= MAX_BUS_QUEUE_DEPTH);
  for(i=0;i<bus_queue_depth;i++){
	if(this_b->slot[i].thread_id == thread_id){
	  release_biu_slot(this_b,i);
	}
  }
}

int get_thread_id(biu_t *this_b, int slot_id){
  return this_b->slot[slot_id].thread_id;
}

void set_thread_count(biu_t *this_b, int thread_count){
  this_b->thread_count = thread_count;
}

int get_thread_count(biu_t *this_b){
  return this_b->thread_count;
}
bool is_biu_busy(biu_t* this_b){
  bool busy = false;
  if (this_b->active_slots > 0) {
	busy = true;
  }
  return busy;
}

void gather_biu_slot_stats(biu_t *this_b) {
  mem_gather_stat(GATHER_BIU_SLOT_VALID_STAT, this_b->active_slots);
}

int get_biu_queue_depth (biu_t *this_b){
    return this_b->bus_queue_depth;
}
