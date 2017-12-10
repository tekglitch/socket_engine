#include "socket_engine.hpp"
#include "modules.hpp"


typedef mod_sinfo (*mod_info)(void); // register a mod


modHandler::modHandler(socket_engine* sengine)
{
    se = sengine;
    cout << "module handler initialized" << endl;
}

modHandler::~modHandler()
{
    cout << "module handler shutdown" << endl;
    for (int x = mods.size(); x > -1; x--)
    {
        FreeModule(mods[x]->mod);
        delete mods[x];
    }
}

bool modHandler::modExists(string file)
{
 for(u_int x = 0; x < mods.size();x++)
    if(file==mods[x]->file)
        return true;
    return false;
}

module* modHandler::newMod(string file,bool load)
{
   // if (file.size()<1 || modExists(file))
    //    return NULL;

    ifstream f;
    f.open(file);

    if (f.is_open()) {
      f.close();
      module* nmd = new module();
      nmd->file = file;
      if(load)
        loadmod(nmd);

      mods.push_back(nmd);
      return nmd;
    } else
        cout << "Unable to locate module: " << file <<endl;


    return NULL;
}






void delMod(module* dmd)
{
    if(dmd!=NULL)
    {
        ///dmd->unload();
        delete dmd;
    }
}


bool modHandler::loadmod(module* mod)
{
    HINSTANCE md = LoadLibrary(mod->file.c_str());
    if(md==NULL)
    {
        cout << "Unable to load module: " << mod->file <<endl;
        FreeLibrary(md);
        return false;

    }
    mod->mod = md;
    mod_info ifo = (mod_info)GetProcAddress(md,"preload");
    if(ifo)
    {
        mod_sinfo minf = ifo();
        if(minf.name.size()==0)
        {
            cout <<"Failed to load module: incompatible format." <<endl;
            FreeLibrary(md);
            return false;
        }
        else if (minf.version != MOD_VERSION)
        {
            cout <<"Failed to load module: incompatible version." <<endl;
            FreeLibrary(md);
            return false;
        } else {
            mod_init minit = (mod_init)GetProcAddress(md,"init");
            if(!minit)
            {
                cout << "Failed loading module: code" << GetLastError() << endl;
                FreeLibrary(md);
                return false;
            }

        }
    } else {
        cout <<"Failed to load module: preload() function not found" <<endl;
        FreeLibrary(md);
        return false;

    }
    cout << "Loaded module: " << mod->file << endl;
   // FreeLibrary(md);
    return true;
}
