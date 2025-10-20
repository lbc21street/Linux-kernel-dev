//=================================================================================================
//
// \file    paramsupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kstrtox.h>
#include <linux/log2.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

#include "data.h"
#include "supportmacros.h"

#include "paramsupport.h"

//
//
//

static int PwbdSetDeviceCount(const char *Value, const struct kernel_param *KernelParam);
static int PwbdSetPartitionCount(const char *Value, const struct kernel_param *KernelParam);
static int PwbdSetDiskSize(const char *Value, const struct kernel_param *KernelParam);
static int PwbdSetSectorSize(const char *Value, const struct kernel_param *KernelParam);

//
//
//

uint8_t devicecount = PWBD_DEFAULT_NUMBER_OF_DEVICES;

static const struct kernel_param_ops PwbdpDeviceCount = {.set = PwbdSetDeviceCount,
                                                         .get = param_get_byte};

module_param_cb(devicecount, &PwbdpDeviceCount, &devicecount,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(devicecount, "Maximum number of devices");

//
//
//

uint8_t partitioncount = PWBD_DEFAULT_NUMBER_OF_PARTITIONS;

static const struct kernel_param_ops PwbdpPartitionCount = {.set = PwbdSetPartitionCount,
                                                            .get = param_get_byte};

module_param_cb(partitioncount, &PwbdpPartitionCount, &partitioncount,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(partitioncount, "Maximum number of partitions per one device");

//
//
//

uint16_t sectorsize = PWBD_DEFAULT_SECTOR_SIZE;

static const struct kernel_param_ops PwbdpSectorSize = {.set = PwbdSetSectorSize,
                                                        .get = param_get_ushort};

module_param_cb(sectorsize, &PwbdpSectorSize, &sectorsize, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(sectorsize, "Device sector size");

//
//
//

uint32_t disksize = PWBD_DEFAULT_DISK_SIZE_MB;

static const struct kernel_param_ops PwbdpDiskSize = {.set = PwbdSetDiskSize,
                                                      .get = param_get_ulong};

module_param_cb(disksize, &PwbdpDiskSize, &disksize, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(disksize, "Disk size (in megabytes)");

//
//
//

static int PwbdpCheckParamString(const char *Val, uint32_t *Length)
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

static int PwbdSetDeviceCount(const char *Value, const struct kernel_param *KernelParam)
{
    if (PwbdpIsParametersCaptured()) {
        return -EBUSY;
    }

    int result = PwbdpCheckParamString(Value, NULL);

    if (result != 0) {
        return result;
    }

    uint8_t deviceCount = 0;

    result = kstrtou8(Value, 10, &deviceCount);

    if (result != 0) {
        pr_err("kstrtou8() failed %d", result);

        return result;
    }

    pr_info("got device count %u", deviceCount);

    if ((deviceCount < 1) || (deviceCount > PWBD_MAX_NUMBER_OF_DEVICES)) {
        pr_err("invalid number of devices %u specified\n", deviceCount);

        return -EINVAL;
    }

    return param_set_byte(Value, KernelParam);
}

//
//
//

static int PwbdSetPartitionCount(const char *Value, const struct kernel_param *KernelParam)
{
    if (PwbdpIsParametersCaptured()) {
        return -EBUSY;
    }

    int result = PwbdpCheckParamString(Value, NULL);

    if (result != 0) {
        return result;
    }

    uint8_t partitionCount = 0;

    result = kstrtou8(Value, 10, &partitionCount);

    if (result != 0) {
        pr_err("kstrtou8() failed %d", result);

        return result;
    }

    pr_info("got partition count %u", partitionCount);

    if ((partitionCount < 1) || (partitionCount > PWBD_MAX_NUMBER_OF_PARTITIONS)) {
        pr_err("invalid number of partitions specified (%u)\n", partitionCount);

        return -EINVAL;
    }

    return param_set_byte(Value, KernelParam);
}

//
//
//

static int PwbdSetSectorSize(const char *Value, const struct kernel_param *KernelParam)
{
    if (PwbdpIsParametersCaptured()) {
        return -EBUSY;
    }

    int result = PwbdpCheckParamString(Value, NULL);

    if (result != 0) {
        return result;
    }

    uint16_t sectorSize = 0;

    result = kstrtou16(Value, 10, &sectorSize);

    if (result != 0) {
        pr_err("kstrtou16() failed %d", result);

        return result;
    }

    uint16_t roundedSectorSize = (uint16_t)rounddown_pow_of_two(sectorSize);

    pr_info("got sector size %u, rounded down to %u", sectorSize, roundedSectorSize);

    if ((roundedSectorSize < PWBD_MIN_SECTOR_SIZE) || (roundedSectorSize > PWBD_MAX_SECTOR_SIZE)) {
        pr_err("invalid sector size specified %u (%u)\n", roundedSectorSize, sectorSize);

        return -EINVAL;
    }

    return param_set_ushort(Value, KernelParam);
}

//
//
//

static int PwbdSetDiskSize(const char *Value, const struct kernel_param *KernelParam)
{
    if (PwbdpIsParametersCaptured()) {
        return -EBUSY;
    }

    int result = PwbdpCheckParamString(Value, NULL);

    if (result != 0) {
        return result;
    }

    uint32_t diskSize = 0;

    result = kstrtou32(Value, 10, &diskSize);

    if (result != 0) {
        pr_err("kstrtou32() failed %d", result);

        return result;
    }

    pr_info("got disk size %u MB", diskSize);

    if ((diskSize < PWBD_MIN_DISK_SIZE_MB) || (diskSize > PWBD_MAX_DISK_SIZE_MB)) {
        pr_err("invalid disk size specified %u\n", diskSize);

        return -EINVAL;
    }

    return param_set_ushort(Value, KernelParam);
}

//=================================================================================================
