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

enum HWST_BRACKET_TYPE {
    HwstBracketTypeNone,
    HwstBracketTypeRound,
    HwstBracketTypeSquare,
    HwstBracketTypeCurly,
    HwstBracketTypeMax,
};

///////////////////////////////////////////////////////////////////////////////

struct HWST_STACK_ENTRY {
    enum HWST_BRACKET_TYPE BracketType;
    __u32 Position;
    struct list_head Links;
};

#define HWST_MAX_STACK_ENTRIES 10000

///////////////////////////////////////////////////////////////////////////////

static int HwstPushEntryStack(enum HWST_BRACKET_TYPE BracketType, __u32 Position);
static struct HWST_STACK_ENTRY *HwstPopEntryStack(void);
static bool HwstIsStackEmpty(void);
// static __u32 HwstGetStackSize(void);
static void HwstPurgeEntriesStack(void);

static int HwstSetParam(const char *val, const struct kernel_param *kp);

///////////////////////////////////////////////////////////////////////////////
