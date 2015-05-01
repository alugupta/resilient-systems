/*
 * mem-bundle.c - This file contains all the routines which check if commands
 * for the FB-DIMM configuration can be scheduled into bundles or not.
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

int no_conflict_with_bundle_cmd(command_t * this_c,command_t *bundle[],int command_count);
void schedule_for_aging(int sched_policy,command_t *bundle[],int *command_count,tick_t now,int chan_id,char *debug_string) ;
void create_bundle(int sched_policy,command_t *bundle[],int *command_count,tick_t now,int chan_id,char *debug_string) ;
void create_lp_bundle(int sched_policy,command_t *bundle[],int *command_count,tick_t now,int chan_id,char *debug_string) ;
void create_RW_bundle(int sched_policy,command_t *bundle[],int *command_count,tick_t now,int chan_id,char *debug_string) ;
void schedule_trans_cmd_in_bundle(tick_t now,
	transaction_t *this_t,
	int tindex,
	command_t *bundle[],
	int *command_count,
	char *debug_string
	);

/*  This function will set up bundles and the timing
 *  related to the overall bundle.
 */

bool is_bank_open(int chan_id,int rank_id,int bank_id, int row_id) {
	if (dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].row_id == row_id &&
		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].ras_done_time > dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rp_done_time)
	  return true;
	else 
	  return false;
	
		
}

void commands2bundle(tick_t now, int chan_id,char * debug_string) 
{
  int i = 0;
  int command_count = 0;
  command_t *this_c;
  command_t *bundle[BUNDLE_SIZE]; /* bundle */
  /** First check if the bus is free **/
  if (up_bus_idle(chan_id) == FALSE)
	return;
  /** First reset all bundle commands to be null **/
  for ( i=0; i < BUNDLE_SIZE; i ++) {
	bundle[i] = NULL;
  }

  /** First check to see if a refresh can be issued **/
  if (dram_system.config.auto_refresh_enabled == TRUE ){ 
	command_t * rq_c;
	rq_c = dram_system.dram_controller[chan_id].refresh_queue.rq_head; 
	/* Traverse through the refresh queue and see if you can issue refresh
	 * commands in the same bundle **/
	while (command_count < BUNDLE_SIZE && rq_c != NULL ) {
	  this_c = rq_c;
	  while (command_count < BUNDLE_SIZE && this_c != NULL ) {
		if (this_c->status == IN_QUEUE && can_issue_refresh_command(rq_c,this_c)) {
		  bundle[command_count] = this_c;
		  command_count ++;
		  issue_fbd_command(now,this_c,chan_id,NULL,debug_string);
		}
		this_c = this_c->next_c;
	  }
	  rq_c = rq_c->rq_next_c;
	}
  }
  i = 0;

  // Now what ??
  int sched_policy = get_transaction_selection_policy(global_biu); 
  if (	 dram_system.dram_controller[chan_id].transaction_queue.transaction_count > 0 &&
	  (dram_system.current_dram_time - dram_system.dram_controller[chan_id].transaction_queue.entry[0].arrival_time > dram_system.config.arrival_threshold)) {
	  schedule_for_aging(sched_policy, bundle,&command_count,now,chan_id,debug_string);
  }
  if (sched_policy == GREEDY ) {
	create_bundle(sched_policy, bundle,&command_count,now,chan_id,debug_string);
  }else if (sched_policy == FCFS) {
	create_bundle(sched_policy, bundle,&command_count,now,chan_id,debug_string);
  }else if (sched_policy == LEAST_PENDING ) {
	create_lp_bundle(sched_policy, bundle,&command_count,now,chan_id,debug_string);
  }else if (sched_policy == MOST_PENDING ) {
	create_lp_bundle(sched_policy, bundle,&command_count,now,chan_id,debug_string);
  }else if (sched_policy == OBF) {
	create_bundle(sched_policy, bundle,&command_count,now,chan_id,debug_string);
	if (command_count < BUNDLE_SIZE)
		create_bundle(GREEDY, bundle,&command_count,now,chan_id,debug_string);
  }else if (sched_policy == RIFF) {
	create_RW_bundle(sched_policy, bundle,&command_count,now,chan_id,debug_string);
    }
  if (command_count != BUNDLE_SIZE) { /* we need to insert some no-ops */
  }
  if (command_count > 0) {
	issue_bundle(now, chan_id, bundle);
	mem_gather_stat(GATHER_BUNDLE_STAT, command_count);
	gather_extra_bundle_stats(bundle, command_count);
	update_amb_power_stats(bundle,command_count,now);
  }
  else { /** No commands or bundle to issue therefore bus is idle */
	dram_controller_t *this_dc;
	this_dc = &(dram_system.dram_controller[chan_id]);
	//this_dc->up_bus.completion_time = dram_system.current_dram_time;
  }
}

