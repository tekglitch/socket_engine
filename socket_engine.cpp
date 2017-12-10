#include "socket_engine.hpp"


socket_engine::socket_engine() : running(1),_stime(0),conns(0)
{
    this->set_time();
    this->_stime=this->get_time();
    #ifdef _WIN32
    WORD versionRequested = MAKEWORD (1, 1);
    WSADATA wsaData;

    if (WSAStartup (versionRequested, & wsaData))
       return;

    if (LOBYTE (wsaData.wVersion) != 1||
        HIBYTE (wsaData.wVersion) != 1)
    {
       WSACleanup();

       return;
    }
    #else
    #endif
    srand(time(NULL)^getpid());
    pids.push_back(getpid());
    #ifdef USE_BANSERVER
        bs.fds = this->create_sclient("127.0.0.1",22345,NULL,SE_FL_BS);
        bs.state=0;
        this->_qsend(bs.fds,"VRV 1 BETA\r\n");
    #endif
}

socket_engine::~socket_engine()
{

    this->shutdown();
}

string socket_engine::mkstring(int len)
{
    int x = 0;
    char tmp[len];
    memset (&tmp,0,len);
    for (x = 0; x < len; x++)
        tmp[x]=(rand()%2) ? (char)((rand()%26)+65) : tolower((char)((rand()%26)+65));

    tmp[x] = 0;
    return string(tmp);
}

string socket_engine::get_line(se_fdset* fds,char delim)
{
        string tmp;
        int pos = fds->recvq.find(delim);
        /* below isn't required as our while loop only runs if \n is found.. but its a double safety mechanism */
        if (pos <= 0)
            return "";

        tmp = fds->recvq.substr(0,pos-1);
        fds->recvq = fds->recvq.substr(pos+1);
        return tmp;
}

se_fdset* socket_engine::idx2fds(int idx)
{
    if (idx <= this->fdsets.size() && idx > -1)
        return fdsets[idx];

    return NULL;
}
/*
void socket_engine::probe(string ip,se_fdset* fds)
{
    se_fdset* pfds = create_sclient()
}
*/
vector<string> socket_engine::str2prm(string buf,char delim)
{
    vector<string> prms;
    do
    {
        int pos = buf.find(delim);
        if (pos == -1)
        {
            if (buf.length() > 0)
                prms.push_back(buf);
            break;
        }
        prms.push_back(buf.substr(0,pos));
        buf = buf.substr(pos+1);
    } while (buf.length() > 0);
    return prms;
}


vector<string> socket_engine::get_line_as_vec(se_fdset* fds,char delim1,char delim2)
{
    return this->str2prm(this->get_line(fds,delim1),delim2);
}
string socket_engine::prm2str(vector<string>prms,int spos,int epos)
{
    string temp = "";
    if (epos==0)
        epos = prms.size();

    for(int x = spos; x < epos;x++)
    {
        temp.append(prms[x]);
        if (x!=epos-1)
            temp.append(" ");
    }
    return temp;
}
void socket_engine::shutdown()
{
    #ifdef WIN32
    WSACleanup ();
    #endif
    if (this->fdsets.size()==0)
    {
        this->running=0;
        return;
    }
    for (int x = this->fdsets.size()-1;x>=0;x--)
    {
        this->delfd(this->fdsets[x]);
        sleep(100);
    }
    this->fdsets.clear(); //make sure
    this->running=0;
}

se_fdset* socket_engine::fd2fds(int fd)
{
    if(fd<1)
        return NULL;

    for(int x=0;x<this->fdsets.size();x++)
        if(this->fdsets[x]->fd==fd)
            return this->fdsets[x];

    return NULL;
}

long socket_engine::get_timeu()
{
    return this->_time.tv_usec;
}

long socket_engine::get_times()
{
    return this->_time.tv_sec;
}

timeval socket_engine::get_timev()
{
    return this->_time;
}

long socket_engine::get_time()
{
    return (this->_time.tv_sec * 1000 + this->_time.tv_usec / 1000);
}

void socket_engine::set_time()
{
    gettimeofday(&this->_time, NULL);
}


long socket_engine::get_stime()
{
    return this->_stime;
}


