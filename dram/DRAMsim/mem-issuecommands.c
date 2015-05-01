/*
 * mem-issuecommands.c Scheduling support for all traditional DRAM systems from
 * SDRAM to DDRII. Also post command issue initialization of states for all
 * configurations is done here.
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
void issue_cmd(tick_t now,command_t *this_c,transaction_t *this_t,int chan_id);
command_t * get_next_cmd_to_issue(int chan_id,int trans_sel_policy);

command_t *next_pre_in_transaction_queue(int transaction_selection_policy, int cmd_type,int chan_id,int rank_id,int bank_id,bool write_burst) {
 /* We scan till we get the first precharge that can be issued */
  int i;
  int first_queued_command_found;
  command_t *this_c;
  for(i = 0;i < dram_system.dram_controller[chan_id].transaction_queue.transaction_count ; i++){
	this_c = dram_system.dram_controller[chan_id].transaction_queue.entry[i].next_c;
	first_queued_command_found = FALSE;
	while((this_c != NULL) && (first_queued_command_found == FALSE)){
	  if((this_c->chan_id == chan_id) && (this_c->status == IN_QUEUE))
	  {
		first_queued_command_found = TRUE;
		if(dram_system.dram_controller[chan_id].transaction_queue.entry[i].status == MEM_STATE_ISSUED){  /* command not issued, see if we can issue it */
		  if (can_issue_command(i,this_c)) {
			return this_c;
		  }
		}
	  }
	  this_c = this_c->next_c;
	}
  }
	return NULL;
}
command_t *next_cmd_in_transaction_queue(int transaction_selection_policy, int cmd_type,int chan_id,int rank_id,int bank_id,bool write_burst) {
  int i;
  int first_queued_command_found;
  command_t *this_c;
  for(i = 0;i < dram_system.dram_controller[chan_id].transaction_queue.transaction_count ; i++){
	this_c = dram_system.dram_controller[chan_id].transaction_queue.entry[i].next_c;
	first_queued_command_found = FALSE;
	while((this_c != NULL) && (first_queued_command_found == FALSE)){
      if((this_c->chan_id == chan_id) && (this_c->status == IN_QUEUE))
      {
        first_queued_command_found = TRUE;
        if (((cmd_type == CAS && (this_c->command == CAS || this_c->command == CAS_WRITE)) || (this_c->command == cmd_type))){  /* command not issued, see if we can issue it */
          if(cmd_type == PRECHARGE) {
            return this_c;
          }else {
            if (bank_id == -1) {
              if (this_c->rank_id == rank_id)
                return this_c;
            }else {
              if (this_c->rank_id == rank_id && this_c->bank_id == bank_id)
                return this_c;
            }
          }

        }
      }
      this_c = this_c->next_c;
	}
  }
  return NULL;
}

int get_rbrr_cmd_index( int nextRBRR_cmd) {
  int index = -1;
  if (nextRBRR_cmd == RAS) 
	index = 0;
  else if (nextRBRR_cmd == CAS)
	index = 1;
  else if (nextRBRR_cmd == PRECHARGE)
	index = 2;
  else if (nextRBRR_cmd == REFRESH)
	index = 3;
  return index;
}

    
void set_rbrr_next_command(dram_controller_t *this_dc,int nextRBRR_cmd) {
  if (nextRBRR_cmd == RAS) 
	 this_dc->sched_cmd_info.current_cmd = CAS;
  else if (nextRBRR_cmd == CAS)
	 this_dc->sched_cmd_info.current_cmd = PRECHARGE;
  else if (nextRBRR_cmd == PRECHARGE)
	 this_dc->sched_cmd_info.current_cmd = RAS;
}
/* The update is done as follows
 * RAS -> 
 *  switch current_rank to rank_id +1 mod num_ranks
 *  switch ranks bank to bank_id +1 mod num_banks
 * CAS -> 
 *  switch ranks bank to bank_id +1 mod num_banks
 *  if bank rolled over switch current_rank to rank_id +1 mod num_ranks
 *  PRE
 *  nothing
 */
void update_rbrr_rank_bank_info(int transaction_selection_policy,
	dram_controller_t *this_dc, int cmd,int rank_id,int bank_id, int cmd_index, int rank_count,int bank_count){
	if (cmd == RAS) {
	  this_dc->sched_cmd_info.current_bank[cmd_index][rank_id] = (bank_id + 1) %  bank_count;
	  this_dc->sched_cmd_info.current_rank[cmd_index] = (rank_id + 1) %  rank_count;
	}else if (cmd == CAS) {
	  this_dc->sched_cmd_info.current_bank[cmd_index][rank_id] = (bank_id + 1) %  bank_count;
	  if ( 	this_dc->sched_cmd_info.current_bank[cmd_index][rank_id] == 0) 
		this_dc->sched_cmd_info.current_rank[cmd_index] = (rank_id + 1) %  rank_count;
	}
}


/* 
 * Check for refresh queue status first.
 * - Refresh policies possible -> refresh top priority/ refresh oppourtunistic
 *   - returns is_refresh_queue_full 
 *   - Get refresh_command that can go
 *   	- Any rank/bank/cmd constraints need to be satisfied
 *   
 *   - Find first command that can go
 *   	- satisfying possible constraints 
 *   		- cmd type
 *   		- rank/bank
 *   		- arrival order
 */   		

