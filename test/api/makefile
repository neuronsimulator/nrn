ifeq ($(OS),Windows_NT)
    UNAME_S := Windows
else
    UNAME_S := $(shell uname -s)
endif


ifeq ($(UNAME_S),Linux)
	CXXFLAGS = -g -rdynamic -Wl,--no-as-needed -ldl -Wall
	CCFLAGS = -g -rdynamic -Wl,--no-as-needed -ldl -Wall
	CXXFLAGSNOLINKER = -g -Wl,--no-as-needed -ldl -Wall
endif
ifeq ($(UNAME_S),Darwin)
	CXXFLAGS = -std=c++14 -g -rdynamic -ldl -Wall
	CCFLAGS = -g -rdynamic -ldl -Wall
	CXXFLAGSNOLINKER = -std=c++14 -g -Wall
endif

# TODO: handle Windows (mingw?)

all: hh_sim vclamp sections netcon

hh_sim: hh_sim.cpp
	$(CXX) $(CXXFLAGS) hh_sim.cpp -o hh_sim

vclamp: vclamp.cpp 
	$(CXX) $(CXXFLAGS) vclamp.cpp -o vclamp

sections: sections.c
	$(CC) $(CCFLAGS) sections.c -o sections

# TODO: dynamic lookup is probably not possible on Windows; need to have a 
#       version of the library available during linking (but won't have to be
#       embedded)
netcon: netcon.cpp
	$(CXX) $(CXXFLAGS) -undefined dynamic_lookup netcon.cpp -o netcon

# TODO: introspection example

# TODO: morphology example with PlotShape