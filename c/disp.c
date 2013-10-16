/* disp.c : dispatcher
 */

#include <xeroskernel.h>
#include <xeroslib.h>
#include <stdarg.h>
#include <utility.h>
#include <sleep.h>


extern void end_of_intr(void);

// head and tail of the ready_q
static pcb      *head = NULL;
static pcb      *tail = NULL;
// keep track currently running process, blocked processes and dead processes 
pcb 	*running=NULL;
pcb	*blocked_q=NULL;
pcb	*dead_q=NULL;
// keep reference to idleproc to put it in running state when required
extern unsigned int idleproc_pid;

/**
 * If a process dies, this method finds processes that were waiting for 
 * the newly dead target.  Appropriate error message supplied and process
 * formerly blocked process is put back on ready q
 * 
 * pid is the pid of the newly dead process that might have other
 * processes waiting on it
 * */
void notify_waiters(unsigned int pid){
	TRACE
	pcb* q = blocked_q;
	pcb* prev = blocked_q;
	pcb* dest;
	
	unsigned int destpid;
	while (q){
		
		if (q->ret==SYS_SEND){
			va_list ap = (va_list)q->args;
			destpid = va_arg(ap, unsigned int);
			
		}else if (q->ret==SYS_RECV){
			va_list ap = (va_list)q->args;
			unsigned int* from_pid = (unsigned int*)va_arg(ap, unsigned int);
			destpid = *from_pid;
		}
		
		if (destpid == pid){
			DBGMSG("found pid %d waiting on our dying pid %d\n",q->pid, pid);
			dest = &proctab[pid%MAX_PROC];
			prev->next = q->next;
			q->ret=BAD_PID;
			ready(q);
		}
		prev=q;
		q=q->next;
	}
}

// convience function mostly for debugging output
// given a state, return the head of the q that holds pcb with that state
pcb** get_q(int state){
	if (state==STATE_READY){
		return &head;
	}else if (state==STATE_RUNNING){
		return &running;
	}else if ((state==STATE_DEAD)||(state==STATE_STOPPED)){
		return &dead_q;
	}else if (state==STATE_BLOCKED){
		return &blocked_q;
	}
	return NULL;
}
// for debugging
// show the current q contents
void print_q(pcb* q){
	#ifdef DEBUG
/********************************/
	pcb* p;
	p=q;
	kprintf("\n%dq - ",q);
	while(p){
		kprintf("%d:%d, ",p->pid, p->state);
		p=p->next;
	}
	kprintf("\n");
	#endif
}

//given head of q, search q for pid and delete its pcb from q
// return the pid number on success, -1 on failure to find
int remove(pcb** q, unsigned int pid){
	pcb* queue=*q;
	pcb** prev;
	prev = q;
	if (!queue)
		return -1;  // q is empty 
	while (queue){
		if (queue->pid == pid){
			//found our target
			*prev=queue->next;
			return (int) pid;
		}
		*prev=queue;
		queue=queue->next;
	}
	return -1; //not found
}



// given a q of pcb, append pcb to the tail
// return the number of pcb in the q after the append operation
// precondition is that all links from other q to this have already been removed
int append(pcb** q, pcb* p){
/********************************/
	int count =0;
	
	//print_q(*q);
	
	if (p->next!=NULL){
		kprintf("WARNING,pcb %d next was not null: resetting to null", p->pid);
		p->next=NULL;
	}
	if (*q==NULL){
		count++;
		*q=p;
	}else{
		//get tail of q and append to it
		pcb* prev = *q;
		pcb* next = *q;
		while(next){
			if (next==p){
				kprintf("WARNING: trying to add to queue an existing member: ignoring\n");
				return count;
			}
			prev=next;
			next=next->next;
			count++;
		}
		//append our new value
		prev->next=p;
		count++;
	}
	//#ifdef DEBUG
	//print_q(*q);
	//#endif
	return count;
}

//pop the head of the given queue and return the popped pcb
pcb* deque(pcb** q){
/********************************/

	pcb* result;
	result = *q;  //return head of the q
	if(*q) {
		*q=(*q)->next;
	}else{
		return NULL;
	}
	result->next=NULL;
	//#ifdef DEBUG
		//print_q(*q);
		//kprintf("deque ret pid %d", result->pid);
	//#endif
	return result;	
}

