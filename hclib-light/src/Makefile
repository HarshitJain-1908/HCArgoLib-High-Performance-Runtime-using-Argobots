INCLUDE=-I$(ARGOBOTS_INSTALL_DIR)include -I/home/vanshika/PRMPProject/hclib-light/hclib-install/include
LIBRARY=-L$(ARGOBOTS_INSTALL_DIR)lib -Wl,-rpath,$(ARGOBOTS_INSTALL_DIR)/lib -L/home/vanshika/PRMPProject/hclib-light/hclib-install/lib -Wl,-rpath,/home/vanshika/PRMPProject/hclib-light/hclib-install/lib
argobots = argobots-runtime my_runtime.so my_hello_world

all: clean $(argobots)

argobots-runtime:
	gcc $(INCLUDE) -L$(ARGOBOTS_INSTALL_DIR)lib -o argobots-runtime argobots-runtime.c -labt -lpthread -g

my_runtime.so: my_runtime.c my_runtime.h
	gcc $(INCLUDE) $(LIBRARY) -shared -o my_runtime.so -fPIC my_runtime.c -lhclib -labt -lpthread 

my_hello_world: my_hello_world.c
	gcc -o my_hello_world my_hello_world.c -lmy_runtime

clean:
	rm -rf *.o $(argobots)
