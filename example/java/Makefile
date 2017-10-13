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
	java -classpath "$(LIBDIR):." -Djava.library.path=$(LIBDIR) Main $(LOCAL_PORT) $(REMOTE_IP) $(REMOTE_PORT)
