#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384 // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8 // Shift page number this much

// Simulated RAM
unsigned char mem[MEM_SIZE];

int free_page_count = PAGE_COUNT - 1; // Zero page is always used

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Return the process's page table page number
//
int get_page_table(int proc_num)
{
    return mem[PAGE_COUNT + proc_num]; // Zero page process page table number table starts at index PAGE_COUNT
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    // Sets all values of free page table to 0 (all pages not in use)
    mem[0] = 1; // set zero page to in use

    for (int i = 1; i > MEM_SIZE; i++)
    {
        mem[i] = 0;
    }
}

//
// Allocate a physical page
//
// Returns the number of the page, or 0xff if no more pages available
//
unsigned char find_page(void)
{
    // Loop free page map, return first free page (start on one because zero page is always in use)
    for (unsigned char i = 1; i < PAGE_COUNT; i++)
    {
        if (mem[i] == 0)
        {
            mem[i] = 1;
            free_page_count--;
            return i;
        }
    }
    return 0xff;
}

//
// Allocate pages for a new process
//
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
    if (free_page_count < page_count)
    {
        printf("Could not allocate space for process #%d\n", proc_num);
        return;
    }

    int proc_page_table_num = find_page(); 
    mem[PAGE_COUNT + proc_num] = proc_page_table_num; // Store new processes page table number in the page table map

    for (int virtual_page_num = 0; virtual_page_num < page_count; virtual_page_num++)
    {
        int proc_data_page_num = find_page(); // Get a new page for each data page
        mem[get_address(proc_page_table_num, virtual_page_num)] = proc_data_page_num; // Go to the process's page table address, and store the process's data_page numbers in the page table
    }
}

//
// Sets the zero page to mark page page_num as free
//
void deallocate_page(int page_num)
{
    mem[page_num] = 0;
}

//
// Sets a process's data pages and page table to free
//
void kill(int proc_num)
{
    int process_page_table = get_page_table(proc_num);
    for (int offset = 0; offset < PAGE_SIZE; offset++)
    {
        int data_page_address = get_address(process_page_table, offset);
        if (mem[data_page_address] != 0) // If the page is not free
        {                                               // If there is an entry at this location in the page table
            int data_page_num = mem[data_page_address]; // Get data_page number
            deallocate_page(data_page_num); // Free the page
            mem[data_page_address] = 0;     // Remove entry from the page_table
        }
    }
    deallocate_page(process_page_table);
}

//
// Returns physical address from a process's virtual addres
//
int get_physical_address(int process_num, int virtual_address)
{
    int virtual_page = virtual_address >> 8;           // Find the virtual page (Top bits)
    int page_table_address = get_address(get_page_table(process_num), 0); // Get the physical address to the process's page table

    int physical_page_num = mem[page_table_address + virtual_page]; // Look up the virtual page's physical number in the process's page table
    int offset = virtual_address & 255; // Find offset within data page (low bits)  

    return get_address(physical_page_num, offset); // Return the address to the physical page + the offset
}

//
// Stores the value a a process's virtual address
//
void store_value(int proc_num, int virtual_address, int value)
{
    int address = get_physical_address(proc_num, virtual_address);
    mem[address] = value;

    printf("Store proc %d: %d => %d, value=%d\n",
           proc_num, virtual_address, address, value);
}

//
// Returns the value at a process's virtual address
//
unsigned char read_value(int proc_num, int virtual_address)
{
    int address = get_physical_address(proc_num, virtual_address);

    printf("Load proc %d: %d => %d, value=%d\n",
           proc_num, virtual_address, address, mem[address]);

    return mem[address];
}

//
// Print the free page map
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++)
    {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0 ? '.' : '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);
    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++)
    {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0)
        {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1)
    {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }

    initialize_mem();

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "np") == 0)
        {
            int proc_num = atoi(argv[++i]);
            int pages = atoi(argv[++i]);
            new_process(proc_num, pages);
        }
        else if (strcmp(argv[i], "pfm") == 0)
        {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0)
        {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "kp") == 0)
        {
            int proc_num = atoi(argv[++i]);
            kill(proc_num);
        }
        else if (strcmp(argv[i], "sb") == 0)
        {
            int proc_num = atoi(argv[++i]);
            int virt_add = atoi(argv[++i]);
            int val = atoi(argv[++i]);
            store_value(proc_num, virt_add, val);
        }
        else if (strcmp(argv[i], "lb") == 0)
        {
            int proc_num = atoi(argv[++i]);
            int virt_addr = atoi(argv[++i]);
            read_value(proc_num, virt_addr);
        }
    }
}
