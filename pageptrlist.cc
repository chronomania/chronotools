#include <algorithm>
#include <cstdio>
#include <map>

#include "pageptrlist.hh"
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
    for(unsigned a=0; a<b.refs.size(); ++a)
    {
        b.refs[a].offset += offset;
        refs.push_back(b.refs[a]);
    }
    b.data.clear();
    b.refs.clear();
}

void PagePtrList::AddItem(const vector<unsigned char>& d,
                          unsigned short ptraddr)
{
    items.push_back(Data(d, ptraddr));
}

void PagePtrList::AddItem(const string& s,
                          unsigned short ptraddr)
{
    vector<unsigned char> d(s.data(), s.data() + s.size());
    items.push_back(Data(d, ptraddr));
}

void PagePtrList::Sort()
{
    sort(items.begin(), items.end());
}

void PagePtrList::Combine()
{
    unsigned n = 0, c = 0, t = 0;
    for(unsigned a=0; a<items.size(); ++a)
    {
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
                    
                    items[b].Combine(items[a], pos);
                    
                    goto Done;
                }
            }
        }
    Done: ;
    }
    
    fprintf(stderr, "%u/%u strings combined, saving %u/%u bytes\n",
        n, items.size(), c, t);
}

namespace
{
    static unsigned refcounter = 0;
}

void PagePtrList::Create(insertor& ins,
                         unsigned char page,
                         const std::string& what,
                         const std::string& tablename)
{
    Sort();
    Combine();
    
    O65linker::LinkageWish wish;
    wish.SetLinkagePage(page);
    
    map<unsigned short, string> NamedRefs;
    
    for(unsigned a=0; a<items.size(); ++a)
    {
        const Data& d = items[a];
        if(d.data.empty()) continue;
        
        O65 tmp;
        tmp.LoadCodeFrom(d.data);
        
        for(unsigned b=0; b<d.refs.size(); ++b)
        {
            char Buf[64];
            std::sprintf(Buf, "<ref%u>", ++refcounter);
        
            string name = Buf;
            tmp.DeclareCodeGlobal(name, d.refs[b].offset);
            NamedRefs[d.refs[b].ptraddr] = name;
        }
        
        //char Buf[64];
        //sprintf(Buf, " #%u", a);
        ins.objects.AddObject(tmp, what, wish);
    }
    
    for(map<unsigned short, string>::const_iterator
        i = NamedRefs.begin(); i != NamedRefs.end(); ++i)
    {
        //fprintf(stderr, "Now: %u\t%s\n", i->first, i->second.c_str());

        unsigned address = i->first | (page << 16) | 0xC00000;
        O65 tmp;
        
        unsigned first = i->first, prev = first;
        
        map<unsigned short, string>::const_iterator last;
        
        for(last=i, prev=first; ++last != NamedRefs.end(); prev = last->first)
        {
            if(last->first != prev+2) break;
        }

        unsigned n = (prev - first + 1);
        tmp.ResizeCode(2 * n);
        
        for(last=i, prev=first; ++last != NamedRefs.end(); prev = last->first)
        {
            if(last->first != prev+2) break;
            tmp.DeclareWordRelocation(i->second, prev - first);
            i = last;
        }
        
        wish.SetAddress(address);
        tmp.LocateCode(address);
        if(!tablename.empty()) tmp.DeclareCodeGlobal(tablename, address);
        ins.objects.AddObject(tmp, what+" table", wish);
        //fprintf(stderr, "Wrote %u pointers...\n", n);
    }
}

void PagePtrList::Create(insertor& ins,
                         const std::string& what,
                         const std::string& tablename)
{
    Sort();
    Combine();
    
    O65linker::LinkageWish wish;
    wish.SetLinkageGroup(ins.objects.CreateLinkageGroup());
    
    map<unsigned short, string> NamedRefs;
    
    for(unsigned a=0; a<items.size(); ++a)
    {
        const Data& d = items[a];
        if(d.data.empty()) continue;
        
        O65 tmp;
        tmp.LoadCodeFrom(d.data);
        
        for(unsigned b=0; b<d.refs.size(); ++b)
        {
            char Buf[64];
            std::sprintf(Buf, "<ref%u>", ++refcounter);
        
            string name = Buf;
            tmp.DeclareCodeGlobal(name, d.refs[b].offset);
            
            NamedRefs[d.refs[b].ptraddr] = name;
        }
        
        ins.objects.AddObject(tmp, what, wish);
    }

/*    
    fprintf(stderr, "Named refs:\n");
    for(map<unsigned short, string>::const_iterator
        i = NamedRefs.begin(); i != NamedRefs.end(); ++i)
    {
        fprintf(stderr, "%u\t%s\n", i->first, i->second.c_str());
    }
*/
    
    for(map<unsigned short, string>::const_iterator
        i = NamedRefs.begin(); i != NamedRefs.end(); ++i)
    {
        O65 tmp;
        
        unsigned first = i->first, prev = first;
        
        map<unsigned short, string>::const_iterator last;
        
        for(last=i, prev=first; ++last != NamedRefs.end(); prev = last->first)
        {
            if(last->first != prev+2) break;
        }

        unsigned n = (prev - first + 1);
        tmp.ResizeCode(2 * n);
        
        for(last=i, prev=first; ++last != NamedRefs.end(); prev = last->first)
        {
            if(last->first != prev+2) break;
            tmp.DeclareWordRelocation(i->second, prev - first);
            i = last;
        }
        
        if(!tablename.empty()) tmp.DeclareCodeGlobal(tablename, 0);
        ins.objects.AddObject(tmp, what+" table", wish);
    }
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
    
    tmp.Sort();
    
    Dump(tmp);
    tmp.Combine();
    
    Dump(tmp);
}
#endif
