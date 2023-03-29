/**
 * @file mm.c
 * @brief A 64-bit struct-based implicit free list memory allocator
 *
 * 15-213: Introduction to Computer Systems
 *
 * TODO: insert your documentation here. :)
 *
 *************************************************************************
 *
 * ADVICE FOR STUDENTS.
 * - Step 0: Please read the writeup!
 * - Step 1: Write your heap checker.
 * - Step 2: Write contracts / debugging assert statements.
 * - Good luck, and have fun!
 *
 *************************************************************************
 *
 * @author Miu Nakajima <mnakajim@andrew.cmu.edu>
 */

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* Do not change the following! */

#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* def DRIVER */

/* You can change anything from here onward */

/*
 *****************************************************************************
 * If DEBUG is defined (such as when running mdriver-dbg), these macros      *
 * are enabled. You can use them to print debugging output and to check      *
 * contracts only in debug mode.                                             *
 *                                                                           *
 * Only debugging macros with names beginning "dbg_" are allowed.            *
 * You may not define any other macros having arguments.                     *
 *****************************************************************************
 */
#ifdef DEBUG
/* When DEBUG is defined, these form aliases to useful functions */
#define dbg_requires(expr) assert(expr)
#define dbg_assert(expr) assert(expr)
#define dbg_ensures(expr) assert(expr)
#define dbg_printf(...) ((void)printf(__VA_ARGS__))
#define dbg_printheap(...) print_heap(__VA_ARGS__)
#else
/* When DEBUG is not defined, these should emit no code whatsoever,
 * not even from evaluation of argument expressions.  However,
 * argument expressions should still be syntax-checked and should
 * count as uses of any variables involved.  This used to use a
 * straightforward hack involving sizeof(), but that can sometimes
 * provoke warnings about misuse of sizeof().  I _hope_ that this
 * newer, less straightforward hack will be more robust.
 * Hat tip to Stack Overflow poster chqrlie (see
 * https://stackoverflow.com/questions/72647780).
 */
#define dbg_discard_expr_(...) ((void)((0) && printf(__VA_ARGS__)))
#define dbg_requires(expr) dbg_discard_expr_("%d", !(expr))
#define dbg_assert(expr) dbg_discard_expr_("%d", !(expr))
#define dbg_ensures(expr) dbg_discard_expr_("%d", !(expr))
#define dbg_printf(...) dbg_discard_expr_(__VA_ARGS__)
#define dbg_printheap(...) ((void)((0) && print_heap(__VA_ARGS__)))
#endif

/* Basic constants */

typedef uint64_t word_t;

/** @brief Word and header size (bytes) */
static const size_t wsize = sizeof(word_t);

/** @brief Double word size (bytes) */
static const size_t dsize = 2 * wsize;

/** @brief Minimum block size (bytes) */
static const size_t min_block_size = 2 * dsize;

/**
 * TODO: explain what chunksize is
 * (Must be divisible by dsize)
 */
static const size_t chunksize = (1 << 12);

/**
 * TODO: explain what alloc_mask is
 */
static const word_t alloc_mask = 0x1;

/**
 * TODO: explain what size_mask is
 */
static const word_t size_mask = ~(word_t)0xF;

/** @brief Represents the header and payload of one block in the heap */
typedef struct block {
    /** @brief Header contains size + allocation flag */
    word_t header;

    /**
     * @brief A pointer to the block payload.
    */
    union {
        struct {
            struct block *prev_empty;
            struct block *next_empty;
        };
        char payload[0];
    };

} block_t;

/* Global variables */

/** @brief Pointer to first block in the heap */
static block_t *heap_start = NULL;

/** @brief Pointer to first block in explicit list */
// static block_t *exp_start = NULL;

static const size_t seglist_size = 14;

/** @brief Pointers to first block of segregated lists */
static block_t *seglist[14] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                NULL, NULL, NULL, NULL, NULL, NULL};

