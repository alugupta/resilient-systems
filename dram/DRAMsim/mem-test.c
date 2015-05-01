/*
 * mem-test.c - Portion of the dram simulator which parses command line
 * options/ generates access distributions
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


#include <math.h>

#include "mem-system.h"
#include "mem-biu.h"
#include <errno.h>
#include <time.h>
extern int errno;

#define MAX_FILENAME_LENGTH	1024
#define UNIFORM_DISTRIBUTION 1
#define GAUSSIAN_DISTRIBUTION 2
#define POISSON_DISTRIBUTION 3
#define PI 3.141592654
#define MAX_TRACES 8
void 	set_cpu_memory_frequency_ratio(biu_t *this_b, int mem_frequency);
double 	box_muller(double m, double s);
double 	poisson_rng(double m);
double 	gammaln(double xx);
int 	get_mem_access_type(float access_distribution[]);

BUS_event_t *current_bus_event;
FILE *trace_fileptr[MAX_TRACES];
/* The SYSTEM lives in main for now*/
int
main(int argc,char *argv[]){

	int		argc_index;
	size_t 		length;
	BUS_event_t     *next_e = NULL;

	biu_t *global_biu;
	int max_inst;	
	tick_t current_cpu_time;
	tick_t max_cpu_time;			
	int cpu_frequency;
	int dram_type;				/* dram type {sdram|ddrsdram|drdram|ddr2} */
	int dram_frequency;
	int memory_frequency;
	int channel_count;                      /* total number of logical channels */
	int channel_width;                      /* logical channel width in bytes */
	int strict_ordering_flag;
	int refresh_interval;                   /* in milliseconds */

	int chipset_delay;                      /* latency through chipset, in DRAM ticks */
	int bus_queue_depth;
	int biu_delay;                          /* latency through BIU, in CPU ticks */
	int address_mapping_policy;		/* {burger_map|burger_alt_map|sdram_base_map|intel845g_map} */
	int row_buffer_management_policy;	/* {open_page|close_page|perfect_page|auto_page} */
	int refresh_issue_policy;                   
	int biu_debug_flag;
	int transaction_debug_flag;
	int dram_debug_flag;
	int trace_input_flag;
	int all_debug_flag;     		/* if this is set, all dram related debug flags are turned on */
	int wave_debug_flag;    		/* cheesy Text Based waveforms  */
	int cas_wave_debug_flag;    		/* cheesy Text Based waveforms  */
	int bundle_debug_flag;    		/* Watch Bundles that are issued  */
	int amb_buffer_debug_flag;		/* watch amb buffers */
	unsigned int debug_tran_id_threshold;		/* tran id at which to start debug output */
	unsigned int debug_tran_id;			/* tran id  which to print out why commands are not being output*/
	unsigned int debug_ref_tran_id;			/* tran id  which to print out why commands are not being output*/
	int get_next_event_flag = FALSE;

	char *power_stats_fileout= NULL;
	char *common_stats_fileout = NULL; 
	char *trace_filein [MAX_TRACES];

	bool biu_stats = false;
	bool biu_slot_stats = false;
	bool biu_access_dist_stats = false;
	bool bundle_stats = false;
	bool tran_queue_stats = true;
	bool bank_hit_stats = false;
	bool bank_conflict_stats = false;
	bool cas_per_ras_stats = false;

	double average_interarrival_cycle_count;               /* every X cycles, a transaction arrives */
	double arrival_thresh_hold;                            /* if drand48() exceed this thresh_hold. */
	int arrival_distribution_model;
	int slot_id;
	int fake_access_type;
	int fake_rid;
	int fake_v_address;			/* virtual address */
	int fake_p_address;			/* physical address */
	int thread_id;
	char spd_filein[MAX_FILENAME_LENGTH];
	char power_filein[MAX_FILENAME_LENGTH] = "\0";
	FILE *spd_fileptr;
	FILE *power_fileptr;
	float access_distribution[4]; /* used to set distribution precentages of access types */
	int num_trace = 0;
	int transaction_total = 0;
	int transaction_retd_total = 0;
	int last_retd_total = 0;
	time_t sim_start_time;
	time_t sim_end_time;
	int i;
	/* set defaults */
	
	max_inst			= 1000000;	/* actually "cycle count" in this tester */
	cpu_frequency 			= 1000;
	dram_type 			= SDRAM;
	dram_frequency 			= 100;
	memory_frequency 			= 100;
	channel_count 			= 1;
	channel_width 			= 8;	/* bytes */
	refresh_interval 		= -1; //Ohm: change this from -1 to 0
	chipset_delay			= 10;	/* DRAM ticks */
	bus_queue_depth			= 32;
	row_buffer_management_policy 	= MEM_STATE_INVALID;
	chipset_delay                   = 10;		/* DRAM ticks */
	biu_delay                       = 10;		/* CPU ticks */
	address_mapping_policy 		= SDRAM_BASE_MAP;
	refresh_issue_policy = REFRESH_HIGHEST_PRIORITY;
	strict_ordering_flag  		= FALSE;
	biu_debug_flag 			= FALSE;
	transaction_debug_flag		= FALSE;
	dram_debug_flag			= FALSE;
	trace_input_flag  		= FALSE;
	all_debug_flag     		= FALSE;
	wave_debug_flag  		= FALSE;
	cas_wave_debug_flag  		= FALSE;
	bundle_debug_flag  		= FALSE;
	amb_buffer_debug_flag	= FALSE;
	average_interarrival_cycle_count= 10.0;	/* every 10 cycles, inject a new memory request */
	arrival_distribution_model = UNIFORM_DISTRIBUTION;
	debug_tran_id_threshold		= 0;
	debug_tran_id		= 0;
	debug_ref_tran_id		= 0;
	spd_filein[0] 			= '\0';
	argc_index = 1;

	// in the below scheme, all access types have equal probability
	access_distribution[0] = 0.20;	
	access_distribution[1] = 0.40;	
	access_distribution[2] = 0.60;	
	access_distribution[3] = 0.80;	

	
	setbuf(stdout,NULL);
	setbuf(stderr,NULL);

	/* Now init variables */
	init_biu(NULL);
	global_biu = get_biu_address();
	set_biu_depth(global_biu, bus_queue_depth);

	/* After handling all that, now override default numbers */
	set_cpu_frequency(global_biu, cpu_frequency);				/* input value in MHz */
	init_dram_system();
	biu_set_mem_cfg(global_biu,get_dram_system_config());
	set_dram_type(dram_type);					/* SDRAM, DDRSDRAM,  */
	set_dram_frequency(dram_frequency);                             /* units in MHz */
	set_dram_channel_width(channel_width);				/* data bus width, in bytes */
	set_dram_transaction_granularity(64);                   	/* How many bytes are fetched per transaction? */
									/* should be L2:bsize, but allow that here */

	dram_frequency = MEM_STATE_INVALID;
	channel_count = MEM_STATE_INVALID;
	cpu_frequency = MEM_STATE_INVALID;
	dram_type = MEM_STATE_INVALID;
	channel_width = MEM_STATE_INVALID;

	if((argc < 2) || (strncmp(argv[1], "-help",5) == 0)){
		fprintf(stdout,"Usage: %s -options optionswitch\n",argv[0]);
		fprintf(stdout,"-trace_file FILENAME\n");
		fprintf(stdout,"-cpu:frequency <frequency of processor>\n");
		fprintf(stdout,"-dram:frequency <frequency of dram>\n");
		fprintf(stdout,"-max:inst <Maximum number of cpu cycles to tick>\n");
		fprintf(stdout,"-dram:type <DRAM type> allowed options sdram,ddrsdram,ddr,ddr2,drdram,fbdimm\n");
		fprintf(stdout,"-dram:channel_count <number of channels>\n");
		fprintf(stdout,"-dram:channel_width <Data bus width in bytes>\n");
		fprintf(stdout,"-dram:refresh <Time to refresh entire dram in ms: default 64 ms>\n");
		fprintf(stdout,"-dram:refresh_issue_policy <Refresh schedulign priority - options are priority,opportunistic>\n");
		fprintf(stdout,"-dram:refresh_policy <Refresh policy >\n");
		fprintf(stdout,"-dram:chipset_delay <Time to get transaction in and out of biu>\n");
		fprintf(stdout,"-dram:spd_input <File that lists configuration timing information>\n");
		fprintf(stdout,"-dram:power_input <File that lists configuration power information>\n");
		fprintf(stdout,"-dram:row_buffer_management_policy <Page policy - open_page,close_page>\n");
		fprintf(stdout,"-biu:trans_selection <Scheduling policy - fcfs,wang,least_pending,most_pending,greedy,obf(open bank first),riff(read first)>\n");
		fprintf(stdout,"-dram:address_mapping_policy <Address Mapping policy - burger_base_map,burger_alt_map,sdram_base_map,sdram_close_page_map,intel845g_map>\n");
		fprintf(stdout,"-dram:strict_ordering Enable in-order processing of memory requests\n");
		fprintf(stdout,"-stat:biu To record distribution of latency of read requests\n");
		fprintf(stdout,"-stat:biu_slot To record distribution of memory request types\n");
		fprintf(stdout,"-stat:biu-access-dist Records the time interval between arrival of two consecutive memory requests\n");
		fprintf(stdout,"-stat:bundle Records the composition of a bundle (FBDIMM packet) \n");
		fprintf(stdout,"-stat:tran_queue Records the occupancy of the transaction queue\n");
		fprintf(stdout,"-stat:dram:bank_hit Open page keeps track of the bank hit statistics\n");
		fprintf(stdout,"-stat:dram:bank_conflict Open page keeps track of the bank conflict statistics\n");
		fprintf(stdout,"-stat:power <File that records periodic power consumption patterns on a per rank basis>\n");
		fprintf(stdout,"-stat:all <File into which all statistics barring periodic power is recorded>\n");
		
		fprintf(stdout,"-debug:wave Displays text-based view of issued commands life-cycle\n");
		fprintf(stdout,"-debug:cas Displays text-based view of issued column access read and write commands life-cycle\n");
		fprintf(stdout,"-debug:transaction Displays text-based view of a transactions issued/retired\n");
		fprintf(stdout,"-debug:biu Displays text-based view of a memory requests issue/retirement in the biu\n");
		fprintf(stdout,"-debug:bundle Displays text-based view of the FB-DIMM packets that are issued\n");
		fprintf(stdout,"-debug:amb Displays text-based view of the FB-DIMM AMB occupancy and release\n");
		fprintf(stdout,"-debug:threshold <Transaction id after which debug info is displayed>\n");
		fprintf(stdout,"-watch:trans <Transaction id to watch >\n");
		fprintf(stdout,"-watch:ref_trans <Refresh Transaction id to watch >\n");
		
		fprintf(stdout,"-help To view this message\n");
		exit(0);
	}

	while(argc_index < argc){
		length = strlen(argv[argc_index]);
		if(strncmp(argv[argc_index], "-max:inst",length) == 0) {
			sscanf(argv[argc_index+1],"%d",&max_inst);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-cpu:frequency",length) == 0) {
			sscanf(argv[argc_index+1],"%d",&cpu_frequency);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-dram:type",length) == 0) {
			length = strlen(argv[argc_index+1]);
			if(strncmp(argv[argc_index+1], "sdram",length) == 0) {
				dram_type = SDRAM;
			} else if(strncmp(argv[argc_index+1], "ddrsdram",length) == 0) {
				dram_type = DDRSDRAM;
			} else if(strncmp(argv[argc_index+1], "ddr2",length) == 0) {
				dram_type = DDR2;
			} else {
				fprintf(stdout,"Unknown RAM type [%s]\n",argv[argc_index+1]);
				_exit(0);
			}
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-dram:frequency",length) == 0) {
			sscanf(argv[argc_index+1],"%d",&dram_frequency);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-dram:channel_count",length) == 0) {
			sscanf(argv[argc_index+1],"%d",&channel_count);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-dram:channel_width",length) == 0) {
			sscanf(argv[argc_index+1],"%d",&channel_width);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-dram:refresh",length) == 0) {
			sscanf(argv[argc_index+1],"%d",&refresh_interval);
			argc_index += 2;
			
		} else if(strncmp(argv[argc_index], "-dram:chipset_delay",length) == 0) {
			sscanf(argv[argc_index+1],"%d",&channel_width);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-dram:spd_input",length) == 0) {
			sscanf(argv[argc_index+1],"%s",&spd_filein[0]);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-dram:power_input",length) == 0) {
			sscanf(argv[argc_index+1],"%s",power_filein);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-trace_file",length) == 0) {
		  int j = ++argc_index;
		  while ((j< argc) && strncmp(argv[j],"-",1) != 0) {
			trace_filein[num_trace] = (char *) malloc(sizeof(char)*MAX_FILENAME_LENGTH);
			sscanf(argv[j],"%s",trace_filein[num_trace]);
			num_trace++;
			j++;
		  }
		  argc_index += num_trace;
		} else if(strncmp(argv[argc_index], "-dram:row_buffer_management_policy",length) == 0) {
			length = strlen(argv[argc_index+1]);
			if(strncmp(argv[argc_index+1], "open_page",length) == 0) {
				row_buffer_management_policy = OPEN_PAGE;
			} else if(strncmp(argv[argc_index+1], "close_page",length) == 0) {
				row_buffer_management_policy = CLOSE_PAGE;
			} else {
				fprintf(stdout,"Unknown row buffer management policy. [%s]\n",argv[argc_index+1]);
				_exit(0);
			}
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-dram:address_mapping_policy",length) == 0) {
		  length = strlen(argv[argc_index+1]);
		  if(strncmp(argv[argc_index+1], "burger_base_map",length) == 0) {
			address_mapping_policy = BURGER_BASE_MAP;
		  } else if(strncmp(argv[argc_index+1], "burger_alt_map",length) == 0) {
			address_mapping_policy = BURGER_ALT_MAP;
		  } else if(strncmp(argv[argc_index+1], "sdram_base_map",length) == 0) {
			address_mapping_policy = SDRAM_BASE_MAP;
		  } else if(strncmp(argv[argc_index+1], "sdram_hiperf_map",length) == 0) {
			address_mapping_policy = SDRAM_HIPERF_MAP;
		  } else if(strncmp(argv[argc_index+1], "intel845g_map",length) == 0) {
			address_mapping_policy = INTEL845G_MAP;
		  } else if(strncmp(argv[argc_index+1], "sdram_close_page_map",length) == 0) {
			address_mapping_policy = SDRAM_CLOSE_PAGE_MAP;
		  } else {
			fprintf(stdout,"Unknown Address mapping policy. [%s]\n",argv[argc_index+1]);
			_exit(0);
		  }
		  argc_index += 2;

		}else if(strncmp(argv[argc_index], "-biu:trans_selection",length) == 0) {
			length = strlen(argv[argc_index+1]);
			if(strncmp(argv[argc_index+1], "fcfs",length) == 0) {
				set_transaction_selection_policy(global_biu,FCFS);
			} else if(strncmp(argv[argc_index+1], "riff",length) == 0) {
				set_transaction_selection_policy(global_biu,RIFF);
			} else if(strncmp(argv[argc_index+1], "hstp",length) == 0) {
				set_transaction_selection_policy(global_biu,HSTP);
			} else if(strncmp(argv[argc_index+1], "obf",length) == 0) {
				set_transaction_selection_policy(global_biu,OBF);
			} else if(strncmp(argv[argc_index+1], "wang",length) == 0) {
				set_transaction_selection_policy(global_biu,WANG);
			} else if(strncmp(argv[argc_index+1], "most_pending",length) == 0) {
				set_transaction_selection_policy(global_biu,MOST_PENDING);
			} else if(strncmp(argv[argc_index+1], "least_pending",length) == 0) {
				set_transaction_selection_policy(global_biu,LEAST_PENDING);
			}else if(strncmp(argv[argc_index+1], "greedy",length) == 0) {
				set_transaction_selection_policy(global_biu,GREEDY);
			} else {
				fprintf(stdout,"Unknown Transaction Selection Type [%s]\n",argv[argc_index+1]);
				_exit(0);
			}
			argc_index += 2;
		}else if(strncmp(argv[argc_index], "-dram:refresh_issue_policy",length) == 0) {
		  length = strlen(argv[argc_index+1]);
		  if(strncmp(argv[argc_index+1], "opportunistic",length) == 0) {
			refresh_issue_policy = REFRESH_OPPORTUNISTIC;
		  } else if(strncmp(argv[argc_index+1], "priority",length) == 0) {
			refresh_issue_policy = REFRESH_HIGHEST_PRIORITY;
		  }
			argc_index += 2;
		}else if(strncmp(argv[argc_index], "-dram:refresh_policy",length) == 0) {
		  length = strlen(argv[argc_index+1]);
          if (!strncmp(argv[argc_index+1], "refresh_one_chan_all_rank_all_bank", length)){
            set_dram_refresh_policy(REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK);
          } else if (!strncmp(argv[argc_index+1], "refresh_one_chan_all_rank_one_bank", length)) {
            set_dram_refresh_policy(REFRESH_ONE_CHAN_ALL_RANK_ONE_BANK);
          } else if (!strncmp(argv[argc_index+1], "refresh_one_chan_one_rank_one_bank", length)) {
            set_dram_refresh_policy(REFRESH_ONE_CHAN_ONE_RANK_ONE_BANK);
          } else if (!strncmp(argv[argc_index+1], "refresh_one_chan_one_rank_all_bank", length)) {
            set_dram_refresh_policy(REFRESH_ONE_CHAN_ONE_RANK_ALL_BANK);
          } else {
            fprintf(stderr,"\n\n\n\nExpecting refresh policy, found [%s] instead\n\n\n",argv[argc_index+1]);
            set_dram_refresh_policy(REFRESH_ONE_CHAN_ALL_RANK_ALL_BANK);
          }
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-stat:biu",length) == 0) {
		  biu_stats = true;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-stat:biu-slot",length) == 0) {
		  biu_slot_stats = true;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-stat:biu-access-dist",length) == 0) {
		  biu_access_dist_stats = true;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-stat:bundle",length) == 0) {
		  bundle_stats = true;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-stat:tran_queue",length) == 0) {
		  tran_queue_stats = true;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-stat:dram:bank_hit",length) == 0) {
		  bank_hit_stats = true;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-stat:dram:bank_conflict",length) == 0) {
		  bank_conflict_stats = true;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-stat:dram:cas_per_ras",length) == 0) {
		  cas_per_ras_stats = true;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-stat:power",length) == 0) {
		  power_stats_fileout = (char *) malloc (sizeof(char)*MAX_FILENAME_LENGTH);
		  strcpy(power_stats_fileout,argv[argc_index+1]);
			argc_index+=2;
		} else if(strncmp(argv[argc_index], "-stat:all",length) == 0) {
		  common_stats_fileout = (char *) malloc (sizeof(char)*MAX_FILENAME_LENGTH);
		  strcpy(common_stats_fileout,argv[argc_index+1]);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-dram:strict_ordering",length) == 0) {
			strict_ordering_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:biu",length) == 0) {
			biu_debug_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:transaction",length) == 0) {
			transaction_debug_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:dram",length) == 0) {
			dram_debug_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:wave",length) == 0) {
			wave_debug_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:cas",length) == 0) {
			cas_wave_debug_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:all",length) == 0) {
			all_debug_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:biu",length) == 0) {
			biu_debug_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:bundle",length) == 0) {
			bundle_debug_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:amb",length) == 0) {
			amb_buffer_debug_flag = TRUE;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-debug:threshold",length) == 0) {
			sscanf(argv[argc_index+1],"%u",&debug_tran_id_threshold);
			argc_index += 2;
		} else if(strncmp(argv[argc_index], "-watch:trans",length) == 0) {
			sscanf(argv[argc_index+1],"%d",&debug_tran_id);
			// Watch Transaction option
			set_tran_watch(debug_tran_id);
			argc_index+=2;
		} else if(strncmp(argv[argc_index], "-watch:ref_trans",length) == 0) {
			sscanf(argv[argc_index+1],"%d",&debug_ref_tran_id);
			printf("%d",debug_ref_tran_id);
			set_ref_tran_watch(debug_ref_tran_id);
			argc_index+=2;
		} else if(strncmp(argv[argc_index], "-arrivaldist:poisson", length) == 0) {
			arrival_distribution_model = POISSON_DISTRIBUTION;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-arrivaldist:gaussian", length) == 0) {
			arrival_distribution_model = GAUSSIAN_DISTRIBUTION;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-arrivaldist:uniform", length) == 0) {
			arrival_distribution_model = UNIFORM_DISTRIBUTION;
			argc_index++;
		} else if(strncmp(argv[argc_index], "-accessdist", length) == 0) {
			sscanf(argv[argc_index+1], "%e", &access_distribution[0]);
			sscanf(argv[argc_index+2], "%e", &access_distribution[1]);
			sscanf(argv[argc_index+3], "%e", &access_distribution[2]);
			sscanf(argv[argc_index+4], "%e", &access_distribution[3]);
			argc_index += 5;
		} else {
			fprintf(stdout,"Unknown option [%s]\n",argv[argc_index]);
			_exit(0);
		}
	}
	if (num_trace != 0) {
	  int i;
	  trace_input_flag = true;
	  for (i=0;i<num_trace;i++) {
		assert(trace_filein[i] != NULL);
		if ((trace_fileptr[i] = fopen (trace_filein[i],"r")) == NULL) {
		  fprintf(stderr," Error opening trace input %s\n",trace_filein[i]);
		  exit(0);
		}
	  }
	  current_bus_event = (BUS_event_t*) calloc(num_trace,sizeof(BUS_event_t));
	  for (i=0;i<num_trace;i++) {
		current_bus_event[i].already_run = TRUE;
	  }
	  if (current_bus_event == NULL) {
		fprintf(stderr," Error no memory available \n");
		exit(0);
	  }
	}
	
	/* Read the timing parameters in */
	if((spd_filein[0] != '\0') && ((spd_fileptr = fopen(&spd_filein[0], "r+")) != NULL)){
		read_dram_config_from_file(spd_fileptr,get_dram_system_config());
	}
	/* Read the power parameters in */
	if((power_filein[0] != '\0') && ((power_fileptr = fopen(power_filein, "r+")) != NULL)){
		read_power_config_from_file(power_fileptr,&get_dram_system_config()->dram_power_config);
	}
	/* Set up the power output if power_filein exists or name of power output
	 * is specified*/
	if(power_stats_fileout != NULL) {
		mem_initialize_power_collection(get_dram_power_info(), power_stats_fileout);
	}
	
	/* Finally, allow the command line switches to override defaults */
	if(dram_frequency != MEM_STATE_INVALID){
		set_dram_frequency(dram_frequency);                 /* units in MHz */
	}
	if (channel_count != MEM_STATE_INVALID)
		set_dram_channel_count(channel_count);                          /* single channel of memory */

	if (cpu_frequency != MEM_STATE_INVALID)
		set_cpu_frequency(global_biu, cpu_frequency);				/* input value in MHz */
	if (dram_type != MEM_STATE_INVALID)
		set_dram_type(dram_type);					/* SDRAM, DDRSDRAM,  */
	if (channel_width!= MEM_STATE_INVALID)
		set_dram_channel_width(channel_width);				/* data bus width, in bytes */
	
	dram_frequency = get_dram_frequency();
	memory_frequency = get_memory_frequency();
	set_cpu_memory_frequency_ratio(global_biu, memory_frequency);     /* finalize the ratio here */
	if(refresh_interval > 0){
		enable_auto_refresh(TRUE,refresh_interval);		/* set auto refresh, cycle through every "refresh_interval" ms */
	}else if (refresh_interval == 0) { /* Explicitly disable refresh interval **/ 
		enable_auto_refresh(FALSE,refresh_interval);
	}
	set_strict_ordering(strict_ordering_flag);
	if (address_mapping_policy != MEM_STATE_INVALID)
	set_pa_mapping_policy(address_mapping_policy);  
	if (row_buffer_management_policy != MEM_STATE_INVALID)
		set_dram_row_buffer_management_policy(row_buffer_management_policy);       	/* OPEN_PAGE, CLOSED_PAGE etc */
	if(chipset_delay > 1){				/* someone overrode default, so let us set the delay for them */
		set_chipset_delay(chipset_delay);
	}
	
	set_biu_delay(global_biu, biu_delay);
	set_dram_refresh_issue_policy(refresh_issue_policy);
	// Check that wang transaction selection policy is only with close page -
	// refer to David Wangs Phd Thesis - rank hopping algorithm.
	if (get_transaction_selection_policy(global_biu) == WANG && (
          get_dram_row_buffer_management_policy() == OPEN_PAGE ||
          (get_dram_type() != DDR2 && get_dram_type() != DDR3))) {
	  fprintf(stderr," Warning: The Wang transaction selection policy is only for close page systems \n");
	}
	
	/** FBDIMM : Initialize the buffers **/
	init_amb_buffer();

	convert_config_dram_cycles_to_mem_cycles();

	set_biu_debug(global_biu, biu_debug_flag);
	set_dram_debug(dram_debug_flag);
	set_transaction_debug(transaction_debug_flag);
	

	set_debug_tran_id_threshold(debug_tran_id_threshold);

	//       	
	set_wave_debug(wave_debug_flag);
	set_wave_cas_debug(cas_wave_debug_flag);
	set_bundle_debug(bundle_debug_flag);
	set_amb_buffer_debug(amb_buffer_debug_flag);

	if(all_debug_flag == TRUE){
		set_biu_debug(global_biu, TRUE);
		set_dram_debug(TRUE);
		set_transaction_debug(TRUE);
		set_wave_debug(FALSE);
	} else if (wave_debug_flag == TRUE){
		set_biu_debug(global_biu, FALSE);
		set_dram_debug(FALSE);
		//set_transaction_debug(FALSE);
		set_wave_debug(TRUE);
	}
	if (common_stats_fileout != NULL) 
	  mem_stat_init_common_file(common_stats_fileout);
	if (biu_stats)
	mem_gather_stat_init(GATHER_BUS_STAT,	0,	0	);
	if (bundle_stats){
	  mem_gather_stat_init(GATHER_BUNDLE_STAT,	0,	0	);
	  init_extra_bundle_stat_collector();
	}
	if (tran_queue_stats)
	mem_gather_stat_init(GATHER_TRAN_QUEUE_VALID_STAT,	0,	0	);
	if (biu_slot_stats)
	mem_gather_stat_init(GATHER_BIU_SLOT_VALID_STAT,	0,	0	);
	if (biu_access_dist_stats)
	mem_gather_stat_init(GATHER_BIU_ACCESS_DIST_STAT,	0,	0	);

	
	if (bank_hit_stats)
	mem_gather_stat_init(GATHER_BANK_HIT_STAT,	0,0);
	if (bank_conflict_stats)
	mem_gather_stat_init(GATHER_BANK_CONFLICT_STAT,0,0);
	if (cas_per_ras_stats)
	mem_gather_stat_init(GATHER_CAS_PER_RAS_STAT,0,0);

	if (arrival_distribution_model == UNIFORM_DISTRIBUTION) {
		arrival_thresh_hold = 1.0 - (1.0 / (double) average_interarrival_cycle_count);
	}
	if (arrival_distribution_model == GAUSSIAN_DISTRIBUTION) {
		arrival_thresh_hold = 1.0 - (1.0 / box_muller((double)average_interarrival_cycle_count, 10));
	}
    dram_dump_config(stdout);	
	max_cpu_time = max_inst;	/* This is a hack to leverage the -max:inst switch here for cycle count */
	current_cpu_time = 0;
	/*set_addr_debug(TRUE);*/
	/* record start of execution time, used in rate stats */ 
	{
	sim_start_time = time((time_t *)NULL);

	/* output simulation conditions */
	char *s = ctime(&sim_start_time);
	if (s[strlen(s)-1] == '\n')
	  s[strlen(s)-1] = '\0';
	fprintf(stdout, "\nDRAMsim: simulation started @ %s \n", s);
	}			  
	if(trace_input_flag == TRUE){
	 // next_e = (BUS_event_t *)calloc(sizeof(BUS_event_t),1);

	  next_e = get_next_bus_event(next_e,num_trace);
	  while(current_cpu_time < max_cpu_time && next_e!= NULL && next_e->attributes != MEM_STATE_INVALID){
		if(current_cpu_time > next_e->timestamp){
		  slot_id = find_free_biu_slot(global_biu, MEM_STATE_INVALID); 
		  fake_rid = slot_id;
		  if ( slot_id == MEM_STATE_INVALID){
		  } else {
			/* place request into BIU */
			fill_biu_slot(global_biu, 
				slot_id, 
				next_e->trace_id, 
				(tick_t) next_e->timestamp,
				fake_rid, 
				(unsigned int) next_e->address,
				next_e->attributes, 
				MEM_STATE_INVALID, 
				NULL,
				NULL);
			transaction_total++;
			get_next_event_flag = TRUE;
			next_e->already_run = TRUE;
		  }
		} 
		thread_id = MEM_STATE_INVALID;
		set_current_cpu_time(global_biu,(tick_t)current_cpu_time);
		//if(bus_queue_status_check(global_biu, thread_id) == BUSY){
		update_dram_system((tick_t)current_cpu_time);
		//}
		current_cpu_time = (int)get_current_cpu_time(global_biu);
		current_cpu_time++;
		/* look for loads that have already returned the critical word */
		for (i=0;i<num_trace;i++) {
		  slot_id = find_critical_word_ready_slot(global_biu, i);
		  if (slot_id != MEM_STATE_INVALID) {
			/* return data to the CPU.  Since there is no CPU, we don't care */
		  } else{
			/* find slots that's ready to retire, and retire it */
			slot_id = find_completed_slot(global_biu, i, (tick_t) current_cpu_time);
			if(slot_id != MEM_STATE_INVALID) {
			  release_biu_slot(global_biu, slot_id);
              transaction_retd_total++;
			}
		  }
		}
		if(get_next_event_flag == TRUE){
		  next_e = get_next_bus_event(next_e,num_trace);
		  get_next_event_flag = FALSE;
		}
         if (current_cpu_time % 1000000 == 0) {
          fprintf(stdout,"---------- TIME %llu TRANS SIM(%d) RETD(%d)-------------\n",current_cpu_time,transaction_total,transaction_retd_total);
#if 1
          if (last_retd_total == transaction_retd_total) {
            fprintf(stderr,"-- Error:DEADLOCK DETECTED -- \n");
            print_transaction_queue((tick_t)global_biu->mem2cpu_clock_ratio * current_cpu_time);
            exit(0);
          }
#endif
            last_retd_total = transaction_retd_total;
        }

	  }
	} else {

	  while(current_cpu_time < max_cpu_time){
		/* check and see if we need to create a new memory reference request */
		if( drand48() > arrival_thresh_hold){                   /* interarrival probability function */
		  //gaussian distribution function
		  if (arrival_distribution_model == GAUSSIAN_DISTRIBUTION) {
			arrival_thresh_hold = 1.0 - (1.0 / box_muller((double)average_interarrival_cycle_count, 10));
		  }
		  //poisson distribution function
		  else if (arrival_distribution_model == POISSON_DISTRIBUTION) {
			arrival_thresh_hold = 1.0 - (1.0 / poisson_rng((double)average_interarrival_cycle_count));
		  }


		  /* Inject new memory reference */
		  fake_access_type = get_mem_access_type(access_distribution);
		  if(fake_access_type == MEMORY_READ_COMMAND){
			fake_rid = 1 + (int)(drand48() * 100000);
		  } else {
			fake_rid = 0;
		  }
		  fake_v_address = (int)(drand48() * (unsigned int) (-1));

		  fake_p_address = fake_v_address;

		  slot_id = find_free_biu_slot(global_biu, MEM_STATE_INVALID);  /* make sure this has highest priority */
		  if ( slot_id == MEM_STATE_INVALID){
			/* can't get a free slot, retry later */
		  } else {
			/* place request into BIU */
			fill_biu_slot(global_biu, 
				slot_id, 
				0, 
				(tick_t) current_cpu_time,
				fake_rid, 
				(unsigned int) fake_p_address,
				fake_access_type, 
				MEM_STATE_INVALID, 
				NULL,
				NULL);
			transaction_total++;
		  }
		}
		thread_id = MEM_STATE_INVALID;
		set_current_cpu_time(global_biu,(tick_t)current_cpu_time);
		if(bus_queue_status_check(global_biu, thread_id) == BUSY){
		  //fprintf(stderr,"Calling update_dram_system %d\n",current_cpu_time);
		  update_dram_system((tick_t)current_cpu_time);
		}
		current_cpu_time = (int)get_current_cpu_time(global_biu);	
		current_cpu_time = current_cpu_time + 1;
		set_current_cpu_time(global_biu, current_cpu_time);

		/* look for loads that have already returned the critical word */
		slot_id = find_critical_word_ready_slot(global_biu, 0);
		if (slot_id != MEM_STATE_INVALID) {
		  /* return data to the CPU.  Since there is no CPU, we don't care */
		} else{
		  /* find slots that's ready to retire, and retire it */
		  slot_id = find_completed_slot(global_biu, 0, (tick_t) current_cpu_time);
		  if(slot_id != MEM_STATE_INVALID) {
			release_biu_slot(global_biu, slot_id);
            transaction_retd_total++;
		  }
		}
        if (current_cpu_time % 1000000 == 0) {
          fprintf(stdout,"---------- TIME %llu TRANS SIM(%d) RETD(%d)-------------\n",current_cpu_time,transaction_total,transaction_retd_total);
#if 1
          if (last_retd_total == transaction_retd_total) {
            fprintf(stderr,"-- DEADLOCK DETECTED -- \n");
            print_transaction_queue((tick_t)global_biu->mem2cpu_clock_ratio * current_cpu_time);
            exit(0);
          }
#endif
          last_retd_total = transaction_retd_total;
        }


		}
	  }
	{
	  sim_end_time = time((time_t *)NULL);

	  /* output simulation conditions */
	  char *s = ctime(&sim_end_time);
	  if (s[strlen(s)-1] == '\n')
		s[strlen(s)-1] = '\0';
	  fprintf(stdout, "\nDRAMsim: simulation ended @ %s \n", s);
	  int sim_elapsed_time = MAX(sim_end_time - sim_start_time, 1);
	  fprintf(stdout, "Simulation Duration %d\n",sim_elapsed_time);
	}

		fprintf(stdout,"---------- SIMULATION END -------------\n");
		fprintf(stdout,"---------- END TIME %llu -------------\n",current_cpu_time);
		fprintf(stdout,"---------- NUM TRANSACTIONS SIMULATED %u -------\n",transaction_total);
		fprintf(stdout,"---------- NUM IFETCH TRANSACTIONS %d ----------\n", get_num_ifetch());
		fprintf(stdout,"---------- NUM PREFETCH TRANSACTIONS %d ---------\n", get_num_prefetch()); 
		fprintf(stdout,"---------- NUM READ TRANSACTIONS %d -------------\n", get_num_read()); 
		fprintf(stdout,"---------- NUM WRITE TRANSACTIONS %d -------------\n", get_num_write()); 
		if (biu_stats) {
		  mem_print_stat(GATHER_BUS_STAT,false);
		}
		if (bundle_stats) {
		  mem_print_stat(GATHER_BUNDLE_STAT,false);
		  print_extra_bundle_stats(false);
		}
		if (tran_queue_stats) {
		  mem_print_stat(GATHER_TRAN_QUEUE_VALID_STAT,false);
		}
		if (biu_slot_stats) {
		  mem_print_stat(GATHER_BIU_SLOT_VALID_STAT,false);
		}
		if (biu_access_dist_stats) {
		  print_biu_access_count(global_biu,NULL);
		  mem_print_stat(GATHER_BIU_ACCESS_DIST_STAT,false);
		}

		fprintf(stdout,"----- POWER STATS ----\n");
		print_global_power_stats(stdout);
		print_transaction_queue((tick_t)global_biu->mem2cpu_clock_ratio * current_cpu_time);
		fcloseall();
		if (trace_input_flag) {
		  free(current_bus_event);
		}
		if (common_stats_fileout!=	NULL)
		  free (common_stats_fileout);
	return 0;
}

