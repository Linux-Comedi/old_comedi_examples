
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

int bakechans[4];

void do_time(long t);

typedef struct setpt_struct setpt;
struct setpt_struct{
	void (*update_setpt)(temp_chan *it);
	void (*display)(temp_chan *it,int line);
};

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


int runsec=0,nextsec,startsec,endbakesec;
struct timeval now;

int main(int argc,char *argv[])
{
	int c;

	init_pcld789();
	init_nc();
	
	gettimeofday(&now,NULL);
	startsec=now.tv_sec;
	nextsec=startsec+1;
	
	while(1){
		c=get_a_char();
		switch(c){
		case 'q':
			exit(0);
		case 'L'-64:
			wrefresh(curscr);
			break;
		default:
		}
	}

	return 0;
}

int get_a_char(void)
{
	int c;
	
	while(1){
		gettimeofday(&now,NULL);
		if(now.tv_sec>=nextsec){
			update_temps();
			update_display();
			runsec++;
			nextsec++;
			gettimeofday(&now,NULL);
		}
		timeout(1000-now.tv_usec/1000);
		if((c=getch())!=ERR)return c;
	}
}

void update_temps(void)
{
#if 0
	int i,j;
	char s[20];
#endif
	
	temps[9].update(temps+9);
	temps[16].update(temps+16);
}

void update_display(void)
{
	move(24,0);
	refresh();
}

void init_display(void)
{
	refresh();
}

int new_temp_chan(temp_chan *it)
{
	temps[n_temps]=*it;
	return n_temps++;
}

void do_time(long t)
{
	/*t-=7*3600;*/  /* hack correction for GMT->PDT */
	attrset(COLOR_PAIR(0));
	mvaddstr(40,0,asctime(localtime(&t)));
	
}


