CXX = g++
CPPFLAGS =
LDFLAGS = -ldl -limplementation1
LDLIBS = -L./implementation1/

ARTIFACT = ./main
CXXSRC = ./main.cc

$(ARTIFACT): $(CXXSRC)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -o $(ARTIFACT) $(CXXSRC)

test: $(ARTIFACT)
	$(ARTIFACT)

clean:
	rm $(ARTIFACT)
