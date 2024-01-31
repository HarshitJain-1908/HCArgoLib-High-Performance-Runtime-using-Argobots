#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "my_runtime.h"

int fib(int  n) {

    if (n < 2) return n;

    // HCArgoLib_start_finish() ;

    HCArgoLib_finish([&]() {
        int x = HCArgoLib_async();
        int y = fib(n-2);
    });
    
    return x + y;

}
int main(int argc, char *argv[]) {

    HCArgoLib_init(argc, argv);

    int res = fib(10);

    HCArgoLib_finalize();

    return 0;
}