/*
 * This is a little helper function to parse options that
 * are common to most of the examples.
 */

//#include <stdio.h>
#include <comedilib.h>
//#include <fcntl.h>
//#include <unistd.h>
//#include <errno.h>
//#include <getopt.h>
//#include <ctype.h>
//#include <malloc.h>
//#include <string.h>
//#include <stdlib.h>
#include "examples.h"


char *filename="/dev/comedi0";
int verbose;

int value=0;
int subdevice=0;
int channel=0;
int aref=AREF_GROUND;
int range=0;
int n_chan=4;
int n_scan=1000;
double freq=1000.0;

#if 0
static void usage(const char *prog)
{
    printf ("Usage: %s [options] [value]\n", (prog) ? prog : "Program");
    printf ("Options are as follows:  (some options may be ignored by this program)\n");
    printf ("-f Path   Filename of comedi device to open (default /dev/comedi0).\n");
    printf ("-s Num    Subdevice number (default 0).\n");
    printf ("-c Num    First Channel number (default 0).\n");
    printf ("-r Num    Input Range (default 0).\n");
    printf ("-n Num    Number of channels (default 4).\n");
    printf ("-N Num    Number of scans (default 1000).\n");
    printf ("-F Num    Frequency (default 1000).\n");
    printf ("-v        Verbose.\n");
    printf ("-d        Analog reference: Differential.\n");
    printf ("-m        Analog reference: Common.\n");
    printf ("-g        Analog reference: Ground.\n");
    printf ("-o        Analog reference: Other.\n");
    printf ("-a Num    Analog reference (as a number).\n");
    printf ("-h        Print this Help.\n");
}
#endif

int parse_options(int argc, char *argv[])
{
#if 0
	int c;


	while (-1 != (c = getopt(argc, argv, "a:c:s:r:f:n:N:F:vdgomh"))) {
		switch (c) {
		case 'f':
			filename = optarg;
			break;
		case 's':
			subdevice = strtoul(optarg,NULL,0);
			break;
		case 'c':
			channel = strtoul(optarg,NULL,0);
			break;
		case 'a':
			aref = strtoul(optarg,NULL,0);
			break;
		case 'r':
			range = strtoul(optarg,NULL,0);
			break;
		case 'n':
			n_chan = strtoul(optarg,NULL,0);
			break;
		case 'N':
			n_scan = strtoul(optarg,NULL,0);
			break;
		case 'F':
			freq = strtoul(optarg,NULL,0);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'd':
			aref = AREF_DIFF;
			break;
		case 'g':
			aref = AREF_GROUND;
			break;
		case 'o':
			aref = AREF_OTHER;
			break;
		case 'm':
			aref = AREF_COMMON;
			break;
		case 'h':
		        usage(argv[0]);
			exit(0);
			break;
		default:
			printf("bad option\n");
			exit(1);
		}
	}
	if(optind < argc) {
		/* data value */
		sscanf(argv[optind++],"%d",&value);
	}

	return argc;
#endif
	return 0;
}

#if 0
char *cmd_src(int src,char *buf)
{
	buf[0]=0;

	if(src&TRIG_NONE)strcat(buf,"none|");
	if(src&TRIG_NOW)strcat(buf,"now|");
	if(src&TRIG_FOLLOW)strcat(buf, "follow|");
	if(src&TRIG_TIME)strcat(buf, "time|");
	if(src&TRIG_TIMER)strcat(buf, "timer|");
	if(src&TRIG_COUNT)strcat(buf, "count|");
	if(src&TRIG_EXT)strcat(buf, "ext|");
	if(src&TRIG_INT)strcat(buf, "int|");
#ifdef TRIG_OTHER
	if(src&TRIG_OTHER)strcat(buf, "other|");
#endif

	if(strlen(buf)==0){
		sprintf(buf,"unknown(0x%08x)",src);
	}else{
		buf[strlen(buf)-1]=0;
	}

	return buf;
}

void dump_cmd(FILE *out,comedi_cmd *cmd)
{
	char buf[100];

	fprintf(out,"start:      %-8s %d\n",
		cmd_src(cmd->start_src,buf),
		cmd->start_arg);

	fprintf(out,"scan_begin: %-8s %d\n",
		cmd_src(cmd->scan_begin_src,buf),
		cmd->scan_begin_arg);

	fprintf(out,"convert:    %-8s %d\n",
		cmd_src(cmd->convert_src,buf),
		cmd->convert_arg);

	fprintf(out,"scan_end:   %-8s %d\n",
		cmd_src(cmd->scan_end_src,buf),
		cmd->scan_end_arg);

	fprintf(out,"stop:       %-8s %d\n",
		cmd_src(cmd->stop_src,buf),
		cmd->stop_arg);
}
#endif

