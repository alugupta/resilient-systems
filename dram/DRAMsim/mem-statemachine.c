/*
 * mem-statemachine.c - Routines which update the state of each command and any
 * associated system state.
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
extern dram_system_t dram_system;
extern biu_t *global_biu;


void fbd_cas_write_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string);
void fbd_cas_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string);
void fbd_ras_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string);
void fbd_ras_all_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string);
void fbd_precharge_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string);
void fbd_precharge_all_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string);
void fbd_cas_with_drive_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string);
void fbd_refresh_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) ;
void fbd_refresh_all_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) ;
/* This function checks if the buffer has to be released */

void check_buffer_release(tick_t now,command_t *this_c) {
	if (this_c->command == CAS || this_c->command == CAS_AND_PRECHARGE || this_c->command == CAS_WITH_DRIVE) {
		if (this_c->dimm_data_tran_comp_time <= now) {
		  release_down_buffer(this_c->chan_id,this_c->rank_id,this_c->bank_id,this_c->tran_id);
		}
	}else if (this_c->command == CAS_WRITE_AND_PRECHARGE|| this_c->command == CAS_WRITE ) {
		if (this_c->link_data_tran_comp_time <= now){
		  release_up_buffer(this_c->chan_id,this_c->rank_id,this_c->bank_id,this_c->tran_id);
		}
	}
}
/************************************************************
 * update_command_states 
 * Updates the state of the inflight commands of a transaction.
 *
 * tran_index - Transaction index ( != transaction id)
 * debug_string 
 * **********************************************************/
int update_command_states(tick_t now, int chan_id,int tran_index, char *debug_string)
{
  int done = TRUE;
  int rank_id, bank_id, row_id, col_id;
  command_t *this_c;
  int update = FALSE;
  int all_scheduled = TRUE;
  transaction_t * this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index];
  assert (tran_index < MAX_TRANSACTION_QUEUE_DEPTH);

  this_c = this_t->next_c;
  while(this_c != NULL){
	assert(strlen(debug_string) < MAX_DEBUG_STRING_SIZE); /** Added to take care of a need to allow larger debug strings **/
	if (this_c->status != IN_QUEUE && this_c->status != MEM_STATE_COMPLETED ) { // Only check for those commands which have already been issued and not done
	  update = TRUE;
	  if (cas_wave_debug() || wave_debug()) {
		rank_id = this_c->rank_id;
		bank_id = this_c->bank_id;
		row_id  = this_c->row_id;
		col_id  = this_c->col_id;
		switch (this_c->command) {
		  case CAS:
			cas_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			break;
		  case CAS_AND_PRECHARGE:
		  case CAS_WITH_DRIVE:
			if( dram_system.config.dram_type == FBD_DDR2) {
			  fbd_cas_with_drive_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			}else 
			  cas_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			break;
		  case CAS_WRITE_AND_PRECHARGE:
		  case CAS_WRITE:
			cas_write_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			break;
		  case RAS:
			ras_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			break;
		  case PRECHARGE:
			precharge_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			break;
		  case PRECHARGE_ALL:
			precharge_all_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			break;
		  case RAS_ALL:
			ras_all_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			break;
		  case DRIVE:
			drive_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			break;
		  case DATA:
			data_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
			break;
		  default:
			fprintf(stderr, "I am confused, unknown command type: %d", this_c->command);
			break;
		}
        //if (this_c->status == MEM_STATE_COMPLETED)
         // fprintf(stderr,"[%llu] done CMD %d tid %llu completion time %llu status %d \n", now,this_c->command,this_c->tran_id,
           //   this_c->completion_time,
           //   this_c->status);
	  } else {
        if (this_c->completion_time <= now) {
          this_c->status = MEM_STATE_COMPLETED;
         // fprintf(stderr,"[%llu] done CMD %d tid %llu completion time %llu status %d \n", now,this_c->command,this_c->tran_id,
           //   this_c->completion_time,
           //   this_c->status);
        }
		 // Check about releasing the buffers here
		 if (dram_system.config.dram_type == FBD_DDR2)
		 check_buffer_release(now,this_c);
	  }
	} else if (this_c->status == IN_QUEUE) {
	  all_scheduled = FALSE;
	}
	if(this_c->status != MEM_STATE_COMPLETED){
	  done = FALSE;
	}
	this_c = this_c->next_c;
	
  }
  if (this_t->critical_word_ready_time <= now && this_t->issued_cas) {
	this_t->critical_word_ready = true;
  }
  return done;
}

void cas_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {

  if (dram_system.config.dram_type == FBD_DDR2) {
	fbd_cas_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
  }
  else {
	if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time<= dram_system.current_dram_time)){
	  dram_system.dram_controller[chan_id].command_bus.status = IDLE;
	  dram_system.dram_controller[chan_id].col_command_bus.status = IDLE;
	  this_c->status = DRAM_PROCESSING;
	  if((dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command == CAS_WRITE)
		  || (dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command == CAS)
		  || (dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command == CAS_WITH_DRIVE)){
		mem_gather_stat(GATHER_BANK_HIT_STAT, 
			(int) (this_c->dram_proc_comp_time - dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_done_time)); 
	  }
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_done_time = this_c->dram_proc_comp_time;
#ifdef DEBUG
	  if (wave_debug() || cas_wave_debug()){ build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC); }
