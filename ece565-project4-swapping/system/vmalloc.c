#include <xinu.h>

uint32 find_free_vaddress(uint32 num_pages_required)
{
    uint32 vaddr;
    uint32 free_page_counter = 0;

    pd_t *pd;
    pt_t *pt;

    virt_addr_t vaddr_split;

    // Only the system process has access to the PDPT area
    write_pdbr((unsigned long)SYSTEM_PD_BASE);

    for(vaddr = VHEAP_START; vaddr >= VHEAP_START; vaddr+=PAGE_SIZE)
    {
        // if (currpid == 18 && (vaddr >= 0xFFFF0000))
        // {kprintf("Loop with 0x%x\n", vaddr);}
        split_vaddr(&vaddr_split, vaddr);

        pd = (pd_t*)((vaddr_split.pd_offset << 2) + (unsigned long)proctab[currpid].PDBR);
        pt = (pt_t*)((pd->pd_base << 12) + (vaddr_split.pt_offset << 2));

        // kprintf("pt at %x, pd_pres=%d, pt_valid=%d\n", pt, pd->pd_pres, pt->pt_valid);

        if (!pd->pd_pres || !pt->pt_valid)
        {
            free_page_counter++;

            if (free_page_counter == num_pages_required)
            {
                write_pdbr((unsigned long)proctab[currpid].PDBR);
                return (vaddr - (num_pages_required-1)*PAGE_SIZE);
            }
        }
        else
        {
            free_page_counter = 0;
        }
    }

    write_pdbr((unsigned long)proctab[currpid].PDBR);
    return 0;
}

uint32 set_pdpt_to_valid(uint32 begin_address, uint32 num_pages_required)
{
    uint32 i, j;
    pd_t *pd;
    pt_t *pt;
    virt_addr_t vaddr_split;

    write_pdbr((unsigned long)SYSTEM_PD_BASE);
    
    for(i = 0; i < num_pages_required; i++)
    {
        
        // kprintf("Allocating %x\n", begin_address);
        split_vaddr(&vaddr_split, begin_address);
        pd = (pd_t*)((vaddr_split.pd_offset << 2) + (unsigned long)proctab[currpid].PDBR);

        if (pd->pd_pres)
        {
            pt = (pt_t*)((pd->pd_base << 12) + (vaddr_split.pt_offset << 2));
        }
        else
        {
            // Create new PT
            pt = (pt_t*)find_free_PDPT();

            if (pt == 0)
            {
                // kprintf("SYSERROR: vmalloc\n");
                return 0;
            }

            for (j = 0; j < PAGE_SIZE/4; j++)
            {
                // kprintf("Init0:%x\n", &pt[j]);
                init_pte(&pt[j], 0, 0, 0);
            }

            init_pde(pd, 1, (unsigned int)pt >> 12);
        }
        init_pte(pt, 0, 1, 0);
        begin_address = begin_address+PAGE_SIZE;

    }

    write_pdbr((unsigned long)proctab[currpid].PDBR);

    return 1;
}

char* vmalloc (uint32 nbytes)
{
    // Go through the virtual address space of the process and find enough consecutive pages
    //  to allocate to this process

    intmask mask = disable();

    uint32 num_pages_required = 
        (((nbytes) % PAGE_SIZE) == 0) ? (nbytes/PAGE_SIZE) : (nbytes/PAGE_SIZE) + 1;

    uint32 begin_address;

    begin_address = find_free_vaddress(num_pages_required);

    if (begin_address == 0)
    {
        // kprintf("-------VMALLOC FAILED -------\n");
        return (char*)SYSERR;
    }

    if (set_pdpt_to_valid(begin_address, num_pages_required) == 0)
    {
        return (char*)SYSERR;
    }

    restore(mask);

    return (char*)begin_address;
}