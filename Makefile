############################################################################
#
# Makefile fuer ytree
#
############################################################################


CC          ?= cc

#
# ADD_CFLAGS: Add -DVI_KEYS if you want vi-cursor-keys
#

DESTDIR     = /usr

ADD_CFLAGS  = # -DVI_KEYS

BINDIR      = $(DESTDIR)/bin
MANDIR      = $(DESTDIR)/share/man/man1
MANESDIR    = $(DESTDIR)/share/man/es/man1


# Default configuration for Linux (WSL/Ubuntu) with ncurses 6
COLOR       = -DCOLOR_SUPPORT 
#CLOCK	    = -DCLOCK_SUPPORT # Experimental!
READLINE    = -DREADLINE_SUPPORT
# Use -std=c99 or -std=gnu99 for modernization. -D_GNU_SOURCE is kept for glibc extensions.
CFLAGS      += -D_GNU_SOURCE $(COLOR) $(CLOCK) $(READLINE) $(ADD_CFLAGS)
LDFLAGS     += -lncurses -ltinfo -lreadline


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
OBJS	= archive.o archive_reader.o chgrp.o chmod.o chown.o clock.o color.o copy.o    \
	  delete.o dirwin.o disp.o edit.o error.o execute.o filespec.o      \
	  filewin.o freesp.o global.o group.o hex.o history.o init.o input.o keyhtab.o \
	  login.o main.o match.o mkdir.o move.o passwd.o pipe.o    \
	  print.o profile.o quit.o readtree.o rename.o rmdir.o  \
	  sort.o stat.o system.o tilde.o usermode.o util.o view.o 

# mktime.o and termcap.o removed from OBJS list

$(MAIN):	$(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS) $(LDFLAGS)

install:	$(MAIN)
	if [ ! -e $(BINDIR) ]; then mkdir -p $(BINDIR); fi
	install $(MAIN) $(BINDIR)
	gzip -9c ytree.1 > ytree.1.gz
	if [ ! -e $(MANDIR) ]; then mkdir -p $(MANDIR); fi
	install -m 0644 ytree.1.gz  $(MANDIR)/

uninstall:	clobber
	rm -f $(BINDIR)/$(MAIN)
	rm -f $(MANDIR)/ytree.1.gz
	rm -f $(MANESDIR)/ytree.1.es.gz

clean:
	rm -f core *.o *~ *.orig *.bak 
		
clobber:	clean
	rm -f $(MAIN) ytree.1.es.gz ytree.1.gz


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
input.o: config.h ytree.h tilde.h input.c
keyhtab.o: config.h ytree.h keyhtab.c
login.o: config.h ytree.h login.c
main.o: config.h ytree.h main.c
match.o: config.h ytree.h match.c
mkdir.o: config.h ytree.h mkdir.c
move.o: config.h ytree.h move.c
passwd.o: config.h ytree.h passwd.c
pipe.o: config.h ytree.h pipe.c
print.o: ytree.h print.c config.h
profile.o: config.h ytree.h profile.c
quit.o: config.h ytree.h quit.c
readtree.o: config.h ytree.h readtree.c
rename.o: config.h ytree.h rename.c
rmdir.o: config.h ytree.h rmdir.c
sort.o: config.h ytree.h sort.c
stat.o: config.h ytree.h stat.c
system.o: config.h ytree.h system.c
tilde.o: config.h tilde.h tilde.c
usermode.o: config.h ytree.h usermode.c
util.o: config.h ytree.h util.c
view.o: config.h ytree.h view.c