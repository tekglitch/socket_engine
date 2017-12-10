#ifndef __BOT_HPP__
#define __BOT_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <csignal>
#include <malloc.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#ifdef HAS_FCNTL
#include <fcntl.h>
#endif

#include <errno.h>
#include <iomanip>
#include <time.h>
#include <sys/time.h>
#include <list>
#include <sstream>
#include <vector>
#include <cstdio>
#include <iostream>
#include <list>
#include <unistd.h>
#include <cstring>
#include <stdio.h>

#ifdef _WIN32
#ifndef HAS_SOCKLEN_T
typedef int socklen_t;
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <wincrypt.h>
//#include "win32.hpp" // win32 dependant functions
#define sleep(x) Sleep(x)
//#define close(x) closesocket(x)
#define ioctl(x1,x2,x3) ioctlsocket(x1,x2,x3)
#else
#define _close(x) close(x)

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
///#include "unix.hpp"
#endif


#define SE_CLIENT_RECVQ_LENGTH 64000
#define SE_CPMS 10 // Cycles Per MilliSecond
#define SE_MAX_FDS 5000
#define SE_DEBUG_MODE 1

#define dbg(x1) if(SE_DEBUG_MODE==1) cout <<"dbg: "<< x1 << endl;
#if defined(HAS_EPOLL)
//#define USE_EPOLL
#elif defined (HAS_POLL)
//#define USE_POLL
#else
#define USE_SELECT
#endif

#include <signal.h>

using namespace std;


struct errSet
{
    int fatal;
    vector<string> list;
    void call();
};



struct se_fdset
{
    se_fdset(int tfd, void(*cb)(se_fdset*),int flgs) :fd(tfd),index(0),flags(flgs),ctime(0),sbytes(0),lastread(0),rbytes(0),recvq(""),sendq(""),
                                                         maxBufLen(64000),ip("0.0.0.0"),uid("NULL"),callback(cb),callback_serv(NULL) {};

    ~se_fdset(){};
    int fd,index,flags,ctime;
    u_long sbytes,lastread,rbytes;
    string recvq,sendq;
    int maxBufLen;
    string ip;

    string uid; // 15 byte random string
    sockaddr_in saddr;
    void _send(string data) { sendq.append(data); };
    errSet errs;
    void(*callback)(se_fdset*);
    void(*callback_serv)(se_fdset*,se_fdset*);// null unless im a server and include SE_FL_SERV in my flags
};


class socket_engine
{

protected:
    int running;
private:
    timeval _time;
    long _stime;
    int conns;
    vector<se_fdset*> fdsets;

}

#endif // __BOT_HPP__
