/*
 * mem-amb-buffer.c - Portion of the dram simulator which tracks and updates
 * the occupancy state of the AMB buffer. Note this is required only for
 * FB-DIMM code.
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
#include "mem-system.h"
#endif
#ifndef BIU_H
#include "mem-biu.h"
#endif

/* 
 * The DRAM system is a globally observable object.
 */

extern dram_system_t 		dram_system;
extern biu_t			*global_biu;

int has_up_buffer (int chan_id, int rank_id, int bank_id, uint64_t tid) {
  int i;
  dram_controller_t *this_dc;
  this_dc = &(dram_system.dram_controller[chan_id]);
  assert(chan_id >= 0 && chan_id < dram_system.config.channel_count);
  for (i = 0; i < dram_system.config.up_buffer_cnt; i++) {
	if (this_dc->rank[rank_id].my_dimm->up_buffer[i].occupied == TRUE && this_dc->rank[rank_id].my_dimm->up_buffer[i].tran_id == tid) {
	  return TRUE;
	}
  }
  return false;
}

int has_down_buffer (int chan_id, int rank_id, int bank_id, uint64_t tid) {
  int i;
  dram_controller_t *this_dc;
  this_dc = &(dram_system.dram_controller[chan_id]);
  assert(chan_id >= 0 && chan_id < dram_system.config.channel_count);
  assert(this_dc);
  assert(rank_id >=0 && rank_id < dram_system.config.rank_count);
  for (i = 0; i < dram_system.config.down_buffer_cnt; i++) {
	if(this_dc->rank[rank_id].my_dimm->down_buffer[i].occupied == TRUE && this_dc->rank[rank_id].my_dimm->down_buffer[i].tran_id == tid) { 
	  return TRUE;
	}
  }
  return FALSE;

}

int is_up_buffer_free (int chan_id, int rank_id, int bank_id, uint64_t tid) {
	int i;
	dram_controller_t *this_dc;
	this_dc = &(dram_system.dram_controller[chan_id]);
	 assert(chan_id >= 0 && chan_id < dram_system.config.channel_count);
	for (i = 0; i < dram_system.config.up_buffer_cnt; i++) {
		if ( (this_dc->rank[rank_id].my_dimm->up_buffer[i].occupied == FALSE) || 
			(this_dc->rank[rank_id].my_dimm->up_buffer[i].occupied == TRUE && this_dc->rank[rank_id].my_dimm->up_buffer[i].tran_id == tid)) {
			return TRUE;
		}
	}
#ifdef DEBUG
	if (get_tran_watch(tid) ) {
	   		fprintf(stdout,"[%llu]DATA (%d) amb not available  ",dram_system.current_dram_time, tid); 
			print_buffer_contents(chan_id,rank_id);
	}
#endif
	 
	return FALSE;
}

int is_down_buffer_free (int chan_id, int rank_id, int bank_id, uint64_t tid) {
	int i;
	dram_controller_t *this_dc;
	this_dc = &(dram_system.dram_controller[chan_id]);
	 assert(chan_id >= 0 && chan_id < dram_system.config.channel_count);
	 assert(this_dc);
	 assert(rank_id >=0 && rank_id < dram_system.config.rank_count);
	for (i = 0; i < dram_system.config.down_buffer_cnt; i++) {
	  
		if ((this_dc->rank[rank_id].my_dimm->down_buffer[i].occupied == FALSE ) || 
			(this_dc->rank[rank_id].my_dimm->down_buffer[i].occupied == TRUE && this_dc->rank[rank_id].my_dimm->down_buffer[i].tran_id == tid)) { 
		  return TRUE;
		}
	}
	return FALSE;
}

