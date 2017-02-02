.PHONY : all debug release backup
.SUFFIXES : .cpp .o

SOURCES  := $(wildcard *.cpp)
INCLUDES := 
OBJECTS  := $(SOURCES:.cpp=.o)
LIBRARY := -lpthread
CPP := g++
#CPP := arm-linux-gnueabihf-g++
TARGET = test

all : debug

$(TARGET) : $(OBJECTS)
	$(CPP) -o $@  $^ -pg $(LIBRARY)

.cpp.o : $(SOURCES)
	$(CPP) $(SOURCES) $(CPPFLAGS) $(INCLUDES) $(LIBRARY)

clean :
	rm -rf $(OBJECTS) $(TARGET) *~ gmon.out *.bak

debug : CPPFLAGS := -g -pg -c -Wall -std=c++11 -fopenmp
debug : $(TARGET)

release : CPPFLAGS := -O3 -pg -c -Wall -std=c++11 -fopenmp
release : $(TARGET)
#	./$(TARGET)
#	gprof ./$(TARGET) gmon.out