void issue_bundle(tick_t now, int chan_id, command_t *bundle[]) {
	dram_controller_t *this_dc;
	this_dc = &(dram_system.dram_controller[chan_id]);

	this_dc->bundle_id_ctr++;	
	this_dc->up_bus.status = BUS_BUSY;
	this_dc->up_bus.completion_time = dram_system.current_dram_time + dram_system.config.t_bundle;
#ifdef DEBUG
	if (bundle_debug()) {
		print_bundle(now,bundle,chan_id);
	}
#endif
}

int no_conflict_with_bundle_cmd(command_t * this_c,command_t *bundle[],int command_count) {
  int j;
  /*** Check if there is a conflict with a previous command
   * in the queue i.e. there should be no 2 commands i.e.
   * dram commands and not the data /drive commands , which
   * are for the same rank ***/
  for (j = 0; j< command_count; j++) {
	if (bundle[j] != NULL && 
		bundle[j]->chan_id == this_c->chan_id && /** Should be true ***/
		bundle[j]->rank_id == this_c->rank_id) {
	  switch ( bundle[j]->command ) {
		case REFRESH: 
		case PRECHARGE: 
		case PRECHARGE_ALL: 
		case CAS: 
		case CAS_WRITE:
		case RAS_ALL: 
		  if (this_c->command == RAS || this_c->command == PRECHARGE || 
			  this_c->command == RAS_ALL || this_c->command == PRECHARGE_ALL ||
			  this_c->command == CAS	|| this_c->command == CAS_WRITE ||
              this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_AND_PRECHARGE || this_c->command == CAS_WRITE_AND_PRECHARGE)
			return FALSE;
		  break;
		case RAS: if (this_c->command == RAS || this_c->command == PRECHARGE || 
					  this_c->command == RAS_ALL || this_c->command == PRECHARGE_ALL)
					return FALSE;
		  else if (this_c->command == CAS || this_c->command == CAS_WRITE || 
              this_c->command == CAS_WRITE_AND_PRECHARGE || this_c->command == CAS_AND_PRECHARGE) {
			/*** If posted cas then proceed else false **/
			if (dram_system.config.posted_cas_flag == TRUE) {
			  /** If to the same row and bank then allow .. else return
			   * false **/
			  if (!(bundle[j]->bank_id == this_c->bank_id && bundle[j]->row_id == this_c->row_id)) 
				return FALSE;
			}else
			  return FALSE;
			break;


		  }
	  }
	}
  }
  return TRUE;
}
/* This function checks if the first transaction needs to be explicitly
 * scheduled earlier
 */ 
void schedule_for_aging
(int sched_policy,command_t *bundle[],int *command_count,tick_t now,int chan_id,char *debug_string) {

  int i = 0;
  transaction_t *this_t;
  bool first_queued_transaction = false;

  if (dram_system.dram_controller[chan_id].transaction_queue.transaction_count > 0 
	  && (*command_count < BUNDLE_SIZE && !first_queued_transaction))  { 
	this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[0]);

	if (this_t->status != MEM_STATE_COMPLETED) 
	  /* go through each command in this tran to see if its
	   * a)schedulable (passes the "can_issue" test)
	   */
	  schedule_trans_cmd_in_bundle(now,this_t,i,bundle,command_count,debug_string);
  }
}



