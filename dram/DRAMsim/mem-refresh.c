

/*** 
 * mem-refresh.c All code reqd for inserting refresh into the system is added here 
 * 
 * Note that the testing code is similar to the code in mem-commandissuetest.c for
 * ras/prefetch/ras_all/prefetch_all and may need to be integrated.
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

/**
 * Replace refresh command sequence with a singel refresh command.
 * Refresh can be issued when the bank is already precharged.
 * This is easily satisfied in closed page.
 * Open page policy may require insertion of an additional precharge command
 * prior to it.
 */

#ifndef MEMORY_SYSTEM_H
#include "mem-system.h"
#endif
#ifndef BIU_H
#include "mem-biu.h"
#endif

extern dram_system_t dram_system;
extern biu_t *global_biu;
addresses_t get_next_refresh_address (int chan_id) ;
command_t* build_refresh_cmd_queue(tick_t now ,addresses_t refresh_address,unsigned int refresh_id);
int can_issue_refresh_ras(command_t* this_rq_c, command_t *this_c);
int can_issue_refresh_prec(command_t* this_rq_c, command_t *this_c);
int can_issue_refresh_ras_all(command_t* this_rq_c, command_t *this_c);
int can_issue_refresh_prec_all(command_t* this_rq_c, command_t *this_c);
int can_issue_refresh_refresh_all(command_t *this_rq_c,command_t *this_c); 
int can_issue_refresh_refresh(command_t *this_rq_c,command_t *this_c); 
void remove_refresh_transaction(command_t *prev_rq_c, command_t* this_rq_c,int chan_id) ;
void update_tran_for_refresh(int chan_id) ;
/***
 * The refresh queue can have commands whose progress is not tracked because
 * the dram state gets updated only when there is a refresh command on the
 * queue. 
 * **/
void update_refresh_missing_cycles (tick_t now,int chan_id,char * debug_string) {
  int i;
 addresses_t refresh_address;
  if (now> (dram_system.config.refresh_cycle_count + dram_system.dram_controller[chan_id].refresh_queue.last_refresh_cycle +
		dram_system.config.t_rfc )){
	int num_refresh_missed = (now - dram_system.dram_controller[chan_id].refresh_queue.last_refresh_cycle)/dram_system.config.refresh_cycle_count;	
	for (i =0 ; i <num_refresh_missed;i++) {
	  refresh_address = get_next_refresh_address(chan_id);
	}

	dram_system.dram_controller[chan_id].refresh_queue.last_refresh_cycle = (dram_system.current_dram_time/dram_system.config.refresh_cycle_count)* dram_system.config.refresh_cycle_count;
	fprintf(stderr,"[%llu] Error: missing refreshes %d Last Refresh [%llu]\n",now,num_refresh_missed,dram_system.dram_controller[chan_id].refresh_queue.last_refresh_cycle);
	print_transaction_queue(dram_system.current_dram_time);
	exit(0);
  }
}
void update_refresh_status(tick_t now,int chan_id,char * debug_string) {
	/* Traverse through queue and update the status of every command **/
   command_t *temp_rq_c;
   command_t *prev_rq_c;
   command_t *temp_c;
   temp_rq_c = dram_system.dram_controller[chan_id].refresh_queue.rq_head;
   prev_rq_c = NULL;
   int done,remove_refresh_tran = FALSE;
   while (temp_rq_c != NULL) {
	 temp_c = temp_rq_c;
	 done = TRUE;
	 while (temp_c != NULL) {
	   switch (temp_c->command) {
		case PRECHARGE:
	      precharge_transition(temp_c, temp_c->tran_id, chan_id, temp_c->rank_id, temp_c->bank_id, temp_c->row_id, debug_string);
    	  break;
		case PRECHARGE_ALL:
	      precharge_all_transition(temp_c, temp_c->tran_id, chan_id, temp_c->rank_id, temp_c->bank_id, temp_c->row_id, debug_string);
    	  break;
		case REFRESH:
	      refresh_transition(temp_c, temp_c->tran_id, chan_id, temp_c->rank_id, temp_c->bank_id, temp_c->row_id, debug_string);
    	  break;
		case REFRESH_ALL:
	      refresh_all_transition(temp_c, temp_c->tran_id, chan_id, temp_c->rank_id, temp_c->bank_id, temp_c->row_id, debug_string);
    	  break;

	   }
	   if (temp_c->status != MEM_STATE_COMPLETED)
		 done = FALSE;
	 	temp_c= temp_c->next_c;
		
	 }
	 if (done == TRUE) {
	   remove_refresh_tran = TRUE;
	   remove_refresh_transaction(prev_rq_c,temp_rq_c,chan_id);
	   if (prev_rq_c == NULL)
		 temp_rq_c = dram_system.dram_controller[chan_id].refresh_queue.rq_head;
	   else
		 temp_rq_c = prev_rq_c->rq_next_c;
	 }else {
	     prev_rq_c = temp_rq_c;
		 temp_rq_c = temp_rq_c->rq_next_c;
	 }
   }
  //if ( remove_refresh_tran == TRUE && dram_system.config.row_buffer_management_policy == OPEN_PAGE)
  //	update_tran_for_refresh(chan_id);	
	
}

