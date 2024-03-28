#include "threadlist.h"

ThreadNode::ThreadNode(){
	this->tid = 0;
	this->pre = this->next = nullptr;
}

ThreadNode::ThreadNode(pthread_t tid){
	this->tid = tid;
	this->pre = this->next = nullptr;
}


ThreadList::ThreadList(){
	this->m_threadList = new ThreadNode(-1);
	this->head = this->m_threadList;
	this->rear = new ThreadNode(-1);
	this->head->next = this->rear;
	this->rear->pre = this->head;
	pthread_mutex_init(&this->m_mutex, nullptr);
}

ThreadList::~ThreadList(){
	ThreadNode* cur = this->head;
	while(cur){
		ThreadNode* tp = cur->next;
		std::cout << "delete " << cur->tid << std::endl;
		delete cur;
		cur = tp;
	}
	pthread_mutex_destroy(&this->m_mutex);
}


ThreadNode* ThreadList::push_front(pthread_t element){
	pthread_mutex_lock(&this->m_mutex);
	ThreadNode* cur = new ThreadNode(element);
	ThreadNode* tmp = this->head->next;
	this->head->next = cur;
	tmp->pre = cur;
	cur->pre = this->head;
	cur->next = tmp;
	this->S.insert(cur);
	pthread_mutex_unlock(&this->m_mutex);
	return cur;
}

ThreadNode* ThreadList::push_back(pthread_t element){
	pthread_mutex_lock(&this->m_mutex);
	ThreadNode* cur = new ThreadNode(element);
	ThreadNode* tmp = this->rear->pre;
	tmp->next = cur;
	this->rear->pre = cur;
	cur->pre = tmp;
	cur->next = this->rear;
	this->S.insert(cur);
	pthread_mutex_unlock(&this->m_mutex);
	return cur;
}

void ThreadList::erase(ThreadNode *iter){
	if(this->S.find(iter) == this->S.end())	return ;
	ThreadNode* it1 = iter->pre, *it2 = iter->next;
	it1->next = it2;
	it2->pre = it1;
	this->S.erase(iter);
	delete iter;
}

void ThreadList::moveToFront(ThreadNode *iter){
	if(this->S.find(iter) == this->S.end())	return ;
	ThreadNode* it1 = iter->pre, *it2 = iter->next;
	it1->next = it2;
	it2->pre = it1;
	
	ThreadNode* tmp = this->head->next;
	this->head->next = iter;
	tmp->pre = iter;
	iter->pre = this->head;
	iter->next = tmp;
}	


void ThreadList::print(){
	ThreadNode* cur = this->head;
	cur = cur->next;
	while(cur != this->rear){
		std::cout << cur->tid << ' ';
		cur = cur->next;
	}
	std::cout << std::endl;
}

bool ThreadList::empty(){
	return this->S.size() == 0;
}

int ThreadList::size(){
	return this->S.size();
}



// int ThreadList::size(){
// 	int length = 0;
// 	pthread_mutex_lock(&this->m_mutex);
// 	length = this->m_threadList.size();
// 	pthread_mutex_unlock(&this->m_mutex);
// 	return length;
// }