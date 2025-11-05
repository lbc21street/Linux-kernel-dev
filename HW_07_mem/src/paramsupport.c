//=================================================================================================
//
// \file    paramsupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kstrtox.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/types.h>

#include "supportmacros.h"

#include "memtest.h"
#include "paramsupport.h"

//
//
//

static int MtpSetMemoryTestMode(const char *Value, const struct kernel_param *KernelParam);

//
//
//

uint8_t testmode = MT_MODE_INVALID;

static const struct kernel_param_ops MtTestModeParamOps = {.set = MtpSetMemoryTestMode,
                                                           .get = param_get_byte};

module_param_cb(testmode, &MtTestModeParamOps, &testmode, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(testmode, "Memory test mode");

//
//
//

static int MtpCheckParamString(const char *Val, uint32_t *Length)
{
    uint32_t valLength = strlen(Val);

    if (valLength && (Val[valLength - 1] == ASCII_LF)) {
        --valLength;
    }

    if (valLength == 0) {
        pr_err("invalid param string");

        return -EINVAL;
    }

    if (ARGUMENT_PRESENT(Length)) {
        *Length = valLength;
    }

    return 0;
}

//
//
//

static inline bool MtpIsParametersCaptured(void)
{
    return BooleanFlagOn(MtCtrl.Flags, MT_CTLFL_PARAMETERS_CAPTURED);
}

//
//
//

static int MtpSetMemoryTestMode(const char *Value, const struct kernel_param *KernelParam)
{
    if (MtpIsParametersCaptured()) {
        return -EBUSY;
    }

    int result = MtpCheckParamString(Value, NULL);

    if (result != 0) {
        return result;
    }

    uint8_t localTestMode = 0;

    result = kstrtou8(Value, 10, &localTestMode);

    if (result != 0) {
        pr_err("kstrtou8() failed %d", result);

        return result;
    }

    pr_info("got test mode %u", localTestMode);

    if ((localTestMode <= MT_MODE_INVALID) || (localTestMode > MT_MODE_MAX)) {
        pr_err("invalid test mode %u specified\n", localTestMode);

        return -EINVAL;
    }

    testmode = localTestMode;

    return 0;
}

//=================================================================================================
