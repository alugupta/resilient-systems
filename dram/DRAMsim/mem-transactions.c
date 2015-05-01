/*
 * mem-transactions.c - Simulates manipulation of the transaction queue
 * 						and the transactions themselves
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

// Support for CAS With Drive -aj
int cas_with_drive = 1;

extern dram_system_t dram_system;
extern biu_t *global_biu;
/*** 
 * Function which returns the location of the transaction in the
 * transaction queue ... Legacy code!!
 * ***/
void update_trans_pending_queue(int transaction_index,int chan_id,int location) ;
void resort_trans_pending_queue(int chan_id,int updated_entry) ;
void add_trans_pending_queue(int chan_id,int transaction_index);
void del_trans_pending_queue(int chan_id,int transaction_index);

int get_transaction_index(int chan_id,uint64_t transaction_id){
	int i;
	for (i=0;i<dram_system.dram_controller[chan_id].transaction_queue.transaction_count; i++) {
		if (dram_system.dram_controller[chan_id].transaction_queue.entry[i].transaction_id == transaction_id)
			return i;
	}
	return MEM_STATE_INVALID;
}

void schedule_new_transaction(int chan_id) {
  int transaction_index;
  int next_slot_id;
  /* Then if the command bus(ses) are still open and idle, and something *could* be scheduled,
   * but isn't, then we can look to create a new transaction by looking in the biu for pending requests
   * FBDIMM : You check the up_bus 
   * If Refresh enabled you check if you have a refresh waiting ... */
  if(dram_system.dram_controller[chan_id].transaction_queue.transaction_count >= (dram_system.config.max_tq_size)){
	if (get_transaction_debug())
	  fprintf(stdout,"[%llu] Transaction queue [%d] full. Cannot start new transaction.\n",dram_system.current_dram_time,chan_id);
	return ;
  } 

  if((next_slot_id = get_next_request_from_biu(global_biu)) != MEM_STATE_INVALID){		
	/* If command bus is idle, see if there is another request in BIU that needs to be serviced. 
	 * We start by finding the request we want to service.
	 * Specifically, we want the slot_id of the request
	 * and either move it from VALID to MEM_STATE_SCHEDULED  or from MEM_STATE_SCHEDULED to COMPLETED
	 */
	transaction_index = add_transaction(dram_system.current_dram_time, 
		get_access_type(global_biu, next_slot_id),
		next_slot_id,&chan_id);
	if(transaction_index != MEM_STATE_INVALID){
	  set_last_transaction_type(global_biu, get_access_type(global_biu, next_slot_id));
	  set_biu_slot_status(global_biu, next_slot_id, MEM_STATE_SCHEDULED);
	  if(get_transaction_debug()){
		fprintf(stdout,"Start @ [%llu]: ",dram_system.current_dram_time);
		print_transaction(dram_system.current_dram_time,chan_id,dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].transaction_id);
	  }
	} else if (get_transaction_debug()){
	  fprintf(stdout,"Transaction queue full. Cannot start new transaction.\n");
	}
  }
}


/*
 * This function takes whatever and adds a transactions to the transaction list.  
 */

long long rank_distrib[8][8];
long long tot_reqs;
int get_thread_set(int thread_id) {
	return dram_system.config.thread_set_map[thread_id];
}

