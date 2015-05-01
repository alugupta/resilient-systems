/*
 * mem-commandissuetest.c : This file comprises of routines which test if
 * individual commands maybe released without violating timing constraints/
 * creating conflicts ont he various buses etc.
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

// Just calculates the cmd bus and dram reqd time 
void get_reqd_times(command_t * this_c,tick_t *cmd_bus_reqd_time,tick_t *dram_reqd_time) {
  if (dram_system.config.dram_type == FBD_DDR2) {
 *cmd_bus_reqd_time = dram_system.current_dram_time + 
	(dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id ): dram_system.config.t_bus*(dram_system.config.rank_count - 1))+
	dram_system.config.t_bundle +
	dram_system.config.t_amb_up;
  }else 
	*cmd_bus_reqd_time = dram_system.current_dram_time;

  *dram_reqd_time = *cmd_bus_reqd_time + dram_system.config.row_command_duration;
	return;
}

extern dram_system_t dram_system;

bool fbd_can_issue_cas(int tran_index, command_t *this_c);
bool fbd_cas_with_drive_timing_chk(int tran_index, command_t *this_c, tick_t cmd_bus_reqd_time, tick_t data_bus_reqd_time, tick_t down_bus_reqd_time);
bool fbd_cas_read_timing_chk(int tran_index, command_t *this_c,tick_t cmd_bus_reqd_time, tick_t data_bus_reqd_time);
bool fbd_cas_write_timing_chk(int tran_index, command_t *this_c,tick_t cmd_bus_reqd_time, tick_t data_bus_reqd_time);
bool check_for_refresh(command_t * this_c,int tran_index,tick_t cmd_bus_reqd_time) ;
bool check_cmd_bus_required_time (command_t * this_c, tick_t cmd_bus_reqd_time);
bool check_data_bus_required_time (command_t * this_c, tick_t data_bus_reqd_time);
bool check_down_bus_required_time (command_t * this_c, command_t *temp_c,tick_t down_bus_reqd_time);


bool can_issue_command(int tran_index, command_t *this_c) {
  switch (this_c->command) {
	case CAS:
	case CAS_WITH_DRIVE:
	case CAS_WRITE:
	  return can_issue_cas(tran_index, this_c);
	case CAS_WRITE_AND_PRECHARGE:
	case CAS_AND_PRECHARGE:
	  return can_issue_cas_with_precharge(tran_index, this_c);
	case RAS:
	  return can_issue_ras(tran_index, this_c);
	case PRECHARGE:
	  return can_issue_prec(tran_index, this_c);
	case DATA:
	  return can_issue_data(tran_index,this_c);
	case DRIVE:
	  return can_issue_drive(tran_index, this_c);
	default:
	  fprintf(stderr,"Not sure of command type, unable to issue");
	  return false;
  }

  return false; 
}

/* 
 * This code checks to see if the bank is open, or will be open in time, and if the data bus is free so a CAS can be issued.
 * command is read or write 
 */

bool can_issue_cas(int tran_index, command_t *this_c){
  int i;
  int bank_is_open;
  int bank_willbe_open;
  int rank_id, bank_id, row_id;
  tick_t data_bus_free_time = 0;
  tick_t data_bus_reqd_time = 0;
  dram_controller_t *this_dc;

  this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  rank_id = this_c->rank_id;
  bank_id = this_c->bank_id;
  row_id  = this_c->row_id;

  bank_is_open = FALSE;
  bank_willbe_open = FALSE;

  data_bus_free_time = this_dc->data_bus.completion_time; 

  if (dram_system.config.dram_type == FBD_DDR2) {
    return (fbd_can_issue_cas(tran_index, this_c));

  }else {
    if(this_c->command == CAS || this_c->command == CAS_AND_PRECHARGE){ 
      data_bus_reqd_time	= dram_system.current_dram_time +  dram_system.config.t_cas;
      if(this_dc->data_bus.current_rank_id != rank_id){ 		/* if the existing data bus isn't owned by the current rank */
        data_bus_free_time += dram_system.config.t_dqs; 
      }
      /* In these systems, a read cannot start in a given rank until writes fully complete */
      if((dram_system.config.dram_type == SDRAM) || (dram_system.config.dram_type == DDRSDRAM) || (dram_system.config.dram_type == DDR2)){
        if (dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time >
            (dram_system.current_dram_time + dram_system.config.t_cmd)){
#ifdef DEBUG
          if (get_tran_watch(this_c->tran_id)) {
            fprintf(stdout,"[%llu]CAS(%llu) t_wr conflict  [%llu] \n",dram_system.current_dram_time, this_c->tran_id,
                dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time); 
          }
#endif
          return false;

        }
      }
    } else if (this_c->command == CAS_WRITE || this_c->command == CAS_WRITE_AND_PRECHARGE) {
      if(this_dc->data_bus.current_rank_id != MEM_STATE_INVALID) { 		/* if the existing data bus isn't owned by the controller */
        data_bus_free_time += dram_system.config.t_dqs; 
      }
      if (get_tran_watch(this_c->tran_id)) {
        fprintf(stderr," [%llu] Seting data bus reqd time to %llu + t_cwd %d\n",
            dram_system.current_dram_time,
            dram_system.current_dram_time,
            dram_system.config.t_cwd);
      }
	  if (dram_system.config.t_cwd >=0)
	      data_bus_reqd_time	= dram_system.current_dram_time + dram_system.config.t_cwd;
      else
	      data_bus_reqd_time	= dram_system.current_dram_time;
    }

    if(dram_system.config.posted_cas_flag == TRUE){
      data_bus_reqd_time	+= dram_system.config.t_al; // This seems to happen ??
    }

    if(data_bus_free_time > data_bus_reqd_time){			/* if data bus isn't going to be free in time, return false */
#ifdef DEBUG
      if (get_tran_watch(this_c->tran_id)) {
        fprintf(stdout,"[%llu]CAS(%llu) Data Bus Not Free  till [%llu] reqd [%llu]\n",dram_system.current_dram_time, 
            this_c->tran_id,
            data_bus_free_time,
            data_bus_reqd_time); 
      }
#endif
      return false;
    }
	
	if (dram_system.config.row_buffer_management_policy == PERFECT_PAGE) {	/* This policy doesn't case about issue rules as long as data bus is ready */
	  return true;				
	} 

	if (dram_system.config.strict_ordering_flag == TRUE){			/* if strict_ordering is set, suppress if earlier CAS exists in queue */
	  for(i=0;i<tran_index;i++){
		if (!this_dc->transaction_queue.entry[i].issued_cas) {
#ifdef DEBUG
		  if (get_tran_watch(this_c->tran_id)) {
			fprintf(stdout,"[%llu]CAS(%llu) Strict Ordering In force \n",dram_system.current_dram_time, this_c->tran_id); 
		  }
#endif
		  return false;
		}
	  }
	}
	/** Check if all previous Refresh commands i.e. RAS_ALL /PREC_ALL have been
	 * issued **/
	if (dram_system.config.auto_refresh_enabled == TRUE) {
	  if (check_for_refresh(this_c,tran_index,dram_system.current_dram_time) == FALSE){
#ifdef DEBUG
		if (get_tran_watch(this_c->tran_id)) {
		  fprintf(stdout,"[%llu]CAS(%llu) Refresh conflict  \n",dram_system.current_dram_time, this_c->tran_id); 
		}
#endif
		return false;
	  }
	}
	/* Check if bank is ready i.e.
	 * rcd_done_time > rp_done_time
	 * row open == reqd row
	 * rcd_done_time <= dram_reqd_time
	 */
	tick_t dram_reqd_time = dram_system.current_dram_time + dram_system.config.col_command_duration;
	if (dram_system.config.posted_cas_flag == TRUE)
	  dram_reqd_time += dram_system.config.t_al;
	if ((this_dc->rank[rank_id].bank[bank_id].rcd_done_time > this_dc->rank[rank_id].bank[bank_id].rp_done_time)	&&
		(this_dc->rank[rank_id].bank[bank_id].row_id == row_id) &&
		(this_dc->rank[rank_id].bank[bank_id].rcd_done_time <= dram_reqd_time)){
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) {
		fprintf(stdout," Issuing the CAS\n");
	  }
#endif
	  return true;
	}else {
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) {
		fprintf(stdout,"[%llu]CAS(%llu) Bank Not Open: rcd_done [%llu] rp_done[%llu] ras_done[%llu] Status (%d) row_id(%x) my row_id (%x)\n",
			dram_system.current_dram_time,
			this_c->tran_id,
			this_dc->rank[rank_id].bank[bank_id].rcd_done_time,
			this_dc->rank[rank_id].bank[bank_id].rp_done_time,
			this_dc->rank[rank_id].bank[bank_id].ras_done_time,
			this_dc->rank[rank_id].bank[bank_id].status,
			this_dc->rank[rank_id].bank[bank_id].row_id,
			row_id
			);
	  }
#endif
	  return false;

	}
  }
