//=================================================================================================
//
// \file    ex_list.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define EXLST_CMD_ADD "add "
#define EXLST_CMD_ADD_LENGTH (sizeof(EXLST_CMD_ADD) - sizeof(ASCII_NULL))
#define EXLST_CMD_ADD_TAIL "addtail "
#define EXLST_CMD_ADD_TAIL_LENGTH (sizeof(EXLST_CMD_ADD_TAIL) - sizeof(ASCII_NULL))
#define EXLST_CMD_REMOVE "remove"
#define EXLST_CMD_REMOVE_TAIL "removetail"
#define EXLST_CMD_REMOVE_BY "removeby "
#define EXLST_CMD_REMOVE_BY_LENGTH (sizeof(EXLST_CMD_REMOVE_BY) - sizeof(ASCII_NULL))
#define EXLST_CMD_PEEK "peek"
#define EXLST_CMD_PEEK_TAIL "peektail"
#define EXLST_CMD_FIND "find "
#define EXLST_CMD_FIND_LENGTH (sizeof(EXLST_CMD_FIND) - sizeof(ASCII_NULL))
#define EXLST_CMD_ENUM "enum"
#define EXLST_CMD_ENUM_TAIL "enumtail"
#define EXLST_CMD_ROTATE "rotate"
#define EXLST_CMD_HASH "hash"
#define EXLST_CMD_HASH_TAIL "hashtail"
#define EXLST_CMD_SIZE "size"
#define EXLST_CMD_CLEAR "clear"
#define EXLST_CMD_EXIT "exit"

//
//
//

typedef struct _EXLST_CTRL {

    struct list_head DataEntries;
    uint32_t DataEntryCount;

    bool Active;

} EXLST_CTRL, *PEXLST_CTRL;

//
//
//

typedef struct _EXLST_DATA_ENTRY {

    struct list_head Links;
    uint32_t Value;

} EXLST_DATA_ENTRY, *PEXLST_DATA_ENTRY;

#define EXLST_MAX_DATA_ENTRIES 10000

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
