/* sleep.c : sleep device (assignment 2)
 */

#include <xeroskernel.h>
#include <sleep.h>
#include <utility.h>

/* Your code goes here */
#include <sleep.h>
 
//list of sleepers sorted by shortest to longest naps
pcb* headsleeper = NULL;

//debug function to verify 
void print_sleepq(void){
/********************************/
	pcb* p;
	p=headsleeper;
	kprintf("sleepq - ");
	while(p){
		kprintf("%d:%d, ",p->pid, p->sleepticks);
		kassert (p->state==STATE_SLEEP);
		p=p->next;
	}
	kprintf("\n");
}

//convert supplied sleep time into interrupt time ticks
unsigned int milliseconds_to_ticks(unsigned int milliseconds){
	return TIMEDIVISOR*milliseconds/1000;
}

/**
 * Place process in sleep queue in order of min sleep time to max.
 * Only head of q contains absolute sleep time, all others are relative
 * to their predecessor.  Accumulator is used to calculate absolute time
 * as required.
 *
 * milliseconds is the time to sleep
 * p is the process to put to sleep
 * */
void sleep (unsigned int milliseconds, pcb* p){
	//print_sleepq();
	p->next = NULL;
    p->state = STATE_SLEEP;
    p->sleepticks = milliseconds_to_ticks(milliseconds);
    // we should not be using zeroth element in the array
	kassert(p != proctab);  
	pcb* prev = headsleeper;
	pcb* next = headsleeper;
	unsigned int accumulator=0;
	//only head is absolute # ticks left to sleep
	//all others are relative to prev sleeper
	if ((next)&&(next->sleepticks < p->sleepticks)){
		accumulator = next->sleepticks;
		while((next)&&(accumulator < p->sleepticks)){
			prev=next;
			next=next->next;
			accumulator += next->sleepticks;
		}
		accumulator -= next->sleepticks;
		p->next=next;
		prev->next=p;
		
		//only save the diff between the curr and earlier (prev) sleeper
		p->sleepticks=p->sleepticks-accumulator;
		//adjust the one after us to be relative to us instead of prev
		p->next->sleepticks = p->next->sleepticks - p->sleepticks;
		kassert(p->sleepticks >=0);
	}else{
		p->next=next;
		if (next){
			next->sleepticks-=p->sleepticks;
		}
		headsleeper=p;
	}
	
	//print_sleepq();
}

// every tick, scan sleepers and awaken those that need it
// update the headsleeper ticks left to sleep
void tick(void){
	if (!headsleeper) return;
	//print_sleepq();
	pcb* p;
	if (headsleeper->sleepticks <= 1){
		headsleeper->sleepticks=0;
		while ((headsleeper)&&(headsleeper->sleepticks ==0)){
			p=headsleeper;
			headsleeper=headsleeper->next;
			p->next=NULL;
			DBGMSG("Waking up pid %d\n",p->pid);
			//print_sleepq();
			ready(p);
			//print_sleepq();
		}
	}
	if ((headsleeper)&&(headsleeper->sleepticks>0))
		headsleeper->sleepticks--;
	//print_sleepq();
}

// sleeping does not pause output, this is for control of output for capture
// note there is a pause function defined in intr.S that may cause name collision
// so renamed to avoid
void mypause(int cycles){
	int i;
	int j;
	for (i=0;i<cycles;i++){
		for(j=0;j<1000000;j++){
		}
	}
}
