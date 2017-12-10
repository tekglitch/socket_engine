#include "socket_engine.hpp"
/// rename to "socket_engine"
/**

works on unix systems but select returns -1 with errno set to 0 on windows
. problem has been rectified. fd was not being initialized durring se_fdset creation ~glitch
new problem: on WIN systems select becomes unreliable above 64 connections.
it stops reporting read/write events but still accepts incomming connections?

**/

///delfd
using namespace std;
enum
{
    CAN_=0,
    CANT_=(1<<0),
    TRIED_=(1<<1)
};
socket_engine* se = new socket_engine();

socket_engine* getSocketEngine()
{
    return se;
}
struct Client;

struct Server
{
    se_fdset* fds;
    vector<se_fdset*> clis;
    void add(se_fdset* cfds) { clis.push_back(cfds); };
    Server(se_fdset* tfds) : fds(tfds) {};
    ~Server() { for (int x = 0;x<clis.size();x++) { se->delfd(clis[x]); delete clis[x]; } };

};

struct Client
{
    se_fdset* fds;
    long ttl,stime;
    string rstr;

    Client(se_fdset* tfds) :fds(tfds),ttl((int)(rand()%1500)+1), stime(se->get_times()),rstr(se->mkstring(10000)) {};

};

struct ClientType
{
    string name,desc;
    void(*callback)(se_fdset*);
};

Server* myserv;
vector<Client*> clis;
vector<ClientType*> clitypes;


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

    if(prmc>1 && prms[1]=="001")
        fds->_send("JOIN #collective\r\n");

    else if (prmc>3 && prms[1]=="PRIVMSG")
    {
        prms[3] = prms[3].substr(1);
        if(prms[3]=="!clients")
        {
            if(prmc>4 && prms[4]=="-t")
            {
                if(prmc==5)
                {
                    se->_send(fds,"PRIVMSG %s :There are currently %i categories:\r\n",prms[2].c_str(),clitypes.size());
                    if(clitypes.size()==0)
                        return;
                    for (int x = 0; x < clitypes.size();x++)
                        se->_send(fds,"PRIVMSG %s :%s, %s\r\n",prms[2].c_str(),clitypes[x]->name.c_str(), clitypes[x]->desc.c_str());
                } else {

                }
            }
        }
        if(prms[3]=="!conns")
        {
            for (int x = 0; x < se->ccount();x++)
            {
                se_fdset* tfds = se->idx2fds(x);
                if (tfds==NULL)
                    continue;
                string type = (tfds->flags & SE_FL_SERV) ? "server":"client";
                if(x==fds->index)
                    type = "me";
                #ifdef USE_BANSERVER
                else if(tfds==se->bs.fds)
                    type = "banserver";
                #endif
                se->_send(fds, "PRIVMSG %s :[%i](%s) IDX: %i, FD: %i, Connected: %i seconds.\r\n",prms[2].c_str(),x,type.c_str(),tfds->index,tfds->fd,(se->get_times()-tfds->ctime));
            }
        }
        else if (prms[3]=="!close")
        {
            if (prmc < 5)
            {
                se->_send(fds,"PRIVMSG %s :Syntax: !close socketfd\r\n",prms[2].c_str());
                return;
            }
            se_fdset* dfds = se->fd2fds(atoi(prms[4].c_str()));
            if(dfds==NULL)
                return;

            se->delfd(dfds);
            se->_send(fds,"PRIVMSG %s :Removed %s.\r\n",prms[2].c_str(),prms[4].c_str());

        }
        else if (prms[3]=="!sd")
        {
            se->shutdown(); // incase the user doesn't call it
        }
        else if (prms[3]=="!ban")
        {
            if(prms[4]=="-a"  && prmc>6)
            {
             //   se->_qsendf(se->bs.fds, "ADD -b %s %s %s\r\n", prms[5].c_str(),prms[6].c_str(), se->prm2str(prms,7).c_str());
            } else if (prms[4]=="-d" && prmc>5)
            {
             //  se->_qsendf(se->bs.fds, "DEL -b %s\r\n", prms[5].c_str());
            }
        }
        else if (prms[3]=="!exempt")
        {
            if(prms[4]=="-a" && prmc>6)
            {
             //   se->_qsendf(se->bs.fds, "ADD -e %s %s %s\r\n", prms[5].c_str(),prms[6].c_str(), se->prm2str(prms,7).c_str());
            } else if (prms[4]=="-d" && prmc>5)
            {
              // se->_qsendf(se->bs.fds, "DEL -e %s\r\n", prms[5].c_str());
            }
        }
    }
}

void bitch(se_fdset* fds)
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

void myclis(se_fdset* fds)
{
    if(fds==NULL||fds->fd<0)
        return;
    while(1)
    {
        vector<string> prms = se->get_line_as_vec(fds);
        int prmc = prms.size();
        if(prmc==0)
            break;

        if (prms[0]=="PING")
            fds->_send("PONG %s\r\n",prms[1].c_str());
    }
}

void servAccept(se_fdset* sfds,se_fdset* cfds)
{
    if(sfds==myserv->fds)
        myserv->clis.push_back(cfds);
        se->_send(cfds,"yo wtf is up homie(%i)\r\n",cfds->index);
}



void mkserv()
{
    ///if auto accept flag is set  parameter 3 will become the new clients callback,
    ///parameter 4 will be the server's accept callback which can be NULL.
    /// if parameter 4 is NULL, than parameter 3 cannot be NULL and must be used as the new clients callback
    se_fdset* fds = se->create_sserv("127.0.0.1",7456,myclis,servAccept,SE_FL_SERV|SE_FL_AACCEPT);

    myserv = new Server(fds);
}

int xtr=0;
void ncycle()
{

  //else se->shutdown();
  sleep(100);
}

int main(int argc,char *argv[])
{

    //mkserv();
    //se->mdh = new modHandler(se);
   // se->mdh->newMod(string("./mods/irc.dll"), 1);
    //se->mdh->newMod(string("./mods/http.dll"), 1);
    //se->mdh->newMod(string("./mods/ftp.dll"), 1);

    //mkserv();
    cout << "hello?" << endl;
    //se->run(ncycle);

    //se->shutdown();
    //delete se;
    return 0;
}
