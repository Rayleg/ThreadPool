#include <iostream>
#include <queue>
#include <pthread.h>

using namespace std;

enum Priority {
	P_LOW = 0,
	P_NORMAL,
	P_HIGH
};

class Task {
public:
	virtual void do() = 0;
};

struct ThreadArg {
	ThreadPool * tp;
	Task * task;
	ThreadArg() : tp(NULL), func(NULL) {}
	ThreadArg(ThreadPool *pool, Task * t ) : tp(pool), task(t) {}
};

class ThreadPool {
	enum State {
		PROCESS,
		STOPPING
	} state; // isStopping
	queue< Task * > queue[3];
	pthread_mutex_t task_mutex;
	pthread_cond_t task_cond;
	int high_in_process;
	int num_of_workers;
	int max_workers;
	pthread_t manage_thread;
	vector< pthread_t > workers;

	static void run( void * arg ) {
		ThreadArg * targ = (ThreadArg *)arg;
		delete targ;
	}
	Task * chooseTask() {
		Task * task;

		if (queue[P_HIGH].empty() && queue[P_NORMAL].empty() && queue[P_LOW].empty())
			return NULL;
		if (high_in_process < 3 && !queue[P_HIGH].empty()) {
			task = queue[P_HIGH].front();
			queue[P_HIGH].pop();
			high_in_process++;
			return task;
		}
		if (!queue[P_NORMAL].empty()) {
			task = queue[P_NORMAL].front();
			queue[P_NORMAL].pop();
		}
		else {
			task = queue[P_LOW].front();
			queue[P_LOW].pop();
		}
		high_in_process = 0;
		return task;
	}
	static void manage( void * arg) {
		ThreadPool * self = (ThreadPool *)arg;
		Task * task;
		while (1) {
			pthread_mutex_lock(self->task_mutex);
			while (self->state != STOPPING && (task = chooseTask()) == NULL)
				pthread_cond_wait(self->task_cond, self->task_mutex);
			if (self->state != STOPPING) {
				pthread_mutex_unlock(self->mutex);
				pthread_exit(NULL);
			}
			pthread_mutex_unlock(self->mutex);
			task->do();
			delete task;
		}
	}
public:
	ThreadPool( int n ) : max_workers(n), state(PROCESS), num_of_workers(0) {
		pthread_t t;
		cout << "Initialize TreadPool with " << max_workers << " threads" << endl;
		for (int i = 0; i < max_workers; i++) {
			pthread_create(&t, NULL, manage, self);
			workers.push_back(t);
		}
	}
	bool Enqueue( Task *t, Priority priority ) {
		if (state == STOPPING)
			return false;
		pthread_mutex_lock(task_mutex);
		queue[priority].push(t);
		pthread_cond_signal(task_cond);
		pthread_mutex_unlock(task_mutex);
		return true;
	}
	void Stop() {
		state = STOPPING;
		for (int i = 0; i < max_workers; i++) {
			pthread_join(&workers[i], NULL);
			pthread_destroy(&workers[i], NULL, manage, self);
		}
		workers.clear();
	}
	~ThreadPool() {
		Stop();
	}
};

int main( int argc, char ** argv ) {
	return 0;
}