void create_bundle(int sched_policy,command_t *bundle[],int *command_count,tick_t now,int chan_id,char *debug_string) {

  int i = 0;
  transaction_t *this_t;
  bool first_queued_transaction = false;
  while (i < dram_system.dram_controller[chan_id].transaction_queue.transaction_count && (*command_count < BUNDLE_SIZE && !first_queued_transaction))  { 
	/* Things to remember:
	 * --data for writes are inserted like commands, just have to make sure to scedule them in groups of 2
	 * --scheduling multiple commands from the same transaction is only
	 *   allowed if going writes.
	 * --must use the "can_issue_x" functions
	 * --set the start time to 'now' and the command states to command issue? i think
	 * --dont forget to implement 'can_issue_drive' and 'can_issue_data'
	 * --whatever the channel width is, we must make sure that write data comes in even numbers, to keep 1 command + 2 data packet structure
	 * --are we interleaving write data? NO
	 * --posted cas must be taken into account
	 */
	this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[i]);
	if (sched_policy == FCFS && this_t->status == MEM_STATE_SCHEDULED) {
	  first_queued_transaction = TRUE;
	}

	if ((this_t->status != MEM_STATE_COMPLETED) 
		&& ((sched_policy == OBF && is_bank_open(this_t->address.chan_id,this_t->address.rank_id,this_t->address.bank_id,this_t->address.row_id)) || (sched_policy == FCFS) || (sched_policy == GREEDY))) {
	  /* go through each command in this tran to see if its
	   * a)schedulable (passes the "can_issue" test)
	   */
	  schedule_trans_cmd_in_bundle(now,this_t,i,bundle,command_count,debug_string);
	}
	i++;
  }
}
// Creates a bundle taking into account Read over Write etc policies
void create_RW_bundle(int sched_policy,command_t *bundle[],int *command_count,tick_t now,int chan_id,char *debug_string) {

  int i = 0;
  transaction_t *this_t;
  while (i < dram_system.dram_controller[chan_id].transaction_queue.transaction_count && (*command_count < BUNDLE_SIZE ))  { 
	/* Things to remember:
	 * --data for writes are inserted like commands, just have to make sure to scedule them in groups of 2
	 * --scheduling multiple commands from the same transaction is only
	 *   allowed if going writes.
	 * --must use the "can_issue_x" functions
	 * --set the start time to 'now' and the command states to command issue? i think
	 * --dont forget to implement 'can_issue_drive' and 'can_issue_data'
	 * --whatever the channel width is, we must make sure that write data comes in even numbers, to keep 1 command + 2 data packet structure
	 * --are we interleaving write data? NO
	 * --posted cas must be taken into account
	 */
	this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[i]);

	if ((this_t->status != MEM_STATE_COMPLETED)
       && (sched_policy == RIFF && this_t->transaction_type != MEMORY_WRITE_COMMAND)) { 
	  schedule_trans_cmd_in_bundle(now,this_t,i,bundle,command_count,debug_string);
	}
	i++;
  }
  i = 0;
  // Now schedule the other part
   while (i < dram_system.dram_controller[chan_id].transaction_queue.transaction_count && (*command_count < BUNDLE_SIZE ))  { 
	/* Things to remember:
	 * --data for writes are inserted like commands, just have to make sure to scedule them in groups of 2
	 * --scheduling multiple commands from the same transaction is only
	 *   allowed if going writes.
	 * --must use the "can_issue_x" functions
	 * --set the start time to 'now' and the command states to command issue? i think
	 * --dont forget to implement 'can_issue_drive' and 'can_issue_data'
	 * --whatever the channel width is, we must make sure that write data comes in even numbers, to keep 1 command + 2 data packet structure
	 * --are we interleaving write data? NO
	 * --posted cas must be taken into account
	 */
	this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[i]);

	if ((this_t->status != MEM_STATE_COMPLETED)
       && (sched_policy == RIFF && this_t->transaction_type == MEMORY_WRITE_COMMAND)) { 
	  schedule_trans_cmd_in_bundle(now,this_t,i,bundle,command_count,debug_string);
	}
	i++;
  }

}


void create_lp_bundle(int sched_policy,command_t *bundle[],int *command_count,tick_t now,int chan_id,char *debug_string) {

  int i = 0,j=0;
  transaction_t *this_t;
  command_t *this_c;
  for(j = 0;(j < dram_system.dram_controller[chan_id].transaction_queue.transaction_count) && (*command_count < BUNDLE_SIZE ); j++){
    this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[j];
    if (dram_system.dram_controller[chan_id].transaction_queue.entry[j].status == MEM_STATE_ISSUED ) {
      schedule_trans_cmd_in_bundle(now,this_t,j,bundle,command_count,debug_string);
    }
  }

  if (sched_policy == LEAST_PENDING)
	i= dram_system.dram_controller[chan_id].pending_queue.queue_size - 1;
  else
	i =0;
  while ( (*command_count < BUNDLE_SIZE && i>= 0 && i <dram_system.dram_controller[chan_id].pending_queue.queue_size  )) {
//  for (i= dram_system.dram_controller[chan_id].pending_queue.queue_size - 1; (*command_count < BUNDLE_SIZE && i>= 0);i--) 
	for (j=0;j< dram_system.dram_controller[chan_id].pending_queue.pq_entry[i].transaction_count && *command_count < BUNDLE_SIZE ;j++) {
	  int tindex = get_transaction_index(chan_id,dram_system.dram_controller[chan_id].pending_queue.pq_entry[i].transaction_list[j]);
	  this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[tindex];
	  this_c = this_t->next_c;


	/** Variables added to help measure command latency **/
	  if ((this_t->status != MEM_STATE_COMPLETED) ) {
		schedule_trans_cmd_in_bundle(now,this_t,tindex,bundle,command_count,debug_string);
	  }
	}
	if (sched_policy == LEAST_PENDING)
		i--;
	else
	  i++;
  }
}

