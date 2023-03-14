CXX = g++
DEBUG = 1
# ifeq ($(DEBUG), 1)
#     CXXFLAGS += -g
# else
#     CXXFLAGS += -O2
# endif

CXXFLAGS += -std=c++17
CXXFLAGS += -Wall -Wextra -Wno-sign-compare
CXXFLAGS += -lpthread
CXXFLAGS += -Og -g -flto

server: src/main.cc Logger.o HttpResponseBuilder.o HttpResponseBuilder.o TcpSocket.o WebServer.o Logger.o HttpContext.o HttpParser.o Mime.o DefaultErrorPages.o
	$(CXX) -o server.out  $^ $(CXXFLAGS)

Logger.o: src/Logger.cc
	$(CXX) -o Logger.o $^ -c $(CXXFLAGS)

HttpResponseBuilder.o: src/HttpResponseBuilder.cc
	$(CXX) -o HttpResponseBuilder.o $^ -c $(CXXFLAGS)

HttpParser.o: src/HttpParser.cc
	$(CXX) -o HttpParser.o $^ -c $(CXXFLAGS)

TcpSocket.o: src/TcpSocket.cc
	$(CXX) -o TcpSocket.o $^ -c $(CXXFLAGS)

HttpContext.o: src/HttpContext.cc
	$(CXX) -o HttpContext.o $^ -c $(CXXFLAGS)

WebServer.o: src/WebServer.cc
	$(CXX) -o WebServer.o $^ -c $(CXXFLAGS)

Mime.o: src/Mime.cc
	$(CXX) -o Mime.o $^ -c $(CXXFLAGS)

DefaultErrorPages.o: src/DefaultErrorPages.cc
	$(CXX) -o DefaultErrorPages.o $^ -c $(CXXFLAGS)

clean:
	rm DefaultErrorPages.o HttpResponseBuilder.o TcpSocket.o WebServer.o Logger.o HttpContext.o HttpParser.o Mime.o server.out