int add_transaction(	tick_t now, int transaction_type, int slot_id,int *chan_id){
	transaction_t   *this_t = NULL;
	uint64_t    address = 0;
	addresses_t     this_a; 
	int		transaction_index;
	int		thread_id;
	thread_id = get_thread_id(global_biu,slot_id);

		address 			= get_physical_address(global_biu, slot_id);
		this_a.physical_address 	= address;
		// We need to setup an option in case address is already broken down!
		if (convert_address(dram_system.config.physical_address_mapping_policy, &dram_system.config,&(this_a)) == ADDRESS_MAPPING_FAILURE) {
		  /* ignore address mapping failure for now... */
	  }
//	  if(thread_id != -1){
	  if(thread_id != -1 ){
		int scramble_id = get_thread_set(thread_id);
		this_a.chan_id     = (this_a.chan_id + scramble_id) % dram_system.config.channel_count;
		this_a.rank_id     = (this_a.rank_id + scramble_id) % dram_system.config.rank_count;
		this_a.bank_id     = (this_a.bank_id + scramble_id) % dram_system.config.bank_count;
		this_a.row_id      = (this_a.row_id + scramble_id) % dram_system.config.row_count;
	  }

	*chan_id = this_a.chan_id;
	if(dram_system.dram_controller[this_a.chan_id].transaction_queue.transaction_count >= (dram_system.config.max_tq_size)){
	  return MEM_STATE_INVALID;
	} 
	transaction_index = dram_system.dram_controller[this_a.chan_id].transaction_queue.transaction_count;
	if (dram_system.dram_controller[this_a.chan_id].transaction_queue.transaction_count <= 0) {
	  dram_system.tq_info.last_transaction_retired_time = dram_system.current_dram_time;
	  //fprintf(stdout,"Reset retired time to %llu\n",dram_system.tq_info.last_transaction_retired_time);
	}
	this_a.thread_id = thread_id;
	this_t = &(dram_system.dram_controller[this_a.chan_id].transaction_queue.entry[transaction_index]);	/* in order queue. Grab next entry */
	
	this_t->status 			= MEM_STATE_VALID;
	this_t->arrival_time 		= now;
	this_t->completion_time 	= 0;
	this_t->transaction_type 	= transaction_type;
	this_t->transaction_id 	= dram_system.tq_info.transaction_id_ctr++;
	this_t->slot_id			= slot_id;
	this_t->critical_word_ready	= FALSE;
	this_t->critical_word_available	= FALSE;
	this_t->critical_word_ready_time= 0;
	this_t->issued_data = false;
	this_t->issued_command = false;
	this_t->issued_cas = false;
	this_t->issued_ras = false;
	this_t->tindex = transaction_index;


	// Update Address
	this_t->address.physical_address= address;
	this_t->address.chan_id= this_a.chan_id;
	this_t->address.bank_id= this_a.bank_id;
	this_t->address.rank_id= this_a.rank_id;
	this_t->address.row_id= this_a.row_id;
	  /* STATS FOR RANK & CHANNEL */
	  rank_distrib[ this_a.chan_id ][ this_a.rank_id ]++;
	  tot_reqs++;

	  /* */
		
	mem_gather_stat(GATHER_REQUEST_LOCALITY_STAT, (int)address);		/* send this address to the stat collection */

	if(get_transaction_debug()){
		print_addresses(&(this_a));
	}
	/*
	//Ohm--stat for dram_access
	dram_system.dram_controller[this_a.chan_id].rank[this_a.rank_id].r_p_info.dram_access++;
	dram_system.dram_controller[this_a.chan_id].rank[this_a.rank_id].r_p_gblinfo.dram_access++;
	*/
	this_t->next_c = transaction2commands(now,
			this_t->transaction_id,
			transaction_type,
			&(this_a)); 
	this_t->status = MEM_STATE_SCHEDULED;
	dram_system.dram_controller[this_a.chan_id].transaction_queue.transaction_count++;

	/* If the transaction selection policy is MOST/LEAST than you sort it */
	int trans_sel_policy = get_transaction_selection_policy(global_biu);
	if (trans_sel_policy == MOST_PENDING || trans_sel_policy == LEAST_PENDING) {
	  add_trans_pending_queue(this_a.chan_id,transaction_index);
	}
	
	if (transaction_type == MEMORY_WRITE_COMMAND) {
	  if (dram_system.dram_controller[this_t->address.chan_id].active_write_trans++ > 4) 
		dram_system.dram_controller[this_t->address.chan_id].active_write_flag = true;
	}
	return transaction_index;
}

