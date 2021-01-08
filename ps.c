#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main(int argc, char *argv[])
{
	if(argc!=1)
	{
		printf(1,"Please enter a valid command\n");
	}
	else
	{
		getps();
	}
	exit();
}