#ifdef DEBUG
  if (get_tran_watch(this_c->tran_id)) {
	fprintf(stdout," Issuing the CAS\n");
  }
#endif
  return true;
}

/* 
 *  RAS can only be issued if it's idle or if its precharging and trp will expire.
 */

bool can_issue_ras(int tran_index, command_t *this_c){
  dram_controller_t *this_dc = &dram_system.dram_controller[this_c->chan_id];
  int rank_id = this_c->rank_id, bank_id = this_c->bank_id;
  tick_t cmd_bus_reqd_time = 0;
  tick_t dram_reqd_time = 0;
  transaction_t * this_t = &(this_dc->transaction_queue.entry[tran_index]);
  command_t *temp_c = NULL;
  int i;
  get_reqd_times (this_c,&cmd_bus_reqd_time,&dram_reqd_time);

  /* Check that your dat has been issued for a write command */
  if (dram_system.config.dram_type == FBD_DDR2 && this_t->transaction_type == MEMORY_WRITE_COMMAND && !this_t->issued_data ) {
#ifdef DEBUG
    if (get_tran_watch(this_c->tran_id))  fprintf(stdout,"[%llu]RAS(%llu) Data still not started to be issued ",dram_system.current_dram_time, this_c->tran_id);
#endif
    return false;
  }
  /* Check for conflicts with refresh */ 
  if (dram_system.config.auto_refresh_enabled == TRUE) {
	if (check_for_refresh(this_c,tran_index,cmd_bus_reqd_time) == FALSE){
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) 
		fprintf(stdout,"[%llu]RAS(%lu) Refresh Conflict\n",dram_system.current_dram_time, this_c->tran_id); 
#endif
	  return false;
	}
  }

  /* Check for whether the bank is in the right state */
  if((this_dc->rank[rank_id].bank[bank_id].ras_done_time <= this_dc->rank[rank_id].bank[bank_id].rp_done_time ) &&
	  (this_dc->rank[rank_id].bank[bank_id].rp_done_time <= dram_reqd_time)){
  } else {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) 
	  fprintf(stdout,"[%llu]RAS(%llu) Bank Conflict RP Done Time %llu Ras done time %llu \n",dram_system.current_dram_time, this_c->tran_id,
		  this_dc->rank[rank_id].bank[bank_id].rp_done_time,
		  this_dc->rank[rank_id].bank[bank_id].ras_done_time); 
#endif
	return false;
  }
  /** Check for conflict on command bus **/
  if (check_cmd_bus_required_time ( this_c, cmd_bus_reqd_time) == FALSE) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) 
	  printf("[%llu]RAS(%llu) DIMM Bus Busy \n",dram_system.current_dram_time, this_c->tran_id); 
#endif
	return false;
  }
  /* If type = DDR2 or DDR3, check to see that tRRD and tFAW is honored */
  if ((dram_system.config.dram_type == DDR2) || (dram_system.config.dram_type == DDR3) || (dram_system.config.dram_type == FBD_DDR2)){
	if((dram_system.current_dram_time - dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].activation_history[0]) <
		dram_system.config.t_rrd){
	  this_dc->t_rrd_violated++;
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) {
		fprintf(stdout,"[%llu]RAS(%llu) t_RRD violated \n",dram_system.current_dram_time, this_c->tran_id); 
	  }
#endif
	  return false;           /* violates t_RRD */
	} else if((dram_system.current_dram_time - dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].activation_history[3]) <
		dram_system.config.t_faw){
	  /*
		 fprintf(stdout,"tFAW [%d] \n",(int)dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].activation_history[3]);
		 */
	  this_dc->t_faw_violated++;
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) {
		fprintf(stdout,"[%llu]RAS(%llu) t_FAW violated \n",dram_system.current_dram_time, this_c->tran_id); 
	  }
