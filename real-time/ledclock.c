
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/comedi.h>
#include <linux/comedilib.h>
#include <rtai.h>

#define BUFSIZE 1024

int dev=1;
int in_subd=3;
int out_subd=0;

sampl_t data[BUFSIZE];

int do_cmd(void)
{
	int ret;

	comedi_cmd cmd;
	unsigned int chanlist[1];

	cmd.subdev = in_subd;
	cmd.flags = 0;

	cmd.start_src = TRIG_NOW;
	cmd.start_arg = 0;

	cmd.scan_begin_src = TRIG_EXT;
	cmd.scan_begin_arg = 0;

	cmd.convert_src = TRIG_ANY;
	cmd.convert_arg = 0;

	cmd.scan_end_src = TRIG_COUNT;
	cmd.scan_end_arg = 1;

	cmd.stop_src = TRIG_NONE;
	cmd.stop_arg = 0;

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
	int tmp;
	int ret;

	tmp=counter;
	ret = comedi_dio_bitfield(dev,out_subd,0xff,&tmp);
//	printk("who's yer daddy? %d %d\n",counter,ret);
	counter++;

	return 0;
}

int init_module(void)
{
	int ret;

	printk("Comedi real-time example #1\n");

	ret = comedi_open(dev);
	printk("comedi_open: %d\n",ret);

	ret = comedi_lock(dev,in_subd);
	printk("comedi_lock: %d\n",ret);
	
	ret = comedi_lock(dev,out_subd);
	printk("comedi_lock: %d\n",ret);

	ret = comedi_register_callback(dev,in_subd,COMEDI_CB_EOS,callback,NULL);
	printk("comedi_register_callback: %d\n",ret);

	do_cmd();

	return 0;
}

void cleanup_module(void)
{
	int ret;

	ret = comedi_cancel(dev,in_subd);
	printk("comedi_cancel: %d\n",ret);

	/* not necessary */
	//comedi_register_callback(dev,in_subd,0,NULL,NULL);

	ret = comedi_unlock(dev,in_subd);
	printk("comedi_unlock: %d\n",ret);

	ret = comedi_unlock(dev,out_subd);
	printk("comedi_unlock: %d\n",ret);

	comedi_close(dev);
	printk("comedi_close:\n");
}

