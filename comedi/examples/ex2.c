
#include <linux/kernel.h>
#include <linux/comedi.h>
#include <linux/comedilib.h>

#include <rtai.h>
#include <rtai_sched.h>

RT_TASK *task;

unsigned int subdevice;
unsigned int channel;
sampl_t *data;

unsigned int mask;

static void create_cmd(comedi_cmd *cmd);

static int callback(unsigned int i, void *arg)
{
	mask = i;
	rt_task_resume(task);
	return 0;
}

int main(int argc,char *argv[])
{
	int i;
	int ret;
	comedi_cmd cmd;

	task = rt_whoami();

	create_cmd(&cmd);

	ret = comedi_command_test(0,&cmd);
	rt_printk("comedi_cmd_test = %d\n",ret);

	comedi_register_callback(0,subdevice,COMEDI_CB_EOS,callback,NULL);
	comedi_command(0,&cmd);

	for(i=0;i<200;i++){
		rt_task_suspend(task);
		ret = comedi_get_buf_head_pos(0,subdevice);
		rt_printk("mask = %d, head_pos = %d\n",mask,ret);
	}

	comedi_cancel(0,subdevice);

	return 42;
}

static void create_cmd(comedi_cmd *cmd)
{
	cmd->subdev = subdevice;
	cmd->flags = 0;

	cmd->start_src = TRIG_NOW;
	cmd->start_arg = 0;

	cmd->scan_begin_src = TRIG_TIMER;
	cmd->scan_begin_arg = 1000000000;

	cmd->convert_src = TRIG_TIMER;
	cmd->convert_arg = 1000000;

	cmd->scan_end_src = TRIG_COUNT;
	cmd->scan_end_arg = 1;

	cmd->stop_src = TRIG_COUNT;
	cmd->stop_arg = 10000;

	cmd->chanlist_len = 1;
	cmd->chanlist = &channel;
}

