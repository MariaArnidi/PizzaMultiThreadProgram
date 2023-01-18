#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


#define NTEL 3
#define NCOOK 2
#define NOVEN 10
#define NDELIVERER 7
#define TORDERLOW 1
#define TORDERHIGH 5
#define NORDERLOW 1
#define NORDERHIGH 5
#define TPAYMENTLOW 1
#define TPAYMENTHIGH 2
#define CPIZZA 10
#define PFAIL 0.05
#define TPREP 1
#define TBAKE 10
#define TPACK 2
#define TDELLOW 5
#define TDELHIGH 15 

int currenttimeseconds();
int randomint(int min,int max);
void output(char* out);
void* packeterrun(void* arg);
void* ovenrun(void* arg);
void* delivererrun(void* arg);
void* cookrun(void* arg);
void* orderrun(void* ordno);


        