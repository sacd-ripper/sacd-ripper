/*! \file atomic.h
 \brief Atomic Operations.
*/ 

/*
 * PowerPC atomic operations
 *
 * Copied from the linux 2.6.x kernel sources:
 *  - renamed functions to psl1ght convention
 *  - removed all kernel dependencies
 *  - removed PPC_ACQUIRE_BARRIER, PPC_RELEASE_BARRIER macros
 *  - removed PPC405_ERR77 macro
 *
 */

#ifndef __SYS_ATOMIC_H__
#define __SYS_ATOMIC_H__

#include <ppu-types.h>

typedef struct { volatile u32 counter; } atomic_t; 
typedef struct { u64 counter; } atomic64_t;

static inline u32 sysAtomicRead(const atomic_t *v)
{
        u32 t;

        __asm__ volatile("lwz%U1%X1 %0,%1" : "=r"(t) : "m"(v->counter));

        return t;
}

static inline void sysAtomicSet(atomic_t *v, int i)
{
        __asm__ volatile("stw%U0%X0 %1,%0" : "=m"(v->counter) : "r"(i));
}

static inline void sysAtomicAdd(u32 a, atomic_t *v)
{
        u32 t;

        __asm__ volatile(
"1:     lwarx   %0,0,%3         # atomic_add\n\
        add     %0,%2,%0\n"
"       stwcx.  %0,0,%3 \n\
        bne-    1b"
        : "=&r" (t), "+m" (v->counter)
        : "r" (a), "r" (&v->counter)
        : "cc");
}

static inline u32 sysAtomicAddReturn(u32 a, atomic_t *v)
{
        u32 t;

        __asm__ volatile(
"1:     lwarx   %0,0,%2         # atomic_add_return\n\
        add     %0,%1,%0\n"
"       stwcx.  %0,0,%2 \n\
        bne-    1b"
        : "=&r" (t)
        : "r" (a), "r" (&v->counter)
        : "cc", "memory");

        return t;
}

#define sysAtomicAddNegative(a, v)       (sysAtomicAddReturn((a), (v)) < 0)

static inline void sysAtomicSub(u32 a, atomic_t *v)
{
        u32 t;

        __asm__ volatile(
"1:     lwarx   %0,0,%3         # atomic_sub\n\
        subf    %0,%2,%0\n"
"       stwcx.  %0,0,%3 \n\
        bne-    1b"
        : "=&r" (t), "+m" (v->counter)
        : "r" (a), "r" (&v->counter)
        : "cc");
}

static inline u32 sysAtomicSubReturn(u32 a, atomic_t *v)
{
        u32 t;

        __asm__ volatile(
"1:     lwarx   %0,0,%2         # atomic_sub_return\n\
        subf    %0,%1,%0\n"
"       stwcx.  %0,0,%2 \n\
        bne-    1b"
        : "=&r" (t)
        : "r" (a), "r" (&v->counter)
        : "cc", "memory");

        return t;
}

static inline void sysAtomicInc(atomic_t *v)
{
        u32 t;

        __asm__ volatile(
"1:     lwarx   %0,0,%2         # atomic_inc\n\
        addic   %0,%0,1\n"
"       stwcx.  %0,0,%2 \n\
        bne-    1b"
        : "=&r" (t), "+m" (v->counter)
        : "r" (&v->counter)
        : "cc", "xer");
}

static inline u32 sysAtomicIncReturn(atomic_t *v)
{
        u32 t;

        __asm__ volatile(
"1:     lwarx   %0,0,%1         # atomic_inc_return\n\
        addic   %0,%0,1\n"
"       stwcx.  %0,0,%1 \n\
        bne-    1b"
        : "=&r" (t)
        : "r" (&v->counter)
        : "cc", "xer", "memory");

        return t;
}

