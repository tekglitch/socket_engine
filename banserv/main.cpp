/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include "socket_engine.hpp"
#include <fstream>

#define DB_PASS "hjv6jhgvtjhuvg"
#define DB_BINDHOST "127.0.0.1"
#define DB_PORT 22345
#define DB_VERSION 1

/// NEED FILE LOCKING
struct _s_serv
{
    int fd,curpos;
    FILE* db;

} me;
struct _s_incli
{
    int fd,state; //state: 0=init,1=vrv ok,2=vrpok

} cli;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

string prm2str(vector<string>prms,int spos=0,int epos=0)
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

// ?27.0.0* | 127.0.0.1
int simple_regex_check(string ip1, string ip2)
{
    unsigned int idx1=0,idx2=0; //start pos
    while(1)
    {
        next:
        if (idx1==ip1.length()-1)
            return 1;
        else if (idx2==ip2.length()-1)
            return 0;
        if(ip1[idx1]=='?')
        {
            //skip one char
            idx1++;
            idx2++;
            continue;
        }
        else if (ip1[idx1]=='*')
        {
            if(idx1==ip1.length()-1)
                return 1;
            idx1++;
            for(;idx2<ip2.length();idx2++)
            {
                if(ip2[idx2]==ip1[idx1])
                    goto next;
            }
            return 0;
        }
        if(ip1[idx1]!=ip2[idx2])
        {
            return 0;
        }
        idx1++;
        idx2++;
    }
    return 0;
}

void send_line(char *buf,...)
{
   va_list args;
   char buffer[strlen(buf)+512];
   memset(&buffer,0,sizeof(buffer));
   va_start(args, buf);
   vsnprintf(buffer, sizeof(buffer), buf, args);
   va_end(args);
   send(cli.fd,buffer,strlen(buffer),0);
}

