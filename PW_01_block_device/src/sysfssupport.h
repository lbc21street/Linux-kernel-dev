//=================================================================================================
//
// \file    sysfssupport.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include "devicesupport.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define PWBD_CLASS_ATTRIBUTE_ADD_NAME "add"
#define PWBD_DEVICE_ATTRIBUTE_REMOVE_NAME "remove"

//
//
//

[[nodiscard]] int PwbdCreateClass(void);

void PwbdDestroyClass(void);

[[nodiscard]] int PwbdCreateClassDevice(PPWBD_DEVICE Device);

void PwbdDestroyClassDevice(PPWBD_DEVICE Device);

[[nodiscard]] int PwbdCreateClassAttributeAdd(void);

void PwbdRemoveClassAttributeAdd(void);

[[nodiscard]] int PwbdCreateDeviceAttributeRemove(PPWBD_DEVICE Device);

void PwbdRemoveDeviceAttributeRemove(PPWBD_DEVICE Device);

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