void remove_transaction(int chan_id,int tindex){
	transaction_t   *this_t;
	command_t	*this_c;
	int		i;
	int		victim_index;


	if(get_transaction_debug()){ 
	  if ( dram_system.dram_controller[chan_id].transaction_queue.entry[tindex].transaction_type != MEMORY_WRITE_COMMAND) {
         fprintf(stdout,"[%llu]Removing transaction %llu at [%d] Time in Queue %llu Memory Latency %llu\n",
             dram_system.current_dram_time,
                             dram_system.dram_controller[chan_id].transaction_queue.entry[tindex].transaction_id,
                                             tindex, dram_system.current_dram_time - dram_system.dram_controller[chan_id].transaction_queue.entry[tindex].arrival_time,
                                                              dram_system.dram_controller[chan_id].transaction_queue.entry[tindex].critical_word_ready_time -dram_system.dram_controller[chan_id].transaction_queue.entry[tindex].arrival_time );
               } else {
                             fprintf(stdout,"[%llu] Removing transaction %llu at [%d] Time in Queue %llu \n",
             dram_system.current_dram_time,
                                                 dram_system.dram_controller[chan_id].transaction_queue.entry[tindex].transaction_id,
                                                                 tindex, dram_system.current_dram_time - dram_system.dram_controller[chan_id].transaction_queue.entry[tindex].arrival_time );



	  }
	}
	dram_system.tq_info.last_transaction_retired_time = dram_system.current_dram_time;
	this_t =  &(dram_system.dram_controller[chan_id].transaction_queue.entry[tindex]);
	this_c = this_t->next_c;
	while(this_c->next_c != NULL){
		this_c = this_c->next_c;
	}
	this_c->next_c = dram_system.tq_info.free_command_pool;
	dram_system.tq_info.free_command_pool = this_t->next_c;
	this_t ->next_c = NULL;

	for(i = tindex; i < (dram_system.dram_controller[chan_id].transaction_queue.transaction_count - 1) ; i++){
		dram_system.dram_controller[chan_id].transaction_queue.entry[i] 			= dram_system.dram_controller[chan_id].transaction_queue.entry[i+1];
		dram_system.dram_controller[chan_id].transaction_queue.entry[i].tindex 	= i;
	}
	dram_system.dram_controller[chan_id].transaction_queue.transaction_count--;
	victim_index = dram_system.dram_controller[chan_id].transaction_queue.transaction_count;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].status 			= MEM_STATE_INVALID;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].arrival_time 			= 0;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].completion_time 		= 0;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].transaction_type 		= MEM_STATE_INVALID;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].transaction_id 		= 0;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].slot_id = MEM_STATE_INVALID;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].address.chan_id       = MEM_STATE_INVALID;
    dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].address.physical_address      = 0;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].critical_word_ready		= FALSE;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].critical_word_available		= FALSE;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].critical_word_ready_time	= 0;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].next_c 			= NULL;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].next_ptr 			= NULL;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].issued_data = false;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].issued_command = false;
	dram_system.dram_controller[chan_id].transaction_queue.entry[victim_index].issued_cas = false;

}



/* This function takes the translated address, inserts command sequences into a command queue 
   This is a little bit hairy, since we need to create command chains here that needs to check
   against the current state of the memory system as well as previously scheduled transactions
   that had not yet executed.
   */

