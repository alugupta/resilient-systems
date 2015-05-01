/*
 * mem-interface.h - Routines that interface the DRAM memory system to a
 * processor simulator.
 *
 *  This is released as part of the sim-dram infrastructure for power/timing
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

#include "options.h"
#include "memory.h"
#include "cache.h"

//#define SIM_MASE
//#define ALPHA_SIM

#define MEM_INVALID -1	/* Sometime this has to MEM_STATE_INVALID */
#define RID_INVALID                     -1
#if 0
/* Registers dram related options. */
void dram_reg_options(struct opt_odb_t *odb);

/* Checks dram related options. */
void dram_check_options(struct opt_odb_t *odb);
#endif
/* Initiates a new memory request. */
mem_status_t
dram_access_memory(unsigned int cmd,
		   md_addr_t baddr, 
		   tick_t now, 
		   dram_access_t *access);

/* Updates the DRAM memory system, called each cycle.  Replaces dram_cb_check. */
#ifdef ALPHA_SIM
#define bool int
#define true 1
#define false 0
#endif
bool dram_update_system(tick_t now);

/* Prints the dram stats, called from sim_aux_stats. */
void dram_print_stats_and_close();

/* Squashes bid entry from memory system. */
void dram_squash_entry(int rid);

/* Checks for MEM_RETRY condition */
#ifdef ALPHA_SIM
int dram_mem_system_tickle(struct cache *l1, struct cache *l2, md_addr_t baddr);
#else
#ifdef SIM_MASE
int dram_mem_system_tickle(struct cache_t *l1, struct cache_t *l2, md_addr_t baddr);
#endif
#endif

#if 0
void dram_dump_config(FILE *stream);
#endif

extern int mem_use_dram;

