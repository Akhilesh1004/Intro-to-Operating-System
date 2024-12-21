/*
Student No.: 111705034
Student Name: 鄭秉豐
Email: rty78452@gmail.com
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not
supposed to be posted to a public server, such as a
public GitHub repository or a public web page.
*/


#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define MEMORY_POOL 20000
#define HEADER_SIZE 32 
#define MAX_LEVELS 11
#define FLAG  0x1

//63 bit size 1 bit flag;

typedef struct block {
    size_t size_flag;
    struct block *next;
    struct block *prev;
    struct block *free_next;
}Block;

void *memory_pool = NULL;
Block* free_lists[MAX_LEVELS];

size_t align_size(size_t size) {
    if(size%32 != 0) return ((size/32)+1)*32;
    else return size;
}

int get_level_index(size_t size) {
    int level = 0;
    size_t block_size = 32;
    while (block_size < size && level < MAX_LEVELS - 1) {
        block_size *= 2;
        level++;
    }
    return level;
}

void insert_into_free_list(Block* chunk) {
    int level = get_level_index(chunk->size_flag>>1);
    //char buffer[256];
    //sprintf(buffer, "Insert chunk size= %lu\n", chunk->size_flag>>1);
    //write(1, buffer, strlen(buffer));
    if (free_lists[level] == NULL) {
        free_lists[level] = chunk;
        chunk->free_next = NULL;
        chunk->size_flag |= FLAG;
    } else {
        Block* current = free_lists[level];
        while(current->free_next) {
            current = current->free_next;
        }
        current->free_next = chunk;
        chunk->free_next = NULL;
        chunk->size_flag |= FLAG;
    }
}

void remove_from_free_list(Block* chunk) {
    int level = get_level_index(chunk->size_flag>>1);
    //char buffer[256];
    //sprintf(buffer, "remove chunk size = %lu\n", chunk->size_flag>>1);
    //write(1, buffer, strlen(buffer));
    if (free_lists[level] == chunk) {
        free_lists[level] = chunk->free_next;
        chunk->size_flag &= ~FLAG;
    } else {
        Block* current = free_lists[level];
        while(current->free_next && current->free_next != chunk) {
            current = current->free_next;
        }
        if (current->free_next == chunk) {
            current->free_next = chunk->free_next;
            chunk->size_flag &= ~FLAG;
        }
    }
    chunk->free_next = NULL;
}

Block* find_best_fit_in_level(int level, size_t size) {
    Block* current = free_lists[level];
    Block* best_fit = NULL;
    while (current) {
        if (current->size_flag>>1 >= size) {
            if (!best_fit || current->size_flag>>1 < best_fit->size_flag>>1) {
                best_fit = current;
                if (best_fit->size_flag>>1 == size) break;
            }
        }
        current = current->free_next;
    }
    return best_fit;
}

Block* split_chunk(Block* chunk, size_t size) {
    if((chunk->size_flag>>1) < (size + HEADER_SIZE + 32)){
        chunk->size_flag &= ~FLAG;
    }else{
        size_t remaining_size = (chunk->size_flag>>1) - size - HEADER_SIZE;
        //char buffer[256];
        //sprintf(buffer, "remaining chunk size = %lu\n", remaining_size);
        //write(1, buffer, strlen(buffer));
        Block* new_chunk = (Block*)((char*)chunk + HEADER_SIZE + size);
        new_chunk->size_flag = (remaining_size<<1) | FLAG;
        new_chunk->prev = chunk;
        new_chunk->next = chunk->next;
        if (new_chunk->next) {
            new_chunk->next->prev = new_chunk;
        }
        insert_into_free_list(new_chunk);
        chunk->next = new_chunk;
        chunk->size_flag = (size<<1) & ~FLAG;
    }
    return chunk;
}

void* malloc(size_t size) {
    if (size == 0) {
        // Handle malloc(0)
        size_t max_size = 0;
        for (int i = 0; i < MAX_LEVELS; i++) {
            Block *current = free_lists[i];
            while (current) {
                if (current->size_flag>>1 > max_size) {
                    max_size = current->size_flag>>1;
                }
                current = current->free_next;
            }
        }
        char buffer[256];
        sprintf(buffer, "Max Free Chunk Size = %lu\n", max_size);
        write(1, buffer, strlen(buffer));
        // Release the memory pool
        if (memory_pool) {
            munmap(memory_pool, MEMORY_POOL);
            memory_pool = NULL;
            memset(free_lists, 0, sizeof(free_lists));
        }
        return NULL;
    }
    if (!memory_pool) {
        memory_pool = mmap(NULL, MEMORY_POOL, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
        if (memory_pool == MAP_FAILED) {
            return NULL;
        }
        // Initialize the chunk list with a single large free chunk
        Block* chunk_list_head = (Block*)memory_pool;
        chunk_list_head->size_flag = ((MEMORY_POOL - HEADER_SIZE)<<1) | FLAG;
        chunk_list_head->next = NULL;
        chunk_list_head->prev = NULL;
        chunk_list_head->free_next = NULL;
        memset(free_lists, 0, sizeof(free_lists));
        // Insert the initial chunk into the appropriate free list
        insert_into_free_list(chunk_list_head);
    }
    size = align_size(size);
    int level = get_level_index(size);
    //char buffer1[256];
    //sprintf(buffer1, "malloc size = %lu\n", size);
    //write(1, buffer1, strlen(buffer1));
    Block* chunk = NULL;
    for (int i = level; i < MAX_LEVELS; i++) {
        chunk = find_best_fit_in_level(i, size);
        if (chunk) break;
    }
    // No suitable chunk found
    if (!chunk){
        //sprintf(buffer1, "no chunk\n");
        //write(1, buffer1, strlen(buffer1));
        return NULL;
    }
    remove_from_free_list(chunk);
    chunk = split_chunk(chunk, size);
    return (void*)((char*)chunk+HEADER_SIZE);
}

void free(void *ptr) {
    if (!ptr) return ;
    if (ptr < memory_pool || ptr >= (void*)((char*)memory_pool + MEMORY_POOL)) return ;
    Block* chunk = (Block*)((char*)ptr - HEADER_SIZE);
    if (chunk->size_flag & FLAG) return ;
    chunk->size_flag |= FLAG;
    //char buffer1[256];
    //sprintf(buffer1, "free size = %lu\n", chunk->size_flag>>1);
    //write(1, buffer1, strlen(buffer1));
    if (chunk->prev && (chunk->prev->size_flag & FLAG)) {
        Block* prev_chunk = chunk->prev;
        remove_from_free_list(prev_chunk);
        prev_chunk->size_flag = ((prev_chunk->size_flag>>1) + (chunk->size_flag>>1) + HEADER_SIZE )<<1;
        prev_chunk->next = chunk->next;
        if (chunk->next) {
            chunk->next->prev = prev_chunk;
        }
        chunk = prev_chunk;
    }
    if (chunk->next && (chunk->next->size_flag & FLAG)) {
        Block* next_chunk = chunk->next;
        remove_from_free_list(next_chunk);
        chunk->size_flag = ((next_chunk->size_flag>>1) + (chunk->size_flag>>1) + HEADER_SIZE )<<1;
        chunk->next = next_chunk->next;
        if (next_chunk->next) {
            next_chunk->next->prev = chunk;
        }
    }
    insert_into_free_list(chunk);
}