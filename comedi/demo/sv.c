/*
   A little auto-calibration utility, for boards
   that support it.

   Right now, it only supports NI E series boards,
   but it should be easily portable.

   A few things need improvement here:
    - current system gets "close", but doesn't
      do any fine-tuning
    - no pre/post gain discrimination for the
      A/D zero offset.
    - should read (and use) the actual reference
      voltage value from eeprom
    - statistics would be nice, to show how good
      the calibration is.
    - doesn't check unipolar ranges
    - "alternate calibrations" would be cool--to
      accurately measure 0 in a unipolar range
    - more portable
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <comedilib.h>
#include <stdlib.h>


typedef struct{
	comedi_t *dev;
	int subd;
	int chan;
	int range;
	int aref;

	lsampl_t *data;
	comedi_insn insn;

	int maxdata;
	int order;

	comedi_range *rng;

	double current_ave;
	double current_var2;
	int current_n;

	double chunk_ave;
	double chunk_var2;
	int chunk_n;
}new_sv_t;

int sv_measure(comedi_t *dev,int subdev,int chan,int range,int aref,
	double *result,double *error);

int _sv_init(new_sv_t *sv,comedi_t *dev,int subdev,int chan,int range,int aref);
int _sv_read_chunk(new_sv_t *sv);
int _sv_calc_chunk_stats(new_sv_t *sv);


int main(void)
{
	int i;
	double res,err;
	comedi_t *dev;
	int subdev = 0;
	int chan = 0;
	int n_ranges;

	dev = comedi_open("/dev/comedi0");

	n_ranges = comedi_get_n_ranges(dev,subdev,chan);
	for(i=0;i<n_ranges;i++){
		sv_measure(dev,subdev,chan,i,AREF_OTHER,&res,&err);
		printf("%g %g\n",res,err);
	}

	return 0;
}

#define CHUNK_ORDER 7
#define CHUNK_SIZE (1<<CHUNK_ORDER)

int sv_measure(comedi_t *dev,int subdev,int chan,int range,int aref,
	double *result,double *error)
{
	new_sv_t sv;

	_sv_init(&sv,dev,subdev,chan,range,aref);

	while(sv.current_n < 1024){
		_sv_read_chunk(&sv);
		_sv_calc_chunk_stats(&sv);
		_sv_merge_chunk(&sv);
	}
}

int _sv_init(new_sv_t *sv,comedi_t *dev,int subdev,int chan,int range,int aref)
{
	memset(sv,0,sizeof(*sv));

	sv->dev = dev;
	sv->subd = subdev;
	sv->chan = chan;
	sv->range = range;
	sv->aref = aref;

	sv->data = malloc(sizeof(lsampl_t)*CHUNK_SIZE);

	sv->insn.insn = INSN_READ;
	sv->insn.n = CHUNK_SIZE;
	sv->insn.data = sv->data;
	sv->insn.subdev = subdev;
	sv->insn.chanspec = CR_PACK(chan,range,aref);
	/* enable dithering */
	sv->insn.chanspec |= (1<<26);

	sv->maxdata=comedi_get_maxdata(dev,subdev,chan);
	sv->rng=comedi_get_range(dev,subdev,chan,range);

	sv->current_ave = 0;
	sv->current_var2 = 0;
	sv->current_n = 0;

	return 0;
}

int _sv_read_chunk(new_sv_t *sv)
{
	int ret;

	ret = comedi_do_insn(sv->dev,&sv->insn);
	if(ret<0)return ret;

	if(ret<sv->insn.n){
		printf("insn barfed\n");
		return -1;
	}

	return 0;
}

int _sv_calc_chunk_stats(new_sv_t *sv)
{
	double a,s2,s,x;
	int i,n;

	n = CHUNK_SIZE;

	s=0;
	s2=0;
	a=comedi_to_phys(sv->data[0],sv->rng,sv->maxdata);
	for(i=0;i<n;i++){
		x=comedi_to_phys(sv->data[i],sv->rng,sv->maxdata);
		s+=x-a;
		s2+=(x-a)*(x-a);
	}
	sv->chunk_sum=n*a+s;
	sv->chunk_var2=n*s2-s*s;
	sv->chunk_n = n;

	return 0;
}

int _sv_merge_chunk(new_sv_t *sv)
{
	int n;

	n = sv->current_n + sv->chunk_n;
	sv->current_sum = sv->current_sum + sv->chunk_sum;
	sv->current_var = 0; /* more complicated */

	return 0;
}

#if 0
int comedi_data_read_n(comedi_t *it,unsigned int subdev,unsigned int chan,
	unsigned int range,unsigned int aref,lsampl_t *data,unsigned int n)
{
	comedi_insn insn;
	int ret;

	if(n==0)return 0;

	insn.insn = INSN_READ;
	insn.n = n;
	insn.data = data;
	insn.subdev = subdev;
	insn.chanspec = CR_PACK(chan,range,aref);
	/* enable dithering */
	insn.chanspec |= (1<<26);
	
	ret = comedi_do_insn(it,&insn);

	if(ret>0)return n;

	printf("insn barfed: subdev=%d, chan=%d, range=%d, aref=%d, "
		"n=%d, ret=%d, %s\n",subdev,chan,range,aref,n,ret,
		strerror(errno));

	return ret;
}
#endif

#if 0
int new_sv_measure(new_sv_t *sv)
{
	lsampl_t *data;
	int n,i,ret;

	double a,x,s,s2;

	n=1<<sv->order;

	data=malloc(sizeof(lsampl_t)*n);
	if(data == NULL)
	{
		perror("comedi_calibrate");
		exit(1);
	}

	for(i=0;i<n;){
		ret = comedi_data_read_n(dev,sv->subd,sv->chan,sv->range,
			sv->aref,data+i,n-i);
		if(ret<0){
			printf("barf\n");
			goto out;
		}
		i+=ret;
	}

	s=0;
	s2=0;
	a=comedi_to_phys(data[0],sv->rng,sv->maxdata);
	for(i=0;i<n;i++){
		x=comedi_to_phys(data[i],sv->rng,sv->maxdata);
		s+=x-a;
		s2+=(x-a)*(x-a);
	}
	sv->average=a+s/n;
	sv->stddev=sqrt(n*s2-s*s)/n;
	sv->error=sv->stddev/sqrt(n);

	ret=n;

out:
	free(data);

	return ret;
}
#endif

#if 0
int new_sv_measure_order(new_sv_t *sv,int order)
{
	lsampl_t *data;
	int n,i,ret;
	double a,x,s,s2;

	n=1<<order;

	data=malloc(sizeof(lsampl_t)*n);
	if(data == NULL)
	{
		perror("comedi_calibrate");
		exit(1);
	}

	for(i=0;i<n;){
		ret = comedi_data_read_n(dev,sv->subd,sv->chan,sv->range,
			sv->aref,data+i,n-i);
		if(ret<0){
			printf("barf order\n");
			goto out;
		}
		i+=ret;
	}

	s=0;
	s2=0;
	a=comedi_to_phys(data[0],sv->rng,sv->maxdata);
	for(i=0;i<n;i++){
		x=comedi_to_phys(data[i],sv->rng,sv->maxdata);
		s+=x-a;
		s2+=(x-a)*(x-a);
	}
	sv->average=a+s/n;
	sv->stddev=sqrt(n*s2-s*s)/n;
	sv->error=sv->stddev/sqrt(n);

	ret=n;

out:
	free(data);

	return ret;
}
#endif