string fget_line()
{
    int buf;
    stringstream temp;
    me.curpos = ftell(me.db);
    while((buf=fgetc(me.db))!=EOF)
    {
        if(feof(me.db))
            return ""; //empty file?
        if(buf=='\n')
        {
            //cout << "|"<<temp.str()<<"|"<<endl;
            //if(temp.str()[temp.str().length()-2] == '\r')
            //temp.str(temp.str().substr(0,temp.str().length()-2));/
            //fseek(me.db,+1,SEEK_CUR);
            return temp.str();
        }


        temp << (char)buf;
    }

    return ""; // no lines left i guess
}
vector<string> str2prm(string buf)
{
    vector<string> prms;
    do
    {
        int pos = buf.find(' ');
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

void del_by_type(string type,string ip)
{
        fseek(me.db, 0, 0);
        string line = "";
        vector<string> temp;
        int needsave=0;
        while((line=fget_line())!="")
        {
            if(feof(me.db))
                break;
            vector<string> prms = str2prm(line);
            if (prms.size()==0)
                continue;
                //type only contains one usable byte. 'E' or 'B'
            if(prms[0][0]==type[0])
            {
                prms[0] = prms[0].substr(2);
                long ttl = atoi(prms[1].c_str());

                //a ttl of 0 is a perm. ban
                if(prms[0]==ip)
                {
                    send_line("DEL %s\r\n",ip.c_str());
                    needsave=1;
                    continue;
                }   else { temp.push_back(line); }
            }
        }
        if(needsave==1)
        {
            freopen("bans.db","w",me.db);
            for (u_int x=0;x<temp.size();x++)
            {
                temp[x].append("\n");
                fputs(temp[x].c_str(),me.db);
            }
            freopen("bans.db","a+",me.db);
        } else{ temp.clear(); }
        cout << "done" << endl;
}

int is_exempt(int idx,string cip)
{
    fseek(me.db,0,SEEK_SET);
    string line = "";
    while((line=fget_line())!="")
    {
        if(feof(me.db))
            return 0;
        if(line[0]=='E')
        {
            int pos = line.find_first_of(' ');
            if (pos <1)
            {
                cerr << "ERROR :Malformed database"<<endl;
                continue;
            }
            string ip = line.substr(2,pos-2);
            line = line.substr(pos+1);
            pos = line.find_first_of(' ');
            long ttl = atoi(line.substr(0,pos).c_str());
            line = line.substr(pos+1);
            fseek(me.db,0,SEEK_SET);
            if(simple_regex_check(ip,cip)==1)
            {
                send_line("EXM %i %s\r\n",idx,line.c_str());
                return 1;
            } else
            continue;
        }
    }
    return 0;
}

int is_banned(int idx,string cip)
{
    fseek(me.db,0,SEEK_SET);
    string line = "";
    while((line=fget_line())!="")
    {
        if(feof(me.db))
            return 0;
        if(line[0]=='B')
        {
            int pos = line.find_first_of(' ');
            if (pos <1)
            {
                cerr << "ERROR :Malformed database"<<endl;
                continue;
            }
            string ip = line.substr(2,pos-2);
            line = line.substr(pos+1);
            pos = line.find_first_of(' ');
            long ttl = atoi(line.substr(0,pos).c_str());
            line = line.substr(pos+1);
            fseek(me.db,0,SEEK_SET);
            if(simple_regex_check(ip,cip)==1 && !is_exempt(idx,ip))
            {
                send_line("BAN %i %s\r\n",idx,line.c_str());
                return 1;
            } else
            continue;
        }
    }
    return 0;
}
long get_time()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

int pass_matches(string pass)
{
    //do random shit like hash passwords etc.

    if(pass==DB_PASS)
        return 1;

    return 0;
}


int parse_data(string buf)
{
    cout << ">>" << buf <<endl;
    vector<string> prms;
    do
    {
        int pos = buf.find(' ');
        if (pos == -1)
        {
            if (buf.length() > 0)
                prms.push_back(buf);
            break;
        }
        prms.push_back(buf.substr(0,pos));
        buf = buf.substr(pos+1);
    } while (buf.length() > 0);
    int prmc = prms.size();
    if(prmc<1)
        return 0;
    if (cli.state <2)
    {
        if (prms[0]=="VRV" && prmc>2)
        {
            if(atoi(prms[1].c_str())!=DB_VERSION || prms[2]!="BETA")
            {
                send_line("VRV FAILED (Version missmatch)\r\n");
                sleep(50);
                close(cli.fd);
                return -1; //reset
            }
            cli.state++;
            send_line("VRV OK\r\n");
        }
        else if (prms[0]=="VRP" && prmc>1)
        {
            if(cli.state != 1)
            {
                send_line("VRP FAILED (Expected VRV command first.)\r\n");
                sleep(50);
                close(cli.fd);
                return -1; //reset
            }
            if (!pass_matches(prms[1]))
            {
                send_line("VRP FAILED (Invalid connect password)\r\n");

                sleep(50);
                close(cli.fd);
                return -1; //reset
            }
            cli.state++;
            send_line("VRP OK\r\n");

        }
        else
        {
            send_line("ERROR :Not verified. (%s)\r\n",prms[0].c_str());
            sleep(50);
            close(cli.fd);
            return -1; //reset

        }
        return 0;
    }
    if(prms[0]=="CHK" && prmc==3)
    {
        //1==ip, 2==index number
        is_banned(atoi(prms[2].c_str()),prms[1]);
    }
    else if (prms[0]=="ADD" && prmc>3)
    {// add Exempt or Ban
        string type = (prms[1]=="-e") ? "E":"B";
        long ttl = (atoi(prms[3].c_str())==0) ? 0 : (atoi(prms[3].c_str())+get_time());
        stringstream s;
        s<<type<<":"<<prms[2]<<" "<< ttl << " " << prm2str(prms,4)<<"\n";
        fseek(me.db,0,SEEK_END);//move to 1 char BEFORE EOF
        fputs(s.str().c_str(), me.db);
        fflush(me.db);
    }
    else if (prms[0]=="DEL" && prmc>2)
    {
        string type = (prms[1]=="-e") ? "E":"B";
        del_by_type(type,prms[2]);
    }
    else if(prms[0]=="LST")
    {

    }
    return 0;
}
int do_read()
{
    char buffer[1024];
        memset(&buffer,0,1024);
       //  bzero(buffer,256);
         unsigned long wNoBlock = 1;
         ioctl(cli.fd, FIONBIO, &wNoBlock);
         int n = recv(cli.fd,buffer,1024,0);
         wNoBlock = 0;
         ioctl(cli.fd, FIONBIO, &wNoBlock);

         if(n==-1)
         {
             int e = errno;
             switch(e)
             {
                 case 0:
                    return 0;
                break;
                default:
                    cout << "unknown error: " << e <<endl;
                break;
             }
             cout << EAGAIN <<endl;
         }
         if (n < 0)
         {
             close(cli.fd);
             return n;
         }
         cout << n << endl;
         string recvq = buffer;
        do
        {
            string tmp;
            int pos = recvq.find('\n');
            /* below isn't required as our while loop only runs if \n is found.. but its a double safety mechanism */
            if (pos <= 0)
                break;

            tmp = recvq.substr(0,pos-1);
            recvq = recvq.substr(pos+1); //+1 stops

            if ((int)tmp.length() < 1)
                    break;

            if(parse_data(tmp)==-1)
                return -1;
        } while ((int)recvq.find('\n') > -1);
    return 0;
}
void chk_ttl()
{
        fseek(me.db, 0, 0);
        string line = "";
        vector<string> temp;
        int needsave=0;
        while((line=fget_line())!="")
        {
            if(feof(me.db))
                break;
            vector<string> prms = str2prm(line);
            if (prms.size()==0)
                continue;
            if(prms[0][0]=='B' || prms[0][0]=='E') // bans and exempts
            {
                prms[0] = prms[0].substr(2);
                long ttl = atoi(prms[1].c_str());

                //a ttl of 0 is a perm. ban
                if(ttl != 0 && get_time()>=ttl)
                {
                    needsave=1;
                    continue; // this ban's ttl has expired so we skip adding it to the buffer
                }else { temp.push_back(line); }
            }
        }
        if(needsave==1)
        {
            freopen("bans.db","w",me.db);
            for (u_int x=0;x<temp.size();x++)
            {
                temp[x].append("\n");
                fputs(temp[x].c_str(),me.db);
            }
            freopen("bans.db","a+",me.db);
        } else{ temp.clear(); }
}
int main(int argc, char *argv[])
{

    me.db=fopen("bans.db","r+");
    if(!me.db)
    {
        cerr << "uhoh i couldnt open the db file."<<endl;
        return 0;
    }
    #ifdef WIN32
    WORD versionRequested = MAKEWORD (1, 1);
    WSADATA wsaData;

    if (WSAStartup (versionRequested, & wsaData))
       return 0;

    if (LOBYTE (wsaData.wVersion) != 1||
        HIBYTE (wsaData.wVersion) != 1)
    {
       WSACleanup();

       return 0;
    }
    #endif



     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     struct hostent *hostent_p;
     if (!(hostent_p = gethostbyname(DB_BINDHOST)))
     {
        cout<< "Error resolving remote hostname"<<endl;
        return NULL;
     }
     me.fd = socket(AF_INET, SOCK_STREAM, 0);
     if (me.fd < 0)
        error("ERROR opening socket");
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = *(long *) hostent_p->h_addr;
     serv_addr.sin_port = htons((long)DB_PORT);
     if (bind(me.fd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              error("ERROR on binding");
     listen(me.fd,5);
     clilen = sizeof(cli_addr);
     reset:
     cli.fd = accept(me.fd,(struct sockaddr *) &cli_addr,&clilen);
    if (cli.fd < 0)
        error("ERROR on accept");
    printf("Sock accepted.");
    cli.state =0;
    //fd_set rSet;
    //FD_ZERO(&rSet);
     while(1)
     {
        //FD_SET(cli.fd,&rSet);
        fflush(me.db); // need to lookup how intensive calling fflush every cycle is on resources
        do_read();
        /* timeval tv;
        tv.tv_sec=0;
        tv.tv_usec=0;
        if(select(2,&rSet,NULL,NULL,&tv)<0)
            goto reset;

        if(FD_ISSET(cli.fd,&rSet))
        {
            if(do_read()<1)
                continue;
            FD_CLR(cli.fd,&rSet);
        }
        */
        chk_ttl();

     }

     goto reset;
     close(cli.fd);
     close(me.fd);
     fclose(me.db);
     #ifdef WIN32
     WSACleanup();
     #endif
     return 0;

}