#endif
	  return false;           /* violates t_FAW */
	}
  }

  /*** We have to traverse the entire queue unless we set up a data bus
   * for each rank and track it 
   * Do not issue ras for the followign reasons
   * 1. Previous commands i.e. RAS/CAS to the same bank are incomplete i.e. 
   * reached mem_state_complete.
   * **/
  int sched_policy = get_transaction_selection_policy(global_biu);
  for (i=0;i< this_dc->transaction_queue.transaction_count; i ++) {
    temp_c = this_dc->transaction_queue.entry[i].next_c;
    if( temp_c->rank_id == rank_id &&
        temp_c->bank_id == bank_id) {
      if (temp_c->row_id == this_c->row_id && i < tran_index &&
          !this_dc->transaction_queue.entry[i].status != MEM_STATE_ISSUED) {
        /* this implies that its probably this trans that issued the
         * precharge */
#ifdef DEBUG			  
        if (get_tran_watch(this_c->tran_id)) 
          fprintf(stdout,"[%llu]RAS(%llu) !issued cmd Tran[%d] Issued command-staus[%d]  \n",dram_system.current_dram_time, this_c->tran_id,
              i,this_dc->transaction_queue.entry[i].status);
#endif
        return false;
      }
      if (dram_system.config.row_buffer_management_policy == OPEN_PAGE && i!= tran_index &&
          this_dc->transaction_queue.entry[i].status != MEM_STATE_SCHEDULED) {
        if (this_t->status != MEM_STATE_ISSUED) {
          if (this_dc->transaction_queue.entry[i].status == MEM_STATE_ISSUED){
            /* this implies that its probably this trans that issued the
             * precharge */
#ifdef DEBUG			  
            if (get_tran_watch(this_c->tran_id)) 
              fprintf(stdout,"[%llu]RAS(%llu) !issued cmd Tran[%d] Issued command-staus[%d]  \n",dram_system.current_dram_time, this_c->tran_id,
                  i,this_dc->transaction_queue.entry[i].status);
#endif
            return false;
          }
        }
      }else if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE &&
          i < tran_index && this_dc->transaction_queue.entry[i].status != MEM_STATE_SCHEDULED) {

      }
      if (sched_policy == FCFS && i < tran_index   && temp_c->status == IN_QUEUE && temp_c->command != DATA) { // you should never go out of order
#ifdef DEBUG
        if (get_tran_watch(this_c->tran_id))
          fprintf(stdout,"[%lu]RAS(%lu) Bank open to row reqd by [%lu] \n",dram_system.current_dram_time,
              this_c->tran_id, temp_c->tran_id);
#endif
        return false;
      }

    }

  }

  	/** Should we be doing this ?? **/
  if(dram_system.config.posted_cas_flag == TRUE && this_c->refresh == FALSE){		/* posted CAS flag set */
    if ((dram_system.config.dram_type == DDR2) || (dram_system.config.dram_type == DDR3) ){
      tick_t data_bus_free_time = this_dc->data_bus.completion_time; 
      tick_t data_bus_reqd_time	= 0;  
	  if (this_t->transaction_type == MEMORY_WRITE_COMMAND) {
		if(this_dc->data_bus.current_rank_id != MEM_STATE_INVALID){ 		/* if the existing data bus isn't owned by the current rank */
		  data_bus_free_time += dram_system.config.t_dqs; 
		}
		data_bus_reqd_time = dram_reqd_time +
        dram_system.config.col_command_duration + 
        dram_system.config.t_al +
        dram_system.config.t_cwd;
	  }else {
		if(this_dc->data_bus.current_rank_id != rank_id){ 		/* if the existing data bus isn't owned by the current rank */
		  data_bus_free_time += dram_system.config.t_dqs; 
		}
		data_bus_reqd_time = dram_reqd_time +
        dram_system.config.t_cas+ 
        dram_system.config.t_al ;
	  }
      if(data_bus_free_time > data_bus_reqd_time){		/* if data bus isn't going to be free in time, return false */
#ifdef DEBUG
        if (get_tran_watch(this_c->tran_id)) {
          fprintf(stdout,"[%lu]RAS -> posted CAS(%lu) Data Bus Conflict: Bus not free \n",dram_system.current_dram_time, this_c->tran_id); 
        }
#endif
        return false;
      }
    }else if (fbd_can_issue_ras_with_posted_cas(tran_index,this_c->next_c) == false ) {
#ifdef DEBUG
        if (get_tran_watch(this_c->tran_id)) {
          fprintf(stdout,"[%lu]RAS-> posted CAS failed %llu\n",dram_system.current_dram_time, this_c->tran_id); 
        }
#endif
        return false;
    }
  }
  return true;
}

/*
 *	We can issue a precharge as long as the bank is active, no pending CAS is waiting to use it, AND t_ras has expired. 
 */

bool can_issue_prec(int tran_index, command_t *this_c){
  int j;
  command_t *temp_c = NULL;
  dram_controller_t *this_dc;
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  transaction_t * this_t = &(this_dc->transaction_queue.entry[tran_index]);
  tick_t cmd_bus_reqd_time, dram_reqd_time;
  get_reqd_times(this_c,&cmd_bus_reqd_time,&dram_reqd_time);
  int sched_policy = get_transaction_selection_policy(global_biu);
  
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  // Check command bus availibility 
  if ( check_cmd_bus_required_time(this_c,cmd_bus_reqd_time) == FALSE) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) 
	  fprintf(stdout,"[%llu]PREC(%llu) Cmd bus not available \n",dram_system.current_dram_time, 
		  this_c->tran_id); 
#endif
	return false;
  }
 // Check that the dram writes and cas are done on time
  if ((dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time > dram_reqd_time) ||
	  (dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_done_time > dram_reqd_time )){
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) {
	  fprintf(stdout,"[%llu]CAS(%llu) t_wr conflict  [%llu] || cas_done time conflict [%llu] \n",dram_system.current_dram_time, this_c->tran_id,
		  dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].twr_done_time,
		  dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].bank[this_c->bank_id].cas_done_time); 
	}
#endif
	return false;
  }

  // Check that for open page you dont try to close the page that is yours ..
  if (dram_system.config.row_buffer_management_policy == OPEN_PAGE &&
	  ((this_dc->rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time >= this_dc->rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time &&
	   this_dc->rank[this_c->rank_id].bank[this_c->bank_id].row_id == this_c->row_id) ||
	  (this_dc->rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time <= this_dc->rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time))) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) {
	  fprintf(stdout,"PREC [%llu] Bank does not need precharge(%llu) ras done [%llu] rp done[%llu] row_id[%x] my row id[%x]\n",
		  dram_system.current_dram_time,
		  this_c->tran_id,
		  this_dc->rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time,
		  this_dc->rank[this_c->rank_id].bank[this_c->bank_id].rp_done_time,
		  this_dc->rank[this_c->rank_id].bank[this_c->bank_id].row_id,
		  this_c->row_id);

	}
#endif
	return false;
  }
  /* Check that your dat has been issued for a write command */
  if (dram_system.config.dram_type == FBD_DDR2 && this_t->transaction_type == MEMORY_WRITE_COMMAND && !this_t->issued_data ) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id))
	  printf("[%llu]PREC(%llu) Data still not started to be issued ",dram_system.current_dram_time, this_c->tran_id);
#endif
	return false;
  }
  
  /** Check if all previous Refresh commands i.e. RAS_ALL /PREC_ALL have been
   * issued **/
  if (dram_system.config.auto_refresh_enabled == TRUE) {
	if (check_for_refresh(this_c,tran_index,cmd_bus_reqd_time) == FALSE) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) 
	  fprintf(stdout,"[%llu]PREC(%llu) Conflict with Refresh \n",dram_system.current_dram_time, 
		  this_c->tran_id); 
#endif
	  return false;
	}
  }
  /*** Now check for conflicts with your own ras and cas commands */
  if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE) {
	temp_c = this_dc->transaction_queue.entry[tran_index].next_c;
	while (temp_c != NULL) {
	  if (temp_c->command != PRECHARGE && 
		  temp_c->command != DRIVE ) { /** Ensure that you dont check with yourself and only with dram commands**/
		if (temp_c->status == IN_QUEUE){
		  return false;
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) {
	  fprintf(stdout,"[%llu]PREC(%llu) Command still in queue ",dram_system.current_dram_time, 
		  this_c->tran_id); 
	print_command(dram_system.current_dram_time,temp_c);
	}
#endif
		}
	  }
	  temp_c = temp_c->next_c;
	}
  }

  /** Takes care of RAS conflicts i.e. ensures tRAS is done **/
  if (this_dc->rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time > dram_reqd_time) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) { 
	  fprintf(stdout,"[%llu]PREC (%llu) RAS Done conflict {%lld]\n",dram_system.current_dram_time, 
		  this_c->tran_id,
		  this_dc->rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time); 
	}
#endif
	return false;
  }

