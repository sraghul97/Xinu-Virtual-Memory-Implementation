#include <xinu.h>

// Goes through the PDPT Free list and returns the index of the first available page
uint32 find_free_PDPT()
{
    uint32 i;

    for(i = 0; i < MAX_PT_SIZE; i++)
    {
        if (PDPT_FL[i] == TRUE)
        {
            PDPT_FL[i] = FALSE;
            // kprintf("new PDPT index=%d ", i);
            return PDPT_BASE + i*PAGE_SIZE;
        }
    }

    return 0;
}

uint32 find_free_FFS()
{
    uint32 i;

    for(i = 0; i < MAX_FFS_SIZE; i++)
    {
        if (FFS_FL[i] == TRUE)
        {
            FFS_FL[i] = FALSE;
            return FFS_BASE + i*PAGE_SIZE;
        }
    }

    // kprintf("--------FAILED to allocate FFS pages\n--------");
    return 0;
}

uint32 find_free_swap()
{
    uint32 i;

    for(i = 0; i < MAX_SWAP_SIZE; i++)
    {
        if (SWAP_FL[i] == TRUE)
        {
            SWAP_FL[i] = FALSE;
            return SWAP_BASE + i*PAGE_SIZE;
        }
    }

    // kprintf("--------FAILED to allocate FFS pages\n--------");
    return 0;
}

void write_pdbr(unsigned long value)
{
    unsigned long temp =  read_cr3();
	temp = (temp & 0x00000FFF) | ( (value) & 0xFFFFF000);
	write_cr3(temp);
}

uint32 free_ffs_pages()
{
    uint32 i;
    uint32 free_pages = 0;

    for(i = 0; i < MAX_FFS_SIZE; i++)
    {
        if (FFS_FL[i] == TRUE)
        {
            free_pages++;
        }
    }

    return free_pages;
}

uint32 free_swap_pages()
{
    uint32 i;
    uint32 free_pages = 0;

    for(i = 0; i < MAX_SWAP_SIZE; i++)
    {
        if (SWAP_FL[i] == TRUE)
        {
            free_pages++;
        }
    }

    return free_pages;
}

uint32 used_ffs_frames(pid32 pid)
{
    intmask mask = disable();

    // Only the system process has access to the PDPT area
    write_pdbr((unsigned long)SYSTEM_PD_BASE);

    // Go through the page table and find how many entries have their present bit set
    pd_t *pd;
    pt_t *pt;
    int i;
    int j;
    uint32 used_pages = 0;

    pd = (pd_t*)(unsigned long)proctab[pid].PDBR;

    for (i = 8; i < PAGE_SIZE/4; i++)
    {
        if (pd[i].pd_pres)
        {
            pt = (pt_t*)(pd[i].pd_base << 12);
            for (j = 0; j < PAGE_SIZE/4; j++)
            {
                if (pt[j].pt_pres)
                {
                    used_pages++;
                }
            }
        }
    }

    write_pdbr((unsigned long)proctab[currpid].PDBR);

    restore(mask);
    
    return used_pages;
}

uint32 allocated_virtual_pages(pid32 pid)
{
    intmask mask = disable();

    // Only the system process has access to the PDPT area
    write_pdbr((unsigned long)SYSTEM_PD_BASE);

    // Go through the page table and find how many entries have their valid bit set
    pd_t *pd;
    pt_t *pt;
    int i;
    int j;
    uint32 num_virtual_pages = 0;

    pd = (pd_t*)(unsigned long)proctab[pid].PDBR;

    for (i = 0; i < PAGE_SIZE/4; i++)
    {
        if (pd[i].pd_pres)
        {
            pt = (pt_t*)(pd[i].pd_base << 12);
            for (j = 0; j < PAGE_SIZE/4; j++)
            {
                if (pt[j].pt_valid)
                {
                    num_virtual_pages++;
                }
            }
        }
    }

    write_pdbr((unsigned long)proctab[currpid].PDBR);

    restore(mask);
    
    return num_virtual_pages;
}

void init_pde(pd_t *pde, unsigned int pres, unsigned int base)
{
	pde->pd_pres = pres;
	pde->pd_write = 1;
	pde->pd_user = 1;
	pde->pd_pwt = 1;
	pde->pd_pcd = 1;
	pde->pd_acc = 0;
	pde->pd_mbz = 0;
	pde->pd_fmb = 1;
	pde->pd_global = 1;
	pde->pd_avail = 0;
	pde->pd_base = base;
}

void init_pte(pt_t *pte, unsigned int pres, unsigned int valid, unsigned int base)
{
	pte->pt_pres = pres;
	pte->pt_write = 1;
	pte->pt_user = 1;
	pte->pt_pwt = 1;
	pte->pt_pcd = 1;
	pte->pt_acc = 0;
	pte->pt_dirty = 1;
	pte->pt_mbz = 0;
	pte->pt_global = 0;
	pte->pt_avail = 0;
    pte->pt_swap = 0;
    pte->pt_valid = valid;
	pte->pt_base = base;
}

