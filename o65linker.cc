#include "o65linker.hh"
#include "msginsert.hh"

#define IPS_ADDRESS_EXTERN 0x01
#define IPS_ADDRESS_GLOBAL 0x02

#include <list>

using std::list;

struct Object
{
    O65 object;
    unsigned addr;
    string name;
    vector<string> symlist;
    vector<string> extlist;
    
public:
    Object(const O65& obj, const string& what)
    : object(obj),
      addr(0),
      name(what),
      symlist(obj.GetSymbolList()),
      extlist(obj.GetExternList())
    {
    }
    
    Object(): addr(0)
    {
    }
    
    void Release()
    {
        //*this = Object();
    }
};

void O65linker::AddObject(const O65& object, const string& what, unsigned addr)
{
    if(linked)
    {
        fprintf(stderr,
            "O65 linker: Attempt to add object \"%s\""
            " after linking already done\n", what.c_str());
        return;
    }
    Object *newobj = new Object(object, what);
    newobj->addr = addr;
    
    const vector<string>& symlist = newobj->symlist;
    
    unsigned collisions = 0;
    for(unsigned a=0; a<objects.size(); ++a)
    {
        const vector<string>& prevsymlist = objects[a]->symlist;
        for(unsigned b=0; b<symlist.size(); ++b)
            for(unsigned c=0; c<prevsymlist.size(); ++c)
                if(symlist[b] == prevsymlist[c])
                {
                    fprintf(stderr,
                        "O65 linker: ERROR:"
                        " Symbol \"%s\" defined by object \"%s\""
                        " is already present in object \"%s\"\n",
                        symlist[b].c_str(),
                        what.c_str(),
                        objects[a]->name.c_str());
                    ++collisions;
                }
    }
    if(collisions)
    {
        delete newobj;
        return;
    }
    objects.push_back(newobj);
}

const vector<unsigned> O65linker::GetSizeList() const
{
    vector<unsigned> result(objects.size());
    for(unsigned a=0; a<result.size(); ++a)
        result[a] = objects[a]->object.GetCodeSize();
    return result;
}

const vector<unsigned> O65linker::GetAddrList() const
{
    vector<unsigned> result(objects.size());
    for(unsigned a=0; a<result.size(); ++a)
        result[a] = objects[a]->addr;
    return result;
}

void O65linker::PutAddrList(const vector<unsigned>& addrs)
{
    unsigned limit = addrs.size();
    if(objects.size() < limit) limit = objects.size();
    for(unsigned a=0; a<limit; ++a)
    {
        objects[a]->addr = addrs[a];
        objects[a]->object.LocateCode(addrs[a]);
    }
}

const vector<unsigned char>& O65linker::GetCode(unsigned objno) const
{
    return objects[objno]->object.GetCode();
}

const string& O65linker::GetName(unsigned objno) const
{
    return objects[objno]->name;
}

void O65linker::Release(unsigned objno)
{
    objects[objno]->Release();
}

void O65linker::DefineSymbol(const string& name, unsigned value)
{
    if(linked)
    {
        fprintf(stderr, "O65 linker: Attempt to add symbols after linking\n");
    }
    for(unsigned c=0; c<defines.size(); ++c)
    {
        if(defines[c].first == name)
        {
            if(defines[c].second.first != value)
            {
                fprintf(stderr,
                    "O65 linker: Error: %s previously defined as %X,"
                    " can not redefine as %X\n",
                        name.c_str(), defines[c].second.first, value);
            }
            return;
        }
    }
    defines.push_back(std::make_pair(name, std::make_pair(value, false)));
}

void O65linker::AddReference(const string& name, const ReferMethod& reference)
{
    if(linked)
    {
        fprintf(stderr, "O65 linker: Attempt to add references after linking\n");
    }
    referers.push_back(make_pair(reference, make_pair(name, 0)));
}

const vector<pair<ReferMethod, pair<string, unsigned> > >& O65linker::GetReferences() const
{
    if(!linked)
    {
        fprintf(stderr, "O65 linker: Asked for references before linking\n");
    }
    return referers;
}


