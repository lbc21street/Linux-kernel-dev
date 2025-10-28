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

#include "devicesupport.h"
#include "paramsupport.h"
#include "sysfssupport.h"
#include "workqueuesupport.h"

//
//
//

#ifdef PWBD_DETAILED_TRACE
#pragma message "-------- Detailed trace"
#endif // PWBD_DETAILED_TRACE

#ifdef PWBD_USE_MQ
#pragma message "-------- Using MQ"
#else // PWBD_USE_MQ
#pragma message "-------- Using legacy submit_bio"
#endif // PWBD_USE_MQ

#ifdef PWBD_MQ_DIAG
#pragma message "-------- MQ diag stuff"
#endif // PWBD_USE_MQ

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
    atomic_or(PWBD_CTLFL_PARAMETERS_CAPTURED, (atomic_t *)&PwbdCtrl.Flags);

    //
    // capture sysfs module parameters
    //

    PwbdCtrl.NumberOfDevices = devicecount;
    PwbdCtrl.NumberOfPartitions = partitioncount;
    PwbdCtrl.SectorSize = sectorsize;
    PwbdCtrl.DiskSize = MB_TO_BYTES(disksize + PWBD_RESERVED_DISK_SIZE_MB);

    pr_info("captured NumberOfDevices %u NumberOfPartitions %u SectorSize %u DiskSize %llu",
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

    pr_info("entering");

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
MODULE_VERSION("1.0");

//=================================================================================================
