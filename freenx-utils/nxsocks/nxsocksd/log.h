#ifndef _log_h_
#define _log_h_

#include <stdio.h>

#define DEB_CONN        1
#define DEB_CONNS       2
#define DEB_ID          4
#define DEB_IDS         8
#define DEB_SO         16
#define DEB_SOS        32
#define DEB_UDP        64
#define DEB_UDPS      128
#define DEB_RES       256
#define DEB_THR       512
#define DEB_THRTR    1024
#define DEB_CDUMP    8192
#define DEB_DDUMP   16384

#ifdef DEBUG
extern int debug;
#define dprintf0(l,f)         do{if(debug&l)printf(f "\n");}while(0)
#define dprintf1(l,f,a)       do{if(debug&l)printf(f "\n",a);}while(0)
#define dprintf2(l,f,a,b)     do{if(debug&l)printf(f "\n",a,b);}while(0)
#define dprintf3(l,f,a,b,c)   do{if(debug&l)printf(f "\n",a,b,c);}while(0)
#define dprintf4(l,f,a,b,c,d) do{if(debug&l)printf(f "\n",a,b,c,d);}while(0)
#else
#define dprintf0(l,f)
#define dprintf1(l,f,a)
#define dprintf2(l,f,a,b)
#define dprintf3(l,f,a,b,c)
#define dprintf4(l,f,a,b,c,d)
#endif

#define eprintf0(f)           do{fprintf(stderr,f "\n");}while(0)
#define eprintf1(f,a)         do{fprintf(stderr,f "\n",a);}while(0)
#define eprintf2(f,a,b)       do{fprintf(stderr,f "\n",a,b);}while(0)
#define eprintf3(f,a,b,c)     do{fprintf(stderr,f "\n",a,b,c);}while(0)
#define eprintf4(f,a,b,c,d)   do{fprintf(stderr,f "\n",a,b,c,d);}while(0)

#endif