int socket_engine::update_clis_idx()
{
    for (int x=0;x<this->fdsets.size();x++)
    {
        if(this->fdsets[x]->index!=x)
        {
            cerr<<"idx missmatch ("<<this->fdsets[x]->index<<"->"<<x<<"). for socket: "<< this->fdsets[x]->fd <<endl;
            sleep(500);
        }
    }
}

se_fdset* socket_engine::addfd(int fd, void(*cb)(se_fdset*), int flags)
{
    if (fd<0)
        return NULL;
    se_fdset* fds = new se_fdset(fd,cb,flags);

    //u_long wNoBlock = 1;
    //ioctl(fds->fd, FIONBIO, &wNoBlock);
    #ifdef HAS_EPOLL

    #endif

    dbg("Added: "<<fd<< " to event handler.");
    fds->index = this->fdsets.size();
    this->fdsets.push_back(fds);
    fds->uid = this->mkstring(15);
    fds->ctime = this->get_times();
    this->conns++;
    #ifdef USE_SELECT
        cout << "workin?"<<endl;
        if(this->conns>=63)
            dbg("WARNING!!! The winsock select library is considered unreliable above 63 connections.("<<this->conns<<" connections.).");
    #endif
    return fds;
}

//3p1cphuck3R
void socket_engine::delfd(se_fdset* fds)
{
    dbg("removing: "<<fds->fd);
    fds->flags = SE_FL_SHUTDOWN;
    FD_CLR(fds->fd,&this->rSet);
    FD_CLR(fds->fd,&this->wSet);
    FD_CLR(fds->fd,&this->eSet);
    this->fdsets.erase(this->fdsets.begin()+fds->index);
    for (int x = fds->index; x <this->fdsets.size();x++)
    {
        cout << "resetting idx"<<endl;
        if(this->fdsets[x]->index!=x)
            this->fdsets[x]->index = x;

    }

    close(fds->fd);
    delete fds;
    this->conns--;
    #ifdef HAS_EPOLL


    #endif
}
int socket_engine::_read(se_fdset* fds)
{
    if(fds==NULL || fds->fd<1)
        return -1;
    char buf[fds->maxBufLen];
    memset(&buf,0,fds->maxBufLen);
    int rbytes = recv(fds->fd, buf, fds->maxBufLen,0);

    if (rbytes==0)
    {
        cout << "Deleting: "<<fds->fd <<endl;
        this->delfd(fds);
        return 0;
    }
    else if(rbytes==-1)
    {
        int eno = errno;
        cerr<<"recv() failed: ";
        switch(eno)
        {
            case EAGAIN:
            {
                cout<<"timeout.(EAGAIN)"<<endl;
            }
            break;
            #ifndef WIN32
            case ENOTSOCK:
            {
                cout<<"fd is not a valid socket descriptor.(ENOTSOCK)"<<endl;
            }
            break;
            #endif
            case EBADF:
            {
                cout<<"fd:"<<fds->fd<<" is not a valid file descriptor.(EBADF)"<<endl;
            }
            break;
            case EFAULT:
            {
                cout<<"recv buffer overrun,(EFAULT)"<<endl;
            }
            break;
            case EINTR:
            {
                cout<<"signal interrupt.(EINTR)"<<endl;
            }
            break;
            case EINVAL:
            {
                cout<<"invalid argument given.(EINVAL)"<<endl;
            }
            break;
            case ENOMEM:
            {
                cout<<"unable to allocate memory.(ENOMEM)"<<endl;
            }
            break;
            default:
                cout<<"unknown error.(errno:"<<eno<<")"<<endl;
            break;
        }
        this->delfd(fds);
        return -1;
    }

    fds->rbytes += rbytes;
    fds->recvq.append(buf);
    return rbytes;
}

