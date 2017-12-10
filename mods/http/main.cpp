#include <windows.h>
#include <string>
#include "socket_engine.hpp"


#define _VERSION 1
#define _NAME "http"
#define _AUTHOR "glitch"
#define _DESC "http interface under windows 10"
#define _TYPE MOD_TYPE_SERVER

#define HTTP_ADDR "127.0.0.1"
#define HTTP_PORT 80
#define HTTP_HOME "./htdocs"
#define HTTP_OPEN 1



#define DLL_EXPORT __declspec(dllexport)

using namespace std;


extern "C"
{
    mod_sinfo DLL_EXPORT preload();
    void DLL_EXPORT init(socket_engine*);
    void DLL_EXPORT parse(se_fdset*);

}



/// our pointer back to the engine
socket_engine* se = NULL;



/// preload will tell the core what we are. and allow us to do any setup stuff before the module is fully initialized
mod_sinfo DLL_EXPORT preload()
{
    mod_sinfo modi;
    modi.version = _VERSION;
    modi.name=_NAME;
    modi.author=_AUTHOR;
    modi.desc=_DESC;
    modi.type=_TYPE;


    return modi;
}


struct http_user
{
    string ip;
    long ctime;

};

vector<http_user*> users;
void DLL_EXPORT incomming(se_fdset* serv, se_fdset* cli)
{
    for(int x = users.size()-1; x >-1; x--)
    {
        if(users[x]->ip==cli->ip)
        {
            //begin session we already exist
            return;
        }
    }
    //new session
    http_user* husr = new http_user();
    husr->ip = cli->ip;
    husr->ctime = se->get_time();
   // husr->idle = se->get_time();
}

void DLL_EXPORT parse(se_fdset* fds)
{
    cout << fds->recvq <<endl;
}

void loop()
{
    for(int x = users.size()-1; x >-1; x--)
    {
        //if((se->get_time - users[x]->idle)>10000)
        {
          //session timed out

        }
    }
}

void DLL_EXPORT init(socket_engine* sengine)
{
    se = sengine;
    se_fdset* fds = se->create_sserv(HTTP_ADDR, HTTP_PORT, parse,incomming,SE_FL_SERV|SE_FL_AACCEPT);
    if(fds!=NULL)
    {

    }
}
