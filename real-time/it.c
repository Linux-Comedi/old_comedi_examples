
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/comedi.h>
#include <linux/comedilib.h>
#include <rtai.h>

#define BUFSIZE 1024

int dev=0;
int subd=0;

sampl_t data[BUFSIZE];

int do_cmd(void)
{
	int ret;

	comedi_cmd cmd;
	unsigned int chanlist[1];

	cmd.subdev = subd;
	cmd.flags = 0;

	cmd.start_src = TRIG_NOW;
	cmd.start_arg = 0;

	cmd.scan_begin_src = TRIG_TIMER;
	cmd.scan_begin_arg = 1000000000;

	cmd.convert_src = TRIG_TIMER;
	cmd.convert_arg = 0;

	cmd.scan_end_src = TRIG_COUNT;
	cmd.scan_end_arg = 1;

	cmd.stop_src = TRIG_COUNT;
	cmd.stop_arg = 10;

	cmd.chanlist = chanlist;
	cmd.chanlist_len = 1;

	chanlist[0] = CR_PACK(0,0,0);

	cmd.data=data;
	cmd.data_len=BUFSIZE*sizeof(sampl_t);

	ret = comedi_command_test(dev,&cmd);
	printk("command test returned %d\n",ret);

	cmd.chanlist = chanlist;
	cmd.chanlist_len = 1;
	cmd.data=data;
	cmd.data_len=BUFSIZE*sizeof(sampl_t);

	ret = comedi_command_test(dev,&cmd);
	printk("command test returned %d\n",ret);
	if(ret)return ret;

	cmd.chanlist = chanlist;
	cmd.chanlist_len = 1;
	cmd.data=data;
	cmd.data_len=BUFSIZE*sizeof(sampl_t);

	ret = comedi_command(dev,&cmd);
	printk("command returned %d\n",ret);

	return ret;
}

static int counter = 0;

int callback(unsigned int i,void *arg)
{
	rt_printk("who's yer daddy? %d\n",counter);
	counter++;

	return 0;
}

int init_module(void)
{
	int ret;

	printk("Comedi real-time example #1\n");

	ret = comedi_open(dev);
	printk("comedi_open: %d\n",ret);

	ret = comedi_lock(dev,subd);
	printk("comedi_lock: %d\n",ret);

	comedi_register_callback(dev,subd,COMEDI_CB_EOS,callback,NULL);

	do_cmd();

	return 0;
}

void cleanup_module(void)
{
	int ret;

	ret = comedi_cancel(dev,subd);
	printk("comedi_cancel: %d\n",ret);

	ret = comedi_unlock(dev,subd);
	printk("comedi_unlock: %d\n",ret);

	comedi_close(dev);
	printk("comedi_close:\n");
}

