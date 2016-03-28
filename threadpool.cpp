#include <iostream>
#include <queue>
#include <pthread.h>
#include <unistd.h>

using namespace std;

enum Priority {
	P_LOW = 0,
	P_NORMAL,
	P_HIGH
};

class Task {
public:
	virtual void run( void ) = 0;
	virtual ~Task() {}
};

class ThreadPool;

struct ThreadArg {
	ThreadPool * tp;
	Task * task;
	ThreadArg() : tp(NULL), task(NULL) {}
	ThreadArg(ThreadPool *pool, Task * t ) : tp(pool), task(t) {}
};

class ThreadPool {
	enum State {
		PROCESS,
		STOPPING
	} state; // isStopping
	queue< Task * > queues[3];
	pthread_mutex_t task_mutex;
	pthread_cond_t task_cond;
	int num_of_workers;
	int max_workers;
	int high_in_process;
	vector< pthread_t > workers;

	static void run( void * arg ) {
		ThreadArg * targ = (ThreadArg *)arg;
		delete targ;
	}
	Task * chooseTask() {
		Task * task;

		if (queues[P_HIGH].empty() && queues[P_NORMAL].empty() && queues[P_LOW].empty())
			return NULL;
		if (high_in_process < 3 && !queues[P_HIGH].empty()) {
			task = queues[P_HIGH].front();
			queues[P_HIGH].pop();
			high_in_process++;
			return task;
		}
		if (!queues[P_NORMAL].empty()) {
			task = queues[P_NORMAL].front();
			queues[P_NORMAL].pop();
		}
		else {
			task = queues[P_LOW].front();
			queues[P_LOW].pop();
		}
		high_in_process = 0;
		return task;
	}
	static void * manage( void * arg ) {
		ThreadPool * self = (ThreadPool *)arg;
		Task * task;
		while (true) {
			pthread_mutex_lock(&(self->task_mutex));
			while (self->state != STOPPING && (task = self->chooseTask()) == NULL)
				pthread_cond_wait(&(self->task_cond), &(self->task_mutex));
			if (self->state == STOPPING) {
				pthread_mutex_unlock(&(self->task_mutex));
				pthread_exit(NULL);
			}
			pthread_mutex_unlock(&(self->task_mutex));
			task->run();
			delete task;
		}
		return NULL;
	}
public:
	ThreadPool( int n ) : state(PROCESS), num_of_workers(0), max_workers(n), high_in_process(0) {
		pthread_t t;
		pthread_mutex_init(&task_mutex, NULL);
		pthread_cond_init(&task_cond, NULL);
		for (int i = 0; i < max_workers; i++) {
			pthread_create(&t, NULL, manage, this);
			workers.push_back(t);
		}
	}
	bool Enqueue( Task *t, Priority priority ) {
		if (state == STOPPING)
			return false;
		pthread_mutex_lock(&task_mutex);
		queues[priority].push(t);
		pthread_cond_signal(&task_cond);
		pthread_mutex_unlock(&task_mutex);
		return true;
	}
	void Stop() {
        if (state == STOPPING)
            return;
		state = STOPPING;
		pthread_mutex_lock(&task_mutex);
        pthread_cond_broadcast(&task_cond);
		pthread_mutex_unlock(&task_mutex);
		for (int i = 0; i < max_workers; i++) {
			pthread_join(workers[i], NULL);
		}
		workers.clear();
	}
	~ThreadPool() {
		Stop();
		pthread_mutex_destroy(&task_mutex);
		pthread_cond_destroy(&task_cond);
	}
};

class High : public Task {
public:
    virtual void run( void ) {
        sleep(1);
        cout << "High task" << endl;
    }
};

class Normal : public Task {
public:
    virtual void run( void ) {
        sleep(1);
        cout << "Normal task" << endl;
    }
};

class Low : public Task {
public:
    virtual void run( void ) {
        sleep(1);
        cout << "Low task" << endl;
    }
};

int main( int argc, char ** argv ) {
    ThreadPool tp(14);

    for (int i = 0; i < 5; i++) {
        tp.Enqueue(new High(), P_HIGH);
        tp.Enqueue(new Normal(), P_NORMAL);
        tp.Enqueue(new Low(), P_LOW);
    }
    tp.Enqueue(new High(), P_HIGH);
    tp.Enqueue(new Normal(), P_NORMAL);
    tp.Enqueue(new Normal(), P_NORMAL);
    tp.Enqueue(new Low(), P_LOW);
    sleep(2);
    tp.Stop();
	return 0;
}
