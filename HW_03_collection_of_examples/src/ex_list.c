//=================================================================================================
//
// \file    ex_list.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/list.h>
#include <linux/types.h>

#include "ex_list.h"
#include "supportmacros.h"

//
//
//

static int ExlstGetParam(char *buffer, const struct kernel_param *kp);

static int ExlstSetParam(const char *val, const struct kernel_param *kp);

//
//
//

static const struct kernel_param_ops ExlstParamOps = {
    .set = ExlstSetParam,
    .get = ExlstGetParam,
};

char *cmd = "";

module_param_cb(cmd, &ExlstParamOps, NULL, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(cmd, "Just a parameter placeholder for incoming commands");

static uint32_t value;

module_param_cb(value, &ExlstParamOps, &value, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(value, "Some return value");

//
//
//

static EXLST_CTRL ExlstCtrl = {
    .DataEntries = LIST_HEAD_INIT(ExlstCtrl.DataEntries),
    .DataEntryCount = 0,
    .Active = TRUE,
};

//
//
//

static inline PEXLST_DATA_ENTRY ExlstpAllocateDataEntry(void)
{
    uint32_t dataLength = sizeof(EXLST_DATA_ENTRY);

    PEXLST_DATA_ENTRY entry = (PEXLST_DATA_ENTRY)kmalloc(dataLength, GFP_KERNEL);

    if (entry == NULL) {
        pr_err("memory alloc failed for data entry (%u bytes)", dataLength);
    }

    return entry;
}

//
//
//

static inline void ExlstpFreeDataEntry(PEXLST_DATA_ENTRY Entry)
{
    kfree(Entry);
}

//
//
//

static inline void ExlstpInsertDataEntry(PEXLST_DATA_ENTRY Entry, bool Tail)
{
    if (Tail) {
        list_add_tail(&Entry->Links, &ExlstCtrl.DataEntries);
    }

    else {
        list_add(&Entry->Links, &ExlstCtrl.DataEntries);
    }

    ++ExlstCtrl.DataEntryCount;
}

//
//
//

static inline void ExlstpRemoveDataEntry(PEXLST_DATA_ENTRY Entry)
{
    list_del(&Entry->Links);

    --ExlstCtrl.DataEntryCount;
}

//
//
//

static inline void ExlstpInitDataEntry(PEXLST_DATA_ENTRY Entry, uint32_t Value)
{
    Entry->Value = Value;
}

//
//
//

static bool ExlstIsDataEntryListEmpty(void)
{
    if (list_empty(&ExlstCtrl.DataEntries)) {
        // ASSERT(ExlstDataEntryCount == 0);

        return TRUE;
    }

    return FALSE;
}

//
// command processing routines
//

static int ExlstAddDataEntry(uint32_t Value, bool Tail)
{
    if (ExlstCtrl.DataEntryCount == EXLST_MAX_DATA_ENTRIES) {
        pr_warn("reached max number of data entries %u", ExlstCtrl.DataEntryCount);

        return -ERANGE;
    }

    PEXLST_DATA_ENTRY entry = ExlstpAllocateDataEntry();

    if (entry == NULL) {
        return -ENOMEM;
    }

    ExlstpInitDataEntry(entry, Value);
    ExlstpInsertDataEntry(entry, Tail);

    pr_info("added %s entry 0x%px with value %u", Tail ? "last" : "first", entry, entry->Value);

    return 0;
}

//
//
//

static int ExlstRemoveDataEntry(bool Tail)
{
    if (ExlstIsDataEntryListEmpty()) {
        pr_warn("list is empty");

        return -ENODATA;
    }

    PEXLST_DATA_ENTRY entry;

    if (Tail) {
        entry = list_last_entry(&ExlstCtrl.DataEntries, EXLST_DATA_ENTRY, Links);
    }

    else {
        entry = list_first_entry(&ExlstCtrl.DataEntries, EXLST_DATA_ENTRY, Links);
    }

    ExlstpRemoveDataEntry(entry);

    pr_info("removed %s entry 0x%px with value %u", Tail ? "last" : "first", entry, entry->Value);

    ExlstpFreeDataEntry(entry);

    return 0;
}

//
//
//

static int ExlstRemoveDataEntryByValue(uint32_t Value)
{
    PEXLST_DATA_ENTRY entry;
    PEXLST_DATA_ENTRY nextEntry;

    list_for_each_entry_safe(entry, nextEntry, &ExlstCtrl.DataEntries, Links)
    {
        if (entry->Value == Value) {
            ExlstpRemoveDataEntry(entry);

            pr_info("removed entry 0x%px by value %u", entry, entry->Value);

            ExlstpFreeDataEntry(entry);

            return 0;
        }
    }

    pr_warn("no entry found with value %u", Value);

    return -ENOENT;
}

//
//
//

static int ExlstPeekDataEntry(bool Tail)
{
    if (ExlstIsDataEntryListEmpty()) {
        pr_warn("list is empty");

        return -ENODATA;
    }

    PEXLST_DATA_ENTRY entry;

    if (Tail) {
        entry = list_last_entry(&ExlstCtrl.DataEntries, EXLST_DATA_ENTRY, Links);
    }

    else {
        entry = list_first_entry(&ExlstCtrl.DataEntries, EXLST_DATA_ENTRY, Links);
    }

    value = entry->Value;

    pr_info("peeked %s entry 0x%px with value %u", Tail ? "last" : "first", entry, entry->Value);

    return 0;
}

//
//
//

static int ExlstFindDataEntryByValue(uint32_t Value)
{
    PEXLST_DATA_ENTRY entry;

    list_for_each_entry(entry, &ExlstCtrl.DataEntries, Links)
    {
        if (entry->Value == Value) {
            pr_info("found entry 0x%px by value %u", entry, entry->Value);

            value = entry->Value;

            return 0;
        }
    }

    pr_warn("no entry found with value %u", Value);

    return -ENOENT;
}

//
//
//

static void ExlstpPrintDataEntry(PEXLST_DATA_ENTRY Entry)
{
    pr_info("  entry 0x%px - value %u", Entry, Entry->Value);
}

//
//
//

static int ExlstEnumerateDataEntries(bool Tail)
{
    if (ExlstIsDataEntryListEmpty()) {
        pr_warn("list is empty");

        return -ENODATA;
    }

    pr_info("starting from %s entry", Tail ? "last" : "first");

    PEXLST_DATA_ENTRY entry;

    if (Tail) {
        list_for_each_entry_reverse(entry, &ExlstCtrl.DataEntries, Links)
        {
            ExlstpPrintDataEntry(entry);
        }
    }

    else {
        list_for_each_entry(entry, &ExlstCtrl.DataEntries, Links)
        {
            ExlstpPrintDataEntry(entry);
        }
    }

    return 0;
}

//
//
//

static int ExlstRotateDataEntriesLeft(void)
{
    if (ExlstIsDataEntryListEmpty()) {
        pr_warn("list is empty");

        return -ENODATA;
    }

    pr_info("rotating %u entries left", ExlstCtrl.DataEntryCount);

    list_rotate_left(&ExlstCtrl.DataEntries);

    return 0;
}

//
//
//

static int ExlstHashDataEntries(bool Tail)
{
    if (ExlstIsDataEntryListEmpty()) {
        pr_warn("list is empty");

        return -ENODATA;
    }

    pr_info("starting from %s entry", Tail ? "last" : "first");

    PEXLST_DATA_ENTRY entry;
    uint32_t hash = 0;
    uint32_t index = 0;

    if (Tail) {
        list_for_each_entry_reverse(entry, &ExlstCtrl.DataEntries, Links)
        {
            hash ^= entry->Value + 7 * index++;
        }
    }

    else {
        list_for_each_entry(entry, &ExlstCtrl.DataEntries, Links)
        {
            hash ^= entry->Value + 7 * index++;
        }
    }

    hash ^= (ExlstCtrl.DataEntryCount << 16 | ExlstCtrl.DataEntryCount);

    pr_info("hash 0x%08X", hash);

    value = hash;

    return 0;
}

//
//
//

static uint32_t ExlstGetNumberOfDataEntries(void)
{
    value = ExlstCtrl.DataEntryCount;

    return ExlstCtrl.DataEntryCount;
}

//
//
//

static int ExlstClearDataEntries(void)
{
    if (ExlstIsDataEntryListEmpty()) {
        pr_warn("list is empty");

        return -ENODATA;
    }

    PEXLST_DATA_ENTRY entry;
    PEXLST_DATA_ENTRY nextEntry;

    list_for_each_entry_safe(entry, nextEntry, &ExlstCtrl.DataEntries, Links)
    {
        ExlstpRemoveDataEntry(entry);

        pr_info("freeing data entry 0x%px - value %u", entry, entry->Value);

        ExlstpFreeDataEntry(entry);
    }

    // ASSERT(ExlstDataEntryCount == 0);

    return 0;
}

//
//
//

static int ExlstGetParam(char *buffer, const struct kernel_param *kp)
{
    if (kp->arg == &value) {
        pr_info("value -> %u", value);

        return param_get_uint(buffer, kp);
    }

    return -EINVAL;
}

//
//
//

static bool ExlstCheckCommand(const char *val, uint32_t valLength, const char *cmd)
{
    uint32_t length = max(valLength, strlen(cmd));

    if (!strncmp(val, cmd, length)) {
        return TRUE;
    }

    return FALSE;
}

//
//
//

static int ExlstSetParam(const char *val, const struct kernel_param *kp)
{
    if (!ExlstCtrl.Active) {
        pr_warn("command processing is stopped");

        return -ENODEV;
    }

    uint32_t valLength = strlen(val);

    if (valLength && (val[valLength - 1] == ASCII_LF)) {
        --valLength;
    }

    if (valLength == 0) {
        pr_err("invalid input string");

        return -EINVAL;
    }

    int result;
    bool tail = FALSE;

    if (!strncmp(val, EXLST_CMD_ADD_TAIL, EXLST_CMD_ADD_TAIL_LENGTH)) {
        tail = TRUE;
    }

    if (tail || !strncmp(val, EXLST_CMD_ADD, EXLST_CMD_ADD_LENGTH)) {
        uint32_t value;

        result =
            kstrtouint(val + (tail ? EXLST_CMD_ADD_TAIL_LENGTH : EXLST_CMD_ADD_LENGTH), 10, &value);

        if (result != 0) {
            pr_err("kstrtouint() failed %d", result);

            return result;
        }

        return ExlstAddDataEntry(value, tail);
    }

    if (ExlstCheckCommand(val, valLength, EXLST_CMD_REMOVE_TAIL)) {
        tail = TRUE;
    }

    if (tail || ExlstCheckCommand(val, valLength, EXLST_CMD_REMOVE)) {
        return ExlstRemoveDataEntry(tail);
    }

    if (!strncmp(val, EXLST_CMD_REMOVE_BY, EXLST_CMD_REMOVE_BY_LENGTH)) {
        uint32_t value;

        result = kstrtouint(val + EXLST_CMD_REMOVE_BY_LENGTH, 10, &value);

        if (result != 0) {
            pr_err("kstrtouint() failed %d", result);

            return result;
        }

        return ExlstRemoveDataEntryByValue(value);
    }

    if (ExlstCheckCommand(val, valLength, EXLST_CMD_PEEK_TAIL)) {
        tail = TRUE;
    }

    if (tail || ExlstCheckCommand(val, valLength, EXLST_CMD_PEEK)) {
        return ExlstPeekDataEntry(tail);
    }

    if (!strncmp(val, EXLST_CMD_FIND, EXLST_CMD_FIND_LENGTH)) {
        uint32_t value;

        result = kstrtouint(val + EXLST_CMD_FIND_LENGTH, 10, &value);

        if (result != 0) {
            pr_err("kstrtouint() failed %d", result);

            return result;
        }

        return ExlstFindDataEntryByValue(value);
    }

    if (ExlstCheckCommand(val, valLength, EXLST_CMD_ENUM_TAIL)) {
        tail = TRUE;
    }

    if (tail || ExlstCheckCommand(val, valLength, EXLST_CMD_ENUM)) {
        return ExlstEnumerateDataEntries(tail);
    }

    if (ExlstCheckCommand(val, valLength, EXLST_CMD_ROTATE)) {
        return ExlstRotateDataEntriesLeft();
    }

    if (ExlstCheckCommand(val, valLength, EXLST_CMD_HASH_TAIL)) {
        tail = TRUE;
    }

    if (tail || ExlstCheckCommand(val, valLength, EXLST_CMD_HASH)) {
        return ExlstHashDataEntries(tail);
    }

    if (ExlstCheckCommand(val, valLength, EXLST_CMD_SIZE)) {
        pr_info("number of data entries %u", ExlstGetNumberOfDataEntries());

        return 0;
    }

    if (ExlstCheckCommand(val, valLength, EXLST_CMD_CLEAR)) {
        return ExlstClearDataEntries();
    }

    if (ExlstCheckCommand(val, valLength, EXLST_CMD_EXIT)) {
        ExlstCtrl.Active = FALSE;

        pr_info("stopped command processing");

        return 0;
    }

    pr_err("invalid command <%.*s>", valLength, val);

    return -EINVAL;
}

//
//
//

static int __init ExlstInit(void)
{
    pr_info("entering...");

    pr_info("leaving...");

    return 0;
}

//
//
//

static void __exit ExlstExit(void)
{
    pr_info("entering...");

    ExlstClearDataEntries();

    pr_info("leaving...");
}

//
//
//

module_init(ExlstInit);
module_exit(ExlstExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("ex_list - a simple Linux kernel driver for testing the list API");

//=================================================================================================
