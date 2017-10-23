CXX = g++
CXXFLAGS = -O2 -g -std=gnu++14
CXXFLAGS_FUSE = $(CXXFLAGS) -lcrypto -lz `pkg-config fuse --cflags --libs`

all : silo_fuse

silo_fuse : main.o lib/btree.o lib/libsilo.o chunk.o block.o silo.o lib/shtable.o lib/debug.o
	$(CXX) -o $@ $^ $(CXXFLAGS_FUSE)

main.o : main.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS_FUSE)

chunk.o : chunk.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS_FUSE)

block.o : block.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS_FUSE)

silo.o : silo.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS_FUSE)

test : lib/unittest.o lib/btree.o lib/libsilo.o
	$(CXX) -o $@ $^ $(CXXFLAGS)

lib/unittest.o : lib/unittest.cpp lib/btree.o lib/libsilo.o
	$(CXX) -c -o $@ $< $(CXXFLAGS)

lib/btree.o : lib/btree.cpp lib/btree.h
	$(CXX) -c -o $@ $< $(CXXFLAGS)

lib/libsilo.o : lib/libsilo.cpp lib/libsilo.h
	$(CXX) -c -o $@ $< $(CXXFLAGS)

lib/shtable.o: lib/shtable.cpp lib/shtable.h lib/btree.o
	$(CXX) -c -o $@ $< $(CXXFLAGS)

lib/debug.o: lib/debug.cpp lib/debug.h
	$(CXX) -c -o $@ $< $(CXXFLAGS)

dup : lib/duptest.o lib/shtable.o lib/btree.o lib/libsilo.o
	$(CXX) -o $@ $^ $(CXXFLAGS) -lcrypto

lib/duptest.o : lib/duptest.cpp lib/shtable.o lib/btree.o lib/libsilo.o
	$(CXX) -c -o $@ $< $(CXXFLAGS) -lcrypto

clean :
	$(RM) *.o lib/*.o silo_fuse test dup
