################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../atp2tcl.c \
../atpalloc.c \
../atpbcdpm.c \
../atpbool.c \
../atpcase.c \
../atpcheck.c \
../atpchois.c \
../atpcmd.c \
../atpcmplx.c \
../atpdebug.c \
../atpdynam.c \
../atperror.c \
../atpexmp.c \
../atpfindp.c \
../atpframe.c \
../atphelp.c \
../atphelpc.c \
../atphexpm.c \
../atpinit.c \
../atpkeywd.c \
../atplist.c \
../atpmanpg.c \
../atpmem.c \
../atpnullp.c \
../atpnum.c \
../atppage.c \
../atpparse.c \
../atppmchk.c \
../atppmdef.c \
../atppmptr.c \
../atprpblk.c \
../atpstcmp.c \
../atpstr.c \
../atptoken.c \
../atpusrio.c 

OBJS += \
./atp2tcl.o \
./atpalloc.o \
./atpbcdpm.o \
./atpbool.o \
./atpcase.o \
./atpcheck.o \
./atpchois.o \
./atpcmd.o \
./atpcmplx.o \
./atpdebug.o \
./atpdynam.o \
./atperror.o \
./atpexmp.o \
./atpfindp.o \
./atpframe.o \
./atphelp.o \
./atphelpc.o \
./atphexpm.o \
./atpinit.o \
./atpkeywd.o \
./atplist.o \
./atpmanpg.o \
./atpmem.o \
./atpnullp.o \
./atpnum.o \
./atppage.o \
./atpparse.o \
./atppmchk.o \
./atppmdef.o \
./atppmptr.o \
./atprpblk.o \
./atpstcmp.o \
./atpstr.o \
./atptoken.o \
./atpusrio.o 

C_DEPS += \
./atp2tcl.d \
./atpalloc.d \
./atpbcdpm.d \
./atpbool.d \
./atpcase.d \
./atpcheck.d \
./atpchois.d \
./atpcmd.d \
./atpcmplx.d \
./atpdebug.d \
./atpdynam.d \
./atperror.d \
./atpexmp.d \
./atpfindp.d \
./atpframe.d \
./atphelp.d \
./atphelpc.d \
./atphexpm.d \
./atpinit.d \
./atpkeywd.d \
./atplist.d \
./atpmanpg.d \
./atpmem.d \
./atpnullp.d \
./atpnum.d \
./atppage.d \
./atpparse.d \
./atppmchk.d \
./atppmdef.d \
./atppmptr.d \
./atprpblk.d \
./atpstcmp.d \
./atpstr.d \
./atptoken.d \
./atpusrio.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


