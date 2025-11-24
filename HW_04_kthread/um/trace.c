//=================================================================================================
//
// \file    trace.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" UBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <fcntl.h>

#include "pthread.h"
#include "trace.h"

//
//
//

int PthOpenKmsg(void)
{
    PthCtrl.KmsgFd = open("/dev/kmsg", O_WRONLY);

    int result = 0;

    if (PthCtrl.KmsgFd == -1) {
        result = errno;

        printf("[" UBUILD_MODNAME "] open(/dev/kmsg) failed %d\n", result);
    }

    return result;
}

//
//
//

void PthCloseKmsg(void)
{
    if (PthCtrl.KmsgFd != -1) {
        close(PthCtrl.KmsgFd);
        PthCtrl.KmsgFd = -1;
    }
}

//
//
//

void PthWriteKmsg(const char *Format, ...)
{
    if (PthCtrl.KmsgFd != -1) {
        va_list args;
        va_start(args, Format);

        char buffer[1024];

        int charsWritten = vsnprintf(buffer, sizeof(buffer), Format, args);

        va_end(args);

        if (charsWritten < sizeof(buffer)) {
            ssize_t written = write(PthCtrl.KmsgFd, buffer, charsWritten);
        }
    }
}

//=================================================================================================
