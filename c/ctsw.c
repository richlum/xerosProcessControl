/* ctsw.c : context switcher
 */

#include <xeroskernel.h>
#include <i386.h>

/* Your code goes here - You will need to write some assembly code. You must
   use the gnu conventions for specifying the instructions. (i.e this is the
   format used in class and on the slides.) You are not allowed to change the
   compiler/assembler options or issue directives to permit usage of Intel's
   assembly language conventions.
*/

//different entrypoints into context switcher for system call and timer interupts
//respectively.  Both run the commonentrypoint
void _KernelEntryPoint(void);
void _TimerEntryPoint(void);
void _CommonEntryPoint(void);

static unsigned int        saveESP;
static unsigned int        rc;
static long                args;
static unsigned int		   interrupt;

// p is the process to switch to 
// 
int contextswitch( pcb *p ) {
/**********************************/
 
    /* keep every thing on the stack to simplfiy gcc's gesticulations
     */

    saveESP = p->esp;
    rc = p->ret; 
    interrupt=0;
    /* In the assembly code, switching to process
     * 1.  Push eflags and general registers on the stack
     * 2.  Load process's return value into eax (rc)
     * 3.  load processes ESP into edx, and save kernel's ESP in saveESP
     * 4.  Set up the process stack pointer
     * 5.  store the return value on the stack where the processes general
     *     registers, including eax has been stored.  We place the return
     *     value right in eax so when the stack is popped, eax will contain
     *     the return value (offset 28ESP)
     * 6.  pop general registers from the stack
     * 7.  Do an iret to switch to process
     *
     * TimerEntryPoint
     * 1. disable interupts
     * 2. use static var interrupt to indicate thes is a timer interrupt
     * 3. save eax value into rc that exists when interrupt is entered
     * 4. after assembly section, write eax value from rc into pcb->ret
     * 		we overload rc with saved eax for timer intr instead of return
     * 		value
     * 5. note that common lead up to iret will restore eax value we had 
     * 		on entry when we iret.
     *  
     * Switching to kernel
     * 0.  turn off interrupts
     * 1.  Push regs on stack, set static variable interrupt to 
     * 		1 if timer interrupt, jump to common point.
     * 2.  Interrupt indicator set to false since this is syscall
     * 3.  Store request code in static var request
     * 		syscall is passing us request in eax;
     * 4.  Store arguments in static var args
     * 		syscall is passing us ptr to arguments in edx
     * 
     * Common to interupt and syscall handling
     * 4.  exchange the process esp and kernel esp using saveESP and eax
     *     saveESP will contain the process's esp
     * 5.  Pop kernel's general registers and eflags
     * 
     * outside of assembly 
     * 6.  store the stack pointer in the pcb of the process
     * 7a. if interrupt store the saved eax into pcb->ret 
     * 7b. otherwise store syscall args into pcb 
     * 8.  return to system servicing code
     */
 
    __asm __volatile( " \
        pushf \n\
        pusha \n\
        movl    rc, %%eax    \n\
        movl    saveESP, %%edx    \n\
        movl    %%esp, saveESP    \n\
        movl    %%edx, %%esp \n\
        movl    %%eax, 28(%%esp) \n\
        popa 	\n\
        iret 	\n\
   _TimerEntryPoint: \n\
		cli    	\n\
		pusha  	\n\
		movl 	$1, interrupt \n\
		movl	%%eax, rc \n\
		jmp _CommonEntryPoint \n\
   _KernelEntryPoint: \n\
		cli    	\n\
        pusha  	\n\
        movl 	$0, interrupt \n\
        movl    %%eax, rc \n\
        movl	%%edx, args \n\
   _CommonEntryPoint :\n\
		movl    saveESP, %%eax  \n\
        movl    %%esp, saveESP  \n\
        movl    %%eax, %%esp  \n\
        movl    %%ebx, 28(%%esp) \n\
        movl    %%edx, 20(%%esp) \n\
        popa \n\
        popf \n\
        "
        : 
        : 
        : "%eax", "%ebx", "%edx", "%ecx"
    );

    /* save esp and read in the arguments
     */
    p->esp = saveESP;
    if (interrupt){
		//we saved eax to rc on interrupt entry, restore here 
		//  since intr does not update pcb->ret, it will automatically
		//  restore the eax entry value saved here on return when
		//  top of contextswitch is entered, ret-> rc and 
		//  pushed into eax after popa, overwriting any saved eax value
		// 	with the value we saved on interupt entry
		p->ret = rc;
		rc = SYS_TIMER;
	}else{
		//syscall passes request directly in eax. we load that into
		// rc in assembly above within kernelentrypoint.
		// no need to save into pcb because we will do work and 
		// update pcb->ret for return to process anyway
		
		//syscall passes args to us in edx, we copy to args in 
		// kernelentrypoint above. save args into pcb.
		p->args = args;
	}

    return rc;
}


// intialize interrupt vectors that we will use
// for syscall and timer (pre-emptive multitask)  respectively
void contextinit( void ) {
/*******************************/

  set_evec( KERNEL_INT, (int) _KernelEntryPoint );
  set_evec( TIMER_INT,  (int) _TimerEntryPoint  );
  initPIT(TIMEDIVISOR);

}