void issue_new_commands(tick_t now,int chan_id, char *debug_string){
  int i = 0;
  command_t *this_c = NULL;
  dram_controller_t *this_dc;
  int transaction_selection_policy;
  int nextRBRR_rank,nextRBRR_bank,nextRBRR_cmd;

  int command_issued = FALSE;
  command_t *refresh_command_to_issue = NULL;
  int refresh_queue_full = FALSE;
  this_dc = &(dram_system.dram_controller[chan_id]);
  transaction_selection_policy = get_transaction_selection_policy(global_biu);
  command_t *cmd_issued = NULL; 
  if (dram_system.config.auto_refresh_enabled == TRUE) {
	refresh_queue_full = is_refresh_queue_half_full(chan_id);
	refresh_command_to_issue = get_next_refresh_command_to_issue(chan_id,transaction_selection_policy,this_dc->next_command);
	if (refresh_command_to_issue != NULL &&
		((dram_system.config.refresh_issue_policy == REFRESH_OPPORTUNISTIC && refresh_queue_full) ||
		 dram_system.config.refresh_issue_policy == REFRESH_HIGHEST_PRIORITY)) {
	  /* Is refresh queue full ? -> then issue
	   * Find the first refresh command that you can issue -> only if no
	   * read/write command exists do you issue */
		issue_cmd(now,refresh_command_to_issue,NULL,chan_id);
		cmd_issued = refresh_command_to_issue;
		command_issued = TRUE;
	}
  }
  // Do something for aging transactions 
  // Note that there can be transactions for mp/lp which do not get done
  // despite being older ... also check for issued stuff which needs to get
  // done
  if (   dram_system.dram_controller[chan_id].transaction_queue.transaction_count > 0 &&
      (dram_system.current_dram_time - dram_system.dram_controller[chan_id].transaction_queue.entry[0].arrival_time > dram_system.config.arrival_threshold)) {
    // Check if that 
    if (col_command_bus_idle(chan_id) || row_command_bus_idle(chan_id)) {
      command_t *this_c = dram_system.dram_controller[chan_id].transaction_queue.entry[0].next_c;
      while (this_c != NULL && cmd_issued == NULL ) {
        if (this_c->status == IN_QUEUE && can_issue_command(0,this_c)) {
          cmd_issued = this_c;           
          // We need to clean up the command queue in case we are
          // using a CAS but
          //         // there is a PRE/RAS earlier to it.

          if  (dram_system.config.row_buffer_management_policy
              != CLOSE_PAGE &&  cmd_issued->cmd_id != 0
              &&  dram_system.dram_controller[chan_id].transaction_queue.entry[0].status  !=   MEM_STATE_ISSUED
              && (cmd_issued->command != RAS || cmd_issued->command !=   PRECHARGE))
          {
            collapse_cmd_queue(&dram_system.dram_controller[chan_id].transaction_queue.entry[0],cmd_issued);
          }
          issue_cmd(now,cmd_issued, &dram_system.dram_controller[chan_id].transaction_queue.entry[0],chan_id);
          command_issued =  TRUE;

        }
        this_c = this_c->next_c;
      }

    }
  }

  if(transaction_selection_policy == WANG){
	/* The process of finding the next WANG command is as follows 
	 * 1. Find the next valid command.
	 * 2. Find the valid rank / bank combo
	 * 3. Check for a valid combination.
	 * 4. If valid or invalid -> update the next valid command/ rank / bank
	 * numbers to maintain the sequence.
	 * 5. How do we put in the DQS handling capability?
	 * 6. How about refresh?
	 */
	this_c = NULL;
	int num_scans = 0; /* How do we fix this? */
	int row_bus_free = row_command_bus_idle(chan_id);
	int col_bus_free = col_command_bus_idle(chan_id);
	bool pre_chked = false;
	if (row_bus_free || col_bus_free) {
	  while (cmd_issued == NULL && num_scans < dram_system.config.rank_count *  dram_system.config.bank_count *3) {
		this_c = NULL;
		nextRBRR_cmd = this_dc->sched_cmd_info.current_cmd;
		int cmd_index = get_rbrr_cmd_index(nextRBRR_cmd);
		nextRBRR_rank = this_dc->sched_cmd_info.current_rank[cmd_index];
		nextRBRR_bank = this_dc->sched_cmd_info.current_bank[cmd_index][nextRBRR_rank];
		if ((nextRBRR_cmd == CAS && col_bus_free) ||
			((nextRBRR_cmd == PRECHARGE || nextRBRR_cmd == RAS) && row_bus_free)) { /* No need to do any of this */
		  if (nextRBRR_cmd == PRECHARGE ) {
			if (!pre_chked)
			  this_c = next_pre_in_transaction_queue(transaction_selection_policy, nextRBRR_cmd,chan_id,nextRBRR_rank,nextRBRR_bank,false);
			else this_c = NULL;
			pre_chked = true;
		  }else {
			this_c = next_cmd_in_transaction_queue(transaction_selection_policy, nextRBRR_cmd,chan_id,nextRBRR_rank,nextRBRR_bank,false);
		  }
          int tran_index = 0;
          if (this_c != NULL) {
            get_transaction_index(this_c->chan_id,this_c->tran_id);
          }
          if (this_c != NULL && (nextRBRR_cmd == PRECHARGE || 
                (nextRBRR_cmd != PRECHARGE ))){
            if( can_issue_command(tran_index,this_c)) {
              issue_cmd(now,this_c,&dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index],chan_id);  
              command_issued = TRUE;
              cmd_issued = this_c;
            }
          }

		  /* Update the rank and bank stuff */
		}
		update_rbrr_rank_bank_info(transaction_selection_policy, this_dc, nextRBRR_cmd,nextRBRR_rank,nextRBRR_bank, cmd_index, dram_system.config.rank_count, dram_system.config.bank_count);
		set_rbrr_next_command(this_dc,nextRBRR_cmd);
		num_scans++;
	  }
	}
  }else if(transaction_selection_policy == FCFS || transaction_selection_policy == GREEDY || 
	  transaction_selection_policy == OBF || transaction_selection_policy == RIFF ) {
	/* In-order
	 * Check if a transaction can be scheduled only if all transactions before
	 * it have been scheduled.
	 * */
	/* RIFF In this policy -> you try to schedule the reads first - then you try to
	 * schedule a write
	 * Refinement would be to do write-bursting; pend all writes and then do
	 * them back-to-back*/
	if (col_command_bus_idle(chan_id) || row_command_bus_idle(chan_id)) {
	  cmd_issued = get_next_cmd_to_issue(chan_id,transaction_selection_policy);
	  if (cmd_issued != NULL) {
		int tran_index = get_transaction_index(cmd_issued->chan_id,cmd_issued->tran_id);
		transaction_t * this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index];
           
		  if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE && 
              cmd_issued->cmd_id != 0 && this_t->status != MEM_STATE_ISSUED&& 
			  ( cmd_issued->command != PRECHARGE))  {
			collapse_cmd_queue(this_t,cmd_issued);
		  }
			issue_cmd(now,cmd_issued,&dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index],chan_id);  
		command_issued = TRUE;
        // Note that if posted cas works then you actually issue the CAS as
        // well
         if (dram_system.config.posted_cas_flag && cmd_issued->command == RAS) {
            cmd_issued->next_c->posted_cas = true;
			issue_cmd(now,cmd_issued->next_c,this_t,chan_id);  
         }
	  } else if (transaction_selection_policy == OBF || transaction_selection_policy == RIFF ) {
		cmd_issued = get_next_cmd_to_issue(chan_id,FCFS);
        if (cmd_issued != NULL) {
          int tran_index = get_transaction_index(cmd_issued->chan_id,cmd_issued->tran_id);
          transaction_t * this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index];
          if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE && 
              cmd_issued->cmd_id != 0 && this_t->status != MEM_STATE_ISSUED&& 
              ( cmd_issued->command != PRECHARGE))  {
            collapse_cmd_queue(this_t,cmd_issued);
          }
          issue_cmd(now,cmd_issued,&dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index],chan_id);  
          command_issued = TRUE;
          // Note that if posted cas works then you actually issue the CAS as
          // well
          if (dram_system.config.posted_cas_flag && cmd_issued->command == RAS) {
            cmd_issued->next_c->posted_cas = true;
            issue_cmd(now,cmd_issued->next_c,this_t,chan_id);  
          }

		}
	  }
	}
  }else if (transaction_selection_policy == MOST_PENDING || transaction_selection_policy == LEAST_PENDING) {
	if (col_command_bus_idle(chan_id) || row_command_bus_idle(chan_id)) {
	  cmd_issued = get_next_cmd_to_issue(chan_id,transaction_selection_policy);
	  if (cmd_issued != NULL) {
		// We need to clean up the command queue in case we are using a CAS but
		// there is a PRE/RAS earlier to it.
		int tran_index = get_transaction_index(cmd_issued->chan_id,cmd_issued->tran_id);
		transaction_t * this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index];

		if (dram_system.config.row_buffer_management_policy != CLOSE_PAGE && 
		  cmd_issued->cmd_id != 0 && this_t->status != MEM_STATE_ISSUED&& 
			  ( cmd_issued->command != PRECHARGE))  {
		  collapse_cmd_queue(this_t,cmd_issued);
		}
			issue_cmd(now,cmd_issued,this_t,chan_id);  
		command_issued = TRUE;
        // Note that if posted cas works then you actually issue the CAS as
        // well
         if (dram_system.config.posted_cas_flag && cmd_issued->command == RAS) {
            cmd_issued->next_c->posted_cas = true;
			issue_cmd(now,cmd_issued->next_c,this_t,chan_id);  
         }
	  }
	}
  }
  if (command_issued == FALSE) {
	if (refresh_command_to_issue != NULL){ 
			issue_cmd(now,refresh_command_to_issue,NULL,chan_id);  
		command_issued = TRUE;
		cmd_issued = refresh_command_to_issue;
	}else {
	  if (is_refresh_queue_empty(chan_id)) {
		// We issue a refresh command based on which rank is not busy? 
	  }
	}
  }
	if (cmd_issued != NULL) { 
      if (wave_debug()) {
        build_wave(debug_string, i, cmd_issued, MEM_STATE_SCHEDULED);
        if (cmd_issued->command == RAS && !cmd_issued->refresh && dram_system.config.posted_cas_flag ) build_wave(debug_string, i, cmd_issued->next_c, MEM_STATE_SCHEDULED);
      }
    }
}