#endif
	} else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	  this_c->status = LINK_DATA_TRANSMISSION;
	  dram_system.dram_controller[chan_id].data_bus.status = BUS_BUSY;
#ifdef DEBUG
	  if (wave_debug() || cas_wave_debug()) build_wave(&debug_string[0], tran_index, this_c, PROC2DATA);
#endif
	} else if((this_c->status == LINK_DATA_TRANSMISSION) && (this_c->link_data_tran_comp_time <= dram_system.current_dram_time)){
	  //dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras++; 
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = CAS;
	  dram_system.dram_controller[chan_id].data_bus.status = IDLE;
	  if (this_c->command == CAS_AND_PRECHARGE) {
	  	this_c->status = DIMM_PRECHARGING;
#ifdef DEBUG
	  if (wave_debug() || cas_wave_debug()) build_wave(&debug_string[0], tran_index, this_c, LINKDATA2PREC);
#endif
	  }else {
	  this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	  if (wave_debug() || cas_wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	  }
	} else if((this_c->status == DIMM_PRECHARGING) && (this_c->rank_done_time <= dram_system.current_dram_time)){
	  this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	  if (wave_debug() || cas_wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	}
  }
}

void cas_write_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {

  if (dram_system.config.dram_type == FBD_DDR2) {
	fbd_cas_write_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);
  } 

  else {
	if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	  dram_system.dram_controller[chan_id].command_bus.status = IDLE;
	  dram_system.dram_controller[chan_id].col_command_bus.status = IDLE;
	  
	  if(dram_system.config.t_cwd == 0) {						/* DDR SDRAM type */
		dram_system.dram_controller[chan_id].data_bus.status = BUS_BUSY;
//		dram_system.dram_controller[chan_id].data_bus.current_rank_id = MEM_STATE_INVALID;  /* memory controller was "owner of DQS */
		this_c->status = LINK_DATA_TRANSMISSION; 	
		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_done_time =  dram_system.current_dram_time + dram_system.config.t_burst;
	  } else if (dram_system.config.t_cwd > 0){ /* we should enter into a processing state -RDRAM/DDR II/FCRAM/RLDRAM etc */
		this_c->status = DRAM_PROCESSING; 	
	  } else {							/* SDRAM, already transmitting data */
		this_c->status = LINK_DATA_TRANSMISSION; 	
//		dram_system.dram_controller[chan_id].data_bus.current_rank_id = MEM_STATE_INVALID;  /* memory controller was "owner of DQS */
	  }
	  if((dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command == CAS_WRITE)
		  || (dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command == CAS)
		  || (dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command == CAS_WITH_DRIVE)){
		mem_gather_stat(GATHER_BANK_HIT_STAT, 
			(int) (this_c->dram_proc_comp_time - dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_done_time)); 
	  }
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_done_time = this_c->dram_proc_comp_time;
#ifdef DEBUG
	  if (wave_debug() || cas_wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
	} else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){  /* rdram or DDR II */
	  dram_system.dram_controller[chan_id].data_bus.status = BUS_BUSY;
	  this_c->status = LINK_DATA_TRANSMISSION; 	
	  /** FIXME **/
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_done_time = dram_system.current_dram_time + dram_system.config.t_burst;
//	  dram_system.dram_controller[chan_id].data_bus.current_rank_id = MEM_STATE_INVALID;  /* memory controller was "owner of DQS */
	  if((dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command == CAS_WRITE)
		  || (dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command == CAS)){ 
		mem_gather_stat(GATHER_BANK_HIT_STAT,
			(int) (this_c->completion_time - dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_done_time));
	  }

#ifdef DEBUG
	  if (wave_debug() || cas_wave_debug()) build_wave(&debug_string[0], tran_index, this_c, PROC2DATA);
#endif
	} else if((this_c->status == LINK_DATA_TRANSMISSION) && (this_c->link_data_tran_comp_time <= dram_system.current_dram_time)){
	  //dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras++; 
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = CAS_WRITE;
	  dram_system.dram_controller[chan_id].data_bus.status = IDLE;
	  if (this_c->command == CAS_WRITE_AND_PRECHARGE) {
		this_c->status = DIMM_PRECHARGING;
#ifdef DEBUG
		if (wave_debug() || cas_wave_debug()) build_wave(&debug_string[0], tran_index, this_c, LINKDATA2PREC);
#endif
	  }else {
		this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
		if (wave_debug() || cas_wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	  }
	} else if((this_c->status == DIMM_PRECHARGING) && (this_c->rank_done_time <= dram_system.current_dram_time)){
	  this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	  if (wave_debug() || cas_wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	}
  }

}

