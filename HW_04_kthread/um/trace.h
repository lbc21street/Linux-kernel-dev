//=================================================================================================
//
// \file    trace.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define UM_EMERG "<0>"   /* system is unusable */
#define UM_ALERT "<1>"   /* action must be taken immediately */
#define UM_CRIT "<2>"    /* critical conditions */
#define UM_ERR "<3>"     /* error conditions */
#define UM_WARNING "<4>" /* warning conditions */
#define UM_NOTICE "<5>"  /* normal but significant condition */
#define UM_INFO "<6>"    /* informational */
#define UM_DEBUG "<7>"   /* debug-level messages */

//
//
//

int PthOpenKmsg(void);

void PthCloseKmsg(void);

void PthWriteKmsg(const char *Format, ...);

//
//
//

#define pr_emerg(fmt, ...) PthWriteKmsg(UM_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#define pr_alert(fmt, ...) PthWriteKmsg(UM_ALERT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_crit(fmt, ...) PthWriteKmsg(UM_CRIT pr_fmt(fmt), ##__VA_ARGS__)
#define pr_err(fmt, ...) PthWriteKmsg(UM_ERR pr_fmt(fmt), ##__VA_ARGS__)
#define pr_warn(fmt, ...) PthWriteKmsg(UM_WARNING pr_fmt(fmt), ##__VA_ARGS__)
#define pr_notice(fmt, ...) PthWriteKmsg(UM_NOTICE pr_fmt(fmt), ##__VA_ARGS__)
#define pr_info(fmt, ...) PthWriteKmsg(UM_INFO pr_fmt(fmt), ##__VA_ARGS__)

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
