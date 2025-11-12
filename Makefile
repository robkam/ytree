############################################################################
#
# Makefile fuer ytree
#
############################################################################


CC          ?= cc
PANDOC      ?= pandoc

#
# ADD_CFLAGS: Add -DVI_KEYS if you want vi-cursor-keys
#

DESTDIR     = /usr

ADD_CFLAGS  = # -DVI_KEYS

BINDIR      = $(DESTDIR)/bin
MANDIR      = $(DESTDIR)/share/man/man1
MANSRC      = ytree.1.md


# Default configuration for Linux (WSL/Ubuntu) with ncurses 6
# NOTE: This build now requires libarchive.
# On Debian/Ubuntu, install with: sudo apt-get install libarchive-dev
COLOR       = -DCOLOR_SUPPORT 
#CLOCK	    = -DCLOCK_SUPPORT # Experimental!
READLINE    = -DREADLINE_SUPPORT
# Use -std=c99 or -std=gnu99 for modernization. -D_GNU_SOURCE is kept for glibc extensions.
CFLAGS      += -D_GNU_SOURCE -DHAVE_LIBARCHIVE $(COLOR) $(CLOCK) $(READLINE) $(ADD_CFLAGS)
LDFLAGS     += -lncurses -ltinfo -lreadline -larchive


# For systems requiring ncursesw (wide character support) use:
# COLOR       = -DCOLOR_SUPPORT 
# READLINE    = -DREADLINE_SUPPORT
# CFLAGS      = -D_GNU_SOURCE -DWITH_UTF8 $(ADD_CFLAGS) $(COLOR) $(CLOCK) $(READLINE)
# LDFLAGS     = -lncursesw -ltinfo -lreadline

# For simpler POSIX/BSD environments:
# CFLAGS      = $(ADD_CFLAGS) $(COLOR) $(CLOCK)
# LDFLAGS     = -lcurses


##############################################################################


MAIN    = ytree
OBJS	= display_utils.o owner_utils.o path_utils.o string_utils.o tree_utils.o \
	  archive.o archive_reader.o chgrp.o chmod.o chown.o clock.o color.o copy.o    \
	  delete.o dirwin.o disp.o edit.o error.o execute.o filespec.o      \
	  filewin.o freesp.o global.o group.o hex.o history.o init.o input.o keyhtab.o \
	  login.o main.o match.o mkdir.o move.o passwd.o pipe.o    \
	  profile.o quit.o readtree.o rename.o rmdir.o sort.o stat.o \
	  system.o usermode.o view.o 

$(MAIN):	$(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(LDFLAGS)

ytree.1: $(MANSRC)
	$(PANDOC) -s -t man $(MANSRC) -o ytree.1

install:	$(MAIN) ytree.1
	if [ ! -e $(BINDIR) ]; then mkdir -p $(BINDIR); fi
	install $(MAIN) $(BINDIR)
	gzip -9c ytree.1 > ytree.1.gz
	if [ ! -e $(MANDIR) ]; then mkdir -p $(MANDIR); fi
	install -m 0644 ytree.1.gz  $(MANDIR)/

uninstall:	clobber
	rm -f $(BINDIR)/$(MAIN)
	rm -f $(MANDIR)/ytree.1.gz

clean:
	rm -f core *.o *~ *.orig *.bak ytree.1
		
clobber:	clean
	rm -f $(MAIN) ytree.1.gz


##################################################

archive_reader.o: config.h ytree.h archive_reader.c
archive.o: config.h ytree.h archive.c
chgrp.o: config.h ytree.h chgrp.c
chmod.o: config.h ytree.h chmod.c
chown.o: config.h ytree.h chown.c
color.o: config.h ytree.h color.c
copy.o: config.h ytree.h copy.c
delete.o: config.h ytree.h delete.c
dirwin.o: config.h ytree.h dirwin.c
disp.o: config.h ytree.h patchlev.h disp.c
display_utils.o: config.h ytree.h display_utils.c
edit.o: config.h ytree.h edit.c
error.o: config.h ytree.h error.c patchlev.h
execute.o: config.h ytree.h execute.c
filespec.o: config.h ytree.h filespec.c
filewin.o: config.h ytree.h filewin.c
freesp.o: config.h ytree.h freesp.c
global.o: config.h ytree.h global.c
group.o: config.h ytree.h group.c
hex.o: config.h ytree.h hex.c
history.o: config.h ytree.h history.c
init.o: config.h ytree.h init.c
input.o: config.h ytree.h input.c
keyhtab.o: config.h ytree.h keyhtab.c
login.o: config.h ytree.h login.c
main.o: config.h ytree.h main.c
match.o: config.h ytree.h match.c
mkdir.o: config.h ytree.h mkdir.c
move.o: config.h ytree.h move.c
owner_utils.o: config.h ytree.h owner_utils.c
passwd.o: config.h ytree.h passwd.c
path_utils.o: config.h ytree.h path_utils.c
pipe.o: config.h ytree.h pipe.c
profile.o: config.h ytree.h profile.c
quit.o: config.h ytree.h quit.c
readtree.o: config.h ytree.h readtree.c
rename.o: config.h ytree.h rename.c
rmdir.o: config.h ytree.h rmdir.c
sort.o: config.h ytree.h sort.c
stat.o: config.h ytree.h stat.c
string_utils.o: config.h ytree.h string_utils.c
system.o: config.h ytree.h system.c
tree_utils.o: config.h ytree.h tree_utils.c
usermode.o: config.h ytree.h usermode.c
view.o: config.h ytree.h view.c