command_t * get_next_cmd_to_issue(int chan_id,int trans_sel_policy){

  /* In-order
   * Check if a transaction can be scheduled only if all transactions before
   * it have been scheduled.
   * */
  int i;
  command_t *this_c = NULL;
  command_t *cmd_issued = NULL;
	  bool first_queued_command_found = FALSE;	/* make sure command sequences execute in strict order within a transaction */
  if (trans_sel_policy == FCFS || trans_sel_policy == GREEDY || trans_sel_policy == OBF) {
	bool first_queued_transaction = FALSE;


	for(i = 0;(i < dram_system.dram_controller[chan_id].transaction_queue.transaction_count) && (cmd_issued == NULL && (first_queued_transaction == FALSE)); i++){
		bool attempt_schedule = false; // Stuff added for the update latency info
		bool issue_tran = false;
	  this_c = dram_system.dram_controller[chan_id].transaction_queue.entry[i].next_c;
	  first_queued_command_found = FALSE;	/* make sure command sequences execute in strict order within a transaction */
	  if ( dram_system.dram_controller[chan_id].transaction_queue.entry[i].status == MEM_STATE_SCHEDULED && trans_sel_policy == FCFS)
		first_queued_transaction = TRUE;
	  while((this_c != NULL) && (first_queued_command_found == FALSE)
		  &&((trans_sel_policy != OBF) || (trans_sel_policy == OBF && is_bank_open(this_c->chan_id,this_c->rank_id,this_c->bank_id,this_c->row_id)))) {
		if((this_c->chan_id == chan_id) && (this_c->status == IN_QUEUE)){  /* command has not yet issued, lets see if we can issue it */
			if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE|| (this_c->command == CAS || this_c->command == CAS_WRITE || 
				  this_c->command == CAS_WRITE_AND_PRECHARGE || this_c->command == CAS_AND_PRECHARGE)) {
			  first_queued_command_found = TRUE;
			}
			attempt_schedule = true;
		  if (can_issue_command(i,this_c) == TRUE && is_cmd_bus_idle(chan_id,this_c)){
			cmd_issued = this_c;
			issue_tran = true;
			//if (wave_debug()) build_wave(&debug_string[0], i, this_c, MEM_STATE_SCHEDULED);
		  }	  
		}
		this_c = this_c->next_c;
	  }
	}
  }else if (trans_sel_policy == RIFF) {
	for(i = 0;(i < dram_system.dram_controller[chan_id].transaction_queue.transaction_count) && (cmd_issued == NULL ); i++){
      bool attempt_schedule = false; // Stuff added for the update latency info
      bool issue_tran = false;
	  this_c = dram_system.dram_controller[chan_id].transaction_queue.entry[i].next_c;
	  first_queued_command_found = FALSE;	/* make sure command sequences execute in strict order within a transaction */
	  if (dram_system.dram_controller[chan_id].transaction_queue.entry[i].transaction_type == MEMORY_READ_COMMAND || 
		  dram_system.dram_controller[chan_id].transaction_queue.entry[i].transaction_type == MEMORY_IFETCH_COMMAND) {
		while((this_c != NULL) && (first_queued_command_found == FALSE) ) {
		  if((this_c->chan_id == chan_id) && (this_c->status == IN_QUEUE)){  /* command has not yet issued, lets see if we can issue it */
			/* Check if this is a CAS -> yes : try to issue;
			 * 							no : skip till you get CAS */
			attempt_schedule = true;
			if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE|| (this_c->command == CAS || this_c->command == CAS_WRITE || 
				  this_c->command == CAS_WRITE_AND_PRECHARGE || this_c->command == CAS_AND_PRECHARGE)) {
			  first_queued_command_found = TRUE;
			}
			if (can_issue_command(i,this_c) == TRUE && is_cmd_bus_idle(chan_id,this_c)){
			  cmd_issued = this_c;
			issue_tran = true;
			}	  
		  }

		this_c = this_c->next_c;
		}
	  }
	}	
  }else if (trans_sel_policy == MOST_PENDING || trans_sel_policy == LEAST_PENDING) {
    // Do the check for Issued but not completed transactions
    for(i = 0;(i < dram_system.dram_controller[chan_id].transaction_queue.transaction_count) && (cmd_issued == NULL ); i++){
      bool attempt_schedule = false; // Stuff added for the update latency info
      bool issue_tran = false;
      this_c = dram_system.dram_controller[chan_id].transaction_queue.entry[i].next_c;
      first_queued_command_found = FALSE;	/* make sure command sequences execute in strict order within a transaction */
      if (dram_system.dram_controller[chan_id].transaction_queue.entry[i].status == MEM_STATE_ISSUED ) {
        while((this_c != NULL) && (first_queued_command_found == FALSE) ) {
          if((this_c->chan_id == chan_id) && (this_c->status == IN_QUEUE)){  /* command has not yet issued, lets see if we can issue it */
            /* Check if this is a CAS -> yes : try to issue;
             * 							no : skip till you get CAS */
            attempt_schedule = true;
            if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE|| (this_c->command == CAS || this_c->command == CAS_WRITE || 
                  this_c->command == CAS_WRITE_AND_PRECHARGE || this_c->command == CAS_AND_PRECHARGE)) {
              first_queued_command_found = TRUE;
            }
            if (can_issue_command(i,this_c) == TRUE && is_cmd_bus_idle(chan_id,this_c)){
              cmd_issued = this_c;
              issue_tran = true;
            }	  
          }

          this_c = this_c->next_c;
        }
      }
    }	

    if (trans_sel_policy == MOST_PENDING) {
      /* Need to issue commands with most CAS accesses */
      transaction_t *this_t;
      int j;
      for (i=0;i< dram_system.dram_controller[chan_id].pending_queue.queue_size && cmd_issued == NULL ;i++) {
        for (j=0;j< dram_system.dram_controller[chan_id].pending_queue.pq_entry[i].transaction_count && cmd_issued == NULL ;j++) {
          int tindex = get_transaction_index(chan_id,dram_system.dram_controller[chan_id].pending_queue.pq_entry[i].transaction_list[j]);
          this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[tindex];
          this_c = this_t->next_c;
          first_queued_command_found = FALSE;
          bool attempt_schedule = false; // Stuff added for the update latency info
          bool issue_tran = false;
          while((this_c != NULL) && (first_queued_command_found == FALSE) ) {
            if((this_c->status == IN_QUEUE)){  /* command has not yet issued, lets see if we can issue it */
              /* Check if this is a CAS -> yes : try to issue;
               * 							no : skip till you get CAS */
              attempt_schedule = true;
              if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE|| (this_c->command == CAS || this_c->command == CAS_WRITE || 
                    this_c->command == CAS_WRITE_AND_PRECHARGE || this_c->command == CAS_AND_PRECHARGE)) {
                first_queued_command_found = TRUE;
              }
              if (can_issue_command(tindex,this_c) == TRUE && is_cmd_bus_idle(chan_id,this_c)){
                cmd_issued = this_c;
                issue_tran = true;
              }	  
            }

            this_c = this_c->next_c;
          }

        }

      }
    }else if (trans_sel_policy == LEAST_PENDING) {
      /* Need to issue commands with least CAS accesses */
      transaction_t *this_t;
      int j;
      for (i= dram_system.dram_controller[chan_id].pending_queue.queue_size - 1; cmd_issued == NULL && i>= 0;i--) {
        for (j=0;j< dram_system.dram_controller[chan_id].pending_queue.pq_entry[i].transaction_count && cmd_issued == NULL ;j++) {
          int tindex = get_transaction_index(chan_id,dram_system.dram_controller[chan_id].pending_queue.pq_entry[i].transaction_list[j]);
          this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[tindex];
          this_c = this_t->next_c;
          first_queued_command_found = FALSE;
          bool attempt_schedule = false; // Stuff added for the update latency info
          bool issue_tran = false;
          while((this_c != NULL) && (first_queued_command_found == FALSE) ) {

            if((this_c->status == IN_QUEUE)){  /* command has not yet issued, lets see if we can issue it */
              /* Check if this is a CAS -> yes : try to issue;
               * 							no : skip till you get CAS */

              if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE|| (this_c->command == CAS || this_c->command == CAS_WRITE || 
                    this_c->command == CAS_WRITE_AND_PRECHARGE || this_c->command == CAS_AND_PRECHARGE)) {
                first_queued_command_found = TRUE;
              }
              attempt_schedule = true;
              if (can_issue_command(tindex,this_c) == TRUE && is_cmd_bus_idle(chan_id,this_c)){
                cmd_issued = this_c;
                issue_tran = true;
              }	  
            }
            this_c = this_c->next_c;
          }
        }
      }
    }
  }
  return cmd_issued;
}


