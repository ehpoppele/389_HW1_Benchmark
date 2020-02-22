#include <math.h>
#include <ctime>
#include <iostream>
#include <random>
#include <chrono>

//This program should time the reading of a byte from different layers of memory, by using arrays of increasing sizes
//and iterating in a manner so as to foil the prefetcher and force it to prefetch the whole array if possible.
//It accomplishes this largely through the use of random indices and permutations;
//The whole scheme, as well as my process and reasoning behind it, are described in more depth in the readme file.

int div_num = 256; //size of each subarray-- experimentation currently suggests that 256 is the ideal value here
//256 is also the max that can be used, as uint8_t only goes up to 255, and I use that to iterate through my subarrays later

int iters = 50; //Number of iterations run per buffer size; the minimum time from each iteration set is then printed

double time_access(uint8_t ** & buffer, int size){

    //Initial variables; c will used to read from memory
    double elapsed = 0;
    volatile int c = 0;
    int sub_count = size/div_num;

    //Main loop; the choice of div_num (256) iterations is fairly arbitrary, but is nice for when the inner loop uses sub_count iterations
    //As that choice then results in total iterations equal to the size of the array
    for(int i = 0; i < div_num; i++){
        std::clock_t start;
        std::clock_t end;
        
        //Now we create the permutation of div_num
        int* order = new int[sub_count];
        for(int k = 0; k < sub_count; k++){
            order[k] = k;
        }
        int* permutation = new int[sub_count];
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 gen_perm(seed);
        for(int k = 0; k < sub_count; k++){
            std::uniform_int_distribution<> rng_perm(0, sub_count-k-1);
            int x = rng_perm(gen_perm);
            permutation[k] = order[x];
            int temp = order[x];
            order[x] = order[sub_count-k-1];
            order[sub_count-k-1] = temp; //unnecessary but illustrates the point
        }
        
        //Actual timing loop here; timing as little as possible
        start = std::clock();
        for(int j = 0; j < 1024; j++){//try back to sub count
            c = buffer[permutation[j%sub_count]][c];
        }
        end = std::clock();
        
        //Record and delete
        double seconds = (end - start ) / (double) CLOCKS_PER_SEC;
        seconds = (1000000000.0 * seconds);
        elapsed += seconds;
        delete order;
        delete permutation;
    }

    elapsed = elapsed/(div_num * 1024);
    return elapsed;
}

int main(){
    for(int i = 0; i < 17; i++){
        
        //Find size of current buffer, and exponent of buffer (such that 2^exp = size)
        int size = pow(2, 10 + i);
        
        //setup RNG to fill the array
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 gen(seed);
        std::uniform_int_distribution<> rng(0, div_num-1);
        
        //set up array with random bits
        uint8_t **buffer = new uint8_t*[size/div_num];
        for(int i = 0; i < size/div_num; i++){
            buffer[i] = new uint8_t[div_num];
            for(int j = 0; j < div_num; j++){
                buffer[i][j] = rng(gen);
            }
        }

        //call timing function and print results; 10 attempts while searching for minimum (least noise)
        double min = 999;//Need something large for initial min; this also throws out outliers
        for(int i = 0; i < iters; i++){
            double elapsed = time_access(buffer, size);
            if(elapsed < min){
                min = elapsed;
            }
        }
        std::cout << std::to_string(size) << "    " << std::to_string(min) << std::endl;
        //clear memory
        for(int i = 0; i < size/div_num; i++)delete buffer[i];
        delete[] buffer;
    }
    return 0;
}
