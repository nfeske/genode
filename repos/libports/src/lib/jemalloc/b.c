#include <stdio.h>


static int static_int;


struct global_struct
{
	int not_good;
	int even_worse;
};


struct global_struct global_struct;


void call_b(void)
{
	static_int = 7;
	global_struct.not_good = 8;
	global_struct.even_worse = 9;
}


void show_b(void)
{
	printf("state of b: %d, %d, %d\n",
	       static_int, global_struct.not_good, global_struct.even_worse);
}
