ifeq ($(OS),Windows_NT)
    UNAME_S := Windows
else
    UNAME_S := $(shell uname -s)
endif


ifeq ($(UNAME_S),Linux)
	CXXFLAGS = -g -rdynamic -Wl,--no-as-needed -ldl -Wall
	CXXFLAGSNOLINKER = -g -Wl,--no-as-needed -ldl -Wall
endif
ifeq ($(UNAME_S),Darwin)
	CXXFLAGS = -std=c++14 -g -rdynamic -ldl -Wall
	CXXFLAGSNOLINKER = -std=c++14 -g -Wall
endif

# TODO: handle Windows (mingw?)

all: hh_sim

hh_sim: hh_sim.cpp
	$(CXX) $(CXXFLAGS) hh_sim.cpp -o hh_sim
