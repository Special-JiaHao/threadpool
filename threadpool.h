#ifndef THREADPOOL
#define THREADPOOL
#include <unordered_map>
#include <list>
#include "taskqueue.h"
#include "taskqueue.cpp"
#include "threadlist.h"
template<typename T> 
class ThreadPool{
public:
	ThreadPool(int minThreadCount, int maxThreadCount);
	~ThreadPool();
	void addTask(Task<T> task);
private:
	static void* worker(void *arg);
	static void* manager(void *arg);
	void threadExit();
private:
	TaskQueue<T>* taskQu;	/* 任务队列 */
	pthread_t managerID;	/* 管理者线程ID */
	ThreadList *workerIDs;	/* 工作线程IDs */
	int minThreadCount;		/* 最小线程数 */
	int maxThreadCount;		/* 最大线程数 */

	int busyThreadCount;	/* 忙线程数 */
	int liveThreadCount;	/* 存活线程数 */

	int destoryThreadCount;	/* 需要销毁的线程数 */
	pthread_mutex_t threadPoolMutex;	/* 线程池锁 */
	pthread_cond_t notEmpty;	/* 任务队列判空条件变量 */
	std::unordered_map<pthread_t, ThreadNode*> mp;
	bool shutdown;
	static const int coefficient = 2;
};

#endif