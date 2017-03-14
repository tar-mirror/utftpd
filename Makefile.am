#noinst_SCRIPTS=systype
PACKAGE_VERSION=@PACKAGE@-@VERSION@
# i can do that better ...
AUTOMAKE_OPTIONS=no-dependencies ansi2knr readme-alpha 1.3 gnu
man_MANS=utftpd.8 utftpd_make.8 utftpd.conf.5 utftp.1
noinst_HEADERS = timselsysdep.h nonblock.h uocompiler.h str2num.h uostr.h \
	attribs.h uoio.h cdb.h typesize.h cdbmake.h uogetopt.h str_ulong.h  \
	wildmat.h tftplib.h timing.h utftpd_make.h uosock.h no_tftp.h \
	utftpd.h
TESTS=check.1 check.2 check.3 check.4 check.5 check.6 check.7 check.8 \
	check.9 check.10 check.11 check.12 check.13 check.14 check.15 check.16 \
	check.17 check.18 check.19 check.20 check.21 check.22 check.23 check.24 \
	check.25 check.26 check.27 check.28 check.29 check.30 check.31 check.32 \
	check.33 check.34 check.35 check.36 check.37 check.38 check.39 check.40 \
	check.41 check.42 check.43 check.44 check.cleanup
TESTS_ENVIRONMENT=TFTP=@TFTP@ \
	SCCS_GET=@SCCS_GET@ SCCS_DELTA=@SCCS_DELTA@ SCCS_CLEAN=@SCCS_CLEAN@ SCCS_UNGET=@SCCS_UNGET@ \
	RCS_CI=@RCS_CI@ RCS_CO=@RCS_CO@
EXTRA_DIST= inet_aton.c memcmp.c \
	Specfile.in rpmrc buildrpm systype.in \
	$(man_MANS) uostr.3 uogetopt.3 \
	$(TESTS) check_lib check_extra.1 \
	README README.cvs TODO sample.config USAGE FEATURES
MOSTLYCLEANFILES=check..*


noinst_LIBRARIES=libstr.a libio.a libcdb.a libcdbmake.a libtftp.a
libstr_a_SOURCES=uostr.c uostr_cstr.c uostr_uostr.c uostr_cut.c uostr_mem.c uostr_char.c
libio_a_SOURCES=uoio.c uoio_gd_uos.c
libcdb_a_SOURCES=cdb_seek.c cdb_hash.c cdb_unpack.c
libcdbmake_a_SOURCES=cdbmake_add.c   cdbmake_hash.c  cdbmake_pack.c
libtftp_a_SOURCES=tftp_nak.c tftp_s_ctrl.c tftp_send.c tftp_receive.c tftp_prepare.c tftp_f_ctrl.c

noinst_PROGRAMS=miniudpserv
sbin_PROGRAMS=utftpd utftpd_make utftpd_dump
bin_PROGRAMS=utftp
utftpd_SOURCES=utftpd.c uogetopt.c str2long.c str2ulong.c nonblock.c \
	wildmat.c utftpd_recv.c str_ulong.c utftpd_send.c \
	utftpd_sccs.c utftpd_rcs.c utftpd_novc.c wait_check.c timing.c \
	parse_ipport.c utftpd_nak.c
utftpd_LDADD=libstr.a libio.a libcdb.a @LIBOBJS@
utftpd_make_SOURCES=utftpd_make.c str2long.c str2ulong.c str_ulong.c uogetopt.c  parser.y
utftpd_make_LDADD=libio.a libstr.a libcdbmake.a @LIBOBJS@
utftpd_dump_SOURCES=utftpd_dump.c
utftpd_dump_LDADD=libcdb.a @LIBOBJS@
utftp_SOURCES=utftp.c uogetopt.c str2long.c str2ulong.c str_ulong.c
utftp_LDADD=libtftp.a libstr.a libio.a libcdb.a @LIBOBJS@
miniudpserv_SOURCES=miniudpserv.c uogetopt.c str2ulong.c str2long.c
miniudpserv_LDADD=@LIBOBJS@

if HAVE_GNU_MAKE
## We must create the .deps directory if it doesn't exist.  We do it
## in this strange way to make sure the .deps directory exists,
## without trying to run mkdir for each compilation.
## stolen from automake
DEPS_MAGIC := $(shell mkdir deps > /dev/null 2>&1 || :)
endif

if HAVE_GNU_MAKE
# no dependency tracking without GNU make
##
## automake depencency stuff is unreliable, it will not
## recreate .deps/xxx.P if it has been removed.
## and anyway, i dislike hidden directories.
-include $(addprefix ./deps/,$(SOURCES:.c=.d))
mostlyclean-local: 
	rm -rf deps
deps/%.d: %.c $(noinst_HEADERS)
if HAVE_GCC_DEP
	@echo mkdep $< 
	@$(COMPILE) -Wp,-MMD,./deps/$(@F) -E $< >/dev/null
else
# no dependency generating without GCC
	@exit 0
endif
endif

rpm: $(PACKAGE_VERSION).tar.gz Specfile
	$(srcdir)/buildrpm $(srcdir)
success:
	sh systype | mail uwe-generic-counter@ohse.de
cvs-dist:
	@if sed 1q NEWS | grep -e "$(VERSION)" > /dev/null; then :; else \
		echo "NEWS not updated; not releasing" 1>&2; \
		exit 1;                                   \
	fi
	cvs tag `echo "Release-$(VERSION)" | sed 's/\./-/g'`
	$(MAKE) dist

