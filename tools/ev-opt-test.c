#include <stdio.h>
#include "evemu-opt.h"


/*testing*/

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
