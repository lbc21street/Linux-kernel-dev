//=================================================================================================
//
// \file    init.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/atomic.h>
#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "data.h"
#include "supportmacros.h"

#define PWBD_COMPONENT_TRACE_MASK PWBD_TM_INIT
#include "tracesupport.h"

#include "devicesupport.h"
#include "paramsupport.h"
#include "sysfssupport.h"
#include "workqueuesupport.h"

//
//
//

#ifdef PWBD_USE_MQ
#pragma message "-------- Using MQ"
#else // PWBD_USE_MQ
#pragma message "-------- Using legacy submit_bio"
#endif // PWBD_USE_MQ

#ifdef PWBD_MQ_DIAG
#pragma message "-------- MQ diag stuff"
#endif // PWBD_USE_MQ

#ifdef PWBD_USE_GEOMETRY_CAPACITY
#pragma message "-------- Using full geometry capacity"
#endif // PWBD_USE_GEOMETRY_CAPACITY

//
//
//

PWBD_CTRL PwbdCtrl;

//
//
//

static void PwbdpCaptureDeviceParameters(void)
{
    // SetFlag(PwbdCtrl.Flags, PWBD_CTLFL_PARAMETERS_CAPTURED);
    raw_atomic_or(PWBD_CTLFL_PARAMETERS_CAPTURED, (atomic_t *)&PwbdCtrl.Flags);

    //
    // capture sysfs module parameters
    //

    PwbdCtrl.NumberOfDevices = devicecount;
    PwbdCtrl.NumberOfPartitions = partitioncount;
    PwbdCtrl.SectorSize = sectorsize;
    PwbdCtrl.DiskSize = MB_TO_BYTES(disksize);

    pr_info_tl(PWBD_TL_1,
               "captured NumberOfDevices %u NumberOfPartitions %u SectorSize %u DiskSize %llu",
               PwbdCtrl.NumberOfDevices, PwbdCtrl.NumberOfPartitions, PwbdCtrl.SectorSize,
               PwbdCtrl.DiskSize);
}

//
//
//

static void PwbdpTeardown(void)
{
    PwbdUninitializeDevices();

    PwbdRemoveClassAttributeAdd();

    PwbdDestroyClass();

    PwbdDestroyDeviceRemovalWorkQueue();
}

//
//
//

static int __init PwbdInit(void)
{
    int result = 0;

    pr_info_tl(PWBD_TL_1, "entering...");

    do {
        //
        // capture sysfs module parameters
        //

        PwbdpCaptureDeviceParameters();

        result = PwbdAllocateDeviceRemovalWorkQueue();

        if (result != 0) {
            break;
        }

        result = PwbdCreateClass();

        if (result != 0) {
            break;
        }

        result = PwbdCreateClassAttributeAdd();

        if (result != 0) {
            break;
        }

        result = PwbdInitializeDevices();

        if (result != 0) {
            break;
        }

    } while (FALSE);

    if (result == 0) {
        pr_info_tl(PWBD_TL_1, "leaving...");
    }

    else {
        PwbdpTeardown();

        pr_err_tl(PWBD_TL_1, "leaving, result %d", result);
    }

    return result;
}

//
//
//

static void __exit PwbdExit(void)
{
    pr_info_tl(PWBD_TL_1, "entering...");

    PwbdpTeardown();

    pr_info_tl(PWBD_TL_1, "leaving...");
}

//
//
//

module_init(PwbdInit);
module_exit(PwbdExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("pwblkdev - a simple test block device driver for the Linux kernel");
MODULE_VERSION("1.0");

//=================================================================================================
