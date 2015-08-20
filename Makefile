BEL_HOME = ../beldebug

PREFIX  ?= /usr/local
STAGING ?= 
EB      ?= $(BEL_HOME)/ip_cores/etherbone-core/api
ECA	?= $(BEL_HOME)/ip_cores/wr-cores/modules/wr_eca
TLU	?= $(BEL_HOME)/ip_cores/wr-cores/modules/wr_tlu
TARGETS := eca2tlu_demo

EXTRA_FLAGS ?=
CFLAGS      ?= $(EXTRA_FLAGS) -Wall -O2 -I $(EB) -I $(ECA) -I $(TLU)
LIBS        ?= -L $(EB) -L $(ECA) -L $(TLU) -Wl,-rpath,$(PREFIX)/lib -letherbone -leca -ltlu

all:	$(TARGETS)
clean:
	rm -f $(TARGETS)
install:
	mkdir -p $(STAGING)$(PREFIX)/bin
	cp $(TARGETS) $(STAGING)$(PREFIX)/bin

%:	%.c
	gcc $(CFLAGS) -o $@ $< $(LIBS)

%:	%.cpp
	g++ $(CFLAGS) -o $@ $< $(LIBS)
