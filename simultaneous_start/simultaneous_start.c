/*
 * Example of using commands - simultaneous start of analog input
 * and analog output
 * Part of Comedilib
 *
 * Copyright (c) 1999,2000,2001,2002 David A. Schleef <ds@schleef.org>
 *
 * This file may be freely modified, distributed, and combined with
 * other software, as long as proper attribution is given in the
 * source code.
 */

#include <stdio.h>
#include <comedilib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "examples.h"

#define BUFSZ 10000
char buf[BUFSZ];

#define N_CHANS 256
unsigned int ai_chanlist[N_CHANS];
unsigned int ao_chanlist[N_CHANS];

unsigned int n_ai_chan = 1;
unsigned int n_ao_chan = 1;

unsigned int ai_subd;
unsigned int ao_subd;

comedi_t *dev;

int ai_buf_len = 4096;
int ao_buf_len = 4096;
char ai_buf[4096];
char ao_buf[4096];

void setup_ai(comedi_cmd *cmd);
void setup_ao(comedi_cmd *cmd);
void input_some(void);
void output_some(void);

void do_cmd(comedi_t *dev,comedi_cmd *cmd);

char *cmdtest_messages[]={
	"success",
	"invalid source",
	"source conflict",
	"invalid argument",
	"argument conflict",
	"invalid chanlist",
};

int comedi_internal_trigger(comedi_t *dev, unsigned int subd, unsigned int trignum)
{
	comedi_insn insn;
	lsampl_t data[1];

	memset(&insn, 0, sizeof(comedi_insn));
	insn.insn = INSN_INTTRIG;
	insn.subdev = subd;
	insn.data = data;
	insn.n = 1;

	data[0] = trignum;

	return comedi_do_insn(dev, &insn);
}

int main(int argc, char *argv[])
{
	comedi_cmd ai_c,*ai_cmd=&ai_c;
	comedi_cmd ao_c,*ao_cmd=&ao_c;
	int ret;
	struct timeval start,end;
	struct timeval timeout;
	fd_set rdset;
	fd_set wrset;

	parse_options(argc,argv);

	/* The following global variables used in this demo are
	 * defined in common.c, and can be modified by command line
	 * options.  When modifying this demo, you may want to
	 * change them here. */
	//filename = "/dev/comedi0";
	//subdevice = 0;
	//channel = 0;
	//range = 0;
	//aref = AREF_GROUND;
	//n_chan = 4;
	//n_scan = 1000;
	//freq = 1000.0;

	/* open the device */
	dev = comedi_open(filename);
	if(!dev){
		comedi_perror(filename);
		exit(1);
	}

	/* NI E series boards: configure PFI6 to be AO start signal */
	comedi_dio_config(dev,7,6,COMEDI_OUTPUT);

	ai_subd = 0;
	setup_ai(ai_cmd);

	ao_subd = 1;
	setup_ao(ao_cmd);

	/* start the AI command */
	ret=comedi_command(dev,ai_cmd);
	if(ret<0){
		comedi_perror("comedi_command");
		exit(1);
	}

	/* start the AO command */
	ret=comedi_command(dev,ao_cmd);
	if(ret<0){
		comedi_perror("comedi_command");
		exit(1);
	}

	output_some();
	output_some();

	/* this is only for informational purposes */
	gettimeofday(&start,NULL);
	fprintf(stderr,"start time: %ld.%06ld\n",start.tv_sec,start.tv_usec);

	/* go */
	ret = comedi_internal_trigger(dev, ao_subd, 0);
	if(ret<0){
		perror("comedi_internal_trigger");
		exit(1);
	}

	while(1){
		FD_ZERO(&rdset);
		FD_ZERO(&wrset);
		FD_SET(comedi_fileno(dev), &rdset);
		FD_SET(comedi_fileno(dev), &wrset);

		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;

		ret = select(comedi_fileno(dev) + 1, &rdset, &wrset, NULL,
			&timeout);

		if(ret<0){
			if(errno==EINTR)continue;
			perror("select");
		}else if(ret==0){
			printf(".");
			fflush(stdout);
		}else if(FD_ISSET(comedi_fileno(dev), &rdset)){
			input_some();
		}else if(FD_ISSET(comedi_fileno(dev), &wrset)){
			output_some();
		}else{
			printf("unknown file descriptor ready %d\n",ret);
		}
	}

	/* this is only for informational purposes */
	gettimeofday(&end,NULL);
	fprintf(stderr,"end time: %ld.%06ld\n",end.tv_sec,end.tv_usec);

	end.tv_sec-=start.tv_sec;
	if(end.tv_usec<start.tv_usec){
		end.tv_sec--;
		end.tv_usec+=1000000;
	}
	end.tv_usec-=start.tv_usec;
	fprintf(stderr,"time: %ld.%06ld\n",end.tv_sec,end.tv_usec);

	return 0;
}

