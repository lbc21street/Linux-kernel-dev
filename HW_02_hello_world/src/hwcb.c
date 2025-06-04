#define pr_fmt(fmt) "[" KBUILD_MODNAME "] " fmt
// #define pr_fmt(fmt) "[" KBUILD_MODNAME "] " fmt

#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

#include "hwcb.h"

///////////////////////////////////////////////////////////////////////////////

static const struct kernel_param_ops HwcbParamOps = {
    .set = HwcbSetParam,
    .get = HwcbGetParam,
};

///////////////////////////////////////////////////////////////////////////////

static unsigned int idx;
static char ch_val;
static char str_buf[HWCB_MAX_SYMBOLS + sizeof(ASCII_NULL)];

module_param_cb(idx, &HwcbParamOps, &idx, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
module_param_cb(ch_val, &HwcbParamOps, &ch_val, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
module_param_cb(str_buf, &HwcbParamOps, str_buf, S_IRUSR | S_IRGRP | S_IROTH);

MODULE_PARM_DESC(idx, "Last symbol index set");
MODULE_PARM_DESC(ch_val, "Last symbol set");
MODULE_PARM_DESC(str_buf, "Resulting string of symbols previously set");

///////////////////////////////////////////////////////////////////////////////

// static inline void HwcbLockParam(void) { kernel_param_lock(THIS_MODULE); }

// static inline void HwcbUnlockParam(void) { kernel_param_unlock(THIS_MODULE);
// }

///////////////////////////////////////////////////////////////////////////////

static int HwcbGetParam(char *buffer, const struct kernel_param *kp)
{
    if (kp->arg == &idx) {
        pr_info("HwcbGetParam(): idx -> %u\n", idx);

        return param_get_uint(buffer, kp);
    }

    if (kp->arg == &ch_val) {
        pr_info("HwcbGetParam(): ch_val -> '%c' (0x%02X)\n", ch_val, ch_val);

        buffer[0] = ch_val;

        return 1;
    }

    if (kp->arg == str_buf) {
        pr_info("HwcbGetParam(): str_buf -> '%s'\n", str_buf);

        return scnprintf(buffer, PAGE_SIZE, "%s", str_buf);
    }

    return -EINVAL;
}

static int HwcbSetParam(const char *val, const struct kernel_param *kp)
{
    __u32 valLength = strlen(val);

    if (valLength && (val[valLength - 1] == ASCII_LF)) {
        --valLength;
    }

    if (valLength == 0) {
        pr_err("HwstSetParam(): invalid input string\n");

        return -EINVAL;
    }

    int result;

    if (kp->arg == &idx) {
        __u32 value = 0;

        result = kstrtouint(val, 10, &value);

        if (result != 0) {
            pr_err("HwcbSetParam(): kstrtouint() failed %d\n", result);

            return result;
        }

        if (value >= HWCB_MAX_SYMBOLS) {
            pr_err("HwcbSetParam(): idx value %u is too large, max %u\n", value,
                   HWCB_MAX_SYMBOLS - 1);

            return -ERANGE;
        }

        pr_info("HwcbSetParam(): idx <- %u\n", value);

        idx = value;

        return 0;
    }

    if (kp->arg == &ch_val) {
        if (!isprint(val[0])) {
            pr_err("HwcbSetParam(): ch_val value 0x%02X is not a printable symbol\n", val[0]);

            return -EINVAL;
        }

        pr_info("HwcbSetParam(): str_buf[%u] ch_val <- '%c' (0x%02X)\n", idx, val[0], val[0]);

        ch_val = val[0];

        str_buf[idx] = ch_val;

        return 0;
    }

    if (kp->arg == str_buf) {
        pr_err("HwcbSetParam(): setting str_buf is prohibited\n");

        return -EPERM;
    }

    return -EINVAL;
}

///////////////////////////////////////////////////////////////////////////////

static int __init HwcbInit(void)
{
    pr_info("HwcbInit(): init\n");

    return 0;
}

static void __exit HwcbExit(void)
{
    //
    //
    //

    pr_info("HwcbExit(): exit\n");
}

///////////////////////////////////////////////////////////////////////////////

module_init(HwcbInit);
module_exit(HwcbExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("A param testing module for the Linux kernel");

///////////////////////////////////////////////////////////////////////////////