command_t *transaction2commands(tick_t now,
		int tran_id,
		int transaction_type,
		addresses_t *this_a){
	//command_t *temp_c;
	command_t *command_queue;
	bank_t	*this_b;
	int bank_open;
	int bank_conflict;
	int chan_id,bank_id,row_id,rank_id;
	int i;
	int cas_command;
	int cmd_id = 0;

    command_queue = NULL;
    chan_id = this_a->chan_id;
    bank_id = this_a->bank_id;
    row_id = this_a->row_id;
    rank_id = this_a->rank_id;

    bank_open = FALSE;
    bank_conflict = FALSE;
    //bool bank_hit = FALSE;
    this_b = &(dram_system.dram_controller[chan_id].rank[rank_id].bank[bank_id]);

    /*if(this_b->status == ACTIVE) {
      if(this_b->row_id == row_id){
      bank_open = TRUE;
      } else {
      bank_conflict = TRUE;
      }
      } deprecated in newer version of memtest from dave 07/28/2004*/
    if(this_b->row_id == row_id){
      this_b->cas_count_since_ras++;
      if(this_b->status == ACTIVE) {
        //fprintf(stdout, " Bank opened by previous transaction \n");		  
        bank_open = TRUE;
        //bank_hit = TRUE;
      }
    } else {
      mem_gather_stat(GATHER_CAS_PER_RAS_STAT, this_b->cas_count_since_ras);
      this_b->cas_count_since_ras = 0;
      if(this_b->status == ACTIVE) {
        bank_conflict = TRUE;
      }
    }
    bank_conflict = TRUE;
    bank_open= TRUE;
    if(transaction_type == MEMORY_WRITE_COMMAND ){
      cas_command = CAS_WRITE;
    } else {
      if (dram_system.config.dram_type == FBD_DDR2 && cas_with_drive == TRUE) {
        cas_command = CAS_WITH_DRIVE;
      }else {
        cas_command = CAS;
      }
    }
    if((dram_system.config.row_buffer_management_policy == OPEN_PAGE)){ 
      /* current policy is open page, so we need to check if the row and bank address match */
      /* If they match, we only need a CAS.  If bank hit, and different row, then we need a */
      /* precharge then RAS. */

      /*** FBDIMM : For a CAS WRITE send Data commands
       * 			Number of Data commands = (cacheline / number of bytes
       * 			per outgoing packet
       * ****/	
      if (dram_system.config.dram_type == FBD_DDR2 && cas_command == CAS_WRITE) {
        for (i=0; i < dram_system.config.data_cmd_count; i++) {
          int dpos = (i == 0)? DATA_FIRST: (i == dram_system.config.data_cmd_count - 1) ? DATA_LAST : DATA_MIDDLE;
          command_queue = add_command(now,
              command_queue,
              DATA,
              this_a,
              tran_id,
              0,
              dpos, -1);
        }
      }


      if(bank_conflict == TRUE){  			/* or if we run into open bank limitations */
        command_queue = add_command(now,
            command_queue,
            PRECHARGE,
            this_a,
            tran_id,0,0,cmd_id++);

        command_queue = add_command(now,
            command_queue,
            RAS,
            this_a,
            tran_id,0,0,cmd_id++);

      } else if(bank_open == FALSE){ 			/* insert RAS */
        command_queue = add_command(now,
            command_queue,
            RAS,
            this_a,
            tran_id,0,0,cmd_id++);
      } else {
      }
      for(i=0;i<dram_system.config.packet_count;i++){ /* add CAS. How many do we add? */
        int dpos = 0;
        int dword = 0; 
        if (cas_command == CAS_WITH_DRIVE) {
          dpos = i == 0? DATA_FIRST: i == dram_system.config.data_cmd_count - 1 ? DATA_LAST : DATA_MIDDLE;

          dword = dram_system.config.cacheline_size/DATA_BYTES_PER_READ_BUNDLE;
        }

        command_queue = add_command(now,
            command_queue,
            cas_command,
            this_a,
            tran_id,
            dword,
            dpos, cmd_id++);
      }
      /*** FBDIMM: For a Drive command currently add one **/
      // If CAS With Drive, no need of DRIVE commands -aj (unless of course you wish to add more drives)
      if (dram_system.config.dram_type == FBD_DDR2 && cas_command == CAS) {
        command_queue = add_command(now,
            command_queue,
            DRIVE,
            this_a,
            tran_id,
            (dram_system.config.cacheline_size)/DATA_BYTES_PER_READ_BUNDLE,
            DATA_LAST,
            cmd_id++);
      }

    } else if ((dram_system.config.row_buffer_management_policy == CLOSE_PAGE)) {  

      /*** FBDIMM : For a CAS WRITE send Data commands
       * 			Number of Data commands = (cacheline / number of bytes
       * 			per outgoing packet
       * ****/	
      if (dram_system.config.dram_type == FBD_DDR2 && cas_command == CAS_WRITE) {
        for (i=0; i < dram_system.config.data_cmd_count; i++) {
          int dpos = i == 0? DATA_FIRST: i == dram_system.config.data_cmd_count - 1 ? DATA_LAST : DATA_MIDDLE;
          command_queue = add_command(now,
              command_queue,
              DATA,
              this_a,
              tran_id,1,dpos,-1); 
        }
      }

      /* We always need RAS, then CAS, then precharge */
      /* this needs to be fixed for the case that two consecutive accesses may hit the same page */
      command_queue = add_command(now,
          command_queue,
          RAS,
          this_a,
          tran_id,0,0,cmd_id++);
      for(i=0;i<dram_system.config.packet_count;i++){
        int dpos = 0;
        int dword = 0; 
        if (cas_command == CAS_WITH_DRIVE) {
          dpos = i == 0? DATA_FIRST: i == dram_system.config.data_cmd_count - 1 ? DATA_LAST : DATA_MIDDLE;
          dword = dram_system.config.cacheline_size/DATA_BYTES_PER_READ_BUNDLE;
        }
        if (dram_system.config.cas_with_prec) {
          if (transaction_type == MEMORY_WRITE_COMMAND) {
            command_queue = add_command(now,
                command_queue,
                CAS_WRITE_AND_PRECHARGE,
                this_a,
                tran_id,dword,dpos,cmd_id++);

          }else { // No support for cas with drive currently
            command_queue = add_command(now,
                command_queue,
                CAS_AND_PRECHARGE,
                this_a,
                tran_id,dword,dpos,cmd_id++);

          }
        }else {		  
          command_queue = add_command(now,
              command_queue,
              cas_command,
              this_a,
              tran_id,dword,dpos,cmd_id++);
        }
      }
      /*** FBDIMM: For a Drive command currently add one **/
      if (dram_system.config.dram_type == FBD_DDR2 && cas_command == CAS) {
        command_queue = add_command(now,
            command_queue,
            DRIVE,
            this_a,
            tran_id,
            (dram_system.config.cacheline_size/DATA_BYTES_PER_READ_BUNDLE),
            DATA_LAST,
            cmd_id++);
      }

      if (!dram_system.config.cas_with_prec) {
        command_queue = add_command(now,
            command_queue,
            PRECHARGE,
            this_a,
            tran_id,0,0,cmd_id++);
      }
    } else if (dram_system.config.row_buffer_management_policy == PERFECT_PAGE) {
      /* "Perfect" buffer management policy only need CAS.  */
      /* suppress cross checking issue, just use CAS for this policy */  	
      for(i=0;i<dram_system.config.packet_count;i++){
        int dpos = 0;
        int dword = 0; 
        if (cas_command == CAS_WITH_DRIVE) {
          dpos = i == 0? DATA_FIRST: i == dram_system.config.data_cmd_count - 1 ? DATA_LAST : DATA_MIDDLE;
          dword = dram_system.config.cacheline_size/DATA_BYTES_PER_READ_BUNDLE;
        }
        command_queue = add_command(now,
            command_queue,
            cas_command,
            this_a,
            tran_id,dword,dpos,cmd_id++);
      }
    } else {
      fprintf(stderr,"I am confused. Unknown buffer mangement policy %d\n ",
          dram_system.config.row_buffer_management_policy);
    }
    return command_queue;
}


