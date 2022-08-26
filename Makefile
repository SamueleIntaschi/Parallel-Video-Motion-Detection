CXX = g++
CXXFLAGS = -std=c++17 
LDFLAGS = -pthread -O3 -ftree-vectorize `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

EXE = fffarm ffmw seq nt res

fffarm: fffarm.cpp
	$(CXX) -DFF_BOUNDED_BUFFER -DDEFAULT_BUFFER_CAPACITY=10 -o fffarm fffarm.cpp $(LDFLAGS)

fffarmtrace: fffarm.cpp
	$(CXX) -DTRACE_FASTFLOW -DFF_BOUNDED_BUFFER -DDEFAULT_BUFFER_CAPACITY=10 -o fffarm fffarm.cpp $(LDFLAGS)

ffmw: ffmw.cpp
	$(CXX) -DFF_BOUNDED_BUFFER -DDEFAULT_BUFFER_CAPACITY=10 -o ffmw ffmw.cpp $(LDFLAGS)

ffmwtrace: ffmw.cpp
	$(CXX) -DTRACE_FASTFLOW -DFF_BOUNDED_BUFFER -DDEFAULT_BUFFER_CAPACITY=10 -o ffmw ffmw.cpp $(LDFLAGS)

nt: nthreads.cpp
	$(CXX) -o nt nthreads.cpp $(LDFLAGS)

seq: sequential.cpp
	$(CXX) -o seq sequential.cpp $(LDFLAGS)

res: results.cpp
	$(CXX) -o res results.cpp

clean:
	rm $(EXE)