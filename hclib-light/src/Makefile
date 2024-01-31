argobots = argobots-runtime my_runtime

all: clean $(argobots)

argobots-runtime:
	gcc -I$(ARGOBOTS_INSTALL_DIR)/include/ -L$(ARGOBOTS_INSTALL_DIR)/lib -o argobots-runtime argobots-runtime.c -labt -lpthread -g

my_runtime:
	gcc -I$(ARGOBOTS_INSTALL_DIR)/include/ -L$(ARGOBOTS_INSTALL_DIR)/lib -o my_runtime my_runtime.c -labt -lpthread -g

clean:
	rm -rf *.o $(argobots)
