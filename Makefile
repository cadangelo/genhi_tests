include /home/chelsea/opt/myMOAB/lib/moab.make 

#INC = -I/home/chelsea/opt/myMOAB/include
#INC = -I/home/chelsea//moab/tools/dagmc
#LIBS = -L/home/chelsea/opt/myMOAB/lib
#MOAB_CXXFLAGS =  -Wall -pipe -pedantic -Wno-long-long ${INC} ${LIBS}
CXXFLAGS += ${MOAB_CXXFLAGS} ${MOAB_INCLUDES} ${MOAB_CPPFLAGS} -g 
CXX = g++

MOAB_LIBS_LINK = ${MOAB_LDFLAGS} -L${MOAB_LIBDIR} -ldagmc -lMOAB    -lhdf5    -lpthread -lz -ldl -lm     -lpthread -lz -ldl -lm
#MOAB_LIBS_LINK = ${MOAB_LDFLAGS}  -L${MOAB_LIBDIR} -lMOAB

all: generate_cube 

generate_cube.o: generate_cube.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@ 

generate_cube: generate_cube.o
	$(CXX) $(CXXFLAGS) $^ $(MOAB_LIBS_LINK) -static -o $@
	

clean: 
	rm -f *.o generate_cube