void issue_cas(tick_t now,command_t *this_c, int chan_id) {
  this_c->status = LINK_COMMAND_TRANSMISSION;
  if (dram_system.config.dram_type == FBD_DDR2) {
	lock_buffer(this_c,chan_id);
	this_c->link_comm_tran_comp_time = now + dram_system.config.t_bundle + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus * (dram_system.config.rank_count - 1));
	this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + 
	  dram_system.config.t_amb_up + 
	  (this_c->posted_cas ? dram_system.config.row_command_duration: 0);
	this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time + 
	  dram_system.config.col_command_duration;
	this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time + 
	  dram_system.config.t_cac + 
	  (dram_system.config.posted_cas_flag? dram_system.config.t_al :0);
	this_c->dimm_data_tran_comp_time = this_c->dram_proc_comp_time + 
	  dram_system.config.t_burst;
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = 0;
	this_c->rank_done_time = this_c->dimm_data_tran_comp_time;
	this_c->completion_time = this_c->dimm_data_tran_comp_time;
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->data_bus_completion_time = this_c->dimm_data_tran_comp_time;	
	if (amb_buffer_debug()) 
	  print_buffer_contents(chan_id, this_c->rank_id);

  }else {
  int tran_index;
  tran_index = get_transaction_index(this_c->chan_id,this_c->tran_id);
  dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index].status = MEM_STATE_ISSUED;
  dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index].issued_cas= true;
  set_col_command_bus(chan_id,this_c->command);
  this_c->link_comm_tran_comp_time = now + 
	  (this_c->posted_cas ? dram_system.config.row_command_duration: 0) +
    dram_system.config.col_command_duration;
	this_c->amb_proc_comp_time = 0;
	this_c->dimm_comm_tran_comp_time = 0;
	this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time + 
	  dram_system.config.t_cac + 
	  (dram_system.config.posted_cas_flag? dram_system.config.t_al :0);
	this_c->dimm_data_tran_comp_time = 0;
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = this_c->dram_proc_comp_time + 
	  dram_system.config.t_burst;
    // Set up Data bus time
    dram_system.dram_controller[chan_id].data_bus.completion_time = this_c->link_data_tran_comp_time;
      dram_system.dram_controller[chan_id].data_bus.current_rank_id = this_c->rank_id;
    //now + dram_system.config.t_cas + 
    //(dram_system.config.posted_cas_flag? (dram_system.config.t_al + dram_system.config.row_command_duration):0) + dram_system.config.t_burst;
	
	  this_c->rank_done_time = this_c->link_data_tran_comp_time ;
	if (this_c->command == CAS_AND_PRECHARGE) {
      // Check for t_ras time now
      
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time = MAX(dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time, 
          this_c->link_data_tran_comp_time + dram_system.config.t_rtp) + dram_system.config.t_rp;
	}else {
	}
	  this_c->completion_time = this_c->rank_done_time;
  }
  this_c->status = LINK_COMMAND_TRANSMISSION;
}

