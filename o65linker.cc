#include "o65linker.hh"
#include "msginsert.hh"

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

void O65linker::AddObject(const O65& object, const string& what)
{
    if(linked)
    {
        fprintf(stderr,
            "O65 linker: Attempt to add object \"%s\""
            " after linking already done\n", what.c_str());
        return;
    }
    Object *newobj = new Object(object, what);
    
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
    defines.push_back(make_pair(name, make_pair(value, false)));
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
