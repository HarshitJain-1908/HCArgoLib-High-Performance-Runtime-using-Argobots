include $(HCLIB_ROOT)/include/hclib.mak

TARGETS := fib fib-reducer prime

all: clean $(TARGETS) clean-obj

%: %.cpp
	$(CXX) -O3 $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^  $(PROJECT_LDLIBS) -g

clean-obj:
	rm -rf *.o

clean:
	rm -rf *.o $(TARGETS)