/*
 * atomic_inc_and_test - increment and test
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
#define sysAtomicIncAndTest(v) (sysAtomicIncReturn(v) == 0)

static inline void sysAtomicDec(atomic_t *v)
{
        u32 t;

        __asm__ volatile(
"1:     lwarx   %0,0,%2         # atomic_dec\n\
        addic   %0,%0,-1\n"
"       stwcx.  %0,0,%2\n\
        bne-    1b"
        : "=&r" (t), "+m" (v->counter)
        : "r" (&v->counter)
        : "cc", "xer");
}

static inline u32 sysAtomicDecReturn(atomic_t *v)
{
        u32 t;

        __asm__ volatile(
"1:     lwarx   %0,0,%1         # atomic_dec_return\n\
        addic   %0,%0,-1\n"
"       stwcx.  %0,0,%1\n\
        bne-    1b"
        : "=&r" (t)
        : "r" (&v->counter)
        : "cc", "xer", "memory");

        return t;
}

/*
 * Atomic exchange
 *
 * Changes the memory location '*ptr' to be val and returns
 * the previous value stored there.
 */
static inline u32 __xchg_u32(volatile void *p, u32 val)
{
    u32 prev;

    __asm__ volatile(
"1: lwarx   %0,0,%2 \n"
"   stwcx.  %3,0,%2 \n\
    bne-    1b"
    : "=&r" (prev), "+m" (*(volatile unsigned int *)p)
    : "r" (p), "r" (val)
    : "cc", "memory");

    return prev;
}

static inline u32 __xchg_u64(volatile void *p, u32 val)
{
    u32 prev;

    __asm__ volatile(
"1: ldarx   %0,0,%2 \n"
"   stdcx.  %3,0,%2 \n\
    bne-    1b"
    : "=&r" (prev), "+m" (*(volatile u32 *)p)
    : "r" (p), "r" (val)
    : "cc", "memory");

    return prev;
}

/*
 * This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid xchg().
 */
extern void __xchg_called_with_bad_pointer(void);

static inline u32 __xchg(volatile void *ptr, u32 x, unsigned int size)
{
    switch (size) {
    case 4:
        return __xchg_u32(ptr, x);
    case 8:
        return __xchg_u64(ptr, x);
    }
    __xchg_called_with_bad_pointer();
    return x;
}

#define xchg(ptr,x)                              \
  ({                                             \
     __typeof__(*(ptr)) _x_ = (x);               \
     (__typeof__(*(ptr))) __xchg((ptr), (u32)_x_, sizeof(*(ptr))); \
  })

/*
 * Compare and exchange - if *p == old, set it to new,
 * and return the old value of *p.
 */
static inline u64
__cmpxchg_u32(volatile unsigned int *p, u64 old, u64 new)
{
    unsigned int prev;

    __asm__ volatile (
"1: lwarx   %0,0,%2     # __cmpxchg_u32\n\
    cmpw    0,%0,%3\n\
    bne-    2f\n"
"   stwcx.  %4,0,%2\n\
    bne-    1b"
    "\n\
2:"
    : "=&r" (prev), "+m" (*p)
    : "r" (p), "r" (old), "r" (new)
    : "cc", "memory");

    return prev;
}

static inline u64
__cmpxchg_u64(volatile u64 *p, u64 old, u64 new)
{
    u64 prev;

    __asm__ volatile (
"1: ldarx   %0,0,%2     # __cmpxchg_u64\n\
    cmpd    0,%0,%3\n\
    bne-    2f\n\
    stdcx.  %4,0,%2\n\
    bne-    1b"
    "\n\
2:"
    : "=&r" (prev), "+m" (*p)
    : "r" (p), "r" (old), "r" (new)
    : "cc", "memory");

    return prev;
}

/* This function doesn't exist, so you'll get a linker error
   if something tries to do an invalid cmpxchg().  */
extern void __cmpxchg_called_with_bad_pointer(void);

static inline u64
__cmpxchg(volatile void *ptr, u64 old, u64 new,
      unsigned int size)
{
    switch (size) {
    case 4:
        return __cmpxchg_u32(ptr, old, new);
    case 8:
        return __cmpxchg_u64(ptr, old, new);
    }
    __cmpxchg_called_with_bad_pointer();
    return old;
}

#define cmpxchg(ptr, o, n)                       \
  ({                                     \
     __typeof__(*(ptr)) _o_ = (o);                   \
     __typeof__(*(ptr)) _n_ = (n);                   \
     (__typeof__(*(ptr))) __cmpxchg((ptr), (u64)_o_,        \
                    (u64)_n_, sizeof(*(ptr))); \
  })