// Brinda: FIXME : Need to make sure that in fcfs you dont trample on
// transactions which are going to the same bank
  for(j=0;j<((sched_policy == FCFS) ? tran_index : this_dc->transaction_queue.transaction_count);j++){						/* checked all preceeding transactions for not yet completed CAS command */
	temp_c = this_dc->transaction_queue.entry[j].next_c;

	if ((temp_c->rank_id == this_c->rank_id) && j != tran_index && temp_c->bank_id == this_c->bank_id ){
      // If you are to the same row and this is an earlier transaction dont do
      // anything?
      if (temp_c->row_id == this_c->row_id && j < tran_index && !this_dc->transaction_queue.entry[j].issued_cas) {
#ifdef DEBUG
			if (get_tran_watch(this_c->tran_id)) 
			  fprintf(stdout,"[%llu]PREC (%llu) Conflict with earlier transaction to same row [%llu]  \n",dram_system.current_dram_time, 
				  this_c->tran_id, temp_c->tran_id); 
#endif
			return false;
		  }

	  if (dram_system.config.row_buffer_management_policy == OPEN_PAGE && j!= tran_index ) {
		if (temp_c->row_id == this_dc->rank[this_c->rank_id].bank[this_c->bank_id].row_id) {
		  // The bank is open to the correct row
		  // If the transaction is a read or has data issued then let it go
		  if (this_dc->transaction_queue.entry[j].status != MEM_STATE_SCHEDULED &&
			  !this_dc->transaction_queue.entry[j].issued_cas &&
			  this_dc->transaction_queue.entry[j].status != MEM_STATE_COMPLETED){
#ifdef DEBUG
			if (get_tran_watch(this_c->tran_id)) 
			  fprintf(stdout,"[%llu]PREC (%llu) Bank open to row reqd by [%llu] %d \n",dram_system.current_dram_time, 
				  this_c->tran_id, temp_c->tran_id,this_dc->transaction_queue.entry[j].status); 
#endif
			return false;
		  }

		} else { // bank is open to Different row =>  
		  if (this_dc->transaction_queue.entry[j].status == MEM_STATE_ISSUED ){
#ifdef DEBUG
			if (get_tran_watch(this_c->tran_id)) 
			  fprintf(stdout,"[%llu]PREC (%llu) Same row[%llu] waiting for it to get done\n",dram_system.current_dram_time, 
				  this_c->tran_id, temp_c->tran_id); 
#endif
			return false;
		  }	
		}
		if (sched_policy == FCFS ) { // you should never go out of order
#ifdef DEBUG
		  if (get_tran_watch(this_c->tran_id)) 
			fprintf(stdout,"[%llu]PREC (%llu) Bank open to row reqd by [%llu] \n",dram_system.current_dram_time, 
				this_c->tran_id, temp_c->tran_id); 
#endif
		  return false;
		}

	  }else { // Close page

		if (this_dc->transaction_queue.entry[j].status == MEM_STATE_ISSUED && j!= tran_index){
		  while(temp_c != NULL){
			if (temp_c->command != DATA && temp_c->status == IN_QUEUE) {
#ifdef DEBUG
			  if (get_tran_watch(this_c->tran_id)) { 
				printf("[%llu]PREC(%llu) Trans status (%d) Conflict with command ",dram_system.current_dram_time, 
					this_c->tran_id,
					this_dc->transaction_queue.entry[j].status); 
				print_command(dram_system.current_dram_time,temp_c);
			  }
#endif
			  return false;
			}
			temp_c = temp_c->next_c;
		  }
		}
	  }
      }
	}

  return true;

}

bool can_issue_cas_with_precharge(int tran_index, command_t *this_c){
  int retval = FALSE; 
  if ( (retval = can_issue_cas(tran_index,this_c)) == TRUE) {
	  int j;
	  command_t *temp_c = NULL;
	  
	  dram_controller_t *this_dc = &dram_system.dram_controller[this_c->chan_id];
	  int sched_policy = get_transaction_selection_policy(global_biu);

	  tick_t dram_reqd_time = dram_system.current_dram_time +  dram_system.config.t_cas + dram_system.config.t_burst;
    if (dram_system.config.dram_type == FBD_DDR2) {
      dram_reqd_time +=
        (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id ): dram_system.config.t_bus*(dram_system.config.rank_count - 1)) +
          dram_system.config.t_bundle +
          dram_system.config.t_amb_up ;
    }
      /*** Your RAS should be issued and all your data should have been sent*/
	  if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE) {
		if (this_dc->transaction_queue.entry[tran_index].status != MEM_STATE_ISSUED) {
#ifdef DEBUG
		  if (get_tran_watch(this_c->tran_id)) {
			fprintf(stdout,"[%llu]PREC(%llu) Transaction not started ",dram_system.current_dram_time, 
				this_c->tran_id); 
		  }
#endif
		  return false;
		}
	  }

	  /** Takes care of RAS conflicts i.e. ensures tRAS is done **/
	  if (this_dc->rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time > dram_reqd_time) {
#ifdef DEBUG
		if (get_tran_watch(this_c->tran_id)) { 
		  fprintf(stdout,"[%llu]PREC (%llu) RAS Done conflict [%lld] > [%llu]\n",dram_system.current_dram_time, 
			  this_c->tran_id,
			  this_dc->rank[this_c->rank_id].bank[this_c->bank_id].ras_done_time,dram_reqd_time); 
		}
#endif
		return false;
	  }

	  // Brinda: FIXME : Need to make sure that in fcfs you dont trample on
	  // transactions which are going to the same bank
	  for(j=0;j<((sched_policy == FCFS) ? tran_index : this_dc->transaction_queue.transaction_count);j++){						/* checked all preceeding transactions for not yet completed CAS command */
		temp_c = this_dc->transaction_queue.entry[j].next_c;
        if ((temp_c->rank_id == this_c->rank_id) && j != tran_index && temp_c->bank_id == this_c->bank_id ){
          if ((temp_c->rank_id == this_c->rank_id) && j != tran_index && temp_c->bank_id == this_c->bank_id ){
            // If you are to the same row and this is an earlier transaction dont do
            // anything?
            if (temp_c->row_id == this_c->row_id && j < tran_index && !this_dc->transaction_queue.entry[j].issued_cas) {
#ifdef DEBUG
              if (get_tran_watch(this_c->tran_id)) 
                fprintf(stdout,"[%llu]CAS_WITH_PREC (%llu) Conflict with earlier transaction to same row [%llu]  \n",dram_system.current_dram_time, 
                    this_c->tran_id, temp_c->tran_id); 
#endif
              return false;
            }
          }
          if (sched_policy == FCFS && !this_dc->transaction_queue.entry[j].issued_cas) { // you should never go out of order
#ifdef DEBUG
            if (get_tran_watch(this_c->tran_id)) 
              fprintf(stdout,"[%llu]CAS_WITH_PREC (%llu) Bank open to row reqd by [%llu] \n",dram_system.current_dram_time, 
                  this_c->tran_id, temp_c->tran_id); 
#endif
            return false;
          }
          if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE) {
            if (this_dc->transaction_queue.entry[j].status == MEM_STATE_ISSUED){
              while(temp_c != NULL){
                if (temp_c->command != DATA && temp_c->status == IN_QUEUE) {
#ifdef DEBUG
                  if (get_tran_watch(this_c->tran_id)) { 
                    printf("[%llu]CAS_WITH_PREC(%llu) Trans status (%d) Conflict with command ",dram_system.current_dram_time, 
                        this_c->tran_id,
                        this_dc->transaction_queue.entry[j].status); 
                    print_command(dram_system.current_dram_time,temp_c);
                  }
#endif
                  return false;
                }
                temp_c = temp_c->next_c;
              }
            }
          }
        }
      }
	  return true;
	}else { return false; }
}

/***
 * FBDIMM
 * Data commands can be issued as long as there are 
 * 1. No other commands in flight i.e. up_bus is free - Checked in
 * is_up_bus_free already
 * 2. If its the first data command a up_buffer is free.
 * 3. No previous Datas to the same rank/bank are waiting in queue.
 * ***/
