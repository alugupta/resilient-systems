/*
 * mem-dram.c - DRAM configuration routines/ access to dram_system/ Core
 * Controller Code. 
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
#include <math.h>
/* 
 * The DRAM system is a globally observable object.
 */

dram_system_t 		dram_system;

/* 
 * Function Calls for the Memory System.
 */

/*
 *  Initializes all the basic configurations.  
 */

void init_dram_system_configuration(){
  int chan_id, rank_id;
  int i;
  dram_system_configuration_t *this_c;
  global_biu = get_biu_address();
  this_c = &(dram_system.config);
  set_dram_type(SDRAM);					/* explicitly initialize the variables */
  set_dram_frequency(100);				/* explicitly initialize the variables */
  set_memory_frequency(100);				/* explicitly initialize the variables */
  set_dram_channel_count(1);    				/* single channel of memory */
  set_dram_channel_width(8);  				/* 8 byte wide data bus */
  set_pa_mapping_policy(SDRAM_BASE_MAP);    /* generic mapping policy */
  set_dram_transaction_granularity(64); 			/* 64 bytes long cachelines */
  set_dram_row_buffer_management_policy(OPEN_PAGE);

  this_c->strict_ordering_flag		= FALSE;
   this_c->packet_count          = MAX(1,(this_c->cacheline_size / (this_c->channel_width * 8 )));

  this_c->t_burst		 	= 	 this_c->cacheline_size / (this_c->channel_width);
  this_c->auto_refresh_enabled		= TRUE;
  this_c->refresh_policy			= REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK;
  this_c->refresh_issue_policy			= REFRESH_OPPORTUNISTIC;
  this_c->refresh_time			= 100;			
  this_c->refresh_cycle_count	= 10000;
  this_c->dram_debug			= FALSE;
  this_c->wave_debug			= FALSE;
  this_c->wave_cas_debug		= FALSE;
  this_c->issue_debug		= FALSE;
  this_c->memory2dram_freq_ratio = 1;

  this_c->arrival_threshold = 1500;
  /** Just setup the default types for FBD stuff too **/
  this_c->data_cmd_count 			= MAX(1,this_c->cacheline_size/DATA_BYTES_PER_WRITE_BUNDLE);
  this_c->drive_cmd_count 		= 1;
  this_c->num_thread_sets = 1;
  this_c->thread_sets[0] = 32;
  this_c->max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;
	this_c->single_rank = true;
	dram_system.tq_info.transaction_id_ctr = 0;
	dram_system.tq_info.num_ifetch_tran = 0;
	dram_system.tq_info.num_read_tran = 0;
	dram_system.tq_info.num_prefetch_tran = 0;
	dram_system.tq_info.num_write_tran = 0;
  for (i=0;i < MAX_CHANNEL_COUNT;i++)
	dram_system.dram_controller[i].transaction_queue.transaction_count = 0;
   
  //Ohm__Adding for power calculation                                        

 initialize_power_config(&dram_system.config.dram_power_config);

  //clear all the variables                                                   
  for(chan_id = 0; chan_id < MAX_CHANNEL_COUNT; chan_id++){
	for(rank_id = 0; rank_id < MAX_RANK_COUNT; rank_id++) {
	  memset((void *)&dram_system.dram_controller[chan_id].rank[rank_id].r_p_info,
		  0,sizeof(power_counter_t));
	  memset((void *)&dram_system.dram_controller[chan_id].rank[rank_id].r_p_gblinfo,
		  0,sizeof(power_counter_t));
	  memset((void *)dram_system.dram_controller[chan_id].rank[rank_id].bank,
		  0,sizeof(bank_t)*MAX_BANK_COUNT);
	  dram_system.dram_controller[chan_id].rank[rank_id].cke_bit = false;
	  if (dram_system.config.single_rank) {
		memset((void*)&dram_system.dram_controller[chan_id].dimm[rank_id],0,sizeof(dimm_t));
		dram_system.dram_controller[chan_id].rank[rank_id].my_dimm = &dram_system.dram_controller[chan_id].dimm[rank_id];
	  }else {
		if ((rank_id & 0x1)) { // Odd dimm
		  dram_system.dram_controller[chan_id].rank[rank_id].my_dimm = &dram_system.dram_controller[chan_id].dimm[rank_id-1];
		}else {
		  memset((void *)&dram_system.dram_controller[chan_id].dimm[rank_id],0,sizeof(dimm_t));
		  dram_system.dram_controller[chan_id].rank[rank_id].my_dimm = &dram_system.dram_controller[chan_id].dimm[rank_id];

		}
	  }
	}
	  memset((void *)&dram_system.dram_controller[chan_id].sched_cmd_info,
		  0,sizeof(rbrr_t));
      dram_system.dram_controller[chan_id].sched_cmd_info.current_cmd = RAS;
      dram_system.dram_controller[chan_id].sched_cmd_info.last_cmd = MEM_STATE_INVALID;
      dram_system.dram_controller[chan_id].sched_cmd_info.last_ref = false;
	  
    }
	/** Setup the parameters in the biu **/
    // Setup the global biu configuration pointer here
	biu_set_mem_cfg(global_biu,&dram_system.config);
}

/* 
 *  set configurations for specific DRAM Systems. 
 *
 *  We set the init here so we can use 16 256Mbit parts to get 512MB of total memory
 */

