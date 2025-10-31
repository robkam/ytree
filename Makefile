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


# Uncomment the lines for your system (default is linux)

###########################
# LINUX (tested with ncurses 6)
###########################
COLOR       = -DCOLOR_SUPPORT 
#CLOCK	    = -DCLOCK_SUPPORT # Experimental!
READLINE    = -DREADLINE_SUPPORT
CFLAGS      += -D_GNU_SOURCE $(COLOR) $(CLOCK) $(READLINE) $(ADD_CFLAGS)
LDFLAGS     += -lncurses -ltinfo -lreadline

###########################
# LINUX with Wide character support (tested with ncurses 6)
# Requires libncursesw!
###########################
#COLOR       = -DCOLOR_SUPPORT 
##CLOCK	    = -DCLOCK_SUPPORT # Experimental!
#READLINE    = -DREADLINE_SUPPORT
#CFLAGS      = -D_GNU_SOURCE -DWITH_UTF8 $(ADD_CFLAGS) $(COLOR) $(CLOCK) $(READLINE)
#LDFLAGS     = -lncursesw -ltinfo -lreadline

###########################
# LINUX without libreadline (tested with ncurses 6)
###########################
#COLOR       = -DCOLOR_SUPPORT 
#CLOCK	     = -DCLOCK_SUPPORT # Experimental!
#CFLAGS      = $(ADD_CFLAGS) $(COLOR) $(CLOCK)
#LDFLAGS     = -lncurses -ltinfo

###########################
# LINUX (X11 using PDCurses) thanks to  Mark Hessling
# see http://www.lightlink.com/hessling/
###########################
#COLOR       = -DCOLOR_SUPPORT -DXCURSES
#CLOCK       = -DCLOCK_SUPPORT # Experimental!
#CFLAGS      = $(ADD_CFLAGS) $(COLOR) $(CLOCK)
#LDFLAGS     = -lXCurses -L/usr/X11R6/lib -lXaw -lXmu -lXt -lX11 -lSM -lICE -lXext

###########################
# The Hurd (GNU) (tested with ncurses 5.0)
###########################
#COLOR       = -DCOLOR_SUPPORT
##CLOCK      = -DCLOCK_SUPPORT # Experimental!
#CFLAGS      = $(ADD_CFLAGS) $(COLOR) $(CLOCK)
#LDFLAGS     = -lncurses

###########################
# OpenBSD / NetBSD
###########################
#COLOR       = -DCOLOR_SUPPORT 
##CLOCK	     = -DCLOCK_SUPPORT # Experimental!
#READLINE    = -DREADLINE_SUPPORT
#CFLAGS      = $(ADD_CFLAGS) $(COLOR) $(CLOCK)
#LDFLAGS     = -lcurses -lcompat -ledit
#MANDIR      = /usr/share/man/man1
#BINDIR      = /usr/local/bin


###########################
# FreeBSD (Thanks to Peter Brevik)
###########################
#CFLAGS      = -DCOLOR_SUPPORT -DNCURSES
#LDFLAGS     = -lncurses -lcompat

###########################
# MAC OS X
###########################
#CFLAGS      = -DCOLOR_SUPPORT -DNCURSES
#LDFLAGS     = -lncurses 

###########################
# DOS/Windows with DJGPP
###########################
#CC          = gcc
#CFLAGS      = $(ADD_CFLAGS) $(COLOR)
#LDFLAGS     = -lpdcurses -lpanel -lreadline

###########################
# NeXT (Thanks to fifi)
###########################
#CFLAGS     = -DTERMCAP $(ADD_CFLAGS)
#LDFLAGS    = -lcurses -ltermcap

############################
# UNIX SVR3 (or SCO3.2v4.2)
############################
#CFLAGS     = -DSVR3 $(ADD_CFLAGS)
#LDFLAGS    = -lcurses -lPW # -lc_s

############################
# SCO OpenServer "SCO_SV s90909 3.2 5.0.5 i386" (Thanks to BKS)
############################
#CFLAGS     = -DSVR3 $(ADD_CFLAGS) $(COLOR) $(CLOCK) -I/usr/local/bks-V3_SCO/ncurses-5.0-elf/include
#COLOR      = -DCOLOR_SUPPORT
#LDFLAGS    = -lncurses -L/usr/local/bks-V3_SCO/ncurses-5.0-elf/lib -belf

###########################
# Interactive
###########################
#CFLAGS     = -DSVR3 -Disc386 $(ADD_CFLAGS)
#LDFLAGS    = -lcurses -lPW

