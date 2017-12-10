#ifndef __SOCKET_ENGINE_HPP__
#define __SOCKET_ENGINE_HPP__


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
//#define WIN32

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
//#include "unix.hpp"
#endif


#include "defines.hpp"


#define dbg(x1) if(SE_DEBUG_MODE==1) cout <<"dbg: "<< x1 << endl;
#if defined(HAS_EPOLL)
//#define USE_EPOLL
#elif defined (HAS_POLL)
//#define USE_POLL
#else
#define USE_SELECT
#endif

#include <signal.h>
#undef USE_BANSERVER
#define BS_PASS "hjv6jhgvtjhuvg"

using namespace std;
enum SE_FLAG
{
    SE_FL_NONE = (1<<0), //unknown clients
    SE_FL_INIT = (1<<1), //connecting(usually for clients.)
    SE_FL_SHUTDOWN = (1<<2), //socket shutting down.(client or server)
    SE_FL_SERV = (1<<3), // all servers MUST contain this flag
    SE_FL_AACCEPT = (1<<4), // auto accept incomming connections(accepts callback)
    SE_FL_CLIENT = (1<<5), // all clients SHOULD contain this flag, but it's not required.
    SE_FL_BS = (1<<6), // ban server

};

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
    void _send(char* buf,...)
    {
       va_list args;
       char buffer[strlen(buf)+512];
       memset(&buffer,0,sizeof(buffer));
       va_start(args, buf);
       vsnprintf(buffer, sizeof(buffer), buf, args);
       va_end(args);
         sendq.append(buffer);
    };
    errSet errs;
    void(*callback)(se_fdset*);
    void(*callback_serv)(se_fdset*,se_fdset*);// null unless im a server and include SE_FL_SERV in my flags
};

#include "modules.hpp"
class socket_engine
{

protected:
    int running;
private:
    timeval _time;
    long _stime;
    int conns;
    vector<se_fdset*> fdsets;
    //vector<se_sfdset*> sfdsets;
public:
    socket_engine();
    ~socket_engine();

    fd_set rSet, wSet, eSet;

    void shutdown();
    string mkstring(int len=6);
    modHandler* mdh;
    string get_line(se_fdset*,char='\n');
    se_fdset* idx2fds(int);
    vector<string> str2prm(string,char=' ');
    vector<string> get_line_as_vec(se_fdset*,char='\n',char=' ');
    string prm2str(vector<string>prms, int spos=0,int epos=0);
    se_fdset* fd2fds(int);
    long get_timeu();
    long get_times();
    long get_time();
    long get_stime();
    timeval get_timev();
    void set_time();
    int update_clis_idx();
    se_fdset* addfd(int, void(*)(se_fdset*),int=SE_FL_INIT);
    void delfd(se_fdset*);
    int _read(se_fdset*);
    int _write(se_fdset*);
    void _send(se_fdset*,string);
    void _send(int, string);
    void _qsend(se_fdset*,string);
    void _qsendf(se_fdset*, char*,...);
    void _send(se_fdset*, char*,...);
    se_fdset* create_sclient(string,long,void(*)(se_fdset*)=NULL,int=SE_FL_CLIENT|SE_FL_INIT);
    se_fdset* create_sserv(string,long,void(*)(se_fdset*),void(*)(se_fdset*,se_fdset*)=NULL,int=SE_FL_SERV);
    se_fdset* _accept(se_fdset*);
    int ccount();
    #ifdef USE_BANSERVER
    struct _s_bs
    {
        se_fdset* fds;
        vector<se_fdset*> list; // currently checking.
        int state;
    }bs;
    string ban_checking;
    void ban_response();
    int ban_lookup(se_fdset*);
    int ban_add(string,unsigned long, string);
    void ban_del(string);
    void ban_del(int);



    #endif
    #if defined(HAS_EPOLL)

    #elif defined(HAS_POLL)

    #else
    void reset_fdsets();
    #endif
    socket_engine* getSE() { return this; }

    int cycle();
    int run(void(*)(void)=NULL);
    vector<int> pids;
};

socket_engine* getSocketEngine();


#endif
