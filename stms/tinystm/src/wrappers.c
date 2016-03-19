/*
 * File:
 *   wrappers.c
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 *   Patrick Marlier <patrick.marlier@unine.ch>
 * Description:
 *   STM wrapper functions for different data types.
 *
 * Copyright (c) 2007-2014.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This program has a dual license and can also be distributed
 * under the terms of the MIT license.
 */

#include <assert.h>

#include "utils.h"
#include "stm_internal.h"
#include "wrappers.h"

# define TM_LOAD(addr)                int_stm_load(tx, addr)
# define TM_STORE(addr, val)          int_stm_store(tx, addr, val)
# define TM_STORE2(addr, val, mask)   int_stm_store2(tx, addr, val, mask)

typedef union convert_64 {
  uint64_t u64;
  uint32_t u32[2];
  uint16_t u16[4];
  uint8_t u8[8];
  int64_t s64;
  double d;
} convert_64_t;

static void sanity_checks(void)
{
  COMPILE_TIME_ASSERT(sizeof(convert_64_t) == 8);
  COMPILE_TIME_ASSERT(sizeof(stm_word_t) == 8);
  COMPILE_TIME_ASSERT(sizeof(char) == 1);
  COMPILE_TIME_ASSERT(sizeof(short) == 2);
  COMPILE_TIME_ASSERT(sizeof(int) == 4);
  COMPILE_TIME_ASSERT(sizeof(long) == 8);
  COMPILE_TIME_ASSERT(sizeof(float) == 4);
  COMPILE_TIME_ASSERT(sizeof(double) == 8);
}

extern INLINE void int_stm_store_u8(volatile uint8_t *addr, uint8_t value);
extern INLINE void int_stm_store_u16(volatile uint16_t *addr, uint16_t value);
extern INLINE void int_stm_store_u32(volatile uint32_t *addr, uint32_t value);
extern INLINE void int_stm_store_u64(volatile uint64_t *addr, uint64_t value);
extern INLINE uint8_t int_stm_load_u8(volatile uint8_t *addr);
extern INLINE uint16_t int_stm_load_u16(volatile uint16_t *addr);
extern INLINE uint32_t int_stm_load_u32(volatile uint32_t *addr);
extern INLINE uint64_t int_stm_load_u64(volatile uint64_t *addr);


/* ################################################################### *
 * INLINE LOADS
 * ################################################################### */

INLINE
uint8_t int_stm_load_u8(volatile uint8_t *addr)
{
  TX_GET;
  convert_64_t val;
  val.u64 = (uint64_t)TM_LOAD((volatile stm_word_t *)((uintptr_t)addr & ~(uintptr_t)0x07));
  return val.u8[(uintptr_t)addr & 0x07];
}

INLINE
uint16_t int_stm_load_u16(volatile uint16_t *addr)
{
  TX_GET;
  convert_64_t val;
  val.u64 = (uint64_t)TM_LOAD((volatile stm_word_t *)((uintptr_t)addr & ~(uintptr_t)0x07));
  return val.u16[((uintptr_t)addr & 0x07) >> 1];
}

INLINE
uint32_t int_stm_load_u32(volatile uint32_t *addr)
{
  TX_GET;
  convert_64_t val;
  val.u64 = (uint64_t)TM_LOAD((volatile stm_word_t *)((uintptr_t)addr & ~(uintptr_t)0x07));
  return val.u32[((uintptr_t)addr & 0x07) >> 2];
}

INLINE
uint64_t int_stm_load_u64(volatile uint64_t *addr)
{
  TX_GET;
  return (uint64_t)TM_LOAD((volatile stm_word_t *)addr);
}

/* ################################################################### *
 * INLINE STORES
 * ################################################################### */

INLINE
void int_stm_store_u8(volatile uint8_t *addr, uint8_t value)
{
  TX_GET;
  convert_64_t val, mask;
  val.u8[(uintptr_t)addr & 0x07] = value;
  mask.u64 = 0;
  mask.u8[(uintptr_t)addr & 0x07] = ~(uint8_t)0;
  TM_STORE2((volatile stm_word_t *)((uintptr_t)addr & ~(uintptr_t)0x07), (stm_word_t)val.u64, (stm_word_t)mask.u64);
}

INLINE
void int_stm_store_u16(volatile uint16_t *addr, uint16_t value)
{
  TX_GET;
  convert_64_t val, mask;
  val.u16[((uintptr_t)addr & 0x07) >> 1] = value;
  mask.u64 = 0;
  mask.u16[((uintptr_t)addr & 0x07) >> 1] = ~(uint16_t)0;
  TM_STORE2((volatile stm_word_t *)((uintptr_t)addr & ~(uintptr_t)0x07), (stm_word_t)val.u64, (stm_word_t)mask.u64);
}

INLINE
void int_stm_store_u32(volatile uint32_t *addr, uint32_t value)
{
  TX_GET;
  convert_64_t val, mask;
  val.u32[((uintptr_t)addr & 0x07) >> 2] = value;
  mask.u64 = 0;
  mask.u32[((uintptr_t)addr & 0x07) >> 2] = ~(uint32_t)0;
  TM_STORE2((volatile stm_word_t *)((uintptr_t)addr & ~(uintptr_t)0x07), (stm_word_t)val.u64, (stm_word_t)mask.u64);
}

INLINE
void int_stm_store_u64(volatile uint64_t *addr, uint64_t value)
{
  TX_GET;
  return TM_STORE((volatile stm_word_t *)addr, (stm_word_t)value);
}

/* ################################################################### *
 * STORES
 * ################################################################### */

_CALLCONV void stm_store_u8(volatile uint8_t *addr, uint8_t value)
{
  int_stm_store_u8(addr, value);
}

_CALLCONV void stm_store_u16(volatile uint16_t *addr, uint16_t value)
{
  int_stm_store_u16(addr, value);
}

_CALLCONV void stm_store_u32(volatile uint32_t *addr, uint32_t value)
{
  int_stm_store_u32(addr, value);
}

_CALLCONV void stm_store_u64(volatile uint64_t *addr, uint64_t value)
{
  int_stm_store_u64(addr, value);
}

/* ################################################################### *
 * LOADS
 * ################################################################### */

_CALLCONV uint8_t stm_load_u8(volatile uint8_t *addr)
{
  return int_stm_load_u8(addr);
}

_CALLCONV uint16_t stm_load_u16(volatile uint16_t *addr)
{
  return int_stm_load_u16(addr);
}

_CALLCONV uint32_t stm_load_u32(volatile uint32_t *addr)
{
  return int_stm_load_u32(addr);
}

_CALLCONV uint64_t stm_load_u64(volatile uint64_t *addr)
{
  return int_stm_load_u64(addr);
}

#undef TM_LOAD
#undef TM_STORE
#undef TM_STORE2

