# Makefile for Independent JPEG Group's software using the xloadimage
# configuration information.  This is loosely based on makefile.unix.

include ../Make.conf
include ../jpeg.conf

CFLAGS= $(CC_OPT_FLAGS) $(JPEG_CC_FLAGS)

# Link-time cc options:
LDFLAGS= 

# To link any special libraries, add the necessary -l commands here.
# In particular, on some versions of HP-UX (and probably other SysV-derived
# systems) there is a faster alternate malloc(3) library that you can use
# by adding "-lmalloc" to this line.
LDLIBS= 

# miscellaneous OS-dependent stuff
# linker
LD= $(CC)

# source files (independently compilable files)
SOURCES= jbsmooth.c jcarith.c jccolor.c jcdeflts.c jcexpand.c jchuff.c \
        jcmain.c jcmaster.c jcmcu.c jcpipe.c jcsample.c jdarith.c jdcolor.c \
        jddeflts.c jdhuff.c jdmain.c jdmaster.c jdmcu.c jdpipe.c jdsample.c \
        jerror.c jquant1.c jquant2.c jfwddct.c jrevdct.c jutils.c jmemmgr.c \
        jrdjfif.c jrdgif.c jrdppm.c jrdrle.c jrdtarga.c jwrjfif.c jwrgif.c \
        jwrppm.c jwrrle.c jwrtarga.c
# virtual source files (not present in distribution file, see SETUP)
VIRTSOURCES= jmemsys.c
# system-dependent implementations of virtual source files
SYSDEPFILES= jmemansi.c jmemname.c jmemnobs.c jmemdos.c jmemdos.h \
        jmemdosa.asm
# files included by source files
INCLUDES= jinclude.h jconfig.h jpegdata.h jversion.h jmemsys.h
# documentation, test, and support files
DOCS= README SETUP USAGE CHANGELOG cjpeg.1 djpeg.1 architecture codingrules
MAKEFILES= makefile.ansi makefile.unix makefile.manx makefile.sas \
        makcjpeg.st makdjpeg.st makljpeg.st makefile.mc5 makefile.mc6 \
        makefile.bcc makefile.mms makefile.vms makvms.opt
OTHERFILES= ansi2knr.c ckconfig.c example.c
TESTFILES= testorig.jpg testimg.ppm testimg.gif testimg.jpg
DISTFILES= $(DOCS) $(MAKEFILES) $(SOURCES) $(SYSDEPFILES) $(INCLUDES) \
        $(OTHERFILES) $(TESTFILES)
# objectfiles common to cjpeg and djpeg
COMOBJECTS= jutils.o jerror.o jmemmgr.o jmemsys.o
# compression objectfiles
CLIBOBJECTS= jcmaster.o jcdeflts.o jcarith.o jccolor.o jcexpand.o jchuff.o \
        jcmcu.o jcpipe.o jcsample.o jfwddct.o jwrjfif.o jrdgif.o jrdppm.o \
        jrdrle.o jrdtarga.o
COBJECTS= jcmain.o $(CLIBOBJECTS) $(COMOBJECTS)
# decompression objectfiles
DLIBOBJECTS= jdmaster.o jddeflts.o jbsmooth.o jdarith.o jdcolor.o jdhuff.o \
        jdmcu.o jdpipe.o jdsample.o jquant1.o jquant2.o jrevdct.o jrdjfif.o \
        jwrgif.o jwrppm.o jwrrle.o jwrtarga.o
DOBJECTS= jdmain.o $(DLIBOBJECTS) $(COMOBJECTS)
# These objectfiles are included in libjpeg.a
LIBOBJECTS= $(CLIBOBJECTS) $(DLIBOBJECTS) $(COMOBJECTS)

all: ansi2knr libjpeg.a

ansi2knr: ansi2knr.c
	$(CC) $(CFLAGS) -o ansi2knr ansi2knr.c

cjpeg: ansi2knr $(COBJECTS)
	$(LD) $(LDFLAGS) -o cjpeg $(COBJECTS) $(LDLIBS)

djpeg: ansi2knr $(DOBJECTS)
	$(LD) $(LDFLAGS) -o djpeg $(DOBJECTS) $(LDLIBS)

# libjpeg.a is useful if you are including the JPEG software in a larger
# program; you'd include it in your link, rather than the individual modules.
libjpeg.a: ansi2knr $(LIBOBJECTS)
	$(RM) libjpeg.a
	$(AR) libjpeg.a  $(LIBOBJECTS)
	$(RANLIB) libjpeg.a

