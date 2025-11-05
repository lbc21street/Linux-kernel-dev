//=================================================================================================
//
// \file    memtest.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/gfp.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

typedef enum _MT_TEST_MODE {
    MT_MODE_INVALID = 0,
    MT_MODE_KMALLOC,
    MT_MODE_VMALLOC,
    MT_MODE_KMEM_CACHE,
    MT_MODE_MEMPOOL,
    MT_MODE_GET_FREE_PAGES,
    MT_MODE_ALLOC_PAGES,
    MT_MODE_MAX,

} MT_TEST_MODE;

//
//
//

typedef enum _MT_CTRL_FLAGS {
    MT_CTLFL_PARAMETERS_CAPTURED = 0x00000001,

} MT_CTRL_FLAGS;

//
//
//

typedef struct _MT_CTRL_FLAGS_BF {
    uint32_t FlParametersCaptured : 1;
    uint32_t FlReserved : 31;

} MT_CTRL_FLAGS_BF;

static_assert(sizeof(MT_CTRL_FLAGS_BF) == sizeof(MT_CTRL_FLAGS));

//
//
//

typedef struct _MT_CTRL {
    union {
        MT_CTRL_FLAGS Flags;
        MT_CTRL_FLAGS_BF FlagsBf;
    };

    uintptr_t TotalRamPages;
    uint64_t TotalRamBytes;

    MT_TEST_MODE TestMode;

} MT_CTRL, *PMT_CTRL;

extern MT_CTRL MtCtrl;

//
//
//

#define MT_DEFAULT_GFP_FLAGS (GFP_KERNEL | __GFP_NOWARN)
#define MT_GFP_FLAGS_WITH_NORETRY (MT_DEFAULT_GFP_FLAGS | __GFP_NORETRY)

//
//
//

typedef enum _MT_PHYSICAL_PAGES_STATUS {
    MT_PHPS_NOT_APPLICABLE = 0,
    MT_PHPS_CONTIGUOUS,
    MT_PHPS_NON_CONTIGUOUS,
    MT_PHPS_MAX_STATUS,

} MT_PHYSICAL_PAGES_STATUS;

//
//
//

#define MT_KMEM_CACHE_NAME "TestKmemCache"

#define MT_MIN_MEMPOOL_BLOCK_COUNT 1024

//
//
//

typedef struct _MT_TEST_BLOCK {
    struct list_head Links;

    char Payload[PAGE_SIZE - sizeof(struct list_head)];

} MT_TEST_BLOCK, *PMT_TEST_BLOCK;

//
//
//

#define MT_BYTE_OFFSET(Va) ((uint32_t)((uintptr_t)(Va) & ~PAGE_MASK))

#define MT_NUMBER_OF_PAGES(Va, Size)                                                               \
    ((uint32_t)(((uintptr_t)(Size) >> PAGE_SHIFT) +                                                \
                ((MT_BYTE_OFFSET(Va) + MT_BYTE_OFFSET(Size) + PAGE_SIZE - 1) >> PAGE_SHIFT)))

#define MT_ARE_PAGES_CONTIGUOUS(a1, a2) (((uintptr_t)(a1) + PAGE_SIZE) == (uintptr_t)(a2))

//
//
//

int MtTestMemory(MT_TEST_MODE TestMode);

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
