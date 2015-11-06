#include<stdio.h>
#include<errno.h>

#include<fcntl.h>
#include<unistd.h>

#include"ptl.h"

int ptl_init(struct ptl_lock*lock)
{
	if(lock==NULL)
	{
		errno = EINVAL;
		return -1;
	}

	FILE*tmp;
	int fd;
	fd = open("/dev/null",O_RDONLY|O_CLOEXEC|O_NOCTTY); 
	tmp = fdopen(fd,"r");
	if(tmp==NULL)
		return -1;
	lock->flag = 0;
	lock->count = 0;
	lock->file = tmp;
	return 0;
}
int ptl_destroy(struct ptl_lock*lock)
{
	if(lock==NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if(lock->file==stdin||lock->file==stdout||lock->file==stderr)
	{
		lock->file=NULL;
		lock->flag = 0;
		return 0;
	}

	fclose(lock->file);
	lock->file=NULL;
	lock->flag = 0;
	lock->count = 0;

	return 0;
}

int ptl_unlock(struct ptl_lock*lock)
{
	int err;
	err = ptl_trylock(lock);
	if(err!=0)
		return -1;

	if(lock->count<1)
	{
		/* unreachable */
		funlockfile(lock->file);
		errno = EINVAL;
		return -1;
	}

	if(lock->count==1)
	{
		lock->count=0;
		funlockfile(lock->file);
		errno = EBUSY;
		return -1;
	}

	lock->count = lock->count - 2;

	funlockfile(lock->file);
	funlockfile(lock->file);

	return 0;	
}

int ptl_lock(struct ptl_lock*lock)
{
	FILE*tmp;

	int err;
	if(lock==NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if(lock->flag==0)
	{
		/* lock has never been held before */
		flockfile(stderr);

		/* 
		once we have stderr locked, lock->flag won't be
		changed by any other thread.
		*/
		if(lock->flag==1)
		{
			/* we 'misread' on first check */
			funlockfile(stderr);
			/* lock->file will not be a corrupt value since flag==1 */
			flockfile(lock->file);
			/* FIXME check for overflow? */
			lock->count = lock->count + 1;
			return 0;
		}	
		else
		{
			/* we're the first to hold the lock */
			int fd;
			fd = open("/dev/null",O_RDONLY|O_CLOEXEC|O_NOCTTY);
			tmp = fdopen(fd,"r");
			if(tmp==NULL)
			{
				/* fail */
				err = errno;
					funlockfile(stderr);
				errno = err;
				return -1;	
			}

			/* acquire new lock */
			flockfile(tmp);

			/*
			as long as these two updates
			occur within these two locks,
			their order shouldn't matter.
			*/
			lock->file = tmp;	
			lock->flag = 1;

			lock->count =  1;

			funlockfile(stderr);
			/* return holding the new lock */
			return 0;
		}	
	}

	/* lock has been held before */

	flockfile(lock->file);
	/* FIXME check for overflow? */
	lock->count = lock->count + 1;

	return 0;
}

int ptl_trylock(struct ptl_lock*lock)
{
	int err;
	FILE*tmp;

	if(lock==NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if(lock->flag==0)
	{ 
		err = ftrylockfile(stderr);

		if(err!=0)
		{
			errno=EBUSY;
			return -1; 
		}
		if(lock->flag==1)
		{
			funlockfile(stderr);
			/* NOTE state can change before we re-lock */
			err = ftrylockfile(lock->file);
			if(err==0)
			{
				/* FIXME check for overflow? */
				lock->count = lock->count + 1;
				return 0;
			}
			else
			{
				errno = EBUSY;
				return -1;
			}
		}

		int fd;
		fd = open("/dev/null",O_RDONLY|O_CLOEXEC|O_NOCTTY);
		tmp = fdopen(fd,"r");
		if(tmp==NULL)
		{
			err = errno;
				funlockfile(stderr);
			errno = err;
			return -1;
		}

		/* since this is a new file, it is guarenteed to succeed */
		flockfile(tmp);

		lock->file = tmp;
		lock->flag = 1;
		lock->count = 1;

		funlockfile(stderr);
		return 0;
	} 
	err = ftrylockfile(lock->file);	
	if(err!=0)
	{
		errno = EBUSY;
		return -1;
	}

	/* FIXME check for overflow? */
	lock->count = lock->count + 1;
	return 0;
}

