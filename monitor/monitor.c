

#include <stdio.h>
#include <comedilib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <ncurses.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "sv.h"
#include "sci_print.h"


typedef struct obs_struct obs;

struct obs_struct{
	char name[20];
	unsigned int subdevice;
	unsigned int chanspec;
	void (*observe)(int i);
};

obs obs_list[100];
int n_obs;

comedi_t *dev;

struct timeval now;
unsigned long nextsec;

void init_nc(void);
void setup_chans(void);
void dump_chans(void);

void observe_ai(int i);
void observe_ai2(int i);
void observe_ai3(int i);
void observe_misc(int i);
void observe_dio(int i);

/* layout */

int row_time = 0;
int row_info = 1;
int row_field = 2;
int col_time = 0;
int col_channel = 0;
int col_data = 20;

static char *subd_type_abbr[]={
	[COMEDI_SUBD_AI] = "ai",
	[COMEDI_SUBD_AO] = "ao",
	[COMEDI_SUBD_DI] = "di",
	[COMEDI_SUBD_DO] = "do",
	[COMEDI_SUBD_DIO] = "dio",
	[COMEDI_SUBD_COUNTER] = "ctr",
	[COMEDI_SUBD_TIMER] = "tmr",
	[COMEDI_SUBD_MEMORY] = "mem",
	[COMEDI_SUBD_CALIB] = "cal",
	[COMEDI_SUBD_PROC] = "pr",
};

void setup_chans(void)
{
	int subd;
	int n_subd;
	int n_chan;
	int type;
	int chan;
	int flags;

	n_subd = comedi_get_n_subdevices(dev);

	for(subd=0; subd<n_subd; subd++){
		type = comedi_get_subdevice_type(dev, subd);
		flags = comedi_get_subdevice_flags(dev, subd);
		if(flags & SDF_INTERNAL)continue;

		n_chan = comedi_get_n_channels(dev, subd);

		for(chan=0; chan<n_chan; chan++){
			obs_list[n_obs].subdevice = subd;
			obs_list[n_obs].chanspec = CR_PACK(chan,0,0);
			sprintf(obs_list[n_obs].name,"%3s %01d:%01d:%02d",
				subd_type_abbr[type],
				0,subd,chan);
			switch(type){
			case COMEDI_SUBD_AI:
				if(chan<8){
					obs_list[n_obs].chanspec = CR_PACK(chan,0,AREF_OTHER);
					strcat(obs_list[n_obs].name," oth");
				}else{
					strcat(obs_list[n_obs].name," gnd");
				}
				if(chan<8){
					obs_list[n_obs].observe = observe_ai3;
				}else{
					obs_list[n_obs].observe = observe_ai2;
				}
				break;
			case COMEDI_SUBD_DO:
			case COMEDI_SUBD_DI:
			case COMEDI_SUBD_DIO:
				obs_list[n_obs].observe = observe_dio;
				chan = n_chan;
				break;
			default:
				obs_list[n_obs].observe = observe_misc;
				break;
			}
			n_obs++;
		}
	}
}


void observe_ai(int i)
{
	char s[80];
	lsampl_t data;
	int ret;

	ret = comedi_data_read(dev,obs_list[i].subdevice,
		CR_CHAN(obs_list[i].chanspec),
		CR_RANGE(obs_list[i].chanspec),
		CR_AREF(obs_list[i].chanspec),
		&data);
	if(ret<0){
		//sprintf(s,"err=%d",errno);
		strcpy(s,strerror(errno));
	}else{
		sprintf(s,"%d     ",data);
	}
	mvaddstr(row_field+i,col_data,s);
}

void observe_ai2(int i)
{
	char s[80];
	lsampl_t data;
	int ret;
	comedi_range *range;
	lsampl_t maxdata;
	double v;

	ret = comedi_data_read(dev,obs_list[i].subdevice,
		CR_CHAN(obs_list[i].chanspec),
		CR_RANGE(obs_list[i].chanspec),
		CR_AREF(obs_list[i].chanspec),
		&data);
	range = comedi_get_range(dev,obs_list[i].subdevice,
		CR_CHAN(obs_list[i].chanspec),
		CR_RANGE(obs_list[i].chanspec));
	maxdata = comedi_get_maxdata(dev,obs_list[i].subdevice,
		CR_CHAN(obs_list[i].chanspec));
	v = comedi_to_phys(data,range,maxdata);
	if(ret<0){
		//sprintf(s,"err=%d",errno);
		strcpy(s,strerror(errno));
	}else{
		if(isnan(v)){
			if(data==0){
				sprintf(s,"below     ");
			}else{
				sprintf(s,"above     ");
			}
		}else{
			sprintf(s,"%g     ",v);
		}
	}
	mvaddstr(row_field+i,col_data,s);
}

void observe_ai3(int i)
{
	new2_sv_t sv;
	char s[80];

	new2_sv_measure_order(&sv, dev, obs_list[i].subdevice,
		obs_list[i].chanspec, 6);

	sci_sprint_alt(s,sv.average,sv.error);
	strcat(s,"     ");

	mvaddstr(row_field+i,col_data,s);
}

void observe_misc(int i)
{
	char s[80];
	lsampl_t data;

	comedi_data_read(dev,obs_list[i].subdevice,
		obs_list[i].chanspec,0,0,&data);

	sprintf(s,"%d     ", data);
	mvaddstr(row_field+i,col_data,s);
}

void observe_dio(int i)
{
	char s[80];
	unsigned int data;
	int j;
	int ret;
	int shift = 2;
	int n = 8;

	ret = comedi_dio_bitfield(dev,obs_list[i].subdevice,0,&data);

	for(j=0;j<n+(n>>shift);j++){
		s[j]=' ';
	}
	s[j]=0;

	for(j=0;j<n;j++){
		s[j+(j>>shift)] = (data>>(n-1-j))?'1':'0';
	}
	mvaddstr(row_field+i,col_data,s);
}

void dump_chans(void)
{
	int i;

	for(i=0; i<n_obs; i++){
		obs_list[i].observe(i);
	}
}

void do_time(void)
{
	long t;
	char s[20];

	attrset(COLOR_PAIR(0));
	t = now.tv_sec;
	mvaddstr(row_time,col_time,asctime(localtime(&t)));
	mvaddstr(row_time,col_time+25,tzname[0]);

	sprintf(s,"+%06dus", (unsigned int)now.tv_usec);
	mvaddstr(row_time,col_time+29,s);
}

void update_display(void)
{
	attrset(COLOR_PAIR(0));
	do_time();
	dump_chans();

	move(row_info,0);
	refresh();
}

void init_display(void)
{
	int i;

	attrset(COLOR_PAIR(0));
	do_time();
	for(i=0; i<n_obs; i++){
		mvaddstr(row_field+i,col_channel,obs_list[i].name);
	}
	attron(A_REVERSE);
	move(row_info,0);
	for(i=0;i<80;i++)addch(' ');
	mvaddstr(row_info,col_channel,"channel");
	mvaddstr(row_info,col_data,"data");
	attroff(A_REVERSE);

	refresh();
}

int main(int argc, char *argv[])
{

	dev = comedi_open("/dev/comedi0");

	comedi_set_global_oor_behavior(COMEDI_OOR_NUMBER);

	init_nc();
	setup_chans();

	init_display();

	while(1){
		gettimeofday(&now, NULL);

		if(now.tv_sec >= nextsec){
			update_display();
			gettimeofday(&now, NULL);

			nextsec = now.tv_sec + 1;
		}

		usleep((nextsec-now.tv_sec)*1000 - now.tv_usec/1000);
	}

	return 0;
}