void ras_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {

  if (dram_system.config.dram_type == FBD_DDR2) {
	fbd_ras_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
  }else {

	if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	  this_c->status = DRAM_PROCESSING;
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = ACTIVATING;
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].row_id = this_c->row_id;

	  if(dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command != IDLE){
		/* DW: Look at this again.
		 * last command set to IDLE if init or REFRESH, then all other RAS due to bank conflict. 
		 * Gather stat for bank conflict.
		 */
		mem_gather_stat(GATHER_BANK_CONFLICT_STAT, 
			(int) (this_c->completion_time - dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].ras_done_time));
	  }

	  dram_system.dram_controller[chan_id].command_bus.status = IDLE;
	  dram_system.dram_controller[chan_id].row_command_bus.status = IDLE;
#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
	} else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = ACTIVE;
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = RAS;
	  this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	}
  }
}

void precharge_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {

  /*** For the FB-DIMM PRECHARGE transistions are 
   *                       
   * LINK_COMMAND_TRANSMISSION ----> AMB_PROCESSING ----> AMB_COMMAND_TRANs---> DRAM_PROCESSING -----> MEM_STATE_COMPLETED
   * t(bundle) + t(bus)			t(ambup)  				t(row_command_duration)		t(rp)
   * ***/

  if (dram_system.config.dram_type == FBD_DDR2) {
	fbd_precharge_transition(this_c, tran_index, chan_id, rank_id, bank_id, row_id, debug_string);

  }
  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = PRECHARGING;
//	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rp_done_time = dram_system.current_dram_time + 
//	  dram_system.config.t_rp;
	this_c->status = DRAM_PROCESSING;
	dram_system.dram_controller[chan_id].command_bus.status = IDLE;
	dram_system.dram_controller[chan_id].row_command_bus.status = IDLE;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
  } else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = IDLE;
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = PRECHARGE;
	/*
	   mem_gather_stat(GATHER_CAS_PER_RAS_STAT, 
	   dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras);
	   dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras = 0;
	   */
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
}

void precharge_all_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {
  int i,j;
  if (dram_system.config.dram_type == FBD_DDR2) {
	fbd_precharge_all_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
  }else {
	if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time<= dram_system.current_dram_time)){
	  for(i = 0; i< dram_system.config.rank_count;i++){
		for(j = 0; j< dram_system.config.bank_count;j++){
		  dram_system.dram_controller[chan_id].rank[i].bank[j].status = PRECHARGING;
//		  dram_system.dram_controller[chan_id].rank[i].bank[j].rp_done_time = 
//			dram_system.current_dram_time + 
//			dram_system.config.t_rp;
		}
	  }
	  this_c->status = DRAM_PROCESSING;
	  dram_system.dram_controller[chan_id].command_bus.status = IDLE;
	  dram_system.dram_controller[chan_id].row_command_bus.status = IDLE;
#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
	} else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	  for(i = 0; i< dram_system.config.rank_count;i++){
		for(j = 0; j< dram_system.config.bank_count;j++){
		  dram_system.dram_controller[chan_id].rank[i].bank[j].status = IDLE;
		  dram_system.dram_controller[chan_id].rank[i].bank[j].last_command = PRECHARGE;
		  /*
			 mem_gather_stat(GATHER_CAS_PER_RAS_STAT, 
			 dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras);
			 */
		  dram_system.dram_controller[chan_id].rank[i].bank[j].cas_count_since_ras = 0;
		}
	  }
	  this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	}
  }
}

void ras_all_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {
  int i, j;
  if (dram_system.config.dram_type == FBD_DDR2) {
	fbd_ras_all_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
  }else {
	if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	  for(i = 0; i < dram_system.config.rank_count;i++){
		for(j= 0; j< dram_system.config.bank_count;j++){
		  dram_system.dram_controller[chan_id].rank[i].bank[j].status = ACTIVATING;
//		  dram_system.dram_controller[chan_id].rank[i].bank[j].ras_done_time = 
//			dram_system.current_dram_time + 
//			dram_system.config.t_ras;
		}
	  }
	  this_c->status = DRAM_PROCESSING;
	  dram_system.dram_controller[chan_id].command_bus.status = IDLE;
	  dram_system.dram_controller[chan_id].row_command_bus.status = IDLE;
#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
	} else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	  for(i = 0; i < dram_system.config.rank_count;i++){
		for(j= 0; j< dram_system.config.bank_count;j++){
		  dram_system.dram_controller[chan_id].rank[i].bank[j].status = ACTIVE;
		  dram_system.dram_controller[chan_id].rank[i].bank[j].last_command = RAS;
		  //dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras = 0;
		}
	  }
	  this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	}
  }
}

