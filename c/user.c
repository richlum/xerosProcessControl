/* user.c : User processes
 */

#include <xeroskernel.h>
#include <utility.h>
#include <xeroslib.h>
#include <sleep.h>

/* Your code goes here */
#define pausetime 1

/*************************************************/
// top half of this file is the test cases that compiles
// if a2tests is specified make option.
//
//
// bottom half is the producer consumer solution
// that compiles with either debug or no option 
/*************************************************/

#ifdef TESTS
/****************Some Test Cases *********************************/
#define buffsize 64

extern unsigned int root_pid =0;
unsigned int consumer_pid = 0;
unsigned int producer_pid = 0;
unsigned int rcvr_pid=0;
unsigned int sender1pid=0;
unsigned int sender2pid=0;
unsigned int rcvr1pid =0;
unsigned int rcvr2pid =0;


 void producer( void ) {
/****************************/

    char buffer[buffsize];
	producer_pid = sysgetpid();
    DBGMSG("producer  pid = %d\n", producer_pid);

	// wait until consumer has at least started so we can use his pid
	while(!consumer_pid){

	}
	// send will copy from sender to producer pid 3 to consumer pid 2 (blocked waiting to receive)
	sprintf(buffer,"TEST1: tx pid %d send before receiver ready: send blocks", producer_pid);
	sysputs(buffer);
	char teststr[buffsize];
	sprintf (teststr, "TEST1: MSG: send msg from pid %d, sender blocks until rcvr ready\0", producer_pid);
    int bytes_sent=0;
    bytes_sent = syssend(consumer_pid,teststr,strlen(teststr));
    char result[buffsize];
    sprintf (result,"TEST1: tx producer sent %d bytes to consumer pid\n", bytes_sent);
    sysputs(result);
	mypause(pausetime);
	// allow consumer to invoke send before we invoke recv
	// consumer will block waiting for us to be ready to receive
	// receiver will copy from sender,consumer,pid 2 to 
	// receiver, producser, pid3
	// receive a  response
	int bytes_recvd = 0;
	memset(teststr,0,buffsize);
	bytes_recvd = sysrecv(&consumer_pid, teststr, sizeof(result));
	sprintf(result,"TEST2: rx producer received %d bytes from consumer", bytes_recvd);
	sysputs(result);
	sysputs(teststr);
	mypause(pausetime);
	//test sending a large buffer to a small buffer
	sprintf(result,"TEST3: tx Sending %d byte msg to 64 byte receiver",125);
	sysputs(result);
	char largemsg[140];
	sprintf(largemsg,"TEST3: MSG pid: %d,large message being sent to a smaller message buffer to prove that it will correclty truncate and not crash", producer_pid );
	bytes_sent = syssend(consumer_pid,largemsg,strlen(largemsg));
    sprintf (result," TEST3: tx  producer sent %d bytes to consumer\n", bytes_sent);
    sysputs(result);
	mypause(pausetime);
	//sysyield();
	
	char abuffer[buffsize];
	//// now send a message to rcvr_pid process
	sprintf(result,"TEST4: tx pid %d send to waiting universal receiver",producer_pid);
	memset(abuffer, 0, buffsize);
	sprintf(abuffer,"TEST4: MSG 4. Producer pid %d sent",producer_pid);
	bytes_sent=syssend(rcvr_pid, abuffer, strlen(abuffer));
	sprintf(result, "TEST4: tx producer sent %d bytes to rcvr_pid", bytes_sent);
	sysputs(result);
	mypause(pausetime);
    return;
}

 void consumer( void ) {
/****************************/

    consumer_pid = sysgetpid();
    DBGMSG("consumer pid = %d\n", consumer_pid);

	char abuffer[buffsize];
	int bytes_recvd = 0;
	//wait until producer has started so we can use his pid
	while (!producer_pid){

	}
	unsigned int recv_from_pid = producer_pid;
	

	bytes_recvd = sysrecv(&recv_from_pid,abuffer, buffsize);
	char result[buffsize];
	sprintf(result, "TEST1: rx consumer recvd %d bytes from pid 3\n", bytes_recvd);
	sysputs (result);
	sprintf(result, "TEST1: rx msg recd by %d was:\n", consumer_pid);
	sysputs(result);
	sysputs(abuffer); 

	mypause(pausetime);
	// now send a response to producer, no yield so we will block waiting for
	// producer/reciver to show up.  rcvr will copy message
	memset(abuffer, 0 , buffsize);
	sprintf(result,"TEST%d: tx  send to waiting receiver", 2);
	sysputs(result);
	sprintf(abuffer,"TEST2: msg Consumer %d sending to waiting reciver\n", consumer_pid);
	int bytes_sent=syssend(producer_pid, abuffer, strlen(abuffer));
	sprintf(result, "TEST2: tx Consumer sent %d bytes\n", bytes_sent);
	sysputs (result);
	mypause(pausetime);
	// set up reciver for large message sender
	bytes_recvd = sysrecv(&recv_from_pid,abuffer, buffsize);
	//char result[buffsize];
	sprintf(result, "TEST3: consumer recvd %d bytes from producer\n", bytes_recvd);
	sysputs (result);
	sysputs(abuffer);
	mypause(pausetime);
	
	
	//// now send a message to rcvr_pid process
	sprintf(result, "TEST4: tx pid %d send to waiting universal receiver",consumer_pid);
	sysputs(result);
	memset(abuffer, 0, buffsize);
	sprintf(abuffer,"TEST4: MSG  Consumer %d is sending",consumer_pid);
	bytes_sent=syssend(rcvr_pid, abuffer, strlen(abuffer));
	
	sprintf(result, "TEST4: tx consumer sent %d bytes to universal rcvr", bytes_sent);
	sysputs(result);
	mypause(pausetime);
	
	unsigned int invalid = 99;
	sprintf(abuffer, "TEST5: tx send to invalid pid %d  ",invalid);
	sysputs(abuffer);
	sprintf(abuffer, "TEST5: MSG send to invalid pid %d  ",invalid);
	bytes_sent=syssend(invalid, abuffer, strlen(abuffer));
	sprintf(result, "TEST5: tx consumer sent %d bytes invalid pid", bytes_sent);
	sysputs(result);
	mypause(pausetime);
	
	unsigned int mypid = sysgetpid();
	sprintf(abuffer, "TEST6: rx from own pid %d  ",mypid);
	sysputs(abuffer);
	sprintf(abuffer, "TEST6: MSG send to own pid %d  ",mypid);
	bytes_sent=sysrecv(&mypid, abuffer, strlen(abuffer));
	sprintf(result, "TEST6: rx consumer sent %d bytes invalid pid", bytes_sent);
	sysputs(result);
	mypause(pausetime);
	
	
	sprintf(abuffer, "TEST7: rx bad memory address %d  ",0x10);
	sysputs(abuffer);
	sprintf(abuffer, "TEST7: MSG bad memory address %d  ",0x10);
	bytes_sent=sysrecv(&rcvr_pid, (char*)0x10, strlen(abuffer));
	sprintf(result, "TEST7: rx consumer recvd %d bytes bad buffer address", bytes_sent);
	sysputs(result);
	mypause(pausetime);
	
    return;
}


