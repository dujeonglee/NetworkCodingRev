.PHONY : all debug release backup library
.SUFFIXES : .cpp .o

SOURCES  := $(wildcard *.cpp)
INCLUDES := -I./basiclibrary/avltree -I./basiclibrary/threadpool -I./basiclibrary/singleshottimer
OBJECTS  := $(SOURCES:.cpp=.o)
LIBRARY := -lpthread
CPP := g++
TARGET = ncsocket


$(TARGET) : $(OBJECTS)
	$(CROSS)$(CPP) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(LIBRARY)

.cpp.o : $(SOURCES) 
	$(CROSS)$(CPP) $(SOURCES) $(CPPFLAGS) $(INCLUDES) $(LIBRARY)

./basiclibrary/.git/config : 
	git clone https://github.com/dujeonglee/basiclibrary.git

all : debug

clean :
	rm -rf $(OBJECTS) $(TARGET) *~ gmon.out *.bak *.stackdump

debug : CPPFLAGS := -g -c -Wall -std=c++0x
debug : ./basiclibrary/.git/config
debug : $(TARGET)

release : CPPFLAGS := -O3 -c -Wall -std=c++0x
release : ./basiclibrary/.git/config
release : $(TARGET)


library : TARGET := libncsocket.so
library : CPPFLAGS := -fPIC -O3 -c -Wall -std=c++0x
library : LDFLAGS := -shared -Wl,-soname,libncsocket.so.1
library : SOURCES  := $(filter-out main.cpp , $(wildcard *.cpp))
library : OBJECTS  := $(SOURCES:.cpp=.o)
library : $(TARGET)

