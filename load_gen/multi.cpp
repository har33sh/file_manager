#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

using namespace std;

#define NUM_THREADS 5

void *PrintHello(void *threadid) {
   long tid;
   tid = (long)threadid;
   int random_number = rand();
   cout << "Starting Thread ID, " << tid <<" "<<random_number<< endl;
   pthread_exit(NULL);
}

int main () {
   pthread_t threads[NUM_THREADS];
   int rc;
   int i;
   srand(time(NULL));
   for( i = 0; i < NUM_THREADS; i++ ) {
      cout << "Creating thread, " << i << endl;
      rc = pthread_create(&threads[i], NULL, PrintHello, (void *)i);

      if (rc) {
         cout << "Error:unable to create thread," << rc << endl;
         exit(-1);
      }
   }
   pthread_exit(NULL);
}