void refresh_all_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {
  int i, j;
  if (dram_system.config.dram_type == FBD_DDR2) {
	fbd_refresh_all_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
  }else {
	if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
		for(j= 0; j< dram_system.config.bank_count;j++){
		  dram_system.dram_controller[chan_id].rank[rank_id].bank[j].row_id = this_c->row_id;
		  dram_system.dram_controller[chan_id].rank[rank_id].bank[j].status = ACTIVATING;
//		  dram_system.dram_controller[chan_id].rank[rank_id].bank[j].rp_done_time =
//			dram_system.current_dram_time + dram_system.config.t_rfc;
		}

	  }else {
		for(i = 0; i < dram_system.config.rank_count;i++){
		  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
			for(j= 0; j< dram_system.config.bank_count;j++){
			  dram_system.dram_controller[chan_id].rank[i].bank[j].row_id = this_c->row_id;
			  dram_system.dram_controller[chan_id].rank[i].bank[j].status = ACTIVATING;
//			  dram_system.dram_controller[chan_id].rank[i].bank[j].rp_done_time = 
//				dram_system.current_dram_time + dram_system.config.t_rfc;
//			  dram_system.dram_controller[chan_id].rank[i].bank[j].rfc_done_time = 
//				dram_system.current_dram_time + dram_system.config.t_rfc;
			}
		  }else {
			dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].row_id = this_c->row_id;
			dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].status = ACTIVATING;
//			dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].rp_done_time = 
//			  dram_system.current_dram_time + dram_system.config.t_rfc;
		  }
		}
	  }
	  this_c->status = DRAM_PROCESSING;
	  dram_system.dram_controller[chan_id].command_bus.status = IDLE;
	  dram_system.dram_controller[chan_id].row_command_bus.status = IDLE;
#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
	} else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
		for(j= 0; j< dram_system.config.bank_count;j++){
		  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].status = IDLE;
		  dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].last_command = REFRESH_ALL;
		}
	  }else {
		for(i = 0; i < dram_system.config.rank_count;i++){
		  if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
			for(j= 0; j< dram_system.config.bank_count;j++){
			  dram_system.dram_controller[chan_id].rank[i].bank[j].status = IDLE;
			  dram_system.dram_controller[chan_id].rank[i].bank[j].last_command = REFRESH_ALL;
			}
		  } else {
			dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].status = IDLE;
			dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].last_command = REFRESH_ALL;
		  }
		}


		//dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras = 0;
	  }
	  this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	  if (wave_debug()) build_wave(debug_string, tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	}
  }
}
/**
 * send command
 * wait for dram to process = t_ras+t_rp
 */
void refresh_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {

  if (dram_system.config.dram_type == FBD_DDR2) {
	fbd_refresh_transition(this_c,  tran_index, chan_id,  rank_id, bank_id,row_id,debug_string);
  }else {

	if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	  this_c->status = DRAM_PROCESSING;
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = ACTIVATING;
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].row_id = this_c->row_id;
//	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rp_done_time = 
//		dram_system.current_dram_time + dram_system.config.t_rc;
//	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rfc_done_time = 
//		dram_system.current_dram_time + dram_system.config.t_rfc;
	  dram_system.dram_controller[chan_id].command_bus.status = IDLE;
	  dram_system.dram_controller[chan_id].row_command_bus.status = IDLE;

#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2PROC);
#endif
	} else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = IDLE;
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = REFRESH;
	  this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	}
  }
}


/***************** FB-DIMM specific commands 
 * LINK_COMMAND_TRANSMISSION ---> AMB_PROCESSING  ---> DRAM_PROCESSING ------> LINK_DATA_TRANSMISSION ----> MEM_STATE_COMPLETED
 * t(bundle) + t(bus)			 t(amb_in)			 t(amb_out)	   	                 t(burst)                   t(burst)
 * Data is already available out of the DRAM -> current assumption is that the
 * amb still has to packetize it.
 * ***********************/
void drive_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {
  transaction_t   *this_t;
  this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index]);	
  if (dram_system.config.dram_type == FBD_DDR2) { /* Ensuring that the normal protocol does not come here */
	if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	  this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	  if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif
	  /*** Down bus is going to be locked after the processing -> should we
	   * be doing anything here about it ***/
	}else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	  this_c->status = AMB_DOWN_PROCESSING;
#ifdef DEBUG
	  if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2PROC);
#endif
	  /*** Down bus is going to be locked after the processing -> should we
	   * be doing anything here about it ***/
	}else if((this_c->status == AMB_DOWN_PROCESSING) && (this_c->amb_down_proc_comp_time <= dram_system.current_dram_time)){
	  /** t(Burst) = f( data size requested)
	   *  if 1 16 byte (default) chunk takes t_bundle 
	   *  then to send entire_column you will need N*t_bundle transmission
	   *  time where N is the number of words in a col. width
	   */
	  /** Down bus is also busy -> mark busy **/
//	  dram_system.dram_controller[chan_id].down_bus.completion_time = this_c->completion_time;
//	  dram_system.dram_controller[chan_id].down_bus.current_rank_id = rank_id; 
//	  dram_system.dram_controller[chan_id].down_bus.status			= BUS_BUSY; 
	  this_c->status = LINK_DATA_TRANSMISSION;
#ifdef DEBUG
	  if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, PROC2DATA);
