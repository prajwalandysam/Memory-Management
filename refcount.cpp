
/********************************************************************************************
 CSL 316 - Assignment 1- Semester 6 2024
 Progammer: Prajwal Sam Rachapudy
 Date Due: 24th Jan 2024
 Purpose: This C++ program implements a simple memory manager that handles memory allocation,
 deallocation, and compaction. It uses a list of memory blocks to manage both used and free memory
 spaces.
 The memory manager reads commands from an input file to allocate, deallocate, and assign memory.
****************************************************************************************************/

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <algorithm>

#define MEMORY_SIZE_MB 64
#define MEMORY_SIZE 250

struct MemoryBlock
{
    std::string var_name;
    int start_address;
    int size;
    int reference_count;
};

struct MemoryManager
{
    std::vector<MemoryBlock> used_blocks;
    std::vector<MemoryBlock> free_blocks;
};

MemoryManager create_memory_manager()
{
    MemoryManager manager;
    manager.free_blocks.push_back({"", 0, MEMORY_SIZE, 0});
    return manager;
}
void compact_memory(MemoryManager &manager)
{
    std::sort(manager.used_blocks.begin(), manager.used_blocks.end(),
              [](const MemoryBlock &a, const MemoryBlock &b)
              {
                  return a.start_address < b.start_address;
              });

    size_t current_address = 0;
    for (auto &block : manager.used_blocks)
    {
        block.start_address = static_cast<int>(current_address);
        current_address += block.size;
    }

    // Update free blocks
    manager.free_blocks.clear(); // Clear existing free blocks

    // Find gaps between used blocks and create new free blocks
    for (size_t i = 0; i < manager.used_blocks.size() - 1; ++i)
    {
        size_t gap_size = manager.used_blocks[i + 1].start_address - (manager.used_blocks[i].start_address + manager.used_blocks[i].size);
        if (gap_size > 0)
        {
            manager.free_blocks.push_back({"", static_cast<int>(manager.used_blocks[i].start_address + manager.used_blocks[i].size), static_cast<int>(gap_size), 0});
        }
    }

    // Add the remaining memory as a free block
    size_t remaining_memory = MEMORY_SIZE - current_address;
    if (remaining_memory > 0)
    {
        manager.free_blocks.push_back({"", static_cast<int>(current_address), static_cast<int>(remaining_memory), 0});
    }
}
MemoryBlock *allocate_memory(MemoryManager &manager, int size, std::string name)
{
    for (auto it = manager.free_blocks.begin(); it != manager.free_blocks.end(); ++it)
    {
        if (it->size >= size)
        {
            MemoryBlock allocated_block;
            allocated_block.var_name = name;
            allocated_block.start_address = it->start_address;
            allocated_block.size = size;
            allocated_block.reference_count = 1;

            // Update free blocks
            int remaining_size = it->size - size;
            if (remaining_size > 0)
            {
                // If there is remaining space, create a new free block
                it->start_address += size;
                it->size = remaining_size;
            }
            else
            {
                // Remove the block from the free list
                it = manager.free_blocks.erase(it);
            }

            // Update used blocks
            manager.used_blocks.push_back(allocated_block);

            return &(manager.used_blocks.back());
        }
    }
    std::cout << "\033[33mAttempting to compact memory due to insufficient space for allocation.\033[0m\n";

    compact_memory(manager); // Compact the memory to try to make space

    // Try allocation again after compaction
    for (auto it = manager.free_blocks.begin(); it != manager.free_blocks.end(); ++it)
    {
        if (it->size >= size)
        {
            MemoryBlock allocated_block;
            allocated_block.var_name = name;
            allocated_block.start_address = it->start_address;
            allocated_block.size = size;
            allocated_block.reference_count = 1;

            // Update free blocks
            int remaining_size = it->size - size;
            if (remaining_size > 0)
            {
                it->start_address += size;
                it->size = remaining_size;
            }
            else
            {
                it = manager.free_blocks.erase(it);
            }

            // Update used blocks
            manager.used_blocks.push_back(allocated_block);

            return &(manager.used_blocks.back());
        }
    }

    // If allocation still fails after compaction
    std::cerr << "\033[31mAllocation failed: Not enough space even after compaction.\033[0m\n";

    return nullptr; // Allocation failed
}

