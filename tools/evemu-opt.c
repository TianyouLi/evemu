#define _GNU_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "evemu-opt.h"

struct option static evemu_options[] = {
  {"mouse",    required_argument, 0, 0},
  {"mouse-x",  required_argument, 0, 0},
  {"mouse-y",  required_argument, 0, 0},
  {"device",   required_argument, 0, 0},
  {0,          0,                 0, 0}
};

static unsigned int evemu_options_count()
{
  return sizeof(evemu_options) / sizeof(struct option) -1;
}

enum EvemuOptionType {
  None,
    Mouse,
    MouseX,
    MouseY,
    Device
};

static int evemu_option_type(int index, enum EvemuOptionType* opt_type)
{
  switch (index) {
  case 0:
  case 'm':
    *opt_type = Mouse;
    break;
  case 1:
  case 'x':
    *opt_type = MouseX;
    break;
  case 2:
  case 'y':
    *opt_type = MouseY;
    break;
  case 3:
  case 'd':
    *opt_type = Device;
    break;
  default:
    return 0;
  }
}

static int evemu_update_options(int index, char* arg, struct EvemuOptions* opts)
{
  enum EvemuOptionType opt_type = None;
  if (!evemu_option_type(index, &opt_type)) {
    return 0;
  }

  switch (opt_type) {
  case Mouse:
    if (opts->mouse) {
      printf("Mouse device %s has already been specified.\n"
             "Only one mouse device can be specified by -m or --mouse.\n", arg);
      return 0;
    }
    opts->mouse = arg;
    break;
  case MouseX:
    if (opts->mouseX) {
      printf("Mouse X %s has already been specified.\n"
             "Only one mouse device can be specified by -x or --mouse-x.\n", arg);
      return 0;
    }
    opts->mouseX = atoi(arg);
    break;
  case MouseY:
    if (opts->mouseY) {
      printf("Mouse Y %s has already been specified.\n"
             "Only one mouse device can be specified by -y or --mouse-y.\n", arg);
      return 0;
    }
    opts->mouseY = atoi(arg);
    break;
  case Device:
    if (opts->device_count < MAX_DEVICES) {
      opts->devices[opts->device_count] = arg;
      opts->device_count++;
    } else {
      printf("You can not specify the number of --device greater than %d.\n", MAX_DEVICES);
      return 0;
    }
    break;
  default:
    return 0;
  }

  return 1;
}

void evemu_print_options() {
  static char* opt_desc[] = {
    "-m",
    "--mouse",
    "  Mouse input device node path, for example, /dev/input/event12",
    "",
    "-x",
    "--mouse-x",
    "  Mouse initial X axis offset, for example, 100 or -100",
    "",
    "-y",
    "--mouse-y",
    "  Mouse initial Y axis offset, for example, 100 or -100",
    "",
    "-d",
    "--device",
    "  Other input device node path, for example, /dev/input/event10.",
    "  You can specify multiple input device by using multiple -d options.",
    ""
  };

  char* prefix="\t";
  char* surfix="\n";

  for (int i=0; i< sizeof(opt_desc) / sizeof(char*); i++) {
    printf("%s%s%s", prefix, opt_desc[i], surfix);
  }
}

int evemu_parse_options(int argc, char* argv[], struct EvemuOptions* opts)
{
  if (opts == NULL)
    return 0;

  int c = 0;
  memset(opts, 0, sizeof(*opts));
  do {
    int option_index = 0;
    c = getopt_long(argc, argv, "m:d:x:y:", evemu_options, &option_index);

    switch(c) {
    case 0:
      if (!evemu_update_options(option_index, optarg, opts))
        return 0;
      break;
    case 'm':
    case 'd':
    case 'x':
    case 'y':
      if (!evemu_update_options(c, optarg, opts))
        return 0;
      break;
    case -1:
      break;
    default:
      evemu_print_options();
      return 0;
    }
  } while (c != -1);

  if (optind < argc) {
    evemu_print_options();
    return 0;
  } 
  
  return 1;
}



#if 0
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
  printf("\n---------------------------------\n");
  evemu_parse_options(argc, argv, &opts);
  evemu_dump_options(&opts);
  
  return 0;
}

#endif

