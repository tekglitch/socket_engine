#include <windows.h>
#include <string>
#include "socket_engine.hpp"


#define _VERSION 1
#define _NAME "irc"
#define _AUTHOR "glitch"
#define _DESC "irc interface under windows 10"
#define _TYPE MOD_TYPE_CLIENT;


#define IRC_NICK "mod-test"
#define IRC_USER "mod-testu"
#define IRC_RNAME "mod test rname"
#define IRC_CHAN "#collective"
#define IRC_SERVER "irc.umad.us"
#define IRC_PORT 6667





#define DLL_EXPORT __declspec(dllexport)

using namespace std;


extern "C"
{
mod_info DLL_EXPORT preload();
void DLL_EXPORT init(socket_engine*);
void DLL_EXPORT parse(se_fdset*);
struct irc_instance;
}

socket_engine* se = NULL;
irc_instance* irci = NULL;
/// preload will tell the core what we are. In turn, if verified, it will send us a pointer to the engine;
mod_info DLL_EXPORT preload()
{
    mod_info modi;
    modi.version = _VERSION;
    modi.name=_NAME;
    modi.author=_AUTHOR;
    modi.desc=_DESC;
    modi.type=_TYPE;




    return modi;
}



struct irc_cmode
{

};
struct irc_umode
{

};
struct irc_user
{

};
struct irc_channel
{
    string name,title;
    vector<irc_user*> users;
    vector<irc_cmode*> modes;
    //vector<irc_links*> lins;
    irc_channel(string n) : name(n), title("") {}
};
struct irc_instance
{
    string nick, user, rname, server;
    long port, ctime, ptime;
    int idx;
    string mchan;
    vector<irc_channel*> chans;
    vector<irc_umode*> modes;
    se_fdset* fds;
    irc_instance(string n, string u, string rn, string c, string addr, long p) : nick(n), user(u), rname(rn), server(addr), port(p), idx(0),mchan(""), fds(NULL) {}
    ~irc_instance() {}
};

void parseLine(se_fdset* fds, string buf)
{
    if(fds==NULL||fds->fd<0)
        return;
    vector<string> prms;
    do
    {
        int pos = buf.find(' ');
        if (pos == -1)
        {
            prms.push_back(buf);
            break;
        }
        prms.push_back(buf.substr(0,pos));
        buf = buf.substr(pos+1);
    } while (buf.length() > 0);
    int prmc = prms.size();
    if (prms[0]=="PING")
        fds->_send("PONG %s\r\n",prms[1].c_str());
    if(prms[0]=="JOIN")
    {
        irci->chans.push_back(new irc_channel(prms[1]));
    }
    if(prmc>1 && prms[1]=="001")
        fds->_send("JOIN %s\r\n",IRC_CHAN);

    else if (prmc>3 && prms[1]=="PRIVMSG")
    {
        prms[3] = prms[3].substr(1);
    }
}
void parse(se_fdset* fds)
{
    if(fds==NULL||fds->fd<0)
        return;

    do
    {
        string tmp;
        int pos = fds->recvq.find('\n');
        /* below isn't required as our while loop only runs if \n is found.. but its a double safety mechanism */
        if (pos <= 0)
            break;

        tmp = fds->recvq.substr(0,pos-1);
        fds->recvq = fds->recvq.substr(pos+1); //+1 stops



        if ((int)tmp.length() < 1)
                return;

        //cout << tmp.c_str() << endl;
        parseLine(fds,tmp);

    } while ((int)fds->recvq.find('\n') > -1);


}


// if preload is a success the engine will call our init() function
void DLL_EXPORT init(socket_engine* sengine)
{
    se = sengine;
    se_fdset* fds = se->create_sclient("irc.umad.us", 6667, parse);
    if(fds!=NULL)
    {
        irci = new irc_instance(IRC_NICK,IRC_USER,IRC_RNAME,IRC_CHAN,IRC_SERVER,IRC_PORT);
        irci->fds = fds;
        se->_send(NULL,"NICK %s\r\n;USER %s * *:%s\r\n",IRC_NICK, IRC_USER, IRC_RNAME);
    }
}
