/*
 * Demo of counters
 * Part of Comedilib
 *
 * Copyright (c) 2001 David A. Schleef <ds@schleef.org>
 *
 * This file may be freely modified, distributed, and combined with
 * other software, as long as proper attribution is given in the
 * source code.
 */
/*
   A little input demo
 */

#include <stdio.h>
#include <comedilib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include "examples.h"

comedi_t *device;


int main(int argc, char *argv[])
{
	lsampl_t data;
	int ret;

	parse_options(argc,argv);

	device=comedi_open(filename);
	if(!device){
		comedi_perror(filename);
		exit(0);
	}

	return 0;
}


#define M_PACK2(a,b) (((a)<<2)|(b))
#define M_PACK4(a,b,c,d) ((M_PACK2(a,b)<<4)|M_PACK2(c,d))

#define M_IGN 0
#define M_INC 1
#define M_DEC 2

struct counter_config_struct{
	unsigned int id;
	unsigned int inp0_src;
	unsigned int inp0_arg;
	unsigned int inp0_mach;
	unsigned int inp1_src;
	unsigned int inp1_arg;
	unsigned int inp1_mach;
};

void cfg1(void)
{
	/* Simple event counter */
	/* Accumulator increments on positive edge of input signal */
	/* STC: simple event counting */

	cfg->id = ID_COUNTER_CFG;

	cfg->inp0_src = TRIG_EXT;
	cfg->inp0_arg = CR_PACK(chan,0,0);
	cfg->inp0_mach = M_PACK2(M_INC,M_IGN);
}

void cfg2(void)
{
	/* Simple event counter */
	/* Accumulator increments on negative edge of input signal */
	/* STC: simple event counting */

	cfg->id = ID_COUNTER_CFG;

	cfg->inp0_src = TRIG_EXT;
	cfg->inp0_arg = CR_PACK(chan,0,0);
	cfg->inp0_mach = M_PACK2(M_IGN,M_INC);
}

void cfg3(void)
{
	/* event counter, with gate */
	/* if gate is set, accumulator increments on positive edge of
	   input signal */
	/* STC: simple gated event counting */

	cfg->id = ID_COUNTER_CFG;

	cfg->inp0_src = TRIG_EXT;
	cfg->inp0_arg = CR_PACK(chan,0,0);
	cfg->inp0_mach = M_PACK4(M_INC,M_IGN,M_IGN,M_IGN);

	cfg->inp1_src = TRIG_EXT;
	cfg->inp1_arg = CR_PACK(gate,0,0);
	cfg->inp1_mach = M_PACK4(M_IGN,M_IGN,M_IGN,M_IGN);
}

void cfg4(void)
{
	/* pulse length measurement */
	/* if gate is set, accumulator increments every ns */
	/* STC: single pulse width measurement */

	cfg->id = ID_COUNTER_CFG;

	cfg->inp0_src = TRIG_TIMER;
	cfg->inp0_mach = M_PACK4(M_INC,M_IGN,M_IGN,M_IGN);

	cfg->inp1_src = TRIG_EXT;
	cfg->inp1_arg = CR_PACK(gate,0,0);
	cfg->inp1_mach = M_PACK4(M_IGN,M_IGN,M_IGN,M_IGN);

	/* how to specify completed flag? */
}

void cfg5(void)
{
	/* timestamping */
	/* accumulator increments every ns, latch on chan positive edge */
	/* STC: buffered cumulative event counting */

	cfg->id = ID_COUNTER_CFG;

	cfg->inp0_src = TRIG_TIMER;
	cfg->inp0_mach = M_PACK2(M_INC,M_IGN);

	/* how to reset at end of pulse?  (to make non-cumulative
	 * event counting */
	
	cmd->scan_begin_src = TRIG_EXT;
	cmd->scan_begin_arg = CR_PACK(chan,0,0)|CR_INVERT;
}

void cfg6(void)
{
	/* up/down counter */
	/* if updown is 1, accumulator increments on rising edge of chan,
	 * otherwise decrements. */
	/* STC: relative position sensing */

	cfg->id = ID_COUNTER_CFG;

	cfg->inp0_src = TRIG_EXT;
	cfg->inp0_arg = CR_PACK(chan,0,0);
	cfg->inp0_mach = M_PACK4(M_INC,M_IGN,M_DEC,M_IGN);

	cfg->inp1_src = TRIG_EXT;
	cfg->inp1_arg = CR_PACK(updown,0,0);
	cfg->inp1_mach = M_PACK4(M_IGN,M_IGN,M_IGN,M_IGN);
}

void cfg6(void)
{
	/* quadrature counter */
	/* complicated, watch carefully */

	cfg->id = ID_COUNTER_CFG;

	cfg->inp0_src = TRIG_EXT;
	cfg->inp0_arg = CR_PACK(chan,0,0);
	cfg->inp0_mach = M_PACK4(M_INC,M_DEC,M_DEC,M_INC);

	cfg->inp1_src = TRIG_EXT;
	cfg->inp1_arg = CR_PACK(updown,0,0);
	cfg->inp1_mach = M_PACK4(M_DEC,M_INC,M_INC,M_DEC);
}

void cfg7(void)
{
	/* up/down tracking */
	/* up/down counter, but latches with every change of the
	 * accumulator */

	cfg->id = ID_COUNTER_CFG;

	cfg->inp0_src = TRIG_EXT;
	cfg->inp0_arg = CR_PACK(chan,0,0);
	cfg->inp0_mach = M_PACK4(M_INC,M_IGN,M_DEC,M_IGN);

	cfg->inp1_src = TRIG_EXT;
	cfg->inp1_arg = CR_PACK(updown,0,0);
	cfg->inp1_mach = M_PACK4(M_IGN,M_IGN,M_IGN,M_IGN);

	cmd->scan_start_src = TRIG_FOLLOW;
	cmd->scan_start_arg = 0; /* follow inp0 */
}

void cfgN(void)
{
	/* frequency generation */
	/* XXX */

	cfg->id = ID_COUNTER_CFG;

	cfg->set_src = TRIG_TIMER;
	cfg->set_arg = 0;

	cfg->reset_src = TRIG_FOLLOW;
	cfg->reset_arg = 0;

	cfg->out0_src = TRIG_EXT;
	cfg->out0_arg = CR_PACK(out,0,0);
}


