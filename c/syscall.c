/* syscall.c : syscalls
 */

#include <xeroskernel.h>
#include <stdarg.h>
#include <utility.h>

//user side originated system calls
// req = request type (sysgetpid, yield, sysputs etc..
//  	other parameters depend on type of request
int syscall( int req, ... ) {
/**********************************/

    va_list     ap;
    int         rc;

    va_start( ap, req );

	// req = request (SYS_STOP, SYS_SEND etc..
	// ap = arguments to request on stack in order, depends on req type
	//	eg SYS_SEND, ap = dest_pid, ap+1 = *buffer, ap+2 = buffer_len 
	//		- assuming all args are same size (ptr length = sizeof (int))
	//	kernel returns result in eax which is loaded into rc.
    __asm __volatile( " \
        movl %1, %%eax \n\
        movl %2, %%edx \n\
        int  %3 \n\
        movl %%eax, %0 \n\
        "
        : "=g" (rc)
        : "g" (req), "g" (ap), "i" (KERNEL_INT)
        : "%eax" 
    );
 
    va_end( ap );

    return( rc );
}

 int syscreate( funcptr fp, int stack ) {
/*********************************************/

    return( syscall( SYS_CREATE, fp, stack ) );
}

 int sysyield( void ) {
/***************************/

    return( syscall( SYS_YIELD ) );
}

 int sysstop( void ) {
/**************************/
    return( syscall( SYS_STOP ) );
}
// get the pid of the current process
unsigned int sysgetpid(void){
    return (syscall (SYS_GETPID) );
}
// send a null terminated string to kernel to output
void sysputs(char* str){
    syscall (SYS_PUTS, str);
}

// send a message to another pid.
// BLOCKING call
//	returns number of bytes acceptd by destination or 
//	returns -1 if dest process does not exist
//	returns -2 if process trying to send to self
//	returns -3 for other send error. 
extern int syssend(unsigned int dest_pid, void *buffer, int buffer_len){
	
	int result = (syscall (SYS_SEND,dest_pid, buffer, buffer_len));
	
	return result;
}


// receive a message from another pid
// BLOCKING call
//	if from_pid = 0, receive from any pid, will return pid 
//	returns number of bytes received or
//	returns -1 if pid to receive from is invalid
//	returns -2 if recv pid  = send pid
//	returns -3 if other recv error (eg neg buff size, invalid buff addr, etc
//	

extern int sysrecv(unsigned int *from_pid, void *buffer, int buffer_len){
	return (syscall (SYS_RECV, from_pid, buffer, buffer_len));
}

extern unsigned int syssleep(unsigned int milliseconds){
	return (syscall (SYS_SLEEP, milliseconds));
	
}



