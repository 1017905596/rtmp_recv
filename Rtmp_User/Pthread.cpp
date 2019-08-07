#include "Pthread.h"

CPthread::CPthread(){

}

CPthread::~CPthread(){

}


int CPthread::Create(LPTHREAD_START_ROUTINE thr_func, LPVOID arg_list){
#ifdef WIN
	CreateThread( NULL, 0, thr_func, arg_list, 0, &thr_id );
#else
    pthread_create( &thr_id, NULL, thr_func, arg_list );
#endif
	return 0;
}