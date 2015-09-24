include /home/chelsea/opt/myMOAB/lib/moab.make 

INC = -I/home/chelsea/opt/myMOAB/include
LIBS = -L/home/chelsea/opt/myMOAB/lib
MOAB_CXXFLAGS =  -Wall -pipe -pedantic -Wno-long-long ${INC} ${LIBS}
CXXFLAGS += ${MOAB_CXXFLAGS} -g 
CXX = g++

all: generate_cube 

generate_cube.o: generate_cube.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@ 

generate_cube: generate_cube.o
	$(CXX) $(CXXFLAGS) $(LD_FLAGS) $^ -o $@
	

clean: 
	rm -f *.o generate_cube
