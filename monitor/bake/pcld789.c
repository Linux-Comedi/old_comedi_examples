
#include <bake.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifndef PCL_DEV
#define PCL_DEV "/dev/comedi1"
#endif

/* for thermocouple amplifier PCLD-789 */

#define N_SAMPLES 100
#define PCLD789_GAIN 50

static char *pcld789_names[]={
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"15"
};

static comedi_t *pcl711;
static int cjc_tchan;
static int rh_tchan;

static int cjc_chan=1;
static int rh_chan=2;
static int tc_chan=0;
static int pcl_ad_subdev=0;
static int pcl_do_subdev=3;

static void update_cjc(temp_chan *it);
static void update_tc(temp_chan *it);
static void update_rh(temp_chan *it);
static double cjc_temp(double v);
static double volt_to_rh(double v);

static void atex(void)
{
	int i;

	for(i=0;i<4;i++)
		comedi_dio_write(pcl711,pcl_do_subdev,i+8,0);
}


void init_pcld789(void)
{
	int i;
	temp_chan it;
	
	pcl711=comedi_open(PCL_DEV);
	if(pcl711==NULL){
		fprintf(stderr,"open " PCL_DEV " failed\n");
		return;
		exit(1);
	}
		
	atexit(atex);
	
	for(i=0;i<16;i++){
		it.temp=0;
		it.update=update_tc;
		it.private=(void *)i;
		strncpy(it.name,pcld789_names[i],20);
		it.name[19]=0;
		new_temp_chan(&it);
	}
	
	it.temp=0;
	it.update=update_cjc;
	strncpy(it.name,"cold junction",20);
	it.name[19]=0;
	update_cjc(&it);
	cjc_tchan=new_temp_chan(&it);
	
	it.temp=0;
	it.update=update_rh;
	strncpy(it.name,"relative humidity",20);
	it.name[19]=0;
	update_rh(&it);
	rh_tchan=new_temp_chan(&it);
}

static double cjc_temp(double v)
{
	return v/24.4;
}

static double volt_to_rh(double v)
{
	return (v-0.876)/0.0302;
}

static void update_rh(temp_chan *it)
{
	double v;
	comedi_sv_t sv;

	comedi_sv_init(&sv,pcl711,pcl_ad_subdev,rh_chan);
	comedi_sv_measure(&sv,&v);

	v-=0.00273; /* fix board offset */

	it->temp=volt_to_rh(v);
}

static void update_cjc(temp_chan *it)
{
	double v;
	comedi_sv_t sv;

	comedi_sv_init(&sv,pcl711,pcl_ad_subdev,cjc_chan);
	comedi_sv_measure(&sv,&v);

	v-=0.00273; /* fix board offset */

	v*=1000;	/* to mV */
	it->temp=cjc_temp(v);
}

static void update_tc(temp_chan *it)
{
	double cjc_emf=thermoc_emf(temps[cjc_tchan].temp);
	double v=0;
	comedi_sv_t sv;
	int i;
	int ret;

	comedi_sv_init(&sv,pcl711,pcl_ad_subdev,tc_chan);


	for(i=0;i<4;i++){
		comedi_dio_write(pcl711,pcl_do_subdev,i,(((int)(it->private))>>i)&1);
	}

	/* delay hack -- reminder to get usleep working */
#if 0
	/* (2*3.22 ms)/25 us = 256 */
	sv.n=256;
	if((ret=comedi_sv_measure(&sv,&v))<0)
		printf("error=%d\n",ret);
	/*usleep(100);*/
#endif

	sv.n=100;
	if((ret=comedi_sv_measure(&sv,&v))<0)
		printf("error=%d\n",ret);

	v-=0.00273;	/* fix board offset */
	v*=1000;	/* to mV */

	it->temp=thermoc_temp(v/PCLD789_GAIN+cjc_emf);
	if(it->temp>1200)it->temp=NAN;
}

void bake(void)
{
	int i;
	
	for(i=0;i<4;i++)
		comedi_dio_write(pcl711,pcl_do_subdev,i+8,bakechans[i]);
}

