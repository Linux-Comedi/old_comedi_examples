
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

static void atex(void)
{
	clear();
	refresh();
	endwin();
}

void init_nc(void)
{
	initscr();
	start_color();
	cbreak();
	noecho();
	init_pair(1,COLOR_RED,COLOR_BLACK);
	init_pair(2,COLOR_GREEN,COLOR_BLACK);
	init_pair(3,COLOR_BLUE,COLOR_BLACK);
	attrset(COLOR_PAIR(0));
	keypad(stdscr,TRUE);
	timeout(10);

	clear();
	
	refresh();

	atexit(atex);
}


