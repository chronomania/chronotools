#include <algorithm>
#include <cstdio>
#include <map>

#include "scriptfile.hh"
#include "pageptrlist.hh"
#include "msginsert.hh"
#include "ctinsert.hh"

using namespace std;

void PagePtrList::Combine(Data& a, const Data& b, unsigned offset) const
{
    // Item "a" will eat item "b".
    
    for(std::list<Reference>::const_iterator
        i = b.refs.begin(); i != b.refs.end(); ++i)
    {
        Reference tmp(*i);
        tmp.offset += offset;
        a.refs.push_back(tmp);
    }
}

void PagePtrList::AddItem(const vector<unsigned char>& d,
                          unsigned short ptraddr)
{
    items.push_back(Data(d, ptraddr));
}

#if 0
void PagePtrList::AddItem(const string& s,
                          unsigned short ptraddr)
{
    vector<unsigned char> d(s.data(), s.data() + s.size());
    items.push_back(Data(d, ptraddr));
}
#endif

class PagePtrListFriend
{
public:
    static bool ItemLengthCompare(const PagePtrList::Data& a, const PagePtrList::Data& b)
    {
        unsigned aa = a.data.size(), bb = b.data.size();
        if(aa != bb) return aa > bb;
        return a.data < b.data;
    }
    static bool ItemAddressCompare(const PagePtrList::Data& a, const PagePtrList::Data& b)
    {
        unsigned aa = a.refs.empty() ? 0 : a.refs.begin()->ptraddr;
        unsigned bb = b.refs.empty() ? 0 : b.refs.begin()->ptraddr;
        if(aa != bb) return aa < bb;
        return a.data < b.data;
    }
};


namespace
{
}

void PagePtrList::Combine()
{
    items.sort(PagePtrListFriend::ItemLengthCompare);
    
#if 0
    unsigned n = 0, c = 0, t = 0;
#endif
    
    for(list<Data>::iterator a = items.begin(); a != items.end(); ++a)
    {
        Data& A = *a;
        
        MessageWorking();
        
#if 0
        t += A.data.size();
#endif
        
        list<Data>::iterator next=a; ++next;
        while(next != items.end())
        {
            list<Data>::iterator b = next++;
            Data& B = *b;
            
            const vector<unsigned char>& Haystack = A.data;
            const vector<unsigned char>& Needle   = B.data;
            
            vector<unsigned char>::const_iterator
                pos = std::search(Haystack.begin(), Haystack.end(),
                                  Needle.begin(),   Needle.end());
            
            if(pos != Haystack.end())
            {
                unsigned int_pos = pos - Haystack.begin();
            
#if 0
                ++n;
                c += Needle.size();
#endif
                
                Combine(A, B, int_pos);
                
                items.erase(b);
            }
        }
    }
    
#if 0
    fprintf(stderr, "\n%u/%u strings combined, saving %u/%u bytes\n",
        n, items.size(), c, t);
#endif

#if 0
    /* If you want to insert them in more-like-original order */
    items.sort(PagePtrListFriend::ItemAddressCompare);
#endif
}

namespace
{
    const string CreateRefName() /* anonymous pointer */
    {
        static unsigned refcounter = 0;
        char Buf[64];
        std::sprintf(Buf, "<ref%u>", ++refcounter);
        return Buf;
    }
    const string CreateRefName(unsigned address)
    {
        string result = "$";
        result += Base62Label(address);
        return result;
    }
}

unsigned PagePtrList::Size() const
{
    unsigned result = 0;
    for(list<Data>::const_iterator a = items.begin(); a != items.end(); ++a)
        result += a->data.size();
    return result;
}

const vector<unsigned char> PagePtrList::GetS() const
{
    vector<unsigned char> result;
    result.reserve(Size());
    for(list<Data>::const_iterator a = items.begin(); a != items.end(); ++a)
        result.insert(result.end(), a->data.begin(), a->data.end());
    return result;
}

struct PagePtrList::PagePtrInfo
{
    enum { has_page, has_base, anonymous } type;
    
    unsigned char page;
    unsigned base;
};

void PagePtrList::Create(insertor& ins,
                         unsigned char page,
                         const std::string& what,
                         const std::string& tablename)
{
    PagePtrInfo info;
    info.type     = PagePtrInfo::has_page;
    info.page     = page;
    
    Create(ins, what, tablename, info);
}

void PagePtrList::Create(insertor& ins,
                         const std::string& what,
                         unsigned original_address,
                         const std::string& tablename)
{
    PagePtrInfo info;
    info.type     = PagePtrInfo::has_base;
    info.base     = original_address;

    Create(ins, what, tablename,info);
           
}