bool can_issue_data(int tran_index, command_t *this_c) {
  int retval = TRUE;
  if (dram_system.config.auto_refresh_enabled == TRUE) {
	if (check_for_refresh(this_c,tran_index,dram_system.current_dram_time) == FALSE) {
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) {
		fprintf(stdout,"[%llu]DATA (%llu) refresh conflict  ",dram_system.current_dram_time, this_c->tran_id); 
	  }
#endif
	  return false;
	}
  }
  retval = is_up_buffer_free(this_c->chan_id,this_c->rank_id,this_c->bank_id,dram_system.dram_controller[this_c->chan_id].transaction_queue.entry[tran_index].transaction_id);
   return retval;
}
/***
 * FBDIMM
 * 1. Check that the up_bus is free to send the command. (Already done )
 * 2. Check that the down_bus is free when data is returning i.e. no other
 * drive conflicts.
 * 3. Check that the data is already availible to you i.e. your cas read has
 * completed.
 * **/
bool can_issue_drive(int tran_index, command_t *this_c) {
  int i;
  dram_controller_t *this_dc;
  tick_t down_bus_reqd_time = 0;
  tick_t data_reqd_time = 0;
  command_t *temp_c;

  this_dc = &(dram_system.dram_controller[this_c->chan_id]);

  down_bus_reqd_time = dram_system.current_dram_time + 
	dram_system.config.t_bundle + 
	(dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id ): dram_system.config.t_bus*(dram_system.config.rank_count - 1))+ // bus latency is needed to calculate when bus is needed
	dram_system.config.t_amb_up +
	dram_system.config.t_amb_down;
  data_reqd_time = dram_system.current_dram_time + 
	(dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id ): dram_system.config.t_bus*( dram_system.config.rank_count - 1)) + // bus latency is needed to calculate when bus is needed
	dram_system.config.t_bundle + 
	dram_system.config.t_amb_up;



  /*** Check for down bus being free **/  // for cas with drive, it's automatically checked in check_current_with_cas_with_drive func
  /***
   * say k = rank of command being tested and r = another drive command on the
   * same channel.
   * 
   * if k is greater than r, command k may start data xmission at t_bundle*dw - t_bus*(k-r)
   * if r is greater than k, command k may start data xmission at t_bundle*dw + t_bus*(r-k)
   *
   * both simplify to t_bundle*dw + t_bus*(r-k)
   *
   * -kb
   ***/ 

  for(i=0;i<tran_index;i++){
	temp_c = this_dc->transaction_queue.entry[i].next_c;
	while(temp_c != NULL){
	  if (temp_c->command == DRIVE || temp_c->command == CAS_WITH_DRIVE || this_c->command == CAS_AND_PRECHARGE) {
		if (check_down_bus_required_time(this_c,temp_c,down_bus_reqd_time) == FALSE)
		  return false;
	  }
	  temp_c = temp_c->next_c;
	}
  }


  /*** Check that your cas has data ready for you when you reach 
   * i.e. when the cas completes the drive would have reached **/
  temp_c = this_dc->transaction_queue.entry[tran_index].next_c;
  while (temp_c != NULL) {
	if (temp_c->command == CAS) {
	  if (temp_c->status == IN_QUEUE || data_reqd_time < temp_c->dimm_data_tran_comp_time) {
		return false;
	  }
	}
	temp_c = temp_c->next_c;

  }

  if (dram_system.config.auto_refresh_enabled == TRUE) {
	if (check_for_refresh(this_c,tran_index,dram_system.current_dram_time) == FALSE) {
	  return false;
	}
  }


  return true;  
}


/* 
 * FB-DIMM
 * This code checks if a CAS or a CAS_WRITE can be issued. It is similar to the can_issue_cas used in other cases. 
 * It checks the following : 
 * 1) if the bank is open, or  will be open in time, and 
 * 2) there are no other CAS operations in flight currently. 
 *
 * fbd_can_issue_cas(int tran_index,command_t *this_c)
 * tran_index ->  my transaction id
 * this_c ->  the command in question
 *
 * One should check that the following resources are availible
 * 1. Up Bus to DIMM - Checked by is_up_bus_idle .. not by state of prev
 * commands
 * 2. Command Bus to DRAM on DIMM
 * 3. Data Bus on DIMM
 * 4. Buffers on DIMM
 * 5. DRAM Rows in Bank 
 * 6. Down Bus 
 * 
 */
bool fbd_can_issue_cas(int tran_index, command_t *this_c){

  int i;
  int bank_is_open;
  int bank_willbe_open;
  int rank_id, bank_id, row_id;

  command_t *temp_c;
  dram_controller_t *this_dc;
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);

  tick_t	down_bus_reqd_time = 0; /* Get rid of compiler warnings */
  tick_t	data_bus_reqd_time = 0;
  tick_t	cmd_bus_reqd_time = 0;

  rank_id = this_c->rank_id;
  bank_id = this_c->bank_id;
  row_id  = this_c->row_id;

  bank_is_open = FALSE;
  bank_willbe_open = FALSE;



  /***** Check for conflicts on Data bus and Command Bus *****/
  if(this_c->command == CAS  ){ 
	data_bus_reqd_time = dram_system.current_dram_time + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id ): dram_system.config.t_bus*(dram_system.config.rank_count - 1) )+
	  dram_system.config.t_bundle +  
	  dram_system.config.t_amb_up + 
	  dram_system.config.t_cas +
	  (dram_system.config.posted_cas_flag? dram_system.config.t_al : 0);
	cmd_bus_reqd_time = dram_system.current_dram_time + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus*(dram_system.config.rank_count - 1) )+
	  dram_system.config.t_bundle +  
	  dram_system.config.t_amb_up;
  } else if(this_c->command == CAS_WITH_DRIVE  || this_c->command == CAS_AND_PRECHARGE){   // support for cas with drive -aj
	data_bus_reqd_time = dram_system.current_dram_time + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus*(dram_system.config.rank_count - 1)) +
	  dram_system.config.t_bundle +  
	  dram_system.config.t_amb_up + 
	  dram_system.config.t_cas+
	  (dram_system.config.posted_cas_flag? dram_system.config.t_al : 0);
	cmd_bus_reqd_time = dram_system.current_dram_time + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus*(dram_system.config.rank_count - 1) )+
	  dram_system.config.t_bundle +  
	  dram_system.config.t_amb_up; 

	// since data is being sent with the drive, require the down bus as well
    down_bus_reqd_time = data_bus_reqd_time + 
           (dram_system.config.t_burst * DATA_BYTES_PER_READ_BUNDLE/dram_system.config.cacheline_size) +
                 dram_system.config.t_amb_down; 

//	down_bus_reqd_time = data_bus_reqd_time  + 
//	  dram_system.config.t_burst +
//	  dram_system.config.t_amb_down; 

  } else if(this_c->command == CAS_WRITE  || this_c->command == CAS_WRITE_AND_PRECHARGE){
	if (dram_system.config.t_cwd >=0) {	
	  data_bus_reqd_time = dram_system.current_dram_time + 
		dram_system.config.t_bundle +  
		dram_system.config.t_amb_up + 
		dram_system.config.col_command_duration + 
		(dram_system.config.posted_cas_flag? dram_system.config.t_al : 0 )+ 
		dram_system.config.t_cwd;
	}else {
	  data_bus_reqd_time = dram_system.current_dram_time + 
		dram_system.config.t_bundle +  
		dram_system.config.t_amb_up ;
	}
	cmd_bus_reqd_time = dram_system.current_dram_time + 
	  dram_system.config.t_bundle +  
	  dram_system.config.t_amb_up;
  }

  /* Check if the on-dimm bus is available first */
  if (check_cmd_bus_required_time (this_c, cmd_bus_reqd_time) == FALSE) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) {
	  fprintf(stdout,"[%llu]CAS(%llu) Command bus not available %llu\n",dram_system.current_dram_time,this_c->tran_id,
		  dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time); 
	}