int socket_engine::_write(se_fdset* fds)
{
    if(fds==NULL || fds->fd<1)
        return -1;
    if (fds->sendq.length()<1)
        return 0;


    int sbytes = send(fds->fd,fds->sendq.c_str(),fds->sendq.length(),0);
    if (sbytes==0)
        return 0;
    else if(sbytes==-1)
    {
        cerr<<"send() failed: ";
        switch(errno)
        {
            #ifndef WIN32


            case ECONNRESET:
            {
                cout<<"connection reset by peer.(ECONNRESET)"<<endl;

            }
            break;
            case EDESTADDRREQ:
            {
                cout<<"socket not ready, missing peer address.(EDESTADDRREQ)"<<endl;

            }
            break;
            case EISCONN:
            {
                cout<<"connection-mode socket was connected already but a recipient was still specified.(EISCONN)"<<endl;

            }
            break;
            case EMSGSIZE:
            {
                cout<<"message to large for atomical communication.(EMSGSIZE)"<<endl;

            }
            break;
            case ENOBUFS:
            {
                cout<<"output queue overrun.(ENOBUFS)"<<endl;

            }
            break;
            case ENOTCONN:
            {
                cout<<"socket not connected and no target has been given.(ENOTCONN)"<<endl;

            }
            break;
            case ENOTSOCK:
            {
                cout<<"sockfd is not a valid socket.(ENOTSOCK)"<<endl;

            }
            break;
            case EOPNOTSUPP:
            {
                cout<<"inappropriate flag bit for socket type.(EOPNOTSUPP)"<<endl;

            }
            break;
            #endif
            case EAGAIN:
            {
                cout<<"unable to send packet.(EAGAIN)"<<endl;

            }
            break;
            case EBADF:
            {
                cout<<"not a valid file descriptor.(EBADF)"<<endl;

            }
            break;

            case EFAULT:
            {
                cout<<"invalid space address was specified.(EFAULT)"<<endl;

            }
            break;
            case EINTR:
            {
                cout<<"received signal.(EINTR)"<<endl;

            }
            break;
            case EINVAL:
            {
                cout<<"invalid argument given.(EINVAL)"<<endl;

            }
            break;
            case ENOMEM:
            {
                cout<<"unable to allocate memory.(ENOMEM)"<<endl;

            }
            break;
            case EPIPE:
            {
                cout<<"local end has been shutdown.(EPIPE)"<<endl;

            }
            break;
            default:
            {
                cout << "unknown error.(errno: "<<errno<<")"<<endl;

            }
            break;
        }
        return -1;
    }

    fds->sbytes += sbytes;
    fds->sendq=fds->sendq.substr(sbytes);
    return sbytes;
}

void socket_engine::_send(se_fdset* fds,string data)
{
    if(fds==NULL || fds->fd<1)
        return;
    fds->sendq.append(data);
}

void socket_engine::_send(int fd,string data)
{
    if(fd<1)
        return;
    se_fdset* fds = this->fd2fds(fd);
    if(fds==NULL)
        return;

    this->_send(fds,data);
}

// will try and run even if an error is called
void socket_engine::_qsend(se_fdset* fds, string data)
{
    if(fds==NULL || fds->fd<1)
        return;
    fds->sbytes += send(fds->fd, data.c_str(), data.length(),0);
}

void socket_engine::_qsendf(se_fdset* fds,char *buf,...)
{
    if(fds==NULL || fds->fd<1)
        return;
   va_list args;
   char buffer[strlen(buf)+512];
   memset(&buffer,0,sizeof(buffer));
   va_start(args, buf);
   vsnprintf(buffer, sizeof(buffer), buf, args);
   va_end(args);
   this->_qsend(fds,buffer);
}
extern "C"
{


void socket_engine::_send(se_fdset* fds, char *buf,...)
{
    if(fds==NULL || fds->fd<1)
        return;
   va_list args;
   char buffer[strlen(buf)+512];
   memset(&buffer,0,sizeof(buffer));
   va_start(args, buf);
   vsnprintf(buffer, sizeof(buffer), buf, args);
   va_end(args);
   this->_send(fds,string(buffer));
}
}
#if defined(USE_EPOLL)

#elif defined(USE_POLL)

#else


void socket_engine::reset_fdsets()
{
    FD_ZERO (&this->rSet);
    FD_ZERO (&this->wSet);
    FD_ZERO (&this->eSet);
    for(u_int x = 0; x < fdsets.size();x++)
    {
        se_fdset* fds=fdsets[x];
        if(fds->flags & SE_FL_SHUTDOWN)
            continue;
        if(fds->recvq.length() < SE_CLIENT_RECVQ_LENGTH)
            FD_SET(fds->fd, &this->rSet);
        if(fds->sendq.length() > 0)
            FD_SET(fds->fd, &this->wSet);
        FD_SET(fds->fd, &(this->eSet));
    }
}
#endif


