//=================================================================================================
//
// \file    memtest.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <asm-generic/io.h>
#include <linux/atomic.h>
#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/log2.h>
#include <linux/mempool.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

// #include <linux/delay.h>

#include "memtest.h"
#include "supportmacros.h"

//
//
//

static const char *MtAllocatorNames[MT_MODE_MAX] = {
    "invalid", "kmalloc", "vmalloc", "kmem_cache", "mempool", "__get_free_pages", "alloc_pages"};

//
//
//

static const char *MtpGetAllocatorName(MT_TEST_MODE TestMode)
{
    if (TestMode < MT_MODE_MAX) {
        return MtAllocatorNames[TestMode];
    }

    return "unknown";
}

//
//
//

static const char *MtPhysicalPagesStatusNames[MT_PHPS_MAX_STATUS] = {"n/a", "contiguous",
                                                                     "non-contiguous"};

//
//
//

static const char *MtpGetPhysPagesStatusName(MT_PHISICAL_PAGES_STATUS Status)
{
    if (Status < MT_PHPS_MAX_STATUS) {
        return MtPhysicalPagesStatusNames[Status];
    }

    return "unknown";
}

//
//
//

static inline uint64_t MtpStartInterval(void)
{
    return get_jiffies_64();
}

//
//
//

static inline uint32_t MtpMeasureInterval(uint64_t StartTime)
{
    return jiffies64_to_msecs(get_jiffies_64() - StartTime);
}

//
//
//

static phys_addr_t MtpGetPhysicalAddress(const void *Address)
{
    phys_addr_t physAddress;

    //
    // spotted in virt_to_hvpfn() (from linux/hyperv.h)
    //

    if (is_vmalloc_addr((void *)Address)) {
        physAddress = page_to_pfn(vmalloc_to_page((void *)Address)) << PAGE_SHIFT;
    }

    else {
        physAddress = __pa(Address);
    }

    return physAddress;
}

//
//
//

static MT_PHISICAL_PAGES_STATUS MtpCheckPhysicalPagesContiguous(const void *StartVa, size_t Size)
{
    uint32_t pages = MT_NUMBER_OF_PAGES(StartVa, Size);

    if (pages < 2) {
        return MT_PHPS_NOT_APPLICABLE;
    }

    uintptr_t pageAddress = PAGE_ALIGN_DOWN((uintptr_t)StartVa);

    uint32_t c = 0;

    while (c < (pages - 1)) {
        phys_addr_t physAddress;
        phys_addr_t nextPhysAddress;

        physAddress = MtpGetPhysicalAddress((void *)pageAddress);
        nextPhysAddress = MtpGetPhysicalAddress((void *)(pageAddress + PAGE_SIZE));

        if (!MT_ARE_PAGES_CONTIGUOUS(physAddress, nextPhysAddress)) {
            return MT_PHPS_NON_CONTIGUOUS;
        }

        pageAddress += PAGE_SIZE;

        ++c;

    } // while (c < (pages - 1))

    return MT_PHPS_CONTIGUOUS;
}

static MT_PHISICAL_PAGES_STATUS
MtpCheckPhysicalPagesContiguousFromList(const struct list_head *List)
{
    if (list_is_singular(List)) {
        return MT_PHPS_NOT_APPLICABLE;
    }

    struct list_head *ple = List->next;
    struct list_head *nextPle = ple->next;

    while (nextPle != List) {
        uintptr_t pageAddress = PAGE_ALIGN_DOWN((uintptr_t)container_of(ple, MT_TEST_BLOCK, Links));
        uintptr_t nextPageAddress =
            PAGE_ALIGN_DOWN((uintptr_t)container_of(nextPle, MT_TEST_BLOCK, Links));

        phys_addr_t physAddress;
        phys_addr_t nextPhysAddress;

        physAddress = MtpGetPhysicalAddress((void *)pageAddress);
        nextPhysAddress = MtpGetPhysicalAddress((void *)(nextPageAddress));

        if (!MT_ARE_PAGES_CONTIGUOUS(physAddress, nextPhysAddress)) {
            return MT_PHPS_NON_CONTIGUOUS;
        }

        ple = ple->next;
        nextPle = nextPle->next;

    } // while (nextPle != List)

    return MT_PHPS_CONTIGUOUS;
}

//
//
//

