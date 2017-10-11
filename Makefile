.PHONY : all debug release staticlibrary dynamiclibrary clean
.SUFFIXES : .cpp .o

INCLUDES := -I./basiclibrary/avltree -I./basiclibrary/threadpool -I./basiclibrary/singleshottimer
OPTIMIZATIONCPPFLAGS := -O3
BASICCPPFLAGS := -c -Wall -std=c++0x
CPP := g++

LIBOUT := out/lib
LIBRARY_HEADER := $(LIBOUT)/api.h
STATIC_LIBRARY := $(LIBOUT)/libncsocket.a
STATICLIBOBJOUT := out/static_lib_objs
DYNAMIC_LIBRARY := $(LIBOUT)/libncsocket.so
DYNAMICLIBOBJOUT := out/dynamic_lib_objs
JAVA_LIBRARY := $(LIBOUT)/libjncsocket.so
JAVALIBOBJOUT := out/java_lib_objs

all : release

dynamic : run_dynamic
static : run_static

run_static : main_stc.o $(STATIC_LIBRARY)
	$(CROSS)$(CPP) -o $@ main_stc.o $(STATIC_LIBRARY) -lpthread
run_dynamic : main_dyn.o $(DYNAMIC_LIBRARY)
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

dynamiclibrary : $(DYNAMIC_LIBRARY)
$(DYNAMIC_LIBRARY) : $(DYNAMICLIBOBJOUT) $(LIBOUT) $(LIBRARY_HEADER) basiclibrary/.git/config $(DYNAMICLIBOBJOUT)/chk.o $(DYNAMICLIBOBJOUT)/finite_field.o $(DYNAMICLIBOBJOUT)/tx.o $(DYNAMICLIBOBJOUT)/rx.o $(DYNAMICLIBOBJOUT)/api.o
	$(CROSS)$(CPP) -shared -o $@ $(DYNAMICLIBOBJOUT)/chk.o $(DYNAMICLIBOBJOUT)/finite_field.o $(DYNAMICLIBOBJOUT)/tx.o $(DYNAMICLIBOBJOUT)/rx.o $(DYNAMICLIBOBJOUT)/api.o
$(DYNAMICLIBOBJOUT) :
	mkdir -p $(DYNAMICLIBOBJOUT)
DYNAMICLIBCPPFLAGS := -fPIC $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS)
$(DYNAMICLIBOBJOUT)/chk.o : Makefile chk.h chk.cpp
	$(CROSS)$(CPP) chk.cpp -o $@ $(DYNAMICLIBCPPFLAGS) $(INCLUDES)
$(DYNAMICLIBOBJOUT)/finite_field.o : Makefile finite_field.h finite_field.cpp
	$(CROSS)$(CPP) finite_field.cpp -o $@ $(DYNAMICLIBCPPFLAGS) $(INCLUDES)
$(DYNAMICLIBOBJOUT)/tx.o : Makefile tx.h tx.cpp
	$(CROSS)$(CPP) tx.cpp -o $@ $(DYNAMICLIBCPPFLAGS) $(INCLUDES)
$(DYNAMICLIBOBJOUT)/rx.o : Makefile rx.h rx.cpp
	$(CROSS)$(CPP) rx.cpp -o $@ $(DYNAMICLIBCPPFLAGS) $(INCLUDES)
$(DYNAMICLIBOBJOUT)/api.o : Makefile api.h api.cpp
	$(CROSS)$(CPP) api.cpp -o $@ $(DYNAMICLIBCPPFLAGS) $(INCLUDES)

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

javalibrary : $(JAVA_LIBRARY)
$(JAVA_LIBRARY) : $(JAVALIBOBJOUT) $(LIBOUT) $(LIBRARY_HEADER) basiclibrary/.git/config $(JAVALIBOBJOUT)/chk.o $(JAVALIBOBJOUT)/finite_field.o $(JAVALIBOBJOUT)/tx.o $(JAVALIBOBJOUT)/rx.o $(JAVALIBOBJOUT)/api.o $(JAVALIBOBJOUT)/Japi.o
	$(CROSS)$(CPP) -shared -o $@ $(JAVALIBOBJOUT)/chk.o $(JAVALIBOBJOUT)/finite_field.o $(JAVALIBOBJOUT)/tx.o $(JAVALIBOBJOUT)/rx.o $(JAVALIBOBJOUT)/api.o $(JAVALIBOBJOUT)/Japi.o
$(JAVALIBOBJOUT) :
	mkdir -p $(JAVALIBOBJOUT)
JAVALIBCPPFLAGS := -fPIC $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS)
$(JAVALIBOBJOUT)/chk.o : Makefile chk.h chk.cpp
	$(CROSS)$(CPP) chk.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/finite_field.o : Makefile finite_field.h finite_field.cpp
	$(CROSS)$(CPP) finite_field.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/tx.o : Makefile tx.h tx.cpp
	$(CROSS)$(CPP) tx.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/rx.o : Makefile rx.h rx.cpp
	$(CROSS)$(CPP) rx.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/api.o : Makefile api.h api.cpp
	$(CROSS)$(CPP) api.cpp -o $@ $(DYNAMICLIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/Japi.o : Makefile Japi.h Japi.cpp
	$(CROSS)$(CPP) Japi.cpp -o $@ $(DYNAMICLIBCPPFLAGS) $(INCLUDES) -I/usr/lib/jvm/java-8-openjdk-amd64/include/ -I/usr/lib/jvm/java-8-openjdk-amd64/include/linux
