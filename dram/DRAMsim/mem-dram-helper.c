/*
 * mem-dram-helper.c Misc routines which print out commands/transaction details
 * do base calculations.
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


int sim_dram_log2(int input){
  int result ;
  result = MEM_STATE_INVALID;
  if(input == 1){
    result = 0;
  } else if (input == 2){
    result = 1;
  } else if (input == 4){
    result = 2;
  } else if (input == 8){
    result = 3;
  } else if (input == 16){
    result = 4;
  } else if (input == 32){
    result = 5;
  } else if (input == 64){
    result = 6;
  } else if (input == 128){
    result = 7;
  } else if (input == 256){
    result = 8;
  } else if (input == 512){
    result = 9;
  } else if (input == 1024){
    result = 10;
  } else if (input == 2048){
    result = 11;
  } else if (input == 4096){
    result = 12;
  } else if (input == 8192){
    result = 13;
  } else if (input == 16384){
    result = 14;
  } else if (input == 32768){
    result = 15;
  } else if (input == 65536){
    result = 16;
  } else if (input == 65536*2){
    result = 17;
  } else if (input == 65536*4){
    result = 18;
  } else if (input == 65536*8){
    result = 19;
  } else if (input == 65536*16){
    result = 20;
  } else if (input == 65536*32){
    result = 21;
  } else if (input == 65536*64){
    result = 22;
  } else if (input == 65536*128){
    result = 23;
  } else if (input == 65536*256){
    result = 24;
  } else if (input == 65536*512){
    result = 25;
  } else if (input == 65536*1024){
    result = 26;
  } else if (input == 65536*2048){
    result = 27;
  } else if (input == 65536*4096){
    result = 28;
  } else if (input == 65536*8192){
    result = 29;
  } else if (input == 65536*16384){
    result = 30;
  } else {
    fprintf(stderr,"Error, %d is not a nice power of 2\n",input);
	result = 1; /** FIXME ***/
	/*
    _exit(2);
	*/
  }
  return result;
}

void print_dram_system_state(){
  rank_t *this_r;
  int i,j,k,channel_count,rank_count,bank_count;

  channel_count 	= dram_system.config.channel_count;
  rank_count 	= dram_system.config.rank_count;
  bank_count 	= dram_system.config.bank_count;

  for(i=0;i<channel_count;i++){
    for(j=0;j<rank_count;j++){
      this_r = &(dram_system.dram_controller[i].rank[j]);
      for(k=0;k<bank_count;k++){
	fprintf(stdout,"controller[%2d] rank [%2d] bank[%2x] status[", i, j, k);
	print_status(this_r->bank[k].status);
	fprintf(stdout,"] row_id[0x%8x]\n", this_r->bank[k].row_id); 
      }
    }
  }
}

void print_status(int status){
  switch(status) {
  case IDLE:
    fprintf(stdout,"IDLE");
    break;
  case PRECHARGING:
    fprintf(stdout,"PREC");
    break;
  case ACTIVATING:
    fprintf(stdout,"ACTG");
    break;
  case ACTIVE:
    fprintf(stdout,"ACT ");
    break;
  default:
    fprintf(stdout,"UNKN");
    break;
  }
}

void print_bus_status(){
  int chan_id;
  for(chan_id = 0;chan_id < dram_system.config.channel_count;chan_id++){
    fprintf(stdout,"Channel[%2d] Command [%2d] Data[%2d]\n",
	    chan_id,
	    dram_system.dram_controller[chan_id].command_bus.status,
	    dram_system.dram_controller[chan_id].data_bus.status);
  }
}

