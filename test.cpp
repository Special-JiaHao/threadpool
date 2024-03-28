#include <unistd.h>
#include <iostream>
#include "threadpool.h"
#include "threadpool.cpp"
#define DEBUG 1
void taskFun(void *arg){
	if(DEBUG)	std::cout << "Task" << (*(int*)arg) << " 运行在Thread:" << pthread_self() << std::endl;
	sleep(1);
}
int main(){
	ThreadPool<int> threadPool(2, 10);
	for(int i = 0; i < 100; i ++ ){
		int* arg = new int(i);
		threadPool.addTask({taskFun, arg});
		if(i == 50)	sleep(8);
		else if(i < 50)	usleep(200 * 1000);
		else if(i > 50){
			usleep(50 * 1000);
		}
	}
	sleep(200);
	return 0;
}