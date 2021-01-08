#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main(int argc, char *argv[])
{
	int wtime=0, rtime=0;
	int pid = fork();
	if(pid<0)
	{
		printf(1,"Failed to fork {time}\n");
		exit();
	}
	else 
	{
		if(pid==0)
		{
			if(argc==1)
			{
				printf(1,"Please enter valid command\n");
				exit();
			}
			else
			{
				if(exec(argv[1],argv+1)<0)
				{
					printf(1,"Error: %s Failed\n",argv[1]);
					exit();
				}
			}
		}
		else
		{
			int b = waitx(&wtime, &rtime);
			if(argc!=1)
			{
				printf(1,"Time taken by %s\n Wait time: %d  Run time: %d status: %d\n",argv[1],wtime,rtime,b);
				exit();
			}
			else
			{
				exit();
			}
		}
	}
}