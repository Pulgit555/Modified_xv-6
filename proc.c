#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

int max_queue_ticks[5]={2,4,8,16,32}; //1 timer tick=2 ticks
struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

//int abc=3;
static struct proc *initproc;

struct proc *qu[5][NPROC];
int q_index[5] = {-1,-1,-1,-1,-1};
// q_index[0]=-1;
// q_index[1]=-1;
// q_index[2]=-1;
// q_index[3]=-1;
// q_index[4]=-1;
int nextpid = 1;
// max_queue_ticks[0]=1;
// max_queue_ticks[1]=2;
// max_queue_ticks[2]=4;
// max_queue_ticks[3]=8;
// max_queue_ticks[4]=16;

extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

 acquire(&tickslock);
  p->ctime = ticks;   //start time of process is initialized to current time 
  release(&tickslock);
  p->etime = 0;   //end time of process is initialized to 0 
  p->rtime = 0;   //run time of process is initialized to 0 
  p->iotime = 0;  //Input-Output time of process is initialized to 0
  //cprintf("%d\n",p->ctime);
  p->n_run = 0;
  p->priority = 60;
  #ifdef MLFQ
    p->cur_q = 0;
    for(int i=0;i<5;i++)
    {
      p->q[i]=0;
    }
    p->qtime = 0;
    p->flg = 0;
  #endif
  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  #ifdef MLFQ
    int alread=0;
    for(int i=0;i<=q_index[0];i++)
    {
      if(qu[0][i]->pid==p->pid)
      {
        alread=1;
        break;
      }
    }
    if(alread!=1)
    {
      p->cur_q=0;
      qu[0][q_index[0]+1]=p;
      q_index[0]++;
      p->qtime=ticks;
    // if(p->pid>=abc)
      // cprintf("%d %d %d\n",p->pid,p->cur_q,ticks);
    }
  #endif

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  #ifdef MLFQ
    int alread=0;
    for(int i=0;i<=q_index[np->cur_q];i++)
    {
      if(qu[np->cur_q][i]->pid==np->pid)
      {
        alread=1;
        break;
      }
    }
    if(alread!=1)
    {
      qu[np->cur_q][q_index[np->cur_q]+1]=np;
      q_index[np->cur_q]++;
      np->qtime=ticks;
     // if(np->pid>=abc)
     // cprintf("%d %d %d\n",np->pid,np->cur_q,ticks);
    }
  #endif

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  curproc->etime = ticks;  // marking the end of a process
  cprintf("Time taken by the process pid %d time %d\nRun time %d IO time %d end time %d create time %d priority %d\n",curproc->pid,curproc->etime-curproc->ctime,curproc->rtime,curproc->iotime,curproc->etime,curproc->ctime,curproc->priority);
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        //cprintf("\netime %d  ctime %d    rtime %d   \n", p->etime, p->ctime, p->rtime);
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        #ifdef MLFQ
        int alread=0,ind=0;
        for(int i=0;i<=q_index[p->cur_q];i++)
        {
          if(qu[p->cur_q][i]->pid==p->pid)
          {
            ind=i;
            alread=1;
            break;
          }
        }
        if(alread==1)
        {
          for(int i=ind;i<q_index[p->cur_q];i++)
          {
            qu[p->cur_q][i] = qu[p->cur_q][i+1];
          }
          q_index[p->cur_q]--;
          // if(p->pid>=abc)
       //   cprintf("%d %d %d\n",p->pid,p->cur_q,ticks);
        }
        #endif
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int waitx(int* wtime, int* rtime)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        #ifdef MLFQ
        int alread=0,ind=0;
        for(int i=0;i<=q_index[p->cur_q];i++)
        {
          if(qu[p->cur_q][i]->pid==p->pid)
          {
            ind=i;
            alread=1;
            break;
          }
        }
        if(alread==1)
        {
          for(int i=ind;i<q_index[p->cur_q];i++)
          {
            qu[p->cur_q][i] = qu[p->cur_q][i+1];
          }
          q_index[p->cur_q]--;
         //if(p->pid>=abc)
          //cprintf("%d %d %d\n",p->pid,p->cur_q,ticks);
        }
        #endif
        release(&ptable.lock);
        *rtime = p->rtime;
        *wtime = p->etime - p->ctime - p->rtime - p->iotime;
        cprintf("\netime %d  ctime %d    rtime %d  iotime %d priority %d\n", p->etime, p->ctime, p->rtime, p->iotime, p->priority);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int getps()
{
  struct proc* p;
  #ifndef MLFQ
  cprintf("PID\t\tPriority\tState\t\tc_time\t\tr_time\t\tw_time\t\tn_run\t\tName\n");
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state==SLEEPING || p->state==RUNNING || p->state==RUNNABLE || p->state==ZOMBIE)
    {
      cprintf("%d\t\t",p->pid);
      cprintf("%d\t\t",p->priority);
      if(p->state==SLEEPING)
        cprintf("SLEEPING\t");
      else if(p->state==RUNNING)
        cprintf("RUNNING \t");
      else if(p->state==RUNNABLE)
        cprintf("RUNNABLE\t");
      else if(p->state==ZOMBIE)
        cprintf("ZOMBIE \t");
      cprintf("%d\t\t",p->ctime);                   
      cprintf("%d\t\t",p->rtime);
      cprintf("%d\t\t",ticks - p->ctime - p->rtime - p->iotime);
      cprintf("%d\t\t",p->n_run);
      cprintf("%s\n",p->name);
    }
  }
  #else
  #ifdef MLFQ
  cprintf("PID\t\tPriority\tState\t\tc_time\t\tr_time\t\tw_time\t\tn_run\t\tcur_q\tq0\tq1\tq2\tq3\tq04\tName\n");
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->state==SLEEPING || p->state==RUNNING || p->state==RUNNABLE || p->state==ZOMBIE)
    {
      cprintf("%d\t\t",p->pid);
      cprintf("%d\t\t",p->priority);
      if(p->state==SLEEPING)
        cprintf("SLEEPING\t");
      else if(p->state==RUNNING)
        cprintf("RUNNING \t");
      else if(p->state==RUNNABLE)
        cprintf("RUNNABLE\t");
      else if(p->state==ZOMBIE)
        cprintf("ZOMBIE  \t");
      cprintf("%d\t\t",p->ctime);                   
      cprintf("%d\t\t",p->rtime);
      cprintf("%d\t\t",ticks - p->qtime);
      cprintf("%d\t\t",p->n_run);
      cprintf("%d\t",p->cur_q);
      for(int i=0;i<5;i++)
      {
        cprintf("%d\t",p->q[i]);
      }
      cprintf("%s\n",p->name);
    }
  }
  #endif
  #endif
  release(&ptable.lock);
  return 0;
}

