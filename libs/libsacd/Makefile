#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment. export PSL1GHT=<path>")
endif

include	$(PSL1GHT)/ppu_rules

BUILD		:=	build

#---------------------------------------------------------------------------------
ifeq ($(strip $(PLATFORM)),)
#---------------------------------------------------------------------------------
export BASEDIR		:= $(CURDIR)
export DEPS			:= $(BASEDIR)/deps
export LIBS			:=	$(BASEDIR)/lib

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------

export LIBDIR		:= $(LIBS)
export DEPSDIR		:=	$(DEPS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

LIBRARY		:= $(LIBDIR)/libsacd

#---------------------------------------------------------------------------------
DEFINCS		:= -I$(BASEDIR) -I$(PSL1GHT)/ppu/include -I$(BASEDIR)/../libid3 -I$(BASEDIR)/../libcommon
INCLUDES	:=	$(DEFINCS)

CFLAGS		:= -O2 -mregnames -Wall -mcpu=cell $(MACHDEP) -DLIBSACD_INTERNAL $(INCLUDES) -Wa,-mcell -DWITH_PL3
ASFLAGS		:=	$(MACHDEP) -mregnames -mcpu=cell -D__ASSEMBLY__ -Wa,-mcell $(INCLUDES)

#---------------------------------------------------------------------------------
VPATH :=	$(BASEDIR)

#---------------------------------------------------------------------------------
OBJS		:= \
			scarletbook.o \
			dsdiff.o \
			dsf.o \
			dst_decoder_ps3.o \
            sacd_input.o \
            scarletbook_print.o \
            scarletbook_id3.o \
            scarletbook_read.o \
            scarletbook_output.o \
            scarletbook_helpers.o \
            sac_accessor.o \
            ioctl.o \
            iso_writer.o \
            sacd_ripper.pb.o \
            sacd_pb_stream.o \
            sacd_reader.o 
all: ppu

#---------------------------------------------------------------------------------
ppu:
#---------------------------------------------------------------------------------
	@[ -d $(LIBS) ] || mkdir -p $(LIBS)
	@[ -d $(DEPS) ] || mkdir -p $(DEPS)
	@[ -d ppu ] || mkdir -p ppu
	@$(MAKE) PLATFORM=ppu lib -C ppu -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
install: all
#---------------------------------------------------------------------------------
	@[ -d $(PSL1GHT)/ppu/lib ] || mkdir -p $(PSL1GHT)/ppu/lib
	@cp -frv $(CURDIR)/lib/ppu/*.a $(PSL1GHT)/ppu/lib

#---------------------------------------------------------------------------------
$(LIBRARY).a: $(OBJS)
#---------------------------------------------------------------------------------

.PHONY: lib ppu install

#---------------------------------------------------------------------------------
lib: $(LIBRARY).a
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
clean:
#---------------------------------------------------------------------------------
	@echo clean ...
	@rm -rf ppu
	@rm -rf $(DEPS)
	@rm -rf $(LIBS)

-include $(DEPSDIR)/*.d
