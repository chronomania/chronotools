#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <cctype>

#include "macrogenerator.hh"

static const char WildCard = '?';

static const std::string CreatePattern(const std::string& a, const std::string& b)
{
    if(a == b) return a;
    std::string beginpart, endpart;
    
    size_t beginpartlen = 0;
    while(beginpartlen < a.size()
       && beginpartlen < b.size()
       && a[beginpartlen] == b[beginpartlen])
    {
        ++beginpartlen;
    }
    beginpart = a.substr(0, beginpartlen);
    
    size_t endpartlen = 0;
    while(a.size() > beginpartlen + endpartlen
       && b.size() > beginpartlen + endpartlen
       && a[a.size()-endpartlen-1] == b[b.size()-endpartlen-1])
    {
        ++endpartlen;
    }
    endpart = a.substr(a.size()-endpartlen);
    return beginpart + WildCard + endpart;
}

static const std::string ExpandPattern(const std::string& pat1,
                                       const std::string& pat2)
{
    const std::string result = CreatePattern(pat1, pat2);
    return result;
}

static void ExpandPattern(std::string& pat,
                          const std::string& a, const std::string& b)
{
    const std::string result = ExpandPattern(pat, ExpandPattern(a, b));
/*
    std::cerr << "Expanding (" << pat
              << ") with (" << a
              << ") & (" << b
              << ") -> (" << result << ")\n";
*/
    pat = result;
}

typedef std::list<std::string> codevec_t;
typedef codevec_t::iterator vec_it;

static const std::string GetEye(const std::string& pat, const std::string& s)
{
    size_t patbegin = s.size(), patend = 0, end;
    for(end=0; end<pat.size(); ++end)
        if(pat[end] == WildCard) { patbegin = end; break; }

    for(size_t a=1; a<=pat.size(); ++a)
    {
        size_t patpos = pat.size()-a;
        size_t   spos = s.size()-a;
        patend = spos;
        if(patpos <= end) break;
        if(pat[patpos] == WildCard) break;
    }
    return s.substr(patbegin, patend-patbegin+1);
}

static bool TestAcceptPattern(const std::string& pat, const std::string& s)
{
/*
    std::cerr << "pat(" << pat
              << ") in (" << s
              << ")";
*/
    size_t patend = 0;
    size_t patbegin = pat.find(WildCard), end = patbegin;
    if(patbegin == pat.npos) { end = pat.size(); patbegin = s.size(); }
    
    if(s.compare(0, patbegin, pat, 0, patbegin) != 0) return false;
    
    if(end != pat.size())
    {
        if(end > 0 && std::isalnum(pat[end-1])) return false;
        if(end+1 < pat.size() && std::isalnum(pat[end+1])) return false;
    }
    
    for(size_t a=1; a<=pat.size(); ++a)
    {
        size_t patpos = pat.size()-a;
        size_t   spos = s.size()-a;
        patend = spos;
        if(patpos <= end) break;
        if(pat[patpos] == WildCard) break;
        if(pat[patpos] != s[spos])
        {
            //std::cerr << " -> mismatch\n";
            return false;
        }
    }
    std::string patstring = s.substr(patbegin, patend-patbegin+1);
/*
    std::cerr << " -> (" << patstring
              << ")\n";
*/
    if(patstring.empty()) return true;
    for(size_t a=0; a<patstring.size(); ++a)
    {
        if(!std::isalnum(patstring[a])) return false;
    }
    
    return true;
}

static bool TestAcceptPatterns(const std::vector<std::string>& pats,
                               const std::list<vec_it>& itlist)
{
    typedef std::list<vec_it>::const_iterator it_it;
    //std::cerr << "Testing " << itlist.size() << "\n";
    for(it_it i = itlist.begin(); i != itlist.end(); ++i)
    {
        vec_it j = *i;
        for(size_t a=0; a<pats.size(); ++a)
        {
            if(!TestAcceptPattern(pats[a], *j)) return false;
            ++j;
        }
    }
    return true;
}