void remove_refresh_transaction(command_t *prev_rq_c, command_t* this_rq_c,int chan_id) {
	command_t *temp_c;
#ifdef DEBUG
	if (get_transaction_debug()) {
	  fprintf(stderr, "[%llu] Removing Refresh Transaction %llu\n",dram_system.current_dram_time,
		  this_rq_c->tran_id);
	}
#endif
	if (prev_rq_c == NULL) {/* First item in the queue */
	  dram_system.dram_controller[chan_id].refresh_queue.rq_head = this_rq_c->rq_next_c;
	} else {
		prev_rq_c->rq_next_c = this_rq_c->rq_next_c;
	}
	/** Return the commands to the command pool **/
	temp_c = this_rq_c;
	while(temp_c->next_c != NULL){
		temp_c = temp_c->next_c;
	}
	temp_c->next_c = dram_system.tq_info.free_command_pool;
	dram_system.tq_info.free_command_pool = this_rq_c;
	this_rq_c->rq_next_c = NULL;
	/** Update the queue size **/
	dram_system.dram_controller[chan_id].refresh_queue.size--;

}
// This should only add RAS/PREC if needed as refresh has changed the system
void update_tran_for_refresh(int chan_id) {
  int i,j;
  command_t *command_queue = NULL;
  command_t *temp_c;
  transaction_t *this_t;
  int bank_open,bank_conflict;
  addresses_t this_a;
  int bank_id,rank_id,row_id;
  int cmd_id;
  if (dram_system.config.row_buffer_management_policy == OPEN_PAGE) {
	// This should be traversed in the order that we will encounter it ??
	for (i=0;i<dram_system.dram_controller[chan_id].transaction_queue.transaction_count;i++) {
	  this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[i];
	  cmd_id = 0;
	  command_queue = NULL;
	  if ((this_t->status == MEM_STATE_VALID || 
			this_t->status == MEM_STATE_SCHEDULED || 
			this_t->status == MEM_STATE_DATA_ISSUED)) {
		bank_id = this_t->next_c->bank_id;
		rank_id = this_t->next_c->rank_id;
		row_id = this_t->next_c->row_id;
		/** Check the status of the bank based on current state **/
		if (dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].ras_done_time > 
			dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rp_done_time) {
		  /* => some row is open */
		  bank_open = TRUE;
		  if (dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].row_id == row_id) {
			bank_conflict = FALSE;
		  }else {
			bank_conflict = TRUE;
		  }
		}else { /* bank is precharged or precharging */
		  bank_open = FALSE;
		  bank_conflict = FALSE; 
		}
		//	   int sched_policy = get_transaction_selection_policy(global_biu);
		//	   This is the default state of the bank
		//	   obf/greedy/fcfs are together
		//	   mostpending/least pending are distinct
		bool to_same_bank_row = false;
		
		for (j=0;j<dram_system.dram_controller[chan_id].transaction_queue.transaction_count;j++) {
		  temp_c = dram_system.dram_controller[chan_id].transaction_queue.entry[j].next_c;
		  if (j<i && 
			   temp_c->rank_id ==rank_id && temp_c->bank_id == bank_id &&
			  temp_c->row_id == row_id)
			to_same_bank_row = true;

		  if (j!= i &&
			  temp_c->rank_id ==rank_id && temp_c->bank_id == bank_id &&
			  (j < i || (dram_system.dram_controller[chan_id].transaction_queue.entry[j].status != MEM_STATE_SCHEDULED &&
						 dram_system.dram_controller[chan_id].transaction_queue.entry[j].status != MEM_STATE_COMPLETED &&	
						 (this_t->status != MEM_STATE_SCHEDULED || (temp_c->row_id == row_id ) || !to_same_bank_row)))) {
			while(temp_c != NULL){
			  // RAS to same bank 
			  // if we are only scheduled not issued
			  // 
			  if ( (temp_c->command == RAS)){
				if(temp_c->row_id == row_id){
				  bank_open = TRUE;
				  bank_conflict = FALSE;
				} else if (temp_c->row_id != row_id){
				  bank_conflict = TRUE;
				  bank_open = FALSE;
				}
			  }else if ((temp_c->command == PRECHARGE)){
				bank_open = FALSE;
				if(temp_c->row_id == row_id){
				  bank_conflict = FALSE;
				} else if (temp_c->row_id != row_id){
				  bank_conflict = TRUE;
				}
			  } 
			  temp_c = temp_c->next_c;
			}
		  }
		}
		command_t *current_cmd_seq = this_t->next_c;
		command_t *prev_cmd = NULL;
		int num_ignored = 0;
		if (dram_system.config.dram_type == FBD_DDR2) {
		  while (current_cmd_seq != NULL && current_cmd_seq->command == DATA) {
			prev_cmd = current_cmd_seq;
			current_cmd_seq = current_cmd_seq->next_c;
			num_ignored++;
		  }
		}
		if (current_cmd_seq->command != PRECHARGE) { 
#ifdef DEBUG
		  if ((bank_conflict == TRUE || 
				(bank_conflict == FALSE && ((bank_open == FALSE && (current_cmd_seq->command != RAS)) ||
											(bank_open == TRUE && current_cmd_seq->command == RAS)))) &&	  
			  (get_transaction_debug() || get_tran_watch(current_cmd_seq->tran_id))) {
			fprintf(stderr,"Modifying Transaction (%llu) index (%d) bank conflict %d bank open %d my row %x actual row %x ras[%llu] rp [%llu]\n",current_cmd_seq->tran_id, i,
				bank_conflict,bank_open,
				dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].row_id,
				row_id,
				dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].ras_done_time,
				dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rp_done_time);
		  }
#endif
		  if (bank_conflict == TRUE ) {
			this_a.chan_id = chan_id;
			this_a.rank_id = rank_id;
			this_a.bank_id = bank_id;
			this_a.row_id =  row_id;
			command_queue = add_command (current_cmd_seq->start_time,
				command_queue,
				PRECHARGE,     
				&this_a,
				this_t->transaction_id,
				0,
				0,
				0);

			if (current_cmd_seq->command != RAS) 
			  command_queue = add_command (current_cmd_seq->start_time,
				  command_queue,
				  RAS,     
				  &this_a,
				  this_t->transaction_id,
				  0,
				  0,
				  0);
#ifdef DEBUG
			if (get_transaction_debug() || get_tran_watch(current_cmd_seq->tran_id)) {
			  fprintf(stderr,"Modified Transaction added PRE RAS\n");
			}
#endif
		  } else {
			if (bank_open == FALSE ) { // bankconflict = false - 
			  this_a.chan_id = chan_id;
			  this_a.rank_id = rank_id;
			  this_a.bank_id = bank_id;
			  this_a.row_id =  row_id;
			  if (current_cmd_seq->command != RAS) {
				command_queue = add_command (current_cmd_seq->start_time,
					command_queue,
					RAS,     
					&this_a,
					this_t->transaction_id,
					0,
					0,
					0);
			  }
#ifdef DEBUG
			  if (get_transaction_debug() || get_tran_watch(current_cmd_seq->tran_id)) {
				fprintf(stderr,"Modified Transaction added RAS\n");
			  }
#endif
			} 
		  }
		  if (command_queue != NULL) {
			temp_c = command_queue;
			while (temp_c->next_c != NULL) {
			  if (temp_c->command != DATA)
				temp_c->cmd_id = cmd_id++;
			  temp_c = temp_c->next_c;
			}
			if (prev_cmd == NULL) {// this is usually ddrx systems
			  //				command_queue->next_c = current_cmd_seq;
			  temp_c->next_c = current_cmd_seq;
			  this_t->next_c = command_queue;
			}else {
			  prev_cmd->next_c = command_queue;
			  temp_c->next_c = current_cmd_seq;
			  //				command_queue->next_c = current_cmd_seq;
			}
			//		   current_cmd_seq->next_c = command_queue;
			//		   this_t->next_c = command_queue;
			// Do the rest of the cmd_ids
			while(temp_c != NULL) {
			  if (temp_c->command != DATA)
				temp_c->cmd_id = cmd_id++;
			  temp_c = temp_c->next_c;
			}
		  }
#ifdef DEBUG
		  if ((bank_conflict == TRUE || 
				(bank_conflict == FALSE && ((bank_open == FALSE && (current_cmd_seq->command != RAS)) ||
											(bank_open == TRUE && current_cmd_seq->command == RAS)))) &&	  
			  get_transaction_debug()) {
			print_transaction(dram_system.current_dram_time, chan_id,this_t->transaction_id);
		  }
#endif
		}else {// If precharge it might not be necessary 
		} 
	  }
	}
  } 
}