/** gaussian number generator ganked from the internet ->
 * http://www.taygeta.com/random/gaussian.html
 */

double box_muller (double m, double s) {
	double x1, x2, w, y1;
	static double y2;
	static int use_last = 0;

	if (use_last)		        /* use value from previous call */
	{
		y1 = y2;
		use_last = 0;
	}
	else
	{
		do {
			x1 = 2.0 * drand48() - 1.0;
			x2 = 2.0 * drand48() - 1.0;
			w = x1 * x1 + x2 * x2;
		} while ( w >= 1.0 );

		w = sqrt( (-2.0 * log( w ) ) / w );
		y1 = x1 * w;
		y2 = x2 * w;
		use_last = 1;
	}
	return floor(m+y1*s);
}

double gammaln(double xx) {
	double x, y, tmp, ser;
	static double cof[6] = {76.18009172947146, -86.50532032941677,
				24.01409824083091, -1.231739572450155,
				0.1208650973866179e-2, -0.5395239384953e-5};
	int j;

	y = x = xx;
	tmp = x + 5.5;
	tmp -= (x+0.5) * log(tmp);
	ser = 1.000000000190015;
	for (j=0; j<=5; j++) ser += cof[j]/++y;
	return -tmp + log(2.5066282746310005 * ser/x);
						
}
/**
 * from the book "Numerical Recipes in C: The Art of Scientific Computing"
 **/

