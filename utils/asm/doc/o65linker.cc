#include "o65linker.hh"

struct Object
{
    O65 object;
    unsigned addr;
    vector<string> symlist;
    vector<string> extlist;
    
public:
    Object(const O65& obj) : object(obj),
                             addr(0),
                             symlist(obj.GetSymbolList()),
                             extlist(obj.GetExternList())
    {
    }
};

void O65linker::AddObject(const O65& object)
{
    objects.push_back(new Object(object));
}

const vector<unsigned> O65linker::GetSizeList() const
{
    vector<unsigned> result(objects.size());
    for(unsigned a=0; a<result.size(); ++a)
        result[a] = objects[a]->object.GetCodeSize();
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

const vector<unsigned char>& O65linker::GetCode(unsigned num) const
{
    return objects[num]->object.GetCode();
}

void O65linker::DefineSymbol(const string& name, unsigned value)
{
    defines.push_back(make_pair(name, value));
}

void O65linker::LinkSymbol(const string& name, unsigned value)
{
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
    // For all modules, satisfy their needs.
    for(unsigned a=0; a<objects.size(); ++a)
    {
        Object& o = *objects[a];
        for(unsigned b=0; b<o.extlist.size(); ++b)
        {
            const string& ext = o.extlist[b];
            
            unsigned found=0, addr=0, defcount=0;
            // Find out which module defines this symbol.
            for(unsigned c=0; c<objects.size(); ++c)
                if(objects[c]->object.HasSym(ext))
                {
                    addr = objects[c]->object.GetSymAddress(ext);
                    ++found;
                }
            for(unsigned c=0; c<defines.size(); ++c)
                if(defines[c].first == ext)
                {
                    addr = defines[c].second;
                    ++defcount;
                }
            if(found == 0 && !defcount)
            {
                fprintf(stderr, "O65 linker: ERROR: Symbol '%s' still undefined\n", ext.c_str());
                // FIXME: where?
            }
            else if((found+defcount) != 1)
            {
                fprintf(stderr, "O65 linker: ERROR: Symbol '%s' defined in %u module(s) and %u global(s)\n",
                    ext.c_str(), found, defcount);
            }
            
            objects[b]->object.LinkSym(ext, addr);
            
            if(found > 0 || defcount > 0)
            {
                o.extlist.erase(o.extlist.begin() + b);
                --b;
            }
        }
        if(o.extlist.size())
        {
            fprintf(stderr, "O65 linker: ERROR: Still %u undefined symbol(s)\n", o.extlist.size());
            // FIXME: where?
        }
    }
 
    for(unsigned a=0; a<objects.size(); ++a)
        objects[a]->object.Verify();
}

O65linker::O65linker()
{
}

O65linker::~O65linker()
{
}