#endif
	return false;
  }

  /*** Check for Refresh first ***/
  if (dram_system.config.auto_refresh_enabled == TRUE) {
	if (check_for_refresh(this_c,tran_index,cmd_bus_reqd_time) == FALSE){
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) {
		fprintf(stdout,"[%llu]CAS(%llu) Conflict with REFRESH  \n",dram_system.current_dram_time,this_c->tran_id); 
	  }
#endif
	  return false;
	}
  }
	// Check for bank availability next
 if((this_dc->rank[rank_id].bank[bank_id].ras_done_time > this_dc->rank[rank_id].bank[bank_id].rp_done_time) &&      /* bank will be open in time */
	  (this_dc->rank[rank_id].bank[bank_id].row_id == row_id)){
	if(this_dc->rank[rank_id].bank[bank_id].rcd_done_time <= 
		(cmd_bus_reqd_time + 
		 dram_system.config.col_command_duration)){
	  bank_willbe_open = TRUE;
	} else if((dram_system.config.posted_cas_flag == TRUE) && 
		(this_dc->rank[rank_id].bank[bank_id].rcd_done_time <= (cmd_bus_reqd_time + 
																dram_system.config.t_al + 
																dram_system.config.col_command_duration))){
	  bank_willbe_open = TRUE;
	}
  }
  if(bank_willbe_open == TRUE){
  } else {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) {
	  fprintf(stdout,"[%llu] t_rcd not satisfied [%llu] t_ras done [%llu] t_rp done [%llu] row_id [%x] my row id [%x]\n",
		  dram_system.current_dram_time,
		  this_dc->rank[rank_id].bank[bank_id].rcd_done_time,
		  this_dc->rank[rank_id].bank[bank_id].ras_done_time,
		  this_dc->rank[rank_id].bank[bank_id].rp_done_time,
		  this_dc->rank[rank_id].bank[bank_id].row_id,
		  row_id);
	}
#endif
	return false;
  }
  // Check for earlier transaction conflict
  for(i=0;i<tran_index;i++){
	temp_c = this_dc->transaction_queue.entry[i].next_c;
	if ( this_c->rank_id == temp_c-> rank_id && this_c->bank_id == temp_c->bank_id && this_c->row_id == temp_c->row_id &&
         !this_dc->transaction_queue.entry[i].issued_cas) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) {
	  fprintf(stdout,"[%llu] Conflict with previous Transaction %d to same row",
		  dram_system.current_dram_time,temp_c->tran_id  );
	}
#endif
    }
  }

  /** Check for individual commands timing chekcs **/
  if (this_c->command == CAS_AND_PRECHARGE || this_c->command == CAS_WITH_DRIVE) {
	if (fbd_cas_with_drive_timing_chk (tran_index, this_c,cmd_bus_reqd_time,data_bus_reqd_time,down_bus_reqd_time) == FALSE) {
	  return false;
	}
  }else if (this_c->command == CAS_WRITE || this_c->command == CAS_WRITE_AND_PRECHARGE) {
	if (fbd_cas_write_timing_chk (tran_index, this_c,cmd_bus_reqd_time,data_bus_reqd_time) == FALSE) {
	  return false;
	}
  }else if (this_c->command == CAS) {
	if (fbd_cas_read_timing_chk (tran_index, this_c,cmd_bus_reqd_time, data_bus_reqd_time) == FALSE) {
	  return false;
	}
  }

  /*** Check that your RAS has been issued and t_rcd is satisfied***/
  temp_c = this_dc->transaction_queue.entry[tran_index].next_c;
  if (dram_system.config.row_buffer_management_policy == CLOSE_PAGE) { 
	while (temp_c != NULL) {
	  if (temp_c->command == RAS) {
		if (temp_c->status == IN_QUEUE) {
#ifdef DEBUG
		  if (get_tran_watch(this_c->tran_id)) {
			fprintf(stdout,"[%llu] CAS - RAS !issued (status == IN_QUEUE (%d)) || rcd_done_time(%llu)\n",
				dram_system.current_dram_time,
				temp_c->status == IN_QUEUE,
				temp_c->dram_proc_comp_time);
		  }
#endif
		  return false;
		}
	  }
	  temp_c = temp_c->next_c;
	}
  }
  /** Check if the buffer is availible for CAS **/  
  // add support of CAS with drive -aj
  if ((this_c->command == CAS || this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_AND_PRECHARGE) && (is_down_buffer_free(this_c->chan_id,rank_id,bank_id,this_c->tran_id) == FALSE)) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) {
	  fprintf(stdout,"[%llu]CAS(%llu) Down Buffer is not free  \n",dram_system.current_dram_time,this_c->tran_id); 
	}
#endif
	return false;
  }


  /*** Now check if we have the appropriate row open and ready ***/

  if (dram_system.config.row_buffer_management_policy == PERFECT_PAGE) {	/* This policy doesn't care about issue rules as long as data bus is ready */
	return true;				
  }
  if (dram_system.config.strict_ordering_flag == TRUE){			/* if strict_ordering is set, suppress if earlier CAS exists in queue */
	for(i=0;i<tran_index;i++){
	  temp_c = this_dc->transaction_queue.entry[i].next_c;
	  while(temp_c != NULL){
		if(((temp_c->command == CAS) ||  (temp_c->command == CAS_WRITE) ||  (temp_c->command == CAS_WITH_DRIVE) || (temp_c->command == CAS_AND_PRECHARGE) || (temp_c->command == CAS_WRITE_AND_PRECHARGE)) &&  (temp_c->status == IN_QUEUE)){
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) {
		fprintf(stdout,"[%llu]CAS(%llu) Strict Ordering overhead \n",dram_system.current_dram_time,this_c->tran_id); 
	  }
#endif
		  return false;
		}
		temp_c = temp_c->next_c;
	  }	
	}
  }

	return true; 
}	 

bool fbd_cas_read_timing_chk(int tran_index, command_t *this_c,tick_t cmd_bus_reqd_time, tick_t data_bus_reqd_time){

  int rank_id, bank_id, row_id;

  dram_controller_t *this_dc = &(dram_system.dram_controller[this_c->chan_id]);

  this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  rank_id = this_c->rank_id;
  bank_id = this_c->bank_id;
  row_id  = this_c->row_id;
  // On chip resources 
  if (check_data_bus_required_time (this_c, data_bus_reqd_time) == FALSE) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) {
	  fprintf(stdout,"[%llu]CAS(%llu) Data bus not available \n",dram_system.current_dram_time,this_c->tran_id); 
	}
#endif
	return false;
  }

  return true;
}

