# 线程池

> [项目地址：Special-JiaHao/threadpool: 线程池 (github.com)](https://github.com/Special-JiaHao/threadpool/tree/main)

## 测试编译

```shell
> g++ taskqueue.cpp threadlist.cpp threadpool.cpp test.cpp -o test -lpthread
> ./test
```

## 核心参数

- 任务队列：存储待处理的任务`(函数地址+参数)`

  > 1. 任务抽象：函数地址+参数
  > 1. 任务队列：先进先出`queue`
  > 1. 互斥访问：互斥锁`mutex`
  > 1. 基本接口：添加任务`addTask`、弹出对头任务`getTask`、队列判空`empty`、获取待处理的任务数`size`

  ```cpp
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
  	std::queue<Task<T>> m_taskQu;	//  任务队列
  	pthread_mutex_t m_mutex;        //  互斥锁
  };
  ```

- 工作者线程

  > 1. 核心任务就是从任务队列中取出元素并执行
  > 2. 在任务列队空时，堵塞（等待唤醒信号）
  > 3. 被唤醒后检查唤醒原因，如果是销魂信号，则进行线程的退出

- 工作者线程链：管理当前创建的所以工作线程（用于任务处理的线程）

  > 1. 添加工作者线程`（线程ID）`
  > 2. 删除工作者线程`(删除指定线程ID的工作者线程)` 

  :::info

  较好的实现方式：双链表，如果没有特殊要求，其实也可以通过一个简单的哈希表来实现，用于存储当前存活的线程即可. 以下通过双链表来实现，可以更好管理线程信息，同时可以使得链表头部均为空闲线程，链表尾部为均为忙碌线程. 

  :::

  ```cpp
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
  ```

- 管理者线程

- 最大线程数

- 最小线程数

- 繁忙的线程数

- 存活的线程数

- 线程销魂信号`(计数)`

- 关闭标志

- 线程池锁（粒度问题）

- 线程空的条件变量

  ```cpp
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
  	std::unordered_map<pthread_t, ThreadNode*> mp;	/* 线程ID到节点地址 */
  	bool shutdown;		/* 线程池状态 */
  	static const int coefficient = 2;	/* 销毁与创建线程常数 */
  };
  ```

## 任务队列

- 任务抽象

  > 1. 函数地址使用`void(*)(void *)`类型来表示，并不影响函数的调用. 同时往往函数是常驻内存的，空间由操作系统来释放
  > 2. 参数使用指向对应类型的指针，往往任务执行完成后需要释放这部分内存空间

  ![](https://raw.githubusercontent.com/Special-JiaHao/images/main/任务抽象1.svg)

  ```cpp
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
  ```

- 队列抽象

  > 1. 构造（初始化互斥锁）与析构函数（销毁互斥锁）

  ```cpp
  template<typename T> 
  TaskQueue<T>::TaskQueue(){
  	pthread_mutex_init(&this->m_mutex, nullptr);
  }
  
  template<typename T> 
  TaskQueue<T>::~TaskQueue(){
  	pthread_mutex_destroy(&this->m_mutex);
  }
  ```

  ![](https://raw.githubusercontent.com/Special-JiaHao/images/main/任务队列.png)

  > 2. 往任务队列内添加新任务，从队尾插入新任务（互斥锁锁定）.

  ```cpp
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
  ```

  > 3. 当有新的空闲线程可用于处理任务，需要从对头中取出任务（互斥锁锁定）. 

  ```cpp
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
  /*
  	当队列内无任务时，会返回一个空函数. 
  */
  ```

  > 4. 获取任务队列内任务数目

  ```cpp
  template<typename T> 
  bool TaskQueue<T>::empty(){
  	return this->m_taskQu.empty();
  }
  template<typename T> 
  int TaskQueue<T>::size(){
  	return this->m_taskQu.size();
  }
  ```

## 工作者线程链

- 线程抽象

  ```cpp
  ThreadNode::ThreadNode(){
  	this->tid = 0;
  	this->pre = this->next = nullptr;
  }
  ThreadNode::ThreadNode(pthread_t tid){
  	this->tid = tid;
  	this->pre = this->next = nullptr;
  }
  ```

- 线程链表

  > 1. 链表的构造函数与析构函数

  ```cpp
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
  ```

  > 2. 往头部插入线程

  ```cpp
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
  ```

  > 3. 往尾部插入线程

  ```cpp
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
  ```

  > 4. 删除线程

  ```cpp
  void ThreadList::erase(ThreadNode *iter){
  	if(this->S.find(iter) == this->S.end())	return ;
  	ThreadNode* it1 = iter->pre, *it2 = iter->next;
  	it1->next = it2;
  	it2->pre = it1;
  	this->S.erase(iter);
  	delete iter;
  }
  ```

  > 5. 获取当前存活线程数目

  ```cpp
  bool ThreadList::empty(){
  	return this->S.size() == 0;
  }
  
  int ThreadList::size(){
  	return this->S.size();
  }
  ```

![](https://raw.githubusercontent.com/Special-JiaHao/images/main/工作线程链(双向链表).png)

## 工作者线程

![](https://raw.githubusercontent.com/Special-JiaHao/images/main/工作线程任务.svg "工作者线任务")

```cpp
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
```

## 管理者线程

![](https://raw.githubusercontent.com/Special-JiaHao/images/main/管理者线程任务.svg "管理者线程任务")

```cpp
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
```

## 线程池的构造与其他接口

- 线程池的构造函数

  1. 申请任务队列空间
  2. 申请工作线程链空间，创建一定数目的工作线程放入链表中
  3. 初始化最大线程数、最小线程数、繁忙线程数、存活线程数
  4. 初始化互斥锁和信号量

  ```cpp
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
  ```

- 线程池的析构函数

  ```cpp
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
  ```

- 往线程池中添加任务`（调用任务队列添加任务接口）`

  ```cpp
  /*
  	此处不用加锁是因为"所提供的"任务队列内部自动加锁
  */
  template<typename T> 
  void ThreadPool<T>::addTask(Task<T> task){
  	if(this->shutdown)	return ;
  	this->taskQu->addTask(task);
  	pthread_cond_signal(&this->notEmpty);
  }
  ```

- 线程退出

  ```cpp
  /*
  	通过线程ID来获取存储在工作者线程链中该线程的位置，再调用工作者线程链删除节点的接口，最后销毁该线程
  */
  template<typename T> 
  void ThreadPool<T>::threadExit(){
  	pthread_t tid = pthread_self();
  	pthread_mutex_lock(&this->threadPoolMutex);
  	this->workerIDs->erase(this->mp[tid]);
  	this->mp.erase(tid);
  	pthread_mutex_unlock(&this->threadPoolMutex);
  	pthread_exit(NULL);
  }
  ```

  
