/*
 * Example of using commands - asynchronous input
 * Part of Comedilib
 *
 * Copyright (c) 1999,2000,2001 David A. Schleef <ds@schleef.org>
 *
 * This file may be freely modified, distributed, and combined with
 * other software, as long as proper attribution is given in the
 * source code.
 */

/*
 * An example for directly using Comedi commands.  Comedi commands
 * are used for asynchronous acquisition, with the timing controlled
 * by on-board timers or external events.
 */

#include <stdio.h>
#include <comedilib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <plplot/plplot.h>
#include <math.h>
#include <readline/readline.h>
#include "examples.h"

#define BUFSZ 100000
char buf[BUFSZ];

#define N_CHANS 256
unsigned int chanlist[N_CHANS];

comedi_range *rng;
lsampl_t maxdata;

unsigned int chunk;
unsigned int offset;

int go;

void plotinit(void);
void plotdata(sampl_t *data, int n);

void handle_line(char *);

void main_loop(comedi_t *dev);
void input_some(comedi_t *dev);
void start_input(comedi_t *dev, unsigned int s);
int prepare_cmd_lib(comedi_t *dev,int subdevice,comedi_cmd *cmd);

void do_cmd(comedi_t *dev,comedi_cmd *cmd);

char *cmdtest_messages[]={
	"success",
	"invalid source",
	"source conflict",
	"invalid argument",
	"argument conflict",
	"invalid chanlist",
};

int main(int argc, char *argv[])
{
	comedi_t *dev;
	int i;

	parse_options(argc,argv);

	plotinit();
	rl_callback_handler_install("scope: ",handle_line);

	/* The following global variables used in this demo are
	 * defined in common.c, and can be modified by command line
	 * options.  When modifying this demo, you may want to
	 * change them here. */
	//filename = "/dev/comedi0";
	//subdevice = 0;
	//channel = 0;
	//range = 0;
	//aref = AREF_GROUND;
	n_chan = 1;
	n_scan = 1000000;
	freq = 10000.0;

	/* open the device */
	dev = comedi_open(filename);
	if(!dev){
		comedi_perror(filename);
		exit(1);
	}

	comedi_set_global_oor_behavior(COMEDI_OOR_NUMBER);
	maxdata = comedi_get_maxdata(dev, subdevice, channel);
	rng = comedi_get_range(dev, subdevice, channel, range);

	/* Set up channel list */
	for(i=0;i<n_chan;i++){
		chanlist[i]=CR_PACK(channel+i,range,aref);
	}

	start_input(dev,subdevice);

	main_loop(dev);

	exit(0);
}


void main_loop(comedi_t *dev)
{
	fd_set rdset;
	struct timeval timeout;
	int ret;

	go = 1;
	chunk = 20000;
	offset = 0;
	while(go){
		FD_ZERO(&rdset);
		FD_SET(0,&rdset);
		FD_SET(comedi_fileno(dev),&rdset);
		timeout.tv_sec = 0;
		timeout.tv_usec = 50000;
		ret = select(comedi_fileno(dev) + 1, &rdset, NULL, NULL, &timeout);
		if(ret<0){
			perror("select");
		}else if(ret == 0){
			/* timeout */
			ret = comedi_poll(dev,subdevice);
			if(ret>0){
				input_some(dev);
			}
		}else if(FD_ISSET(0, &rdset)){
			rl_callback_read_char();
		}else if(FD_ISSET(comedi_fileno(dev), &rdset)){
			input_some(dev);
		}else{
			/* unknown */
		}
		if(offset>chunk){
			plotdata((sampl_t *)buf,chunk/2);
			offset -= chunk;
			memmove(buf,buf+chunk,offset);
		}
	}
}

void handle_line(char *s)
{
	if(!s){
		printf("quit\n");
		return;
	}
	printf("line: %s\n",s);

	if(strcmp(s,"quit")==0)exit(0);
	if(strcmp(s,"exit")==0)exit(0);

}

void input_some(comedi_t *dev)
{
	int ret;

	ret=read(comedi_fileno(dev),buf + offset,BUFSZ - offset);
	if(ret<0){
		perror("read");
		return;
	}else if(ret == 0){
		return;
	}else{
		offset += ret;
	}
}

void start_input(comedi_t *dev, unsigned int s)
{
	comedi_cmd c,*cmd=&c;
	int ret;

	prepare_cmd_lib(dev,subdevice,cmd);
	
	//fprintf(stderr,"command before testing:\n");
	//dump_cmd(stderr,cmd);

	ret = comedi_command_test(dev,cmd);
	if(ret<0){
		comedi_perror("comedi_command_test");
		if(errno==EIO){
			fprintf(stderr,"Ummm... this subdevice doesn't support commands\n");
		}
		exit(1);
	}
	//fprintf(stderr,"first test returned %d (%s)\n",ret,
	//		cmdtest_messages[ret]);
	//dump_cmd(stderr,cmd);

	ret = comedi_command_test(dev,cmd);
	if(ret<0){
		comedi_perror("comedi_command_test");
		exit(1);
	}
	//fprintf(stderr,"second test returned %d (%s)\n",ret,
	//		cmdtest_messages[ret]);
	if(ret!=0){
		dump_cmd(stderr,cmd);
		fprintf(stderr,"Error preparing command\n");
		exit(1);
	}

	/* start the command */
	ret=comedi_command(dev,cmd);
	if(ret<0){
		comedi_perror("comedi_command");
		exit(1);
	}
}

int prepare_cmd_lib(comedi_t *dev,int subdevice,comedi_cmd *cmd)
{
	int ret;

	memset(cmd,0,sizeof(*cmd));

	ret = comedi_get_cmd_generic_timed(dev,subdevice,cmd,1e9/freq);
	if(ret<0){
		printf("comedi_get_cmd_generic_timed failed\n");
		return ret;
	}

	/* Modify parts of the command */
	cmd->chanlist		= chanlist;
	cmd->chanlist_len	= n_chan;

	cmd->stop_src = TRIG_NONE;
	cmd->stop_arg = 0;

	cmd->scan_end_arg = n_chan;
	if(cmd->stop_src==TRIG_COUNT)cmd->stop_arg = n_scan;

	return 0;
}


void plotinit(void)
{
	plsdev("xwin");
	plinit();
	plspause(0);
	plssub(1,1);
}

void plotdata(sampl_t *data, int n)
{
	double *data_x;
	double *data_y;
	int i;
	double max, min;

	data_x = malloc(sizeof(double)*n);
	data_y = malloc(sizeof(double)*n);

	max = min = 0;
	for(i=0;i<n;i++){
		data_x[i]=i;
		data_y[i]=comedi_to_phys(data[i],rng,maxdata);
		if(max<data_y[i])max = data_y[i];
		if(min>data_y[i])min = data_y[i];
	}

	max = pow(10.0, ceil(log10(fabs(max)))) * ((max<0)?-1:1);
	min = pow(10.0, ceil(log10(fabs(min)))) * ((min<0)?-1:1);

	plbop();

	pladv(1);
	plvsta();
	plwind(0.0,n,min,max);
	plbox("bcnst", 0.0, 0, "bcnstv", 0.0, 0);
	pllab("sample #","voltage","");
	plline(n,data_x,data_y);

	pleop();

	free(data_x);
	free(data_y);
}

