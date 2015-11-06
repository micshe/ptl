#portable-thread-locks, or, pthread-less-locks
ptl is a library for providing mutex locks without requiring explicitly linking or working with the pthread library.  it's designed for writing libraries that need to protect their internal state with mutexes, but don't want to force software that uses them to also require pthreads (particularly as it may be using some other threading implementation).

its built on the stdio locking functions provided by posix, that were designed for exactly this purpose, so they aren't a part of pthreads, and are available when the pthreads library is not, and safely do nothing if they aren't needed.


#intended use
any posix software that requires thread-system agnostic mutex locks can go right to the source and use posix's stdio locks as they are, but these locks can only be initialised during runtime, which makes them unsuitable for protecting static data in a library without requiring the library to call an init() function.  the ptl_lock library provides a dirt simple mechanism for statically initialising stdio based locks, that gets around this library-specific issue.


#documentation

 struct ptl_lock*

this is the ptl lock-object, similar to the pthread_mutex_t datatype for pthreads, and used in the same way.  a struct ptl_lock* must be initialised by ptl_init() before it can be locked by ptl_lock(), and must be destroyed by ptl_destroy() before it falls out of scope, to free the underlying system resources.  if a lock must be ready to be locked from process startup (for example, a static lock protecting access to static variables in a library), the lock can be declared like so:

  static struct ptl_lock* static_lock = PTL_INIT;

this pre-initialises the lock, so it is available once the process is loaded.  if it is possible to avoid using this method, do so, as PTL_INIT ptl locks are slower to lock than runtime initialised ptl locks.


 #define PTL_INIT

equivalent to the PTHREAD_MUTEX_INITIALIZER macro from the posix threads library.  used for creating automatically initialised static ptl_lock* structures that can be acquired by any thread at any time from process startup.


 int ptl_init(struct ptl_lock*lock);

initialises @lock so it can be acquired, similar to the pthread_mutex_init() function.


 int ptl_destroy(struct ptl_lock*lock);

destroys @lock, freeing its resources.


 int ptl_lock(struct ptl_lock*lock);

blocks the current thread until it has locked @lock.  if @lock is NULL, returns -1, with errno = EINVAL.  returns 0 on success.


 int ptl_unlock(struct ptl_lock*lock);

unlocks the lock @lock held by the current thread.  if the current thread does not hold @lock, returns -1 with errno = EBUSY.  if @lock is NULL returns -1 with errno = EINVAL.  returns 0 on success.


 int ptl_trylock(struct ptl_lock*lock);

attempts to acquire the lock for the current thread.  if @lock is acquired, returns 0.  if @lock was not acquired (because another thread holds it), returns -1 with errno = EBUSY.  if @lock is NULL returns -1 with errno = EINVAL.


#caveats
ptl_locks are built on stdio streams to the /dev/null device.  if this device is unavailable (for instance, due to entering a chroot() environment), no new locks will be able to be created.  existing locks should not experience any difficulties, as the /dev/null file descriptor is never read by the ptl_lock(), ptl_trylock(), or ptl_unlock() functions.

ptl_locks are built on stdio stream structures to open files.  the underlying file descriptors behind the stdio file structures are all O_CLOEXEC, so the file descriptors won't leak to any untrusted process in the event of a thread exec'ing, or fork'ing and then later execing.

however, the underlying file descriptors can only be closed by a call to ptl_destroy() call, which is not performed in the atfork() handlers.  this means that file descriptors will leak into forked processes unless each lock is explicitely destroyed in the new process.  this leaking does not interfere with their locking behaviour, but it will waste open file descriptors, which are sometimes a limited resource. 


#issues
these locks are extraordinarily slow, and can never be sped up.  pure stdio locks will be faster, so if speed is really an issue, skip over ptl_locks and go straight to them.

how ptl_locks behave is subject to the underlying posix C library implementation.  one particular issue is the behaviour of the lock in the subprocess after a fork().  posix conformant behaviour requires no stdio functions to be called in a subprocess creating from a multithreaded parent process, as many of the locks may be perminently frozen (and possibly other reasons i've yet to sort out).  under GNU 
libc-2.9 this does not appear to occur, but accessing ptl locks in from any subprocess should probably be avoided even so. 

the win32 api provides an equivalent _lock_file() and _unlock_file() pair of stdio mutex lock functions, but no _try_lock_file() function.  porting to win32 would require a redesign to accomodate this.  this should not effect any cygwin-based win32 software.


#future work
currently all ptl_locks are mutex-locks, that can only be held by one thread at a time.  implementing a shared-lock mechanism ontop of these could be useful.

currently the PTL_INIT macro initialises a structure with C99 syntax.  by replacing the ptl_lock structure with an array of void* pointers, the PTL_INIT macro could be used to initialise a structure with C89 syntax, if this is to become an issue.

posix file locks could be used as a substitute for stdio locks if the behaviour of ptl_locks in subprocesses forked from multithreaded parent processes becomes unmanagable.

locks are messy, particularly when multiple locks need to be held by a thread simultaneously.  a pseudo-atomic compare-and-swap operation, built over ptl_locks would be useful for situations that would otherwise require threads having to simultaneously hold many locks. 