void insert_refresh_transaction(int chan_id) {
  addresses_t refresh_address;  
  command_t * command_queue = NULL;
  command_t * rq_temp_c = NULL;
	/** Check if the time for refresh has passed and t_rfc after it has gone by***/ 
	if(dram_system.current_dram_time > 
		(dram_system.config.refresh_cycle_count + dram_system.dram_controller[chan_id].refresh_queue.last_refresh_cycle )
		&& (dram_system.dram_controller[chan_id].refresh_queue.size < MAX_REFRESH_QUEUE_DEPTH)){
	  refresh_address = get_next_refresh_address(chan_id);
	  command_queue = build_refresh_cmd_queue(dram_system.current_dram_time,refresh_address,dram_system.dram_controller[chan_id].refresh_queue.refresh_counter++);
	  
	  if (dram_system.dram_controller[chan_id].refresh_queue.rq_head == NULL) {
	   dram_system.dram_controller[chan_id].refresh_queue.rq_head = command_queue;
	  }else {
	  	rq_temp_c = dram_system.dram_controller[chan_id].refresh_queue.rq_head;
		while (rq_temp_c->rq_next_c != NULL) {
		  rq_temp_c = rq_temp_c->rq_next_c;
		}
		rq_temp_c->rq_next_c = command_queue;
	  }
		
	 dram_system.dram_controller[chan_id].refresh_queue.refresh_pending	= TRUE;
	dram_system.dram_controller[chan_id].refresh_queue.size++;
	 dram_system.dram_controller[chan_id].refresh_queue.last_refresh_cycle = (dram_system.current_dram_time/dram_system.config.refresh_cycle_count)*
	  															dram_system.config.refresh_cycle_count;
#ifdef DEBUG
	  if (get_transaction_debug()) {
		fprintf(stderr,"[%llu] Inserting Refresh Transaction %d Chan[%d] Rank [%d] Bank[%d] Row[%d]\n",dram_system.current_dram_time,
		   dram_system.dram_controller[chan_id].refresh_queue.refresh_counter - 1,
		   chan_id,
		   refresh_address.rank_id,
		    refresh_address.bank_id,
			refresh_address.row_id);
	 	command_t* temp_c = command_queue;
		while(temp_c != NULL) {
		  print_command(dram_system.current_dram_time, temp_c);
		  temp_c = temp_c->next_c;
	  		}	  
	  } 
#endif
	}
	
}

