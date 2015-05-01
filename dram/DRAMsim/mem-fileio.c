/*
 * mem-fileio.c This program performs the parsing of the spd input file.
 * Note that the power input file is not parsed here.
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
#include "mem-tokens.h"
#include "mem-system.h"

void read_dram_config_from_file(FILE *fin, dram_system_configuration_t *this_c){
  char 	c;
  char 	input_string[256];
  int	input_int;
  int	dram_config_token;

  while ((c = fgetc(fin)) != EOF){
	if((c != EOL) && (c != CR) && (c != SPACE) && (c != TAB)){
	  fscanf(fin,"%s",&input_string[1]);
	  input_string[0] = c;
	} else {
	  fscanf(fin,"%s",&input_string[0]);
	}
	dram_config_token = file_io_token(&input_string[0]);
	switch(dram_config_token){
	  case dram_type_token:
		fscanf(fin,"%s",&input_string[0]);
		if (!strncmp(input_string, "sdram", 5)){
		  set_dram_type(SDRAM);                      /* SDRAM, DDRSDRAM,  etc */
		} else if (!strncmp(input_string, "ddrsdram", 8)){
		  set_dram_type(DDRSDRAM);
		} else if (!strncmp(input_string, "ddr2", 4)){
		  set_dram_type(DDR2);
		} else if (!strncmp(input_string, "ddr3", 4)){
		  set_dram_type(DDR3);
		} else if (!strncmp(input_string, "fbd_ddr2", 8)){
		  set_dram_type(FBD_DDR2);
		} else {
		  set_dram_type(SDRAM);
		}
		break;
	  case data_rate_token:				/* aka memory_frequency: units is MBPS */
		fscanf(fin,"%d",&input_int);
		set_dram_frequency(input_int);
		break;
	  case dram_clock_granularity_token:
		fscanf(fin,"%d",&(this_c->dram_clock_granularity));
		break;
	  case critical_word_token:
		fscanf(fin,"%s",&input_string[0]);
		if (!strncmp(input_string, "TRUE", 4)){
		  this_c->critical_word_first_flag = TRUE;
		} else if (!strncmp(input_string, "FALSE", 5)){
		  this_c->critical_word_first_flag = FALSE;
		} else {
		  fprintf(stderr,"\n\n\n\nCritical Word First Flag, Expecting TRUE or FALSE, found [%s] instead\n\n\n",input_string);
		}
		break;
	  case PA_mapping_policy_token:
		fscanf(fin,"%s",&input_string[0]);
		if (!strncmp(input_string, "burger_base_map", 15)){
		  set_pa_mapping_policy(BURGER_BASE_MAP);  /* How to map physical address to memory address */
		} else if (!strncmp(input_string, "burger_alt_map", 14)){
		  set_pa_mapping_policy(BURGER_ALT_MAP);
		} else if (!strncmp(input_string, "sdram_base_map", 14)){
		  set_pa_mapping_policy(SDRAM_BASE_MAP);
		} else if (!strncmp(input_string, "sdram_hiperf_map", 16)){
		  set_pa_mapping_policy(SDRAM_HIPERF_MAP);
		} else if (!strncmp(input_string, "intel845g_map", 13)){
		  set_pa_mapping_policy(INTEL845G_MAP);
		} else if (!strncmp(input_string, "sdram_close_page_map", 20)){
          set_pa_mapping_policy(SDRAM_CLOSE_PAGE_MAP);
		}else {
		  fprintf(stderr,"\n\n\n\nExpecting mapping policy, found [%s] instead\n\n\n",input_string);
		  set_pa_mapping_policy(SDRAM_BASE_MAP);
		}
		break;
	  case row_buffer_management_policy_token:
		fscanf(fin,"%s",&input_string[0]);
		if (!strncmp(input_string, "open_page", 9)){
		  set_dram_row_buffer_management_policy(OPEN_PAGE);               /* OPEN_PAGE, CLOSED_PAGE etc */
		} else if (!strncmp(input_string, "close_page", 10)){
		  set_dram_row_buffer_management_policy(CLOSE_PAGE);
		} else if (!strncmp(input_string, "perfect_page", 12)){
		  set_dram_row_buffer_management_policy(PERFECT_PAGE);
		} else {
		  fprintf(stderr,"\n\n\n\nExpecting buffer management policy, found [%s] instead\n\n\n",input_string);
		  set_dram_row_buffer_management_policy(OPEN_PAGE);
		}
		break;
	  case cacheline_size_token:
		fscanf(fin,"%d",&input_int);
		set_dram_transaction_granularity(input_int);
		break;
	  case channel_count_token:
		fscanf(fin,"%d",&input_int);
		set_dram_channel_count(input_int);
		break;
	  case channel_width_token:
		fscanf(fin,"%d",&input_int);
		set_dram_channel_width(input_int);
		break;
	  case rank_count_token:
		fscanf(fin,"%d",&input_int);
		set_dram_rank_count(input_int);
		break;
	  case bank_count_token:
		fscanf(fin,"%d",&input_int);
		set_dram_bank_count(input_int);
		break;
	  case row_count_token:
		fscanf(fin,"%d",&input_int);
		set_dram_row_count(input_int);
		break;
	  case col_count_token:
		fscanf(fin,"%d",&input_int);
        set_dram_col_count(input_int);
		break;
	  case t_cas_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_cas = input_int;
		break;
	  case t_cmd_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_cmd = input_int;
		break;
	  case t_cwd_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_cwd = input_int;
		break;
	  case t_dqs_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_dqs = input_int;
		break;
	  case t_faw_token:
        fscanf(fin,"%d",&input_int);
        this_c->t_faw = input_int;
        break;
	  case t_ras_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_ras = input_int;
		break;
	  case t_rcd_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_rcd = input_int;
		break;
	  case t_rc_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_rc = input_int;
		break;
	  case t_rfc_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_rfc = input_int;
		break;
	  case t_rrd_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_rrd = input_int;
		break;
	  case t_rp_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_rp = input_int;
		break;
	  case t_wr_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_wr = input_int;
		break;
	  case t_rtp_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_rtp = input_int;
		break;
	  case posted_cas_token:
		fscanf(fin,"%s",&input_string[0]);
		if (!strncmp(input_string, "TRUE", 4)){
		  this_c->posted_cas_flag = TRUE;
		} else if (!strncmp(input_string, "FALSE", 5)){
		  this_c->posted_cas_flag = FALSE;
		} else {
		  fprintf(stderr,"\n\n\n\nPosted CAS Flag, Expecting TRUE or FALSE, found [%s] instead\n\n\n",input_string);
		}
		break;
	  case t_al_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_al = input_int;
		break;
	  case t_rl_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_rl = input_int;
		break;
	  case t_cac_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_cac = input_int;
		break;
	  case t_rtr_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_rtr = input_int;
		break;
	  case auto_refresh_token:
		fscanf(fin,"%s",&input_string[0]);
		if (!strncmp(input_string, "TRUE", 4)){
		  this_c->auto_refresh_enabled = TRUE;
		} else if (!strncmp(input_string, "FALSE", 5)){
		  this_c->auto_refresh_enabled = FALSE;
		} else {
		  fprintf(stderr,"\n\n\n\nAuto Refresh Enable, Expecting TRUE or FALSE, found [%s] instead\n\n\n",input_string);
		}

		break;
	  case auto_refresh_policy_token:
		fscanf(fin,"%s",&input_string[0]);
		if (!strncmp(input_string, "refresh_one_chan_all_rank_all_bank", 34)){
		  set_dram_refresh_policy(REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK);              
		} else if (!strncmp(input_string, "refresh_one_chan_all_rank_one_bank", 34)) {
		  set_dram_refresh_policy(REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK);
		} else if (!strncmp(input_string, "refresh_one_chan_one_rank_one_bank", 34)) {
		  set_dram_refresh_policy(REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK);
		} else if (!strncmp(input_string, "refresh_one_chan_one_rank_all_bank", 34)) {
		  set_dram_refresh_policy(REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK);
		} else {
		  fprintf(stderr,"\n\n\n\nExpecting refresh policy, found [%s] instead\n\n\n",input_string);
		  set_dram_refresh_policy(REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK);              
		}

		break;
	  case refresh_time_token:
		fscanf(fin,"%d",&input_int);
		set_dram_refresh_time(input_int);
		/* Note: Time is specified in us */
		break;
	  case comment_token:
		while (((c = fgetc(fin)) != EOL) && (c != EOF)){
		  /*comment, to be ignored */
		}
		break;
		/**** FB-DIMM tokens **/
	  case t_bus_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_bus = input_int; //this value is already in MC bus time
		break;
	  case var_latency_token:
		fscanf(fin,"%s",&input_string[0]);
		if (!strncmp(input_string, "TRUE", 4)){
		  this_c->var_latency_flag = TRUE;
		} else if (!strncmp(input_string, "FALSE", 5)){
		  this_c->var_latency_flag = FALSE;
		} else {
		  fprintf(stderr,"\n\n\n\nVariable Latency flag, Expecting TRUE or FALSE, found [%s] instead\n\n\n",input_string);
		}
		break;

	  case 	up_channel_width_token:
		fscanf(fin,"%d",&input_int);
		set_dram_up_channel_width(input_int);
		break;
	  case 	down_channel_width_token:
		fscanf(fin,"%d",&input_int);
		set_dram_down_channel_width(input_int);
		break;
	  case 	t_amb_up_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_amb_up = input_int;
		break;
	  case 	t_amb_down_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_amb_down = input_int;
		break;
	  case 	t_bundle_token:
		fscanf(fin,"%d",&input_int);
		this_c->t_bundle = input_int;
		break;
	  case mem_cont_freq_token:
		fscanf(fin,"%d",&input_int);
		set_memory_frequency(input_int);
		break;
	  case dram_freq_token:
		fscanf(fin,"%d",&input_int);
		set_dram_frequency(input_int);
		break;
	  case amb_up_buffer_token:
		fscanf(fin,"%d",&input_int);
		this_c->up_buffer_cnt = input_int;
		break;
	  case amb_down_buffer_token:
		fscanf(fin,"%d",&input_int);
		this_c->down_buffer_cnt = input_int;
		break;
	  default:
	  case unknown_token:
		fprintf(stderr,"Unknown Token [%s]\n",input_string);
		break;
	}
  }
}

