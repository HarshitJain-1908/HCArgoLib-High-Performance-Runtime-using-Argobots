include $(HCLIB_ROOT)/include/hclib.mak

TARGETS := fib fib-reducer prime

all: clean $(TARGETS) clean-obj

%: %.cpp
	$(CXX) -O3 $(PROJECT_CXXFLAGS) $(PROJECT_LDFLAGS) -o $@ $^  $(PROJECT_LDLIBS) -g

clean-obj:
	rm -rf *.o

clean:
	rm -rf *.o $(TARGETS)

run:
	HCLIB_WORKERS=1 ./fib
	HCLIB_WORKERS=2 ./fib
	HCLIB_WORKERS=4 ./fib
	HCLIB_WORKERS=6 ./fib
	HCLIB_WORKERS=8 ./fib

	HCLIB_WORKERS=1 ./fib-reducer
	HCLIB_WORKERS=2 ./fib-reducer
	HCLIB_WORKERS=4 ./fib-reducer
	HCLIB_WORKERS=6 ./fib-reducer
	HCLIB_WORKERS=8 ./fib-reducer
