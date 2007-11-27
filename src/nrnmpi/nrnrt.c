#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <nrnrt.h>

#if NRN_REALTIME
#include <sched.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
/*#include <rtai_lxrt.h>*/
#include <rtai_sem.h>

extern void nrn_fixed_step();
extern void nrn_fake_step();
extern double t, dt;
extern int stoprun;

int nrn_realtime_;
int nrnrt_overrun_;
double nrn_rtstep_time_;

/*
setting the scheduler to SCHED_FIFO is fully optional and just helps
response times when testing your program in soft realtime mode.
*/
void nrn_setscheduler() {
	if (nrn_realtime_) {
		struct sched_param mysched;
		mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
		if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
			puts("ERROR IN SETTING THE SCHEDULER");
			perror("errno");
			errno = 0;
			exit(1);
		}
	}
}

#define taskname(x) (1000 + (x))
static RT_TASK* maintask_;
static RT_TASK* rtrun_task_;
static SEM* sem_;
static pthread_t rtrun_thread_;
static int volatile end_, rt_running_;
static int period_;
static double tstop_;

static void msleep(int ms) {
	poll(NULL, 0, ms);
}

static int rtrun() {
	RTIME t1;
	int ovrn = 0;
	rt_running_ = 1;
	nrn_rtstep_time_ = 0.;
	start_rt_timer(period_);
	rt_make_hard_real_time();
	nrn_fake_step(); // get as much as possible into the cache
	rt_task_make_periodic(rtrun_task_, rt_get_time() + period_, period_);
	rt_task_wait_period();
	while (t < tstop_) {
		t1 = rt_get_cpu_time_ns();
		nrn_fixed_step();
		nrn_rtstep_time_ = (double)(rt_get_cpu_time_ns() - t1);
		ovrn += rt_task_wait_period();
		if (stoprun) { break; }
	}
	rt_make_soft_real_time();
	stop_rt_timer();
	rt_running_ = 0;
	return ovrn;
}

static void* rtrun_fun(void* arg) {
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (!(rtrun_task_ = rt_task_init_schmod(taskname(1), 0, 1000, 0,
	    SCHED_FIFO, 1))) {
		printf("Cannot init rtrun_fun task %u\n", taskname(1));
		exit(1);
	}
	mlockall(MCL_CURRENT | MCL_FUTURE);
	end_ = 1;
	rt_task_suspend(rtrun_task_);
	rt_sem_wait(sem_);
	while (!end_) {
		nrnrt_overrun_ = rtrun();
		rt_sem_wait(sem_);
	}
	rt_task_delete(rtrun_task_);
	++end_;
	return (void*)0;
}

void nrn_maintask_init() {
	if (nrn_realtime_) {
		char buf[10];
		int i = 0;
		int priority = 0; /* Highest */
		int stack_size = 0; /* Use default (512) */
		int msg_size = 0; /* Use default (256) */
		/* mlockall(MCL_CURRENT | MCL_FUTURE); */
		rt_grow_and_lock_stack(2000000); /* macro in rtai_lxrt.h */
		do {
			sprintf(buf, "NRN%d", ++i);
			printf("buf=|%s|\n", buf);
			printf("%ul\n", nam2num(buf));
		} while (rt_get_adr(nam2num(buf)));
printf("buf=|%s|\n", buf);
		maintask_ = rt_task_init(nam2num(buf), priority,
			stack_size, msg_size);
		printf("rt_task_init for maintask %s\n", buf);
		rtrun_task_ = maintask_; /*we will make a separate
			thread for if if we do not mind taking the extra time
			for switching and semaphore handling. i.e. using
			 large dt and want to be able to press the stop button
			*/
	}
}

void nrn_rtrun_thread_init() {
	int arg = 1;
	pthread_attr_t attr;
	if (rtrun_thread_) { return; }
	end_ = 0;
	pthread_attr_init(&attr);
	if (pthread_create(&rtrun_thread_, &attr, rtrun_fun, (void*)arg)) {
		printf("nrn_rtrun_thread_init error creating thread\n");
	}
	pthread_attr_destroy(&attr);
	/*
	// I do not know why the strange handshaking involving rt_task_suspend
	// and rt_task_resume is being done. I am slavishly following the
	// switching example in the rtai sources. I wonder if the sem
	// is not supposed to be created til after the thread. I am guessing
	// that is unnecessary and it would be sufficient merely to create it
	// before the thread and have the thread wait on the semaphore.
	// Oh well...
	*/
	sem_ = rt_sem_init(nam2num("SEMAPH"), 1);
	while (!end_) {
		msleep(50);
	}
	end_ = 0;
	rt_task_resume(rtrun_task_);
}

void nrn_maintask_delete() {
	if (nrn_realtime_) {
		if (rtrun_thread_) {
			end_ = 1;
			rt_sem_signal(sem_);
			while(end_ <= 1) {
				msleep(50);
			}
		}
		rt_task_delete(maintask_);
		printf("rt_task_delete for maintask\n");
	}
}

int nrn_rt_run(double tstop) {
	RTIME t1;
	RTIME period_in_nanosecs;
	if (rt_running_) {
		stoprun = 1;
		printf("Currently doing a realtime run. Setting stoprun=1.");
		return 0;
	}
	tstop_ = tstop;
	period_in_nanosecs = (RTIME)(dt*1000000.);
	rt_set_periodic_mode();
	period_ = (int) nano2count((RTIME)period_in_nanosecs);

	if (rtrun_thread_) {
		rt_sem_signal(sem_);
	}else{
		nrnrt_overrun_ = rtrun();
	}
	return nrnrt_overrun_;
}

#endif

