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
        return data.size() > b.data.size();
    
    return data < b.data;
}

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

void PagePtrList::Combine()
{
    items.sort();
    
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

unsigned PagePtrList::Size() const
{
    unsigned result = 0;
    for(list<Data>::const_iterator a = items.begin(); a != items.end(); ++a)
        result += a->data.size();
    return result;
}

const vector<unsigned char> PagePtrList::GetS() const
{
    vector<unsigned char> result(Size());
    unsigned pos = 0;
    for(list<Data>::const_iterator a = items.begin(); a != items.end(); ++a)
    {
        std::copy(a->data.begin(), a->data.end(),
                  &result[pos]);
        pos += a->data.size();
    }
    return result;
}

void PagePtrList::Create(insertor& ins,
                         int page,
                         const std::string& what,
                         const std::string& tablename)
{
    Combine();
    
    LinkageWish wish;
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

    for(list<Data>::const_iterator a = items.begin(); a != items.end(); ++a)
    {
        MessageWorking();
        
        const Data& d = *a;
        if(d.data.empty()) continue;
        
        O65 tmp;
        tmp.LoadSegFrom(CODE, d.data);
        
        for(std::list<Reference>::const_iterator
            i = d.refs.begin(); i != d.refs.end(); ++i)
        {
            const Reference& ref = *i;
            const string name = CreateRefName();
            tmp.DeclareGlobal(CODE, name, ref.offset);
            NamedRefs[ref.ptraddr] = name;
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

        tmp.Resize(CODE, n * 2);
        
        for(unsigned a=0; a<n; ++a)
        {
            tmp.DeclareWordRelocation(CODE, i->second, a*2);
            ++i;
        }
        
        if(!tablename.empty()) tmp.DeclareGlobal(CODE, tablename, 0);
        if(page >= 0)
        {
            unsigned address = first | (page << 16) | 0xC00000;
            wish.SetAddress(address);
            tmp.Locate(CODE, address);
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