bool fbd_cas_with_drive_timing_chk(int tran_index, command_t *this_c, tick_t cmd_bus_reqd_time, tick_t data_bus_reqd_time, tick_t down_bus_reqd_time){

  int i;
  int rank_id, bank_id, row_id;

  command_t *temp_c;
  dram_controller_t *this_dc;
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  transaction_t * this_t = &(this_dc->transaction_queue.entry[tran_index]);

  rank_id = this_c->rank_id;
  bank_id = this_c->bank_id;
  row_id  = this_c->row_id;

  // First check if the data bus is available
 
  if (check_data_bus_required_time(this_c,data_bus_reqd_time) == FALSE) {
#ifdef DEBUG
			if (get_tran_watch(this_c->tran_id)) {
			  fprintf(stdout,"[%llu]CAS(%llu) Dimm Data Bus Busy \n",dram_system.current_dram_time,this_c->tran_id); 
			}
#endif
	return false;
  }
  if (dram_system.config.var_latency_flag) {

	for(i=0;i< this_dc->transaction_queue.transaction_count;i++){
	  temp_c = this_dc->transaction_queue.entry[i].next_c;
	  if ( this_dc->transaction_queue.entry[i].transaction_type != MEMORY_WRITE_COMMAND  &&
		  (this_dc->transaction_queue.entry[i].status != MEM_STATE_SCHEDULED || this_dc->transaction_queue.entry[i].status != MEM_STATE_DATA_ISSUED)) {
		while(temp_c != NULL){
		  /*** Check for current CAS in the system i.e. conflict with past read**/
		  if ((temp_c->command == CAS_AND_PRECHARGE || temp_c->command == CAS_WITH_DRIVE || temp_c->command == DRIVE) && temp_c->status != IN_QUEUE ) {

			if (check_down_bus_required_time(this_c,temp_c,down_bus_reqd_time) == FALSE){
#ifdef DEBUG
			  if (get_tran_watch(this_c->tran_id)) {
				fprintf(stdout,"[%llu]CAS(%llu) Link Down Bus Busy trans(%llu)status(%d) till %llu my status(%d)\n",
					dram_system.current_dram_time,
					this_c->tran_id,
					temp_c->tran_id,
					this_dc->transaction_queue.entry[i].status, 
					temp_c->link_data_tran_comp_time,
					this_t->status); 
			  }
#endif
			  return false;
			}
		  }

		  temp_c = temp_c->next_c;
		}
	  }

	}
  }else {
	if (down_bus_reqd_time < this_dc->down_bus.completion_time) {
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) {
		fprintf(stdout,"[%llu]CAS(%llu) Link Down Bus Busy till %llu \n",
			dram_system.current_dram_time,
			this_c->tran_id,
			this_dc->down_bus.completion_time
			);
	  }
#endif
	  return false;
	} 
  }
  return true;
}


/* 
 * FB-DIMM

 * tran_index ->  my transaction id
 * this_c ->  the command in question
 *
 * 
 */
bool fbd_cas_write_timing_chk(int tran_index, command_t *this_c,tick_t cmd_bus_reqd_time, tick_t data_bus_reqd_time){

  int rank_id, bank_id, row_id;
  command_t *temp_c;
  dram_controller_t *this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  rank_id = this_c->rank_id;
  bank_id = this_c->bank_id;
  row_id  = this_c->row_id;

  if (check_data_bus_required_time (this_c, data_bus_reqd_time) == FALSE) {
	return false;
  }
  /*** Check that data for the transaction has reached already
   * caswrite reaches the dimm ***/
  temp_c = this_dc->transaction_queue.entry[tran_index].next_c;
  while(temp_c != NULL){
	if (temp_c->command == DATA && temp_c->status == IN_QUEUE) {
	  return false;
	}
	temp_c = temp_c->next_c;
  }

  return true;
}



/*
 * Note : The ras can already be issued in this case. We are just ensuring that
 * the cas can be issued as well. 
 * 1.Check for data bus availibility on DRAM
 * 2.Check for down bus availibility
 * Note Command bus is availible as RAS can be transmitted.
 */
bool fbd_can_issue_ras_with_posted_cas (int tran_index, command_t *this_c){
  dram_controller_t *this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  int rank_id, bank_id;
  tick_t data_bus_reqd_time = 0;
  tick_t down_bus_reqd_time = 0 ;
  int i;
  command_t *temp_c;
  int casispresent = FALSE;
  int casreadispresent = FALSE;
  int caswithdrive = FALSE;
  this_dc = &(dram_system.dram_controller[this_c->chan_id]);
  rank_id = this_c->rank_id;
  bank_id = this_c->bank_id;

  if (dram_system.config.posted_cas_flag == FALSE) /** Do not call function if you cannot do posted cas **/
	return false;

  if (this_c->command == CAS || this_c->command == CAS_WRITE || this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_AND_PRECHARGE || this_c->command == CAS_WRITE_AND_PRECHARGE) {
    casispresent = TRUE;
    if (this_c->command == CAS) {
      casreadispresent = TRUE;
    }
    if( this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_AND_PRECHARGE) {
      casreadispresent = TRUE;
      caswithdrive = TRUE;
    }
  }
  if (casispresent == FALSE)
	return false;

  if(this_c->command == CAS  ){ 
	data_bus_reqd_time = dram_system.current_dram_time + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus*(dram_system.config.rank_count - 1) )+
	  dram_system.config.t_bundle +  
	  dram_system.config.t_amb_up + 
	  dram_system.config.row_command_duration + // Time for the RAS to go
	  dram_system.config.t_cas +
	  (dram_system.config.posted_cas_flag? dram_system.config.t_al : 0);
  } else if(this_c->command == CAS_WITH_DRIVE  || this_c->command == CAS_AND_PRECHARGE){   // support for cas with drive -aj
	data_bus_reqd_time = dram_system.current_dram_time + 
	  (dram_system.config.var_latency_flag ?dram_system.config.t_bus*(this_c->rank_id): dram_system.config.t_bus*(dram_system.config.rank_count - 1)) +
	  dram_system.config.t_bundle +  
	  dram_system.config.t_amb_up + 
	  dram_system.config.row_command_duration + // Time for the RAS to go
	  dram_system.config.t_cas+
	  (dram_system.config.posted_cas_flag? dram_system.config.t_al : 0);

	// since data is being sent with the drive, require the down bus as well
    down_bus_reqd_time = data_bus_reqd_time +
           (dram_system.config.t_burst * DATA_BYTES_PER_READ_BUNDLE/dram_system.config.cacheline_size) +
                 dram_system.config.t_amb_down;
                 

  } else if(this_c->command == CAS_WRITE  || this_c->command == CAS_WRITE_AND_PRECHARGE){
	if (dram_system.config.t_cwd >=0) {	
	  data_bus_reqd_time = dram_system.current_dram_time + 
		dram_system.config.t_bundle +  
		dram_system.config.t_amb_up + 
	    dram_system.config.row_command_duration + // Time for the RAS to go
		dram_system.config.col_command_duration + 
		(dram_system.config.posted_cas_flag? dram_system.config.t_al : 0 )+ 
		dram_system.config.t_cwd;
	}else {
	  data_bus_reqd_time = dram_system.current_dram_time + 
		dram_system.config.t_bundle +  
		dram_system.config.t_amb_up ;
	}
  }

  if (check_data_bus_required_time(this_c,data_bus_reqd_time) == FALSE) {
#ifdef DEBUG
	if (get_tran_watch(this_c->tran_id)) 
	  fprintf(stdout,"[%llu]RAS_Posted_CAS Rank Data bus not free \n",dram_system.current_dram_time); 
#endif
	return false;
  }

  /*** We have to traverse the entire queue unless we set up a data bus
   * for each rank and track it 
   * **/
  if (caswithdrive) {
	if (!dram_system.config.var_latency_flag){
	  if (down_bus_reqd_time < this_dc->down_bus.completion_time) {
#ifdef DEBUG
		if (get_tran_watch(this_c->tran_id)) {
		  fprintf(stdout,"[%llu]CAS(%llu) Link Down Bus Busy till %llu \n",
			  dram_system.current_dram_time,
			  this_c->tran_id,
			  this_dc->down_bus.completion_time
			  );
		}
#endif
		return false;
	  } 


	}else {

	  for (i=0;i<this_dc->transaction_queue.transaction_count; i ++) {
		temp_c = this_dc->transaction_queue.entry[i].next_c;
		if (this_dc->transaction_queue.entry[i].transaction_type != MEMORY_WRITE_COMMAND) {
		  while(temp_c != NULL){
			if ((temp_c->command == DRIVE || temp_c->command == CAS_WITH_DRIVE || temp_c->command == CAS_AND_PRECHARGE) &&
				(temp_c->status != IN_QUEUE) &&
				check_down_bus_required_time(this_c,temp_c,down_bus_reqd_time) == FALSE) {
#ifdef DEBUG
			  if (get_tran_watch(this_c->tran_id)) 
				printf("[%llu]RAS_Posted_CAS Down link not free @ %llu busy till %llu \n",dram_system.current_dram_time,
					down_bus_reqd_time,
					temp_c->link_data_tran_comp_time); 
#endif
			  return false;
			}
			temp_c = temp_c->next_c;
		  }
		}
	  }
	}
  }
  if( casreadispresent == TRUE ) {
	if (is_down_buffer_free(this_c->chan_id,rank_id,bank_id,this_c->tran_id ) == FALSE) {
#ifdef DEBUG
	  if (get_tran_watch(this_c->tran_id)) {
		fprintf(stdout,"[%llu]RAS_Posted_CAS Down buffer is occupied \n",dram_system.current_dram_time); 
		print_buffer_contents(this_c->chan_id,rank_id);
	  }
#endif
	  return false;
	}
  } else { /** CAS write **/
	temp_c = this_dc->transaction_queue.entry[tran_index].next_c;
	while(temp_c != NULL) {
	  /** All data packets should be availible by data bus requirement time
	   * **/
	  if (temp_c->command == DATA) {
		if (temp_c->status == IN_QUEUE) {
#ifdef DEBUG
			if (get_tran_watch(this_c->tran_id)) 
			   fprintf(stdout,"[%llu] Data packets are not issued \n",dram_system.current_dram_time);
#endif
			return false;
		}else {
		  if (temp_c->amb_proc_comp_time > data_bus_reqd_time) {
#ifdef DEBUG
			if (get_tran_watch(this_c->tran_id)) 
			   fprintf(stdout,"[%llu] Data packets arrive by [%llu] too late for data transmission [%llu]\n",dram_system.current_dram_time,
				   temp_c->amb_proc_comp_time,
				   data_bus_reqd_time);
#endif
			return false;
		  }	
		}	  
	  }
	  temp_c = temp_c->next_c;
	}
  }
