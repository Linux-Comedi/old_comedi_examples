

#include <stdio.h>
#include <comedilib.h>



typedef struct obs_struct obs;

struct obs_struct{
	unsigned int subdevice;
	unsigned int chanspec;
};

obs obs_list[100];
int n_obs;

comedi_t *dev;


void setup_chans(void);
void dump_chans(void);

int main(int argc, char *argv[])
{

	dev = comedi_open("/dev/comedi0");

	setup_chans();

	dump_chans();


	return 0;
}


void setup_chans(void)
{
	int subd;
	int n_subd;
	int n_chan;
	int type;
	int chan;
	int flags;

	n_subd = comedi_get_n_subdevices(dev);

	for(subd=0; subd<n_subd; subd++){
		type = comedi_get_subdevice_type(dev, subd);
		flags = comedi_get_subdevice_flags(dev, subd);
		if(flags & SDF_INTERNAL)continue;

		n_chan = comedi_get_n_channels(dev, subd);

		for(chan=0; chan<n_chan; chan++){
			obs_list[n_obs].subdevice = subd;
			obs_list[n_obs].chanspec = CR_PACK(chan,0,0);
			n_obs++;
		}
	}
}


void dump_chans(void)
{
	int i;

	for(i=0; i<n_obs; i++){
		printf("subd=%d chan=%d\n",
			obs_list[i].subdevice,
			CR_CHAN(obs_list[i].chanspec));

	}
}