// start universal reciever to receive messages from anyone and print them out
// in this case, receiver will block waiting for senders to show up
void receiveall(void ){
	unsigned int fromaddr=0;
	unsigned int* pfromaddr = &fromaddr;
	
	char recv_buffer[buffsize];
	char result[buffsize];
	rcvr_pid= sysgetpid();
	
	//int i;
	for(;;){
		int bytes_in = sysrecv(pfromaddr,recv_buffer,sizeof(recv_buffer));
		sprintf(result, "TEST4: rx recv(0) recvd %d bytes", bytes_in);
		sysputs(result);
		//  updated pid from sysrecv sent back to us
		sprintf(result, "TEST4: rx message was sent by pid %d", fromaddr);
		sysputs(result);
		sysputs(recv_buffer);
		//reset for next message
		fromaddr=0;
		memset(recv_buffer,0, sizeof(recv_buffer));
	}


}


void sender1(void){
	while(sender2pid==0){}
	
	char abuf[buffsize];
	sprintf(abuf,"pid(%d) will block if successful",sysgetpid());
	sysputs(abuf);
	int bytesout = syssend(sender2pid,abuf,sizeof(abuf));
	char result[buffsize];
	sprintf(result,"sender1 result = %d ", bytesout);
	sysputs(result);
	
}