#endif
	} else if((this_c->status == LINK_DATA_TRANSMISSION) && (this_c->link_data_tran_comp_time <= dram_system.current_dram_time)){
	  this_c->status = MEM_STATE_COMPLETED;
	  dram_system.dram_controller[chan_id].down_bus.status			= IDLE; 
	  /** Release the buffer **/
	  release_down_buffer(chan_id,rank_id,bank_id,this_t->transaction_id);
#ifdef DEBUG
	  if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	}
  }

}

/***********************
 * LINK_COMMAND_TRANSMISSION --->  AMB_PROCESSING  ----> AMB_CT -->		DRAM_PROCESSING -> DIMM_DATA_TRANSMISSION ----> MEM_STATE_COMPLETED 
 * 						   |							
 * t(bundle) + t(bus)	   	t(amb_in)+ row_command_duration t(col_cmd_trans)	t(cwd) + t(al)		t(BURST)
 * ***********************/
/* FIXME : Check for CAS done time */
void fbd_cas_write_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {

  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif
  }else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	if(dram_system.config.t_cwd == 0) {						/* DDR SDRAM type */
	  /*
	   * with t_cwd = 0 -> you first send column command and then
	   * t_burst back to back. 
	   * Hence completion time takse that into account. 
	   */
	  this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	  if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
	} else if (dram_system.config.t_cwd > 0){ /* we should enter into a processing state -RDRAM/DDR II/FCRAM/RLDRAM etc */
	  /**
	   * In this case we dont send data till we have readied everything 
	   * **/
	  this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	  if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
	} else {							/* SDRAM, already transmitting data */
	  /** FIXME : This is because we dont keep track of the data bus
	   * anymore :
	   * Once t_AMB is over -> you send the column command with the data */
	  this_c->status = DIMM_DATA_TRANSMISSION;
#ifdef DEBUG
	  if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2DATA);
#endif

	}

  }else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	if (dram_system.config.t_cwd == 0) { /** In this case we send the packet right away **/
	  this_c->status = DIMM_DATA_TRANSMISSION;
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_done_time =  this_c->dimm_data_tran_comp_time;
#ifdef DEBUG
	  if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBCT2DATA);
#endif
	} else if (dram_system.config.t_cwd > 0) {
	  this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	  if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBCT2PROC);
#endif
	}else { /** In this case we are already sending the data **/
	  dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_done_time =  this_c->dimm_data_tran_comp_time;
#ifdef DEBUG
	  if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBCT2DATA);
#endif
	}

  }else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_DATA_TRANSMISSION;
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, PROC2DATA);
#endif
  }else if((this_c->status == DIMM_DATA_TRANSMISSION) && (this_c->dimm_data_tran_comp_time <= dram_system.current_dram_time)){

	/** Release the buffer **/
	release_up_buffer(chan_id,rank_id,bank_id,this_c->tran_id);
	if (this_c->command == CAS_AND_PRECHARGE) {
	this_c->status = DIMM_PRECHARGING;
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, LINKDATA2PREC);
#endif
	}else {
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	}
  } else if((this_c->status == DIMM_PRECHARGING) && (this_c->rank_done_time <= dram_system.current_dram_time)){
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
}
/*** For the DATA CAS_WRITE transistions are 
 *                       
 * LINK_COMMAND_TRANSMISSION ---------------> AMB_PROCESSING -------------------> MEM_STATE_COMPLETED
 * t(bundle) + t(bus)					   t(ambup)
 * Note that the actual command is issued when the write data is received and
 * this is just a place holder for the command.
 * ***/

void data_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {
  transaction_t   *this_t;
  this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index]);	
  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif

  } else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }


}

/*** For the FB-DIMM CAS transistions are 
 *                       
 * LINK_COMMAND_TRANSMISSION ----> AMB DRAM_PROCESSING --> AMB_CT -- -> 	DRAM_PROCESSING -> DIMM_DATA_TRANSMISSION -------> MEM_STATE_COMPLETED
 * t(bundle) + t(bus)		   t(ambup )        t(col_command)      	t(cac)+Tal			t(data)
 * ***/
void fbd_cas_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string){
  transaction_t   *this_t;
  this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index]);	

  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif

  } else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBCT2PROC);
