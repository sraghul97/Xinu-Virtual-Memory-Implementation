# Xinu-Virtual-Memory-Implementation

This repository contains the implementation of virtual memory management and demand paging for the Xinu operating system as part of the ECE465/565 Operating Systems Design course. The project includes the development of system calls for virtual process creation and heap management, with a focus on understanding and applying paging, virtual memory, and interrupt handling concepts.

Key features include:

Implementation of vcreate, vmalloc, and vfree system calls.
Handling of segmentation faults and memory initialization.
Lazy allocation for virtual heap space with optional support for swapping.
Debugging utilities for tracking free and allocated memory pages.
Adherence to strict no-code reuse policies with a focus on incremental testing and debugging.
This project demonstrates a detailed understanding of memory management interactions between hardware and software and aims to enhance the Xinu OS's functionality with advanced paging and virtual memory features.
