.PHONY : all staticlibrary dynamiclibrary clean
.SUFFIXES : .cpp .o

INCLUDES := -I../basiclibrary/avltree -I../basiclibrary/threadpool -I../basiclibrary/singleshottimer
OPTIMIZATIONCPPFLAGS := -O3
BASICCPPFLAGS := -c -Wall -std=c++0x
CPP := g++

LIBOUT := ../lib
LIBRARY_HEADER := $(LIBOUT)/api.h
STATIC_LIBRARY := $(LIBOUT)/libncsocket.a
STATICLIBOBJOUT := static_lib_objs
DYNAMIC_LIBRARY := $(LIBOUT)/libncsocket.so
DYNAMICLIBOBJOUT := dynamic_lib_objs

all : staticlibrary dynamiclibrary

clean :
	rm -rf $(STATIC_LIBRARY) $(STATICLIBOBJOUT) $(DYNAMIC_LIBRARY) $(DYNAMICLIBOBJOUT)

../basiclibrary/.git/config : 
	git clone https://github.com/dujeonglee/basiclibrary.git ../basiclibrary

$(LIBOUT):
	mkdir -p $(LIBOUT)

$(LIBRARY_HEADER) :
	cp ./api.h $(LIBOUT)

dynamiclibrary : $(DYNAMIC_LIBRARY)
$(DYNAMIC_LIBRARY) : $(DYNAMICLIBOBJOUT) $(LIBOUT) $(LIBRARY_HEADER) ../basiclibrary/.git/config $(DYNAMICLIBOBJOUT)/chk.o $(DYNAMICLIBOBJOUT)/finite_field.o $(DYNAMICLIBOBJOUT)/tx.o $(DYNAMICLIBOBJOUT)/rx.o $(DYNAMICLIBOBJOUT)/api.o
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
$(STATIC_LIBRARY) : $(STATICLIBOBJOUT) $(LIBOUT) $(LIBRARY_HEADER) ../basiclibrary/.git/config $(STATICLIBOBJOUT)/chk.o $(STATICLIBOBJOUT)/finite_field.o $(STATICLIBOBJOUT)/tx.o $(STATICLIBOBJOUT)/rx.o $(STATICLIBOBJOUT)/api.o
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
