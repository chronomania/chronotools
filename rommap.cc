#include <set>
#include <vector>

#include "rommap.hh"

using namespace std;

namespace
{
    const bool TryFindExtraSpace = false;
    const unsigned ExtraSpaceMinLen = 128;
    
    vector<unsigned char> space, protect;
}

unsigned char *ROM;

#ifndef WIN32
/* We use memory mapping in Linux. It's fast. */
#include <sys/mman.h>
#define USE_MMAP 1
#endif

namespace
{
    void MarkingError(const set<unsigned>& errlist, const char *type)
    {
        set<unsigned>::const_iterator i;
        
        bool begun=false;
        unsigned first=0;
        unsigned prev=0;
        
        for(i=errlist.begin(); ; ++i)
        {
            if(i == errlist.end() || (begun && prev < *i-1))
            {
                fprintf(stderr, "Error: %06X-%06X already marked %s\n", first,prev, type);
                begun = false;
                if(i == errlist.end()) break;
            }
            
            if(!begun) { begun = true; first = *i; }
            prev = *i;
        }
    }
}

void ShowProtMap()
{
    static const char *const types[4] = {"?","free","protected","ERROR"};
    
    fprintf(stderr, "Prot/free map:\n");
    unsigned romsize = space.size();
    
    bool begun=false;
    unsigned first=0;
    unsigned lasttype=0;
    
    for(unsigned a=0; a<=romsize; ++a)
    {
        unsigned type = 0;
        if(a == romsize
        || (begun && lasttype != (type = (space[a]?1:0) + (protect[a]?2:0)))
          )
        {
            fprintf(stderr, "  %06X-%06X: %s (%u bytes)\n",
                first,a-1, types[lasttype], a-first);
            begun = false;
            if(a == romsize) break;
        }
        if(!begun) { begun = true; first=a; }
        lasttype = type;
    }
}

void MarkFree(unsigned begin, unsigned length)
{
    set<unsigned> error;
    //fprintf(stderr, "Marking %u bytes free at %06X\n", length, begin);
    for(; length>0; --length)
    {
        if(protect[begin]) error.insert(begin);
        space[begin++] = true;
    }
    
    if(!error.empty()) MarkingError(error, "protected, attempted to free");
}

void MarkProt(unsigned begin, unsigned length)
{
    set<unsigned> error;
    
    //fprintf(stderr, "Marking %u bytes protected at %06X\n", length, begin);
    for(; length>0; --length)
    {
        if(space[begin]) error.insert(begin);
        protect[begin++] = true;
    }

    if(!error.empty()) MarkingError(error, "free, attempted to protect");
}

void UnProt(unsigned begin, unsigned length)
{
    for(; length>0; --length)
        protect[begin++] = false;
}

void LoadROM(FILE *fp)
{
    fprintf(stderr, "Loading ROM...");
    unsigned hdrskip = 0;

    char HdrBuf[512];
    fread(HdrBuf, 1, 512, fp);
    if(HdrBuf[1] == 2)
    {
        hdrskip = 512;
    }
    fseek(fp, 0, SEEK_END);
    unsigned romsize = ftell(fp)-hdrskip;
    fprintf(stderr, " %u bytes...", romsize);
    ROM = NULL;
#ifdef USE_MMAP
    /* This takes about 0.0001s on my computer over nfs */
    ROM = (unsigned char *)mmap(NULL, romsize, PROT_READ, MAP_PRIVATE, fileno(fp), hdrskip);
#endif
    /* mmap could have failed, so revert to reading */
    if(ROM == NULL || ROM == (unsigned char*)(-1))
    {
        /* This takes about 5s on my computer over nfs */
        ROM = new unsigned char [romsize];
        fseek(fp, hdrskip, SEEK_SET);
        fread(ROM, 1, romsize, fp);
    }
    else
        fprintf(stderr, " memmap succesful (%p),", (const void *)ROM);
    fprintf(stderr, " done");
    space.clear();
    space.resize(romsize);
    protect.clear();
    protect.resize(romsize);
    fprintf(stderr, "\n");
}

void FindEndSpaces(void)
{
    if(!TryFindExtraSpace) return;
    
    fprintf(stderr, "Finding free space...");
    unsigned pagecount = (space.size()+0xFFFF) >> 16;
    for(unsigned page=0; page<pagecount; ++page)
    {
        unsigned pageend = (page+1) << 16;
        if(pageend >= space.size()) pageend = space.size();
        unsigned pagebegin = (page << 16);
        unsigned pagesize = pageend - pagebegin;
        
        //const char *beginptr = (const char *)&ROM[pagebegin];
        //const char *endptr   = (const char *)&ROM[pageend];
        unsigned size = pagesize;
        
        static const char blanks[] = {0x00, (char)0xEF, (char)0xFF};
        
        for(unsigned blank=0; blank < sizeof(blanks); ++blank)
        {
            unsigned blanklen=0;
            for(unsigned blapos=pagebegin; blapos<pageend; ++blapos)
            {
                if(ROM[blapos] == blanks[blank])
                {
                    ++blanklen;
                    if(blanklen == ExtraSpaceMinLen)
                    {
                        MarkFree(blapos-(ExtraSpaceMinLen-1), ExtraSpaceMinLen);
                    }
                    if(blanklen >= ExtraSpaceMinLen)
                        space[blapos] = true;
                }
                else
                    blanklen=0;
            }
#if 0
            vector<char> BlankBuf(64, blanks[blank]);
            
            const char *ptr = mempos(beginptr, size, &BlankBuf[0], BlankBuf.size());
            if(!ptr)continue;
            
            for(unsigned blapos = (ptr-beginptr);
                ROM[pagebegin+blapos] == blanks[blank]
             && blapos < pageend; ++blapos)
            {
                space[pagebegin+blapos] = true;
            }
#endif
        }
    }
    fprintf(stderr, " done\n");
}

void ListSpaces(void)
{
    fprintf(stderr, "Dumping free space list...");
    unsigned pagecount = (space.size()+0xFFFF) >> 16;
    for(unsigned page=0; page<pagecount; ++page)
    {
        unsigned pageend = (page+1) << 16;
        if(pageend >= space.size()) pageend = space.size();
        unsigned pagesize = pageend - (page << 16);
        
        bool freehere = false;
        unsigned ptr = (page<<16);
        for(unsigned p=0; p<pagesize; ++p, ++ptr)
        {
            unsigned freebegin = p;
            while(p < pagesize && space[ptr])
                ++ptr, ++p;
                
            if(freebegin < p)
            {
                if(!freehere)
                {
                    printf("*s%02X ;Free space in segment $%02X:\n", page, page);
                    freehere = true;
                }
                printf("$%04X:%04X ; %u\n", freebegin, p, p-freebegin);
            }
        }
        if(freehere)
            printf("\n\n");
    }
    fprintf(stderr, " done\n");
}

