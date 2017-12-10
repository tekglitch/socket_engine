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



Server* myserv;
vector<Client*> clis;



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
    //if auto accept flag is set  parameter 3 will become the new clients callback,
    //parameter 4 will be the server's accept callback which can be NULL.
    // if parameter 4 is NULL, than parameter 3 cannot be NULL and must be used as the new clients callback
    se_fdset* fds = se->create_sserv("127.0.0.1",7456,myclis,servAccept,SE_FL_SERV|SE_FL_AACCEPT);

    myserv = new Server(fds);
}

void mkclient(long p)
{
    se_fdset* fds = se->create_sclient("127.0.0.1", p,bitch);
    se->_send(fds,"NICK ok\r\nUSER ok * * :slut\r\n");
    clis.push_back(new Client(fds));
    sleep(50);
}




int xtr=0;
void ncycle()
{
    if (xtr++<2000)
        mkclient(7456);
    //else se->shutdown();
    //sleep(100);
}


int main(int argc,char *argv[])
{
    cout << "here" <<endl;
    //mkserv();
    //mkclient(6667);
    se->run(ncycle);
    se->shutdown();
    delete se;
    return 0;
}
