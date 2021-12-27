CC=gcc -g
CFLAGS=-I. -I../../eclipse-workspace-cpp-slp/SLP
LDFLAGS=-L. -L../../eclipse-workspace-cpp-slp/SLP
RM = rm -f
RANLIB = ranlib
AR = ar rcul

ATP_SRCS = 	atp2tcl.c atpalloc.c atpbcdpm.c atpbool.c atpcase.c \
			atpcheck.c atpchois.c atpcmd.c atpcmplx.c atpdebug.c \
			atpdynam.c atperror.c atpfindp.c atpframe.c atphelp.c \
			atphelpc.c atphexpm.c atpinit.c atpkeywd.c atplist.c \
			atpmanpg.c atpmem.c atpnullp.c atpnum.c atppage.c \
			atpparse.c atppmchk.c atppmdef.c atppmptr.c atprpblk.c \
			atpstcmp.c atpstr.c atptoken.c atpusrio.c

ATP_OBJS =	atp2tcl.o atpalloc.o atpbcdpm.o atpbool.o atpcase.o \
			atpcheck.o atpchois.o atpcmd.o atpcmplx.o atpdebug.o \
			atpdynam.o atperror.o atpfindp.o atpframe.o atphelp.o \
			atphelpc.o atphexpm.o atpinit.o atpkeywd.o atplist.o \
			atpmanpg.o atpmem.o atpnullp.o atpnum.o atppage.o \
			atpparse.o atppmchk.o atppmdef.o atppmptr.o atprpblk.o \
			atpstcmp.o atpstr.o atptoken.o atpusrio.o

.c.o:
		$(RM) $@
		$(CC) -c $(CFLAGS) $*.c

all:: libatp.a atpexmp

libatp.a:	$(ATP_OBJS)
			$(RM) $@
			$(AR) $@ $(ATP_OBJS)
			$(RANLIB) $@

atpexmp:	libatp.a atpexmp.o
			$(RM) $@
			$(CC) -v -o $@ $(CFLAGS) atpexmp.c $(LDFLAGS) -lslp -ltcl -latp -lm
			chmod +x atpexmp

clean:
			rm -f *.a *.o core atpexmp

