
#include <stdio.h>
#include <bigfont.h>
#include <curses.h>


static char bigfont[16*256];

int bigfont_init(void)
{
	FILE *ff;
	
	ff=fopen("/usr/lib/kbd/consolefonts/default8x16","r");
	fread(bigfont,16*256,1,ff);
	fclose(ff);
	
	return 0;
}

void bigfont_putchar(int x,int y,int c)
{
	int i,j,xx;

	for(i=0;i<12;i++){
		j=128;
		for(xx=0;xx<8;xx++){
			if((bigfont[c*16+i+2]&j))attron(A_REVERSE);
			else attroff(A_REVERSE);
			mvaddch(y+i,x+xx,' ');
			j>>=1;
		}
	}
	attroff(A_REVERSE);
	
}