int file_io_token(char *input){
  size_t length;
  length = strlen(input);
  if(strncmp(input, "//",2) == 0) {
	return comment_token;
  } else if (strncmp(input, "type",length) == 0) {
	return dram_type_token;
  } else if (strncmp(input, "datarate",length) == 0) {
	return data_rate_token;
  } else if (strncmp(input, "clock_granularity",length) == 0) {
	return dram_clock_granularity_token;
  } else if (strncmp(input, "critical_word_first",length) == 0) {
	return critical_word_token;
  } else if (strncmp(input, "VA_mapping_policy",length) == 0) {
	return VA_mapping_policy_token;
  } else if (strncmp(input, "PA_mapping_policy",length) == 0) {
	return PA_mapping_policy_token;
  } else if (strncmp(input, "row_buffer_policy",length) == 0) {
	return row_buffer_management_policy_token;
  } else if (strncmp(input, "cacheline_size",length) == 0) {
	return cacheline_size_token;
  } else if (strncmp(input, "channel_count",length) == 0) {
	return channel_count_token;
  } else if (strncmp(input, "channel_width",length) == 0) {
	return channel_width_token;
  } else if (strncmp(input, "rank_count",length) == 0) {
	return rank_count_token;
  } else if (strncmp(input, "bank_count",length) == 0) {
	return bank_count_token;
  } else if (strncmp(input, "row_count",length) == 0) {
	return row_count_token;
  } else if (strncmp(input, "col_count",length) == 0) {
	return col_count_token;
  } else if (strncmp(input, "t_cas",length) == 0) {
    return t_cas_token;
  } else if (strncmp(input, "t_cmd",length) == 0) {
    return t_cmd_token;
  } else if (strncmp(input, "t_cwd",length) == 0) {
    return t_cwd_token;
  } else if (strncmp(input, "t_cac",length) == 0) {
    return t_cac_token;
  } else if (strncmp(input, "t_dqs",length) == 0) {
    return t_dqs_token;
  } else if (strncmp(input, "t_faw",length) == 0) {
    return t_faw_token;
  } else if (strncmp(input, "t_ras",length) == 0) {
	return t_ras_token;
  } else if (strncmp(input, "t_rcd",5) == 0) {
	return t_rcd_token;
  } else if (strncmp(input, "t_rc",4) == 0) {
	return t_rc_token;
  } else if (strncmp(input, "t_rrd",length) == 0) {
    return t_rrd_token;
  } else if (strncmp(input, "t_rp",length) == 0) {
	return t_rp_token;
  } else if (strncmp(input, "t_rfc",length) == 0) {
	return t_rfc_token;
  } else if (strncmp(input, "t_cac",length) == 0) {
	return t_cac_token;
  } else if (strncmp(input, "t_cwd",length) == 0) {
	return t_cwd_token;
  } else if (strncmp(input, "t_rtr",length) == 0) {
	return t_rtr_token;
  } else if (strncmp(input, "t_dqs",length) == 0) {
	return t_dqs_token;
  } else if (strncmp(input, "posted_cas",length) == 0) {
	return posted_cas_token;
  } else if (strncmp(input, "t_al",length) == 0) {
	return t_al_token;
  } else if (strncmp(input, "t_rl",length) == 0) {
	return t_rl_token;
  } else if (strncmp(input, "t_wr",length) == 0) {
	return t_wr_token;
  } else if (strncmp(input, "t_rtp",length) == 0) {
	return t_rtp_token;
  } else if (strncmp(input, "auto_refresh",length) == 0) {
	return auto_refresh_token;
  } else if (strncmp(input, "auto_refresh_policy",length) == 0) {
	return auto_refresh_policy_token;
  } else if (strncmp(input, "refresh_time",length) == 0) {
	return refresh_time_token;

	/******** FB-DIMM specific tokens ****/
  } else if (strncmp(input, "t_bus",length) == 0) {
	return t_bus_token; 
  } else if (strncmp(input,"up_channel_width",length) == 0) {
	return up_channel_width_token;
  }
  else if (strncmp(input,"down_channel_width",length) == 0) {
	return down_channel_width_token;
  }
  else if (strncmp(input,"t_amb_up",length) == 0) {
	return t_amb_up_token;
  }
  else if (strncmp(input,"t_amb_down",length) == 0) {
	return t_amb_down_token;
  }
  else if (strncmp(input,"t_bundle",length) == 0) {
	return t_bundle_token;
  }
  else if (strncmp(input,"t_bus",length) == 0) {
	return t_bus_token;
  }
  else if (strncmp(input,"var_latency",length) == 0) {
	return var_latency_token;
  }
  else if (strncmp(input,"mc_freq",length) == 0) {
	return mem_cont_freq_token;
  }
  else if (strncmp(input,"dram_freq",length) == 0) {
	return dram_freq_token;
  }
  else if (strncmp(input,"amb_up_buffer_count", length) == 0) {
	return amb_up_buffer_token;
  }
  else if (strncmp(input,"amb_down_buffer_count", length) == 0) {
	return amb_down_buffer_token;
  }
  else {
	fprintf(stderr," Unknown token %s %d\n",input,strncmp(input,"var_latency",length));
	return unknown_token;
  }
}
