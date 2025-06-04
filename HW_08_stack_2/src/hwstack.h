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

#define HWST_CMD_PUSH "push "
#define HWST_CMD_PUSH_LENGTH (sizeof(HWST_CMD_PUSH) - sizeof(ASCII_NULL))

#define HWST_CMD_POP "pop"
#define HWST_CMD_TOP "top"
#define HWST_CMD_SIZE "size"
#define HWST_CMD_CLEAR "clear"
#define HWST_CMD_EXIT "exit"

///////////////////////////////////////////////////////////////////////////////

struct HWST_STACK_ENTRY {
    struct list_head Links;
    char Data[]; // flexible array member (FAM)
};

#define HWST_MAX_STACK_ENTRIES 100000

///////////////////////////////////////////////////////////////////////////////

static int HwstPushEntryStack(const void *Data, __u32 DataLength);
static struct HWST_STACK_ENTRY *HwstPopEntryStack(void);
static struct HWST_STACK_ENTRY *HwstPeekTopEntryStack(void);
static bool HwstIsStackEmpty(void);
static __u32 HwstGetStackSize(void);
static void HwstPurgeEntriesStack(void);

static int HwstSetParam(const char *val, const struct kernel_param *kp);

///////////////////////////////////////////////////////////////////////////////