//dispatcher policy engine for deciding who gets to run next
void     dispatch( void ) {
/********************************/

    pcb         *p;
    int         r;
    funcptr     fp;
    int         stack;
    va_list     ap;
    // messaging parameters
    unsigned int	dest_pid; 
    unsigned int	*from_pid = NULL;
    void*		dbuffer;
    int			dbuffer_len;
    void*		sbuffer;
    int			sbuffer_len;
    int 		count;
    

    for( p = next(); p; ) {
		
		p->state=STATE_RUNNING;
		running=p;

      r = contextswitch( p );
      switch( r ) {
      case( SYS_CREATE ):
        ap = (va_list)p->args;
        fp = (funcptr)(va_arg( ap, int ) );
        stack = va_arg( ap, int );
        // create adds process to tail of readyq
        p->ret = create( fp, stack );
        break;
      
      case( SYS_YIELD ):
        ready( p );
        p = next();
        p->state=STATE_RUNNING;
        running=p;
        break;
      
      case( SYS_STOP ):
        p->state = STATE_STOPPED;
        p->next=NULL;
        
        count = append(&dead_q, p);
        DBGMSG("SYS_STOP on pid %d, total dead %d\n",p->pid, count);
        notify_waiters(p->pid);
        
        p = next();
        p->state=STATE_RUNNING;
        running=p;
        break;
      
      case (SYS_GETPID):
        p->ret = p->pid;
        break;
      
      case (SYS_PUTS):
        kprintf("%s\n",*((char**)(p->args)));
        break;
      
      case (SYS_SEND):
		ap = (va_list)p->args;
		dest_pid = va_arg(ap, unsigned int);
		sbuffer = (void*)va_arg(ap,int);
		sbuffer_len = va_arg(ap, int);
		pcb* dest = &proctab[dest_pid%MAX_PROC];
		
		int sent = 0;
		sent = send(dest_pid, sbuffer, sbuffer_len, p);
		
		if (sent>0){
			// sent msg (or part of), 
			p->ret = sent;
			dest->ret = sent;
			ready(p);
			//we have pulled dest from proctable directly, we need to 
			//remove it from q (probably blocked_q)
			DBGMSG("Send found dest in state %d \n",dest->state);
			pcb** aQueueRef = get_q(dest->state);
			if(remove(aQueueRef, dest->pid)){
				DBGMSG("removed %d, state %d, from %x\n", dest->pid, dest->state, *aQueueRef);
			}else{
				DBGMSG("Warning, failed to find pid %d in q for state %d.  Placing on readyq anyway\n",
					dest->pid, dest->state);
			}
			ready(dest);
		} else if (sent <0) {
			// error condition in p->ret, put p back into ready_q to process error
			p->ret = sent;
			ready(p);
		} else {
			// receiver is not ready to receive yet, block sender, recvr will copy msg
			// allow other processes to run
			p->state=STATE_BLOCKED;
			p->ret=SYS_SEND;
			append(&blocked_q,p);
		}
		p = next();
		p->state = STATE_RUNNING;
		running=p;
		break;
		
	  case (SYS_RECV):
		ap = (va_list)p->args;
		from_pid = (unsigned int*)va_arg(ap, unsigned int);
		dbuffer = (void*)va_arg(ap,int);
		dbuffer_len = va_arg(ap, int);
		
		
		int recvd = 0;
		recvd = recv(from_pid, dbuffer, dbuffer_len, p);
		pcb* sndr = &(proctab[(*from_pid)%MAX_PROC]);
		if (recvd>0){
			// received msg (or part of)
			p->ret = recvd;
			sndr->ret = recvd;
			ready(p);
		// from_pid might be zero - universal recv from any
			if (sndr->pid!=0) {
				//we pulled this directly from proc table. clean up links from q
				DBGMSG("Recv found src in state %d \n",sndr->state);
				pcb** aQueueRef = get_q(sndr->state);
				if(remove(aQueueRef, sndr->pid)){
					DBGMSG("removed %d, state %d, from %x\n", sndr->pid, sndr->state, *aQueueRef);
				}else{
					DBGMSG("Warning, failed to find pid %d in q for state %d.  Placing on readyq anyway\n",
						sndr->pid, sndr->state);
						
				}
				
				ready(sndr);
			}			
		}else if (recvd<0){
			//error condition in p->ret, place back into ready_q to process error
			p->ret=recvd;
			ready(p);
		} else {
			// sender is not ready to send yet, block receiver, sender will copy msg
			// allow other processes to run
			p->state=STATE_BLOCKED;
			p->ret=SYS_RECV;
			append(&blocked_q,p);
		}
		p=next();
		p->state = STATE_RUNNING;
		running=p;
		break;
      
      case SYS_TIMER:
		ready( p );
        p = next();
        p->state=STATE_RUNNING;
        running=p;
        tick();
        end_of_intr();
		break;
	
	  case SYS_SLEEP:
		ap = (va_list)p->args;
		unsigned int ms = va_arg(ap, unsigned int);
		DBGMSG("pid %d going to sleep for %d ms\n", p->pid, ms);
		kassert(p->state==STATE_RUNNING);
		kassert(p->next==NULL);
		sleep(ms,p);
		p = next();
        p->state=STATE_RUNNING;
        running=p;
		break;
      
      default:
        kprintf( "Bad Sys request %d, pid = %d\n", r, p->pid );
        break;
      }
    }

    kprintf( "Out of processes: dying\n" );
    
    for( ;; );
}