void set_dram_type(int dram_type){		/* SDRAM, DDR RDRAM etc */
  dram_system_configuration_t *this_c;

  this_c = &(dram_system.config);
  this_c->dram_type = dram_type;

  if (dram_type == SDRAM){				/* PC100 SDRAM -75 chips.  nominal timing assumed */
	this_c->channel_width  		=      8;	/* bytes */
	set_dram_frequency(100);
	this_c->row_command_duration 	=      1; 	/* cycles */
	this_c->col_command_duration 	=      1; 		/* cycles */
	this_c->rank_count  		=      4;  		/* for SDRAM and DDR SDRAM, rank_count is DIMM-bank count */
	/* Micron 256 Mbit SDRAM chip data sheet -75 specs used */
	this_c->bank_count   		=      4;		/* per rank */
	this_c->row_count		= 8*1024;
	this_c->col_count		=    512;	
	this_c->t_ras 	        	=      5; 	/* actually 44ns, so that's 5 cycles */
	this_c->t_rp	 		=      2; 	/* actually 20ns, so that's 2 cycles */ 
	this_c->t_rcd  	        	=      2; 	/* actually 20 ns, so that's only 2 cycles */  
	this_c->t_cas 		        =      2; 	/* this is CAS, use 2 */
	this_c->t_cac 		        = this_c->t_cas - this_c->col_command_duration;    /* delay in between CAS command and start of read data burst */
	this_c->t_cwd 		        =     -1; 	/* SDRAM has no write delay . -1 cycles between end of CASW command and data placement on data bus*/
	this_c->t_rtr  	        	=      0; 	/* SDRAM has no write delay, no need for retire packet */
	this_c->t_dqs      	=      0;
	this_c->t_rc			= this_c->t_ras + this_c->t_rp;
	this_c->t_rfc			= this_c->t_rc;
	this_c->t_wr  	        	=  2; 
	this_c->t_rtp  	        	=  1; 

	this_c->tq_delay =  2;	/* 2 DRAM ticks of delay  @ 100 MHz = 20ns */
  this_c->max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;

	this_c->critical_word_first_flag=   TRUE;
	this_c->dram_clock_granularity 	=      1;
  } else if (dram_type == DDRSDRAM){
	this_c->channel_width  		=      8;
	set_dram_frequency(200);
	this_c->row_command_duration	=  1 * 2;   	/* cycles */
	this_c->col_command_duration	=  1 * 2;	/* cycles */
	this_c->rank_count	  	=      4;   	/* for SDRAM and DDR SDRAM, rank_count is DIMM-bank count */
	this_c->bank_count   		=      4;		/* per rank */
	this_c->row_count		= 8*1024;
	this_c->col_count		=    512;	
	this_c->t_ras 	       		=  5 * 2; 
	this_c->t_rp 	       		=  2 * 2; 
	this_c->t_rcd         		=  2 * 2; 
	this_c->t_cas 	       		=  2 * 2; 
	this_c->t_cac 		        = this_c->t_cas - this_c->col_command_duration;    /* delay in between CAS command and start of read data burst */
	this_c->t_cwd 	       		=  0 * 2;  	/* not really write delay, but it's the DQS delay */
	this_c->t_rtr         		=      0; 
	this_c->t_dqs      	=      2;
	this_c->t_rc			= this_c->t_ras + this_c->t_rp;
	this_c->t_rfc			= this_c->t_rc; 
	this_c->t_wr  	        	=  2; /* FIXME */ 
	this_c->t_rtp  	        	=  2; 

	this_c->tq_delay =2*2;	/* 4 DRAM ticks of delay @ 200 MHz = 20ns */
  this_c->max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;

	this_c->critical_word_first_flag=   TRUE;
	this_c->dram_clock_granularity	=      2;	/* DDR clock granularity */
  } else if (dram_type == DDR2 || dram_type == DDR3 ){
	this_c->channel_width  		=      8;	/* bytes */
	set_dram_frequency(400);
	this_c->row_command_duration	=  1 * 2;   	/* cycles */
	this_c->col_command_duration	=  1 * 2;	/* cycles */
	this_c->rank_count	  	=      4;   	/* for SDRAM and DDR SDRAM, rank_count is DIMM-bank count */
	this_c->bank_count   		=      4;	/* per rank , 8 banks in near future.*/
	this_c->row_count		= 8*1024;
	this_c->col_count		=    512;	
	this_c->t_ras 	       		=  9 * 2; 	/* 45ns @ 400 mbps is 18 cycles */
	this_c->t_rp 	       		=  3 * 2; 	/* 15ns @ 400 mpbs is 6 cycles */
	this_c->t_rcd         		=  3 * 2; 
	this_c->t_cas 	       		=  3 * 2; 
	this_c->t_cac 		        = this_c->t_cas - this_c->col_command_duration;    /* delay in between CAS command and start of read data burst */
	this_c->t_cwd 	       		= this_c->t_cac;
	this_c->t_rtr         		=      0; 
	this_c->t_dqs      	=  1 * 2;
	this_c->t_rc			= this_c->t_ras + this_c->t_rp;
	this_c->t_rfc			= 51; // 127.5 ns 
	this_c->t_wr  	        	=  2; /* FIXME */ 
	this_c->t_rtp  	        	=  6; 

	this_c->tq_delay =2*2;	/* 4 DRAM ticks of delay @ 200 MHz = 20ns */
  this_c->max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;

	this_c->critical_word_first_flag=   TRUE;
	this_c->dram_clock_granularity	=      2;	/* DDR clock granularity */
	this_c->cas_with_prec			= true;
  } else if (dram_type == FBD_DDR2) {
	this_c->up_channel_width  		=      10;	/* bits */
	this_c->down_channel_width  	=      14;	/* bits */
	set_dram_channel_width(8);  				/* 8 byte wide data bus */
	set_memory_frequency(1600);
	set_dram_frequency(400);
	this_c->memory2dram_freq_ratio	=	this_c->memory_frequency/this_c->dram_frequency;
	this_c->row_command_duration	=  1 * 2;   	/* cycles */
	this_c->col_command_duration	=  1 * 2;	/* cycles */
	this_c->rank_count	  	=      4;   	/* for SDRAM and DDR SDRAM, rank_count is DIMM-bank count */
	this_c->bank_count   		=      4;	/* per rank , 8 banks in near future.*/
	this_c->row_count		= 8*1024;
	this_c->col_count		=    512;	
	this_c->t_ras 	       		=  9 * 2; 	/* 45ns @ 400 mbps is 18 cycles */
	this_c->t_rp 	       		=  3 * 2; 	/* 15ns @ 400 mpbs is 6 cycles */
	this_c->t_rcd         		=  3 * 2; 
	this_c->t_cas 	       		=  3 * 2; 
	this_c->t_cac 		        = this_c->t_cas - this_c->col_command_duration;    /* delay in between CAS command and start of read data burst */
	this_c->t_cwd 	       		= this_c->t_cac;
	this_c->t_rtr         		=      0; 
	this_c->t_dqs      	=  	0;
	this_c->t_rc			= this_c->t_ras + this_c->t_rp;
	this_c->t_rfc			= 51; // 127.5 ns 
	this_c->t_wr  	        	=  12; /* FIXME */ 
	this_c->t_rtp  	        	=  8; 

	this_c->t_amb_up		=	6;			/*** Arbitrary Value FIXME **/
	this_c->t_amb_down		=	6;			/*** Arbitrary Value FIXME **/
	this_c->up_buffer_cnt   = 4;			/** Set to as many up buffers as ther are banks in the rank **/
	this_c->down_buffer_cnt   = 4;			/** Set to as many up buffers as ther are banks in the rank **/

	this_c->tq_delay =2*2;	/* 4 DRAM ticks of delay @ 200 MHz = 20ns */
  this_c->max_tq_size = MAX_TRANSACTION_QUEUE_DEPTH;

	this_c->critical_word_first_flag=   FALSE;
	this_c->dram_clock_granularity	=      2;	/* DDR clock granularity */

	this_c->data_cmd_count 			= MAX(1,this_c->cacheline_size/DATA_BYTES_PER_WRITE_BUNDLE);
	this_c->drive_cmd_count 		= 1;
  	//this_c->refresh_policy			= REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK;
  	this_c->refresh_time			= 100;			
  	this_c->refresh_cycle_count		= 10000;
	this_c->cas_with_prec			= true;
  }
  else {
	fprintf(stderr,"Unknown memory type %d\n",dram_type);
	_exit(3);
  }
}

