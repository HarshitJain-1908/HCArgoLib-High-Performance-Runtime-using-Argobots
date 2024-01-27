argobots = argobots-runtime
all: $(argobots)
argobots-runtime:
	gcc -I$(ARGOBOTS_INSTALL_DIR) -o argobots-runtime argobots-runtime.c
clean:
	rm -rf *.o $(argobots)