// initilize process table to all zeros
extern void dispatchinit( void ) {
/********************************/

  memset(proctab, 0, sizeof( pcb ) * MAX_PROC);
}



// add the process to the tail of the read_q
// precondition: any ties to other queues are severed before entry to this q
// will not put idleproc on the readyq
extern void     ready( pcb *p ) {
/*******************************/
	
	unsigned int pid = p->pid;
	kassert(pid == proctab[pid%MAX_PROC].pid);
    p->next = NULL;
    p->state = STATE_READY;
    // we should not be using zeroth element in the array
	kassert(p != proctab);  
	kassert(pid == ((proctab[pid%MAX_PROC]).pid));
	// if its idle proc dont put it in readyq 
	if (p->pid == idleproc_pid) return;
    if( tail ) {
        tail->next = p;
    } else {
        head = p;
    }
    tail = p;
    
}

//extern unsigned int root_pid;

// remove the head of the ready_q and return it
// this is where the decision of when to put idleproc into running state is made
// if no more process available to run, return idleproc
// if idleproc gets put onto readyq, remove it
// if idleproc is running and there is an available process in ready state,
//		stop running idleproc and return new available process
extern pcb  *next( void ) {
/*******************************/
    pcb *p;
	p = head;

    if( p ) {
		unsigned int pid = p->pid;
		kassert(pid == proctab[pid%MAX_PROC].pid);
		// we should be drawing only from READY Q
		kassert(p->state==STATE_READY);
        head = p->next;
        p->next=NULL;
        if( !head ) {
            tail = NULL;
        }
	}

	//no more processes in ready q and nothing is running
//	if ((!p)&&((running==NULL)||(running->pid==idleproc_pid))){
	if (!p){
		p=&proctab[idleproc_pid%MAX_PROC];
		return p;
	}else if ((p->pid == idleproc_pid)&&(head)) {
		// idleproc in readyq - remove idleproc if anything else is ready
		DBGMSG("removing idleproc (%d) from readyq\n", idleproc_pid);
		pcb* idlepcb = p;
		p=head;
		head=head->next;
		if( !head ) {
            tail = NULL;
        }
		p->next=NULL;
		idlepcb->next=NULL;
	} else if ((running->pid==idleproc_pid)&&((p)&&(p->pid!=idleproc_pid))){
		// we've already retrieved a valid process into p in first part of block 
		DBGMSG("removing idleproc (%d) from running, other process available\n", idleproc_pid);
		running->state=STATE_READY;		
	}
	
	return( p );
}