void print_command(tick_t now,
		   command_t *this_c){
  if(this_c == NULL){
    fprintf(stdout," Error-Null command \n");
    return;
  } else {
    fprintf(stdout," NOW[%llu] status[%d] end[%d] ", now, this_c->status, (int) this_c->completion_time);
    if(this_c->command == PRECHARGE) {
      fprintf(stdout,"PRECHARGE ");
    } else if(this_c->command == RAS) {
      fprintf(stdout,"RAS       ");
    } else if(this_c->command == CAS) {
      fprintf(stdout,"CAS       ");
    } else if(this_c->command == CAS_WITH_DRIVE) {
      fprintf(stdout,"CAS_WITH_DRIVE ");
    } else if(this_c->command == CAS_WRITE) {
      fprintf(stdout,"CAS WRITE ");
    } else if(this_c->command == CAS_AND_PRECHARGE) {
      fprintf(stdout,"CAP		  ");
    } else if(this_c->command == CAS_WRITE_AND_PRECHARGE) {
      fprintf(stdout,"CPW       ");
    } else if(this_c->command == DATA ) {
      fprintf(stdout,"DATA 	   ");
    } else if(this_c->command == DRIVE ) {
      fprintf(stdout,"DRIVE 	  ");
    } else if(this_c->command == RAS_ALL) {
      fprintf(stdout,"RAS_ALL   ");
    } else if(this_c->command == PRECHARGE_ALL) {
      fprintf(stdout,"PREC_ALL  ");
    } else if(this_c->command == REFRESH ) {
      fprintf(stdout,"REFRESH   ");
    } else if(this_c->command == REFRESH_ALL ) {
      fprintf(stdout,"REF_ALL   ");
    } else {
      fprintf(stdout,"UNKNOWN[%d] ", this_c->command);
    }
    fprintf(stdout,"chan_id[%2X] rank_id[%2X] bank_id[%2X] row_id[%5X] col_id[%4X] tran_id[%llu] cmd_id[%d] ref[%d]\n",
	    this_c->chan_id,
	    this_c->rank_id,
	    this_c->bank_id,
	    this_c->row_id,
	    this_c->col_id,
	    this_c->tran_id,
		this_c->cmd_id,
        this_c->refresh);
  }
}

void print_transaction(tick_t now, int chan_id, uint64_t tid){
  transaction_t *this_t = NULL;
  command_t *this_c;
  int tindex =  get_transaction_index(chan_id,tid);
  int rid;
  this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[tindex]);
  rid = get_rid(get_biu_address(),this_t->slot_id);	
  assert(tindex <= MAX_TRANSACTION_QUEUE_DEPTH);
  if(this_t->transaction_type == MEMORY_READ_COMMAND ){
    fprintf(stdout,"READ_TRANSACTION [%llu] to addr[%llx] rid(%d)\n",tid,this_t->address.physical_address,rid);
  } else if(this_t->transaction_type == MEMORY_IFETCH_COMMAND ){
    fprintf(stdout,"IFETCH_TRANSACTION [%llu] to addr[%llx] rid(%d)\n",tid,this_t->address.physical_address,rid);
  } else if(this_t->transaction_type == MEMORY_WRITE_COMMAND ){
    fprintf(stdout,"WRITE_TRANSACTION [%llu] to addr[%llx] rid(%d)\n",tid,this_t->address.physical_address,rid );
  } else if(this_t->transaction_type == MEMORY_PREFETCH ){
    fprintf(stdout,"MEMORY_PREFETCH [%llu] to addr[%llx] rid(%d)\n",tid,this_t->address.physical_address,rid);
  }
  this_c = this_t->next_c;
  while(this_c != NULL){
    print_command(now, this_c);
    this_c = this_c->next_c;
  }
  fprintf(stdout,"------------------\n");
  fflush(stdout);
}
void print_transaction_index(tick_t now, int chan_id, uint64_t tindex){
  transaction_t *this_t = NULL;
  command_t *this_c;
  int rid;
  this_t = &(dram_system.dram_controller[chan_id].transaction_queue.entry[tindex]);
  rid = get_rid(get_biu_address(),this_t->slot_id);	
  assert(tindex <= MAX_TRANSACTION_QUEUE_DEPTH);
  if(this_t->transaction_type == MEMORY_READ_COMMAND ){
	fprintf(stdout,"READ_TRANSACTION [%d] to addr[%llx] rid(%d)\n",this_t->transaction_id,this_t->address.physical_address,rid);
  } else if(this_t->transaction_type == MEMORY_IFETCH_COMMAND ){
	fprintf(stdout,"IFETCH_TRANSACTION [%d] to addr[%llx] rid(%d)\n",this_t->transaction_id,this_t->address.physical_address,rid);
  } else if(this_t->transaction_type == MEMORY_WRITE_COMMAND ){
	fprintf(stdout,"WRITE_TRANSACTION [%d] to addr[%llx] rid(%d)\n",this_t->transaction_id,this_t->address.physical_address,rid );
  } else if(this_t->transaction_type == MEMORY_PREFETCH ){
	fprintf(stdout,"MEMORY_PREFETCH [%d] to addr[%llx] rid(%d)\n",this_t->transaction_id,this_t->address.physical_address,rid);
  }
  this_c = this_t->next_c;
  while(this_c != NULL){
	print_command(now, this_c);
	this_c = this_c->next_c;
  }
  fprintf(stdout,"------------------\n");
  fflush(stdout);
}

