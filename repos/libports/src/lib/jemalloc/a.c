#include <stdio.h>
#include <string.h>


static int static_bss_int;

static int static_data_int = 5;

long global_bss_long;

char global_nine_chars[9];


void call_a(void)
{
	static_bss_int  = 17;
	static_data_int = 15;   /* no longer the initial value */
	global_bss_long = 18;
	strcpy(global_nine_chars, "risky");
}


void show_a(void)
{
	printf("state of a: %d, %d, %ld, '%s'\n",
	       static_bss_int, static_data_int, global_bss_long, global_nine_chars);
}