#endif
  } else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){

	this_c->status = DIMM_DATA_TRANSMISSION;
	/** FIXME make sure that this corresponds to time to transmit the data
															  from the DIMM to the amb buffer ***/
	
	/* FBD Power --put code to track RD% HERE  */
	dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle += (this_c->dimm_data_tran_comp_time - dram_system.current_dram_time);
	dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.current_CAS_cycle += (this_c->dimm_data_tran_comp_time - dram_system.current_dram_time);
		 /*fprintf(stderr,"rank[%d-%d] CAS_cycle[%llu] >>CAW_cycle[%llu] last_RAS[%llu]\n", chan_id,rank_id, 
		 dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle, 
		 dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle, 
		 dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time);*/
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, PROC2DATA);
#endif

  } else if((this_c->status == DIMM_DATA_TRANSMISSION) && (this_c->dimm_data_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = MEM_STATE_COMPLETED;
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras++; 
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = CAS;
	

#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
}



// Adding CAS with DRIVE transition -aj

/*** For the FB-DIMM CAS transistions are 
 *                       
 * LINK_COMMAND_TRANSMISSION ----> AMB_PROCESSING --> DIMM_COMMAND_TRANSMISSION  ----> DRAM_PROCESSING -> DIMM_DATA_TRANSMISSION -------> AMB_DOWN_PROCESSING -------> LINK_DATA_TRANSMISSION ------> MEM_STATE_COMPLETED
 * t(bundle) + t(bus)		   t(ambup )        t(col_command)      	t(cac)+Tal			t(data)
 * ***/
void fbd_cas_with_drive_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string){
  transaction_t   *this_t;
  this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index]);	

  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif

  } else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBCT2PROC);
#endif
  } else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){

	this_c->status = DIMM_DATA_TRANSMISSION;
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, PROC2DATA);
#endif
  } else if((this_c->status == DIMM_DATA_TRANSMISSION) && (this_c->dimm_data_tran_comp_time <= dram_system.current_dram_time)){   // FIXME: aj check this out will ya

	// Starts differing from cas without drive from here onwards
	this_c->status = AMB_DOWN_PROCESSING;
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras++; 
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = CAS;  // aj what is this used for?
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, DATA2AMBDOWN);
#endif
  } else if((this_c->status == AMB_DOWN_PROCESSING) && (this_c->amb_down_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = LINK_DATA_TRANSMISSION;
	dram_system.dram_controller[chan_id].down_bus.status			= BUS_BUSY; 
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBDOWN2DATA);
#endif
  } else if((this_c->status == LINK_DATA_TRANSMISSION) && (this_c->link_data_tran_comp_time <= dram_system.current_dram_time)){
	if (this_c->command == CAS_WRITE_AND_PRECHARGE && this_c->rank_done_time > this_c->link_data_tran_comp_time) {
		  this_c->status = DIMM_PRECHARGING;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, LINKDATA2PREC);
#endif
	}else{
		this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
	  /** Release the buffer **/
	  release_down_buffer(chan_id,rank_id,bank_id,this_t->transaction_id);

  } else if((this_c->status == DIMM_PRECHARGING) && (this_c->rank_done_time <= dram_system.current_dram_time)){
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
  
}
/*** For the FB-DIMM CAS_WITH_PRECHARGE transistions are 
 *                       
 * LINK_COMMAND_TRANSMISSION ----> AMB_PROCESSING --> DIMM_COMMAND_TRANSMISSION  ----> DRAM_PROCESSING -> DIMM_DATA_TRANSMISSION -------> AMB_DOWN_PROCESSING -------> LINK_DATA_TRANSMISSION -----> PRECHARGEREMAINS IF ANY _> MEM_STATE_COMPLETED
 * t(bundle) + t(bus)		   t(ambup )        t(col_command)      	t(cac)+Tal			t(data)
 * ***/
void fbd_cas_with_prec_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string){
  transaction_t   *this_t;
  this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[tran_index]);	

  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif
  } else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBCT2PROC);
#endif
  } else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){

	this_c->status = DIMM_DATA_TRANSMISSION;
	/* FBD Power --put code to track RD% HERE  */
	dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle += (this_c->dimm_data_tran_comp_time - dram_system.current_dram_time);
	dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo.current_CAS_cycle += (this_c->dimm_data_tran_comp_time - dram_system.current_dram_time);
	/*fprintf(stderr,"rank[%d-%d] CAS_cycle[%llu] >>CAW_cycle[%llu] last_RAS[%llu]\n", chan_id,rank_id, 
	  dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAS_cycle, 
	  dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.current_CAW_cycle, 
	  dram_system.dram_controller[chan_id].rank[rank_id].r_p_info.last_RAS_time);*/
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, PROC2DATA);
#endif
  } else if((this_c->status == DIMM_DATA_TRANSMISSION) && (this_c->dimm_data_tran_comp_time <= dram_system.current_dram_time)){   // FIXME: aj check this out will ya

	// Starts differing from cas without drive from here onwards
	this_c->status = AMB_DOWN_PROCESSING;
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras++; 
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = CAS;  // aj what is this used for?
#ifdef DEBUG
	if (cas_wave_debug() || wave_debug()) build_wave(&debug_string[0], tran_index, this_c, DATA2AMBDOWN);
#endif
  } else if((this_c->status == AMB_DOWN_PROCESSING) && (this_c->amb_down_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = LINK_DATA_TRANSMISSION;
	dram_system.dram_controller[chan_id].down_bus.status			= BUS_BUSY; 
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBDOWN2DATA);
#endif
  } else if((this_c->status == LINK_DATA_TRANSMISSION) && (this_c->link_data_tran_comp_time <= dram_system.current_dram_time)){

	this_c->status = MEM_STATE_COMPLETED;

	/** Down bus is also busy -> mark busy **/
	//	dram_system.dram_controller[chan_id].down_bus.status			= IDLE; 
	//	dram_system.dram_controller[chan_id].down_bus.completion_time = this_c->completion_time;
	//	dram_system.dram_controller[chan_id].down_bus.current_rank_id = rank_id; 
#ifdef DEBUG
	if (cas_wave_debug() ||wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
	  /** Release the buffer **/
	  release_down_buffer(chan_id,rank_id,bank_id,this_t->transaction_id);

  }
}


