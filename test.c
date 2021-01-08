#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
int number_of_processes = 4;

int main(int argc, char *argv[])
{
  int j;
  // #ifdef PBS
  // int a=1;
  // #endif
  for (j = 0; j < number_of_processes; j++)
  {
    int pid = fork();
    if (pid < 0)
    {
      printf(1, "Fork failed\n");
      continue;
    }
    if (pid == 0)
    {
      volatile int i;
      for (volatile int k = 0; k < number_of_processes-2; k++)
      {
        if ((k + j)%2==0)
        {
          sleep(75+j*5); //io time
        }
        else
        {
          for (i = 0; i < 100000000 - 10*(j+1); i++)
          {
            int x=5;
            x=x+x;
            x=0; //cpu time
          }
        }
        if(k==j)
        {
        	for(i=0;i<10000000;i++)
        	{
        		int y=2;
        		y=y+y;
        		y=0;
        	}
        }
      }
      //getps();
       printf(1, "Process: %d Finished\n", j+1);
      exit();
    }
    else
    {
      #ifdef PBS
     // printf(1,"%d\n",pid);
      // a=a*(-2);
      // if(80-a<0||80-a>100)
      // {
      //   a=1;
      // }
      set_priority(100-(20+j),pid); // will only matter for PBS, comment it out if not implemented yet (better priorty for more IO intensive jobs)
      #endif
    }
  }
   for (j = 0; j < number_of_processes; j++)
   {
     wait();
  }
  getps();
  // int wtime,rtime;
  // int b = waitx(&wtime, &rtime);
  // printf(1,"Time taken by %d\n Wait time: %d  Run time: %d status: %d\n",getpid(),wtime,rtime,b);
  exit();
}