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

#define ASCII_NULL ((char)'\0')
#define ASCII_LF ((char)'\n')

#ifndef TRUE
#define TRUE 1
#endif // TRUE

#ifndef FALSE
#define FALSE 0
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

#define UINT_PTR(val) ((void *)(uintptr_t)(val))
#define PTR_UINT(ptr) ((uint32_t)(uintptr_t)(ptr))

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