void retire_transactions() {

	/* retire the transaction, and return the status to the bus queue slot */
	/* The out of order completion of the transaction is possible because the later transaction may
	   hit an open page, where as the earlier transaction may be delayed by a PRECHARGE + RAS or just RAS.
	   Check to see if transaction ordering is strictly enforced.

	   This option is current disabled as of 9/20/2002.  Problem with Mase generation non-unique req_id's.
	   out of order data return creates havoc there. This should be investigated and fixed.
	   */
	int i;
    transaction_t * this_t = NULL;
	int chan_id;
	if(dram_system.config.strict_ordering_flag == TRUE){ 	/* if strict ordering is set, check only transaction #0 */
	  for (chan_id=0;chan_id < dram_system.config.channel_count;chan_id++) {
		this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[0];
		chan_id = this_t->next_c->chan_id;
		if((this_t->status == MEM_STATE_COMPLETED) &&
			(this_t->completion_time <= dram_system.current_dram_time)){
			/* If this is a refresh or auto precharge transaction, just ignore it. Else... */
			set_critical_word_ready(global_biu, this_t->slot_id);
			set_biu_slot_status(global_biu, this_t->slot_id, MEM_STATE_COMPLETED);
		  int trans_sel_policy = get_transaction_selection_policy(global_biu);
		  if (trans_sel_policy == MOST_PENDING || trans_sel_policy == LEAST_PENDING) 
			del_trans_pending_queue(chan_id,0);

		  /*---------------------small stat counter----------------------*/
		  switch (this_t->transaction_type) {
			case (MEMORY_IFETCH_COMMAND):
			  dram_system.tq_info.num_ifetch_tran++;
			  break;
			case (MEMORY_READ_COMMAND):
			  dram_system.tq_info.num_read_tran++;
			  break;
			case (MEMORY_PREFETCH):
			  dram_system.tq_info.num_prefetch_tran++;
			  break;
			case (MEMORY_WRITE_COMMAND):
			  dram_system.tq_info.num_write_tran++;
			  break;
			default:
			  fprintf(stderr, "unknown transaction type %d\n",this_t->transaction_type);
			  break;
		  }
		  /*---------------------end small stat counter----------------------*/
		  // Do the update on bank conflict count and bank hit count
		  if (!this_t->issued_ras && this_t->issued_cas) {
			dram_system.dram_controller[chan_id].rank[this_t->address.rank_id].bank[this_t->address.bank_id].bank_hit++;
		  } else if (this_t->issued_ras && this_t->issued_cas) {
			dram_system.dram_controller[chan_id].rank[this_t->address.rank_id].bank[this_t->address.bank_id].bank_conflict++;
		  }
		  remove_transaction(chan_id,0);
		  //if (dram_system.config.row_buffer_management_policy == OPEN_PAGE)
			//update_tran_for_refresh(chan_id);
		} else if((this_t->critical_word_ready == TRUE) &&
			(this_t->critical_word_ready_time <= dram_system.current_dram_time)){
		  set_critical_word_ready(global_biu, this_t->slot_id);
		}
	  }
	} else {
	  for (chan_id=0;chan_id < dram_system.config.channel_count;chan_id++) {
		for(i=0;i<dram_system.dram_controller[chan_id].transaction_queue.transaction_count;i++){
	       this_t = &dram_system.dram_controller[chan_id].transaction_queue.entry[i];
	   		chan_id = this_t->next_c->chan_id;
			if((this_t->status == MEM_STATE_COMPLETED) && 
					(this_t->completion_time <= dram_system.current_dram_time)){
              if((get_biu_slot_status(global_biu, this_t->slot_id) != MEM_STATE_COMPLETED)){		
                /* If this is a refresh or auto precharge transaction, just ignore it. Else... */

                set_critical_word_ready(global_biu, this_t->slot_id);
                set_biu_slot_status(global_biu, this_t->slot_id, MEM_STATE_COMPLETED);
				}
				int trans_sel_policy = get_transaction_selection_policy(global_biu);
				if (trans_sel_policy == MOST_PENDING || trans_sel_policy == LEAST_PENDING) 
				  del_trans_pending_queue(chan_id,i);

				/*---------------------small stat counter----------------------*/
					switch (this_t->transaction_type) {
						case (MEMORY_IFETCH_COMMAND):
							dram_system.tq_info.num_ifetch_tran++;
							break;
						case (MEMORY_READ_COMMAND):
							dram_system.tq_info.num_read_tran++;
							break;
						case (MEMORY_PREFETCH):
							dram_system.tq_info.num_prefetch_tran++;
							break;
						case (MEMORY_WRITE_COMMAND):
							dram_system.tq_info.num_write_tran++;
							break;
						default:
							fprintf(stderr, "unknown transaction type %d ID %d\n",this_t->transaction_type,
								this_t->transaction_id);
							break;
					}
				/*---------------------end small stat counter----------------------*/
		  
			// Do the update on bank conflict count and bank hit count
		  if (!this_t->issued_ras && this_t->issued_cas) {
			dram_system.dram_controller[chan_id].rank[this_t->address.rank_id].bank[this_t->address.bank_id].bank_hit++;
		  } else if (this_t->issued_ras && this_t->issued_cas) {
			dram_system.dram_controller[chan_id].rank[this_t->address.rank_id].bank[this_t->address.bank_id].bank_conflict++;
		  }
				remove_transaction(chan_id,i);

			} else if((this_t->critical_word_ready == TRUE) &&
				(this_t->critical_word_ready_time <= dram_system.current_dram_time)){
			  set_critical_word_ready(global_biu, this_t->slot_id);
			}
		}
	  }
	}
}

