.PHONY : all javalibrary clean
.SUFFIXES : .cpp .o

INCLUDES := -I../basiclibrary/avltree -I../basiclibrary/threadpool -I../basiclibrary/singleshottimer
OPTIMIZATIONCPPFLAGS := -O3
BASICCPPFLAGS := -c -Wall -std=c++0x -D_GLIBCXX_USE_NANOSLEEP=1
CPP := g++

LIBOUT := ../lib
LIBRARY_HEADER := $(LIBOUT)/api.h
JAVACLASS := $(LIBOUT)/dujeonglee/networkcoding/Japi.class
JAVA_LIBRARY := $(LIBOUT)/libjncsocket.so
JAVALIBOBJOUT := java_lib_objs

JNI_PATH := $(shell find /usr/lib/jvm -name jni.h)
JNI_INCLUDE := $(join -I, $(subst /jni.h, , $(JNI_PATH)))
JNI_LINUX_INCLUDE := $(join $(JNI_INCLUDE), /linux)
JAVA_INCLUDE := $(JNI_INCLUDE) $(JNI_LINUX_INCLUDE)
ifeq ($(JNI_PATH),)
    $(error jdk is not found)
endif

all : javalibrary

clean :
	rm -rf $(JAVALIBOBJOUT) $(JAVA_LIBRARY) $(LIBOUT)/dujeonglee

basiclibrary/.git/config : 
	git clone https://github.com/dujeonglee/basiclibrary.git ../basiclibrary

$(LIBOUT):
	mkdir -p $(LIBOUT)

$(LIBRARY_HEADER) : api.h
	cp ./api.h $(LIBOUT)

javalibrary : $(JAVA_LIBRARY) $(JAVACLASS)
$(JAVA_LIBRARY) : $(JAVALIBOBJOUT) $(LIBOUT) $(LIBRARY_HEADER) ../basiclibrary/.git/config $(JAVALIBOBJOUT)/chk.o $(JAVALIBOBJOUT)/finite_field.o $(JAVALIBOBJOUT)/tx.o $(JAVALIBOBJOUT)/rx.o $(JAVALIBOBJOUT)/api.o $(JAVALIBOBJOUT)/dujeonglee_networkcoding_Japi.o
	$(CROSS)$(CPP) -shared -o $@ $(JAVALIBOBJOUT)/chk.o $(JAVALIBOBJOUT)/finite_field.o $(JAVALIBOBJOUT)/tx.o $(JAVALIBOBJOUT)/rx.o $(JAVALIBOBJOUT)/api.o $(JAVALIBOBJOUT)/dujeonglee_networkcoding_Japi.o
$(JAVALIBOBJOUT) :
	mkdir -p $(JAVALIBOBJOUT)
JAVALIBCPPFLAGS := -fPIC $(OPTIMIZATIONCPPFLAGS) $(BASICCPPFLAGS)
$(JAVALIBOBJOUT)/chk.o : Makefile common.h encoding_decoding_macro.h debug.h ncsocket.h chk.h chk.cpp
	$(CROSS)$(CPP) chk.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/finite_field.o : Makefile common.h encoding_decoding_macro.h debug.h ncsocket.h finite_field.h finite_field.cpp
	$(CROSS)$(CPP) finite_field.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/tx.o : Makefile common.h encoding_decoding_macro.h debug.h ncsocket.h tx.h tx.cpp
	$(CROSS)$(CPP) tx.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/rx.o : Makefile common.h encoding_decoding_macro.h debug.h ncsocket.h rx.h rx.cpp
	$(CROSS)$(CPP) rx.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/api.o : Makefile common.h encoding_decoding_macro.h debug.h ncsocket.h api.h api.cpp
	$(CROSS)$(CPP) api.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES)
$(JAVALIBOBJOUT)/dujeonglee_networkcoding_Japi.o : Makefile common.h encoding_decoding_macro.h debug.h ncsocket.h dujeonglee_networkcoding_Japi.h dujeonglee_networkcoding_Japi.cpp
	$(CROSS)$(CPP) dujeonglee_networkcoding_Japi.cpp -o $@ $(JAVALIBCPPFLAGS) $(INCLUDES) $(JAVA_INCLUDE)
$(JAVACLASS) : Japi.java
	javac -d $(LIBOUT) ./Japi.java

ncsocket.h :
	ln -s ../c_cpp/$@ ./
chk.h :
	ln -s ../c_cpp/$@ ./
chk.cpp :
	ln -s ../c_cpp/$@ ./
finite_field.h :
	ln -s ../c_cpp/$@ ./
finite_field.cpp :
	ln -s ../c_cpp/$@ ./
tx.h :
	ln -s ../c_cpp/$@ ./
tx.cpp :
	ln -s ../c_cpp/$@ ./
rx.h :
	ln -s ../c_cpp/$@ ./
rx.cpp :
	ln -s ../c_cpp/$@ ./
api.h :
	ln -s ../c_cpp/$@ ./
api.cpp :
	ln -s ../c_cpp/$@ ./
common.h :
	ln -s ../c_cpp/$@ ./
encoding_decoding_macro.h :
	ln -s ../c_cpp/$@ ./
debug.h :
	ln -s ../c_cpp/$@ ./
