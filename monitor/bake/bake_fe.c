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

/* Latin-1 */
#define PLUS_MINUS_CHAR		0xb1
/* DOS (?) */
/*#define PLUS_MINUS_CHAR		0xf1*/

#define MAX_CHAN 30
temp_chan temps[MAX_CHAN];
int n_temps;

#define RECORD_LEN 3601  /* 3601=hack */
double temp_record[MAX_CHAN][RECORD_LEN];

static int row_time=0,
	row_info1=1,
	row_info2=2,
	row_temp=3,
	row_cmd=24,
	row_baketime=0;
static int col_name=0, /* (14) */
	col_fault=15,	/* <0000+00 (8) */
	col_set=24,	/* <0000+00 (8) */
	col_temp=33,	/* 0000.0  (6) */
	col_hilo=40,	/* AA (2) */
	col_ave0=43,	/* 0000.0  (6) */
	col_ave1=50,	/* 0000.0  (6) */
	col_ave2=57,	/* 0000.0  (6) */
	col_bake=70,	/* AAA (3) */
	col_baketime=60;

int avetemp[]={ 60, 600, 3600 };

void do_time(long t);
void switch_range(temp_chan *it);
void switch_limit(temp_chan *it);
void switch_none(temp_chan *it);
void display_range(temp_chan *it,int line);
void display_limit(temp_chan *it,int line);
void display_none(temp_chan *it,int line);

void bakelog(char *str);

#define SET_NONE 0
#define SET_LIMIT 1
#define SET_RANGE 2

typedef struct setpt_struct setpt;
struct setpt_struct{
	void (*update_setpt)(temp_chan *it);
	void (*display)(temp_chan *it,int line);
};

setpt setpts[]={
	{ switch_none, display_none },
	{ switch_limit, display_limit },
	{ switch_range, display_range }
};

#define TEMP_INVAL 0
#define TEMP_LO 1
#define TEMP_OK 2
#define TEMP_HI 3
#define TEMP_NONE 4

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


void init_bake(void)
{
	int i;
	
	for(i=0;i<n_temps;i++){
		temps[i].avetemp[0]=0;
		temps[i].avetemp[1]=0;
		temps[i].avetemp[2]=0;
		temps[i].firstgood=0;
	}
	read_conf_file();
	status_flag=STATUS_IDLE;
}

void read_conf_file(void)
{
	int f_tab;
	char ln[100];
	char *ss[5],*s;
	int len,n,i;
	FILE *conf;
	
	for(i=0;i<n_temps;i++){
		temps[i].set=0;
		temps[i].var=0;
		temps[i].set_type=SET_NONE;
		temps[i].fault=0;
		temps[i].faultable=0;
		temps[i].bakechan=-1;
	}
	
	conf=fopen("bake.conf","r");
	if(!conf){
		do_fault("can't open bake.conf");
		return;
	}
	
	do{
		fgets(ln,100,conf);
		if(feof(conf))break;
		if((s=strstr(ln,"#"))!=NULL)*s=0;
		for(i=strlen(ln)-1;i>=0 && isspace(ln[i]);--i)ln[i]=0;
		if(*ln){
			n=0; f_tab=1;
			len=strlen(ln);
			for(i=0;i<len;i++){
				if(ln[i]=='\t'){
					ln[i]=0;
					f_tab=1;
				}else if(f_tab){
					f_tab=0;
					ss[n]=ln+i;
					n++;
					if(n==5)break;
				}
			}
			for(;n<5;n++)ss[n]=ln+i;
			if(sscanf(ss[0],"%d",&i)==1 && i>=0 && i<n_temps){
				strncpy(temps[i].name,ss[1],19);
				n=sscanf(ss[2],"%lg/%lg",&temps[i].set,
				  &temps[i].var);
				if(n==0)temps[i].set_type=SET_NONE;
				if(n==1)temps[i].set_type=SET_LIMIT;
				if(n==2)temps[i].set_type=SET_RANGE;
				n=sscanf(ss[3],"%lg",&temps[i].fault);
				if(n==0)temps[i].faultable=0;
				if(n==1)temps[i].faultable=1;
				n=sscanf(ss[4],"%d",&temps[i].bakechan);
			}else{
				do_fault("bad line in bake.conf");
				return;
			}
		}
	}while(!feof(conf));
}

int runsec=0,nextsec,startsec,endbakesec;
struct timeval now;

FILE *logfile;

