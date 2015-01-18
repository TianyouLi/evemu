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

static int describe_device(int fd)
{
	struct evemu_device *dev;
	int ret = -ENOMEM;

	dev = evemu_new(NULL);
	if (!dev)
		goto out;
	ret = evemu_extract(dev, fd);
	if (ret)
		goto out;

	evemu_write(dev, stdout);
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


int main(int argc, char *argv[])
{
	int fd;
	struct sigaction act;
  struct EvemuOptions opts;

  if (!evemu_parse_options(argc, argv, &opts)) {
    return -1;
  }
  

	/* char* device = (argc < 2) ? find_event_devices() : strdup(argv[1]); */

	/* if (device == NULL) { */
	/* 	fprintf(stderr, "Usage: %s <device> [output file]\n", argv[0]); */
	/* 	return -1; */
	/* } */
	/* fd = open(device, O_RDONLY | O_NONBLOCK); */
	/* if (fd < 0) { */
	/* 	fprintf(stderr, "error: could not open device\n"); */
	/* 	return -1; */
	/* } */

	memset (&act, '\0', sizeof(act));
	act.sa_handler = &handler;

	if (sigaction(SIGTERM, &act, NULL) < 0) {
		fprintf (stderr, "Could not attach TERM signal handler.\n");
		return 1;
	}
	if (sigaction(SIGINT, &act, NULL) < 0) {
		fprintf (stderr, "Could not attach INT signal handler.\n");
		return 1;
	}

	if (argc < 3)
		output = stdout;
	else {
		output = fopen(argv[2], "w");
		if (!output) {
			fprintf(stderr, "error: could not open output file");
		}
	}

	if (describe_device(fd)) {
		fprintf(stderr, "error: could not describe device\n");
		goto out;
	}

#ifdef EVIOCSCLOCKID
  int clockid = CLOCK_MONOTONIC;
  ioctl(fd, EVIOCSCLOCKID, &clockid);
#endif
  if (ioctl(fd, EVIOCGRAB, (void*)1) < 0) {
    fprintf(stderr, "error: this device is grabbed and I cannot record events\n");
    fprintf(stderr, "see the evemu-record man page for more information\n");
    return -1;
  } else
    ioctl(fd, EVIOCGRAB, (void*)0);

  fprintf(output,  "################################\n");
  fprintf(output,  "#      Waiting for events      #\n");
  fprintf(output,  "################################\n");
  if (evemu_record(output, fd, INFINITE))
    fprintf(stderr, "error: could not describe device\n");


out:
	//free(device);
	close(fd);
	if (output != stdout) {
		fclose(output);
		output = stdout;
	}
	return 0;
}