void sender2(void){
	while (sender1pid==0){}
		
	char abuf[buffsize];
	sprintf(abuf,"pid(%d) will block if successful",sysgetpid());
	sysputs(abuf);
	int bytesout = syssend(sender1pid,abuf,sizeof(abuf));
	char result[buffsize];
	sprintf(result,"sender2 result = %d ", bytesout);
	sysputs(result);
	
}

void rcvr1(void){
	while(rcvr2pid==0){}
	unsigned int target = rcvr2pid;
	
	char abuf[buffsize];
	sprintf(abuf,"recvr1 pid(%d) will block if successful",sysgetpid());
	sysputs(abuf);
	int bytesin = sysrecv(&target,abuf,sizeof(abuf));
	char result[buffsize];
	sprintf(result,"rcvr1 result = %d ", bytesin);
	sysputs(result);
	
}

void rcvr2(void){
	while (rcvr1pid==0){}
	unsigned int target = rcvr1pid;
	
	char abuf[buffsize];
	sprintf(abuf,"recvr2 pid(%d) will block if successful",sysgetpid());
	sysputs(abuf);
	int bytesin = sysrecv(&target,abuf,sizeof(abuf));
	char result[buffsize];
	sprintf(result,"rcvr2 result = %d ", bytesin);
	sysputs(result);
	
}

 void     root( void ) {
/****************************/
	root_pid = sysgetpid();
	
    kprintf("Root has been called\n");
    DBGMSG("root pid  = %d\n", root_pid);
	
	//sysyield();
    int childpid;
    childpid = syscreate( &consumer, 4096 );
    DBGMSG("consumer child pid = %d\n",childpid);
    //sysyield();
    childpid = syscreate( &producer, 4096 );
    DBGMSG("producer child pid = %d\n",childpid);
    //sysyield();
    childpid = syscreate(&receiveall,4096);
    DBGMSG("receive all child pid = %d\n",childpid);
	syssleep(30000);
	
	char abuf[buffsize];
	sprintf(abuf,"ROOT starting send to sender test", 1);
	sysputs(abuf);
	//test ability to detect blocking scenario,
	sender1pid = syscreate(&sender1,4096);
	sender2pid = syscreate(&sender2,4096);
	syssleep(20000);
	
	sprintf(abuf,"ROOT starting recv to rcvr test", 1);
	sysputs(abuf);
	//test ability to detect blocking scenario,
	rcvr1pid = syscreate(&rcvr1,4096);
	rcvr2pid = syscreate(&rcvr2,4096);
	syssleep(20000);
	
}

void idleproc (void){
	
	char abuffer[buffsize];
	sprintf(abuffer,"idleproc is running as pid %d\n", sysgetpid());
	sysputs(abuffer);
	//park idle proc in blocking queue - optional, can just leave it out of any queues
	//but this is a convient way to get it off the run state and out of the ready q
	sysrecv(0,abuffer, buffsize);
	
	//sysputs(abuffer);
	TRACE
	while(1){
		asm
		( " \
		hlt \n\
        "
        : 
        : 
        : "%eax"
		);

	}
}

#else
/****************Extended Produce Consumer*********************************/
#define buffsize 64

