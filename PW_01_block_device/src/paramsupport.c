//=================================================================================================
//
// \file    paramsupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/atomic.h>
#include <linux/ctype.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kstrtox.h>
#include <linux/log2.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

#include "data.h"
#include "supportmacros.h"

#define PWBD_COMPONENT_TRACE_MASK PWBD_TM_PARAM_SUPPORT
#include "tracesupport.h"

#include "paramsupport.h"

//
//
//

static int PwbdSetDeviceCount(const char *Value, const struct kernel_param *KernelParam);
static int PwbdSetPartitionCount(const char *Value, const struct kernel_param *KernelParam);
static int PwbdSetDiskSize(const char *Value, const struct kernel_param *KernelParam);
static int PwbdSetSectorSize(const char *Value, const struct kernel_param *KernelParam);

static int PwbdSetTraceLevel(const char *Value, const struct kernel_param *KernelParam);

static int PwbdSetTraceMask(const char *Value, const struct kernel_param *KernelParam);
static int PwbdGetTraceMask(char *Buffer, const struct kernel_param *KernelParam);

//
//
//

uint8_t devicecount = PWBD_DEFAULT_NUMBER_OF_DEVICES;

static const struct kernel_param_ops PwbdpDeviceCountParamOps = {.set = PwbdSetDeviceCount,
                                                                 .get = param_get_byte};