double poisson_rng (double xm) {	
	static double sq, alxm, g, oldm = (-1.0);
    double em, t, y;

	if (xm < 12.0) {
		if (xm != oldm) {
			oldm = xm;
			g = exp(-xm);
		}
		em = -1;
		t = 1.0;
		do {
			++em;
			t *= drand48();
		} while (t > g);		
	}
	else {
		if (xm != oldm) {
			oldm = xm;
			sq = sqrt(2.0 * xm);
			alxm = log(xm);
			g = xm * alxm-gammaln(xm + 1.0);
		}
		do {
			do {
				y = tan(PI * drand48());
				em = sq * y + xm;
			} while (em < 0.0);
			em = floor(em);
			t = 0.9 * (1.0 + y*y) * exp(em * alxm - gammaln(em+1.0) -g);
		} while (drand48() > t);
	}
	return em;
}

int get_mem_access_type (float *access_distribution) {
	double random_value = drand48();
	int i = 0;
	while (i < MEMORY_ACCESS_TYPES_COUNT-3) {
		if (random_value < access_distribution[i]) {
			return i + 1;
		}
		i++;
	}
	return MEMORY_ACCESS_TYPES_COUNT - 2;
}
BUS_event_t *get_next_bus_event( BUS_event_t *this_e,const int num_trace){ 
	char 		input[1024]; 
	int		control;
	double		timestamp;
     
	int i;
	assert (current_bus_event != NULL);
	for (i=0;i<num_trace;i++) {
	  if (current_bus_event[i].already_run == TRUE) {
		if((fscanf(trace_fileptr[i],"%s",input) != EOF)){ 
		  sscanf(input,"%X",&(current_bus_event[i].address));  /* found starting Hex address */
		  if(fscanf(trace_fileptr[i],"%s",input) == EOF) {
			fprintf(stderr,"Unexpected EOF, Please fix input trace file \n");
			_exit(2);
		  } 
		  if((control = trace_file_io_token(input)) == UNKNOWN){
			fprintf(stderr,"Unknown Token Found [%s]\n",input);
		  } 
		  current_bus_event[i].attributes = control;

		  if(fscanf(trace_fileptr[i],"%s",input) == EOF) {
			fprintf(stderr,"Unexpected EOF, Please fix input trace file \n");
		  }
		  sscanf(input,"%lf",&timestamp);
		  current_bus_event[i].timestamp = (tick_t)timestamp;
		  current_bus_event[i].already_run = false;
		  current_bus_event[i].trace_id = i;
		} else {
		  current_bus_event[i].attributes = MEM_STATE_INVALID;
		}
	  }
	}
	this_e = NULL;
	 for (i=0;this_e == NULL && i<num_trace;i++) {
		if (current_bus_event[i].already_run == false && current_bus_event[i].attributes != MEM_STATE_INVALID) {
			this_e = &current_bus_event[i];
		}
	 }
	  for (i=0;i<num_trace;i++) {
		if (this_e->timestamp > current_bus_event[i].timestamp && current_bus_event[i].attributes != MEM_STATE_INVALID) {
		  this_e = &current_bus_event[i];
		}
	  }
	return this_e;
}