void schedule_trans_cmd_in_bundle(tick_t now,
	transaction_t *this_t,
	int tindex,
	command_t *bundle[],
	int *command_count,
	char *debug_string
	){
  bool attempt_data_schedule = false, attempt_cmd_schedule = false,attempt_schedule = false;
  command_t *this_c = this_t->next_c;
  bool issue_tran = false;
  int chan_id = this_t->address.chan_id;
  while (this_c != NULL && *command_count < BUNDLE_SIZE) {
	if (this_c->status == IN_QUEUE )  {
	  attempt_schedule = true;

	  // No need to try to issue a data command for a transaction if
	  // already tried earlier  
	  if (((this_c->command == DATA && !attempt_data_schedule && *command_count <= (BUNDLE_SIZE - DATA_CMD_SIZE)) || this_c->command != DATA) &&
		  (can_issue_command(tindex, this_c) == TRUE ) ){
	  if (get_tran_watch(this_t->transaction_id	)) {
		print_command(now,this_c);
	  }
		if (this_c->command == DATA)
		  attempt_data_schedule = true;
		else
		  attempt_cmd_schedule = true;
		if (no_conflict_with_bundle_cmd(this_c,bundle,*command_count) == TRUE ) {

		  if (this_c->command == DATA) {
			  if (*command_count <= (BUNDLE_SIZE - DATA_CMD_SIZE)) { /* A data cmd occupies a fixed number of slots -> which we use as 2 but the macro allows us to change the protocol ?? ***/
				bundle[*command_count] = this_c;
				*command_count += DATA_CMD_SIZE;
				issue_fbd_command(now,this_c,chan_id,this_t,debug_string);
				this_t->issued_data = true; // this means it does have a buffer slot			 

			  }
			}
			else if (this_c->command == RAS && dram_system.config.posted_cas_flag == TRUE) {
			  /* must make sure there is room for a ras and a cas at
			   * the same time for the same read command
			   */
			  if ((this_c->next_c != NULL) && 
				  (this_c->next_c->command == CAS || this_c->next_c->command == CAS_WRITE || this_c->next_c->command == CAS_WITH_DRIVE ||
                   this_c->next_c->command == CAS_AND_PRECHARGE || this_c->next_c->command == CAS_WRITE_AND_PRECHARGE)) { //a read/write transition
                if ((*command_count )  <= (BUNDLE_SIZE - 2) ) { //if you can, scehdule them both
                  bundle[*command_count] = this_c;
                  bundle[*command_count+1] = this_c->next_c;
                  issue_fbd_command(now,this_c,chan_id,this_t,debug_string);;
                  this_c->next_c->posted_cas = TRUE;
                  issue_fbd_command(now,this_c->next_c,chan_id,this_t,debug_string);
                  this_c = this_c->next_c;
                  *command_count += 2;
                  if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE){
                    collapse_cmd_queue(this_t,this_c);
                  }
                  issue_tran = TRUE;
                } else {
                }

				/* else this should fall through ..... the can_issues
				 * will fail
				 */
			  }else { /** This command is only a ras i.e. activate a row ?? **/
				bundle[*command_count] = this_c;
				issue_fbd_command(now,this_c,chan_id,this_t,debug_string);;
				issue_tran = TRUE;
				(*command_count)++;
				if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE) {
				  collapse_cmd_queue(this_t,this_c);
				}
			  }
			}else {
			  bundle[*command_count] = this_c;
			  issue_fbd_command(now,this_c,chan_id,this_t,debug_string);;
			  if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE &&
				  (this_c->command != PRECHARGE )){
				collapse_cmd_queue(this_t,this_c);
				//				if (get_transaction_debug())
				//				  print_transaction(now,this_c->tran_id);
			  }
			  issue_tran = TRUE;
			  (*command_count)++;
			}
		  } else {
		  }
		}else{
		  if (this_c->command == DATA)
			attempt_data_schedule = true;
		  else
			attempt_cmd_schedule = true;

		}

	  }

	  this_c = this_c->next_c;
	}

}