void gather_tran_stats() {
  int chan_id;
  int total_trans = 0;
  for (chan_id=0;chan_id<dram_system.config.channel_count;chan_id++) {
	total_trans += dram_system.dram_controller[chan_id].transaction_queue.transaction_count;
  }
	mem_gather_stat(GATHER_TRAN_QUEUE_VALID_STAT, total_trans);
}

void  collapse_cmd_queue(transaction_t * this_t,command_t *cmd_issued){
  /* 
   * We issued a CAS/CASW but has an non-issued RAS/PRE at front - get rid of
   * it
   */
  command_t * temp_c;
  // Get rid of precharge
  command_t * prev_c = NULL;
  temp_c = this_t->next_c;
  while (temp_c != NULL && temp_c->cmd_id != cmd_issued->cmd_id) {
	if (temp_c->status == IN_QUEUE && temp_c->command != DATA) {
	  if (prev_c == NULL) { // First chappie in the queuer
		  this_t->next_c= temp_c->next_c;
		  // prev_c still NULL
	  } else { // Deleting Middle Item
		prev_c->next_c = temp_c->next_c;
		// prev_c non null
	  }
  	 temp_c->next_c = dram_system.tq_info.free_command_pool;
	 dram_system.tq_info.free_command_pool = temp_c;
	  if (prev_c == NULL) { // First chappie in the queue
		temp_c = this_t->next_c;
	  }else {
		temp_c = prev_c->next_c;
	  }
	}else {
	  prev_c = temp_c;
	  temp_c = temp_c->next_c;
	}
  }

}

