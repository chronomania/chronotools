#include "stringoffs.hh"
#include "ctinsert.hh"
#include "logfiles.hh"

void stringoffsmap::GenerateNeederList()
{
    neederlist.clear();
    
    for(unsigned parasitenum=0; parasitenum < size(); ++parasitenum)
    {
        const ctstring &parasite = (*this)[parasitenum].str;
        for(unsigned hostnum = 0; hostnum < size(); ++hostnum)
        {
            // Can't depend on self
            if(hostnum == parasitenum) { Continue: continue; }
            // If the host depends on this "parasite", skip it
            neederlist_t::iterator i;
            for(unsigned tmp=hostnum;;)
            {
                i = neederlist.find(tmp);
                if(i == neederlist.end()) break;
                tmp = i->second;
                // Host depends on "parasite", skip this
                if(tmp == parasitenum) { goto Continue; }
            }
            
            const ctstring &host = (*this)[hostnum].str;
            if(host.size() < parasite.size())
                continue;
            
            unsigned extralen = host.size()-parasite.size();
            
            if(host.compare(extralen, parasite.size(), parasite) == 0)
            {
                for(;;)
                {
                    i = neederlist.find(hostnum);
                    if(i == neederlist.end()) break;
                    // Our "host" depends on someone else.
                    // Take his host instead.
                    hostnum = i->second;
                }
                
                neederlist[parasitenum] = hostnum;
                
                // Now if there are parasites referring to this one
                for(i=neederlist.begin(); i!=neederlist.end(); ++i)
                    if(i->second == parasitenum)
                    {
                        // rerefer them to this one's host
                        i->second = hostnum;
                    }

                break;
            }
        }
    }
    
    /* Now the map lists who needs who. */
}

void stringoffsmap::DumpNeederList() const
{
    FILE *log = GetLogFile("mem", "log_addrs");
    if(!log)return;

    typedef map<ctstring, vector<unsigned> > needertmp_t;
    needertmp_t needertmp;
    for(neederlist_t::const_iterator j = neederlist.begin(); j != neederlist.end(); ++j)
    {
        needertmp[(*this)[j->first].str].push_back(j->first);
    }
    for(needertmp_t::const_iterator j = needertmp.begin(); j != needertmp.end(); ++j)
    {
        unsigned hostnum = 0;
        const vector<unsigned> &tmp = j->second;

        unsigned c=0;
        fprintf(log, "String%s", tmp.size()==1 ? "" : "s");
        for(unsigned a=0; a<tmp.size(); ++a)
        {
            neederlist_t::const_iterator i = neederlist.find(tmp[a]);
            if(i == neederlist.end())
            {
                fprintf(log, "*eek*");
            }
            hostnum = i->second;
            if(++c > 15) { fprintf(log, "\n   "); c=0; }
            fprintf(log, " %u", tmp[a]);
        }
        
        fprintf(log, " (%s) depend%s on string %u(%s)\n",
            DispString(j->first).c_str(),
            tmp.size()==1 ? "s" : "",
            hostnum,
            DispString((*this)[hostnum].str).c_str());
    }
}

const set<unsigned> insertor::GetZStringPageList() const
{
    set<unsigned> result;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        unsigned page = i->address >> 16;
        switch(i->type)
        {
            case stringdata::fixed:
                // This is not a pointer
                continue;
            case stringdata::item:
            case stringdata::tech:
            case stringdata::monster:
                // These are handled differently
                continue;
            case stringdata::zptr8:
            case stringdata::zptr12:
                // These are ok
                break;
            // If we omitted something, compiler should warn
        }
        result.insert(page);
    }
    return result;
}

const stringoffsmap insertor::GetZStringList(unsigned pagenum) const
{
    stringoffsmap result;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        unsigned page = i->address >> 16;
        if(page != pagenum) continue;
        switch(i->type)
        {
            case stringdata::fixed:
                // This is not a pointer
                continue;
            case stringdata::item:
            case stringdata::tech:
            case stringdata::monster:
                // These are handled differently
                continue;
            case stringdata::zptr8:
            case stringdata::zptr12:
                // These are ok
                break;
            // If we omitted something, compiler should warn
        }
        stringoffsdata tmp;
        tmp.str  = i->str;
        tmp.offs = i->address;
        result.push_back(tmp);
    }
    return result;
}
