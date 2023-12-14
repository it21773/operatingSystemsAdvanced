#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

char* remove_first_char(char* string,int len) {
  return (string+len)-1;
}

int main () {
   char sample[] = "this is a test";
//    char* result = remove_first_char(sample, sizeof("this"));
    char* result = sample+sizeof("this")-1;

   printf("%s",result);

}

// char* cuter(char* string, int begin, int end,char* result);

// int main () {
//    char sample[] = "this is a test";
// //    char result[];
//     // for (int i=0;i<strlen(sample);i++) {
//     //     printf("%c\n",sample[i]);
//     // }
//    char* result = cuter(sample,3,5,result);
//    printf("%s",result);
// }

// char* cuter(char* string, int begin, int end,char* result){
//     char result[end-begin];
//     int c = 0;

//     for (int i=begin;i<end;i++){
//         result[c]=string[i];
//         c++;
//     }
//     return result;
// }