int get_open_up_buffer (int chan_id, int rank_id, int bank_id, uint64_t tid) {
	int i;
	dram_controller_t *this_dc;
	this_dc = &(dram_system.dram_controller[chan_id]);
	 assert(chan_id >= 0 && chan_id < dram_system.config.channel_count);
	// first see if you already have an open buffer
	for (i = 0; i < dram_system.config.up_buffer_cnt; i++) {
		if (this_dc->rank[rank_id].my_dimm->up_buffer[i].occupied == TRUE && this_dc->rank[rank_id].my_dimm->up_buffer[i].tran_id == tid) {
			return i;
		}
	}
	// if not, find the first free one
	for (i = 0; i < dram_system.config.up_buffer_cnt; i++) {
		if (this_dc->rank[rank_id].my_dimm->up_buffer[i].occupied == FALSE) {
			return i;
		}
	}
	return -1;
}

int get_open_down_buffer (int chan_id, int rank_id, int bank_id, uint64_t tid) {
	int i;
	dram_controller_t *this_dc;
	this_dc = &(dram_system.dram_controller[chan_id]);
	// first see if you already have an open buffer
	 assert(chan_id >= 0 && chan_id < dram_system.config.channel_count);
	for (i = 0; i < dram_system.config.down_buffer_cnt; i++) {
		if (this_dc->rank[rank_id].my_dimm->down_buffer[i].occupied == TRUE && this_dc->rank[rank_id].my_dimm->down_buffer[i].tran_id == tid) {
			return i;
		}
	}
	// if not, find the first free one
	for (i = 0; i < dram_system.config.down_buffer_cnt; i++) {
		if ( this_dc->rank[rank_id].my_dimm->down_buffer[i].occupied == FALSE ) {
			return i;
		}
	}
	return -1;
}

void print_buffer_contents (int chan_id, int rank_id) {
	int i;
	dram_controller_t *this_dc;
	this_dc = &(dram_system.dram_controller[chan_id]);

	fprintf(stdout,"chan[%d]	rank[%d] free[%d] DOWN BUFFERS: ", chan_id, rank_id,this_dc->rank[rank_id].my_dimm->num_down_buffer_free);
	for (i = 0; i < dram_system.config.down_buffer_cnt; i++) {
		fprintf(stdout,"buff[%d]    occupied[%d]	tid[%d]		value[%d]	|", i, 
			this_dc->rank[rank_id].my_dimm->down_buffer[i].occupied,
			this_dc->rank[rank_id].my_dimm->down_buffer[i].tran_id,
			this_dc->rank[rank_id].my_dimm->down_buffer[i].value);
	}
	fprintf(stdout,"\nchan[%d]	rank[%d] free[%d] UP  BUFFERS: ", chan_id, rank_id,this_dc->rank[rank_id].my_dimm->num_up_buffer_free);
	for (i = 0; i < dram_system.config.up_buffer_cnt; i++) {
		fprintf(stdout,"buff[%d]	occupied[%d] tid[%d]		value[%d]	|", i, 
			this_dc->rank[rank_id].my_dimm->up_buffer[i].occupied,
			this_dc->rank[rank_id].my_dimm->up_buffer[i].tran_id,
			this_dc->rank[rank_id].my_dimm->up_buffer[i].value);
	}
	fprintf(stdout, "\n");
	fprintf(stdout, "-------------------------------------\n");
}

/********************
 * This function releases the down buffers.
 * This function is called when the Drive commands complete.
 * The buffer data held is set to zero and the associated transaction id to
 * invalid 
 * *******************/

void release_down_buffer (int chan_id, int rank_id, int bank_id, uint64_t tid) {
	int i;
	dram_controller_t *this_dc;
	 assert(chan_id >= 0 && chan_id < dram_system.config.channel_count);
	this_dc = &(dram_system.dram_controller[chan_id]);
	
	for (i = 0; i < dram_system.config.down_buffer_cnt; i++) {
		if (this_dc->rank[rank_id].my_dimm->down_buffer[i].tran_id == tid && 
			this_dc->rank[rank_id].my_dimm->down_buffer[i].occupied == TRUE) {
			this_dc->rank[rank_id].my_dimm->down_buffer[i].value = 0;
			this_dc->rank[rank_id].my_dimm->down_buffer[i].occupied = FALSE;
			this_dc->rank[rank_id].my_dimm->down_buffer[i].tran_id = 0;
			this_dc->rank[rank_id].my_dimm->num_down_buffer_free++;
			
			return;
		}
	}
#ifdef DEBUG
	 if (amb_buffer_debug()) {
	   fprintf(stdout,"Releasing down buffers tid %d\n",tid);
	   print_buffer_contents(chan_id, rank_id);
	 }
#endif
	 
}