unsigned int root_pid;


 void childomine( void ) {
/****************************/
	char cbuffer[buffsize];
	char msgbuffer[buffsize];
	sprintf(cbuffer,"First Breath of child pid = %d", sysgetpid());
	sysputs(cbuffer);
	
	syssleep(5000);

	int recvd = sysrecv(&root_pid,msgbuffer,sizeof(cbuffer));
	if (recvd){
		int sleeptime = atoi(msgbuffer);
		sprintf(cbuffer,"Msg from root received. SLEEP TIME = %d", sleeptime);
		sysputs(cbuffer);
		sprintf(cbuffer,"PID (%d) going to sleep now",sysgetpid());
		sysputs(cbuffer);
		syssleep(sleeptime);
		sprintf(cbuffer,"pid %d Stopped Sleeping, Exiting Now", sysgetpid());
		sysputs(cbuffer);
	}else{
		sprintf(cbuffer,"recv failed, error code = %d", recvd);
		sysputs(cbuffer);
	}
	
	return;
}


 void     root( void ) {
/****************************/
	root_pid = sysgetpid();
	char buffer[buffsize];
	char mbuffer[buffsize];
	sprintf(buffer,"ROOT IS ALIVE, PID = %d",root_pid);
	sysputs(buffer);
	
	unsigned int children[4];
	int childpid;
	int i;
	for (i=0;i<4;i++){
		childpid = syscreate( &childomine, 4096 );
		sprintf(buffer, "CHILD is launched, pid = %d", childpid);
		sysputs (buffer);
		children[i]=childpid;
	}
	syssleep(4000);
	
	int bytesout;
	sprintf(mbuffer,"%d",10000);
	bytesout = syssend(children[2],mbuffer, sizeof(mbuffer));
	if (bytesout<=0) {
		sprintf(buffer,"failed to send to %d", children[2]);
	}else{
		sprintf(buffer,"ROOT sent %d bytes", bytesout);
	}
	sysputs(buffer);
	
	sprintf(mbuffer,"%d",7000);
	bytesout = syssend(children[1],mbuffer, sizeof(mbuffer));
	if (bytesout<=0) {
		sprintf(buffer,"failed to send to %d", children[1]);
	}else{
		sprintf(buffer,"ROOT sent %d bytes", bytesout);
	}
	sysputs(buffer);
	
	sprintf(mbuffer,"%d",20000);
	bytesout = syssend(children[0],mbuffer, sizeof(mbuffer));
	if (bytesout<=0) {
		sprintf(buffer,"failed to send to %d", children[0]);
	}else{
		sprintf(buffer,"ROOT sent %d bytes", bytesout);
	}
	sysputs(buffer);
	
	sprintf(mbuffer,"%d",27000);
	bytesout = syssend(children[3],mbuffer, sizeof(mbuffer));
	if (bytesout<=0) {
		sprintf(buffer,"failed to send to %d", children[3]);
	}else{
		sprintf(buffer,"ROOT sent %d bytes", bytesout);
	}
	sysputs(buffer);
	
	sprintf(buffer,"ROOT start attempt to recv from child %d ",  4);
	sysputs(buffer);
	unsigned int childin = children[3];
	int bytesin = sysrecv(&childin,mbuffer, sizeof(mbuffer));
	sprintf(buffer,"ROOT attempt to recv from child 4 resulted in rc = %d", bytesin);
	sysputs(buffer);
	
	sprintf(mbuffer,"The Time has come for all my %d children to say goodbye",4);
	bytesout = syssend(children[2],mbuffer,sizeof(buffer));
	sprintf(buffer,"ROOT attempt to send to child 2 resulted in rc = %d", bytesin);
	sysputs(buffer); 
	

 
}

void idleproc (void){
	//char abuffer[buffsize];
	//TRACE
	//unsigned int anypid=root_pid;
	
	//sprintf(abuffer,"idleproc started as %d\n", sysgetpid());
	//sysrecv(&anypid,abuffer, sizeof(abuffer));
	//sysputs(abuffer);
	
	TRACE
	while(1){
		#ifdef DEBUG
		//show when idleproc is running
		kprintf(".");
		#endif
		asm
		( " \
		hlt \n\
        	"
        	: 
        	: 
        	: "%eax"
		);

	}
}

#endif
