#include "taskqueue.h"
template<typename T> 
Task<T>::Task(){
	function = nullptr;
	arg = nullptr;
}
template<typename T> 
Task<T>::Task(callback function, void *arg){
	this->function = function;
	this->arg = (T*)arg;
}

template<typename T> 
TaskQueue<T>::TaskQueue(){
	pthread_mutex_init(&this->m_mutex, nullptr);
}

template<typename T> 
TaskQueue<T>::~TaskQueue(){
	pthread_mutex_destroy(&this->m_mutex);
}

template<typename T> 
void TaskQueue<T>::addTask(Task<T> task){
	pthread_mutex_lock(&this->m_mutex);
	this->m_taskQu.push(task);
	pthread_mutex_unlock(&this->m_mutex);
}

template<typename T> 
void TaskQueue<T>::addTask(callback function, void *arg){
	pthread_mutex_lock(&this->m_mutex);
	this->m_taskQu.push({function, arg});
	pthread_mutex_unlock(&this->m_mutex);
}

template<typename T> 
Task<T> TaskQueue<T>::getTask(){
	Task<T> task;
	pthread_mutex_lock(&this->m_mutex);
	if(!this->m_taskQu.empty()){
		task = this->m_taskQu.front();
		this->m_taskQu.pop();
	}
	pthread_mutex_unlock(&this->m_mutex);
	return task;
}

template<typename T> 
bool TaskQueue<T>::empty(){
	return this->m_taskQu.empty();
}

template<typename T> 
int TaskQueue<T>::size(){
	return this->m_taskQu.size();
}