#ifndef THREADLIST
#define THREADLIST
#include <iostream>
#include <pthread.h>
#include <unordered_set>

struct ThreadNode{
	pthread_t tid;
	ThreadNode *pre, *next;
	ThreadNode();
	ThreadNode(pthread_t tid);
};
class ThreadList{
public:
	ThreadList();
	~ThreadList();
	ThreadNode* push_front(pthread_t element);
	ThreadNode* push_back(pthread_t element);
	void moveToFront(ThreadNode *iter);
	void erase(ThreadNode *iter);
	bool empty();
	int size();
	void print();
private:
	ThreadNode *m_threadList;
	ThreadNode *rear, *head;
	std::unordered_set<ThreadNode*> S;
	pthread_mutex_t m_mutex;
};

#endif