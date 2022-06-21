CXX = g++
CXXFLAGS = -std=c++17 
LDFLAGS = -pthread -O3 `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

EXE = ff seq nt res

ff: ff.cpp
	$(CXX) -o ff ff.cpp $(LDFLAGS)

ffblock: ff.cpp
	$(CXX) -DBLOCKING_MODE=true -o ffblock ff.cpp $(LDFLAGS)

fftrace: ff.cpp
	$(CXX) -DTRACE_FASTFLOW -o ff ff.cpp $(LDFLAGS)

ffnomap: ff.cpp 
	$(CXX) -DNO_DEFAULT_MAPPING=true -o ffnomap ff.cpp $(LDFLAGS)

cthreads: cthreads.cpp
	$(CXX) -o native cthreads.cpp $(LDFLAGS)

nt: nthreads.cpp
	$(CXX) -o nt nthreads.cpp $(LDFLAGS)

seq: sequential.cpp
	$(CXX) -o seq sequential.cpp $(LDFLAGS)

res: results.cpp
	$(CXX) -o res results.cpp

clean:
	rm $(EXE)
