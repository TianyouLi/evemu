#ifndef __EVEMU_OPT_H__
#define __EVEMU_OPT_H__

#define MAX_DEVICES 10

struct EvemuOptions {
  char* mouse;
  int   mouseX;
  int   mouseY;
  int   device_count;
  char* devices[MAX_DEVICES]; 
};

/**
 * Parse options, return 0 if failed, and print options.
 *
 * [input] argc: argument count, usually getting from main
 * [input] argv: argument list, usually getting from main
 * [output] opts: hold parsing results
 */
int
evemu_parse_options(int argc, char* argv[], struct EvemuOptions* opts);

#endif