int get_dram_type(){		/* SDRAM, DDR RDRAM etc */
  return dram_system.config.dram_type;
}
/*******************************************
 * FBDIMM - This is the frequency of the DRAM
 * For all other configurations the frequency of the 
 * DRAM is the freq of the memory controller.
 * *****************************************/
void set_dram_frequency(int freq){
  if (dram_system.config.dram_type != FBD_DDR2) {
	if(freq < MIN_DRAM_FREQUENCY){
	  dram_system.config.memory_frequency 	= MIN_DRAM_FREQUENCY;		/* MIN DRAM freq */
	} else if (freq > MAX_DRAM_FREQUENCY){
	  dram_system.config.memory_frequency 	= MAX_DRAM_FREQUENCY;		/* MAX DRAM freq */
	} else {
	  dram_system.config.memory_frequency 	= freq;
	}
	dram_system.config.dram_frequency 	=  dram_system.config.memory_frequency;	
  }
  else if(freq < MIN_DRAM_FREQUENCY){
	dram_system.config.dram_frequency 	= MIN_DRAM_FREQUENCY;		/* MIN DRAM freq */
  } else if (freq > MAX_DRAM_FREQUENCY){
	dram_system.config.dram_frequency 	= MAX_DRAM_FREQUENCY;		/* MAX DRAM freq */
  } else {
	dram_system.config.dram_frequency 	= freq;
  }
  	dram_system.config.memory2dram_freq_ratio	=	(float)dram_system.config.memory_frequency/dram_system.config.dram_frequency;

	 power_update_freq_based_values(freq,&dram_system.config.dram_power_config);
}

void set_posted_cas(bool flag){
  dram_system.config.posted_cas_flag = flag;
}


int get_dram_frequency(){
  return dram_system.config.dram_frequency;
}

int get_memory_frequency(){
  return dram_system.config.memory_frequency;
}

void set_memory_frequency(int freq) { /* FBD-DIMM only*/
  if(freq < MIN_DRAM_FREQUENCY){
	dram_system.config.memory_frequency 	= MIN_DRAM_FREQUENCY;		/* MIN DRAM freq */
  } else if (freq > MAX_DRAM_FREQUENCY){
	dram_system.config.memory_frequency 	= MAX_DRAM_FREQUENCY;		/* MAX DRAM freq */
  } else {
	dram_system.config.memory_frequency 	= freq;
  }
}

int get_dram_channel_count(){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  return dram_system.config.channel_count;
}
void set_dram_channel_count(int channel_count){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  dram_system.config.channel_count = MIN(MAX(1,channel_count),MAX_CHANNEL_COUNT);
}

int get_dram_rank_count(){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  return dram_system.config.rank_count;
}
void set_dram_rank_count(int rank_count){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  dram_system.config.rank_count = MIN(MAX(1,rank_count),MAX_RANK_COUNT);
}

void set_dram_bank_count(int bank_count){               /* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  dram_system.config.bank_count = bank_count;
}
int get_dram_bank_count(){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  return dram_system.config.bank_count;
}

int get_dram_row_buffer_management_policy(){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  return dram_system.config.row_buffer_management_policy;
}
void set_dram_row_count(int row_count){                 /* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  dram_system.config.row_count = row_count;
}
void set_dram_buffer_count(int count){					/* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  dram_system.config.up_buffer_cnt= MIN(MAX(1,count),MAX_AMB_BUFFER_COUNT);
  dram_system.config.down_buffer_cnt= MIN(MAX(1,count),MAX_AMB_BUFFER_COUNT);
}

int get_dram_row_count(){
  return dram_system.config.row_count;
}

int get_dram_col_count(){
  return dram_system.config.col_count;
}

void set_dram_col_count(int col_count){                 /* set  1 <= channel_count <= MAX_CHANNEL_COUNT */
  dram_system.config.col_count = col_count;
}

void set_dram_channel_width(int channel_width){
  dram_system.config.channel_width 	= MAX(MIN_CHANNEL_WIDTH,channel_width);			/* smallest width is 2 bytes */
	dram_system.config.t_burst		= MAX(1,dram_system.config.cacheline_size / dram_system.config.channel_width);
}

void set_dram_transaction_granularity(int cacheline_size){
  dram_system.config.cacheline_size = MAX(MIN_CACHE_LINE_SIZE,cacheline_size);				/*bytes */
	dram_system.config.t_burst		= MAX(1,dram_system.config.cacheline_size / dram_system.config.channel_width);
	dram_system.config.data_cmd_count 			= MAX(1,dram_system.config.cacheline_size/DATA_BYTES_PER_WRITE_BUNDLE);
}

void set_pa_mapping_policy(int policy){
  dram_system.config.physical_address_mapping_policy = policy;
}

void set_dram_row_buffer_management_policy(int policy){
  dram_system.config.row_buffer_management_policy = policy;
   
}

void set_dram_refresh_policy(int policy){
  dram_system.config.refresh_policy = policy;
   dram_system.config.refresh_cycle_count = (tick_t)((dram_system.config.refresh_time/dram_system.config.row_count)*dram_system.config.dram_frequency);
	  /* Time between each refresh command */
	if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK)
	  dram_system.config.refresh_cycle_count =  dram_system.config.refresh_cycle_count/(dram_system.config.rank_count * dram_system.config.bank_count);
	else if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK)
	  dram_system.config.refresh_cycle_count =  dram_system.config.refresh_cycle_count/(dram_system.config.rank_count);
	else if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK)
	  dram_system.config.refresh_cycle_count =  dram_system.config.refresh_cycle_count/(dram_system.config.bank_count);

}

