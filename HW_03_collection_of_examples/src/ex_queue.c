//=================================================================================================
//
// \file    ex_queue.c
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
#include <linux/kfifo.h>
#include <linux/types.h>

#include "ex_queue.h"
#include "supportmacros.h"

//
//
//

static int ExqGetParam(char *buffer, const struct kernel_param *kp);

static int ExqSetParam(const char *val, const struct kernel_param *kp);

//
//
//

static const struct kernel_param_ops ExqParamOps = {
    .set = ExqSetParam,
    .get = ExqGetParam,
};

char *cmd = "";

module_param_cb(cmd, &ExqParamOps, NULL, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(cmd, "Just a parameter placeholder for incoming commands");

static uint32_t value;

module_param_cb(value, &ExqParamOps, &value, S_IRUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(value, "Some return value");

//
//
//

EXQ_CTRL ExqCtrl = {
    .Active = TRUE,
};

//
//
//

static int ExqCreateQueue(void)
{
    int result = kfifo_alloc(&ExqCtrl.Queue, EXQ_MAX_DATA_ENTRIES, GFP_KERNEL);

    if (result != 0) {
        pr_err("kfifo_alloc() failed %d", result);

        return result;
    }

    //
    // only for a fifo declared by DECLARE_KFIFO
    //
    // INIT_KFIFO(ExqCtrl.Queue);
    //

    pr_info("inited queue - esize %u recsize %u size %u", kfifo_esize(&ExqCtrl.Queue),
            (uint32_t)kfifo_recsize(&ExqCtrl.Queue), kfifo_size(&ExqCtrl.Queue));

    SetFlag(ExqCtrl.Flags, EXQ_CTLFL_QUEUE_CREATED);

    return result;
}

//
//
//

static void ExqDestroyQueue(void)
{
    if (!FlagOn(ExqCtrl.Flags, EXQ_CTLFL_QUEUE_CREATED)) {
        return;
    }

    pr_info("freeing queue - size %u length %u", kfifo_size(&ExqCtrl.Queue),
            kfifo_len(&ExqCtrl.Queue));

    //
    // do we really have to reset it?
    //

    kfifo_reset(&ExqCtrl.Queue);
    kfifo_free(&ExqCtrl.Queue);

    ClearFlag(ExqCtrl.Flags, EXQ_CTLFL_QUEUE_CREATED);
}

//
//
//

static bool ExqIsDataEntryQueueEmpty(void)
{
    if (kfifo_is_empty(&ExqCtrl.Queue)) {
        return TRUE;
    }

    return FALSE;
}

//
//
//

static int ExqEnqueueDataEntry(uint32_t Value)
{
    int result = kfifo_put(&ExqCtrl.Queue, Value);

    if (result == 0) {
        pr_warn("queue is full (length %u)", kfifo_len(&ExqCtrl.Queue));

        return -EINVAL;
    }

    pr_info("added value %u (length %u) (result %d)", Value, kfifo_len(&ExqCtrl.Queue), result);

    return 0;
}

//
//
//

static int ExqDequeueDataEntry(uint32_t *Value)
{
    uint32_t localValue;
    int result = kfifo_get(&ExqCtrl.Queue, &localValue);

    if (result == 0) {
        pr_warn("queue is empty (max length %u)", kfifo_size(&ExqCtrl.Queue));

        return -ENODATA;
    }

    pr_info("removed value %u (length %u) (result %d)", localValue, kfifo_len(&ExqCtrl.Queue),
            result);

    if (ARGUMENT_PRESENT(Value)) {
        *Value = localValue;
    }

    return 0;
}

//
//
//

static int ExqPeekDataEntry(uint32_t *Value)
{
    uint32_t localValue;
    int result = kfifo_peek(&ExqCtrl.Queue, &localValue);

    if (result == 0) {
        pr_warn("queue is empty (max length %u)", kfifo_size(&ExqCtrl.Queue));

        return -ENODATA;
    }

    pr_info("peeked value %u (length %u) (result %d)", localValue, kfifo_len(&ExqCtrl.Queue),
            result);

    if (ARGUMENT_PRESENT(Value)) {
        *Value = localValue;
    }

    return 0;
}

//
//
//

static uint32_t ExqGetNumberOfDataEntries(void)
{
    value = kfifo_len(&ExqCtrl.Queue);

    return value;
}

//
//
//

static uint32_t ExqGetNumberOfAvailableDataEntries(void)
{
    value = kfifo_avail(&ExqCtrl.Queue);

    return value;
}

//
//
//

static int ExqHashDataEntries(void)
{
    if (ExqIsDataEntryQueueEmpty()) {
        pr_warn("queue is empty");

        return -ENODATA;
    }

    uint32_t count = kfifo_len(&ExqCtrl.Queue);
    uint32_t bufferSize = count * sizeof(uint32_t);

    uint32_t *buffer = (uint32_t *)kmalloc(bufferSize, GFP_KERNEL);

    if (buffer == NULL) {
        pr_err("memory alloc failed for fifo buffer (%u bytes)", bufferSize);

        return -ENOMEM;
    }

    pr_info("allocated buffer 0x%px for %u entry(s)", buffer, count);

    uint32_t copiedEntries = kfifo_out_peek(&ExqCtrl.Queue, buffer, count);

    pr_info("copied %u entry(s)", copiedEntries);

    uint32_t index = 0;
    uint32_t hash = 0;

    while (index < copiedEntries) {

        hash ^= buffer[index] + 7 * index;

        ++index;

    } // while (index < copiedEntries)

    pr_info("freeing buffer 0x%px", buffer);

    kfree(buffer);

    hash ^= (copiedEntries << 16 | copiedEntries);

    pr_info("hash 0x%08X", hash);

    value = hash;

    return 0;
}

//
//
//

static int ExqResetDataEntries(void)
{
    if (ExqIsDataEntryQueueEmpty()) {
        pr_warn("queue is empty");

        return -ENODATA;
    }

    pr_info("resetting queue (length %u)", kfifo_len(&ExqCtrl.Queue));

    kfifo_reset(&ExqCtrl.Queue);

    return 0;
}

//
//
//

static int ExqGetParam(char *buffer, const struct kernel_param *kp)
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

static bool ExqCheckCommand(const char *val, uint32_t valLength, const char *cmd)
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

static int ExqSetParam(const char *val, const struct kernel_param *kp)
{
    if (!ExqCtrl.Active) {
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

    if (!strncmp(val, EXQ_CMD_ADD, EXQ_CMD_ADD_LENGTH)) {
        uint32_t value;

        result = kstrtouint(val + EXQ_CMD_ADD_LENGTH, 10, &value);

        if (result != 0) {
            pr_err("kstrtouint() failed %d", result);

            return result;
        }

        return ExqEnqueueDataEntry(value);
    }

    if (ExqCheckCommand(val, valLength, EXQ_CMD_REMOVE)) {
        return ExqDequeueDataEntry(NULL);
    }

    if (ExqCheckCommand(val, valLength, EXQ_CMD_PEEK)) {
        return ExqPeekDataEntry(NULL);
    }

    if (ExqCheckCommand(val, valLength, EXQ_CMD_LEN)) {
        pr_info("number of data entries %u", ExqGetNumberOfDataEntries());

        return 0;
    }

    if (ExqCheckCommand(val, valLength, EXQ_CMD_AVAIL)) {
        pr_info("number of available data entries %u", ExqGetNumberOfAvailableDataEntries());

        return 0;
    }

    if (ExqCheckCommand(val, valLength, EXQ_CMD_HASH)) {
        return ExqHashDataEntries();
    }

    if (ExqCheckCommand(val, valLength, EXQ_CMD_RESET)) {
        return ExqResetDataEntries();
    }

    if (ExqCheckCommand(val, valLength, EXQ_CMD_EXIT)) {
        ExqCtrl.Active = FALSE;

        pr_info("stopped command processing");

        return 0;
    }

    pr_err("invalid command <%.*s>", valLength, val);

    return -EINVAL;
}

//
//
//

static int __init ExqInit(void)
{
    int result = 0;

    pr_info("entering...");

    do {
        result = ExqCreateQueue();

        if (result != 0) {
            break;
        }

    } while (FALSE);

    if (result != 0) {
        ExqDestroyQueue();

        pr_info("leaving, result %d", result);
    }

    else {
        pr_info("leaving...");
    }

    return result;
}

//
//
//

static void __exit ExqExit(void)
{
    pr_info("entering...");

    ExqDestroyQueue();

    pr_info("leaving...");
}

//
//
//

module_init(ExqInit);
module_exit(ExqExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("ex_queue - a simple Linux kernel driver for testing the queue API");

//=================================================================================================
