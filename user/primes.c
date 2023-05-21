// This is my primes.c implementation.

#include "kernel/types.h"
#include "user/user.h"

void 
sieve(int *left_neighbor)
{
  close(left_neighbor[1]); // never write to the left neighbor

  int prime;
  int have_data = read(left_neighbor[0], &prime, 4); // get the first number(4 bytes) which is a prime from left neighbor
  if(!have_data) { exit(0); }
  printf("prime %d\n", prime);

  int right_neighbor[2];
  pipe(right_neighbor);

  int n;
  if(fork() == 0) {
    sieve(right_neighbor);
  } else {
    while(read(left_neighbor[0], &n, 4)) {
      if (n % prime != 0) {
        write(right_neighbor[1], &n, 4); // send n to right neighbor
      }
    }
    close(left_neighbor[0]);
    close(right_neighbor[1]);
    wait(0);
  }
}

int
main(int argc, char *argv[])
{
  int p[2];
  pipe(p);
  
  if(fork() == 0) {
    sieve(p);
  } else {
    close(p[0]);
    for(int i = 2; i <= 35; i++) {
      write(p[1], &i, 4);
    }
    close(p[1]);
    wait(0);
  }

  exit(0);
}