void set_dram_refresh_time(int refresh_time){
	dram_system.config.refresh_time = refresh_time;
	  dram_system.config.refresh_cycle_count = (refresh_time/dram_system.config.row_count)*dram_system.config.dram_frequency;
	  /* Time between each refresh command */
	if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK)
	  dram_system.config.refresh_cycle_count =  dram_system.config.refresh_cycle_count/(dram_system.config.rank_count * dram_system.config.bank_count);
	if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK)
	  dram_system.config.refresh_cycle_count =  dram_system.config.refresh_cycle_count/(dram_system.config.rank_count );
	else if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK)
	  dram_system.config.refresh_cycle_count =  dram_system.config.refresh_cycle_count/(dram_system.config.bank_count);
 

}
/* oppourtunistic vs highest priority */
void set_dram_refresh_issue_policy(int policy){
  dram_system.config.refresh_issue_policy = policy;
}

void set_fbd_var_latency_flag(int value) {
    if (value) {
      dram_system.config.var_latency_flag = true;
    }else {
      dram_system.config.var_latency_flag = false;
    }
}

void set_dram_debug(int debug_status){
  dram_system.config.dram_debug = debug_status;
}

void set_issue_debug(int debug_status){
  dram_system.config.issue_debug = debug_status;
}
void set_addr_debug(int debug_status){
  dram_system.config.addr_debug = debug_status;
}

void set_wave_debug(int debug_status){
  dram_system.config.wave_debug = debug_status;
}

void set_wave_cas_debug(int debug_status){
  dram_system.config.wave_cas_debug = debug_status;
}

void set_bundle_debug(int debug_status){
  dram_system.config.bundle_debug = debug_status;
}

void set_amb_buffer_debug(int debug_status){
  dram_system.config.amb_buffer_debug = debug_status;
}

/** FB-DIMM : set methods **/
void set_dram_up_channel_width(int channel_width) {
  dram_system.config.up_channel_width = channel_width;
  /** FIXME : Add code to set t_burst i.e. transmission time for data
   * packets*/
}

void set_dram_down_channel_width(int channel_width) {
  dram_system.config.down_channel_width = channel_width;
  /** FIXME : Add code to set t_burst i.e. transmission time for data
   * packets*/
}

void set_t_bundle(int t_bundle) {
  dram_system.config.t_bundle = t_bundle;
}

void set_t_bus(int t_bus) {
  dram_system.config.t_bus = t_bus;
}

/* This has to be enabled late */

void enable_auto_refresh(int flag, int refresh_time_ms){
  if(flag == TRUE){
	dram_system.config.auto_refresh_enabled 	= TRUE;
	dram_system.config.refresh_time 		= refresh_time_ms * 1000.0;	/* input is in milliseconds, keep here in microseconds */
	dram_system.config.refresh_cycle_count 		= (int) (dram_system.config.refresh_time * dram_system.config.dram_frequency/dram_system.config.row_count);
	if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK)
	  dram_system.config.refresh_cycle_count =  dram_system.config.refresh_cycle_count/(dram_system.config.rank_count * dram_system.config.bank_count);
	if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK)
	  dram_system.config.refresh_cycle_count =  dram_system.config.refresh_cycle_count/(dram_system.config.rank_count );
	else if (dram_system.config.refresh_policy == REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK)
	  dram_system.config.refresh_cycle_count =  dram_system.config.refresh_cycle_count/(dram_system.config.bank_count);

  } else {
	dram_system.config.auto_refresh_enabled 	= FALSE;
  }
}


void set_independent_threads(int *thread_sets,int num_thread_sets){
  int i;
  int j;
  int num_threads = 0;
  for (i=0;i<num_thread_sets;i++)
	  dram_system.config.thread_sets[i] = thread_sets[i];
  dram_system.config.num_thread_sets = num_thread_sets;
  for (i=0;i<num_thread_sets;i++) {
	for (j=0;j<thread_sets[i];j++) {
		dram_system.config.thread_set_map[num_threads++] = i;
	}	  
  }
}


 int addr_debug(){
  return (dram_system.config.addr_debug &&
	  (dram_system.tq_info.transaction_id_ctr >= dram_system.tq_info.debug_tran_id_threshold));
}

 int dram_debug(){
  return (dram_system.config.dram_debug &&
	  (dram_system.tq_info.transaction_id_ctr >= dram_system.tq_info.debug_tran_id_threshold));
}

int wave_debug(){
  return (dram_system.config.wave_debug &&
	  (dram_system.tq_info.transaction_id_ctr >= dram_system.tq_info.debug_tran_id_threshold));
}

 int cas_wave_debug(){
  return (dram_system.config.wave_cas_debug);
}

 int bundle_debug(){
  return (dram_system.config.bundle_debug &&
	  (dram_system.tq_info.transaction_id_ctr >= dram_system.tq_info.debug_tran_id_threshold));
}

 int amb_buffer_debug(){
  return (dram_system.config.amb_buffer_debug &&
	  (dram_system.tq_info.transaction_id_ctr >= dram_system.tq_info.debug_tran_id_threshold));
}

void set_strict_ordering(int flag){
  dram_system.config.strict_ordering_flag = flag;
}

void set_transaction_debug(int debug_status){
  dram_system.tq_info.debug_flag = debug_status;
}

void set_debug_tran_id_threshold(uint64_t dtit){
  dram_system.tq_info.debug_tran_id_threshold = dtit;
}

void set_tran_watch(uint64_t tran_id){
  dram_system.tq_info.tran_watch_flag = TRUE;
  dram_system.tq_info.tran_watch_id = tran_id;
}

int get_tran_watch(uint64_t tran_id) {
  return dram_system.tq_info.tran_watch_flag &&
  dram_system.tq_info.tran_watch_id == tran_id;
}
void set_ref_tran_watch(uint64_t tran_id){
  dram_system.config.watch_refresh = TRUE;
  dram_system.config.ref_tran_id = tran_id;
}
int get_ref_tran_watch(uint64_t tran_id) {
  return dram_system.config.watch_refresh && 
	dram_system.config.ref_tran_id == tran_id;
}
int get_transaction_debug(){	
  return (dram_system.tq_info.debug_flag && 
	  (dram_system.tq_info.transaction_id_ctr >= dram_system.tq_info.debug_tran_id_threshold));
}

dram_system_configuration_t *get_dram_system_config(){
  return &(dram_system.config);
}

power_info_t *get_dram_power_info(){
  return &(dram_system.dram_power);
}

dram_system_t *get_dram_system() {
  return &(dram_system);
}