void PagePtrList::Create(insertor& ins,
                         const std::string& what,
                         const std::string& tablename)
{
    PagePtrInfo info;
    info.type     = PagePtrInfo::anonymous;

    Create(ins, what, tablename,info);
           
}

void PagePtrList::Create(insertor& ins,
                         const std::string& what,
                         const std::string& tablename,
                         const PagePtrInfo& info)
{
    Combine();
    
    LinkageWish wish;
    
    switch(info.type)
    {
        case PagePtrInfo::has_page:
            wish.SetLinkagePage(info.page);
            break;
        case PagePtrInfo::has_base:
        case PagePtrInfo::anonymous:
            wish.SetLinkageGroup(ins.objects.CreateLinkageGroup());
            break;
    }
    
    map<unsigned short, string> NamedRefs;
    
    const string objname = what + " data";

    for(list<Data>::const_iterator a = items.begin(); a != items.end(); ++a)
    {
        MessageWorking();
        
        const Data& d = *a;
        if(d.data.empty()) continue;
        
        O65 tmp;
        tmp.LoadSegFrom(CODE, d.data);
        
        string detail = "";
        
        for(std::list<Reference>::const_iterator
            i = d.refs.begin(); i != d.refs.end(); ++i)
        {
            const Reference& ref = *i;
            
            string name;
            
            switch(info.type)
            {
                case PagePtrInfo::has_page:
                    name = CreateRefName(ref.ptraddr | (info.page << 16));
                    break;
                case PagePtrInfo::has_base:
                    name = CreateRefName(ref.ptraddr + (info.base & 0xFF0000));
                    break;
                case PagePtrInfo::anonymous:
                    name = CreateRefName();
                    break;
            }
            
            tmp.DeclareGlobal(CODE, name, ref.offset);
            NamedRefs[ref.ptraddr] = name;
            
            detail += ' ';
            detail += name;
            
#if 0
            for(unsigned a=0; a<d.data.size(); ++a)
            {
                char Buf[8];
                sprintf(Buf, " %02X", d.data[a]);
                detail += Buf;
            }
#endif
        }
        
        ins.objects.AddObject(tmp, objname + detail, wish);
    }
    
    for(map<unsigned short, string>::const_iterator
        i = NamedRefs.begin(); i != NamedRefs.end(); )
    {
        MessageWorking();
        
        //fprintf(stderr, "Now: %u\t%s\n", i->first, i->second.c_str());

        O65 tmp;
        
        unsigned first = i->first, prev = first;
        
        map<unsigned short, string>::const_iterator last;
        
        unsigned n=1;
        for(last=i, prev=first; ++last != NamedRefs.end(); ++n, prev = last->first)
        {
            if(last->first != prev+2) break;
        }

        tmp.Resize(CODE, n * 2);
        
        for(unsigned a=0; a<n; ++a)
        {
            tmp.DeclareWordRelocation(CODE, i->second, a*2);
            ++i;
        }
        
        if(!tablename.empty()) tmp.DeclareGlobal(CODE, tablename, 0);

        switch(info.type)
        {
            case PagePtrInfo::has_page:
            {
                unsigned address = first | (info.page << 16);
                /* This address is supposed to be ROM-based */
                wish.SetAddress(address);
                tmp.Locate(CODE, address);
                break;
            }
            case PagePtrInfo::has_base:
            case PagePtrInfo::anonymous:
                break;
        }

        ins.objects.AddObject(tmp,
            tablename.empty()
                ? what+" table"
                : tablename, wish);
    }
}



#if 0 /* DEBUG */
class PagePtrListFriend
{
public:
    void Dump(const PagePtrList& tmp)
    {
        for(list<PagePtrList::Data>::const_iterator
            a = tmp.items.begin();
            a != tmp.items.end(); ++a)
        {
            const PagePtrList::Data& d = *a;
            
            printf("%.*s\n",
                d.data.size(),
                &d.data[0]);
            
            for(std::list<PagePtrList::Reference>::const_iterator
                i = d.refs.begin(); i != d.refs.end(); ++i)
            {
                printf("- ref %u -> %u\n", i->ptraddr, i->offset);
            }
        }
        printf("----\n");
    }
};

static void Dump(const PagePtrList& p)
{
    PagePtrListDebugger tmp;
    tmp.Dump(p);
}

static const std::vector<unsigned char> Vec(const std::string& s)
{
    std::vector<unsigned char> result(s.begin(), s.end());
    return result;
}

int main(void)
{
    PagePtrList tmp;
    
    tmp.AddItem(Vec("kissa"), 10);
    tmp.AddItem(Vec("kis"),   12);
    tmp.AddItem(Vec("koira"), 16);
    tmp.AddItem(Vec("sa"),    14);
    
    Dump(tmp);
    tmp.Combine();
    
    Dump(tmp);
}
#endif
