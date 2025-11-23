//=================================================================================================
//
// \file    ex_rb_tree.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/types.h>
#include <linux/rbtree.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define EXRBT_CMD_ADD "add "
#define EXRBT_CMD_ADD_LENGTH (sizeof(EXRBT_CMD_ADD) - sizeof(ASCII_NULL))
#define EXRBT_CMD_REMOVE "remove"
#define EXRBT_CMD_REMOVE_LAST "removelast"
#define EXRBT_CMD_REMOVE_BY "removeby "
#define EXRBT_CMD_REMOVE_BY_LENGTH (sizeof(EXRBT_CMD_REMOVE_BY) - sizeof(ASCII_NULL))
#define EXRBT_CMD_PEEK "peek"
#define EXRBT_CMD_PEEK_LAST "peeklast"
#define EXRBT_CMD_FIND "find "
#define EXRBT_CMD_FIND_LENGTH (sizeof(EXRBT_CMD_FIND) - sizeof(ASCII_NULL))
#define EXRBT_CMD_ENUM "enum"
#define EXRBT_CMD_ENUM_LAST "enumlast"
#define EXRBT_CMD_ENUM_RAW "enumraw"
#define EXRBT_CMD_HASH "hash"
#define EXRBT_CMD_HASH_LAST "hashlast"
#define EXRBT_CMD_SIZE "size"
#define EXRBT_CMD_CLEAR "clear"
#define EXRBT_CMD_EXIT "exit"

//
//
//

typedef enum _EXRBT_CTRL_FLAGS {

    EXRBT_CTLFL_TREE_CREATED = 0x00000001,

} EXRBT_CTRL_FLAGS;

//
//
//

typedef struct _EXRBT_CTRL_FLAGS_BF {

    uint32_t FlTreeCreated : 1;
    uint32_t FlReserved : 31;

} EXRBT_CTRL_FLAGS_BF;

static_assert(sizeof(EXRBT_CTRL_FLAGS_BF) == sizeof(EXRBT_CTRL_FLAGS));

//
//
//

typedef struct _EXRBT_CTRL {

    struct rb_root TreeRoot;

    union {
        EXRBT_CTRL_FLAGS Flags;
        EXRBT_CTRL_FLAGS_BF FlagsBf;
    };

    bool Active;

} EXRBT_CTRL, *PEXRBT_CTRL;

//
//
//

typedef struct _EXRBT_DATA_ENTRY {

    struct rb_node Node;
    uint32_t Value;

} EXRBT_DATA_ENTRY, *PEXRBT_DATA_ENTRY;

#define EXRBT_MAX_DATA_ENTRIES 10 * 1024

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
