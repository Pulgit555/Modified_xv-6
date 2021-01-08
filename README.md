# Modified xv-6
It is the modified xv6 operating system including addditional syscalls,user programs and scheduling algorithms.
To run the os use command `make qemu`
and if you want to change the scheduler use command `make qemu SCHEDULER=<SCHEDULER>` 
- SCHEDULER={RR,FCFS,PBS,MLFQ} 

## For adding a syscall ->
we have to make changes in files:-
# Modified xv-6
It is the modified xv6 operating system including addditional syscalls,user programs and scheduling algorithms.
To run the os use command `make qemu`
and if you want to change the scheduler use command `make qemu SCHEDULER=<SCHEDULER>` 
- SCHEDULER={RR,FCFS,PBS,MLFQ} 

## For adding a syscall ->
we have to make changes in files:-
1. syscall.h
2. syscall.c
3. user.h
4. sysproc.c
5. usys.S
6. defs.h
7. proc.h
8. proc.c
9. trap.c(in case of waitx)

## For adding a user program ->
1. Makefile
2. create a new file  
  - time.c (using waitx syscall)
  - ps.c (using getps syscall)
  - setPriority.c (using set_priority syscall)

### Task 1:

1.	**waitx syscall**
		used as `waitx(int* wtime,int* rtime)` 
		we give two pointers as arguments which assign the total number of clock ticks for which it is waiting 
		and total number of clock ticks when process was running.actual waitx() function is implemented in proc.c
		new changes in proc.h are->
```
  	int ctime;                   // Creation time for process
  	int etime;                   // End time for process
  	int rtime;                   // Total time for process
 	int iotime;                  // I-O time for process
 	*wtime = etime-ctime-rtime-iotime
        *rtime = rtime
```        
 ctime is equal to ticks at the time of allocproc()
 		etime is equal to ticks at the time of exit()
 		rtime and iotime gets updated in trap.c file. if process state is running we increment the rtime else if 
 		process state is sleeping we increment the iotime.
 		time.c file is made to implement time user program which can be used as `time command` which uses 
 		syscall waitx .

2.	**getps syscall**
		used as `getps()`
*it will output* 
	-   pid(pid of the process)
	- Priority(current priority of process)
	- State(current state of the process)
	- r_time(number of clock ticks for which process ran on CPU)
	- w_time (waiting time for the process in case of MLFQ, it will get updated each time a process gets selected by the scheduler or if a change in the queue takes place)
	- n_run(number of times process is taken by scheduler)
	- c_time(create time for process)
	- Name(name of the process)
	
*In case of MLFQ it will print further information also*
   - cur_q(current queue number of process)
    - q{i}(Number of ticks the process has ran in that particular queue) 
    ps.c file is used to implement ps user program which can be used as `ps` only which uses getps syscall.

### Task 2:
#### Scheduling Algorithms -
*they are implemented in proc.c*

1. **RR** -> 	DEFAULT algorithm implemented already in the xv6.

2. **FCFS** -> 	First come first served. In this we first find  the smallest creation time among all the 
			processes possible.then check any process with this creation time and then that process starts 
			running. It will be non preemptive so yield() is not used in this case in trap.c

3. **PBS** ->	Priority Based Scheduling. In this we first find the process with highest priority (i.e. 
			smallest priority value) then we search if we have any other process having priority higher than 
			that of previous one then we yield() so that that having highest priority get a chance 
			first.otherwise process selected will run. default priority given is 60 .RR is used for all 
			processes having same highest priority.set_priority syscall is used to change the priority of 
			process it will call yield if priority value (i.e priority is increased for some process).
			user program `setPriority new_priority pid` 

4. **MLFQ** ->   Multi Level Feedback Queue Scheduling. For this case , there are five queues having time slices 
			1,2,4,8,16 timer ticks where 1 timer tick = 2 ticks. First queue (q0) has highest priority and will 
			be processed first after that second queue(q1) gets its chance.aging is also implemented by checking 
			if the wait time of a process in a particular queue will be pushed to higher priority queue to 
			prevent starvation. Maximum waiting time for a queue is 40.process are added in userinit,
			fork,wakeup1 function and processes are removed from queues in wait, waitx,kill function in proc.c.

