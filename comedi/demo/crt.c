
#include <linux/kernel.h>
#include <linux/module.h>

#include <rtai.h>
#include <rtai_sched.h>


static RT_TASK task;

int main(int argc,char *argv[]);
static void _main(int unused);

static int stack_size = 4096;

static int __init init_mod(void)
{
	int ret;

	ret = rt_task_init(&task, _main, 0, stack_size, 0, 0, NULL);
	//printk("rt_task_init = %d\n",ret);

	ret = rt_task_resume(&task);
	//printk("rt_task_resume = %d\n",ret);

	return 0;
}

static void __exit cleanup_mod(void)
{
	rt_task_delete(&task);
}

module_init(init_mod);
module_exit(cleanup_mod);

static void _main(int unused)
{
	int argc = 1;
	int ret;
	char *argv[] = { "task", 0 };

	ret = main(argc,argv);

	exit(ret);
}

void exit(int value)
{
	rt_printk("task exited with %d\n",value);

	rt_task_suspend(&task);

	rt_printk("ack!  task unsuspended when finished!\n");

	while(1)rt_task_suspend(&task);
}

