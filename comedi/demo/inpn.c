/*
 * Multi-channel, multi-range one-shot input demo
 * Part of Comedilib
 *
 * Copyright (c) 1999,2000 David A. Schleef <ds@schleef.org>
 *
 * This file may be freely modified, distributed, and combined with
 * other software, as long as proper attribution is given in the
 * source code.
 */
/*
   This demo opens /dev/comedi0 and looks for an analog input
   subdevice.  If it finds one, it measures one sample on each
   channel for each input range.  The value NaN indicates that
   the measurement was out of range.
 */

#include <comedilib.h>
#include "examples.h"

comedi_t *device;


int main(int argc, char *argv[])
{
	int n_chans,chan;
	int n_ranges;
	int range;
	int maxdata;
	lsampl_t data;
	double voltage;

	parse_options(argc,argv);

	device=comedi_open(filename);
	if(!device){
		comedi_perror(filename);
		exit(0);
	}

	subdevice=comedi_find_subdevice_by_type(device,COMEDI_SUBD_AI,0);
	if(subdevice<0){
		printf("no analog input subdevice found\n");
		exit(0);
	}

	n_chans=comedi_get_n_channels(device,subdevice);
	for(chan=0;chan<n_chans;chan++){
		printf("%d: ",chan);

		n_ranges=comedi_get_n_ranges(device,subdevice,chan);

		maxdata=comedi_get_maxdata(device,subdevice,chan);
		for(range=0;range<n_ranges;range++){
			comedi_data_read(device,subdevice,chan,range,aref,&data);
			voltage=comedi_to_phys(data,comedi_get_range(device,subdevice,chan,range),maxdata);
			printf("%g ",voltage);
		}
		printf("\n");
	}
	
	exit(0);
}