module_param_cb(devicecount, &PwbdpDeviceCountParamOps, &devicecount,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(devicecount, "Maximum number of devices");

//
//
//

uint8_t partitioncount = PWBD_DEFAULT_NUMBER_OF_PARTITIONS;

static const struct kernel_param_ops PwbdpPartitionCountParamOps = {.set = PwbdSetPartitionCount,
                                                                    .get = param_get_byte};

module_param_cb(partitioncount, &PwbdpPartitionCountParamOps, &partitioncount,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(partitioncount, "Maximum number of partitions per one device");

//
//
//

uint16_t sectorsize = PWBD_DEFAULT_SECTOR_SIZE;

static const struct kernel_param_ops PwbdpSectorSizeParamOps = {.set = PwbdSetSectorSize,
                                                                .get = param_get_ushort};

module_param_cb(sectorsize, &PwbdpSectorSizeParamOps, &sectorsize,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(sectorsize, "Device sector size");

//
//
//

uint32_t disksize = PWBD_DEFAULT_DISK_SIZE_MB;

static const struct kernel_param_ops PwbdpDiskSizeParamOps = {.set = PwbdSetDiskSize,
                                                              .get = param_get_uint};

module_param_cb(disksize, &PwbdpDiskSizeParamOps, &disksize, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(disksize, "Disk size (in megabytes)");

//
//
//

uint32_t tracelevel = PWBD_TL_1;

static const struct kernel_param_ops PwbdpTraceLevelParamOps = {.set = PwbdSetTraceLevel,
                                                                .get = param_get_uint};

module_param_cb(tracelevel, &PwbdpTraceLevelParamOps, &tracelevel,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(tracelevel, "Trace level");

//
//
//

uint32_t tracemask = PWBD_TM_ALL;

static const struct kernel_param_ops PwbdpTraceMaskParamOps = {.set = PwbdSetTraceMask,
                                                               .get = PwbdGetTraceMask};

module_param_cb(tracemask, &PwbdpTraceMaskParamOps, &tracemask,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(tracemask, "Trace component mask");

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
        pr_err_tl(PWBD_TL_1, "invalid param string");

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

static inline bool PwbdpIsParametersCaptured(void)
{
    return BooleanFlagOn(PwbdCtrl.Flags, PWBD_CTLFL_PARAMETERS_CAPTURED);
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

    uint8_t numberDevices = 0;

    result = kstrtou8(Value, 10, &numberDevices);

    if (result != 0) {
        pr_err_tl(PWBD_TL_1, "kstrtou8() failed %d", result);

        return result;
    }

    pr_info_tl(PWBD_TL_1, "got device count %u", numberDevices);

    if ((numberDevices < 1) || (numberDevices > PWBD_MAX_NUMBER_OF_DEVICES)) {
        pr_err_tl(PWBD_TL_1, "invalid number of devices %u specified\n", numberDevices);

        return -EINVAL;
    }

    devicecount = numberDevices;

    return 0;
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

    uint8_t numberPartitions = 0;

    result = kstrtou8(Value, 10, &numberPartitions);

    if (result != 0) {
        pr_err_tl(PWBD_TL_1, "kstrtou8() failed %d", result);

        return result;
    }

    pr_info_tl(PWBD_TL_1, "got partition count %u", numberPartitions);

    if ((numberPartitions < 1) || (numberPartitions > PWBD_MAX_NUMBER_OF_PARTITIONS)) {
        pr_err_tl(PWBD_TL_1, "invalid number of partitions specified (%u)\n", numberPartitions);

        return -EINVAL;
    }

    partitioncount = numberPartitions;

    return 0;
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

    uint16_t rawSectorSize = 0;

    result = kstrtou16(Value, 10, &rawSectorSize);

    if (result != 0) {
        pr_err_tl(PWBD_TL_1, "kstrtou16() failed %d", result);

        return result;
    }

    uint16_t roundedSectorSize = (uint16_t)rounddown_pow_of_two(rawSectorSize);

    pr_info_tl(PWBD_TL_1, "got sector size %u, rounded down to %u", rawSectorSize,
               roundedSectorSize);

    if ((roundedSectorSize < PWBD_MIN_SECTOR_SIZE) || (roundedSectorSize > PWBD_MAX_SECTOR_SIZE)) {
        pr_err_tl(PWBD_TL_1, "invalid sector size specified %u (%u)\n", roundedSectorSize,
                  rawSectorSize);

        return -EINVAL;
    }

    sectorsize = roundedSectorSize;

    return 0;
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

    uint32_t diskSizeInMb = 0;

    result = kstrtou32(Value, 10, &diskSizeInMb);

    if (result != 0) {
        pr_err_tl(PWBD_TL_1, "kstrtou32() failed %d", result);

        return result;
    }

    pr_info_tl(PWBD_TL_1, "got disk size %u MB", diskSizeInMb);

    if ((diskSizeInMb < PWBD_MIN_DISK_SIZE_MB) || (diskSizeInMb > PWBD_MAX_DISK_SIZE_MB)) {
        pr_err_tl(PWBD_TL_1, "invalid disk size specified %u\n", diskSizeInMb);

        return -EINVAL;
    }

    disksize = diskSizeInMb;

    return 0;
}

//
//
//

static int PwbdSetTraceLevel(const char *Value, const struct kernel_param *KernelParam)
{
    int result = PwbdpCheckParamString(Value, NULL);

    if (result != 0) {
        return result;
    }

    uint32_t localTraceLevel = 0;

    result = kstrtou32(Value, 10, &localTraceLevel);

    if (result != 0) {
        pr_err_tl(PWBD_TL_1, "kstrtou32() failed %d", result);

        return result;
    }

    pr_info_tl(PWBD_TL_1, "got trace level %u", localTraceLevel);

    if (localTraceLevel > PWBD_TL_MAX) {
        pr_err_tl(PWBD_TL_1, "invalid trace level specified %u\n", localTraceLevel);

        return -EINVAL;
    }

    atomic_xchg((atomic_t *)&tracelevel, localTraceLevel);

    return 0;
}

//
//
//

static int PwbdSetTraceMask(const char *Value, const struct kernel_param *KernelParam)
{
    int result = PwbdpCheckParamString(Value, NULL);

    if (result != 0) {
        return result;
    }

    uint32_t localTraceMask = 0;

    result = kstrtou32(Value, 0, &localTraceMask);

    if (result != 0) {
        pr_err_tl(PWBD_TL_1, "kstrtou32() failed %d", result);

        return result;
    }

    pr_info_tl(PWBD_TL_1, "got trace mask 0x%08X", localTraceMask);

    if (FlagOn(localTraceMask, ~PWBD_TM_ALL)) {
        pr_err_tl(PWBD_TL_1, "invalid trace mask bits specified 0x%08X\n",
                  localTraceMask & ~PWBD_TM_ALL);

        return -EINVAL;
    }

    atomic_xchg((atomic_t *)&tracemask, localTraceMask);

    return 0;
}

static int PwbdGetTraceMask(char *Buffer, const struct kernel_param *KernelParam)
{
    int count = scnprintf(Buffer, PAGE_SIZE, "0x%08X\n", tracemask);

    return count;
}

//=================================================================================================
