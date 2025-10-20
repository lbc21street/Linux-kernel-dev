//=================================================================================================
//
// \file    sysfssupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/ctype.h>
#include <linux/kernel.h>

#include <linux/device/class.h>
#include <linux/sysfs.h>

#include "data.h"
#include "supportmacros.h"

#include "devicesupport.h"
#include "sysfssupport.h"

//
//
//

[[nodiscard]] int PwbdCreateClass(void)
{
    int result = 0;

    PwbdCtrl.Class = class_create(PWBD_DEVICE_NAME); // 6.8 has only one param

    if (!IS_ERR(PwbdCtrl.Class)) {
        pr_info("created class 0x%px", PwbdCtrl.Class);
    }

    else {
        result = PTR_ERR(PwbdCtrl.Class);

        pr_err("class_create() failed %d", result);
    }

    return result;
}

//
//
//

void PwbdDestroyClass(void)
{
    if (PwbdCtrl.Class == NULL) {
        return;
    }

    pr_info("destroying class 0x%px", PwbdCtrl.Class);

    class_destroy(PwbdCtrl.Class);
    PwbdCtrl.Class = NULL;
}

//=================================================================================================
