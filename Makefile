.PHONY : all debug release backup library
.SUFFIXES : .cpp .o

SOURCES  := $(wildcard *.cpp)
INCLUDES := -I./basiclibrary/avltree -I./basiclibrary/threadpool -I./basiclibrary/singleshottimer
OBJECTS  := $(SOURCES:.cpp=.o)
LIBRARY := -lpthread
CPP := g++
#CPP := arm-linux-gnueabihf-g++
TARGET = run

all : debug

$(TARGET) : $(OBJECTS)
	$(CPP) -o $@  $^ $(LIBRARY)

.cpp.o : $(SOURCES) 
	$(CPP) $(SOURCES) $(CPPFLAGS) -DNONPRIMITIVE_KEY $(INCLUDES) $(LIBRARY)

./basiclibrary/.git/config : 
	git clone https://github.com/dujeonglee/basiclibrary.git

clean :
	rm -rf $(OBJECTS) $(TARGET) *~ gmon.out *.bak *.stackdump

debug : CPPFLAGS := -g -c -Wall -std=c++11
debug : ./basiclibrary/.git/config
debug : $(TARGET)

release : CPPFLAGS := -O3 -c -Wall -std=c++11
release : ./basiclibrary/.git/config
release : $(TARGET)
#	./$(TARGET)
#	gprof ./$(TARGET) gmon.out
