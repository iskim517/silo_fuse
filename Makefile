CXX = g++
CXXFLAGS = -O0 -g -std=gnu++11 `pkg-config fuse --cflags --libs`

all : silo_fuse

silo_fuse : main.o
	$(CXX) -o $@ $< $(CXXFLAGS)

main.o : main.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

clean :
	$(RM) main.o silo_fuse
