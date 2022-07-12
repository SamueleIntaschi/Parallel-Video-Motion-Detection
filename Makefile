CXX = g++
CXXFLAGS = -std=c++17 
LDFLAGS = -pthread -O3 -ftree-vectorize `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

EXE = ff seq nt res

ff: ff.cpp
	$(CXX) -o ff ff.cpp $(LDFLAGS)

fftrace: ff.cpp
	$(CXX) -DTRACE_FASTFLOW -o ff ff.cpp $(LDFLAGS)

nt: nthreads.cpp
	$(CXX) -o nt nthreads.cpp $(LDFLAGS)

seq: sequential.cpp
	$(CXX) -o seq sequential.cpp $(LDFLAGS)

res: results.cpp
	$(CXX) -o res results.cpp

clean:
	rm $(EXE)