void issue_cas_write(tick_t now,command_t *this_c, int chan_id) {
  this_c->status = LINK_COMMAND_TRANSMISSION;
  if (dram_system.config.dram_type == FBD_DDR2) {
	this_c->link_comm_tran_comp_time = now + 
	  dram_system.config.t_bundle + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus * (dram_system.config.rank_count - 1));
	this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + 
	  dram_system.config.t_amb_up+
	  (this_c->posted_cas == TRUE ? dram_system.config.row_command_duration: 0);
	this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time + 
	  dram_system.config.col_command_duration ;
	/** To calculate dimm data transmission time we need to check t_cwd */
	if (dram_system.config.t_cwd >= 0) {
	  if (dram_system.config.t_cwd > 0) {
		this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time + dram_system.config.t_cwd + 
		  (dram_system.config.posted_cas_flag ? dram_system.config.t_al : 0);  
		this_c->dimm_data_tran_comp_time = this_c->dram_proc_comp_time + dram_system.config.t_burst;
	  }else {
		this_c->dram_proc_comp_time = 0;
		this_c->dimm_data_tran_comp_time = this_c->dimm_comm_tran_comp_time + dram_system.config.t_cwd + dram_system.config.t_burst;
	  }
	}else {
	  this_c->dram_proc_comp_time = 0;
	  this_c->dimm_data_tran_comp_time = this_c->amb_proc_comp_time + dram_system.config.t_burst;
	}
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = 0;
	this_c->rank_done_time = this_c->dimm_data_tran_comp_time;
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->data_bus_completion_time = this_c->dimm_data_tran_comp_time;	
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time = this_c->dimm_data_tran_comp_time + dram_system.config.t_wr;	
	if (this_c->command == CAS_WRITE_AND_PRECHARGE) {
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time = 
	   MAX(  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time, this_c->link_data_tran_comp_time + dram_system.config.t_wr) + dram_system.config.t_rp;
	} else {
    }
	this_c->completion_time = this_c->rank_done_time;
  }else {
	set_col_command_bus(chan_id,this_c->command);
	this_c->link_comm_tran_comp_time = now +  (dram_system.config.posted_cas_flag ? dram_system.config.col_command_duration : 0) + dram_system.config.col_command_duration;
	this_c->amb_proc_comp_time = 0;
	this_c->dimm_comm_tran_comp_time = 0;
	/** To calculate dimm data transmission time we need to check t_cwd */
	if (dram_system.config.t_cwd >= 0) {
	  if (dram_system.config.t_cwd > 0) {
		this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_cwd + 
		  (dram_system.config.posted_cas_flag ? dram_system.config.t_al : 0);  
		this_c->link_data_tran_comp_time = this_c->dram_proc_comp_time + dram_system.config.t_burst;
	  }else {
		this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_cwd + 
		  (dram_system.config.posted_cas_flag ? dram_system.config.t_al : 0);
		this_c->link_data_tran_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_burst;
	  }
	}else {
	  this_c->dram_proc_comp_time = dram_system.current_dram_time;
	  this_c->link_data_tran_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_burst;
	}
	this_c->amb_down_proc_comp_time = 0;
	this_c->dimm_data_tran_comp_time = 0;
	this_c->rank_done_time = this_c->link_data_tran_comp_time;
   // Check if data bus time is being met at issue --- error checking DRAMsim
	dram_system.dram_controller[chan_id].data_bus.current_rank_id = MEM_STATE_INVALID;
	dram_system.dram_controller[chan_id].data_bus.completion_time = now + 
	  (this_c->posted_cas? dram_system.config.col_command_duration:0) +
	  dram_system.config.col_command_duration +
	  dram_system.config.t_cwd + 
	  (dram_system.config.posted_cas_flag ? dram_system.config.t_al : 0) +
	  dram_system.config.t_burst;

	if((dram_system.config.t_cwd + dram_system.config.col_command_duration) == 0){	/* no write delay */
	  dram_system.dram_controller[chan_id].data_bus.status = BUS_BUSY;
	  dram_system.dram_controller[chan_id].data_bus.completion_time = 
		dram_system.current_dram_time + dram_system.config.t_burst;
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_done_time = 
		dram_system.current_dram_time + dram_system.config.t_burst;
	}
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time = this_c->link_data_tran_comp_time + dram_system.config.t_wr;	
	  this_c->rank_done_time = this_c->link_data_tran_comp_time ;
	if (this_c->command == CAS_WRITE_AND_PRECHARGE) {
		dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time 
	   = MAX(  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time, this_c->link_data_tran_comp_time + dram_system.config.t_wr) + dram_system.config.t_rp;
	}else {
	}
   
	this_c->completion_time = this_c->link_data_tran_comp_time;
  }
  this_c->status = LINK_COMMAND_TRANSMISSION;
}

void issue_cas_with_drive(tick_t now,command_t *this_c, int chan_id) {
//  int tran_index;
//  tran_index = get_transaction_index(this_c->tran_id);
//  dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index].status = MEM_STATE_ISSUED;
  this_c->status = LINK_COMMAND_TRANSMISSION;
  if (dram_system.config.dram_type == FBD_DDR2) {
	lock_buffer(this_c,chan_id);
	this_c->link_comm_tran_comp_time = now + dram_system.config.t_bundle + 
	  (dram_system.config.var_latency_flag? dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus*(dram_system.config.rank_count - 1));

	this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + 
	  dram_system.config.t_amb_up + 
	  (this_c->posted_cas ? dram_system.config.row_command_duration: 0);
	this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time + 
	  dram_system.config.col_command_duration;
	this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time +
	  dram_system.config.t_cac + 
	  (dram_system.config.posted_cas_flag? dram_system.config.t_al :0);
	this_c->dimm_data_tran_comp_time = this_c->dram_proc_comp_time +
	  dram_system.config.t_burst;
    /* Cacheline size in Bytes
     * Channel Width in Bytes
     * DATA_BYTES_PER_READ_BUNDLE in Bytes
     * To create a bundle you need to have burst 
     * DATA_BYTES_PER_READ_BUNDLE/Channel_Width bytes 
     * Get Fraction of time for this operation
     * After this link data starts
     */
	this_c->amb_down_proc_comp_time = this_c->dimm_data_tran_comp_time +
	  dram_system.config.t_amb_down;
	this_c->link_data_tran_comp_time = this_c->dram_proc_comp_time +
     (dram_system.config.t_burst * DATA_BYTES_PER_READ_BUNDLE/dram_system.config.cacheline_size) +
      dram_system.config.t_amb_down +
	  dram_system.config.t_bundle * this_c->data_word + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus * (dram_system.config.rank_count - 1));
	this_c->rank_done_time = this_c->link_data_tran_comp_time;
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->data_bus_completion_time = this_c->dimm_data_tran_comp_time;	
	dram_system.dram_controller[chan_id].down_bus.completion_time = this_c->link_data_tran_comp_time;
	if (this_c->command == CAS_AND_PRECHARGE) {
		dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time 
	   = MAX(this_c->dimm_data_tran_comp_time + dram_system.config.t_rtp,
          dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time) +dram_system.config.t_rp;
	}
	this_c->completion_time = this_c->link_data_tran_comp_time;
	if (amb_buffer_debug()) 
	  print_buffer_contents(chan_id, this_c->rank_id);

  }else {
  }
}


