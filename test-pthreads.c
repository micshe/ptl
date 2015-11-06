/* cc test_pthreads.c ptl.c -o test_pthreads -pthread */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<pthread.h>	/* link with -lpthreads */

#include<unistd.h>

#include"ptl.h"

void* thread1(void*args)
{
	int err;

	struct ptl_lock*lock;
	lock = args;

	printf("thread1: debug: thread1 has begun\n");

	printf("thread1: test: thread1 trying lock\n");
	err = ptl_trylock(lock);
	if(err==0)
	{
		printf("thread1: fail: grabbed lock\n");
		exit(EXIT_FAILURE);
	}
	printf("thread1: pass: failed to grab lock\n");

	pthread_exit(0);
}

void* thread2(void*args)
{
	int err;

	struct ptl_lock*lock;
	lock = args;

	printf("thread2: debug: thread2 has begun\n");

	printf("thread2: test: thread2 trying lock\n");
	err = ptl_trylock(lock);
	if(err==-1)
	{
		printf("thread2: fail: failed to grab lock\n");
		exit(EXIT_FAILURE);
	}
	printf("thread2: pass: grabbed lock\n");

	printf("thread2: test: releasing lock\n");
	err = ptl_unlock(lock);
	if(err==-1)
	{
		printf("thread2: fail: failed to release lock\n");
		exit(EXIT_FAILURE);
	}
	printf("thread2: pass: releasing lock\n");

	pthread_exit(0); 
}

void* thread3(void*args)
{
	int err;

	struct ptl_lock*lock;
	lock = args;

	printf("thread3: debug: thread3 has begun\n");

	printf("thread3: test: thread3 taking lock\n");
	err = ptl_lock(lock);
	if(err==-1)
	{
		printf("thread3: fail: failed to grab lock\n");
		exit(EXIT_FAILURE);
	}
	printf("thread3: pass: grabbed lock\n");

	printf("thread3: test: releasing lock\n");
	err = ptl_unlock(lock);
	if(err==-1)
	{
		printf("thread3: fail: failed to release lock\n");
		exit(EXIT_FAILURE);
	}
	printf("thread3: pass: releasing lock\n");

	pthread_exit(0); 
}

void test_dynamic(void)
{
	int err;

	struct ptl_lock lock;
	printf("thread0: test: initialising a lock\n");
	err = ptl_init(&lock);
	if(err==-1)
	{
		perror("thread0: ptl_init");
		exit(1);
	}
	printf("thread0: pass: initialising a lock\n");

	printf("thread0: test: taking a lock\n");
	err = ptl_lock(&lock);
	if(err==-1)
	{
		perror("thread0: ptl_lock");
		exit(1);
	}
	printf("thread0: pass: taking a lock\n");

	pthread_t tid;
	pthread_create(&tid,NULL,&thread1,&lock); 
	pthread_join(tid,NULL);

	printf("thread0: test: releasing a lock\n");
	err = ptl_unlock(&lock);
	if(err==-1)
	{
		perror("thread0: ptl_unlock");
		exit(1);
	}
	printf("thread0: pass: releasing a lock\n");

	pthread_create(&tid,NULL,&thread2,&lock); 
	pthread_join(tid,NULL);

	printf("thread0: test: taking lock\n");
	err = ptl_lock(&lock);
	if(err==-1)
	{
		perror("thread0: fail: ptl_lock");
		exit(1);
	}
	printf("thread0: pass: taking lock\n");

	pthread_create(&tid,NULL,&thread3,&lock);

	printf("thread0: debug: releasing lock in 5 seconds\n");
	usleep(5*1000*1000);

	printf("thread0: test: releasing lock\n");
	ptl_unlock(&lock); 
	printf("thread0: pass: releasing lock\n");

	pthread_join(tid,NULL);

	return;
}

void test_static(void)
{
	int err; 
	struct ptl_lock lock = PTL_INIT;

#if 0
	printf("thread0: test: initialising a lock\n");
	err = ptl_init(&lock);
	if(err==-1)
	{
		perror("thread0: ptl_init");
		exit(1);
	}
	printf("thread0: pass: initialising a lock\n");
#endif

	printf("thread0: test: taking a lock\n");
	err = ptl_lock(&lock);
	if(err==-1)
	{
		perror("thread0: ptl_lock");
		exit(1);
	}
	printf("thread0: pass: taking a lock\n");

	pthread_t tid;
	pthread_create(&tid,NULL,&thread1,&lock); 
	pthread_join(tid,NULL);

	printf("thread0: test: releasing a lock\n");
	err = ptl_unlock(&lock);
	if(err==-1)
	{
		perror("thread0: ptl_unlock");
		exit(1);
	}
	printf("thread0: pass: releasing a lock\n");

	pthread_create(&tid,NULL,&thread2,&lock); 
	pthread_join(tid,NULL);

	printf("thread0: test: taking lock\n");
	err = ptl_lock(&lock);
	if(err==-1)
	{
		perror("thread0: fail: ptl_lock");
		exit(1);
	}
	printf("thread0: pass: taking lock\n");

	pthread_create(&tid,NULL,&thread3,&lock);

	printf("thread0: debug: releasing lock in 5 seconds\n");
	usleep(5*1000*1000);

	printf("thread0: test: releasing lock\n");
	ptl_unlock(&lock); 
	printf("thread0: pass: releasing lock\n");

	pthread_join(tid,NULL);

	ptl_destroy(&lock);

	return; 
}

