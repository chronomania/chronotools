#include <cstdio>
#include "space.hh"

using namespace std;

unsigned freespacemap::Find(unsigned page, unsigned length)
{
    iterator mapi = find(page);
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
        if(reci->len == length)
        {
            unsigned pos = reci->pos;
            // Found exact match!
            spaceset.erase(reci);
            if(spaceset.size() == 0)
                erase(mapi);
            return pos;
        }
        if(reci->len < length)
        {
            // Too small, not good.
            continue;
        }
        
        // The smaller, the better.
        unsigned score = 0x7FFFFFF - reci->len;
        
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
        return NOWHERE;
    }
    
    const unsigned bestpos = best->pos + best->len - length;
    freespacerec tmp(best->pos, best->len - length);
    spaceset.erase(best);
    if(tmp.len) spaceset.insert(tmp);
    
    return bestpos;
}

void freespacemap::DumpPageMap(unsigned pagenum) const
{
    const_iterator mapi = find(pagenum);
    if(mapi == end())
    {
        fprintf(stderr, "(Page %02X full)\n", pagenum);
        return;
    }

    const freespaceset &spaceset = mapi->second;
    freespaceset::const_iterator reci;
    
    fprintf(stderr, "Map of page %02X:\n", pagenum);
    for(reci = spaceset.begin(); reci != spaceset.end(); ++reci)
        fprintf(stderr, "  %X: %u\n", reci->pos, reci->len);
}

void freespacemap::Report() const
{
    fprintf(stderr, "Free space:");
    const_iterator i;
    unsigned total=0;
    for(i=begin(); i!=end(); ++i)
    {
        freespaceset::const_iterator j;
        unsigned thisfree = 0, hunkcount = 0;
        for(j=i->second.begin(); j!=i->second.end(); ++j)
        {
            thisfree += j->len;
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
    const_iterator i;
    unsigned total=0;
    for(i=begin(); i!=end(); ++i)
    {
        freespaceset::const_iterator j;
        unsigned thisfree = 0;
        for(j=i->second.begin(); j!=i->second.end(); ++j)
            thisfree += j->len;
        total += thisfree;
    }
    return total;
}

unsigned freespacemap::Size(unsigned page) const
{
    const_iterator i = find(page);
    unsigned total=0;
    if(i != end())
    {
        freespaceset::const_iterator j;
        for(j=i->second.begin(); j!=i->second.end(); ++j) total += j->len;
	}
    return total;
}

const set<unsigned> freespacemap::GetPageList() const
{
	set<unsigned> result;
	for(const_iterator i = begin(); i != end(); ++i)
		result.insert(i->first);
	return result;
}
const freespaceset freespacemap::GetList(unsigned pagenum) const
{
	return find(pagenum)->second;
}

void freespacemap::Add(unsigned page, unsigned begin, unsigned length)
{
	(*this)[page].insert(freespacerec(begin, length));
}