void O65linker::LinkSymbol(const string& name, unsigned value)
{
    for(unsigned a=0; a<referers.size(); ++a)
    {
        if(name == referers[a].second.first)
        {
            // resolved referer
            //referers[a].second.first  = "";
            referers[a].second.second = value;
        }
    }
    
    for(unsigned a=0; a<objects.size(); ++a)
    {
        Object& o = *objects[a];
        for(unsigned b=0; b<o.extlist.size(); ++b)
        {
            /* If this module is referring to this symbol */
            if(o.extlist[b] == name)
            {
                o.extlist.erase(o.extlist.begin() + b);
                o.object.LinkSym(name, value);
                --b;
            }
        }
    }
}

void O65linker::Link()
{
    if(linked)
    {
        fprintf(stderr, "O65 linker: Attempt to link twice\n");
        return;
    }
    linked = true;
    
    MessageLinkingModules(objects.size());
    
    // For all modules, satisfy their needs.
    for(unsigned a=0; a<objects.size(); ++a)
    {
        Object& o = *objects[a];
        
        MessageLoadingItem(o.name.c_str());
        
        for(unsigned b=0; b<o.extlist.size(); ++b)
        {
            const string& ext = o.extlist[b];
            
            unsigned found=0, addr=0, defcount=0;
            // Find out which module defines this symbol.
            for(unsigned c=0; c<objects.size(); ++c)
            {
                MessageWorking();
                if(objects[c]->object.HasSym(ext))
                {
                    addr = objects[c]->object.GetSymAddress(ext);
                    ++found;
                }
            }
            for(unsigned c=0; c<defines.size(); ++c)
            {
                MessageWorking();
                if(defines[c].first == ext)
                {
                    addr = defines[c].second.first;
                    defines[c].second.second = true;
                    ++defcount;
                }
            }
            if(found == 0 && !defcount)
            {
                fprintf(stderr, "O65 linker: Symbol '%s' still undefined\n", ext.c_str());
                // FIXME: where?
            }
            else if((found+defcount) != 1)
            {
                fprintf(stderr, "O65 linker: Symbol '%s' defined in %u module(s) and %u global(s)\n",
                    ext.c_str(), found, defcount);
            }
            
/*
            if(found > 0)
                fprintf(stderr, "Extern %u(%s) was resolved with linking\n", b, ext.c_str());
            if(defcount > 0)
                fprintf(stderr, "Extern %u(%s) was resolved with a define\n", b, ext.c_str());
*/
            
            if(found > 0 || defcount > 0)
            {
                o.object.LinkSym(ext, addr);
                
                o.extlist.erase(o.extlist.begin() + b);
                --b;
            }
        }
        if(!o.extlist.empty())
        {
            fprintf(stderr, "O65 linker: Still %u undefined symbol(s)\n", o.extlist.size());
            // FIXME: where?
        }
        
        for(unsigned b=0; b<o.symlist.size(); ++b)
        {
            for(unsigned c=0; c<referers.size(); ++c)
            {
                if(o.symlist[b] == referers[c].second.first)
                {
                    // resolved referer
                    //referers[c].second.first  = "";
                    referers[c].second.second = o.object.GetSymAddress(o.symlist[b]);
                }
            }
        }
    }
    
    MessageDone();
 
    for(unsigned a=0; a<objects.size(); ++a)
        objects[a]->object.Verify();

    for(unsigned a=0; a<referers.size(); ++a)
        if(!referers[a].second.second)
        {
            fprintf(stderr,
                "O65 linker: Unresolved reference: %s\n", referers[a].second.first.c_str());
        }

    for(unsigned c=0; c<defines.size(); ++c)
        if(!defines[c].second.second)
        {
            fprintf(stderr,
                "O65 linker: Warning: Symbol \"%s\" was defined but never used.\n",
                defines[c].first.c_str());
        }
}

O65linker::O65linker(): linked(false)
{
}

O65linker::~O65linker()
{
}

namespace
{
    unsigned LoadIPSword(FILE *fp)
    {
        unsigned char Buf[2];
        fread(Buf, 2, 1, fp);
        return (Buf[0] << 8) | Buf[1];
    }
    unsigned LoadIPSlong(FILE *fp)
    {
        unsigned char Buf[3];
        fread(Buf, 3, 1, fp);
        return (Buf[0] << 16) | (Buf[1] << 8) | Buf[2];
    }
    