static size_t bounds[14] = {32, 48, 64, 128, 256, 384, 512, 766, 1024, 2048,
                            3072, 4096, 5120, 6144};

/*
 *****************************************************************************
 * The functions below are short wrapper functions to perform                *
 * bit manipulation, pointer arithmetic, and other helper operations.        *
 *                                                                           *
 * We've given you the function header comments for the functions below      *
 * to help you understand how this baseline code works.                      *
 *                                                                           *
 * Note that these function header comments are short since the functions    *
 * they are describing are short as well; you will need to provide           *
 * adequate details for the functions that you write yourself!               *
 *****************************************************************************
 */

/*
 * ---------------------------------------------------------------------------
 *                        BEGIN SHORT HELPER FUNCTIONS
 * ---------------------------------------------------------------------------
 */

/**
 * @brief Returns the maximum of two integers.
 * @param[in] x
 * @param[in] y
 * @return `x` if `x > y`, and `y` otherwise.
 */
static size_t max(size_t x, size_t y) {
    return (x > y) ? x : y;
}

/**
 * @brief Rounds `size` up to next multiple of n
 * @param[in] size
 * @param[in] n
 * @return The size after rounding up
 */
static size_t round_up(size_t size, size_t n) {
    return n * ((size + (n - 1)) / n);
}

/**
 * @brief Packs the `size` and `alloc` of a block into a word suitable for
 *        use as a packed value.
 *
 * Packed values are used for both headers and footers.
 *
 * The allocation status is packed into the lowest bit of the word.
 *
 * @param[in] size The size of the block being represented
 * @param[in] alloc True if the block is allocated
 * @return The packed value
 */
static word_t pack(size_t size, bool alloc) {
    word_t word = size;
    if (alloc) {
        word |= alloc_mask;
    }
    return word;
}

/**
 * @brief Extracts the size represented in a packed word.
 *
 * This function simply clears the lowest 4 bits of the word, as the heap
 * is 16-byte aligned.
 *
 * @param[in] word
 * @return The size of the block represented by the word
 */
static size_t extract_size(word_t word) {
    return (word & size_mask);
}

/**
 * @brief Extracts the size of a block from its header.
 * @param[in] block
 * @return The size of the block
 */
static size_t get_size(block_t *block) {
    return extract_size(block->header);
}

/**
 * @brief Given a payload pointer, returns a pointer to the corresponding
 *        block.
 * @param[in] bp A pointer to a block's payload
 * @return The corresponding block
 */
static block_t *payload_to_header(void *bp) {
    return (block_t *)((char *)bp - offsetof(block_t, payload));
}

/**
 * @brief Given a block pointer, returns a pointer to the corresponding
 *        payload.
 * @param[in] block
 * @return A pointer to the block's payload
 * @pre The block must be a valid block, not a boundary tag.
 */
static void *header_to_payload(block_t *block) {
    dbg_requires(get_size(block) != 0);
    return (void *)(block->payload);
}

/**
 * @brief Given a block pointer, returns a pointer to the corresponding
 *        footer.
 * @param[in] block
 * @return A pointer to the block's footer
 * @pre The block must be a valid block, not a boundary tag.
 */
static word_t *header_to_footer(block_t *block) {
    dbg_requires(get_size(block) != 0 &&
                 "Called header_to_footer on the epilogue block");
    return (word_t *)(block->payload + get_size(block) - dsize);
}

/**
 * @brief Given a block footer, returns a pointer to the corresponding
 *        header.
 * @param[in] footer A pointer to the block's footer
 * @return A pointer to the start of the block
 * @pre The footer must be the footer of a valid block, not a boundary tag.
 */
static block_t *footer_to_header(word_t *footer) {
    size_t size = extract_size(*footer);
    dbg_assert(size != 0 && "Called footer_to_header on the prologue block");
    return (block_t *)((char *)footer + wsize - size);
}

