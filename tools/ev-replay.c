#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "evemu.h"
#include "evemu-opt.h"


// a line is empty if
//   whitespace | comments + '\n'
// return zero for a empty line, otherwise non-zero for a non-empty line
int is_empty(char* line, int size) {
  int empty = 0;
  for (int i =0; i < size; i++) {
    char c = line[i];
    switch(c) {
    case ' ':
    case '\t':
    case '\n':
      empty = 0;
      break;
    case '#':
      return 0;
    default:
      return 1;
    }
  }

  return empty;
}

// read a non-empty, none comments line
// return non-zero for success, other value for failure
static int read_line(char** line, size_t* size, FILE* fp) {
  int ret = 0;
  while (1) {
    ret = getline(line, size, fp);
    if (ret == -1) {
      ret =0;
      break;
    } 
    if (!is_empty(*line, ret)) {
      continue;
    } else {
      break;
    }
  }

  return ret;
}

enum Mode {
  None,
  Enter,
  Leave
};

typedef int (*FN_Read_Content)(char* line, struct EvemuOptions* opts);


int read_None(enum Mode* prev, enum Mode* current) {
  int ret =0;
  switch(*current) {
  case None:
    ret = -1;
    break;
  case Enter:
    *prev = *current;
    break;
  case Leave:
    ret = -1;
    break;
  }
  return ret;
}

int read_Enter(enum Mode* prev, enum Mode* current, char* line, struct EvemuOptions* opts, FN_Read_Content func) {
  int ret =0;
  switch(*current) {
  case None:
    ret = func(line, opts);
    break;
  case Enter:
    ret = -1;
    break;
  case Leave:
    *prev = *current;
    break;
  }
  return 0;
}

int read_Leave(enum Mode* prev, enum Mode* current) {
  int ret =0;
  switch(*current) {
  case None:
    ret = -1;
    break;
  case Enter:
    *prev = *current;
    break;
  case Leave:
    ret = -1;
    break;
  }
  return 0;
}

// Expect to read [Device Begin] ... [Device End]
// should no other non-empty content
// 0 for success.
int read_section(FILE* fp, struct EvemuOptions* opts,
                 FN_Read_Content func, char* begin, char* end) {
  char* line = NULL;
  size_t size = 0;
  int ret =0;
  
  // get [Device Begin]
  enum Mode current, prev;
  current = prev = None;
  while (read_line(&line, &size, fp)) {
    if (!strcmp(line, begin)) {
      current = Enter;
    } else if (!strcmp(line, end)) {
      current = Leave;
    } else {
      current = None;
    }
    
    switch(prev) {
    case None:
      ret = read_None(&prev, &current);
      if (ret !=0)
        goto out;
      break;
    case Enter:
      ret = read_Enter(&prev, &current, line, opts, func);
      if (ret !=0)
        goto out;
      break;
    case Leave:
      ret = read_Leave(&prev, &current);
      goto out;
      break;
    }

    if (current == Leave) {
      break;
    }
  }

 out:
  free(line);
  return ret;
}

int read_pair(char* line, char** key, char** value) {
  *key = *value = NULL;

  return sscanf(line, "%ms = %ms\n", key, value) -2;
}

void free_pair(char** key, char** value) {
  if (*key != NULL)
    free(*key);
  if (*value != NULL)
    free(*value);

  *key = *value = NULL;
}

int read_devices_content(char* line, struct EvemuOptions* opts) {
  char* key = NULL;
  char* value = NULL;
  read_pair(line, &key, &value);
  
  if (!strcmp("count", key)) {
    opts->device_count = atoi(value);
  }

  free_pair(&key, &value);
  return 0;
}

// current context, should be handled more graceful, currently use 'static'
static FILE* device_tmpfiles[MAX_DEVICES+1];
static int device_id = -1;
static char* device_type = NULL;

int read_device_id(char* value, struct EvemuOptions* opts) {
  int ret = 0;
  int id = atoi(value);
  if (id <0 || id > opts->device_count-1) {
    ret = -1;
    goto out;
  } 

  // there should no duplicated id
  if (device_id == id) {
    ret = -1;
    goto out;
  }
    
  device_id = id;
  // there should no tmp file created once context id changed
  if (device_tmpfiles[device_id] != NULL) {
    ret = -1;
    goto out;
  }
  device_tmpfiles[device_id] = tmpfile();

 out:
  return ret;
}

int read_device_type(char* value, struct EvemuOptions* opts) {
  int ret = 0;
  if (strcmp("mouse", value) && strcmp("unknown", value)) {
    ret = -1;
  } else {
    free(device_type);
    device_type = strdup(value);
  }

  if (!strcmp("mouse", device_type)) {
    opts->mouse = strdup(device_type);
  }
  
  return ret;
}

int read_device_X(char* value, struct EvemuOptions* opts) {
  int ret = 0;
  if (strcmp("mouse", device_type)) {
    ret = -1;
  } else {
    opts->mouseX = atoi(value);
  }

  return ret;
}

int read_device_Y(char* value, struct EvemuOptions* opts) {
  int ret = 0;
  if (strcmp("mouse", device_type)) {
    ret = -1;
  } else {
    opts->mouseY = atoi(value);
  }

  return ret;
}

int read_device_content(char* line, struct EvemuOptions* opts) {
  // local 
  char* key = NULL;
  char* value = NULL;
  
  int ret = read_pair(line, &key, &value);

  if (ret != 0) {
    if (device_id != -1 && device_type != NULL
        && device_tmpfiles[device_id] != NULL) {
      fprintf(device_tmpfiles[device_id], "%s", line);
    }  
    goto out;
  }
  
  if (!strcmp("id", key)) {
    ret = read_device_id(value, opts);
    goto out;
  }

  if (!strcmp("type", key)) {
    ret = read_device_type(value, opts);
    goto out;
  }

  if (!strcmp("X", key)) {
    ret = read_device_X(value, opts);
    goto out;
  }

  if (!strcmp("Y", key)) {
    ret = read_device_Y(value, opts);
    goto out;
  }
  
 out:
  free_pair(&key, &value);
  return ret;  
}

void device_tmpfiles_dump() {
  for (int i =0; i < MAX_DEVICES +1; i++) {
    FILE* fp = device_tmpfiles[i];
    if (fp != NULL) {
      fprintf(stderr, "--------------------------\n");
      rewind(fp);
      int read = 0;
      char* line = NULL;
      size_t len =0;
      while((read = getline(&line, &len, fp)) != -1) {
        fprintf(stderr, "%s", line);
      }
      free(line);
    }
  }
}

int main(int argc, char* argv[])
{
  // read file from stdin
  FILE* fp = stdin;
  struct EvemuOptions opts;
  memset(&opts, 0, sizeof(opts));

  // read devices section
  static char Devices_Begin[] = "[Devices Begin]\n";
  static char Devices_End[]   = "[Devices End]\n";
  read_section(stdin, &opts, read_devices_content, Devices_Begin, Devices_End);

  // read device sections
  static char Device_Begin[] = "[Device Begin]\n";
  static char Device_End[]   = "[Device End]\n";
  for (int i =0; i < opts.device_count; i++) {
    read_section(stdin, &opts, read_device_content, Device_Begin, Device_End);
  }

  
  // dump opts
  evemu_dump_options(&opts);

  // dump temp files
  device_tmpfiles_dump();
  // success
  return 0;
}