se_fdset* socket_engine::create_sclient(string server, long port,void(*cb)(se_fdset*),int flgs)
{
    dbg("connecting to: " << server.c_str());
    int s;
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        dbg("error creating socket\n");
        return NULL;
    }
    //u_long wNoBlock = 1;
    //ioctl(s, FIONBIO, &wNoBlock);
    struct sockaddr_in addr;
    struct hostent     *hostent_p;
    if (!(hostent_p = gethostbyname(server.c_str())))
    {
        dbg("Error resolving remote hostname\n");
        return NULL;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *(long *) hostent_p->h_addr;
    addr.sin_port = htons(port);
    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        dbg("Error connecting to remote host\n");
        return NULL;
    }
    se_fdset* fds = this->addfd(s,cb,SE_FL_INIT|flgs);
    //wNoBlock = 0;
    //ioctl(s, FIONBIO, &wNoBlock);
    return fds;
}

se_fdset* socket_engine::create_sserv(string localAddr, long localPort,void(*cb)(se_fdset*),void(*cbs)(se_fdset*,se_fdset*),int flgs)
{
    int s, p;
    if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        dbg("Error creating socket\n");
        return NULL;
    }
    struct sockaddr_in local_addr;
    struct hostent     *hostent_p;
    if (!(hostent_p = gethostbyname(localAddr.c_str())))
    {
        dbg("Error resolving ip addr\n");
        return NULL;
    }
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(localPort);
    local_addr.sin_addr.s_addr = *(long *) hostent_p->h_addr;
    if (bind(s, (struct sockaddr *)(&local_addr), sizeof(local_addr)) < 0)
    {
        dbg("Error binding to ip addr\n");
        return NULL;
    }
    if(listen(s, 0) == -1)
    {
        dbg("Error listening on port:" << localPort);
        return NULL;
    }
    dbg("Listening on: "<<localAddr<<"/"<<localPort);
    se_fdset* fds =this->addfd(s,cb,flgs);
    fds->callback_serv = cbs;
    return fds;
}

se_fdset* socket_engine::_accept(se_fdset* fds)
{
    sockaddr_in saddr;
    socklen_t inaddr_size = sizeof(saddr);
    int cfd = accept(fds->fd,(sockaddr*)&saddr, &inaddr_size);
    if(this->fdsets.size()>=SE_MAX_FDS)
    {
       dbg("Error: Max fds have been reached.");
       stringstream line;
       line <<"ERROR :server is full.(" << this->fdsets.size() <<")\r\n";
       send(cfd,line.str().c_str(),line.str().length(),0);
       sleep(50);
       close(cfd);

       return NULL;
    }
    if (cfd<1)
        return NULL;

    se_fdset* cfds = this->addfd(cfd,fds->callback,SE_FL_CLIENT|SE_FL_INIT);
    cfds->saddr=saddr;
    cfds->ip = inet_ntoa(saddr.sin_addr);

    if(fds->callback_serv!=NULL)
        fds->callback_serv(fds,cfds);

    #ifdef USE_BANSERVER
    this->ban_lookup(cfds);
    #endif
    return cfds;

}

int socket_engine::ccount()
{
    return static_cast<int>(this->fdsets.size());
}

#ifdef USE_BANSERVER
void socket_engine::ban_response()
{
    while(1)
    {
        vector<string> prms = this->get_line_as_vec(this->bs.fds);
        int prmc = prms.size();
        if(prmc==0)
            break;
        cout << "here?"<<endl;
        if(this->bs.state < 2)
        {
            if(prms[0]=="VRV" && prmc>1)
            {
                if(prms[1]=="OK")
                {
                    cout << "version ok"<< endl;
                    this->bs.state++;
                    this->_qsendf(this->bs.fds,"VRP %s\r\n",BS_PASS);
                } else {
                    cerr << "Version missmatch for banserver" <<endl;
                }
            } else if(prms[0]=="VRP" && prmc>1)
            {
                cout <<"got VRP"<<endl;
                if(this->bs.state==1)
                {
                    if(prms[1]=="OK")
                    {
                        this->bs.state++;
                        continue;
                    } else
                    {
                        cerr << "Password missmatch for banserver" << endl;
                        return;
                    }
                } else
                {
                    cerr << "banserver tried to verify our pass before our version. so something is pretty fucked up" << endl;
                    continue;
                }
            }
        }
        if (prms[0]=="BAN" && prmc>3)
        {
            cout <<"got ban"<<endl;
            int idx=atoi(prms[1].c_str());
            string reason = this->prm2str(prms,2);
            if(idx<this->bs.list.size() && idx> -1)
            {
                se_fdset* fds = this->bs.list[idx];
                if(fds==NULL)
                    return;

                this->bs.list.erase(this->bs.list.begin()+idx);

                this->_qsendf(fds,"ERROR :You're banned.(%s)\r\n",reason.c_str());
                sleep(50);
                this->delfd(fds);
                continue;
            }
        }
    }
}

