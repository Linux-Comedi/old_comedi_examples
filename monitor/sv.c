

#include <stdlib.h>
#include <comedilib.h>
#include <sv.h>
#include <math.h>


int comedi_data_read_n(comedi_t *dev, unsigned int subd, unsigned int
	chan, unsigned int range, unsigned int aref, lsampl_t *data,
	unsigned int n)
{
	int i;

	for(i=0;i<n;i++){
		comedi_data_read(dev,subd,chan,range,aref,data+i);
	}

	return i;
}

int new2_sv_measure_order(new2_sv_t *sv, comedi_t *dev, unsigned int subd,
	unsigned int chanspec, unsigned int order)
{
	lsampl_t *data;
	int n;
	int ret;
	comedi_range *range;
	lsampl_t maxdata;
	double a,s,s2,x;
	int i;

	n = 1<<order;
	data = malloc(sizeof(lsampl_t)*n);

	ret = comedi_data_read_n(dev, subd,
		CR_CHAN(chanspec),
		CR_RANGE(chanspec),
		CR_AREF(chanspec),
		data, n);

	range = comedi_get_range(dev, subd, CR_CHAN(chanspec),
			CR_RANGE(chanspec));
	maxdata = comedi_get_maxdata(dev, subd, CR_CHAN(chanspec));
	
	s = 0;
	s2 = 0;
	a = comedi_to_phys(data[0], range, maxdata);
	for(i=0;i<n;i++){
		x = comedi_to_phys(data[i], range, maxdata);
		s += x - a;
		s2 += (x-a)*(x-a);
	}

	sv->average = a + s/n;
	sv->stddev = sqrt(n*s2 - s*s)/n;
	sv->error = sv->stddev/sqrt(n);

	free(data);

	return 0;
}