/** Function which returns the "refresh address" and updates the channels
 * refresh addresses accordingly **/

addresses_t get_next_refresh_address (int chan_id) {
  addresses_t this_a;
	 if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) { 
		this_a.physical_address 	= 0;
		this_a.chan_id 			= chan_id;			
		this_a.rank_id			= dram_system.config.rank_count;
		this_a.bank_id			= dram_system.config.bank_count;
		this_a.row_id 			= dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index;
		dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index 	
			= (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index + 1) % dram_system.config.row_count;
	  }else if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {  
		this_a.physical_address 	= 0;
		this_a.chan_id 			= chan_id;			
		this_a.rank_id			= dram_system.config.rank_count;
		this_a.bank_id			= dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_bank_index;
		dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_bank_index 	
			= (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_bank_index + 1) % dram_system.config.bank_count;
		this_a.row_id 			= dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index;
		if (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_bank_index == 0) { /* We rolled over so should increment row count */
			dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index 	
				= (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index + 1) % dram_system.config.row_count;
		}
	 }else if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK) { 
		this_a.physical_address 	= 0;
		this_a.chan_id 			= chan_id;			
		this_a.rank_id			= dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_rank_index;
		this_a.bank_id			= dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_bank_index;
		this_a.row_id 			= dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index;
		dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_rank_index 	
			= (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_rank_index + 1) % dram_system.config.rank_count;
		if (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_rank_index == 0) {
			dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_bank_index 	
			= (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_bank_index + 1) % dram_system.config.bank_count;
			if (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_bank_index == 0) { /* We rolled over so should increment row count */
				dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index 	
					= (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index + 1) % dram_system.config.row_count;
			}
		}
	 }else if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
	   this_a.physical_address     = 0;
	   this_a.chan_id          = chan_id;
	   this_a.bank_id          = dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_bank_index; // Does not matter
	   this_a.row_id           = dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index;
	   this_a.rank_id          = dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_rank_index; // Does not matter
		dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_rank_index 	
			= (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_rank_index + 1) % dram_system.config.rank_count;
	   if (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_rank_index == 0) { /* We rolled over so should increment row count */
		 dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index
		   = (dram_system.dram_controller[this_a.chan_id].refresh_queue.refresh_row_index + 1) % dram_system.config.row_count;
	   }
	 }
//	 fprintf(stderr," Get next refresh address chan %d rank %d row %d\n",this_a.chan_id,this_a.rank_id,this_a.row_id);
	return this_a;

}

/** Flag which updates the refresh_pending flag of a channel **/
#ifdef ALPHA_SIM
int is_refresh_pending(int chan_id){
#else
bool is_refresh_pending(int chan_id){
#endif
	command_t * temp_c = dram_system.dram_controller[chan_id].refresh_queue.rq_head;
	dram_system.dram_controller[chan_id].refresh_queue.refresh_pending = FALSE;
	if (temp_c != NULL) {
	  return true;
	}
	return false;
}		
	

	
/**
 * Command sequences that are supported
 *
 * refresh
 * refresh_all
 * precharge->refresh
 * precharge_all->refresh_all
 *
 * TODO Make all commands special cases of the base command
 *
 * 1. REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK -> (Precharge_all) Refresh_all 
 * 2. REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK -> Not known
 * 3. REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK -> (Precharge) Refresh 
 *
 **/



command_t* build_refresh_cmd_queue(tick_t now ,addresses_t refresh_address,unsigned int refresh_id){
  command_t *command_queue = NULL;
  command_t	*temp_c;
  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK || dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK || dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK ) {
	if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE) {
	  /* precharge all banks */
	  command_queue = add_command(now,
		  command_queue,
		  PRECHARGE_ALL,
		  &refresh_address,
		  refresh_id,
		  0,
		  0,
		  0);	    
	} 
	/* activate this row on all banks */
	command_queue = add_command(now,
		command_queue,
		REFRESH_ALL,
		&refresh_address,
		refresh_id,0,0,0);
  }else if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK) {
	if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE) {
	  /* precharge reqd bank only */
	  command_queue = add_command(now,
		  command_queue,
		  PRECHARGE,
		  &refresh_address,
		  refresh_id,
		  0,
		  0,
		  0);	    
	}
	command_queue = add_command(now,
		command_queue,
		REFRESH,
		&refresh_address,
		refresh_id,0,0,0);

  }
  temp_c = command_queue;
  while (temp_c != NULL) {
	temp_c->refresh = TRUE;
	temp_c = temp_c->next_c;
  }
  return command_queue;	  
}

