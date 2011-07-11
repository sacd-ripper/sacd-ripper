# Makefile for libiconv/src

#### Start of system configuration section. ####

# Directories used by "make":
srcdir = .
top_srcdir = ..

# Directories used by "make install":
prefix = /usr/local
local_prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
datadir = ${prefix}/share
localedir = $(datadir)/locale

# Programs used by "make":
CC = ppu-gcc
CFLAGS = -I/usr/local/ps3dev/psl1ght/ppu/include -I/usr/local/ps3dev/portlibs/ppu/include
CPPFLAGS = 
LDFLAGS = -L/usr/local/ps3dev/psl1ght/ppu/lib -L/usr/local/ps3dev/portlibs/ppu/lib -lrt -llv2
INCLUDES = -I. -I$(srcdir) -I.. -I../include -I$(srcdir)/../include -I../srclib -I$(srcdir)/../srclib
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LIBTOOL_COMPILE = $(LIBTOOL) --mode=compile
LIBTOOL_LINK = $(LIBTOOL) --mode=link
LIBTOOL_INSTALL = $(LIBTOOL) --mode=install
LIBTOOL_UNINSTALL = $(LIBTOOL) --mode=uninstall
RM = rm -f


# Programs used by "make install":
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
mkinstalldirs = $(SHELL) ../autoconf/mkinstalldirs

#### End of system configuration section. ####

SHELL = /bin/sh

# Needed by $(LIBTOOL).
top_builddir = ..

# We cannot link with libintl until libiconv is installed. (When we call
# libtool with arguments "../lib/libiconv.la -lintl", libtool will call ld
# with "../lib/.libs/libiconv.so $libdir/libintl.so $libdir/libiconv.so",
# (remember that $libdir/libintl.la lists $libdir/libiconv.so as a dependency),
# and this gives a fatal linker error on Solaris because the two libiconv.so
# files are different but have the same soname.
# So we can link the iconv executable only after we have installed libiconv,
# i.e. during "make install". The intermediate 'iconv' executable is built
# without internationalization and not linked with libintl.

all : iconv_no_i18n iconv.o
	test `ls -ld . | sed -e 's/^d\(.........\).*/\1/'` = rwxrwxrwx || chmod 777 .

# This is the temporary iconv executable, without internationalization.
iconv_no_i18n : iconv_no_i18n.o ../lib/libiconv.la
	$(LIBTOOL_LINK) $(CC) $(LDFLAGS) iconv_no_i18n.o ../srclib/libicrt.a ../lib/libiconv.la -o $@

iconv_no_i18n.o : $(srcdir)/iconv_no_i18n.c $(srcdir)/iconv.c
	$(CC) -c $(INCLUDES) -I../lib $(CFLAGS) $(CPPFLAGS) -DINSTALLDIR=\"$(bindir)\" -DLOCALEDIR=\"$(localedir)\" $(srcdir)/iconv_no_i18n.c

iconv.o : $(srcdir)/iconv.c
	$(CC) -c $(INCLUDES) -I../lib $(CFLAGS) $(CPPFLAGS) -DINSTALLDIR=\"$(bindir)\" -DLOCALEDIR=\"$(localedir)\" $(srcdir)/iconv.c

# The following rule is necessary to avoid a toplevel "make -n check" failure.
../lib/libiconv.la :
	cd ../lib && $(MAKE) libiconv.la

# Support for relocatability.
RELOCATABLE_LIBRARY_PATH = $(libdir)
RELOCATABLE_SRC_DIR = $(top_srcdir)/srclib
RELOCATABLE_BUILD_DIR = ../srclib
RELOCATABLE_CONFIG_H_DIR = ..

iconv_LDFLAGS = `if test -n '$(RELOCATABLE_LDFLAGS)'; then $(RELOCATABLE_LDFLAGS) $(bindir); fi`

# During "make install", we can build the final iconv executable.
# On HP-UX, in order to ensure that the new libiconv.sl will override the old
# one that is hardcoded in libintl.sl, we need to mention libiconv.sl before
# libintl.sl on the link command line. We have to bypass libtool in order to
# achieve this.
install : all force
	if [ ! -d $(DESTDIR)$(prefix) ] ; then $(mkinstalldirs) $(DESTDIR)$(prefix) ; fi
	if [ ! -d $(DESTDIR)$(exec_prefix) ] ; then $(mkinstalldirs) $(DESTDIR)$(exec_prefix) ; fi
	if [ ! -d $(DESTDIR)$(bindir) ] ; then $(mkinstalldirs) $(DESTDIR)$(bindir) ; fi
	case "linux-gnu" in \
	  hpux* | freebsd*) $(CC) $(LDFLAGS) $(iconv_LDFLAGS) iconv.o ../srclib/libicrt.a -L$(libdir) -liconv  -o iconv;; \
	  *) $(LIBTOOL_LINK) $(CC) $(LDFLAGS) $(iconv_LDFLAGS) iconv.o ../srclib/libicrt.a ../lib/libiconv.la  -o iconv;; \
	esac
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL_INSTALL) $(INSTALL_PROGRAM) iconv $(DESTDIR)$(bindir)/iconv

installdirs : force
	if [ ! -d $(DESTDIR)$(prefix) ] ; then $(mkinstalldirs) $(DESTDIR)$(prefix) ; fi
	if [ ! -d $(DESTDIR)$(exec_prefix) ] ; then $(mkinstalldirs) $(DESTDIR)$(exec_prefix) ; fi
	if [ ! -d $(DESTDIR)$(bindir) ] ; then $(mkinstalldirs) $(DESTDIR)$(bindir) ; fi

uninstall : force
	$(LIBTOOL_UNINSTALL) $(RM) $(DESTDIR)$(bindir)/iconv

check : all

mostlyclean : clean

clean : force
	$(RM) *.o *.lo iconv_no_i18n iconv core
	$(RM) -r .libs _libs

distclean : clean
	$(RM) Makefile

maintainer-clean : distclean

force :