**Que** -> If a process voluntarily relinquishes control of the CPU, it leaves the queuing network, and when the 
	process becomes ready again after the I/O, it is​ ​ inserted at the tail of the same queue, from which it is 
	relinquished earlier​ ​(Explain in the report how could this be exploited by a process.)

**Ans** -> If a process voluntarily relinquishes control of the CPU, it leaves the queuing network, and when the 
	process becomes ready again after the I/O, it is inserted at the tail of the same queue, from which it is 
	relinquished earlier. This can be exploited by a process, as just when the time-slice is about to expire, 
	the process can voluntarily relinquish control of the CPU, and get inserted in the same queue again. If it 
	ran as normal, then due to time-slice getting expired, it would have been preempted to a lower priority 
	queue and will get a chance of running after all processes in higher priority queue are done with their time 
	slices. But the process, after exploitation, will remain in the higher priority queue, so that it can run 
	again sooner that it should have and using this technique this process.and in this way it does not allow
	other processes to get the priority to use CPU . And a single process will use the CPU until it is done.


### Other files ->
 -	benchmark.c (to compare the schedulers)
 -  bench.c (to check whether scheduler are working fine or not)
 -  test.c (for bonus part i.e making graph in MLFQ scheduler)
	​

## Performance analysis.
*Result -> when we run benchmark.c and see its time then* 
			
1. **RR-> 4067**
Output->
```
Time taken by the process 1
Run time 1 IO time 0
$ time benchmark
Process: 10 Finished
Time taken by the process 71
Run time 0 IO time 18
Process: 9 Finished
Time taken by the process 871
Run time 90 IO time 17
Process: 8 Finished
Time taken by the process 1584
Run time 180 IO time 16
Process: 7 Finished
Time taken by the process 2207
Run time 270 IO time 16
Process: 6 Finished
Time taken by the process 2735
Run time 359 IO time 15
Process: 5 Finished
Time taken by the process 3181
Run time 449 IO time 15
Process: 4 Finished
Time taken by the process 3529
Run time 538 IO time 14
Process: 3 Finished
Time taken by the process 3796
Run time 629 IO time 12
Process: 2 Finished
Time taken by the process 3968
Run time 718 IO time 10
Process: 1 Finished
Time taken by the process 4060
Run time 814 IO time 5
PID		Priority	State		c_time		r_time		w_time		n_run		Name
1		60		SLEEPING	0		3		1		25		init
2		60		SLEEPING	4		5		3		25		sh
16		60		SLEEPING	8130		1		1		2		time
17		60		RUNNING 	8132		10		19		21		benchmark
Time taken by the process 4062
Run time 10 IO time 4033
etime 12194  ctime 8132    rtime 10  iotime 4033 priority 60
Time taken by benchmark
 Wait time: 19  Run time: 10 status: 17
Time taken by the process 4067
Run time 4 IO time 4062
```

2. **FCFS-> 4085**
Output->
```
Process: 1 Finished
Time taken by the process 809
Run time 799 IO time 5
Process: 2 Finished
Time taken by the process 1526
Run time 717 IO time 6
Process: 3 Finished
Time taken by the process 2161
Run time 626 IO time 15
Process: 4 Finished
Time taken by the process 2701
Run time 540 IO time 16
Process: 5 Finished
Time taken by the process 3157
Run time 446 IO time 25
Process: 6 Finished
Time taken by the process 3514
Run time 357 IO time 26
Process: 7 Finished
Time taken by the process 3791
Run time 267 IO time 35
Process: 8 Finished
Time taken by the process 3973
Run time 178 IO time 40
Process: 9 Finished
Time taken by the process 4062
Run time 89 IO time 41
Process: 10 Finished
Time taken by the process 4072
Run time 0 IO time 50
PID		Priority	State		c_time		r_time		w_time		n_run		Name
1		60		SLEEPING	0		4		1		22		init
2		60		SLEEPING	5		2		1		16		sh
3		60		SLEEPING	622		2		0		7		time
4		60		RUNNING 	623		9		3		17		benchmark
Time taken by the process 4082
Run time 10 IO time 4069
etime 4705  ctime 623    rtime 10  iotime 4069 priority 60
Time taken by benchmark
 Wait time: 3  Run time: 10 status: 4
Time taken by the process 4085
Run time 4 IO time 4081
```

