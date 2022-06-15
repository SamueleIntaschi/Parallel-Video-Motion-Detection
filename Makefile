CXX = g++
CXXFLAGS = -std=c++17 
LDFLAGS = -pthread -O3 `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

EXE = native ff seq

ff: ff.cpp 
	$(CXX) -DNO_DEFAULT_MAPPING=tru -o ff ff.cpp $(LDFLAGS)

cthreads: cthreads.cpp
	$(CXX) -o native cthreads.cpp $(LDFLAGS)

seq: sequential.cpp
	$(CXX) -o seq sequential.cpp $(LDFLAGS)

clean:
	rm $(EXE)