int can_issue_refresh_command(command_t *this_rq_c, command_t *this_c) {

  switch (this_c->command) {
	
  case PRECHARGE:
    return can_issue_refresh_prec(this_rq_c,this_c);
  case PRECHARGE_ALL:
    return can_issue_refresh_prec_all(this_rq_c,this_c);
  case REFRESH_ALL:
    return can_issue_refresh_refresh_all(this_rq_c,this_c);
  case REFRESH:
//  fprintf(stderr,">>>>> Can issue refresh command\n");
//  print_command(dram_system.current_dram_time,this_c);
    return can_issue_refresh_refresh(this_rq_c,this_c);
  default:
    fprintf(stderr,"Not sure of command type, unable to issue");
    return FALSE;
  }
  return FALSE; 

}


/**
 * 1. Close page -> Has to wait for the activate to complete
 *    Other -> Has to wait for activate to complete if it is the first command
 *    in the queue.
 * 2. Have to wait for previously issued transactions to complete. 
 * 3. Has to wait for ras_done_time to lapse.
 */
int can_issue_refresh_prec(command_t* this_rq_c, command_t *this_c){
  int j,pending_cas;
  command_t *temp_c;
  dram_controller_t *this_dc;
  int chan_id = this_c->chan_id;
  tick_t cmd_bus_reqd_time = dram_system.current_dram_time + 
	(dram_system.config.dram_type == FBD_DDR2 ? 
	 dram_system.config.t_amb_up + dram_system.config.t_bus * dram_system.config.rank_count :0);
  tick_t dram_reqd_time = cmd_bus_reqd_time + 
	dram_system.config.row_command_duration;
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  int bank_id = this_c->bank_id;
  int rank_id = this_c->rank_id;
  pending_cas = FALSE;

	if ( check_cmd_bus_required_time(this_c,cmd_bus_reqd_time) == FALSE)
	  return FALSE;

	/** Takes care of RAS conflicts i.e. ensures tRAS is done **/
	if (this_dc->rank[rank_id].bank[bank_id].ras_done_time > dram_reqd_time
	|| this_dc->rank[rank_id].bank[bank_id].twr_done_time > dram_reqd_time) {	
	  return FALSE;
	}

	for(j=0;j<dram_system.dram_controller[chan_id].transaction_queue.transaction_count;j++){						/* checked all preceeding transactions for not yet completed CAS command */
	  temp_c = dram_system.dram_controller[chan_id].transaction_queue.entry[j].next_c;
	  if ((temp_c->rank_id == rank_id) && (temp_c->bank_id == bank_id) &&
		  dram_system.dram_controller[chan_id].transaction_queue.entry[j].status == MEM_STATE_ISSUED ){
		while(temp_c != NULL){
			if (temp_c->status == IN_QUEUE)
			  return FALSE;
			if (temp_c->rank_done_time > dram_reqd_time)
			  return FALSE;
		  temp_c = temp_c->next_c;
		}
	  }
	}

  return TRUE;
}
/***
 * 1. Ras done time for all banks are honored.
 * 2. Check if there are previous commands in flight that have to complete.
 * Need to add more sophistication into the CAS checks.
 */