tick_t get_dram_current_time(){
  return dram_system.current_dram_time;
}

/*
 *   Functions associated with memory controllers.
 */

void init_dram_controller(int controller_id){
  int i,j;
  dram_controller_t *this_dc;
  rank_t *this_r;

  this_dc = &(dram_system.dram_controller[controller_id]);

  /* initialize all ranks */
  for(i=0;i<MAX_RANK_COUNT;i++){
	this_r = &(this_dc->rank[i]);
	for(j=0;j<MAX_BANK_COUNT;j++){
	  this_r->bank[j].status 			= IDLE;
	  this_r->bank[j].row_id 			= MEM_STATE_INVALID;
	  this_r->bank[j].ras_done_time		= 0;
	  this_r->bank[j].cas_done_time		= 0;
	  this_r->bank[j].rcd_done_time		= 0;
	  this_r->bank[j].rp_done_time		= 0;
	  this_r->bank[j].last_command		= IDLE;
	  this_r->bank[j].cas_count_since_ras	= 0;
	  this_r->bank[j].paging_policy		= OPEN_PAGE;
	}
	for(j = 0;j<4;j++){
        this_r->activation_history[j]           = 0;
    }
  }
  this_dc->id				= controller_id;
  this_dc->command_bus.status 		= IDLE;
  this_dc->data_bus.status 		= IDLE;
  this_dc->data_bus.current_rank_id	= MEM_STATE_INVALID;
  this_dc->row_command_bus.status 	= IDLE;
  this_dc->col_command_bus.status 	= IDLE;
  this_dc->up_bus.status 	= IDLE;
  this_dc->up_bus.completion_time = 0;
  this_dc->down_bus.status 	= IDLE;
  this_dc->last_rank_id     = dram_system.config.rank_count - 1;
  this_dc->last_bank_id     = dram_system.config.bank_count - 1;
  this_dc->last_command     = IDLE;
  this_dc->next_command     = RAS;
  this_dc->bundle_id_ctr = 0;
  this_dc->refresh_queue.refresh_row_index 		= 0;
  this_dc->refresh_queue.refresh_rank_index 		= 0;
  this_dc->refresh_queue.refresh_bank_index 		= 0;
  this_dc->refresh_queue.last_refresh_cycle 		= 0;
  this_dc->refresh_queue.refresh_pending			= FALSE;
}

void init_amb_buffer() {

  int i,j,k;
  dram_controller_t *this_dc;
  dimm_t *this_d;

  if (dram_system.config.dram_type == FBD_DDR2) {
	for(i=0;i<MAX_CHANNEL_COUNT;i++){
	  this_dc = &(dram_system.dram_controller[i]);
	  /* initialize all ranks */
	  for(j=0;j<MAX_RANK_COUNT;j++){
		this_d = &(this_dc->dimm[j]);

		/* set up the amb buffers */
		if (this_d->up_buffer == NULL ) {
		  this_d->up_buffer = (amb_buffer_t *)calloc(dram_system.config.up_buffer_cnt,sizeof(amb_buffer_t));
		}

		if (this_d->up_buffer == NULL ) {
		  fprintf(stderr," Error allocating amb up buffer for channel %d rank %d \n",i,j);
		}
		if (this_d->down_buffer == NULL ) {
		  this_d->down_buffer = (amb_buffer_t *)calloc(dram_system.config.down_buffer_cnt,sizeof(amb_buffer_t));
		}
		if (this_d->down_buffer == NULL ) {
		  fprintf(stderr," Error allocating amb down buffer for channel %d rank %d \n",i,j);
		}
		this_d->num_up_buffer_free = dram_system.config.up_buffer_cnt;
		this_d->num_down_buffer_free = dram_system.config.down_buffer_cnt;

		for (k=0; k < dram_system.config.up_buffer_cnt; k++) {
		  this_d->up_buffer[k].value = 0;
		  this_d->up_buffer[k].tran_id = 0;
		  this_d->up_buffer[k].occupied= FALSE;

		}
		for (k=0; k < dram_system.config.down_buffer_cnt; k++) {
		  this_d->down_buffer[k].value = 0;
		  this_d->down_buffer[k].tran_id = 0;
		  this_d->down_buffer[k].occupied = FALSE;
		}
	  }
	}
  }
}

int row_command_bus_idle(int chan_id){
  dram_controller_t *this_dc;

  this_dc = &(dram_system.dram_controller[chan_id]);
  if((this_dc->command_bus.status == IDLE)) { 
	return TRUE;
  } else {
	return FALSE;
  }
}

int col_command_bus_idle(int chan_id){
  dram_controller_t *this_dc;

  this_dc = &(dram_system.dram_controller[chan_id]);
  if(this_dc->command_bus.status == IDLE){
	return TRUE;
  } else {
	return FALSE;
  }
}

int is_cmd_bus_idle(int chan_id,command_t *this_c) {
	if (this_c->command == CAS || this_c->command == CAS_WRITE || this_c->command == CAS_WRITE_AND_PRECHARGE || this_c->command == CAS_WITH_DRIVE || this_c->command == CAS_AND_PRECHARGE) {
	  return col_command_bus_idle(chan_id);
	}else {
	  return row_command_bus_idle(chan_id);
	}
}

int up_bus_idle(int chan_id){
  dram_controller_t *this_dc;

  this_dc = &(dram_system.dram_controller[chan_id]);
  if ((dram_system.config.dram_type == FBD_DDR2) && (this_dc->up_bus.status == IDLE)) {
	return TRUE;
  } else {
	return FALSE;
  }
}

int down_bus_idle(int chan_id){
  dram_controller_t *this_dc;

  this_dc = &(dram_system.dram_controller[chan_id]);
  if ((dram_system.config.dram_type == FBD_DDR2) && (this_dc->down_bus.status == IDLE)) {
	return TRUE;
  } else {
	return FALSE;
  }
}



void set_row_command_bus(int chan_id, int command){
  dram_controller_t *this_dc;

  this_dc = &(dram_system.dram_controller[chan_id]);
  this_dc->command_bus.status = BUS_BUSY;
  this_dc->command_bus.command = command;
  this_dc->command_bus.completion_time = dram_system.current_dram_time + dram_system.config.col_command_duration;
}

