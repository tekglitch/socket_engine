#ifndef __MODULES_HPP__
#define __MODULES_HPP__

#define MOD_VERSION 1

struct module;
class socket_engine;

enum MOD_TYPE
{
    MOD_TYPE_UNKNOWN = 0,
    MOD_TYPE_CLIENT,
    MOD_TYPE_SERVER,
    MOD_TYPE_CORE,
    MOD_TYPE_EVENT
};

/// functions all modules should contain
typedef void (*mod_init)(socket_engine*); //initialize the module
typedef void (*mod_unload)(module*);

/// functions for client modules
typedef void (*mod_parse)(se_fdset*); // received data from socket.
typedef void (*mod_connected)(se_fdset*);

/// functions for server modules
typedef void (*mod_incomming)(se_fdset*,se_fdset*);
typedef void (*mod_data)(se_fdset*);


class modBase
{


public:
    int version;
    MOD_TYPE type = MOD_TYPE_UNKNOWN;
    string name,author,desc;

    mod_init mit;
    mod_parse mpe;
    mod_unload mud;
    virtual ~modBase() = default;
};


class mod_sinfo : public modBase
{
    mod_incomming minc;
};

class mod_cinfo : public modBase
{
    mod_connected mcon;
};
class socket_engine;
#ifdef _WIN32


struct module
{
    string file,name;
    HINSTANCE mod;
};

class modHandler
{
protected:
    vector<module*> mods;
public:
     modHandler(socket_engine*);
    ~modHandler();
    socket_engine* se;
    bool modExists(string);
    module* newMod(string,bool load=false);
    bool loadmod(module*);
    HINSTANCE unloadmod(string);

};



#else

#endif

#endif