void fbd_precharge_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string){
  /*** For the FB-DIMM PRECHARGE transistions are 
   *                       
   * LINK_COMMAND_TRANSMISSION ----> AMB_PROCESSING ----> AMB_COMMAND_TRANs---> DRAM_PROCESSING -----> MEM_STATE_COMPLETED
   * t(bundle) + t(bus)			t(ambup)  				t(row_command_duration)		t(rp)
   * ***/

  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = AMB_PROCESSING;
	/** The command will activate it inevitably we might want tomove this
	 * to processing **/
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = PRECHARGING;
/*	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rp_done_time = dram_system.current_dram_time + 
	  dram_system.config.t_amb_up +
	  dram_system.config.row_command_duration +
	  dram_system.config.t_rp;
	  */
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif
  } else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBCT2PROC);
#endif
  } else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = IDLE;
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = PRECHARGE;
	mem_gather_stat(GATHER_CAS_PER_RAS_STAT, 
		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras);
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras = 0;
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
}

void fbd_precharge_all_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {

  int i,j;
  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
	  for(j = 0; j< dram_system.config.bank_count;j++){
		dram_system.dram_controller[chan_id].rank[rank_id].bank[j].status = PRECHARGING;
	  }
	}else {
	  for(i = 0; i< dram_system.config.rank_count;i++){
		if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
		  dram_system.dram_controller[chan_id].rank[i].bank[bank_id].status = PRECHARGING;
		}else {
		  for(j = 0; j< dram_system.config.bank_count;j++){
			dram_system.dram_controller[chan_id].rank[i].bank[j].status = PRECHARGING;
		  }
		}
	  }
	}
	this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif
  } else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2PROC);
#endif
  } else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK) {
	  for(j = 0; j< dram_system.config.bank_count;j++){
		dram_system.dram_controller[chan_id].rank[rank_id].bank[j].status = IDLE;
		mem_gather_stat(GATHER_CAS_PER_RAS_STAT, 
			dram_system.dram_controller[chan_id].rank[rank_id].bank[j].cas_count_since_ras);
		dram_system.dram_controller[chan_id].rank[rank_id].bank[j].cas_count_since_ras = 0;
		dram_system.dram_controller[chan_id].rank[rank_id].bank[j].last_command = PRECHARGE;
	  }
	}else {
	  for(i = 0; i< dram_system.config.rank_count;i++){
		if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
		  dram_system.dram_controller[chan_id].rank[i].bank[bank_id].status = IDLE;
		  dram_system.dram_controller[chan_id].rank[i].bank[bank_id].last_command = PRECHARGE;
		  mem_gather_stat(GATHER_CAS_PER_RAS_STAT, 
			  dram_system.dram_controller[chan_id].rank[i].bank[bank_id].cas_count_since_ras);
		  dram_system.dram_controller[chan_id].rank[i].bank[bank_id].cas_count_since_ras = 0;
		}else {
		  for(j = 0; j< dram_system.config.bank_count;j++){
			dram_system.dram_controller[chan_id].rank[i].bank[j].status = IDLE;
			dram_system.dram_controller[chan_id].rank[i].bank[j].last_command = PRECHARGE;
			mem_gather_stat(GATHER_CAS_PER_RAS_STAT, 
				dram_system.dram_controller[chan_id].rank[i].bank[j].cas_count_since_ras);
			dram_system.dram_controller[chan_id].rank[i].bank[j].cas_count_since_ras = 0;
		  }
		}
	  }
	}

	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if ( wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
}

void fbd_ras_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {
  /*** For the FB-DIMM RAS transistions are 
   *                       
   * LINK_COMMAND_TRANSMISSION ------> AMB_PROCESSING ---> AMB_CT -------> DRAM_PROCESSING ---------> MEM_STATE_COMPLETED
   * t(bundle) + t(bus)				t(ambup)  		   row_cmd_dur		t(rcd)
   * Note: tRAS is kept track of globally as in the earlier case
   * ***/
  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = AMB_PROCESSING;
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = ACTIVATING;
	//dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].row_id = this_c->row_id;
	//dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].ras_done_time = this_c->dram_proc_comp_time;
	//dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rcd_done_time = 
	  //dram_system.current_dram_time + 
	  //dram_system.config.t_amb_up +
	  //dram_system.config.row_command_duration +
	  //dram_system.config.t_rcd;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif

  }else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBCT2PROC);
#endif
  }else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = ACTIVE;
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = RAS;
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
}


