.PHONY : all c_cpp java example clean

all : c_cpp java example

c_cpp :
	$(MAKE) -C c_cpp
java :
	$(MAKE) -C java
example : 
	$(MAKE) -C example/c_cpp
	$(MAKE) -C example/java
clean :
	$(MAKE) -C c_cpp clean	
	$(MAKE) -C java clean
	$(MAKE) -C example/c_cpp clean
	$(MAKE) -C example/java clean
	rm -rf ./lib