    struct IPS_item
    {
        unsigned addr;

        bool operator< (const IPS_item& b) const { return addr < b.addr; }
    };
    
    struct IPS_global: public IPS_item
    {
        string   name;
    };
    struct IPS_extern: public IPS_item
    {
        string   name;
        unsigned size;
    };
    struct IPS_lump: public IPS_item
    {
        vector<unsigned char> data;
    };
}

void O65linker::LoadIPSfile(FILE* fp, const string& what)
{
    rewind(fp);
    
    /* FIXME: No validity checks here */
    
    for(int a=0; a<5; ++a) fgetc(fp); // Skip header which should be "PATCH"
    
    list<IPS_global> globals;
    list<IPS_extern> externs;
    list<IPS_lump> lumps;
    
    for(;;)
    {
        unsigned addr = LoadIPSlong(fp);
        if(feof(fp) || addr == 0x454F46) break;
        
        unsigned length = LoadIPSword(fp);
        
        vector<unsigned char> Buf2(length);
        int c = fread(&Buf2[0], 1, length, fp);
        if(c < 0 || c != (int)length) break;
        
        switch(addr)
        {
            case IPS_ADDRESS_GLOBAL:
            {
                std::string name((const char *)&Buf2[0], Buf2.size());
                name = name.c_str();
                unsigned addr = Buf2[name.size()+1]
                             | (Buf2[name.size()+2] << 8)                       
                             | (Buf2[name.size()+3] << 16);
                
                IPS_global tmp;
                tmp.name = name;
                tmp.addr = addr;
                
                globals.push_back(tmp);

                break;
            }
            case IPS_ADDRESS_EXTERN:
            {
                std::string name((const char *)&Buf2[0], Buf2.size());
                name = name.c_str();
                unsigned addr = Buf2[name.size()+1]
                             | (Buf2[name.size()+2] << 8)  
                             | (Buf2[name.size()+3] << 16);
                unsigned size = Buf2[name.size()+4];

                IPS_extern tmp;
                tmp.name = name;
                tmp.addr = addr;
                tmp.size = size;
                
                externs.push_back(tmp);
                
                break;
            }
            default:
            {
                IPS_lump tmp;
                tmp.data = Buf2;
                tmp.addr = addr;
                
                lumps.push_back(tmp);
                
                break;
            }
        }
    }
    
    globals.sort();
    externs.sort();
    lumps.sort();
    
    for(list<IPS_lump>::const_iterator next_lump,
        i = lumps.begin(); i != lumps.end(); i=next_lump)
    {
        next_lump = i; ++next_lump;
        const IPS_lump& lump = *i;
        
        O65 tmp;
        
        tmp.LoadCodeFrom(lump.data);
        tmp.LocateCode(lump.addr);
        
        bool last = next_lump == lumps.end();

        for(list<IPS_global>::iterator next_global,
            j = globals.begin(); j != globals.end(); j = next_global)
        {
            next_global = j; ++next_global;
            
            if(last
            || (j->addr >= lump.addr && j->addr < lump.addr + lump.data.size())
              )
            {
                tmp.DeclareCodeGlobal(j->name, j->addr);
                globals.erase(j);
            }
        }

        for(list<IPS_extern>::iterator next_extern,
            j = externs.begin(); j != externs.end(); j = next_extern)
        {
            next_extern = j; ++next_extern;
            
            if(last
            || (j->addr >= lump.addr && j->addr < lump.addr + lump.data.size())
              )
            {
                switch(j->size)
                {
                    case 1:
                        tmp.DeclareByteRelocation(j->name, j->addr);
                        break;
                    case 2:
                        tmp.DeclareWordRelocation(j->name, j->addr);
                        break;
                    case 3:
                        tmp.DeclareLongRelocation(j->name, j->addr);
                        break;
                }
                externs.erase(j);
            }
        }
        
        char Buf[64];
        sprintf(Buf, "block $%06X of ", lump.addr);
        
        AddObject(tmp, Buf + what, lump.addr);
    }
}
