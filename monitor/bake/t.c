
#include <bake.h>
#include <math.h>


void update_tc(temp_chan *it)
{
	double gnd=update_gnd();
	double cjc_emf=thermoc_emf(temps[CJC_CHAN].temp);
	double v,vv;
	
	comedi_put(in,18,digout|(int)(it->private));
	v=1000*comedi_nget(in,INPUT_CHANNEL,100,vinfo,&vv);
	it->temp=thermoc_temp((v-gnd)/100+cjc_emf);
	if(it->temp>1200)it->temp=NAN;
}


