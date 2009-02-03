#include "o65linker.hh"
#include "msginsert.hh"

#include <list>
#include <utility>
#include <map>

using std::make_pair;
using std::list;
using std::map;

#include "hash.hh"
#include "miscfun.hh"

class O65linker::Object
{
public:
    O65 object;
private:
    std::string name;
public:
    vector<std::string> extlist;
    
    LinkageWish linkage;
    
public:
    Object(const O65& obj, const std::string& what, LinkageWish link)
    : object(obj),
      name(what),
      extlist(obj.GetExternList()),
      linkage(link)
    {
    }
    
    Object()
    : object(),
      name(),
      extlist(),
      linkage()
    {
    }
    
    const std::string& GetName() const { return name; }
    
    void Release()
    {
        //*this = Object();
    }
    
    bool operator< (const Object& b) const
    {
        return linkage.GetAddress() < b.linkage.GetAddress();
    }
};

class O65linker::definedata
{
    std::string name;
    unsigned address;
    bool used;
public:
    definedata(const std::string& n, unsigned a) : name(n), address(a), used(false)
    {
    }
    const std::string& GetName() const { return name; }
    unsigned GetAddress() const { return address; }
    bool IsUsed() const { return used; }
    void Use() { used=true; }
};
class O65linker::referdata
{
    ReferMethod ref;
    std::string name;
public:
    referdata(const ReferMethod& r, const std::string& n): ref(r), name(n)
    {
    }
    const std::string& GetName() const { return name; }
    const ReferMethod& GetReference() const { return ref; }
};


class O65linker::SymCache
{
    typedef hash_map<std::string, unsigned> cachetype;
    
    cachetype sym_cache;
public:
    /* Caches the symbols of a new object */
    void Update(const Object& o, unsigned objnum)
    {
        const vector<std::string> symlist = o.object.GetSymbolList(CODE);
        for(unsigned a=0; a<symlist.size(); ++a)
            sym_cache[symlist[a]] = objnum;
    }
    
    /* Resolves which object is defining the given symbol */
    bool Find(const std::string& sym, unsigned& objnum) const
    {
        cachetype::const_iterator i = sym_cache.find(sym);
        if(i == sym_cache.end()) return false;
        objnum = i->second;
        return true;
    }
};

void O65linker::AddObject(const O65& object, const std::string& what, LinkageWish linkage)
{
    if(linked && linkage.type != LinkageWish::LinkHere)
    {
        fprintf(stderr,
            "O65 linker: Attempt to add object \"%s\""
            " after linking already done\n", what.c_str());
        return;
    }
    
    const vector<std::string> symlist = object.GetSymbolList(CODE);
    
    bool clean = true;
    for(unsigned a=0; a<symlist.size(); ++a)
    {
        unsigned objnum;
        if(symcache->Find(symlist[a], objnum))
        {
            MessageDuplicateDefinitionAt(symlist[a], what,
                AscToWstr(objects[objnum]->GetName()));
            clean = false;
        }
    }
    
    if(!clean)
    {
        return;
    }
    
    Object *newobj = new Object(object, what, linkage);
    
    objects.push_back(newobj);

    symcache->Update(*newobj, objects.size()-1);
}

void O65linker::AddObject(const O65& object, const std::string& what, unsigned address)
{
    LinkageWish wish;
    wish.SetAddress(address);
    AddObject(object, what, wish);
}

const vector<unsigned> O65linker::GetSizeList(SegmentSelection seg) const
{
    vector<unsigned> result;
    unsigned n = objects.size();
    result.reserve(n);
    for(unsigned a=0; a<n; ++a)
        result.push_back(objects[a]->object.GetSegSize(seg));
    return result;
}

const vector<unsigned> O65linker::GetAddrList(SegmentSelection seg) const
{
    vector<unsigned> result;
    unsigned n = objects.size();
    result.reserve(n);
    for(unsigned a=0; a<n; ++a)
    {
        if(seg == CODE)
            result.push_back(objects[a]->linkage.GetAddress());
        else
            result.push_back(0);
    }
    return result;
}

const vector<LinkageWish> O65linker::GetLinkageList(SegmentSelection seg) const
{
    vector<LinkageWish> result;
    unsigned n = objects.size();
    result.reserve(n);
    for(unsigned a=0; a<n; ++a)
        if(seg == CODE)
            result.push_back(objects[a]->linkage);
        else
            result.push_back(LinkageWish());
    return result;
}