/*** FIXME : fbd - check that ras timing is met as far as bank goes rcd/ras
 * difference ***/
void issue_ras(tick_t now,command_t *this_c, int chan_id) {
  int tran_index;
  /** Check as refresh commands do not map onto transaction queue **/
  this_c->status = LINK_COMMAND_TRANSMISSION;
  if (dram_system.config.dram_type == FBD_DDR2) {
	this_c->link_comm_tran_comp_time = now + 
	  dram_system.config.t_bundle + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus * (dram_system.config.rank_count - 1));
	this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + 
	  dram_system.config.t_amb_up;
	this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time + 
	  dram_system.config.row_command_duration;
	this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time + 
	  dram_system.config.t_rcd;
	this_c->dimm_data_tran_comp_time = 0;
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = 0;
	this_c->rank_done_time = this_c->dimm_comm_tran_comp_time +   dram_system.config.row_command_duration +  dram_system.config.t_ras;
	this_c->completion_time = this_c->dram_proc_comp_time;
	if (this_c->command == RAS) {
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time = this_c->amb_proc_comp_time + dram_system.config.t_ras;
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].row_id = this_c->row_id;
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rcd_done_time = this_c->amb_proc_comp_time + dram_system.config.t_rcd;
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	

	}else { /** RAS_ALL **/
	  int i,j;
	  for (i=0;i<dram_system.config.rank_count;i++) {
		 dram_system.dram_controller[chan_id].rank[i].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	
		if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
		  for (j=0;j<dram_system.config.bank_count;j++) {
			dram_system.dram_controller[chan_id].rank[i].bank[j].ras_done_time =   this_c->amb_proc_comp_time +
			  dram_system.config.t_ras;
			dram_system.dram_controller[chan_id].rank[i].bank[j].rcd_done_time =   this_c->dram_proc_comp_time;
			dram_system.dram_controller[chan_id].rank[i].bank[j].row_id =   this_c->row_id;
		  }
		}else if  (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
		  dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].ras_done_time =   this_c->dram_proc_comp_time;
			dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].row_id =   this_c->row_id;
		}	
	  }

	}


  }else {
	set_row_command_bus(chan_id,this_c->command);

	this_c->link_comm_tran_comp_time = now + dram_system.config.row_command_duration;
	this_c->amb_proc_comp_time = 0;
	this_c->dimm_comm_tran_comp_time = 0;
	this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_rcd;
	this_c->dimm_data_tran_comp_time = 0;
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = 0;
	this_c->rank_done_time = now + dram_system.config.t_ras;
	tran_index = get_transaction_index(this_c->chan_id,this_c->tran_id);
	dram_system.dram_controller[this_c->chan_id].transaction_queue.entry[tran_index].status = MEM_STATE_ISSUED;
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].row_id = this_c->row_id;
	 dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time =  this_c->rank_done_time;
	 dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rcd_done_time = this_c->dram_proc_comp_time;
	this_c->completion_time = this_c->dram_proc_comp_time;

  }

  /* keep track of activation history for tFAW and tRRD */
  {
	int i;
	for(i = 3;i>0;i--){
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].activation_history[i] =
		dram_system.dram_controller[chan_id].rank[this_c->rank_id].activation_history[i-1];
	}
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].activation_history[0] = now;
  }



}

void issue_prec (tick_t now,command_t *this_c, int chan_id) {
  /** Check as refresh commands do not map onto transaction queue **/
  this_c->status = LINK_COMMAND_TRANSMISSION;

  if (dram_system.config.dram_type == FBD_DDR2) {
	this_c->link_comm_tran_comp_time = now + dram_system.config.t_bundle + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus * (dram_system.config.rank_count - 1));
	this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_amb_up;
	this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time + dram_system.config.row_command_duration;
	this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time + dram_system.config.t_rp;
	this_c->dimm_data_tran_comp_time = 0;
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = 0;
	this_c->rank_done_time = this_c->dimm_comm_tran_comp_time;
	this_c->completion_time = this_c->dram_proc_comp_time;

	if (this_c->command == PRECHARGE) {
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time = this_c->dram_proc_comp_time;
		dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	
	}else { /** PRECHARGE_ALL **/
	  int i,j;
	  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
			for (j=0;j<dram_system.config.bank_count;j++) {
			  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].rp_done_time =  this_c->dram_proc_comp_time;
			}
	  }else {
		for (i=0;i<dram_system.config.rank_count;i++) {
		  dram_system.dram_controller[chan_id].rank[i].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	
		  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
			for (j=0;j<dram_system.config.bank_count;j++) {
			  dram_system.dram_controller[chan_id].rank[i].bank[j].rp_done_time =  this_c->dram_proc_comp_time;
			}
		  }else if  (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
			dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].rp_done_time =  
			  this_c->dram_proc_comp_time;
		  }
		}
	  }
	}
	
    
  }else {

	set_row_command_bus(chan_id,this_c->command);
	this_c->link_comm_tran_comp_time = now + dram_system.config.row_command_duration;
	this_c->amb_proc_comp_time = 0;
	this_c->dimm_comm_tran_comp_time = 0;
	this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_rp;
	this_c->dimm_data_tran_comp_time = 0;
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = 0;
	this_c->rank_done_time = this_c->dram_proc_comp_time;
	this_c->completion_time = this_c->dram_proc_comp_time;
	
	if (this_c->command == PRECHARGE) {
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time = this_c->dram_proc_comp_time;
	}else { /** PRECHARGE_ALL **/
	  int i,j;
	  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
		for (j=0;j<dram_system.config.bank_count;j++) {
		  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].rp_done_time =  this_c->dram_proc_comp_time;
		}
	  }else {
		for (i=0;i<dram_system.config.rank_count;i++) {
		  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
			for (j=0;j<dram_system.config.bank_count;j++) {
			  dram_system.dram_controller[chan_id].rank[i].bank[j].rp_done_time =  this_c->dram_proc_comp_time;
			}
		  }else if  (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
			dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].rp_done_time =  
			  this_c->dram_proc_comp_time;
		  }
		}
	  }
	}


  }
}


