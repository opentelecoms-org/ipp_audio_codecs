###############################
# DO NOT EDIT BELOW THIS LINE #
###############################

include ../makeopts

# Compiler Specifics
CCOPTS=-c $(OPTIMIZE) 
CCDEFS=-D__unix__ -Dlinux -Dlinux32 -DNDEBUG -DLINUX32 -DNO_SCRATCH_MEMORY_USED
CCINCLS=-I./include -I./vm/include $(IPPINCLS)

LNKOUT=-o
LNKOPTS= 
LNKLIBS=$(IPPLIBS) -lpthread -lm $(ICCLIBS)

SOLINKOPTS=-shared $(LDFLAGS) -Xlinker -x

build: prepare_directory build_codec build_encoder build_decoder ; @echo "Done"

##### api object #####
CPPSRC_API=$(wildcard ./api/*$(CPPSFX))
OBJECT_API=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_API))

CPPSRC_VM=$(wildcard ./vm/src/*$(CPPSFX))
OBJECT_VM=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_VM))

############ Asterisk Codec ############
OBJECT_CODEC=./bin/codec_g729.so
CPPSRC_CODEC=$(wildcard ./samples/util_e$(CPPSFX) ./samples/util_d$(CPPSFX) ./samples/codec_g729.c )
OBJSRC_CODEC=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_CODEC))

build_codec:	$(OBJECT_CODEC) ; @echo "done Asterisk codec"

$(OBJECT_CODEC):	$(OBJSRC_CODEC) $(OBJECT_API)
	$(CC) $(SOLINKOPTS) -o $@ $^ $(LNKLIBS)

##### link encoder example programs #####
OBJECT_ENCODER=./bin/g729fpencoder$(EXESFX)
OBJECT_MY_ENC=./bin/my_enc
CPPSRC_ENCODER=$(wildcard ./samples/encoder$(CPPSFX) ./samples/util_e$(CPPSFX))
CPPSRC_MY_ENC=$(wildcard ./samples/util_e$(CPPSFX) ./samples/my_enc.c)
OBJSRC_ENCODER=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_ENCODER))
OBJSRC_MY_ENC=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_MY_ENC))

build_encoder: $(OBJECT_ENCODER) $(OBJECT_MY_ENC) ; @echo "done encoder"

$(OBJECT_MY_ENC): $(OBJSRC_MY_ENC) $(OBJECT_API) $(OBJECT_VM)
	$(CC) -o$@ $(LNKOPTS) $^ $(LNKLIBS) 
$(OBJECT_ENCODER): $(OBJSRC_ENCODER) $(OBJECT_API) $(OBJECT_VM)
	$(CC) -o$@ $(LNKOPTS) $^ $(LNKLIBS) 

##### link decoder example programs #####
OBJECT_DECODER=./bin/g729fpdecoder$(EXESFX)
OBJECT_MY_DEC=./bin/my_dec
CPPSRC_DECODER=$(wildcard ./samples/decoder$(CPPSFX) ./samples/util_d$(CPPSFX))
CPPSRC_MY_DEC=$(wildcard ./samples/util_d$(CPPSFX) ./samples/my_dec.c)
OBJSRC_DECODER=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_DECODER))
OBJSRC_MY_DEC=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_MY_DEC))

build_decoder: $(OBJECT_DECODER) $(OBJECT_MY_DEC) ; @echo "done decoder"

$(OBJECT_MY_DEC): $(OBJSRC_MY_DEC) $(OBJECT_API) $(OBJECT_VM)
	$(CC) -o$@ $(LNKOPTS) $^ $(LNKLIBS)
$(OBJECT_DECODER): $(OBJSRC_DECODER) $(OBJECT_API) $(OBJECT_VM)
	$(CC) -o$@ $(LNKOPTS) $^ $(LNKLIBS) 

##### compile encoder & decoder objects all together #####
#OBJECTS=$(OBJECT_API)  $(OBJECT_VM) $(OBJSRC_CODEC)
OBJECTS=$(OBJECT_API) $(OBJECT_VM) $(OBJSRC_CODEC) $(OBJSRC_ENCODER) $(OBJSRC_MY_ENC) $(OBJSRC_DECODER) $(OBJSRC_MY_DEC)
$(OBJECTS): %$(OBJSFX): %$(CPPSFX)
	$(CC) $(CCINCLS) $(CCDEFS) $(CCOPTS) -o$@ $<

prepare_directory: FORCE
	@if test ! -d ./bin; then mkdir ./bin; fi

clean: FORCE
	rm -f $(OBJECTS)

distclean: clean
	rm -f $(OBJECT_ENCODER) $(OBJECT_DECODER)

FORCE:
    
