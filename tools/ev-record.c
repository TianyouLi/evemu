/*****************************************************************************
 *
 * evemu - Kernel device emulation
 *
 * Copyright (C) 2010-2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2010 Henrik Rydberg <rydberg@euromail.se>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#define _GNU_SOURCE
#include "evemu.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "evemu-opt.h"

#define INFINITE -1



FILE *output;

static int describe_device(int fd, FILE* fp)
{
	struct evemu_device *dev;
	int ret = -ENOMEM;

	dev = evemu_new(NULL);
	if (!dev)
		goto out;
	ret = evemu_extract(dev, fd);
	if (ret)
		goto out;

	evemu_write(dev, fp);
out:
	evemu_delete(dev);
	return ret;
}

static void handler (int sig __attribute__((unused)))
{
	fflush(output);
	if (output != stdout) {
		fclose(output);
		output = stdout;
	}
}


void dev_clean_all(int* fds, int max) {
  for (int i=0; i < max; i++) {
    if (fds != 0) {
      close(fds[i]);
      fds[i] =0;
    }
  }
}

int dev_init_device(char* dev_name) {
	char* device = dev_name;

	int fd = open(device, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "error: could not open device %s\n", device);
		return -1;
	}

  return fd;
}

int dev_init_mouse(char* dev_name, int x, int y) {
  return dev_init_device(dev_name);
}

int dev_init_all(int* fds, struct EvemuOptions* opts) {
  int offset =0;
  if (opts->mouse != NULL) {
    int fd = dev_init_mouse(opts->mouse, opts->mouseX, opts->mouseY);
    if (fd != -1) {
      fds[0] = fd;
      offset =1;
    } else {
      goto out;
    }
  }
  
  for (int i =0; i < opts->device_count; i++) {
    int fd = dev_init_device(opts->devices[i]);
    if (fd != -1) {
      fds[i+offset] = fd;
    } else {
      goto out;
    }
  }

  return 0;
  
 out:
  dev_clean_all(fds, opts->device_count +1);
  return -1;
}  

int dev_describe_mouse(int id, int fd, char* dev_name, int x, int y, FILE* fp) {
  // Every device start from [Device_Begin], and end with [Device_End]
  fprintf(fp, "\n[Device_Begin]\n");
  fprintf(fp, "id = %d\n", id);
  fprintf(fp, "type = mouse\n");
  fprintf(fp, "X = %d\n", x);
  fprintf(fp, "Y = %d\n", y);

  int ret = describe_device(fd, fp);
  
  fprintf(fp, "\n[Device_End]\n");

  return ret;
}

int dev_describe_device(int id, int fd, char* dev_name, FILE* fp) {
  fprintf(fp, "\n[Device_Begin]\n");
  fprintf(fp, "id = %d\n", id);
  fprintf(fp, "type = unknown\n");
  fprintf(fp, "name = %s\n", dev_name);

  int ret = describe_device(fd, fp);
  
  fprintf(fp, "\n[Device_End]\n");

  return ret;
}

int dev_describe_all(int* fds, struct EvemuOptions* opts, FILE* fp) {
  int offset = 0;
  int ret = 0;
  if (opts->mouse != NULL) {
    offset = 1;
    ret = dev_describe_mouse(0, fds[0], opts->mouse, opts->mouseX, opts->mouseY, fp);
  }

  if (ret) 
    return ret;
  
  for (int i =0; i < opts->device_count; i++) {
    ret = dev_describe_device(i+offset, fds[i+offset], opts->devices[i], fp);
    if (ret) {
      break;
    }
  }
  
  return ret;
}

int sig_handler_install() {
  struct sigaction act;
	memset (&act, '\0', sizeof(act));
	act.sa_handler = &handler;

	if (sigaction(SIGTERM, &act, NULL) < 0) {
		fprintf (stderr, "Could not attach TERM signal handler.\n");
		return -1;
	}
	if (sigaction(SIGINT, &act, NULL) < 0) {
		fprintf (stderr, "Could not attach INT signal handler.\n");
		return -1;
	}

  return 0;
}

int dev_grab_all(int* fds, struct EvemuOptions* opts) {
  int dev_count = opts->mouse == NULL? opts->device_count : opts->device_count+1;
  
  for (int i =0; i < dev_count; i++) {
    int fd = fds[i];
#ifdef EVIOCSCLOCKID
    int clockid = CLOCK_MONOTONIC;
    ioctl(fd, EVIOCSCLOCKID, &clockid);
#endif
    if (ioctl(fd, EVIOCGRAB, (void*)1) < 0) {
      fprintf(stderr, "error: this device is grabbed and I cannot record events\n");
      fprintf(stderr, "see the evemu-record man page for more information\n");
      return -1;
    } 
  }
  
  return 0;
}

int main(int argc, char *argv[])
{
  // Parse options
  struct EvemuOptions opts; 
  if (!evemu_parse_options(argc, argv, &opts)) {
    return -1;
  }

  // Create device fd, and initialize it
  int fds[MAX_DEVICES+1];
  memset(fds, 0, sizeof(fds));
  
  // Create devices and write to output file. If any of devices failed
  // to initialize, then abort the whole application.
  // Note: from now on, fds was filled with values, need to use goto out
  // to clean up resource if error happens
	if (dev_init_all(fds, &opts))
    return -1;

  // Write fds to stdout, make sure it can be read back during replay 
  if (dev_describe_all(fds, &opts, stdout))
    goto out;

  // Grab all devices for record
  if (dev_grab_all(fds, &opts)) 
    goto out;
  
  // Install sig handler
  output = stdout;
  if (sig_handler_install())
    goto out;

  // We now start recording
  int count = opts.mouse == NULL? opts.device_count : opts.device_count +1;
  if (evemu_record_all(stdout, fds, count, INFINITE))
    goto out;

out:
  dev_clean_all(fds, MAX_DEVICES+1);
	
	return 0;
}
