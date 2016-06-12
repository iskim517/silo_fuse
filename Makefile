CXX = g++
CXXFLAGS = -O2 -g -std=gnu++14 -lcrypto -lz `pkg-config fuse --cflags --libs`
CXXFLAGSNOFUSE = -O2 -g -std=gnu++14

all : silo_fuse

silo_fuse : main.o lib/btree.o lib/libsilo.o chunk.o block.o silo.o lib/shtable.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

main.o : main.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

chunk.o : chunk.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

block.o : block.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

silo.o : silo.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

test : lib/unittest.o lib/btree.o lib/libsilo.o
	$(CXX) -o $@ $^ $(CXXFLAGSNOFUSE)

lib/unittest.o : lib/unittest.cpp lib/btree.o lib/libsilo.o
	$(CXX) -c -o $@ $< $(CXXFLAGSNOFUSE)

lib/btree.o : lib/btree.cpp lib/btree.h
	$(CXX) -c -o $@ $< $(CXXFLAGSNOFUSE)

lib/libsilo.o : lib/libsilo.cpp lib/libsilo.h
	$(CXX) -c -o $@ $< $(CXXFLAGSNOFUSE)

clean :
	$(RM) *.o lib/*.o silo_fuse test