int can_issue_refresh_prec_all(command_t* this_rq_c, command_t *this_c){
 int i,j,pending_cas;
  command_t *temp_c;
  dram_controller_t *this_dc;
  int chan_id = this_c->chan_id;
  tick_t dram_cmd_bus_reqd_time = dram_system.current_dram_time + 
						+ (dram_system.config.dram_type == FBD_DDR2 ? 
						dram_system.config.t_bus * dram_system.config.rank_count + dram_system.config.t_amb_up : 0);
  tick_t dram_reqd_time = dram_cmd_bus_reqd_time + dram_system.config.row_command_duration;
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  pending_cas = FALSE;
  
  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
	  if (dram_system.config.dram_type == FBD_DDR2 &&
		  dram_cmd_bus_reqd_time < this_dc->rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time) { 
		return FALSE;
	  }
	for (j=0;j<dram_system.config.bank_count;j++) {
	  if (dram_reqd_time < this_dc->rank[this_c->rank_id].bank[j].ras_done_time ||
		  dram_reqd_time < this_dc->rank[this_c->rank_id].bank[j].twr_done_time) { 
#if DEBUG
	   if (get_ref_tran_watch(this_c->tran_id)) {
		 fprintf(stderr,"[%llu] PREC_ALL ras_done_time(%llu) > dram_reqd_time(%llu) rank(%d) bank(%d)",dram_system.current_dram_time,
			 this_dc->rank[this_c->rank_id].bank[j].ras_done_time,
			 dram_reqd_time,
			 this_c->rank_id, j);
	   }
#endif
		return FALSE; 					/* t_ras not yet satisfied for this bank */
	  }
	}
  }else {
	for (i=0;i<dram_system.config.rank_count;i++) {
	  if (dram_system.config.dram_type == FBD_DDR2 &&
		  dram_cmd_bus_reqd_time < this_dc->rank[i].my_dimm->fbdimm_cmd_bus_completion_time) { 
#if DEBUG
	   if (get_ref_tran_watch(this_c->tran_id)) {
		 fprintf(stderr,"[%llu] PREC_ALL Command bus not available",dram_system.current_dram_time);
	   }
#endif
		return FALSE;
	  }
	  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
		for (j=0;j<dram_system.config.bank_count;j++) {
		  if (dram_reqd_time < this_dc->rank[i].bank[j].ras_done_time || dram_reqd_time < this_dc->rank[i].bank[j].twr_done_time ) { 
#if DEBUG
	   if (get_ref_tran_watch(this_c->tran_id)) {
		 fprintf(stderr,"[%llu] PREC_ALL Bank not ready in time ",dram_system.current_dram_time);
	   }
#endif
			return FALSE; 					/* t_ras not yet satisfied for this bank */
		  }

		}
	  }else {
		if (this_dc->rank[i].bank[this_c->bank_id].ras_done_time > dram_reqd_time ||
			this_dc->rank[i].bank[this_c->bank_id].twr_done_time > dram_reqd_time) {
#if DEBUG
	   if (get_ref_tran_watch(this_c->tran_id)) {
		 fprintf(stderr,"[%llu] PREC_ALL Bank not ready in time ",dram_system.current_dram_time);
	   }
#endif
		  return FALSE; 					/* t_ras not yet satisfied for this bank */
		}
	  }
	}
  }
  for(j=0;j<dram_system.dram_controller[chan_id].transaction_queue.transaction_count;j++){						/* checked all preceeding transactions for not yet completed CAS command */
	temp_c = dram_system.dram_controller[chan_id].transaction_queue.entry[j].next_c;
	if((dram_system.dram_controller[chan_id].transaction_queue.entry[j].status == MEM_STATE_ISSUED 
	// || dram_system.dram_controller[chan_id].transaction_queue.entry[j].status == MEM_STATE_DATA_ISSUED   
		  || dram_system.dram_controller[chan_id].transaction_queue.entry[j].status == MEM_STATE_CRITICAL_WORD_RECEIVED)  && (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK ||
			(dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK && temp_c->bank_id == this_c->bank_id ) ||
			(dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK && temp_c->rank_id == this_c->rank_id ))) { 
	  while(temp_c != NULL){
		if (temp_c->status == IN_QUEUE || 
			(temp_c->rank_done_time > dram_reqd_time)) {
#ifdef DEBUG
		  if (get_ref_tran_watch(this_c->tran_id)) {
			fprintf(stderr,"[%llu] PREC_ALL CAS not complete for issued tran (%d) ",dram_system.current_dram_time,
				dram_system.dram_controller[chan_id].transaction_queue.entry[j].status );
			print_command(dram_system.current_dram_time,temp_c);
		  }
#endif
		  return FALSE;		/* PRECHARGE has to be suppressed until all previous CAS complete */
		} 
		temp_c = temp_c->next_c;
	  }
	}
  }
  return TRUE;
}

/***
 * To check : 
 *  Cmd Bus is available
 *  Banks are precharged.
 *  t_rfc is satisfied.
 */
int can_issue_refresh_refresh(command_t *this_rq_c,command_t *this_c) {

  dram_controller_t *this_dc;
  int rank_id, bank_id;
  tick_t dram_reqd_time;
  tick_t cmd_bus_reqd_time;
  int chan_id = this_c->chan_id;
  int i;
  command_t *temp_c; 
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);

  if (dram_system.config.dram_type == FBD_DDR2) {
	cmd_bus_reqd_time = dram_system.current_dram_time +
	  dram_system.config.t_bus*dram_system.config.rank_count +
	  dram_system.config.t_bundle +
	  dram_system.config.t_amb_up;
  }else {
	cmd_bus_reqd_time = dram_system.current_dram_time; 
  }
  dram_reqd_time = cmd_bus_reqd_time + dram_system.config.row_command_duration;
  rank_id = this_c->rank_id;
  bank_id = this_c->bank_id;

  if((this_dc->rank[rank_id].bank[bank_id].rp_done_time >= this_dc->rank[rank_id].bank[bank_id].ras_done_time ) &&
	   (this_dc->rank[rank_id].bank[bank_id].rp_done_time <= dram_reqd_time)){
  } else {
	if (get_ref_tran_watch(this_c->tran_id)) {
	  fprintf(stderr,"%llu  Failed to issue refresh due to bank status %d rp_done_time %llu \n",
		  dram_system.current_dram_time,
		  this_dc->rank[rank_id].bank[bank_id].status,
		  this_dc->rank[rank_id].bank[bank_id].rp_done_time
		  );
	}
	return FALSE;
  }
  if (dram_system.config.dram_type == FBD_DDR2 && check_cmd_bus_required_time (this_c, cmd_bus_reqd_time) == FALSE) {
	if (get_ref_tran_watch(this_c->tran_id)) {
	  fprintf(stderr,"%llu  Failed to issue refresh due to conflict with cmd bus %llu \n",
		  dram_system.current_dram_time,cmd_bus_reqd_time);
	}
	return FALSE;
  }

  for (i=0;i<dram_system.dram_controller[chan_id].transaction_queue.transaction_count;i++) {
	temp_c = dram_system.dram_controller[chan_id].transaction_queue.entry[i].next_c;
	if (temp_c->rank_id == rank_id &&
		temp_c->bank_id == bank_id && dram_system.dram_controller[chan_id].transaction_queue.entry[i].status == MEM_STATE_ISSUED) {
	  while(temp_c != NULL){
		if (temp_c->status == IN_QUEUE){
		  if (get_ref_tran_watch(this_c->tran_id)) {
			fprintf(stderr,"%llu  Failed to issue refresh due to conflict with issued transaction %llu \n",
				dram_system.current_dram_time,temp_c->tran_id
				);
		  }
		  return FALSE;
		} 
	  temp_c = temp_c->next_c;
	  }
	}
  }
  /** Check if your bank is precharged - open page policy **/
  if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE) {
	temp_c = this_rq_c;
	if (temp_c->status == IN_QUEUE && 
		temp_c->command == PRECHARGE ) { /** Has to be the case **/
	  if (get_ref_tran_watch(this_c->tran_id)) {
		fprintf(stderr,"%llu  Failed to issue refresh due to conflict with precharge \n",
			dram_system.current_dram_time
			);
	  }
	  return FALSE;
	}
  }
  return TRUE;
}