/**
 * @brief Returns the payload size of a given block.
 *
 * The payload size is equal to the entire block size minus the sizes of the
 * block's header and footer.
 *
 * @param[in] block
 * @return The size of the block's payload
 */
static size_t get_payload_size(block_t *block) {
    size_t asize = get_size(block);
    return asize - dsize;
}

/**
 * @brief Returns the allocation status of a given header value.
 *
 * This is based on the lowest bit of the header value.
 *
 * @param[in] word
 * @return The allocation status correpsonding to the word
 */
static bool extract_alloc(word_t word) {
    return (bool)(word & alloc_mask);
}

/**
 * @brief Returns the allocation status of a block, based on its header.
 * @param[in] block
 * @return The allocation status of the block
 */
static bool get_alloc(block_t *block) {
    return extract_alloc(block->header);
}

/**
 * @brief Writes an epilogue header at the given address.
 *
 * The epilogue header has size 0, and is marked as allocated.
 *
 * @param[out] block The location to write the epilogue header
 */
static void write_epilogue(block_t *block) {
    dbg_requires(block != NULL);
    dbg_requires((char *)block == (char *)mem_heap_hi() - 7);
    block->header = pack(0, true);
}

/**
 * @brief Writes a block starting at the given address.
 *
 * This function writes both a header and footer, where the location of the
 * footer is computed in relation to the header.
 *
 * TODO: Are there any preconditions or postconditions?
 *
 * @param[out] block The location to begin writing the block header
 * @param[in] size The size of the new block
 * @param[in] alloc The allocation status of the new block
 */
static void write_block(block_t *block, size_t size, bool alloc) {
    dbg_requires(block != NULL);
    dbg_requires(size > 0);
    block->header = pack(size, alloc);
    word_t *footerp = header_to_footer(block);
    *footerp = pack(size, alloc);
}

/**
 * @brief Finds the next consecutive block on the heap.
 *
 * This function accesses the next block in the "implicit list" of the heap
 * by adding the size of the block.
 *
 * @param[in] block A block in the heap
 * @return The next consecutive block on the heap
 * @pre The block is not the epilogue
 */
static block_t *find_next(block_t *block) {
    dbg_requires(block != NULL);
    dbg_requires(get_size(block) != 0 &&
                 "Called find_next on the last block in the heap");
    return (block_t *)((char *)block + get_size(block));
}

/**
 * @brief Finds the footer of the previous block on the heap.
 * @param[in] block A block in the heap
 * @return The location of the previous block's footer
 */
static word_t *find_prev_footer(block_t *block) {
    // Compute previous footer position as one word before the header
    return &(block->header) - 1;
}

/**
 * @brief Finds the previous consecutive block on the heap.
 *
 * This is the previous block in the "implicit list" of the heap.
 *
 * If the function is called on the first block in the heap, NULL will be
 * returned, since the first block in the heap has no previous block!
 *
 * The position of the previous block is found by reading the previous
 * block's footer to determine its size, then calculating the start of the
 * previous block based on its size.
 *
 * @param[in] block A block in the heap
 * @return The previous consecutive block in the heap.
 */
static block_t *find_prev(block_t *block) {
    dbg_requires(block != NULL);
    word_t *footerp = find_prev_footer(block);

    // Return NULL if called on first block in the heap
    if (extract_size(*footerp) == 0) {
        return NULL;
    }

    return footer_to_header(footerp);
}

static void print_seg_list() {

    for (size_t i = 0; i < seglist_size; i++) {
        block_t *tmp = seglist[i];
        printf("SEG LIST %zu:\n", i);
        while (tmp != NULL) {
            printf("Header: %lu, Allocated: %d, Address: %p\n", tmp->header,
                get_alloc(tmp), (void *)tmp);
            tmp = tmp->next_empty;
        }
    }

}