void split_vaddr(virt_addr_t *vaddr_split, unsigned int vaddr)
{
    vaddr_split->pg_offset = ((1 << 12) - 1) & vaddr;
    vaddr_split->pt_offset = ((((1 << 10) - 1) << 12) & vaddr) >> 12;
    vaddr_split->pd_offset = ((((1 << 10) - 1) << 22) & vaddr) >> 22;
}

void dealloc_pdptffs(pd_t* pd_addr)
{
    pd_t *pd;
    pt_t *pt;
    uint32 i;
    uint32 j;
    uint32 index;

    write_pdbr((unsigned long)SYSTEM_PD_BASE);

    // kprintf("Killing pid%d pd_addr=%x\n", currpid, pd_addr);

    pd = pd_addr;

    for(i = 8; i < PAGE_SIZE/4; i++)
    {
        if (pd[i].pd_pres)
        {
            pt = (pt_t*)(pd[i].pd_base << 12);

            for (j = 0; j < PAGE_SIZE/4; j++)
            {
                // Free FFS
                if (pt[j].pt_pres)
                {
                    index = ((pt[j].pt_base << 12) - FFS_BASE)/PAGE_SIZE;
                    FFS_FL[index] = TRUE;
                    FFS_pid_map[index] = NPROC;
                    FFS_Swap_map[index] = MAX_FFS_SIZE;
                    // kprintf("FFS index kill=%d\n", index);
                }
                pt[j].pt_pres = 0;
                pt[j].pt_valid = 0;
            }

            index = ((unsigned long)pt - PDPT_BASE)/PAGE_SIZE;
            // kprintf("PT Index=%d\n", index);
            PDPT_FL[index] = TRUE;
            
        }
    }

    index = ((unsigned long)pd - PDPT_BASE)/PAGE_SIZE;
    PDPT_FL[index] = TRUE;
    // kprintf("PD Index=%d\n", index);

    // write_pdbr((unsigned long)proctab[currpid].PDBR);

}

void free_swap_space(pid32 pid)
{
    uint32 i;

    for (i = 0; i < MAX_SWAP_SIZE; i++)
    {
        if (SWAP_pid_map[i] == pid)
        {
            SWAP_FL[i] = TRUE;
            SWAP_pid_map[i] = NPROC;
        }
    }
}

uint32 find_FFS_to_evict()
{
    uint32 i;
    pt_t *pt;
    
    while(1)
    {
        if (next_FFS_frame_to_check == 0)
        {
            for(i = 0; i < MAX_FFS_SIZE; i++)
            {
                pt = FFS_to_PTE[i];
                if (pt->pt_acc == TRUE)
                {
                    pt->pt_acc = FALSE;
                }
                else
                {
                    // pt->pt_acc = TRUE;
                    next_FFS_frame_to_check = (i + 1) % MAX_FFS_SIZE;
                    // kprintf("next_frame0 = %d, i=%d  %d\n", next_FFS_frame_to_check, i, (i + 1) % MAX_FFS_SIZE);
                    return FFS_BASE + i*PAGE_SIZE;
                }
            }
        }
        else
        {
            for(i = next_FFS_frame_to_check; i < MAX_FFS_SIZE; i++)
            {
                pt = FFS_to_PTE[i];
                if (pt->pt_acc == TRUE)
                {
                    pt->pt_acc = FALSE;
                }
                else
                {
                    // pt->pt_acc = TRUE;
                    next_FFS_frame_to_check = (i + 1) % MAX_FFS_SIZE;
                    // kprintf("next_frame1 = %d, i = %d\n", next_FFS_frame_to_check, i);
                    return FFS_BASE + i*PAGE_SIZE;
                }
            }

            for(i = 0; i < next_FFS_frame_to_check; i++)
            {
                pt = FFS_to_PTE[i];
                if (pt->pt_acc == TRUE)
                {
                    pt->pt_acc = FALSE;
                }
                else
                {
                    // pt->pt_acc = TRUE;
                    next_FFS_frame_to_check = (i + 1) % MAX_FFS_SIZE;
                    // kprintf("next_frame2 = %d i=%d\n", next_FFS_frame_to_check, i);
                    return FFS_BASE + i*PAGE_SIZE;
                }
            }
        }
    }

    return 0;
}

bool8 perform_eviction(pd_t *pd, uint32 page_addr, uint32 swap_addr)
{
    // Set this page's present bit to 0
    // Its swap bit to 1
    // And its base to point to the frame in stack

    uint32 i;
    uint32 j;
    pt_t *pt;

    bool8 swap;


    for (i = 0; i < PAGE_SIZE/4; i++)
    {
        if(pd[i].pd_pres)
        {
            pt = (pt_t*)(pd[i].pd_base << 12);
            for (j = 0; j < PAGE_SIZE/4; j++)
            {
                if ((pt[j].pt_base << 12) == page_addr)
                {
                    swap = pt[j].pt_swap;
                    pt[j].pt_pres = 0;
                    pt[j].pt_swap = 1;
                    pt[j].pt_base = swap_addr >> 12;
                }
            }
        }
    }

    return swap;
}