/***
 * To check : 
 *  Cmd Bus is available
 *  Banks are precharged.
 *  t_rfc is satisfied.
 */
int can_issue_refresh_refresh_all(command_t *this_rq_c,command_t *this_c) {
	
  dram_controller_t *this_dc;
  tick_t dram_reqd_time;
  tick_t cmd_bus_reqd_time;
  int i,j;
  command_t *temp_c; 
  int chan_id = this_c->chan_id;
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);

  if (dram_system.config.dram_type == FBD_DDR2) {
	cmd_bus_reqd_time = dram_system.current_dram_time +
	  dram_system.config.t_bus*dram_system.config.rank_count +
	  dram_system.config.t_bundle +
	  dram_system.config.t_amb_up;
  }else {
	cmd_bus_reqd_time = dram_system.current_dram_time; 
  }
  dram_reqd_time = cmd_bus_reqd_time + dram_system.config.row_command_duration;

   /** Check if your bank is precharged - open page policy **/
  if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE) {
	temp_c = this_rq_c;
		if ((temp_c->command == PRECHARGE || temp_c->command == PRECHARGE_ALL )&& temp_c->dram_proc_comp_time > dram_reqd_time) { /** Has to be the case **/
#ifdef DEBUG
		if (get_ref_tran_watch(this_c->tran_id)) {
		  fprintf(stderr,"%llu  Failed to issue refresh since PRECHARGE is not done \n",
			  dram_system.current_dram_time);
		}
#endif
		  return FALSE;
		}
  }

  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
	if (dram_system.config.dram_type == FBD_DDR2 && this_dc->rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time >  cmd_bus_reqd_time) {
#ifdef DEBUG
	  if (get_ref_tran_watch(this_c->tran_id)) {
		fprintf(stderr,"%llu  Failed to issue refresh due to bus done time %llu > bus reqd time %llu \n",
			dram_system.current_dram_time,
			this_dc->rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time,
			cmd_bus_reqd_time
			);
	  }
#endif
	  return FALSE; 					/* t_ras not yet satisfied for this bank */
	}
	for (j=0;j<dram_system.config.bank_count;j++) {
	  if (this_dc->rank[this_c->rank_id].bank[j].rp_done_time <  this_dc->rank[this_c->rank_id].bank[j].ras_done_time ||
		  dram_reqd_time < this_dc->rank[this_c->rank_id].bank[j].rp_done_time ) {
#ifdef DEBUG
		if (get_ref_tran_watch(this_c->tran_id)) {
		  fprintf(stderr,"%llu  aFailed to issue refresh due to bank(r%db%d) ras_done_time %llu rp_done_time %llu dram_reqd_time %llu %d %d\n",
			  dram_system.current_dram_time,
			  this_c->rank_id,
			  j,
			  this_dc->rank[this_c->rank_id].bank[j].ras_done_time,
			  this_dc->rank[this_c->rank_id].bank[j].rp_done_time,
			  dram_reqd_time,
			  this_dc->rank[this_c->rank_id].bank[j].rp_done_time <  this_dc->rank[this_c->rank_id].bank[j].ras_done_time,
			  dram_reqd_time < this_dc->rank[this_c->rank_id].bank[j].rp_done_time
			  );
		}
#endif
		return FALSE; 					/* t_ras not yet satisfied for this bank */
	  }

	}
  }else {
  for (i=0;i<dram_system.config.rank_count;i++) {
	 if (dram_system.config.dram_type == FBD_DDR2 && this_dc->rank[i].my_dimm->fbdimm_cmd_bus_completion_time >  cmd_bus_reqd_time) {
#ifdef DEBUG
		if (get_ref_tran_watch(this_c->tran_id)) {
		  fprintf(stderr,"%llu  Failed to issue refresh due to bus reqd time %llu < bus available time %llu \n",
			  dram_system.current_dram_time,
			  cmd_bus_reqd_time,this_dc->rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time
			  );
		}
#endif
	   return FALSE;
	 } 
	if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
	  for (j=0;j<dram_system.config.bank_count;j++) {
	  if (this_dc->rank[i].bank[j].rp_done_time >=  this_dc->rank[i].bank[j].ras_done_time && 
			 (dram_reqd_time > this_dc->rank[i].bank[j].rp_done_time )) { 
		}else {
		  if (get_ref_tran_watch(this_c->tran_id)) {
			fprintf(stderr,"%llu  bFailed to issue refresh due to bank(r%db%d) status %d rp_done_time %llu \n",
				dram_system.current_dram_time,
				i,
				j,
				this_dc->rank[i].bank[j].status,
				this_dc->rank[i].bank[j].rp_done_time
				);
		  }
		  return FALSE; 					/* t_ras not yet satisfied for this bank */
		}
	  }
	}else {
	  if (this_dc->rank[i].bank[this_c->bank_id].rp_done_time >=  this_dc->rank[i].bank[this_c->bank_id].ras_done_time && 
		   (dram_reqd_time > this_dc->rank[i].bank[this_c->bank_id].rp_done_time )) { 
	  }else {
		  if (get_ref_tran_watch(this_c->tran_id)) {
			fprintf(stderr,"%llu  cFailed to issue refresh due to bank(r%db%d) status %d rp_done_time %llu \n",
				dram_system.current_dram_time,
				this_c->rank_id,
				this_c->bank_id,
				this_dc->rank[this_c->rank_id].bank[this_c->bank_id].status,
				this_dc->rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time
				);
		  }
		return FALSE; 					/* t_ras not yet satisfied for this bank */
	  }
	}

  } 
  }
  for (i=0;i<dram_system.dram_controller[chan_id].transaction_queue.transaction_count;i++) {
	temp_c = dram_system.dram_controller[chan_id].transaction_queue.entry[i].next_c;
	if (dram_system.dram_controller[chan_id].transaction_queue.entry[i].status == MEM_STATE_ISSUED) {
	  while(temp_c != NULL){
		if (temp_c->status == IN_QUEUE){
		  if (((dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK ||
				  (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK && temp_c->bank_id == this_c->bank_id) ||
				  (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK && temp_c->rank_id == this_c->rank_id))))
			return FALSE;
		  }
		temp_c = temp_c->next_c;
		} 
	  }
  }
  /** Check if your precharge is issued - open page policy **/
  if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE) {
	temp_c = this_rq_c;
		if ((temp_c->command == PRECHARGE || temp_c->command == PRECHARGE_ALL )&& temp_c->status == IN_QUEUE) { /** Has to be the case **/
#ifdef DEBUG
		if (get_ref_tran_watch(this_c->tran_id)) {
		  fprintf(stderr,"%llu  Failed to issue refresh since PRECHARGE is not done \n",
			  dram_system.current_dram_time);
		}
#endif
		  return FALSE;
		}
  }
	return TRUE;
}


