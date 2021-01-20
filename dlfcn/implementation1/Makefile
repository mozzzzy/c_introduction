CXX = g++
CXXFLAGS = -shared -fPIC
CPPFLAGS =
LDFLAGS =
LDLIBS =

ARTIFACT = ./libimplementation1.so
CXXSRC = ./implementation1.cc

$(ARTIFACT): $(CXXSRC)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -o $(ARTIFACT) $(CXXSRC)

test: $(ARTIFACT)
	$(ARTIFACT)

clean:
	rm $(ARTIFACT)
