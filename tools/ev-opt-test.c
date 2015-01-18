#include <stdio.h>
#include "evemu-opt.h"


/*testing*/

static void evemu_dump_options(struct EvemuOptions* opts) {
  if (opts == NULL) return;

  if (opts->mouse) {
    printf("Mouse is %s\n", opts->mouse);
  }
  if (opts->mouseX) {
    printf("Mouse X is %d\n", opts->mouseX);
  }
  if (opts->mouseY) {
    printf("Mouse Y is %d\n", opts->mouseY);
  }

  printf("Device count is %d\n",  opts->device_count);
  for (int i = 0 ; i < opts->device_count; i++) {
    printf("Device %d is %s\n", i, opts->devices[i]);
  }
}

int main(int argc, char* argv[])
{
  struct EvemuOptions opts;

  printf("\n---------------------------------\n");
  for (int i = 0; i < argc; i++) {
    printf("%s ", argv[i]);
  }
  
  printf("\n---------------------------------\n\n");
  evemu_parse_options(argc, argv, &opts);
  
  printf("\n---------------------------------\n\n");
  evemu_dump_options(&opts);
  return 0;
}
