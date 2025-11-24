//=================================================================================================
//
// \file    supportmacros.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define ___STRINGIZE(str) #str
#define __STRINGIZE(str) ___STRINGIZE(str)

//
//
//

#define ASCII_NULL ((char)'\0')
#define ASCII_LF ((char)'\n')

#ifndef TRUE
#define TRUE true
#endif // TRUE

#ifndef FALSE
#define FALSE false
#endif // FALSE

//
//
//

#ifndef FlagOn
#define FlagOn(f, m) ((f) & (m))
#endif // FlagOn

#ifndef BooleanFlagOn
#define BooleanFlagOn(f, m) ((__u8)(((f) & (m)) != 0))
#endif // BooleanFlagOn

#ifndef SetFlag
#define SetFlag(f, m) ((f) |= (m))
#endif // SetFlag

#ifndef ClearFlag
#define ClearFlag(f, m) ((f) &= ~(m))
#endif // ClearFlag

#ifndef FlagOnXact
#define FlagOnXact(f, m, r) (FlagOn((f), (m)) == (r))
#endif // FlagOnXact

#ifndef FlagOnAll
#define FlagOnAll(f, m) (FlagOnXact((f), (m), (m)))
#endif // FlagOnAll

//
//
//

#ifndef Add2Ptr
#define Add2Ptr(ptr, inc) ((void *)((unsigned char *)(ptr) + (inc)))
#endif // Add2Ptr

#ifndef PtrOffset
#define PtrOffset(base, off) ((__u32)((uintptr_t)(off) - (ULONG_PTR)(base)))
#endif // PtrOffset

//
//
//

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ap) ((char *)((uintptr_t)(ap)) != (char *)(NULL))
#endif // ARGUMENT_PRESENT

//
//
//

#define MB_TO_BYTES(x) ((x) * 1024 * 1024)

//
//
//

#define UINT_PTR(val) ((void *)(uintptr_t)(val))
#define PTR_UINT(ptr) ((uint32_t)(uintptr_t)(ptr))

//
//
//

#ifndef __same_type
#define __same_type(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#endif // __same_type

#ifndef BUILD_BUG_ON_ZERO
#define BUILD_BUG_ON_ZERO(e) ((int)(sizeof(struct { int : (-!!(e)); })))
#endif // BUILD_BUG_ON_ZERO

#ifndef __must_be_array
#define __must_be_array(a) BUILD_BUG_ON_ZERO(__same_type((a), &(a)[0]))
#endif // __must_be_array

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
#endif // ARRAY_SIZE

//
//
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomment"

// #undef ALIGN_DOWN_BY
// #undef ALIGN_UP_BY
// #undef ALIGN_DOWN_POINTER_BY
// #undef ALIGN_UP_POINTER_BY
// #undef ALIGN_DOWN
// #undef ALIGN_UP
// #undef ALIGN_DOWN_POINTER
// #undef ALIGN_UP_POINTER
// #undef IS_ALIGNED

// #define ALIGN_DOWN_BY(length, alignment) \
//     ((ULONG_PTR)(length) & ~((ULONG_PTR)(alignment) - 1))

// #define ALIGN_UP_BY(length, alignment) \
//     (ALIGN_DOWN_BY(((ULONG_PTR)(length) + (alignment) - 1), alignment))

// #define ALIGN_DOWN_POINTER_BY(address, alignment) \
//     ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)(alignment) - 1)))

// #define ALIGN_UP_POINTER_BY(address, alignment) \
//     (ALIGN_DOWN_POINTER_BY(((ULONG_PTR)(address) + (alignment) - 1), alignment))

// #define ALIGN_DOWN(length, type) \
//     ALIGN_DOWN_BY(length, sizeof(type))

// #define ALIGN_UP(length, type) \
//     ALIGN_UP_BY(length, sizeof(type))

// #define ALIGN_DOWN_POINTER(address, type) \
//     ALIGN_DOWN_POINTER_BY(address, sizeof(type))

// #define ALIGN_UP_POINTER(address, type) \
//     ALIGN_UP_POINTER_BY(address, sizeof(type))

// #define IS_ALIGNED(value, alignment) \
//     (((ULONG_PTR)(value) & ((alignment) - 1)) == 0)

// #define IS_TYPE_ALIGNED(value, type) \
//     IS_ALIGNED(value, sizeof(type))

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