void print_bundle(tick_t now,  command_t *bundle[],int chan_id){
  int i;
  int cmd_cnt = 0;
  fprintf(stdout,">>>>>>>>>>>> BUNDLE NOW[%llu] chan[%d]",now,chan_id);
  for (i=0;i<BUNDLE_SIZE;i++) {
    if (bundle[i] != NULL) {
      if (bundle[i]->command == DATA)
	cmd_cnt += DATA_CMD_SIZE;
      else
	cmd_cnt++;
    }
  }

  fprintf(stdout," size[%d] \n",cmd_cnt);
	
  for (i=0; i < BUNDLE_SIZE; i++) {
    if (bundle[i] != NULL) {
      print_command(now,bundle[i]);
    }
  }		
  fprintf(stdout,"------------------\n");
  fflush(stdout);
}


void print_transaction_queue(tick_t now){
  command_t *this_c;
  int i;
  int chan_id;
  for (chan_id =0;chan_id < dram_system.config.channel_count;chan_id++) {
  for(i=0;i<MAX_TRANSACTION_QUEUE_DEPTH;i++){
    fprintf(stdout,"Entry[%2d] Status[%2d] Start_time[%8d] ID[%u] Access Type[%2d] Addr[0x%8X] Channel[%2d] \n",
	    i, dram_system.dram_controller[chan_id].transaction_queue.entry[i].status, 
	    (int)dram_system.dram_controller[chan_id].transaction_queue.entry[i].arrival_time, 
	    dram_system.dram_controller[chan_id].transaction_queue.entry[i].transaction_id,
	    dram_system.dram_controller[chan_id].transaction_queue.entry[i].transaction_type,
	    dram_system.dram_controller[chan_id].transaction_queue.entry[i].address.physical_address,
	    dram_system.dram_controller[chan_id].transaction_queue.entry[i].address.chan_id);
    this_c = dram_system.dram_controller[chan_id].transaction_queue.entry[i].next_c;
    while(this_c != NULL){
      print_command(now, this_c);
      this_c = this_c->next_c;
    }
  }
  }
}

int get_num_ifetch() {
    return dram_system.tq_info.num_ifetch_tran;
}

int get_num_prefetch() {
    return dram_system.tq_info.num_prefetch_tran;
}

int get_num_read() {
  return dram_system.tq_info.num_read_tran;
}

int get_num_write() {
  return dram_system.tq_info.num_write_tran;
}

/* 
 * Build up cycle display for wave.
 */