static int MtpTestKmalloc(const char *AllocatorName)
{
    size_t bytesToAllocate = PAGE_SIZE;
    uint32_t count = 0;

    while (TRUE) {
        uint64_t startTime = MtpStartInterval();

        void *buffer = kmalloc(bytesToAllocate, MT_DEFAULT_GFP_FLAGS);

        uint32_t timeSpent = MtpMeasureInterval(startTime);

        if (buffer == NULL) {
            // clang-format off
            pr_err("[%s] %u - %lu bytes (%u page(s)) spent %u ms => FAILURE",
                AllocatorName,
                count,
                bytesToAllocate,
                MT_NUMBER_OF_PAGES(buffer, bytesToAllocate),
                timeSpent);
            // clang-format on

            break;
        }

        if (is_power_of_2(bytesToAllocate)) {
            MT_PHISICAL_PAGES_STATUS status =
                MtpCheckPhysicalPagesContiguous(buffer, bytesToAllocate);

            // clang-format off
            pr_info("[%s] %u - %lu bytes (%u page(s)) [%s] spent %u ms => SUCCESS",
                AllocatorName,
                count,
                bytesToAllocate,
                MT_NUMBER_OF_PAGES(buffer, bytesToAllocate),
                MtpGetPhysPagesStatusName(status),
                timeSpent);
            // clang-format on
        }

        kfree(buffer);

        bytesToAllocate += PAGE_SIZE;
        ++count;

    } // while (TRUE)

    return 0;
}

//
//
//

static int MtpTestVmalloc(const char *AllocatorName)
{
    size_t bytesToAllocate = PAGE_SIZE;
    uint32_t count = 0;

    while (TRUE) {
        uint64_t startTime = MtpStartInterval();

        void *buffer = __vmalloc(bytesToAllocate, MT_GFP_FLAGS_WITH_NORETRY);

        uint32_t timeSpent = MtpMeasureInterval(startTime);

        if (buffer == NULL) {
            // clang-format off
            pr_err("[%s] %u - %lu bytes (%u page(s)) spent %u ms => FAILURE",
                AllocatorName,
                count,
                bytesToAllocate,
                MT_NUMBER_OF_PAGES(buffer, bytesToAllocate),
                timeSpent);
            // clang-format on

            break;
        }

        MT_PHISICAL_PAGES_STATUS status = MtpCheckPhysicalPagesContiguous(buffer, bytesToAllocate);

        // clang-format off
        pr_info("[%s] %u - %lu bytes (%u page(s)) [%s] spent %u ms => SUCCESS",
            AllocatorName,
            count,
            bytesToAllocate,
            MT_NUMBER_OF_PAGES(buffer, bytesToAllocate),
            MtpGetPhysPagesStatusName(status),
            timeSpent);
        // clang-format on

        vfree(buffer);

        if (bytesToAllocate < 1024 * 1024 * 1024) {
            bytesToAllocate <<= 1;
        }

        else {
            bytesToAllocate += 65536 * PAGE_SIZE; // 256 MB
        }

        ++count;

    } // while (TRUE)

    return 0;
}

//
//
//