static void print_heap() {

    block_t *tmp = heap_start;
    printf("HEAP:\n");
    while (get_size(tmp) != 0) {
        printf(
            "Header: %lu, Size: %lu, Allocated: %d, Address: %p\n",
            tmp->header, get_size(tmp), get_alloc(tmp), (void *)tmp);
        tmp = find_next(tmp);
    }

}

static void print_block(block_t *block) {

    printf("CURRENT BLOCK:\n");
    printf("Header: %lu, Allocated: %d, Address: %p\n", block->header,
            get_alloc(block), (void*)block);
            
}

/**
 * @brief Determines which segregated free list to put block in
 * 
 * @param[in] block
 * @return Index of seg list to insert into
*/
static size_t find_index(size_t size) {
    // printf("Finding index in seglist...\n");

    for (size_t i = 0; i < (seglist_size-1); i++) {
        if (size < bounds[i+1]) {
            dbg_assert(size >= bounds[i]);
            return i;
        }
    }
    return (seglist_size - 1);

}

/**
 * @brief Adds block to explicit list if it is free.
 * 
 * @param[in] block Unallocated block
 * @return Newly queued unallocated block
*/
static block_t *add_seg(block_t *block) {
    // printf("Adding to explicit list: ");
    // print_block(block);
    // printf("\n");

    dbg_assert(!get_alloc(block));
    dbg_assert(block != NULL);

    size_t index = find_index(get_size(block));

    if (seglist[index] == NULL) {
        block->next_empty = NULL;
        block->prev_empty = NULL;
        seglist[index] = block;
    } else {
        dbg_assert(seglist[index]->prev_empty == NULL);
        
        block_t *curr = seglist[index];
        curr->prev_empty = block;
        block->next_empty = curr;
        seglist[index] = block;
        seglist[index]->prev_empty = NULL;
    }
    return block;

}

/**
 * @brief Removes the block from explicit list.
 * @param[in] block
*/
static void remove_seg(block_t *block) {
    // printf("Removing from explicit list: ");
    // print_block(block);
    // printf("\n");

    size_t index = find_index(get_size(block));

    block_t *prev_open = block->prev_empty;
    block_t *next_open = block->next_empty;
    bool prev_null = (prev_open == NULL);
    bool next_null = (next_open == NULL);

    if (prev_null && next_null) {
        // If block is the only open block
        seglist[index] = NULL;
    } else if (prev_null && !next_null) {
        // If block is the first block
        seglist[index] = next_open;
        next_open->prev_empty = NULL;
    } else if (next_null && !prev_null) {
        // If block is the last block
        prev_open->next_empty = NULL;
    } else {
        dbg_assert(prev_open != NULL);
        prev_open->next_empty = next_open;
        next_open->prev_empty = prev_open;
    }
    block->next_empty = NULL;
    block->prev_empty = NULL;
    
}

/*
 * ---------------------------------------------------------------------------
 *                        END SHORT HELPER FUNCTIONS
 * ---------------------------------------------------------------------------
 */

/******** The remaining content below are helper and debug routines ********/

/**
 * @brief Combines adjacent free blocks.
 *
 * @param[in] block
 * @return Coalesced (optimized) block
 */
static block_t *coalesce_block(block_t *block) {
    // printf("Coalescing...\n");

    // If block is NULL or of size zero, return the block.
    if (block == NULL || get_size(block) == 0) {
        return block;
    }

    block_t *prev = find_prev(block); // Returns NULL if it is a prologue.
    block_t *next = find_next(block);

    size_t block_size = get_size(block);

    // Saves allocated status into a boolean variable
    bool prev_alloc;
    bool next_alloc;
    if (prev == NULL) {
        // In the case that find_prev() returned NULL.
        prev_alloc = 1;
    } else {
        prev_alloc = get_alloc(prev);
    }
    if (next == NULL) {
        // In the case that find_next() returned NULL.
        next_alloc = 1;
    } else {
        next_alloc = get_alloc(next);
    }

    if (prev_alloc && next_alloc) {

        // Return original block if both prev and next are allocated.
        add_seg(block);
        return block;

    } else if (!prev_alloc && next_alloc) {

        // In the case prev is unallocated, coalesce with the prev block.
        remove_seg(prev);
        block_size += get_size(prev);
        write_block(prev, block_size, 0);
        add_seg(prev);
        return prev;

    } else if (prev_alloc && !next_alloc) {

        // In the case next is unallocated, coalesce with the next block.
        remove_seg(next);
        block_size += get_size(next);
        write_block(block, block_size, 0);
        add_seg(block);
        return block;

    } else {

        // In the case prev and next are both unallocated.
        remove_seg(prev);
        remove_seg(next);
        block_size += get_size(prev) + get_size(next);
        write_block(prev, block_size, 0);
        add_seg(prev);
        return prev;
        
    }

    return block;
}

