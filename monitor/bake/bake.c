#include <stdio.h>
#include <comedilib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <math.h>
#include <bake.h>
#include <ncurses.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>

#include <time.h>

#define MAX_CHAN 30
temp_chan temps[MAX_CHAN];
int n_temps;

#define RECORD_LEN 3601  /* 3601=hack */
double temp_record[MAX_CHAN][RECORD_LEN];

int avetemp[]={ 60, 600, 3600 };

void do_time(long t);
void switch_range(temp_chan *it);
void switch_limit(temp_chan *it);
void switch_none(temp_chan *it);
void display_range(temp_chan *it,int line);
void display_limit(temp_chan *it,int line);
void display_none(temp_chan *it,int line);

void bakelog(char *str);

#define STATUS_IDLE 0
#define STATUS_BAKING 1
#define STATUS_FAULTED 2

char cmd_str[]="b=bake  s=stop baking  r=reload bake.conf  q=quit program";
char *temp_codes[]={ "--", "LO", "OK", "HI", "  " };
int temp_colors[]={ 0, 2, 0, 1, 0 };
char status_str[80];
int status_flag;
int bakechans[4];
int bake_hours=8;

void print_temp(double temp,int row,int col);
void seconds_to_str(char *s,int sec);
void print_sec(int row,int col,int sec);
void display_fault(temp_chan *it,int line);
void init_display(void);
void update_temps(void);
void update_display(void);
void update_bakehours(void);
int get_a_char(void);
void do_fault(const char *s);
void update_status(void);
void update_bake(void);
void do_endbake(void);
void read_conf_file(void);


int runsec=0,nextsec,startsec;
struct timeval now;

FILE *logfile;

int main(int argc,char *argv[])
{
	gettimeofday(&now,NULL);
	startsec=now.tv_sec;
	nextsec=startsec+1;

	logfile=fopen("bakelog","a");
	bakelog("initialization");

	/*init_jcd();*/
	init_pcld789();
	
	while(1){
		gettimeofday(&now,NULL);
		if(now.tv_sec>=nextsec){
			if(now.tv_sec>nextsec)do_fault("second loss");
		
			update_temps();
			runsec++;
			nextsec++;
			gettimeofday(&now,NULL);
		}
	}

	return 0;
}

void print_time(char *time_str)
{
	char *s;
	long t=now.tv_sec;

	s=asctime(localtime(&t));
	strncpy(time_str,s,strlen(s)-1);
	time_str[strlen(s)-1]=0;
}

void bakelog(char *str)
{
	char time_str[40];

	print_time(time_str);
	fprintf(logfile,"%s: %s\n",time_str,str);
	fflush(logfile);
}

void update_temps(void)
{
	int i;
	char log_line[1000],*ln;
	int len;

	ln=log_line;
	sprintf(ln,"temps: ");
	ln+=7;
	for(i=0;i<n_temps;i++){
		temps[i].update(temps+i);
		sprintf(ln,"%.1f %n",temps[i].temp,&len);
		ln+=len;
	}
	bakelog(log_line);
}

int new_temp_chan(temp_chan *it)
{
	temps[n_temps]=*it;
	return n_temps++;
}

void do_fault(const char *s)
{
	sprintf(status_str,"fault: %s",s);
	bakelog(status_str);
	status_flag=STATUS_FAULTED;
}


