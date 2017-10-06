.PHONY : all debug release staticlibrary sharedlibrary clean
.SUFFIXES : .cpp .o

INCLUDES := -I./basiclibrary/avltree -I./basiclibrary/threadpool -I./basiclibrary/singleshottimer
CPP := g++

./basiclibrary/.git/config : 
	git clone https://github.com/dujeonglee/basiclibrary.git

all : debug

clean :
	rm -rf *.o *.so *.a *~ gmon.out *.bak *.stackdump run ./out

SHAREDLIBCPPFLAGS := -fPIC -O3 -c -Wall -std=c++0x
SHAREDLIBOUT := out/shared
$(SHAREDLIBOUT) :
	mkdir -p $(SHAREDLIBOUT)
$(SHAREDLIBOUT)/finite_field.o : finite_field.h finite_field.cpp
	$(CROSS)$(CPP) finite_field.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOUT)/tx.o : tx.h tx.cpp
	$(CROSS)$(CPP) tx.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOUT)/rx.o : rx.h rx.cpp
	$(CROSS)$(CPP) rx.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
$(SHAREDLIBOUT)/c_stub.o : c_stub.h c_stub.cpp
	$(CROSS)$(CPP) c_stub.cpp -o $@ $(SHAREDLIBCPPFLAGS) $(INCLUDES)
libncsocket.so : $(SHAREDLIBOUT) $(SHAREDLIBOUT)/finite_field.o $(SHAREDLIBOUT)/tx.o $(SHAREDLIBOUT)/rx.o $(SHAREDLIBOUT)/c_stub.o
	$(CROSS)$(CPP) -shared -o $@ $(SHAREDLIBOUT)/finite_field.o $(SHAREDLIBOUT)/tx.o $(SHAREDLIBOUT)/rx.o $(SHAREDLIBOUT)/c_stub.o
sharedlibrary : libncsocket.so

STATICLIBCPPFLAGS := -O3 -c -Wall -std=c++0x
STATICLIBOUT := out/static
$(STATICLIBOUT) :
	mkdir -p $(STATICLIBOUT)
$(STATICLIBOUT)/finite_field.o : finite_field.h finite_field.cpp
	$(CROSS)$(CPP) finite_field.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOUT)/tx.o : tx.h tx.cpp
	$(CROSS)$(CPP) tx.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOUT)/rx.o : rx.h rx.cpp
	$(CROSS)$(CPP) rx.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
$(STATICLIBOUT)/c_stub.o : c_stub.h c_stub.cpp
	$(CROSS)$(CPP) c_stub.cpp -o $@ $(STATICLIBCPPFLAGS) $(INCLUDES)
libncsocket.a : $(STATICLIBOUT) $(STATICLIBOUT)/finite_field.o $(STATICLIBOUT)/tx.o $(STATICLIBOUT)/rx.o $(STATICLIBOUT)/c_stub.o
	$(CROSS)$(AR) rcs $@ $(STATICLIBOUT)/finite_field.o $(STATICLIBOUT)/tx.o $(STATICLIBOUT)/rx.o $(STATICLIBOUT)/c_stub.o
staticlibrary : libncsocket.a

debug : CPPFLAGS := -g -c -Wall -std=c++0x
debug : ./basiclibrary/.git/config
debug : run

release : CPPFLAGS := -O3 -c -Wall -std=c++0x
release : ./basiclibrary/.git/config
release : run

main.o : main.cpp
	$(CROSS)$(CPP) -o $@ main.cpp $(CPPFLAGS) $(INCLUDES)

run : main.o libncsocket.so libncsocket.a
	$(CROSS)$(CPP) -o $@ main.o ./libncsocket.a -lpthread -ldl