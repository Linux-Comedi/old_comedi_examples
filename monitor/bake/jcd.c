
#include <bake.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#ifndef JCD_DEV
#define JCD_DEV "/dev/comedi0"
#endif

#define CJC_NAME "cjc"
static char *names[]={ "1", "2", "3", "4", "5", "6" };
#define TC_NAME(a) names[(a)]

/* for thermocouple reader designed by JCD */

static int in;
static comedi_voltage_info vinfo;

static int in_chan=7,out_chan=18;
static int cjc_tchan;

static void update_tc(temp_chan *it);
static void update_cjc(temp_chan *it);
static double comedi_nget(int in,int chan,int n,
	comedi_voltage_info vinfo,double *vv);
static double update_gnd(void);

void init_jcd(void)
{
	int ir,i;
	temp_chan it;
	
	in=open(JCD_DEV,O_RDWR);
	if(in<0){
		fprintf(stderr,"open " JCD_DEV " failed\n");
		return;
		exit(1);
	}
	
	comedi_setparam(in,in_chan,COMEDI_AREF,COMEDI_AREF_DIFF);
	
	ir=1;
	comedi_setparam(in,in_chan,COMEDI_RANGE,ir);
	comedi_voltage_range(in,in_chan,ir,&vinfo);
	
	comedi_setparam(in,out_chan,COMEDI_IOBITS,0xff);
	
	it.tc=1.0;
	it.temp=0;
	it.update=update_cjc;
	update_cjc(&it);
	it.tc=0.1;
	it.name=CJC_NAME;
	cjc_tchan=new_temp_chan(&it);
	
	for(i=0;i<6;i++){
		it.tc=1.0;
		it.temp=0;
		it.update=update_tc;
		it.private=(void *)i;
		it.name=TC_NAME(i);
		new_temp_chan(&it);
	}
}

static double cjc_temp(double v)
{
	return v/70.3893*294.15-273.15;
}

static void update_cjc(temp_chan *it)
{
	double gnd=update_gnd();
	double v,vv;
	
	comedi_masked_put(in,out_chan,7,7);
	v=1000*comedi_nget(in,in_chan,100,vinfo,&vv);
	it->temp=(1-it->tc)*it->temp+it->tc*cjc_temp((v-gnd)/10);
	
}

static double update_gnd(void)
{
	double v,vv;

	comedi_masked_put(in,out_chan,7,6);
	v=1000*comedi_nget(in,in_chan,100,vinfo,&vv);
	return v;
}

static void update_tc(temp_chan *it)
{
	double gnd=update_gnd();
	double cjc_emf=thermoc_emf(temps[cjc_tchan].temp);
	double v,vv;
	
	comedi_masked_put(in,out_chan,7,(int)(it->private));
	v=1000*comedi_nget(in,in_chan,100,vinfo,&vv);
	it->temp=thermoc_temp((v-gnd)/100+cjc_emf);
	if(it->temp>1200)it->temp=NAN;
}


static double comedi_nget(int in,int chan,int n,
	comedi_voltage_info vinfo,double *vv)
{
	double tot,t2,x,bit;
	comedi_trig it;
	comedi_sample s;
	int r,i;
	
	it.chan=chan;
	it.io=COMEDI_IO_INPUT;
	it.n=n;
	it.trigsrc=COMEDI_TRIGCLK;
	it.trigvar=19999;

	r=ioctl(in,COMEDI_TRIG,&it);
	if(r<0)printf("ioctl errno=%d\n",errno);

	tot=0;
	t2=0;
	for(i=0;i<n;i++){
		read(in,&s,sizeof(comedi_sample));
		x=vinfo.offset+vinfo.delta*s.data;
		if(s.data==4095 || s.data==0)x=NAN;
		tot+=x;
		t2+=x*x;
		/*printf("%d %d\n",i,s.data);*/
	}
	t2/=n;
	tot/=n;
	bit=vinfo.delta;
	if(t2-tot*tot<bit*bit)*vv=bit;
	else *vv=sqrt((t2-tot*tot+bit*bit)/n);
	
	return tot;
}