/**
 * @brief Makes heap larger by a given size.
 *
 * @param[in] size
 * @return Newly created free block
 */
static block_t *extend_heap(size_t size) {
    // printf("Extending heap...\n");
    void *bp;

    // Allocate an even number of words to maintain alignment
    size = round_up(size, dsize);
    if ((bp = mem_sbrk((intptr_t)size)) == (void *)-1) {
        return NULL;
    }

    // Initialize free block header/footer
    block_t *block = payload_to_header(bp);
    write_block(block, size, false);

    // Create new epilogue header
    block_t *block_next = find_next(block);
    write_epilogue(block_next);

    // Coalesce in the case previous block is free
    // Add to explicit list otherwise
    block_t *prev = find_prev(block); // Returns NULL if it is a prologue
    if (prev != NULL && get_alloc(prev) == false) {
        block = coalesce_block(block);
    } else {
        add_seg(block);
    }

    return block;
}

/**
 * @brief Splits block into two blocks: asize, blocksize - asize.
 *
 * @param[in] block
 * @param[in] asize
 */
static void split_block(block_t *block, size_t asize) {
    // printf("Splitting block...\n");
    dbg_requires(get_alloc(block));
    dbg_requires(round_up(asize, dsize) == asize);

    size_t block_size = get_size(block);

    if ((block_size - asize) >= min_block_size) {
        block_t *block_next;
        write_block(block, asize, true);

        block_next = find_next(block);
        write_block(block_next, block_size - asize, false);
        add_seg(block_next);
    }

    dbg_ensures(get_alloc(block));
}

/**
 * @brief Finds an unallocated block larger than given size.
 * @param[in] asize
 * @return Block that fits given size and is unallocated
 */
static block_t *find_fit(size_t asize) {
    block_t *block;

    for (block = heap_start; get_size(block) > 0; block = find_next(block)) {

        if (!(get_alloc(block)) && (asize <= get_size(block))) {
            return block;
        }
    }
    return NULL; // no fit found
}

/**
 * @brief Finds the first open block larger than given size.
 * @param[in] size
 * @return Open block of given size, NULL if none
*/
static block_t *find_open(size_t size) {
    // printf("Finding open block from explicit list...\n");

    block_t *tmp = seglist[find_index(size)];
    while (tmp != NULL) {
        if (get_size(tmp) >= size) {
            return tmp;
        }
        tmp = tmp->next_empty;
    }
    return NULL;

}

/**
 * @brief Checks for errors in the heap.
 *
 * @param[in] line
 * @return boolean value
 */
