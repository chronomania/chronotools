#include <cstdio>
#include "space.hh"

using namespace std;

unsigned freespacemap::Find(unsigned page, unsigned length)
{
    freespacemap::iterator mapi = find(page);
    if(mapi == end())
    {
        fprintf(stderr,
            "Page %02X is FULL! No %u bytes of free space there!\n",
            page, length);
        return NOWHERE;
    }
    freespaceset &spaceset = mapi->second;
    freespaceset::const_iterator reci;

    unsigned bestscore = 0;
    freespaceset::const_iterator best = spaceset.end();
    
    for(reci = spaceset.begin(); reci != spaceset.end(); ++reci)
    {
        if(reci->second == length)
        {
            unsigned pos = reci->first;
            // Found exact match!
            spaceset.erase(reci);
            if(spaceset.size() == 0)
                erase(mapi);
            return pos;
        }
        if(reci->second < length)
        {
            // Too small, not good.
            continue;
        }
        
        // The less free space, the better.
        unsigned score = 0x7FFFFFF - reci->second;
        
        if(score > bestscore)
        {
            bestscore = score;
            best = reci;
            //break;
        }
    }
    if(!bestscore)
    {
        fprintf(stderr,
            "Can't find %u bytes of free space in page %02X! (Total left: %u)\n",
            length, page, Size(page));
        //DumpPageMap(page);
        return NOWHERE;
    }
    
//    fprintf(stderr, "Page %02X: %u bytes used (%u left) ", page, length, Size(page));
    
    const unsigned bestpos = best->first;
    freespacerec tmp(best->first + length, best->second - length);
    spaceset.erase(best);
    if(tmp.second)
        spaceset.insert(tmp);
    
//    fprintf(stderr, "- now %u left\n", Size(page));
    
    return bestpos;
}

void freespacemap::DumpPageMap(unsigned pagenum) const
{
    freespacemap::const_iterator mapi = find(pagenum);
    if(mapi == end())
    {
        fprintf(stderr, "(Page %02X full)\n", pagenum);
        return;
    }

    const freespaceset &spaceset = mapi->second;
    freespaceset::const_iterator reci;
    
    fprintf(stderr, "Map:\n");
    for(reci = spaceset.begin(); reci != spaceset.end(); ++reci)
        fprintf(stderr, "  %X: %u\n", reci->first, reci->second);
}

void freespacemap::Report() const
{
    fprintf(stderr, "Free space:");
    freespacemap::const_iterator i;
    unsigned total=0;
    for(i=begin(); i!=end(); ++i)
    {
        freespaceset::const_iterator j;
        unsigned thisfree = 0, hunkcount = 0;
        for(j=i->second.begin(); j!=i->second.end(); ++j)
        {
            thisfree += j->second;
            ++hunkcount;
        }
        total += thisfree;
        if(thisfree)
        {
            fprintf(stderr, " %02X:%u/%u", i->first, thisfree, hunkcount);
        }
    }
    fprintf(stderr, " - total: %u bytes\n", total);
}

unsigned freespacemap::Size() const
{
    freespacemap::const_iterator i;
    unsigned total=0;
    for(i=begin(); i!=end(); ++i)
    {
        freespaceset::const_iterator j;
        unsigned thisfree = 0;
        for(j=i->second.begin(); j!=i->second.end(); ++j)
            thisfree += j->second;
        total += thisfree;
    }
    return total;
}

unsigned freespacemap::Size(unsigned page) const
{
    freespacemap::const_iterator i = find(page);
    unsigned total=0;
    if(i != end())
    {
        freespaceset::const_iterator j;
        for(j=i->second.begin(); j!=i->second.end(); ++j) total += j->second;
	}
    return total;
}