int is_refresh_queue_full(int chan_id) {
  if (dram_system.dram_controller[chan_id].refresh_queue.size >= MAX_REFRESH_QUEUE_DEPTH ) {
	return 1;
  }
  else
	return 0;
}

int is_refresh_queue_half_full(int chan_id) {
  if (dram_system.dram_controller[chan_id].refresh_queue.size >= (MAX_REFRESH_QUEUE_DEPTH >> 1)) {
	return 1;
  }
  else
	return 0;
}

int is_refresh_queue_empty(int chan_id) {
  if (dram_system.dram_controller[chan_id].refresh_queue.size > 0 ) {
	return FALSE;
  }
  else
	return TRUE;
}

/*
 * This function returns the next refresh command that can be issued.
 * chan_id -> channel for which refresh command is being sought
 * transaction_selection_policy -> Transaction selection policy
 * RBRR stuff
 * command (RAS/PRE/CAS)
 * command_queue -> predetermined sequence
 */
command_t * get_next_refresh_command_to_issue(
	int chan_id,
	int transaction_selection_policy,
	int command){ 
  command_t * rq_c,* this_c;
  int first_queued_command_found = FALSE;
  /* Traverse through the refresh queue and see if you can issue refresh
   * commands in the same bundle **/
  if (row_command_bus_idle(chan_id)) {
            //print_command(dram_system.current_dram_time,this_c);
	rq_c = dram_system.dram_controller[chan_id].refresh_queue.rq_head; 
	while (rq_c != NULL ) {
	  this_c = rq_c;
	  first_queued_command_found = FALSE;
	  while (first_queued_command_found ==FALSE && this_c != NULL ) {
		if (this_c->status == IN_QUEUE) {
		  first_queued_command_found = TRUE;

		  if ( can_issue_refresh_command(rq_c,this_c) == TRUE ) {
//			fprintf(stderr,"REF ");
//			print_command(dram_system.current_dram_time,this_c);
			return this_c;
		  } 
		}
		this_c = this_c->next_c;
	  }
	  rq_c = rq_c->rq_next_c;
	}
  }
	return NULL;
}