void fbd_ras_all_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {
  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	for(rank_id = 0; rank_id< dram_system.config.rank_count;rank_id++){
	  for(bank_id = 0; bank_id< dram_system.config.bank_count;bank_id++){
		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = ACTIVATING;
//		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].ras_done_time = this_c->dram_proc_comp_time;
	  }
	}
	this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif
  } else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	for(rank_id = 0; rank_id< dram_system.config.rank_count;rank_id++){
	  for(bank_id = 0; bank_id< dram_system.config.bank_count;bank_id++){
		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = ACTIVE;
		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = RAS;
		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras = 0;
	  }
	}
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
}

void fbd_refresh_all_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {
  int i,j;
  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK ||
		dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK) {
	  for(i = 0; i< dram_system.config.rank_count;i++){
		if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK ){
		  for(j = 0; j< dram_system.config.bank_count;j++){
			dram_system.dram_controller[chan_id].rank[i].bank[j].status = ACTIVATING;
//			dram_system.dram_controller[chan_id].rank[i].bank[j].ras_done_time = this_c->dimm_comm_tran_comp_time + 
//			  dram_system.config.t_ras;
//			dram_system.dram_controller[chan_id].rank[i].bank[j].rp_done_time = this_c->dimm_comm_tran_comp_time +
//			  dram_system.config.t_rfc;
//			dram_system.dram_controller[chan_id].rank[i].bank[j].rfc_done_time = this_c->dimm_comm_tran_comp_time +
//			  dram_system.config.t_rfc;
		  }
		}else {
		  dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].status = ACTIVATING;
/*		  dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].ras_done_time = this_c->dimm_comm_tran_comp_time +
			dram_system.config.t_ras;
		  dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].rp_done_time = this_c->dimm_comm_tran_comp_time +
			dram_system.config.t_rfc;
		  dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].rfc_done_time = this_c->dimm_comm_tran_comp_time +
			dram_system.config.t_rfc;
			*/

		}
	  }
	}else if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {
	  for (j=0;j<dram_system.config.bank_count;j++) {
		dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].status = ACTIVATING;
/*		dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].ras_done_time = this_c->dimm_comm_tran_comp_time + 
		  							dram_system.config.t_ras;
		dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].rp_done_time = this_c->dimm_comm_tran_comp_time +
		  dram_system.config.t_rfc;
		dram_system.dram_controller[chan_id].rank[this_c->rank_id].bank[j].rfc_done_time = this_c->dimm_comm_tran_comp_time +
		  dram_system.config.t_rfc;
		  */
	  }
	 }
	this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif
  } else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){
	for(i= 0; i< dram_system.config.rank_count;i++){
     if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK) {

	  for(j= 0; j< dram_system.config.bank_count;j++){
		dram_system.dram_controller[chan_id].rank[i].bank[j].status = IDLE;
		dram_system.dram_controller[chan_id].rank[i].bank[j].last_command = REFRESH_ALL;
		dram_system.dram_controller[chan_id].rank[i].bank[j].cas_count_since_ras = 0;
	  }
	 }else {
	   	dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].status = IDLE;
		dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].last_command = REFRESH_ALL;
		dram_system.dram_controller[chan_id].rank[i].bank[this_c->bank_id].cas_count_since_ras = 0;
	 }
	}
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
}

void fbd_refresh_transition(command_t *this_c, int tran_index, int chan_id, int rank_id, int bank_id, int row_id, char *debug_string) {
  if((this_c->status == LINK_COMMAND_TRANSMISSION) && (this_c->link_comm_tran_comp_time <= dram_system.current_dram_time)){
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = ACTIVATING;
/*	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].ras_done_time = this_c->dimm_comm_tran_comp_time + 
	  dram_system.config.t_ras;
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rp_done_time = this_c->dimm_comm_tran_comp_time +
	  dram_system.config.t_rc;
	dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].rfc_done_time = this_c->dimm_comm_tran_comp_time +
	  dram_system.config.t_rc;*/
	this_c->status = AMB_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, XMIT2AMBPROC);
#endif
  } else if((this_c->status == AMB_PROCESSING) && (this_c->amb_proc_comp_time <= dram_system.current_dram_time)){
	this_c->status = DIMM_COMMAND_TRANSMISSION;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DIMM_COMMAND_TRANSMISSION) && (this_c->dimm_comm_tran_comp_time <= dram_system.current_dram_time)){
	this_c->status = DRAM_PROCESSING;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, AMBPROC2AMBCT);
#endif
  } else if((this_c->status == DRAM_PROCESSING) && (this_c->dram_proc_comp_time <= dram_system.current_dram_time)){

		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].status = IDLE;
		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].last_command = REFRESH;
		dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id].cas_count_since_ras = 0;
	this_c->status = MEM_STATE_COMPLETED;
#ifdef DEBUG
	if (wave_debug()) build_wave(&debug_string[0], tran_index, this_c, MEM_STATE_COMPLETED);
#endif
  }
}