void input_some(void)
{
	int ret;

	ret = read(comedi_fileno(dev), ai_buf, ai_buf_len);
	if(ret<0){
		perror("read");
		return;
	}else if(ret == 0){
		return;
	}else{
		printf("read %d\n",ret);
	}
}

void output_some(void)
{
	int ret;

	ret = write(comedi_fileno(dev), ao_buf, ao_buf_len);
	if(ret<0){
		perror("write");
		return;
	}else if(ret == 0){
		return;
	}else{
		printf("write %d\n",ret);
	}
}

void setup_ai(comedi_cmd *cmd)
{
	unsigned int subd;
	int ret;
	int i;

	/* Set up channel list */
	for(i=0;i<n_ai_chan;i++){
		ai_chanlist[i]=CR_PACK(channel+i,range,aref);
	}

	//subd = comedi_get_read_subdevice(dev);
	subd = 0;

	ret = comedi_get_cmd_generic_timed(dev,subdevice,cmd,1e9/freq);
	if(ret<0){
		printf("comedi_get_cmd_generic_timed failed\n");
		return;
	}

	cmd->subdev = subd;

	cmd->chanlist		= ai_chanlist;
	cmd->chanlist_len	= n_ai_chan;

	cmd->scan_end_arg = n_ai_chan;
	if(cmd->stop_src==TRIG_COUNT)cmd->stop_arg = n_scan;

	cmd->start_src = TRIG_EXT;
	cmd->start_arg = 6;
	cmd->stop_src = TRIG_NONE;
	cmd->stop_arg = 0;

	fprintf(stderr,"command before testing:\n");
	dump_cmd(stderr,cmd);

	ret = comedi_command_test(dev,cmd);
	if(ret<0){
		comedi_perror("comedi_command_test");
		if(errno==EIO){
			fprintf(stderr,"Ummm... this subdevice doesn't support commands\n");
		}
		exit(1);
	}
	fprintf(stderr,"first test returned %d (%s)\n",ret,
			cmdtest_messages[ret]);
	dump_cmd(stderr,cmd);

	ret = comedi_command_test(dev,cmd);
	if(ret<0){
		comedi_perror("comedi_command_test");
		exit(1);
	}
	fprintf(stderr,"second test returned %d (%s)\n",ret,
			cmdtest_messages[ret]);
	if(ret!=0){
		dump_cmd(stderr,cmd);
		fprintf(stderr,"Error preparing command\n");
		exit(1);
	}
}

void setup_ao(comedi_cmd *cmd)
{
	unsigned int subd;
	int ret;
	int i;

	/* Set up channel list */
	for(i=0;i<n_ao_chan;i++){
		ao_chanlist[i]=CR_PACK(channel+i,range,aref);
	}

	//subd = comedi_get_write_subdevice(dev);
	subd = 1;

	ret = comedi_get_cmd_generic_timed(dev,subd,cmd,1e9/freq);
	if(ret<0){
		printf("comedi_get_cmd_generic_timed failed\n");
		return;
	}

	cmd->start_src = TRIG_INT;
	cmd->start_arg = 0;
	//cmd->scan_begin_src = TRIG_TIMER;
	//cmd->scan_begin_arg = 100000;
	cmd->convert_src = TRIG_NOW;
	cmd->convert_arg = 0;

	cmd->subdev = subd;

	cmd->chanlist		= ao_chanlist;
	cmd->chanlist_len	= n_ao_chan;

	cmd->scan_end_arg = n_ao_chan;
	if(cmd->stop_src==TRIG_COUNT)cmd->stop_arg = n_scan;

	fprintf(stderr,"command before testing:\n");
	dump_cmd(stderr,cmd);

	ret = comedi_command_test(dev,cmd);
	if(ret<0){
		comedi_perror("comedi_command_test");
		if(errno==EIO){
			fprintf(stderr,"Ummm... this subdevice doesn't support commands\n");
		}
		exit(1);
	}
	fprintf(stderr,"first test returned %d (%s)\n",ret,
			cmdtest_messages[ret]);
	dump_cmd(stderr,cmd);

	ret = comedi_command_test(dev,cmd);
	if(ret<0){
		comedi_perror("comedi_command_test");
		exit(1);
	}
	fprintf(stderr,"second test returned %d (%s)\n",ret,
			cmdtest_messages[ret]);
	if(ret!=0){
		dump_cmd(stderr,cmd);
		fprintf(stderr,"Error preparing command\n");
		exit(1);
	}

}

