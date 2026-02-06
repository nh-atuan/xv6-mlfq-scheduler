// simple_test.c - Minimal test for getpstat syscall
#include "kernel/types.h"
#include "user/user.h"

int main(void)
{
  printf("Testing getpstat syscall...\n");
  
  // Allocate pstat on heap using malloc
  char *buf = malloc(4096);  // Allocate 4KB
  if(buf == 0) {
    printf("malloc failed\n");
    exit(1);
  }
  
  int ret = getpstat((void*)buf);
  printf("getpstat returned: %d\n", ret);
  
  if(ret == 0) {
    printf("SUCCESS!\n");
  } else {
    printf("FAILED!\n");
  }
  
  free(buf);
  exit(0);
}
