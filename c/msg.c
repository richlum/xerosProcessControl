/* msg.c : messaging system (assignment 2)
 */

#include <xeroskernel.h>
#include <stdarg.h>
#include <i386.h>
#include <utility.h>

extern pcb* blocked_q;

/* Your code goes here */


// return min value of pair of ints
int min(int a, int b){
	return ( a<b ? a : b);
}

// copy buffers based on smallest specified buffer length
int copy(void* src, void* dst, int srclen, int dstlen){
	int i;
	char* source;
	source = (char*)src;
	char* destination;
	destination = (char*)dst;
	for (i=0;i<min(srclen,dstlen);i++){
		destination[i]=source[i];
		if (source[i]==NULL) break;  // stop copying when encounter a null
	}
	if (destination[i]!=NULL)
		destination[i]=0; //make sure there is a null termination
	return i;
}


//validate pid is alive (running, blocked, or ready, not stopped or dead
int valid(int pid){
	if (pid==0)
		return 1;  // allow 0 as a receive all pid
	if( ( proctab[pid%MAX_PROC].state==STATE_STOPPED) ||
		(proctab[pid%MAX_PROC].state==STATE_DEAD) ){
		return 0;
	}
	return 1;		
}


// check if we need to return an error code for invalid buffer parms
//	-1 = invalid pid
// 	-2 = src=dest
//	-3 = other error (illegal buffer size, illegal buffer address
//	1 is ok to send message

int validate_buffer(void* buffer, int buffer_len){
	if ((long)buffer < (long)freemem)
		return BAD_BUFFER_PARM;
	if ( (((char*)buffer >= (char*)HOLESTART)) &&
		 ((char*)buffer<=(char*)HOLEEND)   ) 
		return BAD_BUFFER_PARM;
	if ( (int)buffer > (int)maxaddr )
		return BAD_BUFFER_PARM;
	if (buffer_len <= 0)
		return BAD_BUFFER_PARM;
	return 0;
}

// check if we have a valid destination pid and buffer parms
int validate_send(pcb* p, int dest_pid, void* buffer, int buffer_len){ 
	int result ;
		if (!valid(dest_pid)){
			p->ret = BAD_PID;
			return BAD_PID;
		}
		if(p->pid == dest_pid){
			p->ret = SRC_DST_SAME;
			return SRC_DST_SAME;
		}
		result = validate_buffer(buffer,buffer_len);
		//0 if ok, -ve value indicates err type
		return result;
}

// check if we have a valid target
int validate_recv(pcb* p, unsigned int from_pid, void* buffer, int buffer_len){ 
	int result;
		if (!valid(from_pid)){
			p->ret = BAD_PID;
			return BAD_PID;
		}
		if(p->pid == from_pid){
			p->ret = SRC_DST_SAME;
			return SRC_DST_SAME;
		}
		result = validate_buffer(buffer,buffer_len);
		
		return result;
}

// find target, validate target pid, state and wether target is waiting 
// for us. If so initiate message transfer and unblock target,
// if not block or error as approriate
//
//	dest_pid is the pid we want to send to
//	sbuffer is the senders buffer
//	sbuffer_len is the length of senders buffer
//	p is the pcb of the sender.  We write to the ret for return code
//
int send(unsigned int dest_pid, void* sbuffer, int sbuffer_len, pcb* p){
		int rc;	
		if ((rc=validate_send(p,dest_pid,sbuffer,sbuffer_len))<0){
			return rc;
		}
		pcb* dest = &proctab[dest_pid%MAX_PROC];
		if (dest->state == STATE_BLOCKED){	
			va_list ap;
			ap = (va_list)(dest->args);
			// these are dest pids recv args
			unsigned int* dest_waiton_pid;
			dest_waiton_pid = (unsigned int*)va_arg(ap, unsigned int);
			void* dbuffer = (void*)va_arg(ap, int);
			int dbuffer_len = va_arg(ap,int);
			// either the recvr is waiting for our pid or any pid (0)
			if (*dest_waiton_pid==0){
				//copy our pid into recvr's dest pid arg
				*dest_waiton_pid = (p->pid);
			}else if (*dest_waiton_pid!=p->pid){
				// blocked process is waiting for us - check what his request is
				if (dest->ret == SYS_SEND){
					// we cant both be trying to send to eachother at the smae time
					return WOULD_DEADLOCK;
				}
				
				//we want to send to rx, but rx is waiting for someone else. we should block		
				DBGMSG("dest_pid %d, waiting for  src pid %d, but we are pid %d\n",
					dest_pid, *dest_waiton_pid, p->pid);
			}

			int sent_bytes = copy(sbuffer,dbuffer,sbuffer_len,dbuffer_len);
			DBGMSG("send copied %d bytes from pid %d to pid %d which has is expecting to recv from %d\n", 
			 sent_bytes, p->pid, dest_pid, *dest_waiton_pid);
			return sent_bytes;
		}//else recvr not ready for us yet
		return NULL;
}