#define sysAtomicCompareAndSwap(v, o, n) (cmpxchg(&((v)->counter), (o), (n)))
#define sysAtomicSwap(v, new) (xchg(&((v)->counter), new))

/**
 * atomic_add_unless - add unless the number is a given value
 * @v: pointer of type atomic_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as it was not @u.
 * Returns non-zero if @v was not @u, and zero otherwise.
 */
static inline u32 sysAtomicAddUnless(atomic_t *v, u32 a, int u)
{
        u32 t;

        __asm__ volatile (
"1:     lwarx   %0,0,%1         # atomic_add_unless\n\
        cmpw    0,%0,%3 \n\
        beq-    2f \n\
        add     %0,%2,%0 \n"
"       stwcx.  %0,0,%1 \n\
        bne-    1b \n"
"       subf    %0,%2,%0 \n\
2:"
        : "=&r" (t)
        : "r" (&v->counter), "r" (a), "r" (u)
        : "cc", "memory");

        return t != u;
}

#define sysAtomicIncNotZero(v) sysAtomicAddUnless((v), 1, 0)

#define sysAtomicSubAndTest(a, v)       (sysAtomicSubReturn((a), (v)) == 0)
#define sysAtomicDecAndTest(v)          (sysAtomicDecReturn((v)) == 0)

/*
 * Atomically test *v and decrement if it is greater than 0.
 * The function returns the old value of *v minus 1, even if
 * the atomic variable, v, was not decremented.
 */
static inline u32 sysAtomicDecIfPositive(atomic_t *v)
{
        u32 t;

        __asm__ volatile(
"1:     lwarx   %0,0,%1         # atomic_dec_if_positive\n\
        cmpwi   %0,1\n\
        addi    %0,%0,-1\n\
        blt-    2f\n"
"       stwcx.  %0,0,%1\n\
        bne-    1b"
        "\n\
2:"     : "=&b" (t)
        : "r" (&v->counter)
        : "cc", "memory");

        return t;
}

static inline u64 sysAtomic64Read(const atomic64_t *v)
{
        u64 t;

        __asm__ volatile("ld%U1%X1 %0,%1" : "=r"(t) : "m"(v->counter));

        return t;
}

static inline void sysAtomic64Set(atomic64_t *v, u64 i)
{
        __asm__ volatile("std%U0%X0 %1,%0" : "=m"(v->counter) : "r"(i));
}

static inline void sysAtomic64Add(u64 a, atomic64_t *v)
{
        u64 t;

        __asm__ volatile(
"1:     ldarx   %0,0,%3         # atomic64_add\n\
        add     %0,%2,%0\n\
        stdcx.  %0,0,%3 \n\
        bne-    1b"
        : "=&r" (t), "+m" (v->counter)
        : "r" (a), "r" (&v->counter)
        : "cc");
}

static inline u64 sysAtomic64AddReturn(u64 a, atomic64_t *v)
{
        u64 t;

        __asm__ volatile(
"1:     ldarx   %0,0,%2         # atomic64_add_return\n\
        add     %0,%1,%0\n\
        stdcx.  %0,0,%2 \n\
        bne-    1b"
        : "=&r" (t)
        : "r" (a), "r" (&v->counter)
        : "cc", "memory");

        return t;
}

#define sysAtomic64AddNegative(a, v)     (sysAtomic64AddReturn((a), (v)) < 0)

static inline void sysAtomic64Sub(u64 a, atomic64_t *v)
{
        u64 t;

        __asm__ volatile(
"1:     ldarx   %0,0,%3         # atomic64_sub\n\
        subf    %0,%2,%0\n\
        stdcx.  %0,0,%3 \n\
        bne-    1b"
        : "=&r" (t), "+m" (v->counter)
        : "r" (a), "r" (&v->counter)
        : "cc");
}

