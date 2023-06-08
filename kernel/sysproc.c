#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    backtrace();
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler;

  // fetch arguments from user space
  argint(0, &interval);
  argaddr(1, &handler);

  if(interval < 0) { return -1; }

  struct proc *p = myproc();
  // store in the proc struct
  p->alarm.interval = interval;
  p->alarm.handler = handler;
  acquire(&tickslock);
  p->alarm.lasttick = ticks;
  release(&tickslock);

  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();

  // restore the state
  p->trapframe->epc = p->alarm.epc;
  p->trapframe->ra = p->alarm.ra;
  p->trapframe->sp = p->alarm.sp;
  p->trapframe->gp = p->alarm.gp;
  p->trapframe->tp = p->alarm.tp;
  p->trapframe->t0 = p->alarm.t0;
  p->trapframe->t1 = p->alarm.t1;
  p->trapframe->t2 = p->alarm.t2;
  p->trapframe->s0 = p->alarm.s0;
  p->trapframe->s1 = p->alarm.s1;
  p->trapframe->a1 = p->alarm.a1;
  p->trapframe->a2 = p->alarm.a2;
  p->trapframe->a3 = p->alarm.a3;
  p->trapframe->a4 = p->alarm.a4;
  p->trapframe->a5 = p->alarm.a5;
  p->trapframe->a6 = p->alarm.a6;
  p->trapframe->a7 = p->alarm.a7;
  p->trapframe->s2 = p->alarm.s2;
  p->trapframe->s3 = p->alarm.s3;
  p->trapframe->s4 = p->alarm.s4;
  p->trapframe->s5 = p->alarm.s5;
  p->trapframe->s6 = p->alarm.s6;
  p->trapframe->s7 = p->alarm.s7;
  p->trapframe->s8 = p->alarm.s8;
  p->trapframe->s9 = p->alarm.s9;
  p->trapframe->s10 = p->alarm.s10;
  p->trapframe->s11 = p->alarm.s11;
  p->trapframe->t3 = p->alarm.t3;
  p->trapframe->t4 = p->alarm.t4;
  p->trapframe->t5 = p->alarm.t5;
  p->trapframe->t6 = p->alarm.t6;

  // free it
  p->alarm.running = 0;

  return p->alarm.a0; // restore a0
}