void add_trans_pending_queue(int chan_id,int transaction_index){
//  int chan_id = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].address.chan_id;
  int rank_id = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].address.rank_id;
  int bank_id = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].address.bank_id;
  int row_id = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].address.row_id;
	dram_controller_t * this_dc = &dram_system.dram_controller[chan_id];
  /* Walk through pending queue -> if you get a match -> update and sort queue 
   * Else add at the end 
   */
  int i;

  for (i=0;i<this_dc->pending_queue.queue_size;i++) {
	if (this_dc->pending_queue.pq_entry[i].rank_id == rank_id && 
		this_dc->pending_queue.pq_entry[i].bank_id == bank_id &&
		this_dc->pending_queue.pq_entry[i].row_id == row_id) {
	  	update_trans_pending_queue(transaction_index,chan_id,i);
		resort_trans_pending_queue(chan_id,i);
		break;
	}
  }
  if (i>=this_dc->pending_queue.queue_size) {
	// Add at the end
	this_dc->pending_queue.pq_entry[i].rank_id = rank_id;
	this_dc->pending_queue.pq_entry[i].bank_id = bank_id;
	this_dc->pending_queue.pq_entry[i].row_id = row_id;
	  	update_trans_pending_queue(transaction_index,chan_id,i);
		this_dc->pending_queue.queue_size++;
  }

}