void issue_ras_or_prec(tick_t now,command_t *this_c, int chan_id) {
  int tran_index;
  tran_index = get_transaction_index(this_c->chan_id,this_c->tran_id);
  dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index].status = MEM_STATE_ISSUED;
  set_row_command_bus(chan_id,this_c->command);
  this_c->status = LINK_COMMAND_TRANSMISSION;
}

void issue_data(tick_t now,command_t *this_c, int chan_id) {
//  int tran_index;
//  tran_index = get_transaction_index(this_c->tran_id);
  if (dram_system.config.dram_type != FBD_DDR2) {
	fprintf(stderr, "are we even doing this?");
  } else {
	this_c->status = LINK_COMMAND_TRANSMISSION;
	lock_buffer(this_c,chan_id);

	this_c->link_comm_tran_comp_time = now + dram_system.config.t_bundle + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus * (dram_system.config.rank_count - 1));
	this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_amb_up;
	this_c->dimm_comm_tran_comp_time = 0;
	this_c->dram_proc_comp_time = 0;
	this_c->dimm_data_tran_comp_time = 0;
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = 0;
	this_c->rank_done_time = this_c->amb_proc_comp_time;
	this_c->completion_time = this_c->rank_done_time;
	if (amb_buffer_debug()) 
	  print_buffer_contents(chan_id, this_c->rank_id);

  }

}

void issue_drive(tick_t now,command_t *this_c, int chan_id) {
//  int tran_index;
//  tran_index = get_transaction_index(this_c->tran_id);
 // dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index].status = MEM_STATE_ISSUED;
  if (dram_system.config.dram_type != FBD_DDR2) {
	fprintf(stderr, "are we even doing this?");
  } else {
	this_c->link_comm_tran_comp_time = now + dram_system.config.t_bundle + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus * (dram_system.config.rank_count - 1));
	this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_amb_up;
	this_c->dimm_comm_tran_comp_time = 0;
	this_c->dram_proc_comp_time = 0;
	this_c->dimm_data_tran_comp_time = 0;
	this_c->amb_down_proc_comp_time = this_c->amb_proc_comp_time + dram_system.config.t_amb_down;
	this_c->link_data_tran_comp_time = this_c->amb_down_proc_comp_time + this_c->data_word * dram_system.config.t_bundle;
	this_c->rank_done_time = this_c->link_data_tran_comp_time;
	dram_system.dram_controller[chan_id].down_bus.completion_time = this_c->amb_down_proc_comp_time +
	        dram_system.config.t_bundle * this_c->data_word;	
	this_c->completion_time = this_c->rank_done_time;
  }
  this_c->status = LINK_COMMAND_TRANSMISSION;
}


void issue_refresh(tick_t now,command_t *this_c, int chan_id) {
  this_c->status = LINK_COMMAND_TRANSMISSION;
  if (dram_system.config.dram_type == FBD_DDR2) {
	this_c->link_comm_tran_comp_time = now + 
	  dram_system.config.t_bundle + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus * (dram_system.config.rank_count - 1));
	this_c->amb_proc_comp_time = this_c->link_comm_tran_comp_time + 
	  dram_system.config.t_amb_up;
	this_c->dimm_comm_tran_comp_time = this_c->amb_proc_comp_time + 
	  dram_system.config.row_command_duration;
	this_c->dimm_data_tran_comp_time = 0;
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = 0;

	if (this_c->command == REFRESH) { /* Single bank refresh */
	  this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time + 
		dram_system.config.t_rc;
	  this_c->rank_done_time = this_c->dram_proc_comp_time;
		this_c->completion_time = this_c->rank_done_time;
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time = this_c->dram_proc_comp_time;
	  //	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time = this_c->dram_proc_comp_time;
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	

    }else { /** REFRESH_ALL **/
      this_c->dram_proc_comp_time = this_c->dimm_comm_tran_comp_time + 
        dram_system.config.t_rfc;
      this_c->rank_done_time = this_c->dram_proc_comp_time;
      this_c->completion_time = this_c->rank_done_time;
      int i,j;
      if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
        dram_system.dram_controller[chan_id].rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	
        for (j=0;j<dram_system.config.bank_count;j++) {
          dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].rp_done_time =   this_c->dram_proc_comp_time;
        }
      }else {
        for (i=0;i<dram_system.config.rank_count;i++) {
          dram_system.dram_controller[chan_id].rank[i].my_dimm->fbdimm_cmd_bus_completion_time = this_c->dimm_comm_tran_comp_time;	
          if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
            for (j=0;j<dram_system.config.bank_count;j++) {
              dram_system.dram_controller[chan_id].rank[i].bank[j].rp_done_time =   this_c->dram_proc_comp_time;
            }
          }else if  (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
            dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].rp_done_time =   this_c->dram_proc_comp_time;
          }	
        }
      }

	}


  }else {
	set_row_command_bus(chan_id,this_c->command);

	this_c->link_comm_tran_comp_time = now + dram_system.config.row_command_duration;
	this_c->amb_proc_comp_time = 0;
	this_c->dimm_comm_tran_comp_time = 0;
	this_c->dram_proc_comp_time = this_c->link_comm_tran_comp_time + dram_system.config.t_rc;
	this_c->dimm_data_tran_comp_time = 0;
	this_c->amb_down_proc_comp_time = 0;
	this_c->link_data_tran_comp_time = 0;
	
	this_c->rank_done_time = this_c->dram_proc_comp_time;
	if (this_c->command == REFRESH) {
	  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time = this_c->dram_proc_comp_time;

    }else { /** REFRESH_ALL **/
      if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
        int j;
        for (j=0;j<dram_system.config.bank_count;j++) {
          dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].rp_done_time =   this_c->dram_proc_comp_time;
        }
      }else {

        int i,j;
        for (i=0;i<dram_system.config.rank_count;i++) {
          if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
            for (j=0;j<dram_system.config.bank_count;j++) {
              dram_system.dram_controller[chan_id].rank[i].bank[j].rp_done_time =   this_c->dram_proc_comp_time;
            }
          }else if  (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
            dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].rp_done_time =   this_c->dram_proc_comp_time;
          }	
        }
      }

    }
  }
}

void update_critical_word_info (tick_t now,command_t * this_c,transaction_t *this_t) {
  if(dram_system.config.critical_word_first_flag == TRUE ){
	if (dram_system.config.dram_type == FBD_DDR2) 
	  this_t->critical_word_ready_time = this_c->amb_down_proc_comp_time + dram_system.config.t_bundle + dram_system.config.tq_delay;
	else
	  this_t->critical_word_ready_time = this_c->dram_proc_comp_time + dram_system.config.dram_clock_granularity +dram_system.config.tq_delay;
  }else {
	this_t->critical_word_ready_time = this_c->link_data_tran_comp_time + dram_system.config.tq_delay;
  }

}
/**
 * Generic Issue FBD Command
 */
