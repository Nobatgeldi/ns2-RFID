#include "rmac.h"
#include "rfid_hdr.h"

#define SLOT_F 0.5	//slot frame ,we say 0.00006 

static class RfidRMacClass : public TclClass {
public:
	RfidRMacClass() : TclClass("Mac/R-Rfid") {}
	TclObject* create(int,const char* const*) {
		return new RfidRMac();
	}
}class_rfidrmac;

RfidRMac::RfidRMac() :Mac() {
	state(MAC_IDLE);
	ltimer= new LTimer(this);
}

void RfidRMac::recv(Packet *p,Handler *h){
	struct hdr_cmn *cmnh= HDR_CMN(p);
	if (cmnh->direction() ==hdr_cmn::DOWN){
		rfid_hdr *rh=HDR_RFID(p);
		if (rh->cmd==RFID_QUERY){
			state(MAC_POLLING);
			ltimer->start(SLOT_F);
			downtarget_->recv(p,h);
			return;
		}else{
			/* other rfid_command arrived
			 * rmac just hand it over to phy lay
			 */
			downtarget_->recv(p,h);
			h->handle(p); // Ragent recv()should be called
			return;
		}
	}
	if(cmnh->direction()==hdr_cmn::UP){
	/*XXX hold the place for further
	***coding
	*/
		fprintf(stderr,"#####%d\n",HDR_RFID(p)->cmd);
		if (state()==MAC_POLLING && HDR_RFID(p)->cmd==RFID_RN16){
			state(MAC_RECV);
			ltimer->stop();
			fprintf(stderr,"rmac at %.5f get a RN16\n",Scheduler::instance().clock());
		}
		return;
	}
}	

int RfidRMac ::command(int argc,const char*const* argv){
	if (argc==3){
		if (strcmp(argv[1],"set-agent")==0){
			if((agent=(RfidRAgent*)TclObject::lookup(argv[2]))
					==0){
				fprintf(stderr,"rmac set-agent failed\n");
				return TCL_ERROR;
			}
			return TCL_OK;
		}
	}
	return Mac::command(argc,argv);
}
void RfidRMac :: timeup(){
	agent->timeup();
	state(MAC_IDLE);
}
void RfidTimer::start(double time){
	Scheduler &s=Scheduler::instance();
	assert(busy==0);

	busy=1;
	stime=s.clock();
	rtime=time;
	assert(time >=0.0);
	s.schedule(this,&intr,rtime);
}

void RfidTimer::stop(){
	Scheduler &s = Scheduler::instance();
	assert(busy);
	s.cancel(&intr);
	
	busy = 0;
	stime = rtime = 0.0;
}
void RfidTimer::reset(){
	if (busy) 
		stop();
	stime =rtime=0.0;
}
void RfidTimer::restart(double time){
	if (busy)
		stop();
	start(time);
}
void LTimer::handle(Event *e){
	reset();
	mac->timeup();
}