// validate target pid, state and wether target is waiting for us
// if so, initiate transfer and unblock target process.
//
//	from_pid is the pid we want to receive from. 0 means any 
//		if 0, will be updated to senders pid when a message comes in
//	dbuffer is the receivers buffer
//	dbuffer_len is the length of the buffer
// 	p is the pcb of the recv 
//
//
int recv(unsigned int* from_pid, void* dbuffer, int dbuffer_len,pcb* p){
		int rc;
		if ((rc=validate_recv(p,*from_pid,dbuffer,dbuffer_len))<0){
			return rc;
		}
//		DBGMSG("recv, from_pid = %x\n",*from_pid);
		if (*from_pid == 0) {
			// willing to receive from any process, check blocked_q for any senders
			pcb* blocked=blocked_q;
			while (blocked){
				if (blocked->pid == p->pid){
					// this is us (our own pid), skip to next
					// we shouldnt find ourselves in blocked q but protect here to ensure 
					blocked = blocked->next;
					continue;
				}
				va_list ap;
				pcb* sender = blocked;
				ap = (va_list)(sender->args);
				unsigned int dest_pid = va_arg(ap, int);
				void* sbuffer = (void*)va_arg(ap, int);
				int sbuffer_len = va_arg(ap,int);
				// is the sender trying to send to us?
				if (dest_pid==p->pid){
					*from_pid = (sender->pid);
					int recvd_bytes = copy(sbuffer,dbuffer,sbuffer_len,dbuffer_len);
					return recvd_bytes;
				}
				blocked=blocked->next;	
			}	
		}else{
			pcb* sender = &proctab[(*from_pid)%MAX_PROC];
			if (sender->ret == SYS_RECV) {
				va_list ap;
				ap = (va_list)(sender->args);
				// these are dest pids recv args
				unsigned int* dest_waiton_pid;
				dest_waiton_pid = (unsigned int*)va_arg(ap, unsigned int);
				if (*dest_waiton_pid == p->pid){
					// dest is waiting for us to send
					return WOULD_DEADLOCK;
				}
			}
			if (sender->state == STATE_BLOCKED){
				va_list ap = (va_list)(sender->args);
				unsigned int destpid = va_arg(ap, unsigned int);
				void* sbuffer = (void*)va_arg(ap, int);
				int sbuffer_len = va_arg(ap,int);
				if (destpid!=p->pid){
					DBGMSG("sender pid %d, sending to pid %d, but we are pid %d\n",
						*from_pid, destpid, p->pid);	
						//sender may be sending to other processes, keep waiting
					return 0;
				}
				int recd_bytes = copy(sbuffer,dbuffer,sbuffer_len,dbuffer_len);
				DBGMSG("recv copied %d bytes from pid %d to pid %d\n", recd_bytes, *from_pid, p->pid);
				return recd_bytes;
			}
				
		}
		// pid is not zero and there is no one blocked waiting to send
		return 0;
		
}