int set_priority(int new_priority, int pid)
{
  struct proc *p;
  #ifdef PBS
  int preempt=0;
  #endif
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->pid==pid)
    {
      #ifdef PBS
      if(p->priority>new_priority)
      {
        preempt=1;
      }
      #endif
      p->priority = new_priority;
      //cprintf("%d %d\n",p->priority,p->pid);
      break;
    }
  }
  //cprintf("%d %d\n",new_priority,pid);
  release(&ptable.lock);  
  #ifdef PBS
  if(preempt==1)
  {
    yield(); // because priority becomes smaller so it should be again checked for priority.
  }
  #endif
  return 0;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;)
  {
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    #ifdef RR
      struct proc *p;
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if(p->state != RUNNABLE)
          continue;

        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        c->proc = p;
        switchuvm(p);//used for user 
        p->n_run++;
        p->state = RUNNING;

        swtch(&(c->scheduler), p->context);//executing it 
        switchkvm();//used for kernel 

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&ptable.lock);
    #else
    #ifdef FCFS
      struct proc *p;
      struct proc *first_process = 0;
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if(p->state != RUNNABLE)
          continue;
        if(first_process==0)
        {
          first_process=p;
        }
        else
        {
          if(first_process->ctime < p->ctime)
          {
            first_process=p;
          }
        }
      }  
        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
      if(first_process!=0 && first_process->state==RUNNABLE)
      {
        p=first_process;
        //cprintf("Process with pid : %d start time : %d started running\n",first_process->pid,first_process->ctime);
        c->proc = p;
        switchuvm(p);//used for user 
        p->n_run++;
        p->state = RUNNING;

        swtch(&(c->scheduler), p->context);//executing it 
        switchkvm();//used for kernel 

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&ptable.lock);
    #else
    #ifdef PBS
      struct proc *p;
      struct proc *high_priority_process = 0;
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if(p->state != RUNNABLE)
          continue;
        if(high_priority_process==0)
        {
          high_priority_process=p;
        }
        else
        {
          if((high_priority_process->priority)>(p->priority))
          {
            high_priority_process=p;
          }
        }
      }
      if(high_priority_process==0)
      {
        release(&ptable.lock);
      }
      else
      {
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        {
          struct proc *s;
          int higher_priority_process=0;
          for(s = ptable.proc; s < &ptable.proc[NPROC]; s++)
          {
            if(s->state != RUNNABLE)
            {
              continue;
            }
            if(s->priority < (high_priority_process->priority))
            {
              higher_priority_process=1;
              break;
            }
          }
          if(higher_priority_process==1)
          {
            break;
          }
          if(p->state != RUNNABLE)
          {
            continue;
          }
          if(p->priority==(high_priority_process->priority))
          {
           // cprintf("Process with pid : %d priority : %d started running\n",p->pid,p->priority);
            c->proc = p;
            switchuvm(p);//used for user 
            p->n_run++;
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);//executing it 
            switchkvm();//used for kernel 

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;            
          }
        }
        release(&ptable.lock);
      } 
    #else
    #ifdef MLFQ
      int alread=0,ind=0;
      for(int i=1;i<5;i++)
      {
        for(int j=0;j<=q_index[i];j++)
        {
          struct proc *p = qu[i][j];
          int aged = ticks - p->qtime;
          //cprintf("%d\n",aged);
          if(aged > 25)// && p->pid>=3)
          {
            //if(p->pid>=3)
           //cprintf("%d process is aged at queue %d and moving to queue %d\n",p->pid,i,i-1);
            //removing from queue i
            for(int ii=0;ii<=q_index[i];ii++)
            {
              if(qu[i][ii]->pid==p->pid)
              {
                ind=ii;
                alread=1;
                break;
              }
            }
            if(alread==1)
            {
              for(int ii=ind;ii<q_index[i];ii++)
              {
                qu[i][ii] = qu[i][ii+1];
              }
              q_index[i]--;
             // if(p->pid>=abc)
             // cprintf("%d %d %d\n",p->pid,i,ticks);
            }
           // if(p->pid>=3)
            //cprintf("pid %d leaves queue %d and goes to queue %d at %d\n",p->pid,i,i-1,ticks);
           // cprintf("Process with pid : %d shifted to queue : %d due to being aged\n",p->pid,i-1);
            //adding to queue i-1
            alread=0;
            for(int ii=0;ii<=q_index[i-1];ii++)
            {
              if(qu[i-1][ii]->pid==p->pid)
              {
                alread=1;
                break;
              }
            }
            if(alread!=1)
            {
              p->cur_q=i-1;
              qu[i-1][q_index[i-1]+1]=p;
              q_index[i-1]++;
              p->qtime=ticks;
             // if(p->pid>=abc)
             //  cprintf("%d %d %d\n",p->pid,i-1,ticks);
            }       
          }
        }
      }
      struct proc* p=0;
      for(int i=0;i<5;i++)
      {
        if(q_index[i]>=0)
        {
          p=qu[i][0];
          if(p!=0)
          {
            if(p->state==RUNNABLE)
            {
              p->q[i]++;
              if(p->q[i]>=max_queue_ticks[i])
              {
               // p->q[i]=max_queue_ticks[i];
                p->flg=1;
              }
              alread=0;ind=0;
              for(int ii=0;ii<=q_index[i];ii++)
              {
                if(qu[i][ii]->pid==p->pid)
                {
                  ind=ii;
                  alread=1;
                  break;
                }
              }
              if(alread==1)
              {
                for(int ii=ind;ii<q_index[i];ii++)
                {
                  qu[i][ii] = qu[i][ii+1];
                }
                q_index[i]--;
              }
              p->n_run++;
             // cprintf("Process with pid %d and queue %d with current ticks %d\n",p->pid,i,p->q[i]);
              c->proc = p;
              switchuvm(p);//used for user 
              p->state = RUNNING;
              swtch(&(c->scheduler), p->context);//executing it 
              switchkvm();//used for kernel 
              // Process is done running for now.
              // It should have changed its p->state before coming back.
              c->proc = 0;
              if(p!=0&&p->state==RUNNABLE)
              {
                //if(p->pid>=abc)
                 //{
                  // cprintf("%d %d %d\n",p->pid,p->cur_q,ticks);
                //}
                if(p->q[p->cur_q]>=max_queue_ticks[p->cur_q])
                {
                  p->flg=1;
                }
                if(p->flg==1)
                {
                  p->flg=0;
                  if(p->cur_q<4)
                  {
                    p->cur_q++;
                  }
                }
                alread=0;
                for(int ii=0;ii<=q_index[p->cur_q];ii++)
                {
                  if(qu[p->cur_q][ii]->pid==p->pid)
                  {
                    alread=1;
                    break;
                  }
                }
                if(alread!=1)
                {
                  qu[p->cur_q][q_index[p->cur_q]+1]=p;
                  q_index[p->cur_q]++;
                  p->qtime=ticks;
                  //if(p->pid>=abc)
                 // cprintf("%d %d %d\n",p->pid,p->cur_q,ticks);
                }        
              }
            }
          }
          break;
        }
       // break;
      }
      release(&ptable.lock);
    #endif     
    #endif      
    #endif
    #endif
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
    {
      p->state = RUNNABLE;
      #ifdef MLFQ
        int alread=0;
        for(int i=0;i<=q_index[p->cur_q];i++)
        {
          if(qu[p->cur_q][i]->pid==p->pid)
          {
            alread=1;
            break;
          }
        }
        if(alread!=1)
        {
          qu[p->cur_q][q_index[p->cur_q]+1]=p;
          q_index[p->cur_q]++;
          p->qtime=ticks;
          // if(p->pid>=abc)
         //  cprintf("%d %d %d\n",p->pid,p->cur_q,ticks);
        }
         p->qtime=ticks;       
      #endif
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
      {
        p->state = RUNNABLE;
        #ifdef MLFQ
          //p->qtime=0;
          int alread=0;
          for(int i=0;i<=q_index[p->cur_q];i++)
          {
            if(qu[p->cur_q][i]->pid==p->pid)
            {
              alread=1;
              break;
            }
          }
          if(alread!=1)
          {
            qu[p->cur_q][q_index[p->cur_q]+1]=p;
            q_index[p->cur_q]++;
            p->qtime=ticks;
          //  if(p->pid>=abc)
            //cprintf("%d %d %d\n",p->pid,p->cur_q,ticks);
          }       
          // p->qtime=ticks;
        #endif        
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
