.PHONY : all debug release staticlibrary sharedlibrary clean
.SUFFIXES : .cpp .o

INCLUDES := -I./basiclibrary/avltree -I./basiclibrary/threadpool -I./basiclibrary/singleshottimer
OPTIMIZATIONCPPFLAGS := -O3
BASICCPPFLAGS := -c -Wall -std=c++0x
CPP := g++

all : release

dynamic : run_dynamic

static : run_static

run_static : main_stc.o libncsocket.a
	$(CROSS)$(CPP) -o $@ main_stc.o ./libncsocket.a -lpthread
run_dynamic : main_dyn.o libncsocket.so
	$(CROSS)$(CPP) -o $@ main_dyn.o -lpthread -ldl
main_stc.o : main.cpp
	$(CROSS)$(CPP) -o $@ main.cpp $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS) $(INCLUDES)
main_dyn.o : main.cpp
	$(CROSS)$(CPP) -o $@ main.cpp -DDYNAMIC $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS) $(INCLUDES)

clean :
	rm -rf *.o *.so *.a *~ gmon.out *.bak *.stackdump run_* ./out

basiclibrary/.git/config : 
	git clone https://github.com/dujeonglee/basiclibrary.git
SHAREDLIBOUT := out/shared
sharedlibrary : libncsocket.so
libncsocket.so : $(SHAREDLIBOUT) basiclibrary/.git/config $(SHAREDLIBOUT)/finite_field.o $(SHAREDLIBOUT)/tx.o $(SHAREDLIBOUT)/rx.o $(SHAREDLIBOUT)/c_stub.o
	$(CROSS)$(CPP) -shared -o $@ $(SHAREDLIBOUT)/finite_field.o $(SHAREDLIBOUT)/tx.o $(SHAREDLIBOUT)/rx.o $(SHAREDLIBOUT)/c_stub.o
$(SHAREDLIBOUT) :
	mkdir -p $(SHAREDLIBOUT)
SHAREDLIBCPPFLAGS := -fPIC -O3 $(BASICCPPFLAGS)
SHAREDLIBOUT := out/shared
$(SHAREDLIBOUT)/finite_field.o : finite_field.h finite_field.cpp
	$(CROSS)$(CPP) finite_field.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOUT)/tx.o : tx.h tx.cpp
	$(CROSS)$(CPP) tx.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOUT)/rx.o : rx.h rx.cpp
	$(CROSS)$(CPP) rx.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOUT)/c_stub.o : c_stub.h c_stub.cpp
	$(CROSS)$(CPP) c_stub.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)

STATICLIBOUT := out/static
staticlibrary : libncsocket.a
libncsocket.a : $(STATICLIBOUT) basiclibrary/.git/config $(STATICLIBOUT)/finite_field.o $(STATICLIBOUT)/tx.o $(STATICLIBOUT)/rx.o $(STATICLIBOUT)/c_stub.o
	$(CROSS)$(AR) rcs $@ $(STATICLIBOUT)/finite_field.o $(STATICLIBOUT)/tx.o $(STATICLIBOUT)/rx.o $(STATICLIBOUT)/c_stub.o
$(STATICLIBOUT) :
	mkdir -p $(STATICLIBOUT)
STATICLIBCPPFLAGS := -O3 $(BASICCPPFLAGS)
STATICLIBOUT := out/static
$(STATICLIBOUT)/finite_field.o : finite_field.h finite_field.cpp
	$(CROSS)$(CPP) finite_field.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOUT)/tx.o : tx.h tx.cpp
	$(CROSS)$(CPP) tx.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOUT)/rx.o : rx.h rx.cpp
	$(CROSS)$(CPP) rx.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOUT)/c_stub.o : c_stub.h c_stub.cpp
	$(CROSS)$(CPP) c_stub.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
