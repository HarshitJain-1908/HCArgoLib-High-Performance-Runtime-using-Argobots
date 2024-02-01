#include<iostream>
#include "hclib.hpp"
#include <chrono>
#include <thread>

using namespace std;

int main(int argc, char *argv[])
{
    hclib::init(argc, argv);
    hclib::finish([](){
        for(int i=0; i<100; i++){
            hclib::async([](){cout<<"Lambda function called\n";});
            // std::this_thread::sleep_for(std::chrono::milliseconds(3));
         }
    });
    hclib::finalize();

    return 0;
}