3. **PBS-> 4068**
Output->
```
Process: 10 Finished
Time taken by the process 51
Run time 0 IO time 50
Process: 9 Finished
Time taken by the process 138
Run time 90 IO time 45
Process: 8 Finished
Time taken by the process 313
Run time 180 IO time 40
Process: 7 Finished
Time taken by the process 578
Run time 268 IO time 35
Process: 6 Finished
Time taken by the process 933
Run time 359 IO time 30
Process: 5 Finished
Time taken by the process 1378
Run time 449 IO time 25
Process: 4 Finished
Time taken by the process 1911
Run time 537 IO time 20
Process: 3 Finished
Time taken by the process 2540
Run time 632 IO time 15
Process: 2 Finished
Time taken by the process 3257
Run time 722 IO time 10
Process: 1 Finished
Time taken by the process 4062
Run time 809 IO time 5
PID		Priority	State		c_time		r_time		w_time		n_run		Name
1		60		SLEEPING	0		3		2		25		init
2		60		SLEEPING	4		4		4		26		sh
28		60		SLEEPING	12249		1		1		2		time
29		60		RUNNING 	12251		10		2		21		benchmark
Time taken by the process 4064
Run time 11 IO time 4051
etime 16315  ctime 12251    rtime 11  iotime 4051 priority 60
Time taken by benchmark
 Wait time: 2  Run time: 11 status: 29
Time taken by the process 4068
Run time 3 IO time 4064
```

4. **MLFQ-> 4071**   
Output->
```
Process: 10 Finished
Time taken by the process 108
Run time 0 IO time 23
Process: 9 Finished
Time taken by the process 810
Run time 89 IO time 21
Process: 8 Finished
Time taken by the process 1551
Run time 180 IO time 19
Process: 7 Finished
Time taken by the process 2176
Run time 270 IO time 17
Process: 6 Finished
Time taken by the process 2706
Run time 360 IO time 15
Process: 5 Finished
Time taken by the process 3156
Run time 450 IO time 13
Process: 4 Finished
Time taken by the process 3531
Run time 543 IO time 11
Process: 3 Finished
Time taken by the process 3794
Run time 630 IO time 9
Process: 2 Finished
Time taken by the process 3972
Run time 719 IO time 7
Process: 1 Finished
Time taken by the process 4064
Run time 811 IO time 5
PID		Priority	State		c_time		r_time		w_time		n_run		cur_q	q0	q1	q2	q3	q04	Name
1		60		SLEEPING	     0		       4		5330		25		       3	 6	7	12	1	0	init
2		60		SLEEPING	     4		       3		4068		18		       2	 2	14	3	0	0	sh
3		60		SLEEPING	     1267	       2		4065		9		       2	 2	6	1	0	0	time
4		60		RUNNING 	     1269	       8		0		    20		       3	 2	6	12	5	0	benchmark
Time taken by the process 4067
Run time 9 IO time 4038
etime 5336  ctime 1269    rtime 9  iotime 4038 priority 60
Time taken by benchmark
 Wait time: 20  Run time: 9 status: 4
Time taken by the process 4071
Run time 4 IO time 4066
```

#### Time taken by using `time benchmark` we get->
- FCFS - 4085 ticks
- RR - 4067 ticks
- PBS - 4068 ticks
- MLFQ - 4071 ticks

Overall all the schedulers have similar time to finish the process. 
But comparing on the basis of benchmark.c we get FCFS takes the maximum time whereas all other scheduler does 
not have much margin but RR takes the minimum time to complete and then PBS is completed.
### Order ->time taken 
		FCFS > MLFQ > PBS > RR 
**Analysis of benchmark.c ->**  Earlier processes (having lower pid will have less io time and more cpu time 
	than later processes.and priority is also changed in between giving later processes higher priority(i.e 
	priority value less))Thatswhy  FCFS is worst as lower cpu burst process have to wait for larger cpu burst 
	process.But in case of PBS since lower cpu burst process have higher priority it will get a chance to use 
	cpu first thats why its performance is better than FCFS.
	MLFQ is not better than PBS because of overhead of queue change.	
	Overall RR is best among all other schedulers as it give all processes adequate time to run on cpu
	