void build_wave(char *buffer_string, int transaction_id, command_t *this_c, int action){
  int buffer_len;
  int print_detail;

  //print_detail = FALSE;
  print_detail = TRUE;
  buffer_len = strlen(buffer_string);
  switch(this_c->command){
  case RAS:
    sprintf(&buffer_string[buffer_len],"RAS ");
    break;
  case CAS:
    sprintf(&buffer_string[buffer_len],"CAS ");
    break;
  case CAS_WITH_DRIVE:
    sprintf(&buffer_string[buffer_len],"CWD ");
    break;
  case CAS_AND_PRECHARGE:
    sprintf(&buffer_string[buffer_len],"CAP ");
    break;
  case CAS_WRITE:
    sprintf(&buffer_string[buffer_len],"CAW ");
    break;
  case CAS_WRITE_AND_PRECHARGE:
    sprintf(&buffer_string[buffer_len],"CWP ");
    break;
  case RETIRE:
    sprintf(&buffer_string[buffer_len],"RET ");
    break;
  case PRECHARGE:
    sprintf(&buffer_string[buffer_len],"PRE ");
    break;
  case PRECHARGE_ALL:
    sprintf(&buffer_string[buffer_len],"PRA ");
    break;
  case RAS_ALL:
    sprintf(&buffer_string[buffer_len],"RSA ");
    break;
  case DATA:
    sprintf(&buffer_string[buffer_len],"DAT ");
    break;
  case DRIVE:
    sprintf(&buffer_string[buffer_len],"DRV ");
    break;
  case REFRESH:
    sprintf(&buffer_string[buffer_len],"REF ");
    break;
  case REFRESH_ALL:
    sprintf(&buffer_string[buffer_len],"RFA ");
    break;
  }
  buffer_len += 4;
  if(print_detail){
    sprintf(&buffer_string[buffer_len],"tid[%llu] ch[%d] ra[%d] ba[%d] ro[%X] co[%X] ref[%d] ",
	    this_c->tran_id,
	    this_c->chan_id,
	    this_c->rank_id,
	    this_c->bank_id,
	    this_c->row_id,
	    this_c->col_id,
		this_c->refresh);
    buffer_len = strlen(buffer_string);
  }
  switch(action){
  case MEM_STATE_SCHEDULED:
    sprintf(&buffer_string[buffer_len],"SCHEDULED : ");
    break;
  case XMIT2PROC:
    sprintf(&buffer_string[buffer_len],"XMIT2PROC : ");
    break;
  case PROC2DATA:
    sprintf(&buffer_string[buffer_len],"PROC2DATA : ");
    break;
  case MEM_STATE_COMPLETED:
    sprintf(&buffer_string[buffer_len],"COMPLETED : ");
    break;
  case XMIT2AMBPROC:
    sprintf(&buffer_string[buffer_len],"XMIT2AMBPROC : ");
    break;
  case AMBPROC2AMBCT:
    sprintf(&buffer_string[buffer_len],"AMBPROC2AMBCT : ");
    break;
  case AMBPROC2DATA:
    sprintf(&buffer_string[buffer_len],"AMBPROC2DATA : ");
    break;
  case AMBCT2PROC:
    sprintf(&buffer_string[buffer_len],"AMBCT2PROC : ");
    break;
  case AMBCT2DATA:
    sprintf(&buffer_string[buffer_len],"AMBCT2DATA : ");
  case DATA2AMBDOWN:
    sprintf(&buffer_string[buffer_len],"DATA2AMBDOWN : ");
    break;
  case AMBDOWN2DATA:
    sprintf(&buffer_string[buffer_len],"AMBDOWN2DATA : ");
    break;
  }
}

void print_dram_type( int dram_type, FILE *stream ) {
  fprintf(stream, "  dram_type: ");
  switch( dram_type ) {
    case SDRAM: fprintf(stream, "sdram\n");
      break;
    case DDRSDRAM: fprintf(stream, "ddrsdram\n");
      break;
    case DDR2: fprintf(stream, "ddr2\n");
      break;
    case DDR3: fprintf(stream, "ddr3\n");
      break;
    case FBD_DDR2: fprintf(stream, "fbd_ddr2\n");
      break;
    default: fprintf(stream, "unknown\n");
      break;
  }
}

void print_pa_mapping_policy( int papolicy, FILE *stream ) {
  fprintf(stream, "  physical address mapping_policy: ");
  switch( papolicy ) {
    case BURGER_BASE_MAP: fprintf(stream, "burger_base_map\n");
      break;
    case BURGER_ALT_MAP: fprintf(stream, "burger_alt_map\n");
      break;
    case SDRAM_BASE_MAP: fprintf(stream, "sdram_base_map\n");
      break;
    case SDRAM_HIPERF_MAP: fprintf(stream, "sdram_hiperf_map\n");
      break;
    case INTEL845G_MAP: fprintf(stream, "intel845g_map\n");
      break;
    case SDRAM_CLOSE_PAGE_MAP: fprintf(stream, "sdram_close_page_map\n");
      break;
    default: fprintf(stream, "unknown\n");
      break;
  }
}

void print_row_policy( int rowpolicy, FILE *stream ) {
  fprintf(stream, "  row_buffer_management_policy: ");
  switch( rowpolicy ) {
    case OPEN_PAGE: fprintf(stream, "open_page\n");
      break;
    case CLOSE_PAGE: fprintf(stream, "close_page\n");
      break;
    case PERFECT_PAGE: fprintf(stream, "perfect_page\n");
    default: fprintf(stream, "unknown\n");
      break;
  }
}

void print_refresh_policy( int refreshpolicy, FILE *stream ) {
  fprintf(stream, "  row_buffer_management_policy: ");
  switch( refreshpolicy ) {
    case REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK: fprintf(stream, "refresh_one_chan_all_rank_all_bank\n");
      break;
    case REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK: fprintf(stream, "refresh_one_chan_all_rank_one_bank\n");
      break;
    case REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK: fprintf(stream, "refresh_one_chan_one_rank_one_bank\n");
    default: fprintf(stream, "unknown\n");
      break;
  }
}

