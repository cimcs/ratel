/* Example of using sigaction() to setup a signal handler
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define __USE_GNU
#include <ucontext.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
 
static jmp_buf ctx;
static int locked;

// This function will handle a signal.
static void handle_sig(int sig, siginfo_t *info, void *context)
{
  switch (sig)
  {
    case SIGABRT:
      {
        fputs("Catch a SIGABRT\n", stdout);
        locked = 0;

        longjmp(ctx, 2);
        break;
      }
    
  }
}

int register_sig(struct sigaction *sAct)
{
  if (sigaction(SIGABRT, sAct, NULL) < 0) {
    perror ("sigaction error: ");
    return (-1);
  }

  return 1;
}

int testing(int myPID, struct sigaction *sAct)
{
  /* START TESTING */
  
  /* SIGABRT 6 */
  locked = 1;

  if (0 == setjmp(ctx)) 
  {
    // abort();
    raise(SIGABRT);
  }
  else
    printf("Continue ...\n");

  while (locked) {}

  return 1;
}

int main(int argc, char **argv) 
{
  struct sigaction sAct;
  pid_t myPID;

  memset(&sAct, 0, sizeof(sAct));

  // Specify that we will use a signal handler that takes three arguments
  // instead of one, which is the default.
  sAct.sa_flags = SA_SIGINFO;

  // Indicate which function is the signal handler.
  sAct.sa_sigaction = handle_sig;

  myPID = getpid();
  printf("\nPID = %d\n", myPID);
  
  register_sig(&sAct);

  testing(myPID, &sAct);

  printf("Exit\n");

  return 0;
}