void*thread4(void*args)
{
	struct ptl_lock*lock;
	lock = args;

	usleep(5*1000*1000);

	printf("thread4-%d: test: race for lock\n",(int)pthread_self());
	ptl_lock(lock);
	printf("thread4-%d: pass: race for lock\n",(int)pthread_self()); 

	printf("thread4-%d: debug: release lock\n",(int)pthread_self());
	ptl_unlock(lock);

	pthread_exit(0);
}

void test_race(void)
{
	int err;

	struct ptl_lock lock = PTL_INIT; 
	pthread_t set[16];

	printf("thread0: test: racing threads for lock\n");

	int i;
	for(i=0;i<16;++i)
	{
		err = pthread_create(set+i,NULL,thread4,&lock);
		if(err==-1)
		{
			err=errno;
			printf("fail: failed to launch thread. %s",strerror(err));
			exit(1);
		}
	}

	usleep(15*1000*1000);

	for(i=0;i<16;++i)
		pthread_join(set[i],NULL);

	printf("thread0: pass: racing threads for lock\n");
	return;
}

void test_unlock(void)
{
	int err;

	struct ptl_lock*lock;
	struct ptl_lock init = PTL_INIT;

	lock = &init;
	printf("thread0: test: unlock a lock we don't hold\n");
	err = ptl_unlock(lock);
	if(err==0)
	{
		printf("thread0: fail: unlocked a lock we don't hold\n");
		exit(1);
	}
	printf("thread0: pass: failed to unlock a lock we don't hold\n");


	printf("thread0: test: acquire lock\n");
	err = ptl_lock(lock);
	if(err==-1)
	{
		printf("thread0: fail: acquire lock\n");
		exit(1);
	}
	printf("thread0: pass: acquire lock\n");

	printf("thread0: test: launch thread\n");
	pthread_t tid;
	err = pthread_create(&tid,NULL,&thread1,lock);
	if(err==-1)
	{
		err = errno;
		printf("thread0: fail: launch thread: %s\n",strerror(err));
		exit(1);
	}
	printf("thread0: pass: launch thread\n");

	usleep(1*1000*1000);

	printf("thread0: test: join thread\n");
	pthread_join(tid,NULL);
	printf("thread0: pass: join thread\n");

	return;
}

void*thread_fork(void*arg)
{
	struct ptl_lock*lock;
	lock = arg;

	int err;
	err = ptl_lock(lock);
	if(err==-1)
	{
		perror("fail: thread_fork: ptl_lock");
		exit(1);	
	}

	usleep(10*1000*1000);

	ptl_unlock(lock);

	pthread_exit(NULL);
}

void test_fork(void)
{
	int err;
	struct ptl_lock lock;

	err = ptl_init(&lock);
	if(err==-1)
	{
		perror("fail: ptl_init");
		exit(1);
	}

	printf("test: acquire lock in background, wait 10 seconds\n");
	pthread_t tid;
	err = pthread_create(&tid,NULL,&thread_fork,&lock);
	if(err==-1)
	{
		perror("fail: pthread_create");
		exit(1);
	}
	printf("pass: acquire lock in background, wait 10 seconds\n");

	usleep(2*1000*1000);
	
	pid_t pid;
	pid = fork();
	if(pid==-1)
	{
		perror("fail: fork");
		exit(1);
	}
	else if(pid==0)
	{
#if 0
		/**
		when i thought i *could* acquire the background lock, it failed.
		once i thought i *couldn't* it starts succeeding.
		it was because i was checking pid>0 instead of pid==0
		**/
		printf("test: subprocess: attempt to acquire frozen lock (held by thread in parent process)\n");
		err = ptl_trylock(&lock);
		if(err==0)
		{
			printf("test: subprocess: erroniously acquired frozen lock\n");
			exit(1);
		}
		printf("pass: subprocess: failed to acquire frozen lock\n");
		exit(0);	
#else
		printf("test: subprocess: determine if lock has been frozen\n");
		err = ptl_trylock(&lock);
		if(err==0)
			printf("test: subprocess: lock is not frozen\n");
		else
			printf("pass: subprocess: lock has been frozen\n");
		usleep(15*1000*1000);
		printf("subprocess: exiting\n");
		exit(0);
#endif
	}

	printf("test: parent: attempt to acquire lock (held by background thread)\n");
	err = ptl_trylock(&lock);
	if(err==0)
	{
		printf("fail: parent: aquired lock held by background thread\n");
		exit(1);
	}	
	printf("pass: parent: failed attempt to acquire lock (held by background thread)\n");

	usleep(2*1000*1000);

	printf("test: parent: wait on background thread to release lock\n");
	err = ptl_lock(&lock);
	if(err==-1)
	{
		printf("fail: parent: could not acquire lock released by background thread\n");
		exit(0);
	}
	printf("pass: parent: background thread released lock, lock aquired\n");

	printf("parent: waiting on subprocess to exit..."); fflush(stdout);
	wait(NULL);
}

int main(int argc,char*args[])
{
#ifdef _POSIX_THREADSAFE_FUNCTIONS
	printf("_POSIX_THREADSAFE_FUNCTIONS is defined\n");
#else
	printf("_POSIX_THREADSAFE_FUNCTIONS is not defined\n");
#endif 

	printf("testing\n\n");

	test_dynamic(); 
	printf("\n");
	test_static();
	printf("\n");
	test_race();
	printf("\n");
	test_unlock();
	printf("\n");
	test_fork();

	printf("\npass\n"); 

	pthread_exit(0);
	return 0;
}