void print_trans_policy( int policy, FILE *stream ) {
  fprintf(stream, "  transaction_selection_policy: ");
  switch( policy ) {
    case WANG: fprintf(stream, "wang\n");
      break;
    case LEAST_PENDING: fprintf(stream, "least pending\n");
			   break;
    case MOST_PENDING: fprintf(stream, "most pending\n");
			   break;
    case OBF: fprintf(stream, "open bank first\n");
      break;
    case RIFF: fprintf(stream, "Read Instruction Fetch First\n");
      break;
    case FCFS: fprintf(stream, "First Come first Serve\n");
			   break;
    case GREEDY: fprintf(stream, "greedy\n");
			   break;
    default: fprintf(stream, "unknown\n");
      break;
  }
}
/* Dumps the DRAM configuration. */
void dram_dump_config(FILE *stream)
{
  fprintf(stream, "===================== DRAM config ======================\n");
  print_dram_type( dram_system.config.dram_type, stream );
  fprintf(stream, "  dram_frequency: %d MHz\n", dram_system.config.dram_type != FBD_DDR2 ? dram_system.config.memory_frequency : dram_system.config.dram_frequency );
  fprintf(stream, "  memory_frequency: %d MHz\n", dram_system.config.memory_frequency);
  fprintf(stream, "  cpu_frequency: %d MHz\n", get_cpu_frequency(global_biu));
  fprintf(stream, "  dram_clock_granularity: %d\n", dram_system.config.dram_clock_granularity);
  fprintf(stream, "  memfreq2cpufreq ration: %f \n", (float)global_biu->mem2cpu_clock_ratio);
  fprintf(stream, "  memfreq2dramfreq ration: %f \n", dram_system.config.memory2dram_freq_ratio);
  fprintf(stream, "  critical_word_first_flag: %d\n", dram_system.config.critical_word_first_flag);
  print_pa_mapping_policy( dram_system.config.physical_address_mapping_policy, stream);
  print_row_policy( dram_system.config.row_buffer_management_policy, stream);
  print_trans_policy( get_transaction_selection_policy(global_biu), stream);
  fprintf(stream, "  cacheline_size: %d\n", dram_system.config.cacheline_size);
  fprintf(stream, "  chan_count: %d\n", dram_system.config.channel_count);
  fprintf(stream, "  rank_count: %d\n", dram_system.config.rank_count);
  fprintf(stream, "  bank_count: %d\n", dram_system.config.bank_count);
  fprintf(stream, "  row_count: %d\n", dram_system.config.row_count);
  fprintf(stream, "  col_count: %d\n", dram_system.config.col_count);
  fprintf(stream, "  buffer count: %d\n", dram_system.config.up_buffer_cnt);
  fprintf(stream, "  t_rcd: %d\n", dram_system.config.t_rcd);
  fprintf(stream, "  t_cac: %d\n", dram_system.config.t_cac);
  fprintf(stream, "  t_cas: %d\n", dram_system.config.t_cas);
  fprintf(stream, "  t_ras: %d\n", dram_system.config.t_ras);
  fprintf(stream, "  t_rp: %d\n", dram_system.config.t_rp);
  fprintf(stream, "  t_cwd: %d\n", dram_system.config.t_cwd);
  fprintf(stream, "  t_rtr: %d\n", dram_system.config.t_rtr);
  fprintf(stream, "  t_burst: %d\n", dram_system.config.t_burst);
  fprintf(stream, "  t_rc: %d\n", dram_system.config.t_rc);
  fprintf(stream, "  t_rfc: %d\n", dram_system.config.t_rfc);
  fprintf(stream, "  t_al: %d\n", dram_system.config.t_al);
  fprintf(stream, "  t_wr: %d\n", dram_system.config.t_wr);
  fprintf(stream, "  t_rtp: %d\n", dram_system.config.t_rtp);
  fprintf(stream, "  t_dqs: %d\n", dram_system.config.t_dqs);

  // FBDIMM RELATED
  fprintf(stream, "  t_amb_up: %d\n", dram_system.config.t_amb_up);
  fprintf(stream, "  t_amb_down: %d\n", dram_system.config.t_amb_down);
  fprintf(stream, "  t_bundle: %d\n", dram_system.config.t_bundle);
  fprintf(stream, "  t_bus: %d\n", dram_system.config.t_bus);

  fprintf(stream, "  posted_cas: %d\n", dram_system.config.posted_cas_flag);
  fprintf(stream, "  row_command_duration: %d\n", dram_system.config.row_command_duration);
  fprintf(stream, "  col_command_duration: %d\n", dram_system.config.col_command_duration);

  // REFRESH POLICY
  fprintf(stream, "  auto_refresh_enabled: %d\n", dram_system.config.auto_refresh_enabled);
  fprintf(stream, "  auto_refresh_policy: %d\n", dram_system.config.refresh_policy);
  fprintf(stream, "  refresh_time: %f\n", dram_system.config.refresh_time);
  fprintf(stream, "  refresh_cycle_count: %d\n", dram_system.config.refresh_cycle_count);

  fprintf(stream, "  strict_ordering_flag: %d\n", dram_system.config.strict_ordering_flag);

  // DEBUG
  fprintf(stream, "  dram_debug: %d\n", dram_system.config.dram_debug);
  fprintf(stream, "  addr_debug: %d\n", dram_system.config.addr_debug);
  fprintf(stream, "  wave_debug: %d\n", dram_system.config.wave_debug);
  fprintf(stream, "========================================================\n\n\n");
}

