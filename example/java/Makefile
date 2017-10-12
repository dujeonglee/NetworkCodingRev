.PHONY: all clean

LIBDIR := ../../lib
NATIVELIB := $(LIBDIR)/libjncsocket.so
JAVACLASS := $(LIBDIR)/dujeonglee/networkcoding/Japi.class

all : $(NATIVELIB) $(JAVACLASS) 
	javac -classpath $(LIBDIR) Main.java
$(NATIVELIB) : 
	$(MAKE) -C ../../java
$(JAVACLASS) :
	$(MAKE) -C ../../java
clean :
	rm -rf *.class 

run :
	java -classpath "$(LIBDIR):." -Djava.library.path=$(LIBDIR) Main 10000 172.17.0.3 10000
