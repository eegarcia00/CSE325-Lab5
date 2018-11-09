#include "types.h"
#include "stat.h"
#include "user.h"

int x = 1;

void
test()
{
  printf(1, "%d free pages before fork is called\n", getNumFreePages());
  printf(1, "Parent & Child share global variable x\n");

  int pid = fork();
  if(pid == 0) {
    printf(1, "Child x = %d\n", x);
    printf(1, "%d free pages before any changes are made\n", getNumFreePages());

    x = 2;
 
    printf(1, "Child x = %d\n", x);
    printf(1, "%d free pages be after changes are made\n", getNumFreePages());
    exit();
  }

  printf(1, "Parent x = %d\n", x);
  wait();
  printf(1, "Parent x = %d\n", x);
  printf(1, "%d free pages after wait is called\n", getNumFreePages());
  return;
}


int 
main(void)
{
  printf(1, "Currently running Test...\n");
  test();
  printf(1, "Test complete.\n");

  exit();
}
