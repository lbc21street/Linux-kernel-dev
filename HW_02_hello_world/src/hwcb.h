#pragma once

///////////////////////////////////////////////////////////////////////////////

#define ASCII_NULL ((char)'\0')
#define ASCII_LF ((char)'\n')

#ifndef TRUE
#define TRUE 1
#endif // TRUE

#ifndef FALSE
#define FALSE 0
#endif // FALSE

///////////////////////////////////////////////////////////////////////////////

#define HWCB_TEMPLATE_STRING "Hello, World!"

#define HWCB_MAX_SYMBOLS (__u32)(sizeof(HWCB_TEMPLATE_STRING) - sizeof(ASCII_NULL))

///////////////////////////////////////////////////////////////////////////////

static int HwcbGetParam(char *buffer, const struct kernel_param *kp);
static int HwcbSetParam(const char *val, const struct kernel_param *kp);

///////////////////////////////////////////////////////////////////////////////
