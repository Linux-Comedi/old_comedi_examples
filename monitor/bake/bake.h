
#ifndef _IT_H_
#define _IT_H_

#include <comedi.h>
#include <comedilib.h>

typedef struct temp_chan_struct temp_chan;
struct temp_chan_struct{
	char name[20];
	void (*update)(temp_chan *);
	void *private;
	double temp;
	double tc;
	int set_type;
	double set,var;
	double fault;
	int faultable;
	int hilo;
	double avetemp[3];
	int firstgood;
	int bakechan;
};
extern temp_chan temps[];

int new_temp_chan(temp_chan *it);

double thermoc_emf(double);
double thermoc_temp(double);

void init_nc(void);
void init_pcld789(void);
void init_jcd(void);

void bake(void);

extern int bakechans[];

#endif

