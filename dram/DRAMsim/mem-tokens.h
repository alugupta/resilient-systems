/*
 * mem-tokens.h This header file contains definitions for tokens used for
 * parsing the configuraiton spd file.
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

#define 	EOL 	10
#define 	CR 	13
#define 	SPACE	32
#define		TAB	9

enum spd_token_t {
	dram_type_token = 0,
	data_rate_token,
	dram_clock_granularity_token,
	critical_word_token,
	VA_mapping_policy_token,
	PA_mapping_policy_token,
	row_buffer_management_policy_token,
	auto_precharge_interval_token,
	cacheline_size_token,
	channel_count_token,
	channel_width_token,
	rank_count_token,
	bank_count_token,
	row_count_token,
	col_count_token,
	t_cas_token,
	t_cmd_token,
	t_cwd_token,
	t_dqs_token,
	t_faw_token,
	t_ras_token,
	t_rc_token,
	t_rcd_token,
	t_rrd_token,
	t_rp_token,
	t_rfc_token,
	t_wr_token,
	t_rtp_token,
	t_rtr_token,
	t_cac_token,
	posted_cas_token,
	t_al_token,
	t_rl_token,
	auto_refresh_token,
	auto_refresh_policy_token,
	refresh_time_token,
	comment_token,
	unknown_token,
	EOL_token,
	/************ FB-DIMM tokens ******/
	t_bus_token,
	var_latency_token,
	up_channel_width_token,
	down_channel_width_token,
	t_amb_up_token,
	t_amb_down_token,
	t_bundle_token,
	mem_cont_freq_token,
	dram_freq_token,
	amb_up_buffer_token,
	amb_down_buffer_token
};

int file_io_token(char *);


