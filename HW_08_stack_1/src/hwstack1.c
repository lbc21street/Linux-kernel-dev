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

char *string = "";

module_param_cb(string, &HwstParamOps, NULL, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(string, "Just a parameter placeholder for incoming string");

///////////////////////////////////////////////////////////////////////////////

static LIST_HEAD(HwstStackTop);
static __u32 HwstStackSize;

///////////////////////////////////////////////////////////////////////////////

static inline struct HWST_STACK_ENTRY *HwstpAllocateStackEntry(void)
{
    __u32 entryLength = sizeof(struct HWST_STACK_ENTRY);

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

static inline void HwstpInitEntry(struct HWST_STACK_ENTRY *Entry,
                                  enum HWST_BRACKET_TYPE BracketType, __u32 Position)
{
    Entry->BracketType = BracketType;
    Entry->Position = Position;
}

///////////////////////////////////////////////////////////////////////////////

static int HwstPushEntryStack(enum HWST_BRACKET_TYPE BracketType, __u32 Position)
{
    if (HwstStackSize == HWST_MAX_STACK_ENTRIES) {
        pr_err("HwstPushEntryStack(): reached the stack limit %u\n", HwstStackSize);

        return -ERANGE;
    }

    struct HWST_STACK_ENTRY *entry = HwstpAllocateStackEntry();

    if (entry == NULL) {
        return -ENOMEM;
    }

    HwstpInitEntry(entry, BracketType, Position);

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

static bool HwstIsStackEmpty(void)
{
    if (list_empty(&HwstStackTop)) {
        // ASSERT(HwstStackSize == 0);

        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

// static __u32 HwstGetStackSize(void) {
//   //
//   //
//   //

//   return HwstStackSize;
// }

///////////////////////////////////////////////////////////////////////////////

static void HwstPurgeEntriesStack(void)
{
    struct HWST_STACK_ENTRY *entry;
    struct HWST_STACK_ENTRY *nextEntry;

    list_for_each_entry_safe(entry, nextEntry, &HwstStackTop, Links)
    {
        HwstpRemoveEntry(entry);

        // pr_info(
        //     "HwstPurgeEntriesStack(): freeing entry 0x%px (type %u position
        //     %u)\n", entry, entry->BracketType, entry->Position);

        HwstpFreeStackEntry(entry);
    }

    // ASSERT(HwstStackSize == 0);
}

///////////////////////////////////////////////////////////////////////////////

static enum HWST_BRACKET_TYPE HwstGetBracketType(char Symbol)
{
    enum HWST_BRACKET_TYPE type;

    switch (Symbol) {
        case '(':
        case ')':
            type = HwstBracketTypeRound;
            break;

        case '[':
        case ']':
            type = HwstBracketTypeSquare;
            break;

        case '{':
        case '}':
            type = HwstBracketTypeCurly;
            break;

        default:
            type = HwstBracketTypeNone;
            break;
    } // switch (Symbol)

    return type;
}

///////////////////////////////////////////////////////////////////////////////

static char HwstBracketTypeToSymbol(enum HWST_BRACKET_TYPE BracketType)
{
    char symbol;

    switch (BracketType) {
        case HwstBracketTypeRound:
            symbol = '(';
            break;

        case HwstBracketTypeSquare:
            symbol = '[';
            break;

        case HwstBracketTypeCurly:
            symbol = '{';
            break;

        default:
            symbol = '~';
            break;
    } // switch (BracketType)

    return symbol;
}

///////////////////////////////////////////////////////////////////////////////

static inline bool HwstIsOpeningBracket(char Symbol)
{
    if ((Symbol == '(') || (Symbol == '[') || (Symbol == '{')) {
        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

static inline bool HwstIsClosingBracket(char Symbol)
{
    if ((Symbol == ')') || (Symbol == ']') || (Symbol == '}')) {
        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

static int HwstSetParam(const char *val, const struct kernel_param *kp)
{
    __u32 valLength = strlen(val);

    if (valLength && (val[valLength - 1] == ASCII_LF)) {
        --valLength;
    }

    if (valLength == 0) {
        pr_err("HwstSetParam(): invalid input string\n");

        return -EINVAL;
    }

    int result = 0;
    __u32 index = 0;
    __u32 stats[HwstBracketTypeMax] = {0};

    while ((val[index] != ASCII_LF) && (val[index] != ASCII_NULL)) {
        if (!isprint(val[index])) {
            pr_err("HwstSetParam(): 0x%02X is not a printable symbol, quitting\n", val[index]);

            result = -EINVAL;
            break;
        }

        if (HwstIsOpeningBracket(val[index])) {
            enum HWST_BRACKET_TYPE type = HwstGetBracketType(val[index]);

            result = HwstPushEntryStack(type, index);

            if (result != 0) {
                break;
            }

            ++stats[type];
        }

        else if (HwstIsClosingBracket(val[index])) {
            struct HWST_STACK_ENTRY *entry = HwstPopEntryStack();

            if (entry == NULL) {
                pr_err("HwstSetParam(): no opening bracket for '%c' @ %u, quitting\n", val[index],
                       index);

                result = -ENODATA;
                break;
            }

            enum HWST_BRACKET_TYPE openingType = entry->BracketType;
            enum HWST_BRACKET_TYPE closingType = HwstGetBracketType(val[index]);
            __u32 position = entry->Position;

            HwstpFreeStackEntry(entry);

            if (closingType != openingType) {
                pr_err("HwstSetParam(): closing bracket '%c' @ %u doesn't match the "
                       "opening one '%c' @ %u, quitting\n",
                       val[index], index, HwstBracketTypeToSymbol(openingType), position);

                result = -EINVAL;
                break;
            }
        }

        ++index;

    } // while ((val[index] != ASCII_LF) && (val[index] != ASCII_NULL))

    if (result == 0) {
        if (HwstIsStackEmpty()) {
            pr_info("HwstSetParam(): ========== statistics ==========\n");

            if (stats[HwstBracketTypeRound]) {
                pr_info("HwstSetParam(): '()' pairs: %u\n", stats[HwstBracketTypeRound]);
            }

            if (stats[HwstBracketTypeSquare]) {
                pr_info("HwstSetParam(): '[]' pairs: %u\n", stats[HwstBracketTypeSquare]);
            }

            if (stats[HwstBracketTypeCurly]) {
                pr_info("HwstSetParam(): '{}' pairs: %u\n", stats[HwstBracketTypeCurly]);
            }
        }

        else {
            // pr_err("HwstSetParam(): %u unpaired bracket(s) left:\n",
            //        HwstGetStackSize());

            struct HWST_STACK_ENTRY *entry;
            struct HWST_STACK_ENTRY *nextEntry;

            list_for_each_entry_safe(entry, nextEntry, &HwstStackTop, Links)
            {
                HwstpRemoveEntry(entry);

                pr_err("HwstSetParam(): no closing bracket for '%c' @ %u\n",
                       HwstBracketTypeToSymbol(entry->BracketType), entry->Position);

                HwstpFreeStackEntry(entry);
            }

            result = -EINVAL;
        }
    }

    else {
        HwstPurgeEntriesStack();
    }

    return result;
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
MODULE_DESCRIPTION("A stack testing module 1 for the Linux kernel");

///////////////////////////////////////////////////////////////////////////////