static inline u64 sysAtomic64SubReturn(u64 a, atomic64_t *v)
{
        u64 t;

        __asm__ volatile(
"1:     ldarx   %0,0,%2         # atomic64_sub_return\n\
        subf    %0,%1,%0\n\
        stdcx.  %0,0,%2 \n\
        bne-    1b"
        : "=&r" (t)
        : "r" (a), "r" (&v->counter)
        : "cc", "memory");

        return t;
}

static inline void sysAtomic64Inc(atomic64_t *v)
{
        u64 t;

        __asm__ volatile(
"1:     ldarx   %0,0,%2         # atomic64_inc\n\
        addic   %0,%0,1\n\
        stdcx.  %0,0,%2 \n\
        bne-    1b"
        : "=&r" (t), "+m" (v->counter)
        : "r" (&v->counter)
        : "cc", "xer");
}

static inline u64 sysAtomic64IncReturn(atomic64_t *v)
{
        u64 t;

        __asm__ volatile(
"1:     ldarx   %0,0,%1         # atomic64_inc_return\n\
        addic   %0,%0,1\n\
        stdcx.  %0,0,%1 \n\
        bne-    1b"
        : "=&r" (t)
        : "r" (&v->counter)
        : "cc", "xer", "memory");

        return t;
}

/*
 * atomic64_inc_and_test - increment and test
 * @v: pointer of type atomic64_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
#define sysAtomic64IncAndTest(v) (sysAtomic64IncReturn(v) == 0)

static inline void sysAtomic64Dec(atomic64_t *v)
{
        u64 t;

        __asm__ volatile(
"1:     ldarx   %0,0,%2         # atomic64_dec\n\
        addic   %0,%0,-1\n\
        stdcx.  %0,0,%2\n\
        bne-    1b"
        : "=&r" (t), "+m" (v->counter)
        : "r" (&v->counter)
        : "cc", "xer");
}

static inline u64 sysAtomic64DecReturn(atomic64_t *v)
{
        u64 t;

        __asm__ volatile(
"1:     ldarx   %0,0,%1         # atomic64_dec_return\n\
        addic   %0,%0,-1\n\
        stdcx.  %0,0,%1\n\
        bne-    1b"
        : "=&r" (t)
        : "r" (&v->counter)
        : "cc", "xer", "memory");

        return t;
}

#define sysAtomic64SubAndTest(a, v)     (sysAtomic64SubReturn((a), (v)) == 0)
#define sysAtomic64DecAndTest(v)        (sysAtomic64DecReturn((v)) == 0)

/*
 * Atomically test *v and decrement if it is greater than 0.
 * The function returns the old value of *v minus 1.
 */
static inline u64 sysAtomic64DecIfPositive(atomic64_t *v)
{
        u64 t;

        __asm__ volatile(
"1:     ldarx   %0,0,%1         # atomic64_dec_if_positive\n\
        addic.  %0,%0,-1\n\
        blt-    2f\n\
        stdcx.  %0,0,%1\n\
        bne-    1b"
        "\n\
2:"     : "=&r" (t)
        : "r" (&v->counter)
        : "cc", "xer", "memory");

        return t;
}

#define sysAtomic64CompareAndSwap(v, o, n) (cmpxchg(&((v)->counter), (o), (n)))
#define sysAtomic64Swap(v, new) (xchg(&((v)->counter), new))

/**
 * atomic64_add_unless - add unless the number is a given value
 * @v: pointer of type atomic64_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so u64 as it was not @u.
 * Returns non-zero if @v was not @u, and zero otherwise.
 */
static inline u32 sysAtomic64AddUnless(atomic64_t *v, u64 a, u64 u)
{
        u64 t;

        __asm__ volatile (
"1:     ldarx   %0,0,%1         # atomic_add_unless\n\
        cmpd    0,%0,%3 \n\
        beq-    2f \n\
        add     %0,%2,%0 \n"
"       stdcx.  %0,0,%1 \n\
        bne-    1b \n"
"       subf    %0,%2,%0 \n\
2:"
        : "=&r" (t)
        : "r" (&v->counter), "r" (a), "r" (u)
        : "cc", "memory");

        return t != u;
}

#define sysAtomic64IncNotZero(v) sysAtomic64AddUnless((v), 1, 0)

#endif /* __SYS_ATOMIC_H__ */