static int MtpTestKmemCache(const char *AllocatorName)
{
    int result = 0;
    const char *cacheName = MT_KMEM_CACHE_NAME;
    struct kmem_cache *memCache = NULL;

    do {
        uint32_t blockSize = sizeof(MT_TEST_BLOCK);

        //
        // SLAB_NO_MERGE is not available on 6.1.130
        //

        memCache = kmem_cache_create(cacheName, blockSize, 0, 0, NULL);

        if (memCache == NULL) {
            result = -ENOMEM;

            pr_err("kmem_cache_create() failed (blockSize %u)", blockSize);

            break;
        }

        blockSize = kmem_cache_size(memCache);

        pr_info("created kmem_cache <%s> 0x%px - blockSize %u", cacheName, memCache, blockSize);

        LIST_HEAD(blockList);
        size_t bytesToAllocate = blockSize;
        uint32_t count = 0;

        while (TRUE) {
            uint64_t startTime = MtpStartInterval();

            PMT_TEST_BLOCK block =
                (PMT_TEST_BLOCK)kmem_cache_alloc(memCache, MT_GFP_FLAGS_WITH_NORETRY);

            uint32_t timeSpent = MtpMeasureInterval(startTime);

            uint32_t pages = bytesToAllocate >> PAGE_SHIFT;

            if (block == NULL) {
                // clang-format off
                pr_err("[%s] %u - %lu bytes (%u page(s)) spent %u ms => FAILURE",
                    AllocatorName,
                    count,
                    bytesToAllocate,
                    pages,
                    timeSpent);
                // clang-format on

                break;
            }

            list_add(&block->Links, &blockList);

            if (is_power_of_2(pages)) {
                MT_PHISICAL_PAGES_STATUS status =
                    MtpCheckPhysicalPagesContiguousFromList(&blockList);

                // clang-format off
                pr_info("[%s] %u - %lu bytes (%u page(s)) [%s] spent %u ms => SUCCESS",
                    AllocatorName,
                    count,
                    bytesToAllocate,
                    pages,
                    MtpGetPhysPagesStatusName(status),
                    timeSpent);
                // clang-format on
            }

            bytesToAllocate += blockSize;
            ++count;

        } // while (TRUE)

        PMT_TEST_BLOCK entry;
        PMT_TEST_BLOCK nextEntry;

        list_for_each_entry_safe(entry, nextEntry, &blockList, Links)
        {
            list_del(&entry->Links);

            kmem_cache_free(memCache, entry);

        } // list_for_each_entry(entry, &blockList, Links)

    } while (FALSE);

    if (memCache != NULL) {
        pr_info("destroying kmem_cache <%s> 0x%px", cacheName, memCache);

        kmem_cache_destroy(memCache);
    }

    return result;
}

//
//
//

static int MtpTestMempool(const char *AllocatorName)
{
    int result = 0;
    mempool_t *memPool = NULL;

    do {
        size_t blockSize = sizeof(MT_TEST_BLOCK);
        int minBlocks = MT_MIN_MEMPOOL_BLOCK_COUNT;

        memPool = mempool_create_kmalloc_pool(minBlocks, blockSize);

        if (memPool == NULL) {
            result = -ENOMEM;

            pr_err("mempool_create_kmalloc_pool() failed (minBlocks %u blockSize %lu)", minBlocks,
                   blockSize);

            break;
        }

        pr_info("created mempool 0x%px - minBlocks %u blockSize %lu", memPool, minBlocks,
                blockSize);

        LIST_HEAD(blockList);
        size_t bytesToAllocate = blockSize;
        uint64_t allocationLimit = MtCtrl.TotalRamBytes / 4;
        uint32_t count = 0;

        pr_info("allocation limit: %llu bytes", allocationLimit);

        while (TRUE) {
            uint64_t startTime = MtpStartInterval();

            PMT_TEST_BLOCK block =
                (PMT_TEST_BLOCK)mempool_alloc(memPool, MT_GFP_FLAGS_WITH_NORETRY);

            uint32_t timeSpent = MtpMeasureInterval(startTime);

            uint32_t pages = bytesToAllocate >> PAGE_SHIFT;

            if (block == NULL) {
                // clang-format off
                pr_err("[%s] %u - %lu bytes (%u page(s)) spent %u ms => FAILURE",
                    AllocatorName,
                    count,
                    bytesToAllocate,
                    pages,
                    timeSpent);
                // clang-format on

                break;
            }

            list_add(&block->Links, &blockList);

            if (is_power_of_2(pages)) {
                MT_PHISICAL_PAGES_STATUS status =
                    MtpCheckPhysicalPagesContiguousFromList(&blockList);

                // clang-format off
                pr_info("[%s] %u - %lu bytes (%u page(s)) [%s] spent %u ms => SUCCESS",
                    AllocatorName,
                    count,
                    bytesToAllocate,
                    pages,
                    MtpGetPhysPagesStatusName(status),
                    timeSpent);
                // clang-format on
            }

            if (bytesToAllocate >= allocationLimit) {
                break;
            }

            bytesToAllocate += blockSize;
            ++count;

        } // while (TRUE)

        PMT_TEST_BLOCK entry;
        PMT_TEST_BLOCK nextEntry;

        list_for_each_entry_safe(entry, nextEntry, &blockList, Links)
        {
            list_del(&entry->Links);

            mempool_free(entry, memPool);

        } // list_for_each_entry(entry, &blockList, Links)

    } while (FALSE);

    if (memPool != NULL) {
        pr_info("destroying mempool 0x%px", memPool);

        mempool_destroy(memPool);
    }

    return result;
}

//
//
//

