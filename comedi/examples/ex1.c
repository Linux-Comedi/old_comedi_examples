
#include <linux/kernel.h>
#include <rtai.h>
#include <rtai_sched.h>


int main(int argc,char *argv[])
{
	int i;
	RT_TASK *task;
	RTIME tick_period;
	RTIME now;

	task = rt_whoami();

	tick_period = start_rt_timer(nano2count(100000000));
	now = rt_get_time();

	rt_task_make_periodic(task,now + tick_period, tick_period);

	for(i=0;i<100;i++){
		rt_printk("%d\n",i);
		rt_task_wait_period();
	}

	return 42;
}

