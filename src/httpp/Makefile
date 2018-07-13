

CXX=g++
CXXFLAGS+=-Wall -std=c++11 -O3 -I..


.PHONY: all clean

all: ../libhttpp.a

clean: 
	rm -f ../libhttpp.a *.o

FILES_HTTPP := HttpServer.cpp
FILES_HTTPP += http/Connection.cpp
FILES_HTTPP += http/Parser.cpp
FILES_HTTPP += http/Protocol.cpp
FILES_HTTPP += http/Request.cpp
FILES_HTTPP += http/Response.cpp
FILES_HTTPP += http/URL.cpp
FILES_HTTPP += http/Utils.cpp
FILES_HTTPP += utils/Thread.cpp 
FILES_HTTPP += utils/ThreadPool.cpp


../libhttpp.a: $(FILES_HTTPP)
	$(CXX) $(CXXFLAGS) -c  $^
	ar rcs $@ $(notdir $(^:.cpp=.o))

