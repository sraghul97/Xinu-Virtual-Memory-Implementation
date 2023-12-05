#include <xinu.h>

void pagefault_handler()
{
    // Need to allocate a physical page

    intmask mask = disable();

    unsigned long fault_addr;
    pd_t *pd;
    pt_t *pt;
    virt_addr_t vaddr_split;
    uint32 new_ffs_addr;
    uint32 new_swap_addr;
    uint32 curr_swap_addr;
    uint32 ffs_frame_evict;
    uint32 ffs_index;
    uint32 swap_index;
    pid32 pid;
    bool8 swap_of_evicted_page;

    // CR2 has the virtual address that caused the page fault
    fault_addr = read_cr2();

    write_pdbr((unsigned long)SYSTEM_PD_BASE);

    split_vaddr(&vaddr_split, fault_addr);
    pd = (pd_t*)((vaddr_split.pd_offset << 2) + (unsigned long)proctab[currpid].PDBR);
    pt = (pt_t*)((pd->pd_base << 12) + (vaddr_split.pt_offset << 2));

    if (!pd->pd_pres || !pt->pt_valid)
    {
        kprintf("P%d:: SEGMENTATION_FAULT\n", currpid);
        kill(currpid);
    }

    if (pt->pt_pres)
    {
        // Page exists, but still caused a page fault
        // Just return
        return;
    }

    if (pt->pt_swap == TRUE)
    {
        curr_swap_addr = pt->pt_base << 12;
    }

    // Get a new page from the FFS
    new_ffs_addr = find_free_FFS();

    if (new_ffs_addr == 0)
    {
        // FFS is full, need to evict

        // First decide which FFS frame to evict
        ffs_frame_evict = find_FFS_to_evict();
        ffs_index = (ffs_frame_evict - FFS_BASE)/PAGE_SIZE;

        // Need to find which process this FFS frame belongs to
        pid = FFS_pid_map[ffs_index];
        // FFS_pid_map[ffs_index] = currpid; // This frame belongs to currpid now

        if (FFS_Swap_map[ffs_index] != MAX_FFS_SIZE)
        {
            // This means it already has a frame in swap
            // No need to allocate new swap frame, just get the frame's address
            new_swap_addr = SWAP_BASE + FFS_Swap_map[ffs_index]*PAGE_SIZE;
            FFS_Swap_map[ffs_index] = MAX_FFS_SIZE;
        }
        else
        {
            // Find swap frame to move this FFS
            new_swap_addr = find_free_swap();

            if (new_swap_addr == 0)
            {
                kprintf("----- SWAP FULL -----\n");
                halt();
                return;
            }
        }        

        swap_of_evicted_page = perform_eviction(proctab[pid].PDBR, ffs_frame_evict, new_swap_addr);

        swap_index = (new_swap_addr - SWAP_BASE)/PAGE_SIZE;
        SWAP_pid_map[swap_index] = pid;

        pt->pt_base = ffs_frame_evict >> 12;

        #if DEBUG_SWAPPING
            if (proctab[currpid].debug_swap != 0)
            {
                if (swap_of_evicted_page == TRUE)
                {
                    kprintf("eviction:: FFS frame 0x%x, swap frame 0x%x\n", ffs_index, swap_index);
                }
                else
                {
                    kprintf("eviction:: FFS frame 0x%x, swap frame 0x%x copy\n", ffs_index, swap_index);
                }
                proctab[currpid].debug_swap--;
            }
        #endif
        
    }
    else
    {
        pt->pt_base = new_ffs_addr >> 12;
        ffs_index = (new_ffs_addr - FFS_BASE)/PAGE_SIZE;
    }

    // Page is on disk, need to move it to FFS
    if (pt->pt_swap == TRUE)
    {
        swap_index = (curr_swap_addr - SWAP_BASE)/PAGE_SIZE;
        FFS_Swap_map[ffs_index] = swap_index;

        #if DEBUG_SWAPPING
            if (proctab[currpid].debug_swap != 0)
            {
                kprintf("swapping:: swap frame 0x%x, FFS frame 0x%x\n", swap_index, ffs_index);
                proctab[currpid].debug_swap--;
            }
        #endif
    }
    
    
    pt->pt_pres = 1;

    // ffs_index = (new_ffs_addr - FFS_BASE)/PAGE_SIZE;

    FFS_pid_map[ffs_index] = currpid;
    FFS_to_PTE[ffs_index] = pt;

    write_pdbr((unsigned long)proctab[currpid].PDBR);
    restore(mask);
}