**Bonus ->** Graph is made using test.c file and its data is stored in graph_data.txt file. and MLFQ.pnj is the 
timeline graph.

1. syscall.h
2. syscall.c
3. user.h
4. sysproc.c
5. usys.S
6. defs.h
7. proc.h
8. proc.c
9. trap.c(in case of waitx)

## For adding a user program ->
1. Makefile
2. create a new file  
  - time.c (using waitx syscall)
  - ps.c (using getps syscall)
  - setPriority.c (using set_priority syscall)

### Task 1:

1.	**waitx syscall**
		used as `waitx(int* wtime,int* rtime)` 
		we give two pointers as arguments which assign the total number of clock ticks for which it is waiting 
		and total number of clock ticks when process was running.actual waitx() function is implemented in proc.c
		new changes in proc.h are->
  		`int ctime;                   // Creation time for process
  		int etime;                   // End time for process
  		int rtime;                   // Total time for process
 		int iotime;                  // I-O time for process
 		*wtime = etime-ctime-rtime-iotime
 		*rtime = rtime`
 		ctime is equal to ticks at the time of allocproc()
 		etime is equal to ticks at the time of exit()
 		rtime and iotime gets updated in trap.c file. if process state is running we increment the rtime else if 
 		process state is sleeping we increment the iotime.
 		time.c file is made to implement time user program which can be used as `time command` which uses 
 		syscall waitx .

2.	**getps syscall**
		used as `getps()`
		*it will output* 
			- pid(pid of the process)
			- Priority(current priority of process)
			- State(current state of the process)
			- r_time(number of clock ticks for which process ran on CPU)
			- w_time (waiting time for the process in case of MLFQ, it will get updated each time a process 
				gets selected by the scheduler or if a change in the queue takes place)
			- n_run(number of times process is taken by scheduler)
			- c_time(create time for process)
			- Name(name of the process)
		*In case of MLFQ it will print further information also*
			- cur_q(current queue number of process)
			- q{i}(Number of ticks the process has ran in that particular queue) 
		ps.c file is used to implement ps user program which can be used as `ps` only which uses getps syscall.

### Task 2:
#### Scheduling Algorithms -
*they are implemented in proc.c*

1. **RR** -> 	DEFAULT algorithm implemented already in the xv6.

2. **FCFS** -> 	First come first served. In this we first find  the smallest creation time among all the 
			processes possible.then check any process with this creation time and then that process starts 
			running. It will be non preemptive so yield() is not used in this case in trap.c

3. **PBS** ->	Priority Based Scheduling. In this we first find the process with highest priority (i.e. 
			smallest priority value) then we search if we have any other process having priority higher than 
			that of previous one then we yield() so that that having highest priority get a chance 
			first.otherwise process selected will run. default priority given is 60 .RR is used for all 
			processes having same highest priority.set_priority syscall is used to change the priority of 
			process it will call yield if priority value (i.e priority is increased for some process).
			user program `setPriority new_priority pid` 

4. **MLFQ** ->   Multi Level Feedback Queue Scheduling. For this case , there are five queues having time slices 
			1,2,4,8,16 timer ticks where 1 timer tick = 2 ticks. First queue (q0) has highest priority and will 
			be processed first after that second queue(q1) gets its chance.aging is also implemented by checking 
			if the wait time of a process in a particular queue will be pushed to higher priority queue to 
			prevent starvation. Maximum waiting time for a queue is 40.process are added in userinit,
			fork,wakeup1 function and processes are removed from queues in wait, waitx,kill function in proc.c.

**Que** -> If a process voluntarily relinquishes control of the CPU, it leaves the queuing network, and when the 
	process becomes ready again after the I/O, it is​ ​ inserted at the tail of the same queue, from which it is 
	relinquished earlier​ ​(Explain in the report how could this be exploited by a process.)

