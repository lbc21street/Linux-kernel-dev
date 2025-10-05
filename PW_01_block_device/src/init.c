//=================================================================================================
//
// \file    init.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#include "data.h"
#include "supportmacros.h"

#include "devicesupport.h"
#include "queuesupport.h"
#include "iosupport.h"

//
//
//

static int PwbdSetParam(const char *val, const struct kernel_param *kp);

//
//
//

static const struct kernel_param_ops PwbdParamOps = {
    .set = PwbdSetParam,
    .get = NULL,
};

char *cmd = "";

module_param_cb(cmd, &PwbdParamOps, NULL, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(cmd, "Just a parameter placeholder for incoming commands");

//
//
//

PWBD_CTRL PwbdCtrl;

//
//
//

static inline struct Pwbd_DATA_ENTRY *PwbdpAllocateDataEntry(void)
{
    __u32 dataLength = sizeof(struct Pwbd_DATA_ENTRY);

    struct Pwbd_DATA_ENTRY *entry = (struct Pwbd_DATA_ENTRY *)kmalloc(dataLength, GFP_KERNEL);

    if (entry == NULL) {
        pr_err("memory alloc failed for data entry (%u bytes)", dataLength);
    }

    return entry;
}

//
//
//

static inline void PwbdpFreeDataEntry(struct Pwbd_DATA_ENTRY *Entry)
{
    kfree(Entry);
}

//
//
//

static bool PwbdCheckCommand(const char *val, __u32 valLength, const char *cmd)
{
    __u32 length = max(valLength, strlen(cmd));

    if (!strncmp(val, cmd, length)) {
        return TRUE;
    }

    return FALSE;
}

//
//
//

static int PwbdSetParam(const char *val, const struct kernel_param *kp)
{
    // if (!PwbdActive) {
    //     pr_warn("command processing is stopped");

    //     return -ENODEV;
    // }

    __u32 valLength = strlen(val);

    if (valLength && (val[valLength - 1] == ASCII_LF)) {
        --valLength;
    }

    if (valLength == 0) {
        pr_err("invalid input string");

        return -EINVAL;
    }

    if (PwbdCheckCommand(val, valLength, "test"))
    {
        return 0;
    }


    pr_err("invalid command <%.*s>", valLength, val);

    return -EINVAL;
}

//
//
//

static void PwbdpTeardown(void)
{
    PwbdpUnregisterBlockDevice();

    PwbdpDestroyDisk();

    PwbdpFreeTagSet();
}

//
//
//

static int __init PwbdInit(void)
{
    int result = 0;

    pr_info("entering");

    do
    {
        result = PwbdpRegisterBlockDevice();

        if (result != 0) {
            break;
        }

        PwbdpInitStaticMqOps();
        PwbdpInitStaticTagSet();
        PwbdpInitStaticDevOps();

        result = PwbdpAllocateTagSet();

        if (result != 0) {
            break;
        }

        result = PwbdpCreateDisk();

        if (result != 0) {
            break;
        }

    } while (FALSE);

    if (result != 0) {
        PwbdpTeardown();
    }

    pr_info("leaving, result %d", result);

    return result;
}

//
//
//

static void __exit PwbdExit(void)
{
    pr_info("entering");

    PwbdpTeardown();

    pr_info("leaving");
}

//
//
//

module_init(PwbdInit);
module_exit(PwbdExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("pwblkdev - a simple test block device driver for the Linux kernel");

//=================================================================================================