#if there isn't a jmemsys, use the one most unix systems like
jmemsys.c:
	$(CP) jmemnobs.c jmemsys.c

clean:
	$(RM) *.o cjpeg djpeg libjpeg.a ansi2knr core tmpansi.* testout.* *~

distribute:
	$(RM) jpegsrc.tar*
	tar cvf jpegsrc.tar $(DISTFILES)
	compress -v jpegsrc.tar

test: cjpeg djpeg
	$(RM) testout.ppm testout.gif testout.jpg
	./djpeg testorig.jpg >testout.ppm
	./djpeg -gif testorig.jpg >testout.gif
	./cjpeg testimg.ppm >testout.jpg
	cmp testimg.ppm testout.ppm
	cmp testimg.gif testout.gif
	cmp testimg.jpg testout.jpg


jbsmooth.o : jbsmooth.c jinclude.h jconfig.h jpegdata.h 
jcarith.o : jcarith.c jinclude.h jconfig.h jpegdata.h 
jccolor.o : jccolor.c jinclude.h jconfig.h jpegdata.h 
jcdeflts.o : jcdeflts.c jinclude.h jconfig.h jpegdata.h 
jcexpand.o : jcexpand.c jinclude.h jconfig.h jpegdata.h 
jchuff.o : jchuff.c jinclude.h jconfig.h jpegdata.h 
jcmain.o : jcmain.c jinclude.h jconfig.h jpegdata.h jversion.h 
jcmaster.o : jcmaster.c jinclude.h jconfig.h jpegdata.h 
jcmcu.o : jcmcu.c jinclude.h jconfig.h jpegdata.h 
jcpipe.o : jcpipe.c jinclude.h jconfig.h jpegdata.h 
jcsample.o : jcsample.c jinclude.h jconfig.h jpegdata.h 
jdarith.o : jdarith.c jinclude.h jconfig.h jpegdata.h 
jdcolor.o : jdcolor.c jinclude.h jconfig.h jpegdata.h 
jddeflts.o : jddeflts.c jinclude.h jconfig.h jpegdata.h 
jdhuff.o : jdhuff.c jinclude.h jconfig.h jpegdata.h 
jdmain.o : jdmain.c jinclude.h jconfig.h jpegdata.h jversion.h 
jdmaster.o : jdmaster.c jinclude.h jconfig.h jpegdata.h 
jdmcu.o : jdmcu.c jinclude.h jconfig.h jpegdata.h 
jdpipe.o : jdpipe.c jinclude.h jconfig.h jpegdata.h 
jdsample.o : jdsample.c jinclude.h jconfig.h jpegdata.h 
jerror.o : jerror.c jinclude.h jconfig.h jpegdata.h 
jquant1.o : jquant1.c jinclude.h jconfig.h jpegdata.h 
jquant2.o : jquant2.c jinclude.h jconfig.h jpegdata.h 
jfwddct.o : jfwddct.c jinclude.h jconfig.h jpegdata.h 
jrevdct.o : jrevdct.c jinclude.h jconfig.h jpegdata.h 
jutils.o : jutils.c jinclude.h jconfig.h jpegdata.h 
jmemmgr.o : jmemmgr.c jinclude.h jconfig.h jpegdata.h jmemsys.h 
jrdjfif.o : jrdjfif.c jinclude.h jconfig.h jpegdata.h 
jrdgif.o : jrdgif.c jinclude.h jconfig.h jpegdata.h 
jrdppm.o : jrdppm.c jinclude.h jconfig.h jpegdata.h 
jrdrle.o : jrdrle.c jinclude.h jconfig.h jpegdata.h 
jrdtarga.o : jrdtarga.c jinclude.h jconfig.h jpegdata.h 
jwrjfif.o : jwrjfif.c jinclude.h jconfig.h jpegdata.h 
jwrgif.o : jwrgif.c jinclude.h jconfig.h jpegdata.h 
jwrppm.o : jwrppm.c jinclude.h jconfig.h jpegdata.h 
jwrrle.o : jwrrle.c jinclude.h jconfig.h jpegdata.h 
jwrtarga.o : jwrtarga.c jinclude.h jconfig.h jpegdata.h 
jmemsys.o : jmemsys.c jinclude.h jconfig.h jpegdata.h jmemsys.h 