void set_col_command_bus(int chan_id, int command){
  dram_controller_t *this_dc;

  this_dc = &(dram_system.dram_controller[chan_id]);
  this_dc->command_bus.status = BUS_BUSY;
  this_dc->command_bus.command = command;
  this_dc->command_bus.completion_time = dram_system.current_dram_time + dram_system.config.col_command_duration;
  // Hack for posted CAS systems needed
  if (dram_system.config.posted_cas_flag && (command == CAS || command == CAS_WRITE || command == CAS_AND_PRECHARGE ||
        command == CAS_WRITE_AND_PRECHARGE))
    this_dc->command_bus.completion_time+= dram_system.config.row_command_duration;
}


/*
 *  Here are the function calls to the transaction queue 
 */

void init_transaction_queue(){
  int i;
  int j;
  /* initialize entries in the transaction queue */

  dram_system.tq_info.debug_flag		= FALSE;
  dram_system.tq_info.debug_tran_id_threshold = 0;
  dram_system.tq_info.tran_watch_flag	= FALSE;
  dram_system.tq_info.tran_watch_id = 0;

  for(i=0;i<MAX_TRANSACTION_QUEUE_DEPTH;i++){
	for (j=0;j<MAX_CHANNEL_COUNT;j++) {
	  dram_system.dram_controller[j].transaction_queue.entry[i].status 	= MEM_STATE_INVALID;
	  dram_system.dram_controller[j].transaction_queue.entry[i].next_c 	= NULL;
	  dram_system.dram_controller[j].transaction_queue.transaction_count		= 0;
	}
  }
  dram_system.tq_info.free_command_pool		= NULL;
  /* ask dave abou the line below */
  dram_system.config.tq_delay			= 1;		/* set 10 cycle delay through queue */
}

void set_chipset_delay(int delay){
  dram_system.config.tq_delay = MAX(1,delay);
}

int get_chipset_delay(){
  return dram_system.config.tq_delay;
}


/* grab empty command thing from queue */

command_t *acquire_command(){
  command_t        *temp_c;

  if(dram_system.tq_info.free_command_pool == NULL){
	temp_c = (command_t *)calloc(1,sizeof(command_t));
  } else {
	temp_c = dram_system.tq_info.free_command_pool;
	dram_system.tq_info.free_command_pool = dram_system.tq_info.free_command_pool -> next_c;
  }
  temp_c->next_c = NULL;
  return temp_c;
}

/* release command_t into empty command pool */

void release_command(command_t *freed_command){
  freed_command->next_c = dram_system.tq_info.free_command_pool;
  dram_system.tq_info.free_command_pool = freed_command;
}

/*** This command takes the tran_index i.e. location in the transaction queue
 * as an argument 
 * tran_id - Unique Transaction Identifier 
 * data_word and data_word_position -> extra parameters added for FBDIMM only
 *  data_word : signifies size of data chunk that drive / cas with drive
 *  dispatches. cacheline_size/DATA_BYTES_PER_READ_BUNDLE
 *  data_word_position : signfies if its the first or the last or middle data
 *  chunks for write data  
 *  cmd_id : gives the command id used for statistcs
 * ***/     
command_t *add_command(tick_t now,
	command_t *command_queue,
	int command,     
	addresses_t *this_a,
	uint64_t tran_id,
	int data_word,
	int data_word_position,
	int cmd_id){

  command_t *temp_c;
  command_t *this_c;

  temp_c = acquire_command();
  temp_c->start_time     	= now;
  temp_c->status     	= IN_QUEUE;
  temp_c->command        	= command;
  temp_c->tran_id			= tran_id;
  temp_c->chan_id      	= this_a->chan_id;
  temp_c->rank_id      	= this_a->rank_id;
  temp_c->bank_id        	= this_a->bank_id;
  temp_c->row_id  	= this_a->row_id;
  temp_c->col_id 		= this_a->col_id;
  temp_c->next_c         	= NULL;
  temp_c->posted_cas		= FALSE; /** NOTE : Used only in FBDIMM **/
  temp_c->refresh	= FALSE; 
  temp_c->rq_next_c	= NULL; 
  /** Only used for Data, Drive , Cas with drive in FBDIMM **/
  temp_c->data_word = data_word;
  temp_c->data_word_position = data_word_position;

  temp_c->cmd_id =  cmd_id;
  temp_c->link_comm_tran_comp_time = 0;
  temp_c->amb_proc_comp_time = 0;
  temp_c->dimm_comm_tran_comp_time = 0;
  temp_c->dimm_data_tran_comp_time = 0;
  temp_c->dram_proc_comp_time = 0;
  temp_c->amb_down_proc_comp_time = 0;
  temp_c->link_data_tran_comp_time = 0;

  if(command_queue == NULL){
	command_queue = temp_c;
  } else {
	this_c = command_queue;
	while (this_c->next_c != NULL) {
	  this_c = this_c->next_c;
	}
	this_c->next_c = temp_c; /* attach command at end of queue */
  }
  return command_queue;
}


/*
 *  This is the interface to the entire memory system.
 *  The memory system takes commands from biubufferptr, and 
 *  returns results to the biubufferptr as well. 
 *  This update needs the selection policy here
 */

void init_dram_system(){
  int i;
  dram_system.last_dram_time 		= 0;
  dram_system.current_dram_time 		= 0;
  init_dram_system_configuration();
  init_transaction_queue();
  for(i=0;i<MAX_CHANNEL_COUNT;i++){
	init_dram_controller(i);
  }
}
/**
 * All cycles in the config file are in terms of dram cycles.
 * This converts everything in terms of memory controller cycles. 
 */