#ifdef DEBUG
  if (get_tran_watch(this_c->tran_id)) 
	fprintf(stdout,"[%llu] Can issue RAS with posted CAS\n",dram_system.current_dram_time);
#endif
  return true;
}
/***
 * 1. If issued -> Refresh has to wait for the transaction to complete -> note that this could be 
 * incorrect behavior -> but am not sure of how the memory system actually
 * handles this case unless refresh issue policy is opportunistic
 * 2. If not issued -> Check if refresh is going to be done in time to issue
 * i.e.  refresh has completed.
 * 3. Check if there is a command bus conflict - laready taken care of earlier
 */ 
bool check_for_refresh(command_t * this_c,int tran_index,tick_t cmd_bus_reqd_time) {
  command_t *temp_c = NULL, *rq_temp_c = NULL;
  dram_controller_t *this_dc = &dram_system.dram_controller[this_c->chan_id];
  rq_temp_c = this_dc->refresh_queue.rq_head;
  if (this_c->refresh == FALSE){  
	  if (this_dc->transaction_queue.entry[tran_index].status != MEM_STATE_ISSUED) {
    if ((dram_system.config.refresh_issue_policy == REFRESH_OPPORTUNISTIC && is_refresh_queue_half_full(this_c->chan_id)) || (dram_system.config.refresh_issue_policy == REFRESH_HIGHEST_PRIORITY)) {
      while (rq_temp_c != NULL) {
        temp_c = rq_temp_c;
        /** Only check for unissued Refresh stuff **/
        while (temp_c != NULL) {
          if (temp_c->status == IN_QUEUE) { 
            if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK ||
                (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK && temp_c->bank_id == this_c->bank_id ) ||
                (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK && temp_c->rank_id == this_c->rank_id ) ||
                (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK && temp_c->rank_id == this_c->rank_id && temp_c->bank_id == this_c->bank_id )) {
#ifdef DEBUG
              if (get_tran_watch(this_c->tran_id)) {
                fprintf(stdout, "----------------------\n");
                fprintf(stdout,"[%llu]CMD (%d) (%llu) Refresh Conflict tstatus(%d) tindex(%d) ",dram_system.current_dram_time, 
                    this_c->command, this_c->tran_id,
                    this_dc->transaction_queue.entry[tran_index].status,
                    tran_index); 
                print_command(dram_system.current_dram_time,temp_c);
                fprintf(stdout,"----------------------\n");
              }
#endif
              return false;
            }
          }
          temp_c = temp_c->next_c;
        }
        rq_temp_c = rq_temp_c->rq_next_c;
      }
    }
  }
  }
  return true;
}

bool check_data_bus_required_time (command_t * this_c, tick_t data_bus_reqd_time) {

  if ( data_bus_reqd_time < dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].my_dimm->data_bus_completion_time)
	return false;

  return true;
}
/**
 * Function that checks if the command to be issued conflicts with a previously
 * issued command. Returns FALSE if there is a conflict else returns TRUE.
 * 
 * command_t this_c - Command which is to be issued.
 * command_t temp_c - Command which has been already been issued
 * tick_t cmd_bus_reqd_time  Time at which command bus is required
 * */

bool check_cmd_bus_required_time (command_t * this_c, tick_t cmd_bus_reqd_time) {
  /* We just need to worry abotu transmission time */
  if (dram_system.config.dram_type == FBD_DDR2) {
	if (cmd_bus_reqd_time < dram_system.dram_controller[this_c->chan_id].rank[this_c->rank_id].my_dimm->fbdimm_cmd_bus_completion_time)
	  return false;
  }else {
	return (col_command_bus_idle(this_c->chan_id));
  }
return true;
}

bool check_down_bus_required_time (command_t * this_c, command_t *temp_c,tick_t down_bus_reqd_time) {
  if (dram_system.config.var_latency_flag == TRUE) {
	if (this_c->rank_id > temp_c->rank_id) { 
	  if (down_bus_reqd_time < (temp_c->link_data_tran_comp_time - (this_c->rank_id - temp_c->rank_id) * dram_system.config.t_bus))
		return false;
	}else {
	  if (down_bus_reqd_time < (temp_c->link_data_tran_comp_time - (temp_c->rank_id - this_c->rank_id) * dram_system.config.t_bus))
		return false;
	}
  }else {

	if (down_bus_reqd_time < (temp_c->link_data_tran_comp_time - 
		(dram_system.config.rank_count)*dram_system.config.t_bus))
	  return false;
  }
  return true;
}