void update_trans_pending_queue(int transaction_index,int chan_id,int location) {
    
	pending_info_t * this_entry = &dram_system.dram_controller[chan_id].pending_queue.pq_entry[location];
	  
		/* Add after the head */
	  this_entry->transaction_list[this_entry->transaction_count] = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].transaction_id;
	  this_entry->transaction_count++;
}


void resort_trans_pending_queue(int chan_id,int updated_entry) {
	pending_queue_t * pqueue_ptr = &dram_system.dram_controller[chan_id].pending_queue;
	int i;
	pending_info_t entry = pqueue_ptr->pq_entry[updated_entry];
	if (updated_entry+1 < pqueue_ptr->queue_size && pqueue_ptr->pq_entry[updated_entry+1].transaction_count > entry.transaction_count) {
	  i = updated_entry + 1;
	  while (i< pqueue_ptr->queue_size && pqueue_ptr->pq_entry[i].transaction_count > entry.transaction_count) {
		pqueue_ptr->pq_entry[i-1] = pqueue_ptr->pq_entry[i];
		i++;
	  }
	   		
		pqueue_ptr->pq_entry[i-1] = entry;
	}else if (updated_entry -1 >=0 && pqueue_ptr->pq_entry[updated_entry - 1].transaction_count < entry.transaction_count) {
	  i = updated_entry - 1;
	  while (i>= 0 && pqueue_ptr->pq_entry[i].transaction_count < entry.transaction_count) {
		pqueue_ptr->pq_entry[i+1] = pqueue_ptr->pq_entry[i];
		i--;
	  }

	  pqueue_ptr->pq_entry[i+1] = entry;

	}
	
}

void del_trans_pending_queue(int chan_id,int transaction_index){
  //int chan_id = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].address.chan_id;
  int rank_id = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].address.rank_id;
  int bank_id = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].address.bank_id;
  int row_id = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].address.row_id;
  int tran_id = dram_system.dram_controller[chan_id].transaction_queue.entry[transaction_index].transaction_id;
	dram_controller_t * this_dc = &dram_system.dram_controller[chan_id];
  /* Walk through pending queue -> if you get a match -> update and sort queue 
   * Else add at the end 
   */
  int i;
  int tran_count;
  for (i=0;i<this_dc->pending_queue.queue_size;i++) {
	if (this_dc->pending_queue.pq_entry[i].rank_id == rank_id && 
		this_dc->pending_queue.pq_entry[i].bank_id == bank_id &&
		this_dc->pending_queue.pq_entry[i].row_id == row_id) {
	    int j,k;
		for (j=0;
			j<this_dc->pending_queue.pq_entry[i].transaction_count && this_dc->pending_queue.pq_entry[i].transaction_list[j] != tran_id;j++);
		for (k=j;k<this_dc->pending_queue.pq_entry[i].transaction_count-1;k++) 
		  this_dc->pending_queue.pq_entry[i].transaction_list[k] = this_dc->pending_queue.pq_entry[i].transaction_list[k+1];

	  	this_dc->pending_queue.pq_entry[i].transaction_count--;
		// Needed to keep track of whether queue size needs to be reduced
		tran_count = 	this_dc->pending_queue.pq_entry[i].transaction_count;
		
		resort_trans_pending_queue(chan_id,i);
		// Reduce queue size only after resorting of queue to prevent loss of
		// last elements
		if (tran_count == 0) 
		  this_dc->pending_queue.queue_size--;
		break;
	}
  }


}
