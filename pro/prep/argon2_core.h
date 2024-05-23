/*
 * Argon2 source code package
 *
 * Written by Daniel Dinu and Dmitry Khovratovich, 2015
 *
 * This work is licensed under a Creative Commons CC0 1.0 License/Waiver.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with
 * this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 * modified by Agnieszka Bielec <bielecagnieszka8 at gmail.com>
 */

#ifndef ARGON2_CORE_H
#define ARGON2_CORE_H

#include "argon2.h"

#if defined(_MSC_VER)
#define ALIGN(n) __declspec(align(n))
#elif defined(__GNUC__) || defined(__clang)
#define ALIGN(x) __attribute__((__aligned__(x)))
#else
#define ALIGN(x)
#endif

#define CONST_CAST(x) (x)(uintptr_t)

/*************************Argon2 internal
 * constants**************************************************/

enum argon2_core_constants {
    /* Memory block size in bytes */
    ARGON2_BLOCK_SIZE = 1024,
    ARGON2_QWORDS_IN_BLOCK = ARGON2_BLOCK_SIZE / 8,
    ARGON2_OWORDS_IN_BLOCK = ARGON2_BLOCK_SIZE / 16,

    /* Number of pseudo-random values generated by one call to Blake in Argon2i
       to
       generate reference block positions */
    ARGON2_ADDRESSES_IN_BLOCK = 128,

    /* Pre-hashing digest length and its extension*/
    ARGON2_PREHASH_DIGEST_LENGTH = 64,
    ARGON2_PREHASH_SEED_LENGTH = 72
};

/*************************Argon2 internal data
 * types**************************************************/

/*
 * Structure for the (1KB) memory block implemented as 128 64-bit words.
 * Memory blocks can be copied, XORed. Internal words can be accessed by [] (no
 * bounds checking).
 */
typedef struct block_ { uint64_t v[ARGON2_QWORDS_IN_BLOCK]; } block;

/*****************Functions that work with the block******************/

/* Initialize each byte of the block with @in */
void argon2_init_block_value(block *b, uint8_t in);

/* Copy block @src to block @dst */
void argon2_copy_block(block *dst, const block *src);

/* XOR @src onto @dst bytewise */
void argon2_xor_block(block *dst, const block *src);

/*
 * Argon2 instance: memory pointer, number of passes, amount of memory, type,
 * and derived values.
 * Used to evaluate the number and location of blocks to construct in each
 * thread
 */
typedef struct Argon2_instance_t {
    block *memory;          /* Memory pointer */
    void *pseudo_rands;
    uint32_t version;
    uint32_t passes;        /* Number of passes */
    uint32_t memory_blocks; /* Number of blocks in memory */
    uint32_t segment_length;
    uint32_t lane_length;
    uint32_t lanes;
    uint32_t threads;
    argon2_type type;
    int print_internals; /* whether to print the memory blocks */
} argon2_instance_t;

/*
 * Argon2 position: where we construct the block right now. Used to distribute
 * work between threads.
 */
typedef struct Argon2_position_t {
    uint32_t pass;
    uint32_t lane;
    uint8_t slice;
    uint32_t index;
} argon2_position_t;

/*Struct that holds the inputs for thread handling FillSegment*/
typedef struct Argon2_thread_data {
    argon2_instance_t *instance_ptr;
    argon2_position_t pos;
} argon2_thread_data;

/*************************Argon2 core
 * functions**************************************************/


/*
 * Computes absolute position of reference block in the lane following a skewed
 * distribution and using a pseudo-random value as input
 * @param instance Pointer to the current instance
 * @param position Pointer to the current position
 * @param pseudo_rand 32-bit pseudo-random value used to determine the position
 * @param same_lane Indicates if the block will be taken from the current lane.
 * If so we can reference the current segment
 * @pre All pointers must be valid
 */
uint32_t argon2_index_alpha(const argon2_instance_t *instance,
                     const argon2_position_t *position, uint32_t pseudo_rand,
                     int same_lane);

/*
 * Function that validates all inputs against predefined restrictions and return
 * an error code
 * @param context Pointer to current Argon2 context
 * @return ARGON2_OK if everything is all right, otherwise one of error codes
 * (all defined in <argon2.h>
 */
int argon2_validate_inputs(const argon2_context *context);

/*
 * Function allocates memory, hashes the inputs with Blake,  and creates first
 * two blocks. Returns the pointer to the main memory with 2 blocks per lane
 * initialized
 * @param  context  Pointer to the Argon2 internal structure containing memory
 * pointer, and parameters for time and space requirements.
 * @param  instance Current Argon2 instance
 * @return Zero if successful, -1 if memory failed to allocate. @context->state
 * will be modified if successful.
 */
int argon2_initialize(argon2_instance_t *instance, argon2_context *context);
/// @brief Similar to 'argon2_initialize' but save data to a buffer memory instead to the MEMORY
/// @param context The argon2 context
/// @param type The argon2 type
/// @return If sucessfull or not
int opencl_argon2_initialize(argon2_context *context, argon2_type type);

/*
 * XORing the last block of each lane, hashing it, making the tag. Deallocates
 * the memory.
 * @param context Pointer to current Argon2 context (use only the out parameters
 * from it)
 * @param instance Pointer to current instance of Argon2
 * @pre instance->state must point to necessary amount of memory
 * @pre context->out must point to outlen bytes of memory
 * @pre if context->free_cbk is not NULL, it should point to a function that
 * deallocates memory
 */
void argon2_finalize(const argon2_context *context, argon2_instance_t *instance);
int blake2b_long(void *pout, size_t outlen, const void *in, size_t inlen);

/*
 * Function that fills the segment using previous segments also from other
 * threads
 * @param instance Pointer to the current instance
 * @param position Current position
 * @pre all block pointers must be valid
 */
void argon2_fill_segment(const argon2_instance_t *instance,
                  argon2_position_t position);

/*
 * Function that fills the entire memory t_cost times based on the first two
 * blocks in each lane
 * @param instance Pointer to the current instance
 * @return ARGON2_OK if successful, @context->state
 */
int argon2_fill_memory_blocks(argon2_instance_t *instance);

#endif