/********************
 * This function releases the up buffers.
 * This function is called when the Drive commands complete.
 * The buffer data held is set to zero and the associated transaction id to
 * invalid 
 * *******************/

void release_up_buffer (int chan_id, int rank_id, int bank_id, uint64_t tid) {
	int i;
	dram_controller_t *this_dc;
	 assert(chan_id >= 0 && chan_id < dram_system.config.channel_count);
	this_dc = &(dram_system.dram_controller[chan_id]);
#ifdef DEBUG
	
	 if (amb_buffer_debug()) {
	   fprintf(stdout,"Releasing down buffers tid %d\n",tid);
	   print_buffer_contents(chan_id, rank_id);
	 }
#endif
	for (i = 0; i < dram_system.config.up_buffer_cnt; i++) {
		if (this_dc->rank[rank_id].my_dimm->up_buffer[i].tran_id == tid &&
		    this_dc->rank[rank_id].my_dimm->up_buffer[i].occupied == TRUE) {
			this_dc->rank[rank_id].my_dimm->up_buffer[i].value = 0;
			this_dc->rank[rank_id].my_dimm->up_buffer[i].occupied = FALSE;
			this_dc->rank[rank_id].my_dimm->up_buffer[i].tran_id = 0;
			this_dc->rank[rank_id].my_dimm->num_up_buffer_free++;
			return;
		}
	}
}

/**********************
 * This function locks a buffer to a command.
 * The buffer is associated with the command .
 * *******************/

void lock_buffer(command_t* this_c , int chan_id) {
   
  int buffer_id;
  dram_controller_t *this_dc;
  
  assert(chan_id >= 0 && chan_id < dram_system.config.channel_count);
  this_dc = &(dram_system.dram_controller[chan_id]);
  if (this_c->command == DATA) { //write fills up the up buffers
	buffer_id = get_open_up_buffer(chan_id, this_c->rank_id, this_c->bank_id, this_c->tran_id);
  	assert( buffer_id >= 0 && buffer_id < dram_system.config.up_buffer_cnt);
	this_dc->rank[this_c->rank_id].my_dimm->up_buffer[buffer_id].value += 1;
	this_dc->rank[this_c->rank_id].my_dimm->up_buffer[buffer_id].occupied = TRUE;
	this_dc->rank[this_c->rank_id].my_dimm->up_buffer[buffer_id].tran_id = this_c->tran_id;
	if (this_dc->rank[this_c->rank_id].my_dimm->up_buffer[buffer_id].value == 1) {
		this_dc->rank[this_c->rank_id].my_dimm->num_up_buffer_free--;
	}
  }
  else if (this_c->command == CAS || this_c->command == CAS_WITH_DRIVE) { //read fills up the down buffers
	buffer_id = get_open_down_buffer(chan_id, this_c->rank_id, this_c->bank_id, this_c->tran_id);
  	
	assert( (buffer_id >=0) && buffer_id < dram_system.config.down_buffer_cnt) ;
	
	this_dc->rank[this_c->rank_id].my_dimm->down_buffer[buffer_id].value += 1;
	this_dc->rank[this_c->rank_id].my_dimm->down_buffer[buffer_id].occupied = TRUE;
	this_dc->rank[this_c->rank_id].my_dimm->down_buffer[buffer_id].tran_id = this_c->tran_id;
	if (this_dc->rank[this_c->rank_id].my_dimm->down_buffer[buffer_id].value == 1)
		this_dc->rank[this_c->rank_id].my_dimm->num_down_buffer_free--;
  }
}