void O65linker::PutAddrList(const vector<unsigned>& addrs, SegmentSelection seg)
{
    unsigned limit = addrs.size();
    if(objects.size() < limit) limit = objects.size();
    for(unsigned a=0; a<limit; ++a)
    {
        if(seg == CODE)
            objects[a]->linkage.SetAddress(addrs[a]);
        objects[a]->object.Locate(seg, addrs[a]);
    }
}

const vector<unsigned char>& O65linker::GetCode(unsigned objno) const
{
    return objects[objno]->object.GetSeg(CODE);
}

const std::string& O65linker::GetName(unsigned objno) const
{
    return objects[objno]->GetName();
}

void O65linker::Release(unsigned objno)
{
    objects[objno]->Release();
}

void O65linker::DefineSymbol(const std::string& name, unsigned value)
{
    if(linked)
    {
        fprintf(stderr, "O65 linker: Attempt to add symbols after linking\n");
    }
    for(unsigned c=0; c<defines.size(); ++c)
    {
        if(defines[c]->GetName() == name)
        {
            /* Already defined */
            if(defines[c]->GetAddress() != value)
            {
                /* Different address */
                MessageDuplicateDefinitionAs(name, defines[c]->GetAddress(), value);
            }
            /* No need to define again */
            return;
        }
    }
    defines.push_back(new definedata(name, value));
}

void O65linker::AddReference(const std::string& name, const ReferMethod& reference)
{
    unsigned objnum;
    if(symcache->Find(name, objnum))
    {
        const Object& o = *objects[objnum];
        if(o.linkage.type == LinkageWish::LinkHere)
        {
            unsigned value = o.object.GetSymAddress(CODE, name);
            // resolved referer
            FinishReference(reference, value, name);
            return;
        }
    }

    if(linked)
    {
        fprintf(stderr, "O65 linker: Attempt to add references after linking\n");
    }
    referers.push_back(new referdata(reference, name));
}

void O65linker::FinishAndDeleteReference(unsigned refnum, unsigned value)
{
    const std::string& name = referers[refnum]->GetName();
    const ReferMethod& ref = referers[refnum]->GetReference();
    FinishReference(ref, value, name);
    delete referers[refnum];
    referers.erase(referers.begin() + refnum);
}

void O65linker::FinishReference(const ReferMethod& reference,
                                unsigned target,
                                const std::string& what)
{
    unsigned pos    = reference.GetAddr();
    unsigned value  = reference.Evaluate(target);
    unsigned nbytes = reference.GetSize();
    
    if(nbytes < 4)
    {
        value &= (1 << (nbytes*8)) - 1;
    }
    std::string title =
        format("ref %s: $%0*X", what.c_str(), nbytes*2, value);

    vector<unsigned char> bytes;
    for(unsigned n=0; n<nbytes; ++n)
    {
        bytes.push_back(value & 255);
        value >>= 8;
    }
    
    AddLump(bytes, pos, title);
}

void O65linker::AddLump(const vector<unsigned char>& source,
                        unsigned address,
                        const std::string& what,
                        const std::string& name)
{
    O65 tmp;
    tmp.LoadSegFrom(CODE, source);
    tmp.Locate(CODE, address);
    if(!name.empty()) tmp.DeclareGlobal(CODE, name, address);
    
    LinkageWish wish;
    wish.SetAddress(address);
    
    AddObject(tmp, what, wish);
}

