.PHONY: all dynamic static clean

STATIC_LIBRARY := ../../lib/libncsocket.a
DYNAMIC_LIBRARY := ../../lib/libncsocket.so
OPTIMIZATIONCPPFLAGS := -O3
BASICCPPFLAGS := -c -Wall -std=c++0x
INCLUDES := -I../../lib
CPP := g++

all : static dynamic 

clean :
	rm -rf run_static run_dynamic 

static : run_static
run_static : $(STATIC_LIBRARY) main_stc.o 
	$(CROSS)$(CPP) -o $@ main_stc.o $(STATIC_LIBRARY) -lpthread
	rm -f main_stc.o
$(STATIC_LIBRARY) : ../../c_cpp
	$(MAKE) -C ../../c_cpp
main_stc.o : Makefile main.cpp
	$(CROSS)$(CPP) -o $@ main.cpp $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS) $(INCLUDES)

dynamic : run_dynamic
run_dynamic : $(DYNAMIC_LIBRARY) main_dyn.o
	$(CROSS)$(CPP) -o $@ main_dyn.o -lpthread -ldl
	rm -f main_dyn.o
$(DYNAMIC_LIBRARY) : ../../c_cpp
	$(MAKE) -C ../../c_cpp	
main_dyn.o : Makefile main.cpp
	$(CROSS)$(CPP) -o $@ main.cpp -DDYNAMIC $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS) $(INCLUDES)

