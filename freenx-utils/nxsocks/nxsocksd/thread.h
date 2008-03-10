#ifndef _thread_h_
#define _thread_h_

#include "config.h"
#include <signal.h>

typedef void (*handler)(int fd, void *parm);

extern sig_atomic_t thread_stop;

extern int thread_init(void);

extern int thread_mainloop(void);

extern void thread_fd_register(int fd, handler rdh, handler wrh, handler exh,
			       void *parm);

extern void thread_fd_rd_off(int fd);

extern void thread_fd_rd_on(int fd);

extern void thread_fd_wr_off(int fd);

extern void thread_fd_wr_on(int fd);

extern int thread_fd_close(int fd);

extern int thread_timer_register(time_t secs, handler toh, int id, void *parm);

extern void thread_timer_cancel(int id);

#endif