int main(int argc,char *argv[])
{
	int c;

	gettimeofday(&now,NULL);
	startsec=now.tv_sec;
	nextsec=startsec+1;

	logfile=fopen("bakelog","a");
	bakelog("initialization");

	/*init_jcd();*/
	init_pcld789();
	init_nc();
	init_bake();
	init_display();
	
	while(1){
		c=get_a_char();
		switch(c){
		case 'i':
		case 's':
			strcpy(status_str,"idle");
			bakelog("idle");
			status_flag=STATUS_IDLE;
			update_status();
			break;
		case 'b':
			strcpy(status_str,"baking");
			bakelog("bake");
			status_flag=STATUS_BAKING;
			update_status();
			endbakesec=nextsec+bake_hours*3600;
			break;
		case 'r':
			read_conf_file();
			status_flag=STATUS_IDLE;
			update_status();
			init_display();
			break;
		case 'q':
			exit(0);
		case KEY_UP:
			bake_hours++;
			update_bakehours();
			break;
		case KEY_DOWN:
			bake_hours--;
			update_bakehours();
			break;
		case 'L'-64:
			wrefresh(curscr);
			break;
		default:
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

	sprintf(time_str+strlen(s)-1," +%06dus",now.tv_usec);
}

void bakelog(char *str)
{
	char time_str[40];

	print_time(time_str);
	fprintf(logfile,"%s: %s\n",time_str,str);
	fflush(logfile);
}

int get_a_char(void)
{
	int c;
	
	while(1){
		gettimeofday(&now,NULL);
		if(now.tv_sec>=nextsec){
			if(now.tv_sec>nextsec)do_fault("second loss");
			if(now.tv_sec>endbakesec && status_flag==STATUS_BAKING)do_endbake();
		
			update_temps();
			update_bake();
			bake();
			update_display();
			runsec++;
			nextsec++;
			gettimeofday(&now,NULL);
		}
		//timeout((nextsec-now.tv_sec)*1000-now.tv_usec/1000);
		if((c=getch())!=ERR)return c;
	}
}

void update_temps(void)
{
	int i,j;
	char s[20];
	char log_line[1000],*ln;
	int len;

	ln=log_line;
	sprintf(ln,"temps: ");
	ln+=7;
	for(i=0;i<n_temps;i++){
		temps[i].update(temps+i);
		if(temps[i].faultable && isnan(temps[i].temp)){
			sprintf(s,"%d invalid",i);
			do_fault(s);
		}
		if(temps[i].faultable && temps[i].temp>=temps[i].fault){
			sprintf(s,"%d overtemp",i);
			do_fault(s);
		}
		temp_record[i][runsec%RECORD_LEN]=temps[i].temp;
		if(isnan(temps[i].temp)){
			temps[i].firstgood=runsec+1;
			for(j=0;j<3;j++)temps[i].avetemp[j]=0;
		}else for(j=0;j<3;j++){
			temps[i].avetemp[j]+=temps[i].temp/avetemp[j];
			if(temps[i].firstgood+avetemp[j]<=runsec)
			    temps[i].avetemp[j]-=
				temp_record[i][(runsec-avetemp[j])%RECORD_LEN]/avetemp[j];
		}
		setpts[temps[i].set_type].update_setpt(temps+i);
		sprintf(ln,"%.1f %n",temps[i].temp,&len);
		ln+=len;
	}
	bakelog(log_line);
}

void update_display(void)
{
	int i;
	
	for(i=0;i<n_temps;i++){
		attrset(COLOR_PAIR(0));
		print_temp(temps[i].temp,row_temp+i,col_temp);
		attrset(COLOR_PAIR(temp_colors[temps[i].hilo]));
		mvaddstr(row_temp+i,col_hilo,temp_codes[temps[i].hilo]);
	}
	attrset(COLOR_PAIR(0));
	for(i=0;i<n_temps;i++){
		if(temps[i].firstgood+avetemp[0]<=runsec)
			print_temp(temps[i].avetemp[0],row_temp+i,col_ave0);
		else mvaddstr(row_temp+i,col_ave0,"      ");
		if(temps[i].firstgood+avetemp[1]<=runsec)
			print_temp(temps[i].avetemp[1],row_temp+i,col_ave1);
		else mvaddstr(row_temp+i,col_ave1,"      ");
		if(temps[i].firstgood+avetemp[2]<=runsec)
			print_temp(temps[i].avetemp[2],row_temp+i,col_ave2);
		else mvaddstr(row_temp+i,col_ave2,"      ");
	}
	for(i=0;i<4;i++){
		mvaddstr(row_temp+i,col_bake,(bakechans[i])?"ON ":"OFF");
	}
	do_time(now.tv_sec);
	
	move(24,0);
	refresh();
}

void init_display(void)
{
	int i;
	
	mvaddstr(row_info1,col_name,"status: ");
	addstr(status_str);
	attron(A_REVERSE);
	move(row_info2,0);
	for(i=0;i<80;i++)addch(' ');
	mvaddstr(row_info2,col_name,"sensor");
	mvaddstr(row_info2,col_fault," fault");
	mvaddstr(row_info2,col_set," set pt");
	mvaddstr(row_info2,col_temp,"  temp");
	mvaddstr(row_info2,col_hilo,"  ");
	print_sec(row_info2,col_ave0,avetemp[0]);
	print_sec(row_info2,col_ave1,avetemp[1]);
	print_sec(row_info2,col_ave2,avetemp[2]);
	attroff(A_REVERSE);
	for(i=0;i<n_temps;i++){
		mvaddstr(row_temp+i,col_name,temps[i].name);
		setpts[temps[i].set_type].display(temps+i,row_temp+i);
		if(temps[i].faultable)display_fault(temps+i,row_temp+i);
	}
	mvaddstr(row_cmd,40-strlen(cmd_str)/2,cmd_str);
	refresh();
}

int new_temp_chan(temp_chan *it)
{
	temps[n_temps]=*it;
	return n_temps++;
}

void do_time(long t)
{
	char s[20];
	
	/*t-=7*3600;*/  /* hack correction for GMT->PDT */
	attrset(COLOR_PAIR(0));
	mvaddstr(row_time,0,asctime(localtime(&t)));
	mvaddstr(row_time,0+25,tzname[0]);
	
	sprintf(s,"+%06dus",now.tv_usec);
	mvaddstr(row_time,0+29,s);

	if(status_flag==STATUS_BAKING){
		seconds_to_str(s,endbakesec-nextsec);
		mvaddstr(row_time,40,"bake time remaining: ");
		mvaddstr(row_time,61,s);
	}else{
		sprintf(s,"bake interval %dh ",bake_hours);
		mvaddstr(row_baketime,col_baketime,s);
	}
}

void switch_range(temp_chan *it)
{
	if(isnan(it->temp))it->hilo=TEMP_INVAL;
	else if(it->temp>it->set+it->var)
		it->hilo=TEMP_HI;
	else if(it->temp<it->set-it->var)
		it->hilo=TEMP_LO;
	else it->hilo=TEMP_OK;
}

void switch_limit(temp_chan *it)
{
	if(isnan(it->temp))it->hilo=TEMP_INVAL;
	else if(it->temp>it->set)
		it->hilo=TEMP_HI;
	else it->hilo=TEMP_OK;
}

void switch_none(temp_chan *it)
{
	it->hilo=TEMP_NONE;
}

void display_range(temp_chan *it,int line)
{
	char s[10];
	
	sprintf(s," %4.0f",it->set);
	mvaddstr(line,col_set,s);
	addch(PLUS_MINUS_CHAR);
	sprintf(s,"%2.0f",it->var);
	addstr(s);
}

void display_limit(temp_chan *it,int line)
{
	char s[10];
	
	sprintf(s,"<%4.0f",it->set);
	mvaddstr(line,col_set,s);
}

void display_none(temp_chan *it,int line)
{
}

void print_temp(double temp,int row,int col)
{
	char s[10];
	
	if(isnan(temp))mvaddstr(row,col,"  - - ");
	else{
		sprintf(s,"%6.1f",temp);
		mvaddstr(row,col,s);
	}
}

void seconds_to_str(char *s,int sec)
{
	int i;
	
	if(sec<60){
		sprintf(s,"%02ds",sec);
	}else if(sec<60*60){
		sprintf(s,"%02dm%02ds",sec/60,sec%60);
	}else if(sec<24*60*60){
		sprintf(s,"%dh%02dm",sec/3600,(sec/60)%60);
	}else{
		sprintf(s,"%dd%dh",sec/86400,(sec/3600)%24);
	}
	for(i=strlen(s);i<6;i++)s[i]=' ';
	s[i]=0;
}

void print_sec(int row,int col,int sec)
{
	char s[20];
	
	seconds_to_str(s,sec);
	mvaddstr(row,col,s);
}

void display_fault(temp_chan *it,int line)
{
	char s[10];
	
	sprintf(s,"<%4.0f",it->fault);
	mvaddstr(line,col_fault,s);
}

void do_fault(const char *s)
{
	sprintf(status_str,"fault: %s",s);
	bakelog(status_str);
	status_flag=STATUS_FAULTED;
	update_status();
}

void update_status(void)
{
	mvaddstr(row_info1,col_name,"status: ");
	clrtoeol();
	addstr(status_str);
	refresh();
}

void update_bake(void)
{
	int i;
	
	if(status_flag==STATUS_BAKING){
		for(i=0;i<4;i++){
			if(temps[i].bakechan>=0 && temps[i].hilo==TEMP_LO)
				bakechans[temps[i].bakechan]=1;
		}
		for(i=0;i<4;i++){
			if(temps[i].bakechan>=0 && temps[i].hilo==TEMP_HI)
				bakechans[temps[i].bakechan]=0;
		}
	}else{
		for(i=0;i<4;i++)bakechans[temps[i].bakechan]=0;
	}
}

void do_endbake(void)
{
	long t=now.tv_sec;
	
	sprintf(status_str,"stopped at %s",asctime(localtime(&t)));
	bakelog(status_str);
	status_flag=STATUS_IDLE;
	update_status();
}

void update_bakehours(void)
{
	char s[20];
	
	if(bake_hours<0)bake_hours=0;
	sprintf(s,"bake interval %dh ",bake_hours);
	mvaddstr(row_baketime,col_baketime,s);
	/*clrtoeol();*/
	refresh();
}

