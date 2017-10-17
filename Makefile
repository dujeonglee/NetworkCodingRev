.PHONY : all c_cpp java example clean

JNI_PATH := $(shell find /usr/lib/jvm -name jni.h)

all : c_cpp java example

c_cpp :
	$(MAKE) -C c_cpp
java :
ifneq ($(JNI_PATH),)
	$(MAKE) -C java
else
	echo "Skip building java library. jdk is not found"
endif
example : 
	$(MAKE) -C example/c_cpp
ifneq ($(JNI_PATH),)
	$(MAKE) -C example/java
else
	echo "Skip building java example. jdk is not found"
endif
clean :
	$(MAKE) -C c_cpp clean	
	$(MAKE) -C java clean
	$(MAKE) -C example/c_cpp clean
	$(MAKE) -C example/java clean
	rm -rf ./lib
