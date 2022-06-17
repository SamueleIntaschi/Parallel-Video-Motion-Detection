CXX = g++
CXXFLAGS = -std=c++17 
LDFLAGS = -pthread -O3 `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

EXE = native ff seq ntstream

ff: ff.cpp
	$(CXX) -o ff ff.cpp $(LDFLAGS)

ffblock: ff.cpp
	$(CXX) -DBLOCKING_MODE=true -o ff ff.cpp $(LDFLAGS)

ffnomap: ff.cpp 
	$(CXX) -DNO_DEFAULT_MAPPING=true -o ff ff.cpp $(LDFLAGS)

cthreads: cthreads.cpp
	$(CXX) -o native cthreads.cpp $(LDFLAGS)

ntstream: cthreads.cpp
	$(CXX) -o ntstream cthreads_stream.cpp $(LDFLAGS)


seq: sequential.cpp
	$(CXX) -o seq sequential.cpp $(LDFLAGS)

clean:
	rm $(EXE)