**Ans** -> If a process voluntarily relinquishes control of the CPU, it leaves the queuing network, and when the 
	process becomes ready again after the I/O, it is inserted at the tail of the same queue, from which it is 
	relinquished earlier. This can be exploited by a process, as just when the time-slice is about to expire, 
	the process can voluntarily relinquish control of the CPU, and get inserted in the same queue again. If it 
	ran as normal, then due to time-slice getting expired, it would have been preempted to a lower priority 
	queue and will get a chance of running after all processes in higher priority queue are done with their time 
	slices. But the process, after exploitation, will remain in the higher priority queue, so that it can run 
	again sooner that it should have and using this technique this process.and in this way it does not allow
	other processes to get the priority to use CPU . And a single process will use the CPU until it is done.


### Other files ->
 -	benchmark.c (to compare the schedulers)
 -  bench.c (to check whether scheduler are working fine or not)
 -  test.c (for bonus part i.e making graph in MLFQ scheduler)
	​

## Performance analysis.
*Result -> when we run benchmark.c and see its time then* 
			
1. **RR-> 4067**
Output->
`
Time taken by the process 1
Run time 1 IO time 0
$ time benchmark
Process: 10 Finished
Time taken by the process 71
Run time 0 IO time 18
Process: 9 Finished
Time taken by the process 871
Run time 90 IO time 17
Process: 8 Finished
Time taken by the process 1584
Run time 180 IO time 16
Process: 7 Finished
Time taken by the process 2207
Run time 270 IO time 16
Process: 6 Finished
Time taken by the process 2735
Run time 359 IO time 15
Process: 5 Finished
Time taken by the process 3181
Run time 449 IO time 15
Process: 4 Finished
Time taken by the process 3529
Run time 538 IO time 14
Process: 3 Finished
Time taken by the process 3796
Run time 629 IO time 12
Process: 2 Finished
Time taken by the process 3968
Run time 718 IO time 10
Process: 1 Finished
Time taken by the process 4060
Run time 814 IO time 5
PID		Priority	State		c_time		r_time		w_time		n_run		Name
1		60		SLEEPING	0		3		1		25		init
2		60		SLEEPING	4		5		3		25		sh
16		60		SLEEPING	8130		1		1		2		time
17		60		RUNNING 	8132		10		19		21		benchmark
Time taken by the process 4062
Run time 10 IO time 4033
etime 12194  ctime 8132    rtime 10  iotime 4033 priority 60
Time taken by benchmark
 Wait time: 19  Run time: 10 status: 17
Time taken by the process 4067
Run time 4 IO time 4062
`

2. **FCFS-> 4085**
Output->
`
Process: 1 Finished
Time taken by the process 809
Run time 799 IO time 5
Process: 2 Finished
Time taken by the process 1526
Run time 717 IO time 6
Process: 3 Finished
Time taken by the process 2161
Run time 626 IO time 15
Process: 4 Finished
Time taken by the process 2701
Run time 540 IO time 16
Process: 5 Finished
Time taken by the process 3157
Run time 446 IO time 25
Process: 6 Finished
Time taken by the process 3514
Run time 357 IO time 26
Process: 7 Finished
Time taken by the process 3791
Run time 267 IO time 35
Process: 8 Finished
Time taken by the process 3973
Run time 178 IO time 40
Process: 9 Finished
Time taken by the process 4062
Run time 89 IO time 41
Process: 10 Finished
Time taken by the process 4072
Run time 0 IO time 50
PID		Priority	State		c_time		r_time		w_time		n_run		Name
1		60		SLEEPING	0		4		1		22		init
2		60		SLEEPING	5		2		1		16		sh
3		60		SLEEPING	622		2		0		7		time
4		60		RUNNING 	623		9		3		17		benchmark
Time taken by the process 4082
Run time 10 IO time 4069
etime 4705  ctime 623    rtime 10  iotime 4069 priority 60
Time taken by benchmark
 Wait time: 3  Run time: 10 status: 4
Time taken by the process 4085
Run time 4 IO time 4081
` 

