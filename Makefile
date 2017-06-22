CFLAGS = -Wall -Wextra
CFLAGS += -std=c++11
CFLAGS += -I.
CXXFLAGS = $(CFLAGS)

GCC=gcc $(CFLAGS) -ggdb -pthread
GCCSO=$(GCC) -fPIC -shared
CPP=g++ $(CXXFLAGS) -ggdb -pthread
CPPSO=$(CPP) -fPIC -shared

EXEC=tcp-srv tcp-srv.so
EXEC+=tcp-clnt tcp-clnt.so

all: $(EXEC)

.PHONY: module
module: all

clean:
	/bin/rm -f $(EXEC)

tcp-srv: tcp-srv.cc common.cc
	$(CPP) -o $@ $<
tcp-srv.so: tcp-srv.cc common.cc
	$(CPPSO) -o $@ $<
tcp-clnt: tcp-clnt.cc common.cc
	$(CPP) -o $@ $<
tcp-clnt.so: tcp-clnt.cc common.cc
	$(CPPSO) -o $@ $<