void FindPatterns(codevec_t& data)
{
    typedef std::list<vec_it>::const_iterator it_it;

    struct result
    {
        typedef std::pair<unsigned, unsigned> equivalence;
        typedef std::set<equivalence> equivlist;
        typedef std::vector<std::string> patlist;

        std::list<vec_it> itlist;
        unsigned width;
        
        patlist patterns;
        equivlist eqall;
        
        void Modify(codevec_t& data)
        {
            std::map<unsigned, unsigned> eqmap;
            std::set<unsigned> eqset;
            for(equivlist::const_iterator i = eqall.begin();
                                          i != eqall.end();
                                          ++i)
            {
                eqmap[i->second] = i->first;
                eqset.insert(i->second);
            }
            std::string macroname = "MACRO";
            std::string def = "#define ";
            def += macroname;
            def += '(';
            for(size_t p=0,w=0; w<width; ++w)
                if(eqmap.find(w) == eqmap.end()
                && patterns[w].find(WildCard) != patterns[w].npos)
                {
                    char pname[32]; std::sprintf(pname, "_p_%lu", w);
                    if(p++) def += ',';
                    def += pname;
                }
            def += ") ";
            
            for(size_t w=0; w<patterns.size(); ++w)
            {
                if(w > 0) def += " : ";
                
                std::string cmd = patterns[w];
                size_t a = cmd.find(WildCard);
                if(a != cmd.npos)
                {
                    size_t p = w;
                    if(eqmap.find(p) != eqmap.end()) p = eqmap[p];
                    char pname[32]; std::sprintf(pname, "_p_%lu", p);
                    size_t len = std::string(pname).size();
                    cmd.replace(a, 1, pname, 0, len);
                }
                def += cmd;
            }
            
            data.push_front(def);
            
            for(it_it i=itlist.begin(); i!=itlist.end(); ++i)
            {
                vec_it pos = *i;
                
                std::vector<std::string> eyes(width);
                for(size_t w=0; w<width; ++w)
                {
                    //std::cerr << "*pos=(" << *pos << ")\n";
                    eyes[w] = GetEye(patterns[w], *pos);
                    vec_it next = pos; ++next;
                    if(w > 0) data.erase(pos);
                    pos = next;
                }
                
                std::string cmd = macroname;
                cmd += '(';
                for(size_t w=0; w<width; ++w)
                    if(!eyes[w].empty()
                    && eqset.find(w) == eqset.end())
                    {
                        if(w) cmd += ',';
                        cmd += eyes[w];
                    }
                cmd += ')';
                
                pos = *i;
                
                //std::cerr << "*pos(" << *pos << ") = (" << cmd << ")\n";
                *pos = cmd;
            }
        }
        
        void BuildEquivalences()
        {
            size_t depth = itlist.size();
            
            patterns.resize(width);
            
            for(it_it i=itlist.begin(); i!=itlist.end(); ++i)
            {
                vec_it pos = *i, pos0 = *itlist.begin();
                if(pos == pos0)
                {
                    for(size_t w=0; w<width; ++w)
                        patterns[w] = *pos++;
                }
                else
                {
                    for(size_t w=0; w<width; ++w)
                        ExpandPattern(patterns[w], *pos0++, *pos++);
                }
            }
            
            /*
            for(size_t w=0; w<width; ++w)
                std::cerr << patterns[w] << "\n";
            std::cerr << "---\n";
            */
            
            std::vector<equivlist> equivs;

            equivs.reserve(depth);
            for(it_it i=itlist.begin(); i!=itlist.end(); ++i)
            {
                vec_it pos = *i;
                
                std::vector<std::string> eyes(width);
                for(size_t w=0; w<width; ++w)
                    eyes[w] = GetEye(patterns[w], *pos++);
                
                std::set<equivalence> equivalences;
                for(size_t a=0; a<width; ++a)
                    for(size_t b=a+1; b<width; ++b)
                        if(eyes[a] == eyes[b])
                            equivalences.insert(std::make_pair(a, b));
                equivs.push_back(equivalences);
            }

            for(equivlist::const_iterator i = equivs[0].begin();
                                          i != equivs[0].end();
                                          ++i)
            {
                for(size_t a=1; a<equivs.size(); ++a)
                    if(equivs[a].find(*i) == equivs[a].end())
                        goto NotOk;
                eqall.insert(*i);
            NotOk: ;
            }
        }
    };
    
    size_t bestscore=0;
    result bestresult;
    
    for(unsigned macrosize = 4; macrosize >= 1; --macrosize)
    {
        // Try all positions
        for(vec_it i = data.begin(); i != data.end(); )
        {
            vec_it beginpos = i;
            std::list<vec_it> itset;
            
            std::vector<std::string> patterns;
            patterns.reserve(macrosize);
            for(unsigned n=0; n<macrosize; ++n)
            {
                if(i == data.end()) break;
                itset.push_back(i);
                patterns.push_back(*i++);
            }
            if(patterns.size() != macrosize) break;
            
            std::list<vec_it> itlist;
            itlist.push_back(beginpos);
            
            // Now try all begin positions
            for(vec_it begin = data.begin(); begin != data.end(); ++begin)
            {
                //std::cerr << "--new begin--\n";
                for(vec_it j = begin; j != data.end(); )
                {
                    vec_it pos = j, pos0 = beginpos;
                    
                    std::vector<std::string> newpats = patterns;
                    unsigned n=0;
                    for(; n<macrosize; ++n)
                    {
                        if(pos == data.end()) break;
                        
                        for(it_it k=itset.begin(); k!=itset.end(); ++k)
                            if(*k == pos) goto Fail;
                        
                        ExpandPattern(newpats[n], *pos0++, *pos++);
                    }
                    if(n != macrosize) { Fail: ++j; continue; }
                    
                    itlist.push_back(j);
                    if(TestAcceptPatterns(newpats, itlist))
                    {
                        //std::cerr << "ok\n";
                        for(n=0; n<macrosize; ++n)
                            itset.push_back(j++);
                    }
                    else
                    {
                        itlist.pop_back();
                        ++j;
                    }
                }
                
                /*
                if(itlist.size() == 1)
                {
                    // No good
                    continue;
                }
                */
                break;
            }
            if(itlist.size() > 1)
            {
                size_t depth = itlist.size();
                size_t width = macrosize;
                
                size_t score = depth*width;
                
                if(score > bestscore)
                {
                    bestresult.itlist = itlist;
                    bestresult.width  = width;
                    bestscore = score;
                }
            }

            i = beginpos; ++i;
        }
    }
    
    if(bestresult.width > 1)
    {
    /*
        std::cerr << bestresult.width
                  << ", score " << bestscore
                  << " accepted- good enough?\n";
    */
        bestresult.BuildEquivalences();
        bestresult.Modify(data);
    }
}

#if 0
int main(void)
{
    static const char *const lines[] =
    {
        "sep #$20",
        "lda #$1",
        "jsr CharNumber",
        "sta $1,s",
        "lda #$2",
        "jsr CharNumber",
        "sta $2,s",
        "lda #$3",
        "jsr CharNumber",
        "sta $3,s",
        "lda #$4",
        "jsr CharNumber",
        "sta $4,s",
        "koe",
        "lda #$5",
        "jsr CharNumber",
        "sta $5,s",
        "rts",
        NULL
    };
    codevec_t data;
    const char *const *s = lines;
    while(*s) data.push_back(*s++);
    
    FindPatterns(data);
    
    for(vec_it i = data.begin(); i != data.end(); ++i)
        std::cout << *i << std::endl;

    return 0;
};
#endif
