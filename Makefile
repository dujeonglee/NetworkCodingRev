.PHONY : all debug release staticlibrary sharedlibrary clean
.SUFFIXES : .cpp .o

INCLUDES := -I./basiclibrary/avltree -I./basiclibrary/threadpool -I./basiclibrary/singleshottimer
OPTIMIZATIONCPPFLAGS := -O3
BASICCPPFLAGS := -c -Wall -std=c++0x
CPP := g++

LIBOUT := out/lib
LIBRARY_HEADER := $(LIBOUT)/api.h
STATIC_LIBRARY := $(LIBOUT)/libncsocket.a
STATICLIBOBJOUT := out/static_lib_objs
SHARED_LIBRARY := $(LIBOUT)/libncsocket.so
SHAREDLIBOBJOUT := out/shared_lib_objs

all : release

dynamic : run_dynamic
static : run_static

run_static : main_stc.o $(STATIC_LIBRARY)
	$(CROSS)$(CPP) -o $@ main_stc.o $(STATIC_LIBRARY) -lpthread
run_dynamic : main_dyn.o $(SHARED_LIBRARY)
	$(CROSS)$(CPP) -o $@ main_dyn.o -lpthread -ldl
main_stc.o : Makefile main.cpp
	$(CROSS)$(CPP) -o $@ main.cpp $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS) $(INCLUDES)
main_dyn.o : Makefile main.cpp
	$(CROSS)$(CPP) -o $@ main.cpp -DDYNAMIC $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS) $(INCLUDES)

clean :
	rm -rf *.o *.so *.a *~ gmon.out *.bak *.stackdump run_* ./out

basiclibrary/.git/config : 
	git clone https://github.com/dujeonglee/basiclibrary.git

$(LIBOUT):
	mkdir -p $(LIBOUT)

$(LIBRARY_HEADER) :
	cp ./api.h $(LIBOUT)

sharedlibrary : $(SHARED_LIBRARY)
$(SHARED_LIBRARY) : $(SHAREDLIBOBJOUT) $(LIBOUT) $(LIBRARY_HEADER) basiclibrary/.git/config $(SHAREDLIBOBJOUT)/chk.o $(SHAREDLIBOBJOUT)/finite_field.o $(SHAREDLIBOBJOUT)/tx.o $(SHAREDLIBOBJOUT)/rx.o $(SHAREDLIBOBJOUT)/api.o
	$(CROSS)$(CPP) -shared -o $@ $(SHAREDLIBOBJOUT)/chk.o $(SHAREDLIBOBJOUT)/finite_field.o $(SHAREDLIBOBJOUT)/tx.o $(SHAREDLIBOBJOUT)/rx.o $(SHAREDLIBOBJOUT)/api.o
$(SHAREDLIBOBJOUT) :
	mkdir -p $(SHAREDLIBOBJOUT)
SHAREDLIBCPPFLAGS := -fPIC $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS)
$(SHAREDLIBOBJOUT)/chk.o : Makefile chk.h chk.cpp
	$(CROSS)$(CPP) chk.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOBJOUT)/finite_field.o : Makefile finite_field.h finite_field.cpp
	$(CROSS)$(CPP) finite_field.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOBJOUT)/tx.o : Makefile tx.h tx.cpp
	$(CROSS)$(CPP) tx.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOBJOUT)/rx.o : Makefile rx.h rx.cpp
	$(CROSS)$(CPP) rx.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOBJOUT)/api.o : Makefile api.h api.cpp
	$(CROSS)$(CPP) api.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)

staticlibrary : $(STATIC_LIBRARY)
$(STATIC_LIBRARY) : $(STATICLIBOBJOUT) $(LIBOUT) $(LIBRARY_HEADER) basiclibrary/.git/config $(STATICLIBOBJOUT)/chk.o $(STATICLIBOBJOUT)/finite_field.o $(STATICLIBOBJOUT)/tx.o $(STATICLIBOBJOUT)/rx.o $(STATICLIBOBJOUT)/api.o
	$(CROSS)$(AR) rcs $@  $(STATICLIBOBJOUT)/chk.o $(STATICLIBOBJOUT)/finite_field.o $(STATICLIBOBJOUT)/tx.o $(STATICLIBOBJOUT)/rx.o $(STATICLIBOBJOUT)/api.o
$(STATICLIBOBJOUT) :
	mkdir -p $(STATICLIBOBJOUT)
STATICLIBCPPFLAGS := $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS)
$(STATICLIBOBJOUT)/chk.o : Makefile chk.h chk.cpp
	$(CROSS)$(CPP) chk.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOBJOUT)/finite_field.o : Makefile finite_field.h finite_field.cpp
	$(CROSS)$(CPP) finite_field.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOBJOUT)/tx.o : Makefile tx.h tx.cpp
	$(CROSS)$(CPP) tx.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOBJOUT)/rx.o : Makefile rx.h rx.cpp
	$(CROSS)$(CPP) rx.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOBJOUT)/api.o : Makefile api.h api.cpp
	$(CROSS)$(CPP) api.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
