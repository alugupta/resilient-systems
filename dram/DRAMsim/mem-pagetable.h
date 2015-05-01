/*
 * mem-pagetable.h Header file originally written by
 * Yongkui Han and modified by David Wang
 *
 * Page Table implementation. this is based on the 
 * implementation in Duke university. It implements a basic Sequential page 
 * allocation and random page allocation. and all new page allocation algorithm 
 * can be defined and added in the page_access() function.
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

#ifndef _PAGE_TABLE_H_
#define _PAGE_TABLE_H_

/* This section defines the virtual address mapping policy */
/*
VA_EQUATE                                               physical == virtual 
VA_SEQUENTIAL                                           sequential physical page assignment
VA_RANDOM                       
VA_PER_BANK                   
VA_PER_RANK                    
*/
#define		VA_EQUATE		0	/* physical == virtual */
#define		VA_SEQUENTIAL		1	/* sequential physical page assignment */
#define		VA_RANDOM		2	/* random physical page assignment */
#define		VA_PER_BANK		3
#define		VA_PER_RANK		4

/* md_addr_t is 32bits for ARM, and 64bits for Alpha */
#define md_addr_t unsigned int

/* maximal number of PTEs(page table entry).
   if page size is 4KB, we can support 12+20=32 bit virtual address
   if page size is 8KB, we can support 13+20=33 bit virtual address */
#define MAX_MEM_TABLE_SIZE 	0x100000

#if 0
#define MD_PAGE_SIZE            4096

/* this is >>12, because page_size is 4KB. if it is 8KB, then here we need >>13 */
#define MEM_BLOCK(addr)         ((((addr)) >> 12) & 0xfffff)
/* I'd better rewrite this macro, adaptive to the page_size in mmu_t */

/* compute address of access within a host page */
#define MEM_OFFSET(ADDR)        ((ADDR) & (MD_PAGE_SIZE - 1))

//2^(12+19)=2GB maximal main memory size
//#define MAX_MEM_PAGE_NUM	0x80000
#endif

/* this is the struct for PTE(Page Table Entry). */
typedef struct PhyPage {
 	int vblock;		/* virtual block */
 	tick_t access_cycle;	/* this should be the latest access time. it is a hack to use access_count for it. */
} phy_page_t;

typedef struct mmu_t {			/* fictitious global universal memory management unit */
					/* Sits right before BIU.  VA goes in, PA comes out */
	int	va_mapping_policy;	/* aka page allocation policy.  */
	int     page_size;              /* make this a variable. Users may #define it to MD_PAGE_SIZE here */
        int     log2_page_size;

	phy_page_t	*page_table;	/* physical page table, indexed by physical block #, so it is an inverted page table */

	/* physical block of PTE(Page Table Entry) */
	int     phy_mem_table[MAX_MEM_TABLE_SIZE];      /* indexed by virtual block # */
        int     mem_table_size;                         /* mem_table size */
        int     log2_mem_table_size;

	int	mem_size;		/* main memory size */
	int 	page_num;		/* # of physical pages */
	int     phy_mem_counter;        /* # of physical pages that have been allocated */

	tick_t 	access_count;		/* # of accesses to the page table */
	tick_t	hits;			/* # of hits to the page table */
	tick_t	misses;			/* # of misses to the page table */
	tick_t 	replacements;		/* # of page replacements */
	md_addr_t	last_addr;	/* last virtual address */
	int	last_block;		/* last physical block */
} mmu_t;

unsigned int mem_block(mmu_t *, md_addr_t);
unsigned int mem_offset(mmu_t *, md_addr_t);
void page_init(mmu_t *, int);
mmu_t *get_mmu_address();
unsigned int  phy_address(mmu_t *, md_addr_t);
void page_access(mmu_t *, md_addr_t);
int num_free_mem_page(mmu_t *);
void print_page_table(mmu_t *);
void print_mmu_stat(mmu_t *);

#endif
