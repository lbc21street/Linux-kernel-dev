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

#include <linux/device.h>
#include <linux/device/class.h>
#include <linux/kstrtox.h>
#include <linux/stat.h>
#include <linux/sysfs.h>

#include "data.h"
#include "supportmacros.h"

#include "devicesupport.h"
#include "sysfssupport.h"

//
//
//

static ssize_t PwbdpClassAttributeAddShow(struct class *Class, struct class_attribute *Attribute,
                                          char *Buffer);

static ssize_t PwbdpClassAttributeAddStore(struct class *Class, struct class_attribute *Attribute,
                                           const char *Buffer, size_t Count);

static ssize_t PwbdpDeviceAttributeRemoveShow(struct device *Device,
                                              struct device_attribute *Attribute, char *Buffer);

static ssize_t PwbdpDeviceAttributeRemoveStore(struct device *Device,
                                               struct device_attribute *Attribute,
                                               const char *Buffer, size_t Count);

//
//
//

const struct class_attribute PwbdClassAttributeAdd = {
    .attr = {.name = PWBD_CLASS_ATTRIBUTE_ADD_NAME, .mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH},
    .show = PwbdpClassAttributeAddShow,
    .store = PwbdpClassAttributeAddStore,
};

//
//
//

const struct device_attribute PwbdDeviceAttributeRemove = {
    .attr = {.name = PWBD_DEVICE_ATTRIBUTE_REMOVE_NAME,
             .mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH},
    .show = PwbdpDeviceAttributeRemoveShow,
    .store = PwbdpDeviceAttributeRemoveStore,
};

//
//
//

[[nodiscard]] int PwbdCreateClass(void)
{
    int result = 0;

    pr_info("creating class <%s>", PWBD_DEVICE_NAME);

    PwbdCtrl.Class = class_create(THIS_MODULE, PWBD_DEVICE_NAME); // 6.8 has only one param

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

//
//
//

[[nodiscard]] int PwbdCreateClassDevice(PPWBD_DEVICE Device)
{
    int result = 0;

    pr_info_detailed("creating class device - device 0x%px (%u)", Device, Device->DeviceNumber);

    struct device *classDevice = device_create(PwbdCtrl.Class, NULL, 0, Device, "%s%u",
                                               PWBD_DEVICE_NAME, Device->DeviceNumber);

    if (!IS_ERR(classDevice)) {
        pr_info("created class device 0x%px device 0x%px (%u)", classDevice, Device,
                Device->DeviceNumber);

        Device->ClassDevice = classDevice;
    }

    else {
        result = PTR_ERR(classDevice);

        pr_err("device_create() failed %d device 0x%px (%u)", result, Device, Device->DeviceNumber);
    }

    return result;
}

//
//
//

void PwbdDestroyClassDevice(PPWBD_DEVICE Device)
{
    if (Device->ClassDevice == NULL) {
        return;
    }

    pr_info("unregistering class device 0x%px device 0x%px (%u)", Device->ClassDevice, Device,
            Device->DeviceNumber);

    // device_destroy(PwbdCtrl.Class, 0);

    device_unregister(Device->ClassDevice);
    Device->ClassDevice = NULL;
}

//
//
//

[[nodiscard]] static int PwbdpCreateClassAttribute(const struct class_attribute *Attribute)
{
    int result = class_create_file(PwbdCtrl.Class, Attribute);

    if (result == 0) {
        pr_info("created class attribute <%s>", Attribute->attr.name);
    }

    else {
        pr_err("class_create_file() failed %d <%s>", result, Attribute->attr.name);
    }

    return result;
}

//
//
//

static void PwbdpRemoveClassAttribute(const struct class_attribute *Attribute)
{
    if (PwbdCtrl.Class == NULL) {
        return;
    }

    pr_info("removing class attribute <%s>", Attribute->attr.name);

    class_remove_file(PwbdCtrl.Class, Attribute);
}

//
//
//

[[nodiscard]] int PwbdCreateClassAttributeAdd(void)
{
    return PwbdpCreateClassAttribute(&PwbdClassAttributeAdd);
}

//
//
//

void PwbdRemoveClassAttributeAdd(void)
{
    PwbdpRemoveClassAttribute(&PwbdClassAttributeAdd);
}

//
//
//

static ssize_t PwbdpClassAttributeAddShow(struct class *Class, struct class_attribute *Attribute,
                                          char *Buffer)
{
    int count = scnprintf(Buffer, PAGE_SIZE, "%u\n", 0);

    return count;
}

//
//
//

static ssize_t PwbdpClassAttributeAddStore(struct class *Class, struct class_attribute *Attribute,
                                           const char *Buffer, size_t Count)
{
    uint32_t value = 0;

    int result = kstrtouint(Buffer, 10, &value);

    if (result != 0) {
        pr_err("kstrtouint() failed %d", result);

        return result;
    }

    if (value == 1) {
        result = PwbdAddDevice();

        if (result != 0) {
            return result;
        }
    }

    return Count;
}

//
//
//

[[nodiscard]] static int PwbdpCreateDeviceAttribute(PPWBD_DEVICE Device,
                                                    const struct device_attribute *Attribute)
{
    int result = device_create_file(Device->ClassDevice, Attribute);

    if (result == 0) {
        pr_info("created device attribute <%s> device 0x%px (%u)", Attribute->attr.name, Device,
                Device->DeviceNumber);
    }

    else {
        pr_err("device_create_file() failed %d <%s> device 0x%px (%u)", result,
               Attribute->attr.name, Device, Device->DeviceNumber);
    }

    return result;
}

//
//
//

static void PwbdpRemoveDeviceAttribute(PPWBD_DEVICE Device,
                                       const struct device_attribute *Attribute)
{
    if (Device->ClassDevice == NULL) {
        return;
    }

    pr_info("removing device attribute <%s> device 0x%px (%u)", Attribute->attr.name, Device,
            Device->DeviceNumber);

    device_remove_file(Device->ClassDevice, Attribute);
}

//
//
//

[[nodiscard]] int PwbdCreateDeviceAttributeRemove(PPWBD_DEVICE Device)
{
    return PwbdpCreateDeviceAttribute(Device, &PwbdDeviceAttributeRemove);
}

//
//
//

void PwbdRemoveDeviceAttributeRemove(PPWBD_DEVICE Device)
{
    PwbdpRemoveDeviceAttribute(Device, &PwbdDeviceAttributeRemove);
}

//
//
//

static ssize_t PwbdpDeviceAttributeRemoveShow(struct device *Device,
                                              struct device_attribute *Attribute, char *Buffer)
{
    int count = scnprintf(Buffer, PAGE_SIZE, "%u\n", 0);

    return count;
}

//
//
//

static ssize_t PwbdpDeviceAttributeRemoveStore(struct device *Device,
                                               struct device_attribute *Attribute,
                                               const char *Buffer, size_t Count)
{
    uint32_t value = 0;

    int result = kstrtouint(Buffer, 10, &value);

    if (result != 0) {
        pr_err("kstrtouint() failed %d", result);

        return result;
    }

    PPWBD_DEVICE device = (PPWBD_DEVICE)Device->driver_data;

    //
    // [NOTE]
    //
    // quick check
    //
    // we cannot acquire the device lock here since the teardown code may already call
    // device_remove_file() or device_destroy() while holding that lock
    //

    if (FlagOn(PwbdCtrl.Flags, PWBD_CTLFL_TEARING_DOWN)) {
        pr_warn("driver is already being torn down");

        return -ENODEV;
    }

    if (value == 1) {
        PwbdRemoveDeviceDeferred(device);
    }

    return Count;
}

//=================================================================================================