void convert_config_dram_cycles_to_mem_cycles(){
    dram_system_configuration_t *this_c = get_dram_system_config();
    // Not sure wher to put this -> but it needs to be done
    this_c->t_cac = this_c->t_cas - this_c->col_command_duration;
    this_c->t_ras*=(int)this_c->memory2dram_freq_ratio;              /* interval between ACT and PRECHARGE to same bank */
    this_c->t_rcd*=(int)this_c->memory2dram_freq_ratio;              /* RAS to CAS delay of same bank */
    this_c->t_cas*=(int)this_c->memory2dram_freq_ratio;              /* delay between start of CAS command and start of data burst */

    this_c->t_cac*=(int)this_c->memory2dram_freq_ratio;              /* delay between end of CAS command and start of data burst*/
    this_c->t_rp*=(int)this_c->memory2dram_freq_ratio;               /* interval between PRECHARGE and ACT to same bank */
                            /* t_rc is simply t_ras + t_rp */
    this_c->t_rc*=(int)this_c->memory2dram_freq_ratio;
    this_c->t_rfc*=(int)this_c->memory2dram_freq_ratio;
    this_c->t_cwd*=(int)this_c->memory2dram_freq_ratio;              /* delay between end of CAS Write command and start of data packet */
    this_c->t_rtr*=(int)this_c->memory2dram_freq_ratio;              /* delay between start of CAS Write command and start of write retirement command*/
    this_c->t_burst*=(int)this_c->memory2dram_freq_ratio;           /* number of cycles utilized per cacheline burst */

    this_c->t_al*=(int)this_c->memory2dram_freq_ratio;               /* additive latency = t_rcd - 2 (ddr half cycles) */
    this_c->t_rl*=(int)this_c->memory2dram_freq_ratio;               /* read latency  = t_al + t_cas */
    this_c->t_wr*=(int)this_c->memory2dram_freq_ratio;               /* write recovery time latency, time to restore data o cells */
    this_c->t_rtp*=(int)this_c->memory2dram_freq_ratio;               /* write recovery time latency, time to restore data o cells */

    //this_c->t_bus*=(int)this_c->memory2dram_freq_ratio;                  /* FBDIMM - bus delay */
    this_c->t_amb_up*=(int)this_c->memory2dram_freq_ratio;               /* FBDIMM - Amb up delay */
    this_c->t_amb_down*=(int)this_c->memory2dram_freq_ratio;             /* FBDIMM - Amb down delay */
    //this_c->t_bundle*=(int)this_c->memory2dram_freq_ratio;               /* FBDIMM number of cycles utilized to transmit a bundle */
    this_c->row_command_duration*=(int)this_c->memory2dram_freq_ratio; // originally 2 -> goest to 12
    this_c->col_command_duration*=(int)this_c->memory2dram_freq_ratio;

    this_c->t_dqs*=(int)this_c->memory2dram_freq_ratio;          /* rank hand off penalty. 0 for SDRAM, 2 for DDR, */

    this_c->refresh_cycle_count*=(int)this_c->memory2dram_freq_ratio; 
	return;
}
/* This code is currently useless.  It is a placeholder just in case we use different 
 * simulation code for different memory systems. Currently we use a unified code to 
 * simulate everything
 */

void update_dram_system(tick_t now) {		/* cpu ticks */
  if(dram_system.config.dram_type == SDRAM){
	update_base_dram(now);
  } else if(dram_system.config.dram_type == DDRSDRAM){
	update_base_dram(now);
  } else if(dram_system.config.dram_type == DDR2){
	update_base_dram(now);
  } else if(dram_system.config.dram_type == DDR3){
	update_base_dram(now);
  } else if(dram_system.config.dram_type == FBD_DDR2){
	update_base_dram(now);
  } else {
	fprintf(stderr,"Memory type unknown\n");
  }
}


