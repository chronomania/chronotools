#include <algorithm>
#include <cstdio>
#include <map>

#include "pageptrlist.hh"
#include "msginsert.hh"
#include "ctinsert.hh"

using namespace std;

bool PagePtrList::Data::operator< (const Data& b) const
{
    if(data.size() != b.data.size())
        return data.size() < b.data.size();
    
    return data < b.data;
}

void PagePtrList::Data::Combine(Data& b, unsigned offset)
{
    // "b" is better, but it is supposed to leave as empty.
    
    data.clear();
    data.swap(b.data);
    
    for(unsigned a=0; a<refs.size(); ++a)
        refs[a].offset += offset;
    
    refs.insert(refs.end(), b.refs.begin(), b.refs.end());
    b.refs.clear();
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

void PagePtrList::Combine()
{
    sort(items.begin(), items.end());
    
    unsigned n = 0, c = 0, t = 0;
    for(unsigned a=items.size(); a-- > 0; )
    {
        MessageWorking();
        
        t += items[a].data.size();
        for(unsigned b=a; ++b != items.size(); )
        {
            const vector<unsigned char>& A = items[a].data;
            const vector<unsigned char>& B = items[b].data;
            if(A.size() > B.size()) continue;
            
            unsigned diff = B.size() - A.size();
            for(unsigned pos=diff+1; pos-- > 0; )
            {
                if(std::equal(A.begin(), A.end(),
                              B.begin() + pos))
                {
                    ++n;
                    c += A.size();
                    
                    /*
                    fprintf(stderr,
                        "Combine of %u bytes (%u -> %u(%u), pos=%u)\n",
                            A.size(),
                            a, b, B.size(), pos);
                    */
                    
                    items[a].Combine(items[b], pos);
                    
                    goto Done;
                }
            }
        }
    Done: ;
    }
    
    /*
    fprintf(stderr, "%u/%u strings combined, saving %u/%u bytes\n",
        n, items.size(), c, t);
    */
}

namespace
{
    const string CreateRefName()
    {
        static unsigned refcounter = 0;
        char Buf[64];
        std::sprintf(Buf, "<ref%u>", ++refcounter);
        return Buf;
    }
}

void PagePtrList::Create(insertor& ins,
                         int page,
                         const std::string& what,
                         const std::string& tablename)
{
    Combine();
    
    O65linker::LinkageWish wish;
    if(page >= 0)
    {
        wish.SetLinkagePage(page);
    }
    else
    {
        wish.SetLinkageGroup(ins.objects.CreateLinkageGroup());
    }
    
    map<unsigned short, string> NamedRefs;
    
    const string objname = what + " data";
    for(unsigned a=0; a<items.size(); ++a)
    {
        MessageWorking();
        
        const Data& d = items[a];
        if(d.data.empty()) continue;
        
        O65 tmp;
        tmp.LoadCodeFrom(d.data);
        
        for(unsigned b=0; b<d.refs.size(); ++b)
        {
            const string name = CreateRefName();
            tmp.DeclareCodeGlobal(name, d.refs[b].offset);
            NamedRefs[d.refs[b].ptraddr] = name;
        }
        
        //char Buf[64];
        //sprintf(Buf, " #%u", a);
        ins.objects.AddObject(tmp, objname, wish);
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

        tmp.ResizeCode(n * 2);
        
        for(unsigned a=0; a<n; ++a)
        {
            tmp.DeclareWordRelocation(i->second, a*2);
            ++i;
        }
        
        if(!tablename.empty()) tmp.DeclareCodeGlobal(tablename, 0);
        if(page >= 0)
        {
            unsigned address = first | (page << 16) | 0xC00000;
            wish.SetAddress(address);
            tmp.LocateCode(address);
        }
        ins.objects.AddObject(tmp,
            tablename.empty()
                ? what+" table"
                : tablename, wish);
    }
}

void PagePtrList::Create(insertor& ins,
                         const std::string& what,
                         const std::string& tablename)
{
    Create(ins, -1, what, tablename);
}

#if 0
void Dump(const PagePtrList& tmp)
{
    for(unsigned a=0; a<tmp.items.size(); ++a)
    {
        const PagePtrList::Data& d = tmp.items[a];
        
        printf("%.*s\n",
            d.data.size(),
            &d.data[0]);
        
        for(unsigned b=0; b<d.refs.size(); ++b)
            printf("- ref %u -> %u\n", d.refs[b].ptraddr, d.refs[b].offset);
    }
    printf("----\n");
}

int main(void)
{
    PagePtrList tmp;
    
    tmp.AddItem("kissa", 10);
    tmp.AddItem("kis",   12);
    tmp.AddItem("koira", 16);
    tmp.AddItem("sa",    14);
    
    Dump(tmp);
    tmp.Combine();
    
    Dump(tmp);
}
#endif