###########################
# UNIX SVR4
###########################
#CFLAGS     = -DSVR4 $(ADD_CFLAGS)
#LDFLAGS    = -lcurses -lgen

###########################
# OSF1
###########################
#CFLAGS     = -DOSF1 $(ADD_CFLAGS)
#LDFLAGS    = -lcurses

###########################
# HPUX
###########################
#CFLAGS     = -Dhpux $(ADD_CFLAGS)
#LDFLAGS    = -lcurses  -lc -lPW
#CC	    = cc -Ae

###########################
# HPUX-10.x
###########################
#CFLAGS     = -Dhpux $(ADD_CFLAGS)
#LDFLAGS    = -lcur_colr  -lc -lPW
#CC	    = cc -Ae

###########################
# SGI IRIX Rel. 5.2
###########################
#CFLAGS     = -Dsgi $(ADD_CFLAGS)
#LDFLAGS    = -lcurses -lgen

###########################
# Solaris 2
###########################
#CFLAGS     = $(ADD_CFLAGS) -DSVR4
#LDFLAGS    = -lcurses 

###########################
# SUN
###########################
#CFLAGS     = $(ADD_CFLAGS)
#CC	    = acc -Xa -strconst -sys5
#LDFLAGS    = -lcurses 

###########################
# SUN (gcc)
###########################
#CFLAGS     = $(ADD_CFLAGS)
#CC	    = gcc -I/usr/5include -L/usr/5lib
#LDFLAGS    = -lcurses 

###########################
# SUNOS 5.6 (gcc) Thanks to Fabiano Silos Reis
###########################
#CFLAGS     = $(ADD_CFLAGS) -Dsun56
#CC	    = gcc -I/usr/include -L/usr/lib
#LDFLAGS    = -lcurses 

###########################
# IBM RS/6000 (Thanks to jum)
###########################
#CFLAGS     = $(ADD_CFLAGS)
#LDFLAGS    = -lcurses -lPW

###########################
# ULTRIX V4.3
###########################
#CFLAGS     = -Dultrix $(ADD_CFLAGS)
#LDFLAGS    = -lcursesX

###########################
# QNX (tested with ncurses 4.2)
###########################
#COLOR       = -DCOLOR_SUPPORT
#CLOCK	    = -DCLOCK_SUPPORT # Experimental!
#CFLAGS      = $(ADD_CFLAGS) $(COLOR) $(CLOCK) -DHAS_REGCOMP
#LDFLAGS     = -L/home/mhes/lib -lncurses


##############################################################################


MAIN    = ytree
OBJS	= arc.o archive.o chgrp.o chmod.o chown.o clock.o color.o copy.o    \
	  delete.o dirwin.o disp.o edit.o error.o execute.o filespec.o      \
	  filewin.o freesp.o global.o group.o hex.o history.o init.o input.o keyhtab.o lha.o  \
	  login.o main.o match.o mkdir.o move.o passwd.o pipe.o    \
	  print.o profile.o quit.o rar.o readtree.o rename.o rmdir.o rpm.o  \
	  sort.o stat.o system.o tar.o termcap.o tilde.o usermode.o util.o view.o zip.o zoo.o 7z.o

# mktime.o removed from OBJS list

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

arc.o: config.h ytree.h arc.c
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
lha.o: config.h ytree.h lha.c
login.o: config.h ytree.h login.c
main.o: config.h ytree.h main.c
match.o: config.h ytree.h match.c
mkdir.o: config.h ytree.h mkdir.c
# mktime.o: config.h ytree.h mktime.c <-- REMOVED
move.o: config.h ytree.h move.c
passwd.o: config.h ytree.h passwd.c
pipe.o: config.h ytree.h pipe.c
print.o: ytree.h print.c config.h
profile.o: config.h ytree.h profile.c
quit.o: config.h ytree.h quit.c
rar.o: config.h ytree.h rar.c
readtree.o: config.h ytree.h readtree.c
rename.o: config.h ytree.h rename.c
rmdir.o: config.h ytree.h rmdir.c
rpm.o: config.h ytree.h rpm.c
sort.o: config.h ytree.h sort.c
stat.o: config.h ytree.h stat.c
system.o: config.h ytree.h system.c
tar.o: config.h ytree.h tar.c
termcap.o: config.h ytree.h termcap.c
tilde.o: config.h tilde.h tilde.c
usermode.o: config.h ytree.h usermode.c
util.o: config.h ytree.h util.c
view.o: config.h ytree.h view.c
zip.o: config.h ytree.h zip.c
7z.o: config.h ytree.h 7z.c
zoo.o: config.h ytree.h zoo.c