void print_command_detail(tick_t now,
		   command_t *this_c){
  if(this_c == NULL){
    fprintf(stderr," Error-Null command \n");
    return;
  } else {
	print_command(now,this_c);
	fprintf(stdout,"LINKCT		%llu\n",this_c->link_comm_tran_comp_time);
	fprintf(stdout,"AMBPROC		%llu\n",this_c->amb_proc_comp_time);
	fprintf(stdout,"DIMMCT		%llu\n",this_c->dimm_comm_tran_comp_time);
	fprintf(stdout,"DRAMPROC	%llu\n",this_c->dram_proc_comp_time);
	fprintf(stdout,"DIMMDT		%llu\n",this_c->dimm_data_tran_comp_time);
	fprintf(stdout,"AMBDOWN     %llu\n",this_c->amb_down_proc_comp_time);
	fprintf(stdout,"LINKDT		%llu(%d)\n",this_c->link_data_tran_comp_time,this_c->data_word);
	fprintf(stdout,"Completion	%llu\n",this_c->completion_time);
  }
}


void print_rank_status(tick_t now,int rank_id,int chan_id) {
	rank_t *this_r = &dram_system.dram_controller[chan_id].rank[rank_id];
	int i;
	for (i=0;i<MAX_BANK_COUNT;i++) {
		fprintf(stdout,"[%llu] B[%d] ras[%llu] rp[%llu] rcd[%llu] t_wr[%llu] row[%llu]\n",now,i,
		   this_r->bank[i].ras_done_time,	
		   this_r->bank[i].rp_done_time,	
		   this_r->bank[i].rcd_done_time,	
		   this_r->bank[i].twr_done_time,	
		   this_r->bank[i].row_id);	
	}
}

void update_bus_link_status (int chan_id) {
  if (dram_system.config.dram_type == FBD_DDR2){
  if (dram_system.dram_controller[chan_id].up_bus.completion_time <= dram_system.current_dram_time) {
		dram_system.dram_controller[chan_id].up_bus.status = IDLE;
	}
  }else {
	if (dram_system.dram_controller[chan_id].command_bus.completion_time <= dram_system.current_dram_time) {
		dram_system.dram_controller[chan_id].command_bus.status = IDLE;
	}
  }
}

void print_bank_conflict_stats() {

  int i,j,k;
  uint64_t bank_hit = 0;
  uint64_t bank_conflict = 0;
  for (i=0;i<dram_system.config.channel_count;i++)
	for(j=0;j<dram_system.config.rank_count;j++)
	  for(k=0;k<dram_system.config.bank_count;k++) {
		fprintf(stdout,"CH[%d] RA[%d] BA[%d] hits[%llu] conflicts[%llu]\n",i,j,k,dram_system.dram_controller[i].rank[j].bank[k].bank_hit,
			dram_system.dram_controller[i].rank[j].bank[k].bank_conflict);
		bank_hit += dram_system.dram_controller[i].rank[j].bank[k].bank_hit;
		bank_conflict += dram_system.dram_controller[i].rank[j].bank[k].bank_conflict;
	  }
 fprintf(stdout,"Total Hits [%llu] Conflicts [%llu]\n",bank_hit,bank_conflict);
 }
