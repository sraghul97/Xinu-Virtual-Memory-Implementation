#include <xinu.h>

syscall vfree (char* ptr, uint32 nbytes)
{
    intmask mask = disable();
    
    uint32 i;
    uint32 begin_address;
    uint32 num_pages_to_free;
    pd_t *pd;
    pt_t *pt;
    virt_addr_t vaddr_split;
    uint32 index;
    
    begin_address = (uint32)ptr;

    num_pages_to_free = 
        (((nbytes) % PAGE_SIZE) == 0) ? (nbytes/PAGE_SIZE) : (nbytes/PAGE_SIZE) + 1;


    write_pdbr((unsigned long)SYSTEM_PD_BASE);

    // First need to check if it is possible to free all the pages requested to be freed
    for(i = 0; i < num_pages_to_free; i++)
    {
        split_vaddr(&vaddr_split, begin_address);

        pd = (pd_t*)((vaddr_split.pd_offset << 2) + (unsigned long)proctab[currpid].PDBR);

        if (pd->pd_pres)
        {
            pt = (pt_t*)((pd->pd_base << 12) + (vaddr_split.pt_offset << 2));
            
            if (!pt->pt_valid)
            {
                write_pdbr((unsigned long)proctab[currpid].PDBR);
                return SYSERR;
            }
        }
        else
        {
            write_pdbr((unsigned long)proctab[currpid].PDBR);
            return SYSERR;
        }

        begin_address = begin_address+PAGE_SIZE;
    }

    // Perform actual freeing of pages
    begin_address = (uint32)ptr;

    for(i = 0; i < num_pages_to_free; i++)
    {
        split_vaddr(&vaddr_split, begin_address);

        pd = (pd_t*)((vaddr_split.pd_offset << 2) + (unsigned long)proctab[currpid].PDBR);
        pt = (pt_t*)((pd->pd_base << 12) + (vaddr_split.pt_offset << 2));
        
        pt->pt_valid = 0;

        // Free physical page and update free list
        
        if (pt->pt_pres)
        {
            index = ((pt->pt_base << 12) - FFS_BASE)/PAGE_SIZE;
            FFS_FL[index] = TRUE;
            FFS_pid_map[index] = NPROC;
            FFS_Swap_map[index] = MAX_FFS_SIZE;
            // kprintf("FFS index=%d\n", index);
        }

        pt->pt_pres = 0;
        

        begin_address = begin_address+PAGE_SIZE;
    }

    restore(mask);
    
    write_pdbr((unsigned long)proctab[currpid].PDBR);
    return OK;
}