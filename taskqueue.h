#ifndef TASKQUEUE
#define TASKQUEUE
#include <queue>
#include <pthread.h>
using callback = void (*)(void *arg);
//	任务
template<typename T> 
class Task{
public:
	callback function;
	T *arg;
	Task();
	Task(callback function, void *arg);
};

// 任务队列
template<typename T> 
class TaskQueue{
public:
	TaskQueue();
	~TaskQueue();
	void addTask(Task<T> task);
	void addTask(callback function, void *arg);
	Task<T> getTask();
	bool empty();
	int size();
private:
	std::queue<Task<T>> m_taskQu;
	pthread_mutex_t m_mutex;
};
#endif