void issue_fbd_command(tick_t now,command_t *this_c,int chan_id,transaction_t* this_t,char *debug_string){

  assert(chan_id>=0 && chan_id< dram_system.config.channel_count);
  assert(this_c != NULL);

  switch (this_c->command) {
	case DATA : issue_data (now,this_c,chan_id);
				break;
	case DRIVE : issue_drive (now,this_c,chan_id);
				 break;
	case RAS : issue_ras (now,this_c,chan_id);
			   break;
	case CAS : issue_cas (now,this_c,chan_id);
			   break;
	case CAS_AND_PRECHARGE:
	case CAS_WITH_DRIVE : issue_cas_with_drive (now,this_c,chan_id);
						  break;
	case CAS_WRITE_AND_PRECHARGE:
	case CAS_WRITE : issue_cas_write (now,this_c,chan_id);
					 break;
	case PRECHARGE : issue_prec (now,this_c,chan_id);
					 break;
	case RAS_ALL : issue_ras(now,this_c,chan_id);
				   break;
	case PRECHARGE_ALL : issue_prec(now,this_c,chan_id);
						 break;
	case REFRESH_ALL : issue_refresh(now,this_c,chan_id);
					   break;
	case REFRESH : issue_refresh(now,this_c,chan_id);
				   break;
  }
  this_c->start_transmission_time = now; 
  this_c->status = LINK_COMMAND_TRANSMISSION;

  if (this_c->refresh == FALSE) {
	if (this_c->command == DATA && this_t->status != MEM_STATE_ISSUED) {
	  this_t->status = MEM_STATE_DATA_ISSUED;
	}else {
	  this_t->status = MEM_STATE_ISSUED;
	}
	
  }
  if (this_c->command == CAS || this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_WRITE || this_c->command == CAS_AND_PRECHARGE || this_c->command == CAS_WRITE_AND_PRECHARGE) {
    //print_command_detail(now,this_c);
	    this_t->issued_cas	= true;
	    dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras++;
  }
  if (this_c->command == RAS) {
	this_t->issued_ras = true;
	if (dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras!= 0)
	  	mem_gather_stat(GATHER_BANK_CONFLICT_STAT, 
			(int) (dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras));
	    dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras=0;

  }
  if (get_tran_watch(this_c->tran_id) || wave_debug() || (cas_wave_debug() && (this_c->command == CAS_WITH_DRIVE))) build_wave(debug_string, this_c->tran_id, this_c, MEM_STATE_SCHEDULED);

  // Do the power update
  update_power_stats(now,this_c);
  if (this_c->command == CAS_AND_PRECHARGE || this_c->command == CAS || this_c->command == CAS_WITH_DRIVE) {
	update_critical_word_info (now,this_c,this_t);
  }
	//print_command(now,this_c);
}

/**
 * Generic Issue Command
 */
void issue_cmd(tick_t now,command_t *this_c,transaction_t *this_t,int chan_id){

  assert(chan_id>=0 && chan_id< dram_system.config.channel_count);
  //assert(this_c != NULL);
  //assert(this_c->status == IN_QUEUE);
  switch (this_c->command) {
	case RAS : issue_ras (now,this_c,chan_id);
			   break;
	case CAS_AND_PRECHARGE : 
	case CAS : issue_cas (now,this_c,chan_id);
			   break;
	case CAS_WITH_DRIVE : issue_cas_with_drive (now,this_c,chan_id);
						  break;
	case CAS_WRITE_AND_PRECHARGE:
	case CAS_WRITE : issue_cas_write (now,this_c,chan_id);
					 break;
	case PRECHARGE : issue_prec (now,this_c,chan_id);
					 break;
	case RAS_ALL : issue_ras(now,this_c,chan_id);
				   break;
	case PRECHARGE_ALL : issue_prec(now,this_c,chan_id);
						 break;
	case REFRESH_ALL : issue_refresh(now,this_c,chan_id);
					   break;
	case REFRESH : issue_refresh(now,this_c,chan_id);
				   break;
  }
  if (dram_system.dram_controller[chan_id].rank[this_c->rank_id].cke_bit == false ||
	  (dram_system.dram_controller[chan_id].rank[this_c->rank_id].cke_bit == true && 
	   dram_system.dram_controller[chan_id].rank[this_c->rank_id].cke_done < this_c->rank_done_time)) {
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].cke_bit = true;
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].cke_tid = this_c->tran_id;
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].cke_cmd = this_c->command;
	dram_system.dram_controller[chan_id].rank[this_c->rank_id].cke_done = this_c->rank_done_time;
#ifdef DEBUG_POWER 
	fprintf(stdout," [%llu] CKE Bit r[%d] c[%d] t[%d] Till[%llu] ",now,
		this_c->rank_id,
		chan_id,
		this_c->tran_id,
		this_c->rank_done_time);
#endif
  }
  if (!this_c->refresh)
	  this_t->status = MEM_STATE_ISSUED;
  this_c->start_transmission_time = now;
  //if (this_c->command == CAS || this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_WRITE)
  //dram_system.dram_controller[chan_id].transaction_queue.entry[get_transaction_index(this_c->tran_id)].issue_cas = true;	

  if (get_tran_watch(this_c->tran_id) ) print_command(now, this_c );
  // Do the power update
  update_power_stats(now,this_c);
  if (this_c->command == CAS_AND_PRECHARGE || this_c->command == CAS || this_c->command == CAS_WITH_DRIVE) {
	int tran_index = get_transaction_index(this_c->chan_id,this_c->tran_id);
	update_critical_word_info (now,this_c,&dram_system.dram_controller[this_c->chan_id].transaction_queue.entry[tran_index]);
  }
  if (this_c->command == CAS || this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_WRITE || this_c->command == CAS_AND_PRECHARGE || this_c->command == CAS_WRITE_AND_PRECHARGE) {
    //print_command_detail(now,this_c);
	    this_t->issued_cas	= true;
	    dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras++;
  }
  if (this_c->command == RAS) {
	this_t->issued_ras = true;
	if (dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras!= 0)
	  	mem_gather_stat(GATHER_BANK_CONFLICT_STAT, 
			(int) (dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras));
	    dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_count_since_ras=0;

  }
  if ((dram_system.config.issue_debug && (dram_system.tq_info.transaction_id_ctr >= dram_system.tq_info.debug_tran_id_threshold))) {
    if (this_c->posted_cas)
    print_command_detail(now+dram_system.config.row_command_duration,this_c);
    else
    print_command_detail(now,this_c);
  }
}

