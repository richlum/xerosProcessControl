/* create.c : create a process
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <utility.h>
// from limits.h
#define INT_MAX	2147483647

extern pcb		*blocked_q;
extern pcb		*dead_q;
extern int remove(pcb** q, unsigned int pid);
pcb     proctab[MAX_PROC];

/* make sure interrupts are armed later on in the kernel development  */
#define STARTING_EFLAGS         0x00003000
// for enabling interrupts
#define INTR_ENA_EFLAGS			0x00003200
//static int      nextpid = 1;


// fp is the function to create a process for with 2* stack size memory allocation-
// make context frame and put call to sysstop as return address
// put process on readyq
// return the pid of the process created

int      create( funcptr fp, int stack ) {
/***********************************************/

    context_frame       *cf;
    pcb                 *p = NULL;
    int                 i;

    if( stack < PROC_STACK ) {
        stack = PROC_STACK;
    }

    for( i = 1; i < MAX_PROC; i++ ) {
        if( (proctab[i].state == STATE_STOPPED )||
			(proctab[i].state == STATE_DEAD)  )  {
            p = &proctab[i];
            break;
        }
    }
    
    //remove this from any queues that it might be sitting in
    remove(&dead_q, i);
    
    //set the pid to be the old pid incr by proctable size
    // pid mod max_proc yields index into proctable
    if ((proctab[i].pid)%MAX_PROC != i ){
		//pid is not index aligned, initialize it to first index aligned value
		proctab[i].pid = i;
	}else{
		if (proctab[i].pid+MAX_PROC >= INT_MAX){
			// about to exceed max integer, reset to lowest
			proctab[i].pid=i;
		}else{
			proctab[i].pid = proctab[i].pid + MAX_PROC;
		}
	}
	DBGMSG("pid entry = %d for index value %d\n", proctab[i].pid, i);
	
    if( !p ) {
        return( -1 );
    }

	// allocating double the space is requested - leave as is from
	// original supplied source for saftey factor given that memory
	// allocation is not the focus of this exercise.
    cf = kmalloc( stack*2);
    // cf pointing to wrong end of memory (low addr)
    if( !cf ) {
        return( -1 );
    }
	// move cf ponter to other end of allocated (high addr) , give 4 address to pad against boundary
    cf = (context_frame *)((int)cf + stack - 4);
    //  now cf points the (theoritical) high end of stack (plus four)
    //  allocate contextframe on stack, recall stack grows toward negative addresses
    cf--;

    memset(cf, 0x81, sizeof( context_frame ));

    cf->iret_cs = getCS();
    cf->iret_eip = (unsigned int)fp;
    cf->eflags = STARTING_EFLAGS;
    cf->eflags = INTR_ENA_EFLAGS;
	// note  
	// contxt switcher recovers esp from proctab->esp value
	// uses that value to  restore esp before popa
	// also note that this value in cf->esp isnt actually used 
	// popa does NOT restore esp from 
	// context frame.  It ignores contents of cf->esp
	// and sets esp based on popping all registers of
	// the stack.
	// ref intel 64 sw dev manual vol2b for popa instruction
	//  "The value on the stack for the ESP or SP register is ignored."

    cf->esp = (int)(cf + 1);
	// it is the correct value for ebp that will be restored by popa though
    cf->ebp = cf->esp;
    p->esp = (int)cf;
    p->state = STATE_READY;

	// since we allocated four addreses below context frame we can shove a copuple of things
	// below the context frame. Normally, the old EBP and Return address is found underneath
	// the functions local variables.  We emulate that here by writing the address
	// of systop as the return address on stack.  
	// End of user program should pop top of the top of stack
	// return address into EIP
	//  
	//  note that the function itself normally pushes ebp as the first instruction and
	//  pops the ebp just before calling return (which pops the return address into eip)
	//  since the function is responsible for pushing and popping ebp we dont need to 
	//  so that here, just the return address for the eip is required. 
	
	cf->stackSlots[0] = (int) (&sysstop);
    ready( p );
    return( p->pid );
}