void update_base_dram(tick_t current_cpu_time) {		/* This code should work for SDRAM, DDR SDRAM as well as RDRAM */

  tick_t 	dram_stop_time;
  tick_t  expected_dram_time;
  tick_t missing_dram_time;
  int	i;
  int	done;
  int	chan_id;
  char	debug_string[MAX_DEBUG_STRING_SIZE];
  //Ohm--Add for power calculation
  int   all_bnk_pre;
  int j;
  int precharge_bank = 0;

  expected_dram_time = (tick_t) (global_biu->mem2cpu_clock_ratio * current_cpu_time);
    dram_stop_time = (tick_t) (global_biu->mem2cpu_clock_ratio * (current_cpu_time + 1)); 		/* 1 cpu tick into the future */
  debug_string[0] ='\0';
  if((expected_dram_time - dram_system.last_dram_time) < dram_system.config.dram_clock_granularity){ /* handles DDR, QDR and everything else. */
	return;
  }
  else if((dram_system.config.auto_refresh_enabled == TRUE)){
	for(chan_id = 0;chan_id<dram_system.config.channel_count;chan_id++){
		update_refresh_missing_cycles(dram_system.current_dram_time,chan_id,debug_string);
	}
  }
  missing_dram_time = expected_dram_time - (dram_system.last_dram_time + dram_system.config.dram_clock_granularity);//OHm: work here !!!!
  if (missing_dram_time > 0) {
	//decide if the missing cycles are for ACTIVE or PRECHARGE state
	for (chan_id = 0; chan_id < dram_system.config.channel_count; chan_id++) {
	  for (i = 0; i < dram_system.config.rank_count; i++) {
		for (j = 0; j < dram_system.config.bank_count; j++) {
		  if ((dram_system.dram_controller[chan_id].rank[i].bank[j].last_command == PRECHARGE) ||
			  (dram_system.dram_controller[chan_id].rank[i].bank[j].status == PRECHARGING)) {
			precharge_bank++;
		  }
		}
		if (precharge_bank==dram_system.config.bank_count) {//all bank precharged
#ifdef DEBUG_POWER
		  fprintf(stdout,"all_bnk_pre:missing_cycles[%d]",missing_dram_time);
#endif
		  dram_system.dram_controller[chan_id].rank[i].r_p_info.bnk_pre += missing_dram_time;
		  dram_system.dram_controller[chan_id].rank[i].r_p_gblinfo.bnk_pre += missing_dram_time;
		}
		}
	  }
	}
  dram_system.current_dram_time = expected_dram_time;

  while(dram_system.current_dram_time <= dram_stop_time){	/* continue to simulate until time limit */
	/* First thing we have to do is to figure out if a certain command has completed */
	/* Then if that command has completed, we can then retire the transaction */
	debug_string[0] ='\0';
	for(chan_id = 0;chan_id<dram_system.config.channel_count;chan_id++){
	  for(i=0;i<dram_system.dram_controller[chan_id].transaction_queue.transaction_count;i++){
		if (dram_system.dram_controller[chan_id].transaction_queue.entry[i].status != MEM_STATE_COMPLETED &&
			dram_system.dram_controller[chan_id].transaction_queue.entry[i].status != MEM_STATE_SCHEDULED) {
		  done = update_command_states(dram_system.current_dram_time,chan_id, i, debug_string); /** Sending transaction index **/
		  if(done == TRUE) {
#ifdef DEBUG
			if(get_transaction_debug() ){ 
			  fprintf(stdout,"Completed @ [%llu] Duration [%llu] :",
				  dram_system.current_dram_time,
                  dram_system.current_dram_time - - dram_system.dram_controller[chan_id].transaction_queue.entry[i].arrival_time);
			  print_transaction_index(dram_system.current_dram_time,chan_id,i);
			}
#endif
			dram_system.dram_controller[chan_id].transaction_queue.entry[i].completion_time 	= dram_system.current_dram_time  
			  + dram_system.config.tq_delay ;	
			dram_system.dram_controller[chan_id].transaction_queue.entry[i].status = MEM_STATE_COMPLETED;
		  }
		}
	  }
	}

	  retire_transactions();

	  /* now look through the current transaction list for commands we can schedule. *If* the command bus is free */
	  /* AND that command has to be able to be issued, subject to DRAM system restrictions */
	  if(wave_debug() || cas_wave_debug()){			/* draw cheesy text waveform output */
#ifdef DEBUG
		fprintf(stdout,"%8d  ",(int)dram_system.current_dram_time);
	  }
#endif

	  gather_tran_stats();
	  gather_biu_slot_stats(get_biu_address());

	  for(chan_id = 0;chan_id<dram_system.config.channel_count;chan_id++){

		update_refresh_status(dram_system.current_dram_time,chan_id,debug_string);
		update_bus_link_status(chan_id); /* added to take t_bus out of consideration when changing bus status */

		if (dram_system.config.dram_type != FBD_DDR2) {
		  issue_new_commands(dram_system.current_dram_time,chan_id, debug_string);
		}
		else {
		  // Schedule only on even clock cycles
		  if (dram_system.current_dram_time%2 == 0 ) {
			  commands2bundle(dram_system.current_dram_time, chan_id,debug_string);
          }
		}
		// You can only do this once
			schedule_new_transaction(chan_id);

		if (dram_system.config.auto_refresh_enabled == TRUE) { insert_refresh_transaction(chan_id);}

		update_cke_bit(dram_system.current_dram_time,chan_id,dram_system.config.rank_count);
#ifdef DEBUG
		if(wave_debug()|| cas_wave_debug()){			/* draw cheesy text waveform output */
		  if (dram_system.config.dram_type == FBD_DDR2) {
			/*** FIXME We need to figure out how to draw our waveform **/
			fprintf(stdout," CH[%d] ",chan_id);
			if(dram_system.dram_controller[chan_id].up_bus.status == IDLE){
			  fprintf(stdout," UI ");
			} else {
			  fprintf(stdout," UD ");
			}
			if(dram_system.dram_controller[chan_id].down_bus.status == IDLE){
			  fprintf(stdout," DI ");
			} else {
			  fprintf(stdout," DD ");
			}
			fprintf(stdout,"%s",debug_string);
			fprintf(stdout,"\n");
		  }
		  else {
			if(dram_system.dram_controller[chan_id].data_bus.status == IDLE){
			  fprintf(stdout," I ");
			} else {
			  fprintf(stdout," D ");
			}
			if(col_command_bus_idle(chan_id)){	/* unified command bus or just row command bus */
			  fprintf(stdout," |   ");
			} else {
			  fprintf(stdout,"   | ");
			}
			fprintf(stdout,"%s",debug_string);
		  }
		}  
		else if(dram_debug()){ 
		  fprintf(stdout,"%s",debug_string);

		}	
		debug_string[0] = '\0';
		
	  }
#endif
	  /* Ohm--add the code to track BNK_PRE% HERE  */

	  for (chan_id = 0; chan_id < dram_system.config.channel_count; chan_id++) {
		for (i = 0; i < dram_system.config.rank_count; i++) {
		  all_bnk_pre = TRUE;
		  for (j = 0; j < dram_system.config.bank_count; j++) {
		    if (dram_system.dram_controller[chan_id].rank[i].bank[j].rp_done_time < dram_system.dram_controller[chan_id].rank[i].bank[j].ras_done_time)
			  all_bnk_pre = FALSE;
		  }

		  if (all_bnk_pre == TRUE) {
#ifdef DEBUG_POWER
		        fprintf(stdout,"[%d]all_bnk_pre:",i);
#endif
			dram_system.dram_controller[chan_id].rank[i].r_p_info.bnk_pre += 
			  dram_system.config.dram_clock_granularity;
			dram_system.dram_controller[chan_id].rank[i].r_p_gblinfo.bnk_pre += 
			  dram_system.config.dram_clock_granularity;
			if (check_cke_hi_pre(chan_id, i))       {
			  dram_system.dram_controller[chan_id].rank[i].r_p_info.cke_hi_pre += 
				dram_system.config.dram_clock_granularity;
			  dram_system.dram_controller[chan_id].rank[i].r_p_gblinfo.cke_hi_pre += 
				dram_system.config.dram_clock_granularity;
			}
			else {
#ifdef DEBUG_POWER
			  fprintf(stdout,"[%d]cke_lo_pre",i);
#endif
			}
		  }
		  else //ACTIVE state
		  {
			//to find out CKE_HI_ACT
			if (check_cke_hi_act(chan_id, i)) {
			  dram_system.dram_controller[chan_id].rank[i].r_p_info.cke_hi_act += dram_system.config.dram_clock_granularity;;
			  dram_system.dram_controller[chan_id].rank[i].r_p_gblinfo.cke_hi_act += dram_system.config.dram_clock_granularity;;
			}
			else {
#ifdef DEBUG_POWER
			  fprintf(stdout,"[%d]cke_lo_act",i);
#endif
			}
		  }
		}
	  }
	  /* Ohm--Then, print me the power stats */
      if ((dram_system.config.dram_type==DDRSDRAM) || (dram_system.config.dram_type==DDR2) || (dram_system.config.dram_type == FBD_DDR2)){
        print_power_stats(dram_system.current_dram_time, &dram_system); 
      }

	  dram_system.last_dram_time = dram_system.current_dram_time;
	  if (dram_system.current_dram_time%2 == 0) 
		  dram_system.current_dram_time += dram_system.config.dram_clock_granularity;
	  else 
		dram_system.current_dram_time += 1;
#ifdef DEBUG
	  if(wave_debug() || dram_debug() || cas_wave_debug()){
		fprintf(stdout,"\n");
	  }
	  assert (strlen(debug_string) < MAX_DEBUG_STRING_SIZE);
#endif

	}

  }
/** 
 * This function checks if the dram controller has work left to do.
 * Checks if the transaction queue/refresh queue has any entries
 */
#ifdef ALPHA_SIM
  int is_dram_busy() {
	int busy = false;
#else
  bool is_dram_busy() {
	bool busy = false;
#endif
	int chan_id;
	
	
	for(chan_id = 0;busy == false && chan_id<dram_system.config.channel_count;chan_id++){
	  if (dram_system.dram_controller[chan_id].transaction_queue.transaction_count > 0) 
	    busy = true;
	  busy = is_refresh_pending(chan_id);
	}
	return busy;
  }