static int MtpTestGetFreePages(const char *AllocatorName)
{
    size_t bytesToAllocate = PAGE_SIZE;
    uint32_t count = 0;

    while (TRUE) {
        uint64_t startTime = MtpStartInterval();

        uintptr_t buffer = __get_free_pages(MT_DEFAULT_GFP_FLAGS, count);

        uint32_t timeSpent = MtpMeasureInterval(startTime);

        if (buffer == 0) {
            // clang-format off
            pr_err("[%s] %u - %lu bytes (%u page(s)) spent %u ms => FAILURE",
                AllocatorName,
                count,
                bytesToAllocate,
                MT_NUMBER_OF_PAGES(buffer, bytesToAllocate),
                timeSpent);
            // clang-format on

            break;
        }

        if (is_power_of_2(bytesToAllocate)) {
            MT_PHISICAL_PAGES_STATUS status =
                MtpCheckPhysicalPagesContiguous((void *)buffer, bytesToAllocate);

            // clang-format off
            pr_info("[%s] %u - %lu bytes (%u page(s)) [%s] spent %u ms => SUCCESS",
                AllocatorName,
                count,
                bytesToAllocate,
                MT_NUMBER_OF_PAGES(buffer, bytesToAllocate),
                MtpGetPhysPagesStatusName(status),
                timeSpent);
            // clang-format on
        }

        free_pages(buffer, count);

        bytesToAllocate <<= 1;
        ++count;

    } // while (TRUE)

    return 0;
}

//
//
//

static int MtpTestAllocPages(const char *AllocatorName)
{
    size_t bytesToAllocate = PAGE_SIZE;
    uint32_t count = 0;

    while (TRUE) {
        void *buffer = NULL;

        uint64_t startTime = MtpStartInterval();

        struct page *page = alloc_pages(MT_DEFAULT_GFP_FLAGS, count);

        if (page != NULL) {
            //
            // [NOTE]
            //
            // perhaps page_address() would be enough
            //

            buffer = kmap_local_page(page);
        }

        uint32_t timeSpent = MtpMeasureInterval(startTime);

        if (page == NULL) {
            // clang-format off
            pr_err("[%s] %u - %lu bytes (%u page(s)) spent %u ms => FAILURE",
                AllocatorName,
                count,
                bytesToAllocate,
                MT_NUMBER_OF_PAGES(buffer, bytesToAllocate),
                timeSpent);
            // clang-format on

            break;
        }

        if (is_power_of_2(bytesToAllocate)) {
            MT_PHISICAL_PAGES_STATUS status =
                MtpCheckPhysicalPagesContiguous((void *)buffer, bytesToAllocate);

            // clang-format off
            pr_info("[%s] %u - %lu bytes (%u page(s)) [%s] spent %u ms => SUCCESS",
                AllocatorName,
                count,
                bytesToAllocate,
                MT_NUMBER_OF_PAGES(buffer, bytesToAllocate),
                MtpGetPhysPagesStatusName(status),
                timeSpent);
            // clang-format on
        }

        kunmap_local(buffer);

        __free_pages(page, count);

        bytesToAllocate <<= 1;
        ++count;

    } // while (TRUE)

    return 0;
}

//
//
//

int MtTestMemory(MT_TEST_MODE TestMode)
{
    pr_info("=> total RAM pages: %lu (%llu bytes)", MtCtrl.TotalRamPages, MtCtrl.TotalRamBytes);

    int result = 0;
    const char *allocatorName = MtpGetAllocatorName(TestMode);

    switch (TestMode) {
        case MT_MODE_KMALLOC:
            result = MtpTestKmalloc(allocatorName);
            break;

        case MT_MODE_VMALLOC:
            result = MtpTestVmalloc(allocatorName);
            break;

        case MT_MODE_KMEM_CACHE:
            result = MtpTestKmemCache(allocatorName);
            break;

        case MT_MODE_MEMPOOL:
            result = MtpTestMempool(allocatorName);
            break;

        case MT_MODE_GET_FREE_PAGES:
            result = MtpTestGetFreePages(allocatorName);
            break;

        case MT_MODE_ALLOC_PAGES:
            result = MtpTestAllocPages(allocatorName);
            break;

        default:
            result = -EINVAL;
            pr_err("unsupported test mode %u", TestMode);
            break;
    } // switch (TestMode)

    return result;
}

//=================================================================================================