void O65linker::AddLump(const vector<unsigned char>& source,
                        const std::string& what,
                        const std::string& name)
{
    O65 tmp;
    tmp.LoadSegFrom(CODE, source);
    tmp.DeclareGlobal(CODE, name, 0);
    AddObject(tmp, what);
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

    // For each module, satisfy each of their externs one by one.
    for(unsigned a=0; a<objects.size(); ++a)
    {
        Object& o = *objects[a];
        if(o.linkage.type != LinkageWish::LinkHere)
        {
            MessageModuleWithoutAddress(o.GetName(), CODE);
            continue;
        }
        
        MessageLoadingItem(o.GetName());
        
        for(unsigned b=0; b<o.extlist.size(); ++b)
        {
            const std::string& ext = o.extlist[b];
            
            unsigned found=0, addr=0, defcount=0;
            
            unsigned objnum;
            if(symcache->Find(ext, objnum))
            {
                addr = objects[objnum]->object.GetSymAddress(CODE, ext);
                ++found;
            }
            
            // Or if it was an external definition.
            for(unsigned c=0; c<defines.size(); ++c)
            {
                if(defines[c]->GetName() == ext)
                {
                    addr = defines[c]->GetAddress();
                    defines[c]->Use();
                    ++defcount;
                }
            }
            
            if(found == 0 && !defcount)
            {
                MessageUndefinedSymbol(ext, o.GetName());
            }
            else if((found+defcount) != 1)
            {
                MessageDuplicateDefinition(ext, found, defcount);
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
            MessageUndefinedSymbols(o.GetName(), o.extlist.size());
        }
    }

    for(unsigned a=0; a<referers.size(); ++a)
    {
        const std::string& name = referers[a]->GetName();
        unsigned objnum;
        if(symcache->Find(name, objnum))
        {
            const Object& o = *objects[objnum];
            if(o.linkage.type != LinkageWish::LinkHere) continue;
            
            unsigned value = o.object.GetSymAddress(CODE, name);
            
            // resolved referer
            FinishAndDeleteReference(a, value);
            --a;
        }
    }
    
    MessageDone();
 
    for(unsigned a=0; a<objects.size(); ++a)
        objects[a]->object.Verify();

    if(!referers.empty())
    {
        //fprintf(stderr,
        //    "O65 linker: Leftover references found.\n");
        
        // ERROR
        for(unsigned a=0; a<referers.size(); ++a)
            MessageUnresolvedSymbol(referers[a]->GetName());
    }

    for(unsigned c=0; c<defines.size(); ++c)
    {
        if(!defines[c]->IsUsed())
        {
            // WARNING
            MessageUnusedSymbol(defines[c]->GetName());
        }
    }
}

O65linker::O65linker()
   : symcache(new SymCache),
     objects(),
     defines(),
     referers(),
     num_groups_used(0),
     linked(false)
{
}

O65linker::~O65linker()
{
    delete symcache;
    for(unsigned a=0; a<objects.size(); ++a) delete objects[a];
    for(unsigned a=0; a<defines.size(); ++a) delete defines[a];
    for(unsigned a=0; a<referers.size(); ++a) delete referers[a];
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
     public:
        IPS_item(): addr(0) { }
    };
    
    struct IPS_global: public IPS_item
    {
        std::string   name;
     public:
        IPS_global(): IPS_item(), name() { }
    };
    struct IPS_extern: public IPS_item
    {
        std::string   name;
        unsigned size;
     public:
        IPS_extern(): IPS_item(), name(), size() { }
    };
    struct IPS_lump: public IPS_item
    {
        vector<unsigned char> data;
     public:
        IPS_lump(): IPS_item(), data() { }
    };
}

void O65linker::LoadIPSfile(FILE* fp, const std::string& what)
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
        if(feof(fp) || addr == IPS_EOF_MARKER) break;
        
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
        
        tmp.LoadSegFrom(CODE, lump.data);
        tmp.Locate(CODE, lump.addr);
        
        bool last = next_lump == lumps.end();

        for(list<IPS_global>::iterator next_global,
            j = globals.begin(); j != globals.end(); j = next_global)
        {
            next_global = j; ++next_global;
            
            if(last
            || (j->addr >= lump.addr && j->addr < lump.addr + lump.data.size())
              )
            {
                tmp.DeclareGlobal(CODE, j->name, j->addr);
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
                        tmp.DeclareByteRelocation(CODE, j->name, j->addr);
                        break;
                    case 2:
                        tmp.DeclareWordRelocation(CODE, j->name, j->addr);
                        break;
                    case 3:
                        tmp.DeclareLongRelocation(CODE, j->name, j->addr);
                        break;
                }
                externs.erase(j);
            }
        }
        
        LinkageWish wish;
        wish.SetAddress(lump.addr);
        AddObject(tmp, format("block $%06X of ", lump.addr) + what, wish);
    }
}

void O65linker::SortByAddress()
{
    std::sort(objects.begin(), objects.end());
}