int trace_file_io_token(char *input){
	size_t length;
	length = strlen(input);
	if(strncmp(input, "FETCH",length) == 0) {
		return MEMORY_IFETCH_COMMAND;
	} else if (strncmp(input, "IFETCH",length) == 0) {
		return MEMORY_IFETCH_COMMAND;
	} else if (strncmp(input, "IREAD",length) == 0) {
		return MEMORY_IFETCH_COMMAND;
	} else if (strncmp(input, "P_FETCH",length) == 0) {
		return MEMORY_IFETCH_COMMAND;
	} else if (strncmp(input, "P_LOCK_RD",length) == 0) {
		return MEMORY_READ_COMMAND;                    
	} else if (strncmp(input, "P_LOCK_WR",length) == 0) {
		return MEMORY_WRITE_COMMAND;                    
	} else if (strncmp(input, "LOCK_RD",length) == 0) {
		return MEMORY_READ_COMMAND;                    
	} else if (strncmp(input, "LOCK_WR",length) == 0) {
		return MEMORY_WRITE_COMMAND;                    
	} else if (strncmp(input, "MEM_RD",length) == 0) {
		return MEMORY_READ_COMMAND;                    
	} else if (strncmp(input, "WRITE",length) == 0) {
		return MEMORY_WRITE_COMMAND;                    
	} else if (strncmp(input, "DWRITE",length) == 0) {
		return MEMORY_WRITE_COMMAND;                    
	} else if (strncmp(input, "MEM_WR",length) == 0) {
		return MEMORY_WRITE_COMMAND;                    
	} else if (strncmp(input, "READ",length) == 0) {
		return MEMORY_READ_COMMAND;                    
	} else if (strncmp(input, "DREAD",length) == 0) {
		return MEMORY_READ_COMMAND;                    
	} else if (strncmp(input, "P_MEM_RD",length) == 0) {
		return MEMORY_READ_COMMAND;                    
	} else if (strncmp(input, "P_MEM_WR",length) == 0) {
		return MEMORY_WRITE_COMMAND;                    
	} else if (strncmp(input, "P_I/O_RD",length) == 0) {
		return MEMORY_READ_COMMAND;                    
	} else if (strncmp(input, "P_I/O_WR",length) == 0) {
		return MEMORY_WRITE_COMMAND;                    
	} else if (strncmp(input, "IO_RD",length) == 0) {
		return MEMORY_READ_COMMAND;                    
	} else if (strncmp(input, "I/O_RD",length) == 0) {
		return MEMORY_READ_COMMAND;                    
	} else if (strncmp(input, "IO_WR",length) == 0) {
		return MEMORY_WRITE_COMMAND;                    
	} else if (strncmp(input, "I/O_WR",length) == 0) {
		return MEMORY_WRITE_COMMAND;                    
	} else if (strncmp(input, "P_INT_ACK",length) == 0) {
		return MEMORY_UNKNOWN_COMMAND;                    
	} else if (strncmp(input, "INT_ACK",length) == 0) {
		return MEMORY_UNKNOWN_COMMAND;                    
	} else if (strncmp(input, "BOFF",length) == 0) {
		return MEMORY_UNKNOWN_COMMAND;                    
	} else {
		printf("Unknown %s\n",input);
		return UNKNOWN;
	}
}

