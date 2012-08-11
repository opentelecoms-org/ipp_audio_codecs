###############################
# DO NOT EDIT BELOW THIS LINE #
###############################

include ../makeopts

# Compiler Specifics
CCOPTS=-c $(OPTIMIZE)
CCDEFS=-D__unix__ -DNDEBUG -DNO_SCRATCH_MEMORY_USED -DLINUX32
CCINCLS=-I./include -I./vm/include $(IPPINCLS)

LNKOUT=-o
LNKOPTS=
LNKLIBS=$(IPPLIBS) -lpthread $(ICCLIBS)

build: prepare_directory build_codec; @echo "Done"
SOLINKOPTS=-shared $(LDFLAGS) -Xlinker -x


##### api object #####
CPPSRC_API=$(wildcard ./api/*$(CPPSFX))
OBJECT_API=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_API))

CPPSRC_VM=$(wildcard ./vm/src/*$(CPPSFX))
OBJECT_VM=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_VM))

############ Asterisk Codec ############

OBJECT_CODEC=./bin/codec_g723.so
CPPSRC_CODEC=$(wildcard ./samples/util_e$(CPPSFX) ./samples/util_d$(CPPSFX) ./samples/codec_g723.c )
OBJSRC_CODEC=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_CODEC))

build_codec:	$(OBJECT_CODEC) ; @echo "done Asterisk codec"

$(OBJECT_CODEC):	$(OBJSRC_CODEC) $(OBJECT_API)
	$(CC) $(SOLINKOPTS) -o $@ $^ $(LNKLIBS)

##### link encoder example programs #####
OBJECT_ENCODER=./bin/g723encoder$(EXESFX)
CPPSRC_ENCODER=$(wildcard ./samples/g723encoder$(CPPSFX))
OBJSRC_ENCODER=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_ENCODER))

build_encoder: $(OBJECT_ENCODER) ; @echo "done encoder"

$(OBJECT_ENCODER): $(OBJSRC_ENCODER) $(OBJECT_API) $(OBJECT_VM)
	$(CC) -o$@ $(LNKOPTS) $^ $(LNKLIBS)

##### link decoder example programs #####
OBJECT_DECODER=./bin/g723decoder$(EXESFX)
CPPSRC_DECODER=$(wildcard ./samples/g723decoder$(CPPSFX))
OBJSRC_DECODER=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_DECODER))

build_decoder: $(OBJECT_DECODER) ; @echo "done decoder"

$(OBJECT_DECODER): $(OBJSRC_DECODER) $(OBJECT_API) $(OBJECT_VM)
	$(CC) -o$@ $(LNKOPTS) $^ $(LNKLIBS)

##### compile encoder & decoder objects all together #####
#OBJECTS=$(OBJECT_API) $(OBJECT_VM) $(OBJSRC_ENCODER) $(OBJSRC_DECODER)
OBJECTS=$(OBJECT_API)  $(OBJECT_VM) $(OBJSRC_CODEC)
$(OBJECTS): %$(OBJSFX): %$(CPPSFX)
	$(CC) $(CCINCLS) $(CCDEFS) $(CCOPTS) -o$@ $<

prepare_directory: FORCE
	@if test ! -d ./bin; then mkdir ./bin; fi

clean: FORCE
	rm -f $(OBJECTS)

distclean: clean
	rm -f $(OBJECT_ENCODER) $(OBJECT_DECODER)

FORCE:
    
