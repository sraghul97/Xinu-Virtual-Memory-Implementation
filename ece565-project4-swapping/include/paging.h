/* paging.h */

#ifndef _PAGING_H_
#define _PAGING_H_

#define DEBUG_SWAPPING 1

/* Macros */
#define XINU_PAGES      8192    /* number of pages used by default by Xinu               */
#define PAGE_SIZE       4096    /* number of bytes per page                              */
#define MAX_FFS_SIZE    (16*1024) /* size of FFS space  (in frames)                        */
#define MAX_SWAP_SIZE   (32*1024) /* size of swap space  (in frames)                       */
#define MAX_PT_SIZE     1024    /* size of space used for page tables (in frames)        */


#define PDPT_BASE (XINU_PAGES * PAGE_SIZE)
#define VHEAP_START PDPT_BASE
#define SYSTEM_PD_BASE (PDPT_BASE + PAGE_SIZE*9)
#define FFS_BASE (XINU_PAGES * PAGE_SIZE + MAX_PT_SIZE * PAGE_SIZE)
#define SWAP_BASE (FFS_BASE + MAX_FFS_SIZE * PAGE_SIZE)

/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 1;		/* for programmer's use		*/
  unsigned int pt_swap : 1;		/* If this page was moved to swap		*/
  unsigned int pt_valid : 1;    /* If a valid PTE exists */
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

/* Structure for a virtual address */

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

/* Structure for a physical address */

typedef struct{
  unsigned int fm_offset : 12;		/* frame offset			*/
  unsigned int fm_num : 20;		/* frame number			*/
} phy_addr_t;

// Free lists - One for the PDPT space and one for FFS
bool8 PDPT_FL[MAX_PT_SIZE];   /* If 0, the space is not available, if 1, it is available */
bool8 FFS_FL[MAX_FFS_SIZE];   /* If 0, the space is not available, if 1, it is available */
bool8 SWAP_FL[MAX_SWAP_SIZE]; /* If 0, the space is not available, if 1, it is available */

// Mapping between FFS address and PTE
pt_t *FFS_to_PTE[MAX_FFS_SIZE];

// Mapping between swap frames and pid
pid32 SWAP_pid_map[MAX_SWAP_SIZE];

uint32 next_FFS_frame_to_check;

// Mapping between FFS frame and pid
pid32 FFS_pid_map[MAX_FFS_SIZE];

// Mapping between FFS and Swap
uint32 FFS_Swap_map[MAX_FFS_SIZE];

// Number of debug messages to print
uint32 debug_swapping;

/* Functions to manipulate control registers and enable paging (see control_reg.c)	 */

unsigned long read_cr0(void);

unsigned long read_cr2(void);

unsigned long read_cr3(void);

unsigned long read_cr4(void);

void write_cr0(unsigned long n);

void write_cr3(unsigned long n);

void write_cr4(unsigned long n);

void enable_paging();

#endif