3. **PBS-> 4068**
Output->
`
Process: 10 Finished
Time taken by the process 51
Run time 0 IO time 50
Process: 9 Finished
Time taken by the process 138
Run time 90 IO time 45
Process: 8 Finished
Time taken by the process 313
Run time 180 IO time 40
Process: 7 Finished
Time taken by the process 578
Run time 268 IO time 35
Process: 6 Finished
Time taken by the process 933
Run time 359 IO time 30
Process: 5 Finished
Time taken by the process 1378
Run time 449 IO time 25
Process: 4 Finished
Time taken by the process 1911
Run time 537 IO time 20
Process: 3 Finished
Time taken by the process 2540
Run time 632 IO time 15
Process: 2 Finished
Time taken by the process 3257
Run time 722 IO time 10
Process: 1 Finished
Time taken by the process 4062
Run time 809 IO time 5
PID		Priority	State		c_time		r_time		w_time		n_run		Name
1		60		SLEEPING	0		3		2		25		init
2		60		SLEEPING	4		4		4		26		sh
28		60		SLEEPING	12249		1		1		2		time
29		60		RUNNING 	12251		10		2		21		benchmark
Time taken by the process 4064
Run time 11 IO time 4051
etime 16315  ctime 12251    rtime 11  iotime 4051 priority 60
Time taken by benchmark
 Wait time: 2  Run time: 11 status: 29
Time taken by the process 4068
Run time 3 IO time 4064
`

4. **MLFQ-> 4071**   
Output->
`
Process: 10 Finished
Time taken by the process 108
Run time 0 IO time 23
Process: 9 Finished
Time taken by the process 810
Run time 89 IO time 21
Process: 8 Finished
Time taken by the process 1551
Run time 180 IO time 19
Process: 7 Finished
Time taken by the process 2176
Run time 270 IO time 17
Process: 6 Finished
Time taken by the process 2706
Run time 360 IO time 15
Process: 5 Finished
Time taken by the process 3156
Run time 450 IO time 13
Process: 4 Finished
Time taken by the process 3531
Run time 543 IO time 11
Process: 3 Finished
Time taken by the process 3794
Run time 630 IO time 9
Process: 2 Finished
Time taken by the process 3972
Run time 719 IO time 7
Process: 1 Finished
Time taken by the process 4064
Run time 811 IO time 5
PID		Priority	State		c_time		r_time		w_time		n_run		cur_q	q0	q1	q2	q3	q04	Name
1		60		SLEEPING	     0		       4		5330		25		       3	 6	7	12	1	0	init
2		60		SLEEPING	     4		       3		4068		18		       2	 2	14	3	0	0	sh
3		60		SLEEPING	     1267	       2		4065		9		       2	 2	6	1	0	0	time
4		60		RUNNING 	     1269	       8		0		    20		       3	 2	6	12	5	0	benchmark
Time taken by the process 4067
Run time 9 IO time 4038
etime 5336  ctime 1269    rtime 9  iotime 4038 priority 60
Time taken by benchmark
 Wait time: 20  Run time: 9 status: 4
Time taken by the process 4071
Run time 4 IO time 4066
`

#### Time taken by using `time benchmark` we get->
- FCFS - 4085 ticks
- RR - 4067 ticks
- PBS - 4068 ticks
- MLFQ - 4071 ticks

Overall all the schedulers have similar time to finish the process. 
But comparing on the basis of benchmark.c we get FCFS takes the maximum time whereas all other scheduler does 
not have much margin but RR takes the minimum time to complete and then PBS is completed.
### Order ->time taken 
		FCFS > MLFQ > PBS > RR 
**Analysis of benchmark.c ->**  Earlier processes (having lower pid will have less io time and more cpu time 
	than later processes.and priority is also changed in between giving later processes higher priority(i.e 
	priority value less))Thatswhy  FCFS is worst as lower cpu burst process have to wait for larger cpu burst 
	process.But in case of PBS since lower cpu burst process have higher priority it will get a chance to use 
	cpu first thats why its performance is better than FCFS.
	MLFQ is not better than PBS because of overhead of queue change.	
	Overall RR is best among all other schedulers as it give all processes adequate time to run on cpu
**Bonus ->** Graph is made using test.c file and its data is stored in graph_data.txt file. and MLFQ.pnj is the 
timeline graph.
