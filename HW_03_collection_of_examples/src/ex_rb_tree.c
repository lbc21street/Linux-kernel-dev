//=================================================================================================
//
// \file    ex_rb_tree.c
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
#include <linux/rbtree.h>

#include "ex_rb_tree.h"
#include "supportmacros.h"

//
//
//

static int ExrbtGetParam(char *buffer, const struct kernel_param *kp);

static int ExrbtSetParam(const char *val, const struct kernel_param *kp);

//
//
//

static const struct kernel_param_ops ExrbtParamOps = {
    .set = ExrbtSetParam,
    .get = ExrbtGetParam,
};

char *cmd = "";

module_param_cb(cmd, &ExrbtParamOps, NULL, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(cmd, "Just a parameter placeholder for incoming commands");

static uint32_t value;

module_param_cb(value, &ExrbtParamOps, &value, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(value, "Some return value");

//
//
//

EXRBT_CTRL ExrbtCtrl = {
    .Active = TRUE,
};

//
//
//

static int ExrbtCreateTree(void)
{
    ExrbtCtrl.TreeRoot = RB_ROOT;

    pr_info("inited rbtree 0x%px", &ExrbtCtrl.TreeRoot);

    SetFlag(ExrbtCtrl.Flags, EXRBT_CTLFL_TREE_CREATED);

    return 0;
}

//
//
//

static bool ExrbtIsDataEntryTreeEmpty(void)
{
    if (RB_EMPTY_ROOT(&ExrbtCtrl.TreeRoot)) {
        return TRUE;
    }

    return FALSE;
}

//
//
//

static inline PEXRBT_DATA_ENTRY ExrbtpAllocateDataEntry(void)
{
    uint32_t dataLength = sizeof(EXRBT_DATA_ENTRY);

    PEXRBT_DATA_ENTRY entry = (PEXRBT_DATA_ENTRY)kmalloc(dataLength, GFP_KERNEL);

    if (entry == NULL) {
        pr_err("memory alloc failed for data entry (%u bytes)", dataLength);
    }

    return entry;
}

//
//
//

static inline void ExrbtpFreeDataEntry(PEXRBT_DATA_ENTRY Entry)
{
    kfree(Entry);
}

//
//
//

static int ExrbtNodeCompareRoutine(struct rb_node *Node1, const struct rb_node *Node2)
{
    PEXRBT_DATA_ENTRY entry1 = rb_entry(Node1, EXRBT_DATA_ENTRY, Node);
    PEXRBT_DATA_ENTRY entry2 = rb_entry(Node2, EXRBT_DATA_ENTRY, Node);

    if (entry1->Value < entry2->Value) {
        return -1;
    }

    if (entry1->Value > entry2->Value) {
        return 1;
    }

    return 0;
}

//
//
//

static int ExrbtKeyCompareRoutine(const void *Key, const struct rb_node *Node)
{
    uint32_t value = PTR_UINT(Key);
    PEXRBT_DATA_ENTRY entry = rb_entry(Node, EXRBT_DATA_ENTRY, Node);

    if (value < entry->Value) {
        return -1;
    }

    if (value > entry->Value) {
        return 1;
    }

    return 0;
}

//
//
//

static int ExrbtAddDataEntry(uint32_t Value)
{
    PEXRBT_DATA_ENTRY entry = ExrbtpAllocateDataEntry();

    if (entry == NULL) {
        return -ENOMEM;
    }

    entry->Value = Value;

    struct rb_node *node = rb_find_add(&entry->Node, &ExrbtCtrl.TreeRoot, ExrbtNodeCompareRoutine);

    if (node != NULL) {
        pr_warn("node 0x%px value 0x%08X already in tree", node,
                rb_entry(node, EXRBT_DATA_ENTRY, Node)->Value);

        ExrbtpFreeDataEntry(entry);

        return -EEXIST;
    }

    pr_info("added node 0x%px value 0x%08X", &entry->Node, entry->Value);

    return 0;
}

//
//
//

static int ExrbtRemoveDataEntry(bool Last)
{
    if (ExrbtIsDataEntryTreeEmpty()) {
        pr_warn("tree is empty");

        return -ENODATA;
    }

    struct rb_node *node;

    if (Last) {
        node = rb_last(&ExrbtCtrl.TreeRoot);
    }

    else {
        node = rb_first(&ExrbtCtrl.TreeRoot);
    }

    PEXRBT_DATA_ENTRY entry = rb_entry(node, EXRBT_DATA_ENTRY, Node);

    rb_erase(node, &ExrbtCtrl.TreeRoot);

    pr_info("removed %s entry 0x%px with value %u", Last ? "last" : "first", entry, entry->Value);

    ExrbtpFreeDataEntry(entry);

    return 0;
}

//
//
//

static int ExrbtRemoveDataEntryByValue(uint32_t Value)
{
    if (ExrbtIsDataEntryTreeEmpty()) {
        pr_warn("tree is empty");

        return -ENODATA;
    }

    struct rb_node *node = rb_find(UINT_PTR(Value), &ExrbtCtrl.TreeRoot, ExrbtKeyCompareRoutine);

    if (node != NULL) {
        rb_erase(node, &ExrbtCtrl.TreeRoot);

        PEXRBT_DATA_ENTRY entry = rb_entry(node, EXRBT_DATA_ENTRY, Node);

        pr_info("removed entry 0x%px by value %u", entry, entry->Value);

        ExrbtpFreeDataEntry(entry);

        return 0;
    }

    pr_warn("no entry found with value %u", Value);

    return -ENOENT;
}

//
//
//

static int ExrbtPeekDataEntry(bool Last)
{
    if (ExrbtIsDataEntryTreeEmpty()) {
        pr_warn("tree is empty");

        return -ENODATA;
    }

    struct rb_node *node;

    if (Last) {
        node = rb_last(&ExrbtCtrl.TreeRoot);
    }

    else {
        node = rb_first(&ExrbtCtrl.TreeRoot);
    }

    PEXRBT_DATA_ENTRY entry = rb_entry(node, EXRBT_DATA_ENTRY, Node);

    value = entry->Value;

    pr_info("peeked %s entry 0x%px with value %u", Last ? "last" : "first", entry, entry->Value);

    return 0;
}

//
//
//

static int ExrbtFindDataEntryByValue(uint32_t Value)
{
    if (ExrbtIsDataEntryTreeEmpty()) {
        pr_warn("tree is empty");

        return -ENODATA;
    }

    struct rb_node *node = rb_find(UINT_PTR(Value), &ExrbtCtrl.TreeRoot, ExrbtKeyCompareRoutine);

    if (node != NULL) {
        PEXRBT_DATA_ENTRY entry = rb_entry(node, EXRBT_DATA_ENTRY, Node);

        pr_info("found entry 0x%px by value %u", entry, entry->Value);

        value = entry->Value;

        return 0;
    }

    pr_warn("no entry found with value %u", Value);

    return -ENOENT;
}

//
//
//

static void ExrbtpPrintDataEntry(PEXRBT_DATA_ENTRY Entry)
{
    pr_info("  entry 0x%px - value 0x%08X", Entry, Entry->Value);
}

//
//
//

static int ExrbtEnumerateDataEntries(bool Last)
{
    if (ExrbtIsDataEntryTreeEmpty()) {
        pr_warn("tree is empty");

        return -ENODATA;
    }

    pr_info("starting from %s entry", Last ? "last" : "first");

    struct rb_node *node;

    if (Last) {
        node = rb_last(&ExrbtCtrl.TreeRoot);
    }

    else {
        node = rb_first(&ExrbtCtrl.TreeRoot);
    }

    while (TRUE) {
        if (node == NULL) {
            break;
        }

        PEXRBT_DATA_ENTRY entry = rb_entry(node, EXRBT_DATA_ENTRY, Node);

        ExrbtpPrintDataEntry(entry);

        if (Last) {
            node = rb_prev(node);
        }

        else {
            node = rb_next(node);
        }

    } // while (TRUE)

    return 0;
}

//
//
//

static int ExrbtEnumerateDataEntriesRaw(void)
{
    if (ExrbtIsDataEntryTreeEmpty()) {
        pr_warn("tree is empty");

        return -ENODATA;
    }

    pr_info("postorder enum");

    PEXRBT_DATA_ENTRY entry;
    PEXRBT_DATA_ENTRY tempEntry;

    rbtree_postorder_for_each_entry_safe(entry, tempEntry, &ExrbtCtrl.TreeRoot, Node)
    {
        ExrbtpPrintDataEntry(entry);

    } // rbtree_postorder_for_each_entry_safe()

    return 0;
}

//
//
//

static int ExrbtHashDataEntries(bool Last)
{
    if (ExrbtIsDataEntryTreeEmpty()) {
        pr_warn("tree is empty");

        return -ENODATA;
    }

    pr_info("starting from %s entry", Last ? "last" : "first");

    struct rb_node *node;

    if (Last) {
        node = rb_last(&ExrbtCtrl.TreeRoot);
    }

    else {
        node = rb_first(&ExrbtCtrl.TreeRoot);
    }

    uint32_t hash = 0;
    uint32_t index = 0;

    while (TRUE) {
        if (node == NULL) {
            break;
        }

        PEXRBT_DATA_ENTRY entry = rb_entry(node, EXRBT_DATA_ENTRY, Node);

        hash ^= entry->Value + 7 * index++;

        if (Last) {
            node = rb_prev(node);
        }

        else {
            node = rb_next(node);
        }

    } // while (TRUE)

    hash ^= (index << 16 | index);

    pr_info("hash 0x%08X", hash);

    value = hash;

    return 0;
}

//
//
//

static uint32_t ExrbtGetNumberOfDataEntries(void)
{
    PEXRBT_DATA_ENTRY entry;
    PEXRBT_DATA_ENTRY tempEntry;

    uint32_t entries = 0;

    rbtree_postorder_for_each_entry_safe(entry, tempEntry, &ExrbtCtrl.TreeRoot, Node)
    {
        ++entries;

    } // rbtree_postorder_for_each_entry_safe()

    value = entries;

    return entries;
}

//
//
//

static int ExrbtClearDataEntries(void)
{
    if (ExrbtIsDataEntryTreeEmpty()) {
        pr_warn("tree is empty");

        return -ENODATA;
    }

    //
    // [IMPORTANT]
    // [NOTE]
    //
    // we cannot use rbtree_postorder_for_each_entry_safe() since the following:
    //
    // note, however, that it cannot handle other modifications that re-order the rbtree it is
    // iterating over; this includes calling rb_erase() on @pos, as rb_erase() may rebalance the
    // tree, causing us to miss some nodes
    //

    while (TRUE) {
        struct rb_node *node = rb_last(&ExrbtCtrl.TreeRoot);

        if (node == NULL) {
            break;
        }

        PEXRBT_DATA_ENTRY entry = rb_entry(node, EXRBT_DATA_ENTRY, Node);

        pr_info("removing and freeing entry 0x%px value 0x%08X", entry, entry->Value);

        rb_erase(&entry->Node, &ExrbtCtrl.TreeRoot);

        ExrbtpFreeDataEntry(entry);

    } // while (TRUE)

    return 0;
}

//
//
//

static int ExrbtGetParam(char *buffer, const struct kernel_param *kp)
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

static bool ExrbtCheckCommand(const char *val, uint32_t valLength, const char *cmd)
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

static int ExrbtSetParam(const char *val, const struct kernel_param *kp)
{
    if (!ExrbtCtrl.Active) {
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
    bool last = FALSE;

    if (!strncmp(val, EXRBT_CMD_ADD, EXRBT_CMD_ADD_LENGTH)) {
        uint32_t value;

        result = kstrtouint(val + EXRBT_CMD_ADD_LENGTH, 10, &value);

        if (result != 0) {
            pr_err("kstrtouint() failed %d", result);

            return result;
        }

        return ExrbtAddDataEntry(value);
    }

    if (ExrbtCheckCommand(val, valLength, EXRBT_CMD_REMOVE_LAST)) {
        last = TRUE;
    }

    if (last || ExrbtCheckCommand(val, valLength, EXRBT_CMD_REMOVE)) {
        return ExrbtRemoveDataEntry(last);
    }

    if (!strncmp(val, EXRBT_CMD_REMOVE_BY, EXRBT_CMD_REMOVE_BY_LENGTH)) {
        uint32_t value;

        result = kstrtouint(val + EXRBT_CMD_REMOVE_BY_LENGTH, 10, &value);

        if (result != 0) {
            pr_err("kstrtouint() failed %d", result);

            return result;
        }

        return ExrbtRemoveDataEntryByValue(value);
    }

    if (ExrbtCheckCommand(val, valLength, EXRBT_CMD_PEEK_LAST)) {
        last = TRUE;
    }

    if (last || ExrbtCheckCommand(val, valLength, EXRBT_CMD_PEEK)) {
        return ExrbtPeekDataEntry(last);
    }

    if (!strncmp(val, EXRBT_CMD_FIND, EXRBT_CMD_FIND_LENGTH)) {
        uint32_t value;

        result = kstrtouint(val + EXRBT_CMD_FIND_LENGTH, 10, &value);

        if (result != 0) {
            pr_err("kstrtouint() failed %d", result);

            return result;
        }

        return ExrbtFindDataEntryByValue(value);
    }

    if (ExrbtCheckCommand(val, valLength, EXRBT_CMD_ENUM_LAST)) {
        last = TRUE;
    }

    if (last || ExrbtCheckCommand(val, valLength, EXRBT_CMD_ENUM)) {
        return ExrbtEnumerateDataEntries(last);
    }

    if (ExrbtCheckCommand(val, valLength, EXRBT_CMD_ENUM_RAW)) {
        return ExrbtEnumerateDataEntriesRaw();
    }

    if (ExrbtCheckCommand(val, valLength, EXRBT_CMD_HASH_LAST)) {
        last = TRUE;
    }

    if (last || ExrbtCheckCommand(val, valLength, EXRBT_CMD_HASH)) {
        return ExrbtHashDataEntries(last);
    }

    if (ExrbtCheckCommand(val, valLength, EXRBT_CMD_SIZE)) {
        pr_info("number of data entries %u", ExrbtGetNumberOfDataEntries());

        return 0;
    }

    if (ExrbtCheckCommand(val, valLength, EXRBT_CMD_CLEAR)) {
        return ExrbtClearDataEntries();
    }

    if (ExrbtCheckCommand(val, valLength, EXRBT_CMD_EXIT)) {
        ExrbtCtrl.Active = FALSE;

        pr_info("stopped command processing");

        return 0;
    }

    pr_err("invalid command <%.*s>", valLength, val);

    return -EINVAL;
}

//
//
//

static int __init ExrbtInit(void)
{
    pr_info("entering...");

    ExrbtCreateTree();

    pr_info("leaving...");

    return 0;
}

//
//
//

static void __exit ExrbtExit(void)
{
    pr_info("entering...");

    ExrbtClearDataEntries();

    pr_info("leaving...");
}

//
//
//

module_init(ExrbtInit);
module_exit(ExrbtExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("ex_rb_tree - a simple Linux kernel driver for testing the RB tree API");

//=================================================================================================
