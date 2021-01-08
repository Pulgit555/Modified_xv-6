#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
int main(int argc, char *argv[])
{
	if(argc!=3)
	{
		printf(1,"Please enter a valid command\n");
	}
	else
	{
		int pid = fork();
		if(pid==0)
		{
			set_priority(atoi(argv[1]),atoi(argv[2]));
			exit();
		}
	}
	exit();
}