
#ifndef _SV_H_
#define _SV_H_

struct new2_sv_struct{
	double average;
	double stddev;
	double error;
};

typedef struct new2_sv_struct new2_sv_t;

int new2_sv_measure_order(new2_sv_t *sv, comedi_t *dev, unsigned int subd,
	unsigned int chanspec, unsigned int order);

#endif

