#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

/*

gcc -o test_thread test_thread.c -lpthread

*/


pthread_mutex_t mutexDB = PTHREAD_MUTEX_INITIALIZER;
int counter=0;

void touchFile(int groupID, int sensorID) {
        
        FILE *f = NULL;
        char *filename, *dirname;
        if(NULL == (filename = (char *) malloc(sizeof(char *)))){
                exit(1);
        }
        if(NULL == (dirname = (char *) malloc(sizeof(char *)))){
                exit(1);
        }
        sprintf(dirname, "%i", groupID);
        sprintf(filename, "%i/%i.txt", groupID, sensorID);
        
        mkdir(dirname); //create dir if does not exist
        f = fopen(filename, "w"); // update File
        fclose(f);
        
        free(filename);
        free(dirname);
}

void * printStuff(void *arg){
        
        //First acquire lock
        pthread_mutex_lock( &mutexDB );
                
                struct timespec tim, tim2;
                tim.tv_sec = 0;
                tim.tv_nsec = 500000;
                
        int i = 0;
        while (i < 1) {
            printf("%i - This is %i another thread: %s\n", i++, counter++, (char *)arg);
            nanosleep(&tim, &tim2);
        }
        //Finally, release lock
        pthread_mutex_unlock( &mutexDB );

        pthread_exit(NULL);

}
int main(int argc, char *argv[]){

        pthread_t tid1, tid2;
        int err;
        char buff[40] = "Hello World!";
        char str[40];
        char * ptrStr;
        ptrStr = (char *) &buff;
        //char * str;
        memset(str,'\0', sizeof(str));
        //str = "Hello Wooooorld";
        strcpy(str, "Blabla");
        //strncpy(buff, ptrStr+4,8);
        
        strncpy(str, ((char *) &buff) + 4, 8);
        str[8] = '\0';
        printf("%s\n", str);
        
        
        printf("%c\n", str[0]);
        //str = buff;
        
        err = pthread_create(&tid1, NULL, printStuff, "Thread 1");
        if(err != 0){
                printf("Could not create thread: %i\n", err);
                return 1;
        }
        err = pthread_create(&tid2, NULL, printStuff, (void *) "Thread 2");
        if(err != 0){
                printf("Could not create thread: %i\n", err);
                return 1;
        }
        printf("Thread created\n");
        
        pthread_join(tid1, NULL);
                pthread_join(tid2, NULL);
        printf("Thread joined\n");
        
        touchFile(2,1);
        
        return 0;
        
}
