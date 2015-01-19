noinst_PROGRAMS = \
	evemu-echo \
	ev-opt-test

bin_PROGRAMS = \
	evemu-describe \
	evemu-device \
	evemu-record \
	evemu-play \
	evemu-event \
	ev-record \
	ev-replay

LDFLAGS = -all-static

AM_CPPFLAGS =-I$(top_srcdir)/src/

AM_LDFLAGS = $(top_builddir)/src/libevemu.la

evemu_devices_SOURCES = find_event_devices.c find_event_devices.h
evemu_describe_SOURCES = evemu-record.c $(evemu_devices_SOURCES)
evemu_record_SOURCES = $(evemu_describe_SOURCES)

evemu_event_CFLAGS = $(LIBEVDEV_CFLAGS)
evemu_event_LDADD = $(LIBEVDEV_LIBS)

ev_tool_SOURCES = evemu-opt.c evemu-opt.h $(evemu_devices_SOURCES)
ev_tool_CFLAGS = -std=c99

ev_opt_test_SOURCES = ev-opt-test.c $(ev_tool_SOURCES) 
ev_opt_test_CFLAGS = $(ev_tool_CFLAGS)

ev_record_SOURCES = ev-record.c $(ev_tool_SOURCES) 
ev_record_CFLAGS = $(ev_tool_CFLAGS)

ev_replay_SOURCES = ev-replay.c $(ev_tool_SOURCES)
ev_replay_CFLAGS = $(ev_tool_CFLAGS)

# man page generation
if HAVE_DOCTOOLS
# actual man pages
man_pages_sources = evemu-describe.txt evemu-device.txt
# shadow man pages
man_pages_shadows = evemu-record.1 evemu-play.1 evemu-event.1

man_pages = $(man_pages_sources:.txt=.1) $(man_pages_shadows)

$(man_pages_shadows): $(man_pages_sources:.txt=.1)

SUFFIXES = .1 .txt .xml

.xml.1:
	$(AM_V_GEN)$(XMLTO) man $<

.txt.xml:
	$(AM_V_GEN)$(ASCIIDOC) -b docbook -d manpage -o $@ $<

EXTRA_DIST = $(man_pages_sources)
dist_man_MANS = $(man_pages)
CLEANFILES = $(dist_man_MANS)
MAINTAINERCLEANFILES = $(man_pages) *.xml
endif
