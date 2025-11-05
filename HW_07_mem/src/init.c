//=================================================================================================
//
// \file    init.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/atomic.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/types.h>

#include "memtest.h"
#include "paramsupport.h"
#include "supportmacros.h"

//
//
//

MT_CTRL MtCtrl;

//
//
//

static int __init MtInit(void)
{
    int result = 0;

    pr_info("entering...");

    do {
        atomic_or(MT_CTLFL_PARAMETERS_CAPTURED, (atomic_t *)&MtCtrl.Flags);

        MtCtrl.TestMode = testmode;

        if (MtCtrl.TestMode == MT_MODE_INVALID) {
            result = -EINVAL;

            pr_err("invalid memory test mode %u", MtCtrl.TestMode);

            break;
        }

        MtCtrl.TotalRamPages = totalram_pages();
        MtCtrl.TotalRamBytes = (uint64_t)MtCtrl.TotalRamPages << PAGE_SHIFT;

        result = MtTestMemory(MtCtrl.TestMode);

    } while (FALSE);

    if (result != 0) {
        pr_err("leaving, result %d", result);
    }

    else {
        pr_info("leaving...");
    }

    return result;
}

//
//
//

static void __exit MtExit(void)
{
    pr_info("entering...");

    pr_info("leaving...");
}

//
//
//

module_init(MtInit);
module_exit(MtExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION(
    "memtest - a simple driver for the Linux kernel testing various memory allocation techniques");
MODULE_VERSION("1.0");

//=================================================================================================
