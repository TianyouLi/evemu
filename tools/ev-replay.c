#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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

struct UinputDevice {
  int                  id;
  int                  isMouse;
  FILE*                descriptor;
  struct evemu_device* device;
  int                  fd;
  char*                node_name;
  char*                device_name;
};

// current context, should be handled more graceful, currently use 'static'
static struct UinputDevice udevice[MAX_DEVICES+1];
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
  if (udevice[device_id].descriptor != NULL) {
    ret = -1;
    goto out;
  }
  udevice[device_id].descriptor = tmpfile();
  udevice[device_id].id = device_id;
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
    udevice[device_id].isMouse = 1;
  } else {
    udevice[device_id].isMouse = 0;
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
        && udevice[device_id].descriptor != NULL) {
      fprintf(udevice[device_id].descriptor, "%s", line);
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
    FILE* fp = udevice[i].descriptor;
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

int create_uinput_device(struct UinputDevice* ud) {
  int ret =0;
  
  ud->device = evemu_new(NULL);
  if (!ud->device) {
    ret = -1;
    goto out;
  }

  rewind(ud->descriptor);
  ret = evemu_read(ud->device, ud->descriptor);
  if (ret <=0) {
    ret = -1;
    goto out;
  }

  if (strlen(evemu_get_name(ud->device)) == 0) {
		char name[64];
		sprintf(name, "evemu-%d", getpid());
		evemu_set_name(ud->device, name);
	}
  ud->device_name = (char*)evemu_get_name(ud->device);
  
  ret = evemu_create_managed(ud->device);
	if (ret < 0)
		goto out;

  const char *device_node = evemu_get_devnode(ud->device);
	if (!device_node) {
		fprintf(stderr, "can not determine device node\n");
		ret = -1;
    goto out;
	}
  ud->node_name = (char*)device_node;
  
	int fd = open(device_node, O_WRONLY);
	if (fd < 0)	{
		fprintf(stderr, "error %d opening %s: %s\n",
			errno, device_node, strerror(errno));
    ret = -1;
	  goto out;
	}
  ud->fd = fd;
  
 out:
  return ret;
}

int create_uinput_devices(struct EvemuOptions* opts, struct UinputDevice* uds) {
  int ret = 0;
  
  for (int i =0; i < opts->device_count; i++) {
    ret = create_uinput_device(uds +i);
    if (ret) break;
  }
  
  return ret;
}

int destroy_uinput_device(struct UinputDevice* ud) {
  int ret =0;

  if (ud->device != NULL) {
    evemu_destroy(ud->device);
    ud->device = NULL;
    if (ud->fd != -1) {
      close(ud->fd);
    }
  }
  
  return ret;
}

int destroy_uinput_devices(struct EvemuOptions* opts, struct UinputDevice* uds) {
  int ret =0;

  for (int i =0; i < opts->device_count; i++) {
    ret = destroy_uinput_device(uds +i);
    ret |= ret;
  }
  
  return ret;
}


void uinput_device_dump(struct UinputDevice* ud) {
  fprintf(stderr, "id: %d\n", ud->id);
  fprintf(stderr, "isMouse: %d\n", ud->isMouse);
  fprintf(stderr, "node: %s\n", ud->node_name);
  fprintf(stderr, "name: %s\n", ud->device_name);
}


void uinput_devices_dump(struct EvemuOptions* opts, struct UinputDevice* uds) {
  for (int i =0; i < opts->device_count; i++) {
    fprintf(stderr, "--------------device %d-----------\n",i);
    uinput_device_dump(uds +i);
  }
}

int evemu_read_event_with_id(FILE *fp, struct input_event *ev)
{
	unsigned long sec;
	unsigned usec, type, code;
	int value;
	int matched = 0;
	char *line = NULL;
	size_t size = 0;
  int id = -1;
  
	do {
		if (!read_line(&line, &size, fp))
			goto out;
	} while(strlen(line) > 2 && strncmp(line, "E:", 2) != 0);

	if (strlen(line) <= 2 || strncmp(line, "E:", 2) != 0)
		goto out;

	matched = sscanf(line, "E: %d %lu.%06u %04x %04x %d\n",
                   &id, &sec, &usec, &type, &code, &value);
	if (matched != 6) {
		fprintf(stderr, "Invalid event format: %s\n", line);
		return -1;
	}

	ev->time.tv_sec = sec;
	ev->time.tv_usec = usec;
	ev->type = type;
	ev->code = code;
	ev->value = value;

out:
	free(line);
	return id;
}

int evemu_read_event_with_id_realtime(FILE *fp, struct input_event *ev,
			      struct timeval *evtime)
{
	unsigned long usec;
	int ret;

	ret = evemu_read_event_with_id(fp, ev);
	if (ret < 0)
		return ret;

	if (evtime) {
		if (!evtime->tv_sec)
			*evtime = ev->time;
		usec = 1000000L * (ev->time.tv_sec - evtime->tv_sec);
		usec += ev->time.tv_usec - evtime->tv_usec;
		if (usec >= 0) {
			usleep(usec);
			*evtime = ev->time;
		}
	}

	return ret;
}

int evemu_play_with_id(FILE *fp, struct UinputDevice* uds)
{
	struct input_event ev;
	struct timeval evtime;
	int ret;
	struct evemu_device *dev;
  int fd;
  int id;
  
	memset(&evtime, 0, sizeof(evtime));
	while ((id = evemu_read_event_with_id_realtime(fp, &ev, &evtime)) >= 0) {
    dev = uds[id].device;
    fd = uds[id].fd;
		if (dev &&
		    (ev.type != EV_SYN || ev.code != SYN_MT_REPORT) &&
		    !evemu_has_event(dev, ev.type, ev.code))
			fprintf(stderr, "Warn: incompatible event: %d, %d\n",ev.type, ev.code);
		ret = write(fd, &ev, sizeof(ev));
	}

	return ret;
}


static int write_event(int fd, int type, int code, int value) {
    struct input_event ev;
    ev.type = type;
    ev.code = code;
    ev.value= value;

    return write(fd, &ev, sizeof(ev)); 
}

int read_play_events(FILE* fp, struct EvemuOptions* opts, struct UinputDevice* uds, char* section) {
  int ret =0;
  char* line = NULL;
  size_t size = 0;

  // Get event section indicator
  while (read_line(&line, &size, fp)) {
    if (!strcmp(line, section)) {
      break;
    }
  }

  // initialize mouse position if there have
  if (opts->mouse != NULL) {
    for (int i =0 ; i < opts->mouseX; i++) {
      write_event(uds[0].fd, EV_REL, REL_X, 1);
      write_event(uds[0].fd, EV_SYN, SYN_REPORT, 0);
      usleep(500);
    }
    for (int i =0; i < opts->mouseY; i++) {
      write_event(uds[0].fd, EV_REL, REL_Y, 1);
      write_event(uds[0].fd, EV_SYN, SYN_REPORT, 0);
      usleep(500);
    }
  }
  
  // read event and replay
  ret = evemu_play_with_id(fp, uds);
  
 out:
  free(line);
  return ret;
}

static int verbose = 0;

int main(int argc, char* argv[])
{
  // read file from stdin
  FILE* fp = stdin;
  struct EvemuOptions opts;
  memset(&opts, 0, sizeof(opts));

  // read devices section
  static char Devices_Begin[] = "[Devices Begin]\n";
  static char Devices_End[]   = "[Devices End]\n";
  read_section(fp, &opts, read_devices_content, Devices_Begin, Devices_End);

  // read device sections
  static char Device_Begin[] = "[Device Begin]\n";
  static char Device_End[]   = "[Device End]\n";
  for (int i =0; i < opts.device_count; i++) {
    read_section(fp, &opts, read_device_content, Device_Begin, Device_End);
  }

  // Now all device related descriptions are loaded into temp file device_tmpfiles
  // and the number devices was stored in opts.device_count. Time to create uinput
  // device... Remember if opts.mouse != NULL, the first device should be mouse
  create_uinput_devices(&opts, udevice);
  // dump udevices
  if (verbose)
    uinput_devices_dump(&opts, udevice);
  
  
  // Read all recorded events and replay
  static char Events[] = "[Events]\n";
  read_play_events(fp, &opts, udevice, Events);

  
  // Destroy all udevices
  destroy_uinput_devices(&opts, udevice);

  if (verbose) {
    // dump opts
    evemu_dump_options(&opts);

    // dump temp files
    device_tmpfiles_dump();
  }
  
  // success
  return 0;
}
