/*
 * mem-stat.c - auxillary stat collection for memory system.  
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

#include "mem-system.h"
#include "mem-biu.h"
#include "math.h"
aux_stat_t aux_stat;
/* statistics gathering functions */


void mem_gather_stat_init(int stat_type, int page_size, int range ){
  int i;
  if(stat_type == GATHER_TRAN_QUEUE_VALID_STAT) {
	aux_stat.tran_queue_valid_stats = (int *)calloc(get_dram_channel_count()*MAX_TRANSACTION_QUEUE_DEPTH + 1,sizeof(int));
  } else if(stat_type == GATHER_BUNDLE_STAT) {
	aux_stat.bundle_stats = (int *)calloc(BUNDLE_SIZE + 1,sizeof(int));
  } else if(stat_type == GATHER_BIU_SLOT_VALID_STAT) {
//	fprintf(stderr,"Allocating biu_slot_valid_stats\n");	
  	aux_stat.biu_slot_valid_stats = (int *)calloc(MAX_BUS_QUEUE_DEPTH ,sizeof(int));
	if (aux_stat.biu_slot_valid_stats == NULL) 
	  fprintf(stderr,"Failed To allocate biu_slot_valid_stats \n");
  } else if(stat_type == GATHER_BUS_STAT){ 
      aux_stat.bus_stats = (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
      aux_stat.bus_stats->next_b = NULL;
  } else if(stat_type == GATHER_BANK_HIT_STAT){
      aux_stat.bank_hit_stats = (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
      aux_stat.bank_hit_stats->next_b = NULL;
  } else if(stat_type == GATHER_BANK_CONFLICT_STAT){
      aux_stat.bank_conflict_stats = (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
      aux_stat.bank_conflict_stats->next_b = NULL;
  } else if(stat_type == GATHER_CAS_PER_RAS_STAT){
      aux_stat.cas_per_ras_stats = (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
      aux_stat.cas_per_ras_stats->next_b = NULL;
  } else if (stat_type == GATHER_REQUEST_LOCALITY_STAT){
	  /*aux_stat.page_size = log2(page_size);*/	/* convert page size to power_of_2 format */
	  aux_stat.page_size = -9999;
	  aux_stat.locality_range = MIN(range + 1,CMS_MAX_LOCALITY_RANGE);
	  aux_stat.valid_entries = 0;
	  for(i = 0; i< CMS_MAX_LOCALITY_RANGE;i++){
		aux_stat.locality_hit[i] = 0;
		aux_stat.previous_address_page[i] = 0;
	  }
  }else if(stat_type == GATHER_BIU_ACCESS_DIST_STAT) {
	  aux_stat.biu_access_dist_stats= (DRAM_STATS *)calloc(1,sizeof(DRAM_STATS));
	  aux_stat.biu_access_dist_stats->next_b = NULL;
  }
}

/* We set up the common file for output */
void mem_stat_init_common_file(char *filename) {
  if (filename != NULL) {
	aux_stat.stats_fileptr = fopen(filename,"w");
	if (aux_stat.stats_fileptr == NULL) {
	  fprintf(stderr,"Error opening stats common output file %s\n",filename);
		exit(0);
	}
  }
}


/* simply add an accumulation of the bus latency statistic */

void mem_gather_stat(int stat_type, int delta_stat){			/* arrange smallest to largest */
  int found;
  STAT_BUCKET 	*this_b;
  STAT_BUCKET 	*temp_b;
  int i;
  unsigned int current_address;
  unsigned int current_page_index;
  if ((stat_type == GATHER_TRAN_QUEUE_VALID_STAT) && (aux_stat.tran_queue_valid_stats != NULL)) {
	aux_stat.tran_queue_valid_stats[delta_stat]++;
	return;
  }else if((stat_type == GATHER_BIU_SLOT_VALID_STAT) && (aux_stat.biu_slot_valid_stats != NULL)) {
	aux_stat.biu_slot_valid_stats[delta_stat]++;
	return;
  } else if((stat_type == GATHER_BUNDLE_STAT) && (aux_stat.bundle_stats != NULL)) {
     aux_stat.bundle_stats[delta_stat]++;
	return;
  }
  if(delta_stat <= 0){	/* squashed biu entries can return erroneous negative numbers, ignore. Also ignore zeros. */
    return;
  } else if((stat_type == GATHER_BIU_ACCESS_DIST_STAT ) && (aux_stat.biu_access_dist_stats != NULL)) {
    this_b = aux_stat.biu_access_dist_stats->next_b;
  } else if((stat_type == GATHER_BUS_STAT) && (aux_stat.bus_stats != NULL)){
    this_b = aux_stat.bus_stats->next_b;
  } else if((stat_type == GATHER_BANK_HIT_STAT) && (aux_stat.bank_hit_stats != NULL)){
    this_b = aux_stat.bank_hit_stats->next_b;
  } else if((stat_type == GATHER_BANK_CONFLICT_STAT) && (aux_stat.bank_conflict_stats != NULL)){
    this_b = aux_stat.bank_conflict_stats->next_b;
  } else if((stat_type == GATHER_CAS_PER_RAS_STAT) && (aux_stat.cas_per_ras_stats != NULL)){
    this_b = aux_stat.cas_per_ras_stats->next_b;
  } else if((stat_type == GATHER_REQUEST_LOCALITY_STAT) ){
    current_address = (unsigned int) delta_stat;
    current_page_index = current_address >> aux_stat.page_size;  /* page size already in power_of_2 format */
    for(i = (aux_stat.locality_range-1);i>0;i--){
      aux_stat.previous_address_page[i] = aux_stat.previous_address_page[i-1];
      if(aux_stat.previous_address_page[i] == current_page_index){
	aux_stat.locality_hit[i]++;
      }
    }
    aux_stat.valid_entries++;
    aux_stat.previous_address_page[0] = current_page_index;
    return;
  } else {
    return;
  }
  found = FALSE;
  if((this_b == NULL) || (this_b->delta_stat > delta_stat)){     	/* Nothing in this type of */
                                                        		/* transaction. Add a bucket */
    temp_b = (STAT_BUCKET *)calloc(1,sizeof(STAT_BUCKET));
    temp_b -> next_b = this_b;
    temp_b -> count = 1;
    temp_b -> delta_stat = delta_stat;
    if(stat_type == GATHER_BUS_STAT){
      aux_stat.bus_stats->next_b = temp_b;
    } else if (stat_type == GATHER_BANK_HIT_STAT) {
      aux_stat.bank_hit_stats->next_b = temp_b;
    } else if (stat_type == GATHER_BANK_CONFLICT_STAT) {
      aux_stat.bank_conflict_stats->next_b = temp_b;
    } else if (stat_type == GATHER_CAS_PER_RAS_STAT) {
      aux_stat.cas_per_ras_stats->next_b = temp_b;
    } else if (stat_type == GATHER_BIU_ACCESS_DIST_STAT) {
      aux_stat.biu_access_dist_stats->next_b = temp_b;
    }
    found = TRUE;
  } else if(this_b->delta_stat == delta_stat) {		/* add a count to this bucket */
    this_b->count++;
    found = TRUE;
  }
  while(!found && (this_b -> next_b) != NULL){
    if(this_b->next_b->delta_stat == delta_stat) {
      this_b->next_b->count++;
      found = TRUE;
    } else if(this_b->next_b->delta_stat > delta_stat){
      temp_b = (STAT_BUCKET *)calloc(1,sizeof(STAT_BUCKET));
      temp_b -> count = 1;
      temp_b -> delta_stat = delta_stat;
      temp_b -> next_b= this_b->next_b;
      this_b -> next_b= temp_b;
      found = TRUE;
    } else {
      this_b = this_b -> next_b;
    }
  }
  if(!found){                     /* add a new bucket to the end */
    this_b->next_b= (STAT_BUCKET *)calloc(1,sizeof(STAT_BUCKET));
    this_b = this_b->next_b;
    this_b ->next_b = NULL;
    this_b ->count 	= 1;
    this_b ->delta_stat= delta_stat;
  }
}

void free_stats(STAT_BUCKET *start_b){
  STAT_BUCKET     *next_b;
  STAT_BUCKET     *this_b;
  this_b = start_b;
  while(this_b != NULL){
    next_b = this_b -> next_b;
    free(this_b);
    this_b = next_b;
  }
}

void mem_print_stat(int stat_type,bool common_output){
  STAT_BUCKET 	*this_b = NULL;
  char		delta_stat_string[1024];
  FILE		*fileout;
  int		i;
   fileout = aux_stat.stats_fileptr;
   if (fileout == NULL)
	 fileout = stderr;

  if((stat_type == GATHER_BUS_STAT) && (aux_stat.bus_stats != NULL)){
    this_b = aux_stat.bus_stats->next_b;
    sprintf(&delta_stat_string[0],"\tLatency");
     fprintf(fileout,"\n\nLatency Stats: \n");
  } else if((stat_type == GATHER_TRAN_QUEUE_VALID_STAT) && (aux_stat.tran_queue_valid_stats != NULL)){
     fprintf(fileout,"\n\nTransaction Queue Stats: \n");
    sprintf(&delta_stat_string[0],"\tTot Valid Transactions");
	for (i=0;i<get_dram_channel_count()*MAX_TRANSACTION_QUEUE_DEPTH + 1;i++) {
    	fprintf(fileout,"%s %8d %6d\n",delta_stat_string,i,aux_stat.tran_queue_valid_stats[i]);
	}
	return;
  } else if((stat_type == GATHER_BIU_SLOT_VALID_STAT) && (aux_stat.biu_slot_valid_stats != NULL)){
    fprintf(fileout,"\n\nBIU Stats: \n");
    sprintf(&delta_stat_string[0],"\tTot Valid BIU Slots");
	for (i=0;i<get_biu_queue_depth(global_biu);i++) {
    	fprintf(fileout,"%s %8d %6d\n",delta_stat_string,i,aux_stat.biu_slot_valid_stats[i]);
	}
	return;
  } else if((stat_type == GATHER_BUNDLE_STAT) && (aux_stat.bundle_stats != NULL)){
      fprintf(fileout,"\n\nBundle Stats: \n");
    sprintf(&delta_stat_string[0],"\tBundle Size");
	for (i=0;i<BUNDLE_SIZE + 1;i++) {
    	fprintf(fileout,"%s %8d %6d\n",delta_stat_string,i,aux_stat.bundle_stats[i]);
	}
	return;
  } else if((stat_type == GATHER_BIU_ACCESS_DIST_STAT) && (aux_stat.biu_access_dist_stats != NULL)){
    this_b = aux_stat.biu_access_dist_stats->next_b;
    fprintf(fileout,"\n\nBIU Access Dist Stats: \n");
    sprintf(&delta_stat_string[0],"\tBIU Access Distance");
  } else if((stat_type == GATHER_BANK_HIT_STAT) && (aux_stat.bank_hit_stats != NULL)){
    this_b = aux_stat.bank_hit_stats->next_b;
     fprintf(fileout,"\n\nBank Hit Stats: \n");
    sprintf(&delta_stat_string[0],"Hit");
  } else if((stat_type == GATHER_BANK_CONFLICT_STAT) && (aux_stat.bank_conflict_stats != NULL)){
    this_b = aux_stat.bank_conflict_stats->next_b;
     fprintf(fileout,"\n\nBank Conflict Stats: \n");    
    sprintf(&delta_stat_string[0],"Conflict");
  } else if((stat_type == GATHER_CAS_PER_RAS_STAT) && (aux_stat.cas_per_ras_stats != NULL)){
    this_b = aux_stat.cas_per_ras_stats->next_b;
    fprintf(fileout,"\n\nCas Per Ras Stats: \n");
    sprintf(&delta_stat_string[0],"Hits");
  } else if((stat_type == GATHER_REQUEST_LOCALITY_STAT)){
     fprintf(fileout,"\n\nRequest Locality Stats: \n");
    for(i=1; i<=aux_stat.locality_range; i++){
      fprintf(fileout," %3d  %9.5lf\n",
	      i,
	      aux_stat.locality_hit[i]*100.0/(double)aux_stat.valid_entries);
    }
  }else {
	return;
  }
  {
	int i =0;
  while(this_b != NULL && fileout != NULL){
	i++;
    fprintf(fileout,"%s %8d %6d\n",delta_stat_string,this_b->delta_stat,this_b->count);
    this_b = this_b->next_b;
  }
  }
}

void mem_close_stat_fileptrs(){
}

void init_extra_bundle_stat_collector() {
	int i;
	for (i = 0; i < BUNDLE_SIZE; i++) {
		aux_stat.num_cas[i] = 0;
		aux_stat.num_cas_w_drive[i] = 0;
		aux_stat.num_cas_w_prec[i] = 0;
		aux_stat.num_cas_write[i] = 0;
		aux_stat.num_cas_write_w_prec[i] = 0;
		aux_stat.num_ras[i] = 0;
		aux_stat.num_ras_refresh[i] = 0;
		aux_stat.num_ras_all[i] = 0;
		aux_stat.num_prec[i] = 0;
		aux_stat.num_prec_refresh[i] = 0;
		aux_stat.num_prec_all[i] = 0;
		aux_stat.num_refresh[i] = 0;
		aux_stat.num_drive[i] = 0;
		aux_stat.num_data[i] = 0;	
		aux_stat.num_refresh[i] = 0;	
		aux_stat.num_refresh_all[i] = 0;	
	}
}

void gather_extra_bundle_stats(command_t *bundle[], int command_count) {
	int i;
	command_t *this_c;
	for (i = 0; i < command_count; i++) {
		this_c = bundle[i];
		switch (this_c->command) {
			case (RAS):
			   if (this_c->refresh)
				aux_stat.num_ras_refresh[command_count-1]++;
			   else
				aux_stat.num_ras[command_count-1]++;
				break;
			case (CAS):
				aux_stat.num_cas[command_count-1]++;
				break;
			case (CAS_AND_PRECHARGE):
				aux_stat.num_cas_w_prec[command_count-1]++;
				break;
			case (CAS_WITH_DRIVE):
				aux_stat.num_cas_w_drive[command_count-1]++;
				break;
			case (CAS_WRITE):
				aux_stat.num_cas_write[command_count-1]++;
				break;
			case (CAS_WRITE_AND_PRECHARGE):
				aux_stat.num_cas_write_w_prec[command_count-1]++;
				break;
			case (PRECHARGE):
			   if (this_c->refresh)
				aux_stat.num_prec_refresh[command_count-1]++;
			   else
				aux_stat.num_prec[command_count-1]++;
				break;
			case (PRECHARGE_ALL):
				aux_stat.num_prec_all[command_count-1]++;
				break;
			case (RAS_ALL):
				aux_stat.num_ras_all[command_count-1]++;
				break;
			case (DATA):
				aux_stat.num_data[command_count-1]++;
				i++;
				break;
			case (DRIVE):
				aux_stat.num_drive[command_count-1]++;
				break;
			case (REFRESH):
				aux_stat.num_refresh[command_count-1]++;
				break;
			case (REFRESH_ALL):
				aux_stat.num_refresh_all[command_count-1]++;
				break;
			default:
				fprintf(stderr, "Unknown command: %d\n", this_c->command);
				break;
		}
	}
}

void print_extra_bundle_stats(bool common_output) {
  FILE *fileout;
  if (common_output == FALSE) {
	fileout = stdout;
  }else {
	if (aux_stat.stats_fileptr == NULL)
	  fileout = stdout;
	else
	  fileout = aux_stat.stats_fileptr;
  }
  fprintf(fileout, "Bundle Size       1    2   3\n");

  fprintf(fileout, "RAS               %d %d %d\n", aux_stat.num_ras[0], aux_stat.num_ras[1], aux_stat.num_ras[2]);
  fprintf(fileout, "CAS               %d %d %d\n", aux_stat.num_cas[0], aux_stat.num_cas[1], aux_stat.num_cas[2]);
  fprintf(fileout, "CAS w PREC        %d %d %d\n", aux_stat.num_cas_w_prec[0], aux_stat.num_cas_w_prec[1], aux_stat.num_cas_w_prec[2]);
  fprintf(fileout, "CAS w DRIVE       %d %d %d\n", aux_stat.num_cas_w_drive[0], aux_stat.num_cas_w_drive[1], aux_stat.num_cas_w_drive[2]);
  fprintf(fileout, "CAS WRITE         %d %d %d\n", aux_stat.num_cas_write[0], aux_stat.num_cas_write[1], aux_stat.num_cas_write[2]);
  fprintf(fileout, "CAS WRITE w PREC  %d %d %d\n", aux_stat.num_cas_write_w_prec[0], aux_stat.num_cas_write_w_prec[1], aux_stat.num_cas_write_w_prec[2]);
  fprintf(fileout, "PREC              %d %d %d\n", aux_stat.num_prec[0], aux_stat.num_prec[1], aux_stat.num_prec[2]);
  fprintf(fileout, "RAS ALL           %d %d %d\n", aux_stat.num_ras_all[0], aux_stat.num_ras_all[1], aux_stat.num_ras_all[2]);
  fprintf(fileout, "PREC ALL          %d %d %d\n", aux_stat.num_prec_all[0], aux_stat.num_prec_all[1], aux_stat.num_prec_all[2]);
  fprintf(fileout, "RAS REFRESH       %d %d %d\n", aux_stat.num_ras_refresh[0], aux_stat.num_ras_refresh[1], aux_stat.num_ras_refresh[2]);
  fprintf(fileout, "PREC REFRESH      %d %d %d\n", aux_stat.num_prec_refresh[0], aux_stat.num_prec_refresh[1], aux_stat.num_prec_refresh[2]);
  fprintf(fileout, "REFRESH           %d %d %d\n", aux_stat.num_refresh[0], aux_stat.num_refresh[1], aux_stat.num_refresh[2]);
  fprintf(fileout, "REFRESH_ALL       %d %d %d\n", aux_stat.num_refresh_all[0], aux_stat.num_refresh_all[1], aux_stat.num_refresh_all[2]);
  fprintf(fileout, "DATA              %d %d %d\n", aux_stat.num_data[0], aux_stat.num_data[1], aux_stat.num_data[2]);
  fprintf(fileout, "DRIVE             %d %d %d\n", aux_stat.num_drive[0], aux_stat.num_drive[1], aux_stat.num_drive[2]);
}

FILE *get_common_stat_file() {
  return aux_stat.stats_fileptr;
}

