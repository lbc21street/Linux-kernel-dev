//=================================================================================================
//
// \file    tracesupport.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include "supportmacros.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// trace levels
//

#define PWBD_TL_1 1
#define PWBD_TL_2 2
#define PWBD_TL_3 3
#define PWBD_TL_4 4
#define PWBD_TL_5 5
#define PWBD_TL_MAX PWBD_TL_5

//
//
//

#define PWBD_TM_INIT 0x00000001
#define PWBD_TM_DEVICE_SUPPORT 0x00000002
#define PWBD_TM_IO_SUPPORT 0x00000004
#define PWBD_TM_PARAM_SUPPORT 0x00000008
#define PWBD_TM_QUEUE_SUPPORT 0x00000010
#define PWBD_TM_SYSFS_SUPPORT 0x00000020
#define PWBD_TM_WORKQUEUE_SUPPORT 0x00000040

#define PWBD_TM_ALL                                                                                \
    (PWBD_TM_WORKQUEUE_SUPPORT | PWBD_TM_SYSFS_SUPPORT | PWBD_TM_QUEUE_SUPPORT |                   \
     PWBD_TM_PARAM_SUPPORT | PWBD_TM_IO_SUPPORT | PWBD_TM_DEVICE_SUPPORT | PWBD_TM_INIT)

//
//
//

extern uint32_t tracelevel;
extern uint32_t tracemask;

//
//
//

#ifndef PWBD_COMPONENT_TRACE_MASK
#define PWBD_COMPONENT_TRACE_MASK 0
#endif // PWBD_COMPONENT_TRACE_MASK

//
//
//

#define pr_xxx_tl(type, tl, ...)                                                                   \
    ({                                                                                             \
        if (FlagOn(tracemask, PWBD_COMPONENT_TRACE_MASK) && tracelevel && ((tl) <= tracelevel)) {  \
            pr_##type(__VA_ARGS__);                                                                \
        }                                                                                          \
    })

#define pr_emerg_tl(tl, ...) pr_xxx_tl(emerg, tl, __VA_ARGS__)
#define pr_alert_tl(tl, ...) pr_xxx_tl(alert, tl, __VA_ARGS__)
#define pr_crit_tl(tl, ...) pr_xxx_tl(crit, tl, __VA_ARGS__)
#define pr_err_tl(tl, ...) pr_xxx_tl(err, tl, __VA_ARGS__)
#define pr_warn_tl(tl, ...) pr_xxx_tl(warn, tl, __VA_ARGS__)
#define pr_notice_tl(tl, ...) pr_xxx_tl(notice, tl, __VA_ARGS__)
#define pr_info_tl(tl, ...) pr_xxx_tl(info, tl, __VA_ARGS__)

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
