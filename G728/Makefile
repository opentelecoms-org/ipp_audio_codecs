###############################
# DO NOT EDIT BELOW THIS LINE #
###############################

# System Specific
INCLSFX=.h
CPPSFX=.c
OBJSFX=.o
EXESFX=

# IPP Dependencies
IPPINCLS=-I$(IPPROOT)/include
IPPLIBS=-L$(IPPROOT)/sharedlib -lippsc -lipps -lippcore -L$(IPPROOT)/sharedlib/linux32 -lguide

# Compiler Specifics
CCOPTS=-c -O2
CCDEFS=-D__unix__ -DNDEBUG -DNO_SCRATCH_MEMORY_USED -DLINUX32
CCINCLS=-I./include -I./vm/include $(IPPINCLS)

LNKOUT=-o
LNKOPTS=
LNKLIBS=$(IPPLIBS) -lpthread

build: prepare_directory build_encoder build_decoder; @echo "Done"

##### api object #####
CPPSRC_API=$(wildcard ./api/*$(CPPSFX))
OBJECT_API=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_API))

CPPSRC_VM=$(wildcard ./vm/src/*$(CPPSFX))
OBJECT_VM=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_VM))

##### link encoder example programs #####
OBJECT_ENCODER=./bin/g728encoder$(EXESFX)
CPPSRC_ENCODER=$(wildcard ./samples/g728encoder$(CPPSFX))
OBJSRC_ENCODER=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_ENCODER))

build_encoder: $(OBJECT_ENCODER) ; @echo "done encoder"

$(OBJECT_ENCODER): $(OBJSRC_ENCODER) $(OBJECT_API) $(OBJECT_VM)
	$(CC) -o$@ $(LNKOPTS) $^ $(LNKLIBS)

##### link decoder example programs #####
OBJECT_DECODER=./bin/g728decoder$(EXESFX)
CPPSRC_DECODER=$(wildcard ./samples/g728decoder$(CPPSFX))
OBJSRC_DECODER=$(patsubst %$(CPPSFX), %$(OBJSFX), $(CPPSRC_DECODER))

build_decoder: $(OBJECT_DECODER) ; @echo "done decoder"

$(OBJECT_DECODER): $(OBJSRC_DECODER) $(OBJECT_API) $(OBJECT_VM)
	$(CC) -o$@ $(LNKOPTS) $^ $(LNKLIBS)

##### compile encoder & decoder objects all together #####
OBJECTS=$(OBJECT_API)  $(OBJECT_VM)$(OBJSRC_ENCODER) $(OBJSRC_DECODER)
$(OBJECTS): %$(OBJSFX): %$(CPPSFX)
	$(CC) $(CCINCLS) $(CCDEFS) $(CCOPTS) -o$@ $<

prepare_directory: FORCE
	@if test ! -d ./bin; then mkdir ./bin; fi

clean: FORCE
	rm -f $(OBJECTS)

distclean: clean
	rm -f $(OBJECT_ENCODER) $(OBJECT_DECODER)

FORCE:
    
