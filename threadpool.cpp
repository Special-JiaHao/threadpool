#include <iostream>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include "threadpool.h"
#define DEBUG 1
template<typename T> 
ThreadPool<T>::ThreadPool(int minThreadCount, int maxThreadCount){
	do{
		this->taskQu = new TaskQueue<T>;
		if(this->taskQu == nullptr){
			std::cout << "任务队列内存申请失败..." << std::endl;
			break;
		}	
		this->minThreadCount = minThreadCount;
		this->maxThreadCount = maxThreadCount;
		this->busyThreadCount = 0;
		this->destoryThreadCount = 0;
		this->shutdown = false;
		this->workerIDs = new ThreadList;
		if(this->workerIDs == nullptr){
			std::cout << "工作线程内存申请失败..." << std::endl;
			break;
		}
		if(pthread_mutex_init(&this->threadPoolMutex, nullptr) != 0 || pthread_cond_init(&this->notEmpty, nullptr) != 0){
			std::cout << "条件变量或互斥锁初始化失败..." << std::endl;
			break;
		}
		pthread_create(&this->managerID, nullptr, manager, this);
		for(int i = 0; i < this->minThreadCount; i ++ ){
			pthread_t tid;
			pthread_create(&tid, nullptr, worker, this);
			ThreadNode *cur =  this->workerIDs->push_front(tid);
			this->mp[tid] = cur;
		}
		this->liveThreadCount = this->minThreadCount;
		return ;
	}while(false);
	if(this->workerIDs)	delete[] this->workerIDs;
	if(this->taskQu)	delete taskQu;
}


template<typename T> 
void* ThreadPool<T>::worker(void *arg){
	ThreadPool* threadPool = static_cast<ThreadPool*>(arg);
	while(true){
		pthread_mutex_lock(&threadPool->threadPoolMutex);
		while(threadPool->taskQu->empty() && !threadPool->shutdown){
			pthread_cond_wait(&threadPool->notEmpty, &threadPool->threadPoolMutex);
			if(threadPool->destoryThreadCount > 0){
				threadPool->destoryThreadCount -- ;
				if(threadPool->liveThreadCount > threadPool->minThreadCount){
					threadPool->liveThreadCount -- ;
					pthread_mutex_unlock(&threadPool->threadPoolMutex);
					threadPool->threadExit();
				}
			}
		}

		if(threadPool->shutdown){
			pthread_mutex_unlock(&threadPool->threadPoolMutex);
			threadPool->threadExit();
		}

		Task<T> task = threadPool->taskQu->getTask();
		threadPool->busyThreadCount ++ ;
		pthread_mutex_unlock(&threadPool->threadPoolMutex);
		if(DEBUG)	std::cout << "Thread :" << pthread_self() << " start working..." << std::endl;
		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;

		if(DEBUG)	std::cout << "Thread :" << pthread_self() << " end working..." << std::endl;
		pthread_mutex_lock(&threadPool->threadPoolMutex);
		threadPool->workerIDs->moveToFront(threadPool->mp[pthread_self()]);
		threadPool->busyThreadCount -- ;
		pthread_mutex_unlock(&threadPool->threadPoolMutex);
	}	
	return nullptr;
}

template<typename T> 
void* ThreadPool<T>::manager(void *arg){
	ThreadPool* threadPool = static_cast<ThreadPool*>(arg);
	while(true){
		pthread_mutex_lock(&threadPool->threadPoolMutex);
		if(threadPool->shutdown){
			pthread_mutex_unlock(&threadPool->threadPoolMutex);
			break;
		}
		pthread_mutex_unlock(&threadPool->threadPoolMutex);
		sleep(2);
		if(threadPool->taskQu->size() >= threadPool->liveThreadCount * 0.8 && threadPool->liveThreadCount < threadPool->maxThreadCount){
			pthread_mutex_lock(&threadPool->threadPoolMutex);
			int cnt = 0;
			while(cnt < threadPool->coefficient && threadPool->workerIDs->size() < threadPool->maxThreadCount){
				cnt ++ ;
				pthread_t tid;
				pthread_create(&tid, nullptr, worker, threadPool);
				ThreadNode*cur =  threadPool->workerIDs->push_front(tid);
				threadPool->mp[tid] = cur;
				threadPool->liveThreadCount ++ ;
			}
			pthread_mutex_unlock(&threadPool->threadPoolMutex);
		}
		if(threadPool->busyThreadCount * 2 <= threadPool->liveThreadCount && threadPool->liveThreadCount > threadPool->minThreadCount){
			pthread_mutex_lock(&threadPool->threadPoolMutex);
			threadPool->destoryThreadCount = threadPool->coefficient;
			pthread_mutex_unlock(&threadPool->threadPoolMutex);
			for(int i = 0; i < threadPool->destoryThreadCount; i ++ )	pthread_cond_signal(&threadPool->notEmpty);
		}
		if(DEBUG){
			std::cout << "Busy threadCOunt : " << threadPool->busyThreadCount << std::endl;
			std::cout << "live threadCOunt : " << threadPool->liveThreadCount << std::endl;
			std::cout << "Queue size : " << threadPool->taskQu->size() << std::endl;
		}
	}
	return nullptr;

}

template<typename T> 
void ThreadPool<T>::threadExit(){
	pthread_t tid = pthread_self();
	pthread_mutex_lock(&this->threadPoolMutex);
	this->workerIDs->erase(this->mp[tid]);
	this->mp.erase(tid);
	pthread_mutex_unlock(&this->threadPoolMutex);
	pthread_exit(NULL);
}

template<typename T> 
void ThreadPool<T>::addTask(Task<T> task){
	if(this->shutdown)	return ;
	this->taskQu->addTask(task);
	pthread_cond_signal(&this->notEmpty);
}

template<typename T> 
ThreadPool<T>::~ThreadPool(){
	this->shutdown = true;
	pthread_join(this->managerID, nullptr);
	for(int i = 0; i < this->liveThreadCount; i ++ )	pthread_cond_signal(&this->notEmpty);
	if(this->taskQu)	delete this->taskQu;
	if(this->workerIDs)	delete this->workerIDs;
	// if(this->workerIDs)	delete[] this->workerIDs;
	pthread_mutex_destroy(&this->threadPoolMutex);
	pthread_cond_destroy(&this->notEmpty);
}

