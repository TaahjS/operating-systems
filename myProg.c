#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //Header file for sleep(). man 3 sleep for details.
 
 
// A normal C function that is executed as a thread
// when its name is specified in pthread_create()
void* myThreadFun(void* threadNum)
{
    int num = (int *) threadNum; //threadNum is i from main
    printf("Printing Thread Num: %d \n", threadNum);
    sleep(1);
    return NULL;
}
 
 //each thread needs to be printing their number in a loop.
 //each thread should never print out their number before the number before them is done.
 //could put entire main in loop. thread 1 executes, thread 2 executes, exit, then restart.
int main()
{
    pthread_t thread_id;
    int i = 0; //thread number (not id). needs to be put in a for-loop to create multiple threads
    int N; //number of threads
    pthread_create(&thread_id, NULL, myThreadFun, &i); 
    pthread_join(thread_id, NULL); //waits for other threads to complete before executing
    exit(0);
}