int socket_engine::ban_lookup(se_fdset* cfds)
{
    if (cfds==NULL)
        return 0; // already closed
    bs.list.push_back(cfds);
    int idx = bs.list.size()-1;

    this->_qsendf(bs.fds,"CHK %s %i\r\n", cfds->ip.c_str(), idx);
    return 0;
}

int socket_engine::ban_add(string,unsigned long, string)
{
    return 0;
}

void socket_engine::ban_del(string)
{

}
void socket_engine::ban_del(int)
{

}


#endif

int socket_engine::cycle()
{
    this->set_time();
    if(this->fdsets.size()<1)
        return 0;
    #if defined (USE_EPOLL)

    #elif defined (USE_POLL)

    #elif defined (USE_SELECT)

    this->reset_fdsets();

    struct timeval tval;
    tval.tv_sec = 0;
    tval.tv_usec = 100;
    int scount = select(SE_MAX_FDS, &this->rSet, &this->wSet, &this->eSet, &tval);
    if(scount==0)
        return 0;
    else if(scount==-1)
    {
        int eno = errno;
        cerr<<"select() failed: "<<flush;
        switch(eno)
        {
            case EBADF:
            {
                cerr << "bad file descriptor" << endl;
            }
            break;
            case EINTR:
            {
                cerr << "caught signal."<< endl;
            }
            break;
            case EINVAL:
            {
                cerr<<"nfds is invalid/invalid timeout value." << endl;
            }
            break;
            case ENOMEM:
            {
                cerr << "unable to allocate enough memory." << endl;
            }
            break;
            default:
                cerr << "unknown error.(errno: "<<eno<<")"<<endl;

            break;
        }
        this->running = 0;
        return -1;
    }
    for (u_int x = 0; x < fdsets.size(); x++)
    {
        se_fdset* fds = fdsets[x];
        if(fds==NULL)
            continue; //being deleted.
        if(FD_ISSET(fds->fd,&this->eSet))
        {
            cout << "select() wait error" <<endl;
            //we have an error
            FD_CLR(fds->fd,&this->eSet);
        }
        else
        {
            if(FD_ISSET(fds->fd,&this->rSet))
            {
                FD_CLR(fds->fd,&this->rSet);
                //cout << scount <<endl;
                #ifdef USE_BANSERVER
                if(fds->flags & SE_FL_BS)
                {
                    cout << "readin from banserver" << endl;
                    if(this->_read(fds)>0)
                        this->ban_response();

                    continue;
                }
                #endif
                if (fds->flags & SE_FL_SERV && (this->get_time()-fds->lastread)>SE_CPMS)
                {

                    fds->lastread=this->get_time();

                    if(fds->flags & SE_FL_AACCEPT)
                    {
                        se_fdset* cfds = NULL;
                        dbg("Auto accepting connection on fd:"<<fds->fd);
                        cfds = this->_accept(fds);
                        continue;
                    }
                    else
                    {
                        fds->callback(fds);
                    }
                    continue;
                }
                else if(this->_read(fds)<1)
                    continue;

                fds->callback(fds);

            }
            if(FD_ISSET(fds->fd,&this->wSet))
            {
                this->_write(fds);
                FD_CLR(fds->fd,&this->wSet);
            }
        }
    }
    #endif
    return 0;
}


//only called if we run in standalone mode
int socket_engine::run(void(*cb)(void))
{
    while(this->running)
    {
        this->set_time();

        this->cycle();
        if(cb!= NULL)
            cb();
    }
    dbg("shutting down.");
    return 0; //normal shutdown
}

/// pid handler


