argobots = argobots-runtime
all: clean $(argobots)
argobots-runtime:
	gcc -I$(ARGOBOTS_INSTALL_DIR)/argobots-install/include/ -L$(ARGOBOTS_INSTALL_DIR)/argobots-install/lib/ -labt -o argobots-runtime argobots-runtime.c
clean:
	rm -rf *.o $(argobots)