void deallocate_memory(MemoryManager &manager, std::string name) {
    for (auto it = manager.used_blocks.begin(); it != manager.used_blocks.end();) {
        if (it->var_name == name) {
            if (it->size == 0) { // Check if it's a pointer
                // Find the original block that the pointer points to
                for (auto &block : manager.used_blocks) {
                    if (block.start_address == it->start_address && block.var_name != name) {
                        block.reference_count--;
                        // If original block's reference count drops to 0, deallocate it
                        // if (block.reference_count == 0) {
                        //     manager.free_blocks.push_back(block);
                        //     // No need to update size, as it's already correct
                        //     block = manager.used_blocks.erase(block); // Adjust iterator after erasing
                        // }
                        break; // Break since the original block is found and updated
                    }
                }
                it = manager.used_blocks.erase(it); // Remove the pointer variable
            } else { // If it's not a pointer, handle as a normal block
                if (it->reference_count > 0) {
                    it->reference_count--;
                    if (it->reference_count == 0) {
                        manager.free_blocks.push_back(*it);
                        it = manager.used_blocks.erase(it);
                    } else {
                        ++it;
                    }
                } else {
                    std::cerr << "\033[31mError: Attempted to deallocate memory with reference count already at 0\033[0m\n";
                    ++it; // Move to next iterator to avoid infinite loop
                }
            }
            return; // Deallocating a single variable at a time, so return after found
        } else {
            ++it; // Move to the next block if the current one is not the target
        }
    }

    std::cerr << "\033[31mError: Block " << name << " is not found for deallocation\033[0m\n";
}


void print_memory_status(const MemoryManager &manager)
{
    std::cout << "Used Blocks:\n";
    for (const auto &block : manager.used_blocks)
    {
        std::cout << "Address: " << block.start_address << ", Size: " << block.size << ", Reference Count: " << block.reference_count << '\n';
    }

    std::cout << "\nFree Blocks:\n";
    for (const auto &block : manager.free_blocks)
    {
        std::cout << "Address: " << block.start_address << ", Size: " << block.size << '\n';
    }
}

void assign_memory(MemoryManager &manager, std::string a, std::string b) {
    // Check if variable 'a' is already declared
    for (const auto &block : manager.used_blocks) {
        if (block.var_name == a) {
            std::cerr << "\033[31mError: Variable " << a << " is already declared\033[0m\n";
            return;
        }
    }

    // Find the block corresponding to variable 'b'
    for (auto &block : manager.used_blocks) {
        if (block.var_name == b) {
            // Since 'b' is found, increase its reference count
            block.reference_count++;

            // Create a new block for variable 'a' as a pointer to 'b'
            MemoryBlock allocated_block;
            allocated_block.var_name = a;
            allocated_block.start_address = block.start_address;
            allocated_block.size = 0; // Set the size to 0 for a pointer
            // No need to set reference count for the pointer block itself; 
            allocated_block.reference_count=-1;
            // it's the 'b' block's reference count that matters.

            // Add 'a' to the list of pointers pointing to the 'b' block
            // block.pointers_to_this.push_back(a);

            // Update used blocks
            manager.used_blocks.push_back(allocated_block);

            std::cout << "\033[36mPointer " << a << " to " << b << " is declared\033[0m\n";

            return;
        }
    }

    std::cerr << "\033[31mError: Variable " << b << " not found for assignment\033[0m\n";
}


int main()
{
    int size;
    MemoryManager memory_manager = create_memory_manager();

    std::ifstream file("input.txt");
    if (!file.is_open())
    {
        std::cerr << "\033[31mError opening file\033[0m\n";

        return 1;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string variable, command, last;

        iss >> variable >> command;

        if (command == "allocate")
        {
            iss >> size;
            MemoryBlock *block = allocate_memory(memory_manager, size, variable);
            if (block)
            {
                std::cout << "\033[32mAllocated memory at address " << block->start_address << " with size " << block->size << " for variable " << variable << "\033[0m\n";
            }
            else
            {
                std::cout << "Error: Insufficient memory for allocation\n";
            }
            compact_memory(memory_manager);
            print_memory_status(memory_manager);
        }
        else if (command == "=")
        {
            iss >> last;
            assign_memory(memory_manager, variable, last);

            if (memory_manager.used_blocks.empty())
            {
                std::cout << "Error: No block with reference count greater than 0 found for assignment\n";
            }
            compact_memory(memory_manager);
            print_memory_status(memory_manager);
        }
        else if (command == "free")
        {
            deallocate_memory(memory_manager, variable);
            std::cout << "\033[35mDeallocated memory for variable " << variable << "\033[0m\n";
            compact_memory(memory_manager);
            print_memory_status(memory_manager);
        }
        else if (variable == "compact" || variable == "COMPACT")
        {
            std::cout << "\n\n\033[33mcompacting...\n\n\033[0m";
            compact_memory(memory_manager);
            print_memory_status(memory_manager);
        }
    }

    file.close();
    // compact_memory(memory_manager);
    std::cout << "\n\n\033[32mMemory status at the end\033[0m\n\n";
    print_memory_status(memory_manager);

    return 0;
}
