#define pr_fmt(fmt) "[" KBUILD_MODNAME "] " fmt
// #define pr_fmt(fmt) "[" KBUILD_MODNAME "] " fmt

#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

#include "hwstack.h"

///////////////////////////////////////////////////////////////////////////////

static const struct kernel_param_ops HwstParamOps = {
    .set = HwstSetParam,
    .get = NULL,
};

char *cmd = "";

module_param_cb(cmd, &HwstParamOps, NULL, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(cmd, "Just a parameter placeholder for incoming commands");

///////////////////////////////////////////////////////////////////////////////

static bool HwstActive = TRUE;

static LIST_HEAD(HwstStackTop);
static __u32 HwstStackSize;

///////////////////////////////////////////////////////////////////////////////

static inline struct HWST_STACK_ENTRY *HwstpAllocateStackEntry(__u32 DataLength)
{
    __u32 entryLength = offsetof(struct HWST_STACK_ENTRY, Data) + DataLength;

    struct HWST_STACK_ENTRY *entry = (struct HWST_STACK_ENTRY *)kmalloc(entryLength, GFP_KERNEL);

    if (entry == NULL) {
        pr_err("HwstpAllocateStackEntry(): mamory alloc failed (%u bytes)\n", entryLength);
    }

    return entry;
}

///////////////////////////////////////////////////////////////////////////////

static inline void HwstpFreeStackEntry(struct HWST_STACK_ENTRY *Entry)
{
    kfree(Entry);
}

///////////////////////////////////////////////////////////////////////////////

static inline void HwstpInsertEntry(struct HWST_STACK_ENTRY *Entry)
{
    list_add(&Entry->Links, &HwstStackTop);

    ++HwstStackSize;
}

///////////////////////////////////////////////////////////////////////////////

static inline void HwstpRemoveEntry(struct HWST_STACK_ENTRY *Entry)
{
    list_del(&Entry->Links);

    --HwstStackSize;
}

///////////////////////////////////////////////////////////////////////////////

static inline void HwstpInitEntry(struct HWST_STACK_ENTRY *Entry, const void *Data,
                                  __u32 DataLength)
{
    memcpy(Entry->Data, Data, DataLength);
}

///////////////////////////////////////////////////////////////////////////////

static int HwstPushEntryStack(const void *Data, __u32 DataLength)
{
    if (HwstStackSize == HWST_MAX_STACK_ENTRIES) {
        pr_err("HwstPushEntryStack(): reached the stack limit %u\n", HwstStackSize);

        return -ERANGE;
    }

    struct HWST_STACK_ENTRY *entry = HwstpAllocateStackEntry(DataLength);

    if (entry == NULL) {
        return -ENOMEM;
    }

    HwstpInitEntry(entry, Data, DataLength);

    HwstpInsertEntry(entry);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static struct HWST_STACK_ENTRY *HwstPopEntryStack(void)
{
    if (HwstIsStackEmpty()) {
        // pr_warn("HwstPopEntryStack(): stack is empty\n");

        return NULL;
    }

    struct HWST_STACK_ENTRY *entry =
        list_first_entry(&HwstStackTop, struct HWST_STACK_ENTRY, Links);

    HwstpRemoveEntry(entry);

    return entry;
}

///////////////////////////////////////////////////////////////////////////////

static struct HWST_STACK_ENTRY *HwstPeekTopEntryStack(void)
{
    if (HwstIsStackEmpty()) {
        // pr_err("HwstPeekTopEntryStack(): stack is empty\n");

        return NULL;
    }

    return list_first_entry(&HwstStackTop, struct HWST_STACK_ENTRY, Links);
}

///////////////////////////////////////////////////////////////////////////////

static bool HwstIsStackEmpty(void)
{
    if (list_empty(&HwstStackTop)) {
        // ASSERT(HwstStackSize == 0);

        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

static __u32 HwstGetStackSize(void)
{
    //
    //
    //

    return HwstStackSize;
}

///////////////////////////////////////////////////////////////////////////////

static void HwstPurgeEntriesStack(void)
{
    struct HWST_STACK_ENTRY *entry;
    struct HWST_STACK_ENTRY *nextEntry;

    list_for_each_entry_safe(entry, nextEntry, &HwstStackTop, Links)
    {
        HwstpRemoveEntry(entry);

        pr_info("HwstPurgeEntriesStack(): freeing entry 0x%px (value %u)\n", entry,
                *(__u32 *)entry->Data);

        HwstpFreeStackEntry(entry);
    }

    // ASSERT(HwstStackSize == 0);
}

///////////////////////////////////////////////////////////////////////////////

static bool HwstCheckCommand(const char *val, __u32 valLength, const char *cmd)
{
    __u32 length = max(valLength, strlen(cmd));

    if (!strncmp(val, cmd, length)) {
        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

static int HwstSetParam(const char *val, const struct kernel_param *kp)
{
    if (!HwstActive) {
        pr_warn("HwstSetParam(): command processing is stopped\n");

        return -ENODEV;
    }

    __u32 valLength = strlen(val);

    if (valLength && (val[valLength - 1] == ASCII_LF)) {
        --valLength;
    }

    if (valLength == 0) {
        pr_err("HwstSetParam(): invalid input string\n");

        return -EINVAL;
    }

    int result;

    if (!strncmp(val, HWST_CMD_PUSH, HWST_CMD_PUSH_LENGTH)) {
        __u32 value;

        result = kstrtouint(val + HWST_CMD_PUSH_LENGTH, 10, &value);

        if (result != 0) {
            pr_err("HwstSetParam(): kstrtouint() failed %d\n", result);

            return result;
        }

        result = HwstPushEntryStack(&value, sizeof(value));

        if (result == 0) {
            pr_info("HwstSetParam(): pushed %u\n", value);
        }

        return result;
    }

    if (HwstCheckCommand(val, valLength, HWST_CMD_POP)) {
        struct HWST_STACK_ENTRY *entry = HwstPopEntryStack();

        if (entry == NULL) {
            pr_warn("HwstSetParam(): stack is empty\n");

            return -ENODATA;
        }

        pr_info("HwstSetParam(): popped %u\n", *(__u32 *)entry->Data);

        HwstpFreeStackEntry(entry);

        return 0;
    }

    if (HwstCheckCommand(val, valLength, HWST_CMD_TOP)) {
        struct HWST_STACK_ENTRY *entry = HwstPeekTopEntryStack();

        if (entry == NULL) {
            pr_warn("HwstSetParam(): stack is empty\n");

            return -ENODATA;
        }

        pr_info("HwstSetParam(): peeked %u\n", *(__u32 *)entry->Data);

        return 0;
    }

    if (HwstCheckCommand(val, valLength, HWST_CMD_SIZE)) {
        pr_info("HwstSetParam(): stack size %u\n", HwstGetStackSize());

        return 0;
    }

    if (HwstCheckCommand(val, valLength, HWST_CMD_CLEAR)) {
        if (HwstIsStackEmpty()) {
            pr_warn("HwstSetParam(): stack is empty\n");

            return -ENODATA;
        }

        HwstPurgeEntriesStack();

        return 0;
    }

    if (HwstCheckCommand(val, valLength, HWST_CMD_EXIT)) {
        HwstPurgeEntriesStack();

        HwstActive = FALSE;

        pr_info("HwstSetParam(): stopped command processing\n");

        return 0;
    }

    pr_err("HwstSetParam(): invalid command <%.*s>\n", valLength, val);

    return -EINVAL;
}

///////////////////////////////////////////////////////////////////////////////

static int __init HwstInit(void)
{
    pr_info("HwstInit(): init\n");
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static void __exit HwstExit(void)
{
    pr_info("HwstExit(): exit\n");

    HwstPurgeEntriesStack();
}

///////////////////////////////////////////////////////////////////////////////

module_init(HwstInit);
module_exit(HwstExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("A stack testing module 2 for the Linux kernel");

///////////////////////////////////////////////////////////////////////////////