bool mm_checkheap(int line) {
    /*
     * As a filler: one guacamole is equal to 6.02214086 x 10**23 guacas.
     * One might even call it...  the avocado's number.
     *
     * Internal use only: If you mix guacamole on your bibimbap,
     * do you eat it with a pair of chopsticks, or with a spoon?
     */
    // printf("Checking heap... \n");

    // Prologue and epilogue
    block_t *prologue = (block_t*)mem_heap_lo();
    block_t *epilogue = (block_t*)((int*)mem_heap_hi() - 7);
    
    if (get_size(prologue) != 0 || get_size(epilogue) != 0) {
        // Sizes of prologue and epilogue should be 0.
        return false;
    } else if (!get_alloc(prologue) || !get_alloc(epilogue)) {
        // The prologue and epilogue should be marked as allocated.
        return false;
    }

    size_t count_heap = 0;
    size_t count_free = 0;

    block_t *tmp = heap_start;
    while (tmp != epilogue) {

        // Alignment
        size_t block_size = (size_t)tmp->payload;
        if (block_size % dsize != 0) {
            return false;
        }

        // Heap boundaries
        block_size = (size_t)tmp + get_size(tmp);
        if ((size_t)mem_heap_hi() < block_size) {
            return false;
        } else if ((size_t)mem_heap_lo() >= (size_t)tmp) {
            return false;
        } 

        // Header and footer consistency
        word_t curr_header = tmp->header;
        word_t *curr_footer = header_to_footer(tmp);
        bool alloc_hdr = extract_alloc(curr_header);
        bool alloc_ftr = extract_alloc(*curr_footer);
        if (curr_header != *curr_footer) {
            return false;
        } else if (alloc_hdr != alloc_ftr) {
            return false;
        }
        
        // Coalescing
        block_t *prev = find_prev(tmp);
        block_t *next = find_next(tmp);
        if (!get_alloc(prev) || !get_alloc(next)) {
            return false;
        }

        if (!get_alloc(tmp)) {
            count_heap++;
        }

        tmp = next;
    }

    block_t *tmp_seg;
    for (size_t i = 0; i < seglist_size; i++) {

        tmp_seg = seglist[i];

        while (tmp_seg != NULL) {

            // Consistent next/prev pointers
            block_t *next_free = tmp_seg->next_empty;
            if (next_free != NULL) {
                if (next_free->prev_empty != tmp_seg) {
                    return false;
                }
            }

            // Pointers are within bounds
            if ((size_t)mem_heap_hi() < (size_t)tmp_seg) {
                return false;
            } else if ((size_t)mem_heap_lo() > (size_t)tmp_seg) {
                return false;
            }

            count_free++;
            tmp_seg = tmp_seg->next_empty;

        }
    
    }

    if (count_heap != count_free) {
        return false;
    }

    return true;
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @return
 */
bool mm_init(void) {
    // printf("Initializing...\n");

    // Create the initial empty heap
    word_t *start = (word_t *)(mem_sbrk(2 * wsize));

    if (start == (void *)-1) {
        return false;
    }

    for (size_t i = 0; i < seglist_size; i++) {
        seglist[i] = NULL;
    } 

    start[0] = pack(0, true); // Heap prologue (block footer)
    start[1] = pack(0, true); // Heap epilogue (block header)

    // Heap starts with first "block header", currently the epilogue
    heap_start = (block_t *)&(start[1]);

    // Extend the empty heap with a free block of chunksize bytes
    if (extend_heap(chunksize) == NULL) {
        return false;
    }

    return true;
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] size
 * @return
 */
void *malloc(size_t size) {
    // printf("Running malloc()...\n");
    // printf("Requested size: %zu + 16, %zu \n", size, size+16);
    dbg_requires(mm_checkheap(__LINE__));

    size_t asize;      // Adjusted block size
    size_t extendsize; // Amount to extend heap if no fit is found
    block_t *block;
    void *bp = NULL;

    // Initialize heap if it isn't initialized
    if (heap_start == NULL) {
        if (!(mm_init())) {
            dbg_printf("Problem initializing heap. Likely due to sbrk");
            return NULL;
        }
    }

    // Ignore spurious request
    if (size == 0) {
        dbg_ensures(mm_checkheap(__LINE__));
        return bp;
    }

    // Adjust block size to include overhead and to meet alignment requirements
    asize = round_up(size + dsize, dsize);

    // Search the explicit list for a fit
    block = find_open(asize);

    // If no fit is found, request more memory, and then and place the block
    if (block == NULL) {
        // Always request at least chunksize
        extendsize = max(asize, chunksize);
        block = extend_heap(extendsize);
        // extend_heap returns an error
        if (block == NULL) {
            return bp;
        }
    }

    // Remove block from explicit list
    remove_seg(block);

    // The block should be marked as free
    dbg_assert(!get_alloc(block));

    // Mark block as allocated
    size_t block_size = get_size(block);
    write_block(block, block_size, true);

    // Try to split the block if too large
    split_block(block, asize);

    bp = header_to_payload(block);

    // print_heap();
    // print_seg_list();
    // print_block(block);
    // printf("\n\n");

    dbg_ensures(mm_checkheap(__LINE__));
    return bp;
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] bp
 */
void free(void *bp) {
    // printf("Freeing: ");
    dbg_requires(mm_checkheap(__LINE__));

    if (bp == NULL) {
        return;
    }

    block_t *block = payload_to_header(bp);
    // print_block(block);
    size_t size = get_size(block);

    // The block should be marked as allocated
    dbg_assert(get_alloc(block));

    // Mark the block as free
    write_block(block, size, false);

    // Try to coalesce the block with its neighbors
    block = coalesce_block(block);

    // print_heap();
    // print_seg_list();
    // print_block(block);
    // printf("\n\n");

    dbg_ensures(mm_checkheap(__LINE__));
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] ptr
 * @param[in] size
 * @return
 */
void *realloc(void *ptr, size_t size) {
    block_t *block = payload_to_header(ptr);
    size_t copysize;
    void *newptr;

    // If size == 0, then free block and return NULL
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    // If ptr is NULL, then equivalent to malloc
    if (ptr == NULL) {
        return malloc(size);
    }

    // Otherwise, proceed with reallocation
    newptr = malloc(size);

    // If malloc fails, the original block is left untouched
    if (newptr == NULL) {
        return NULL;
    }

    // Copy the old data
    copysize = get_payload_size(block); // gets size of old payload
    if (size < copysize) {
        copysize = size;
    }
    memcpy(newptr, ptr, copysize);

    // Free the old block
    free(ptr);

    return newptr;
}

/**
 * @brief
 *
 * <What does this function do?>
 * <What are the function's arguments?>
 * <What is the function's return value?>
 * <Are there any preconditions or postconditions?>
 *
 * @param[in] elements
 * @param[in] size
 * @return
 */
void *calloc(size_t elements, size_t size) {
    void *bp;
    size_t asize = elements * size;

    if (elements == 0) {
        return NULL;
    }
    if (asize / elements != size) {
        // Multiplication overflowed
        return NULL;
    }

    bp = malloc(asize);
    if (bp == NULL) {
        return NULL;
    }

    // Initialize all bits to 0
    memset(bp, 0, asize);

    return bp;
}

/*
 *****************************************************************************
 * Do not delete the following super-secret(tm) lines!                       *
 *                                                                           *
 * 53 6f 20 79 6f 75 27 72 65 20 74 72 79 69 6e 67 20 74 6f 20               *
 *                                                                           *
 * 66 69 67 75 72 65 20 6f 75 74 20 77 68 61 74 20 74 68 65 20               *
 * 68 65 78 61 64 65 63 69 6d 61 6c 20 64 69 67 69 74 73 20 64               *
 * 6f 2e 2e 2e 20 68 61 68 61 68 61 21 20 41 53 43 49 49 20 69               *
 *                                                                           *
 * 73 6e 27 74 20 74 68 65 20 72 69 67 68 74 20 65 6e 63 6f 64               *
 * 69 6e 67 21 20 4e 69 63 65 20 74 72 79 2c 20 74 68 6f 75 67               *
 * 68 21 20 2d 44 72 2e 20 45 76 69 6c 0a c5 7c fc 80 6e 57 0a               *
 *                                                                           *
 *****************************************************************************
 */
