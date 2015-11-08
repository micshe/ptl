#ifndef PTL_H
#define PTL_H

#ifdef __cplusplus
extern "C" {
#endif

struct ptl_lock
{
	FILE*file;
	unsigned char flag;
	unsigned long count;
};

#define PTL_INIT { .file = NULL, .flag = 0, .count = 0 }

int ptl_init(struct ptl_lock*lock);
int ptl_destroy(struct ptl_lock*lock);

int ptl_lock(struct ptl_lock*lock); 
int ptl_trylock(struct ptl_lock*lock); 
int ptl_unlock(struct ptl_lock*lock);

#ifdef __cplusplus
}
#endif

#endif

