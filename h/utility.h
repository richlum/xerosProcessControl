#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <xeroskernel.h>

// parts culled from assert.h and melded with xeroskernel functions
// to achieve assert like functionality to help catch bugs 

#define __kASSERT_VOID_CAST (void)
#define STR(x) #x
#define __kassert_fail(expr) \
	kprintf ("assertion failed on %s at %s:%d (%s)\n", \
		   STR(expr), __FILE__, __LINE__, __func__) 

//halt is a assembly routine in intr.S, its an assmbly variant of sleep
extern void halt(void);

// this will be a key macro for testing
#define kassert(expr)							\
  ((expr)								\
   ? __kASSERT_VOID_CAST (0)						\
   : ({ __kassert_fail (expr) ; halt(); }))



// utility routine to be able to leave trails of what functions are being called
#ifdef DEBUG
#define TRACE kprintf("Entered:%s:  AT %s:%d\n",__FUNCTION__, __FILE__, __LINE__);
#define DBGMSG( x, ...) kprintf(x, __VA_ARGS__);
#else
#define TRACE
#define DBGMSG(x, ...) 
#endif

#endif


