#ifndef _ATOMIC_MSVC_H
#define _ATOMIC_MSVC_H

#include <windows.h>
#include <stddef.h>

typedef intptr_t  atomic_t;

/*!
 * The Microsoft C/C++ compiler has acquire semantics for reads
 * and release semantics for writes that operate on volatiles.
 * See this for further explanation: 
 * http://msdn.microsoft.com/en-us/library/ms686355(VS.85).aspx
 */

__inline atomic_t sysAtomicRead(volatile atomic_t* ptr) {
  atomic_t value = *ptr;
  return(value);
}

__inline void sysAtomicSet(volatile atomic_t* ptr , atomic_t val) {
  *ptr = val;
}

/*!
 * @brief Atomically compares and exchanges two values. The function will
 * compare the value stored in ptr with oldval and if they are equal
 * it will store newval into ptr.
 * @return The value stored in ptr.
 */
__inline atomic_t sysAtomicCompareAndExchange(volatile atomic_t* ptr,
                                              atomic_t oldval,
                                              atomic_t newval) {
  return (atomic_t) InterlockedCompareExchange((LONG volatile*) ptr,
                                                 (LONG) newval,
                                                 (LONG) oldval);
}

/*!
 * @brief Atomically swap two values. This function will store newval into ptr.
 * @return Old value of ptr.
 */
__inline atomic_t sysAtomicSwap(volatile atomic_t* ptr, atomic_t newval) {
  return (atomic_t) InterlockedExchange((LONG volatile*) ptr, (LONG) newval);
}

/*!
 * @brief Atomically increment a value. The function will increment the value
 * stored in ptr with newval. Newval can be a negative value.
 * @return The new value stored in ptr.
 */
__inline atomic_t sysAtomicInc(volatile atomic_t* ptr,
                                       atomic_t newval) {
  return (atomic_t) InterlockedExchangeAdd((LONG volatile*) ptr, 
                                             (LONG) newval);
}

#endif