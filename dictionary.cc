#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "ctinsert.hh"
#include "ctcset.hh"
#include "miscfun.hh"
#include "config.hh"
#include "stringoffs.hh"
#include "hash.hh"
#include "logfiles.hh"
#include "conjugate.hh"

using namespace std;

#define APPLYD_DUMP       0
#define CAREFUL_TESTING   1

#define PRELOAD_DICTIONARY_TEST 0

/*
Jos selvitet��n N kerralla:
  Kuinka monta erilaista N stringin yhdistelm�� saadaan 1200000:sta...
Valtava m��r�.
  
*/

namespace
{
    bool dictsorter(const ctstring &a, const ctstring &b)
    {
        // Superstrings should be placed first!
        if(a.find(b) != a.npos) return true;
        if(b.find(a) != b.npos) return false;
        
        if(a.size() > b.size())return true;
        if(a.size() < b.size())return false;
        
        return a < b;
    }

    struct PageScript
    {
        unsigned pagenum;
        double urgency;
        ctstring script;
    };

    typedef list<PageScript> PageScriptList;

    unsigned CalcScriptSize(const PageScriptList& tmp)
    {
        unsigned size = 0;
        PageScriptList::const_iterator i;
        for(i = tmp.begin(); i != tmp.end(); ++i)
            size += CalcSize(i->script);
        return size;
    }

    class substringtable
    {
        typedef hash_map<ctstring, double> data_t;
#if CAREFUL_TESTING
        typedef hash_map<ctstring, unsigned> lastpos_t;
        lastpos_t positions[0x40];
#endif
        data_t data;

    public:
        substringtable()
        {
        }

        void Collect(const PageScript& page,
                     unsigned pos, unsigned len)
        {
            const ctstring& script = page.script;
            const ctstring str = script.substr(pos, len);
            
#if CAREFUL_TESTING
            bool needs_testing = false;
            for(unsigned b=1; b<str.size(); ++b)
            {
                unsigned lenmax = str.size()-b;
                // If the string has a start for itself here
                if(str.compare(b, lenmax, str, 0, lenmax) == 0)
                {
                    needs_testing = true;
                    break;
                }
            }
            if(needs_testing)
            {
                /*
                     With this,
                       "     " won't give '   ' three times!
                     This gives more rightful results.
                */
                lastpos_t& posmap = positions[page.pagenum];
                lastpos_t::const_iterator i = posmap.find(str);
                
                if(i != posmap.end() && pos < i->second)  return;
                
                posmap[str] = pos + len;
            }
#endif
            data[str] += page.urgency;
        }
        
        void Finish()
        {
        }
        
        typedef data_t::iterator iterator;
        typedef data_t::const_iterator const_iterator;
        
        unsigned size() const { return data.size(); }
        
        const iterator begin() { return data.begin(); }
        const iterator end()   { return data.end(); }
        const const_iterator begin() const { return data.begin(); }
        const const_iterator end() const   { return data.end(); }
        
        void erase(iterator i) { data.erase(i); }
        
        void clear()
        {
            data.clear();
#if CAREFUL_TESTING
            for(unsigned a=0; a<0x40; ++a) positions[a].clear();
#endif
        }
    };

    struct Finder
    {
    public: // output
        ctstring rep_word;
        ctstring dict_word;
        bool was_extended;
        bool was_previously_effectless;
    public: // input
        time_t begintime;
    private:
        hash_set<ctstring> effectless;

        bool WasEffectless(const ctstring& dictword) const
        {
            return effectless.find(dictword) != effectless.end();
        }
        
        unsigned CalcSaving(const ctstring& word, const ctstring& where) const
        {
            unsigned saving = 0;
            unsigned saved_length = CalcSize(word);
            for(unsigned a=0; a<where.size(); )
            {
                unsigned b = where.find(word, a);
                
                if(b == where.npos)break;
                
                saving += saved_length - 1;

                a = b + word.size();
            }
            return saving;
        }
        unsigned CalcSaving(const ctstring& word, const PageScriptList& pages) const
        {
            unsigned saving = 0;
            for(PageScriptList::const_iterator i=pages.begin(); i!=pages.end(); ++i)
                saving += CalcSaving(word, i->script);
            return saving;
        }

        void LoadSubstrings(substringtable& substrings,
                            const PageScript& page,
                            const ctstring& script,
                            const Conjugatemap* const Conjugater
                           )
        {
            static const unsigned MinWordLen       = GetConf("dictionary", "min_word_length");
            static const unsigned MaxWordLen       = GetConf("dictionary", "max_word_length");
            static const unsigned MaxReuseCount    = GetConf("dictionary", "max_reuse_count");
            static const unsigned MaxSpacesPerWord = GetConf("dictionary", "max_spaces_per_word");
            
            const unsigned dictbegin = 0x21;
            const unsigned dictend   = get_font_begin();
            
            const ctchar spacechar = getchronochar(' ', cset_12pix);
            
            /* Substrings start from each position in the string */
            for(unsigned begin=0; begin<script.size(); ++begin)
            {
                unsigned num_reuses = 0;
                unsigned num_middlespaces = 0;
                unsigned num_endspaces    = 0;
                bool has_nonspace = false;
                
                unsigned maxpos = begin + MaxWordLen;
                if(maxpos > script.size()) maxpos = script.size();
                
                // c=string length, b=position
                for(unsigned length=0, pos=begin; pos<maxpos; ++pos)
                {
                    ctchar ch = script[pos];
                    /* Dictionary words can't contain [nl] etc */
                    if(ch < dictbegin) break;
                    
                    if(ch < dictend)
                    {
                        // Dictionary keys
                        if(++num_reuses > MaxReuseCount)break;
                        has_nonspace = true;
                    }
                    else
                    {
                        // Don't allow conjugate-codes in dictionary keys
                        if(Conjugater->IsConjChar(ch)) break;
                        
                        if(ch == spacechar)
                        {
                            if(has_nonspace)
                            {
                                // Assume we're at the end.
                                ++num_endspaces;
                            }
                            else
                            {
                                // Beginning of the word, ignore spaces
                            }
                        }
                        else
                        {
                            has_nonspace = true;
                            if(num_endspaces)
                            {
                                num_middlespaces += num_endspaces;
                                if(num_middlespaces > MaxSpacesPerWord)break;
                                num_endspaces = 0;
                            }
                        }
                    }
                    
                    if(ch >= 0x100) length += 2; else ++length;
                    
                    if(length >= MinWordLen)
                    {
                        /* Cumulate the substring usage counter */
                        substrings.Collect(page, begin, pos-begin+1);
                    }
                }
            }
        }
        
    public:
        void MarkAsEffectless(const ctstring& dictword)
        {
            effectless.insert(dictword);
        }

        
        const ctstring MakeDictWord(const ctstring& rep_word,
                                    const vector<ctstring>& dict)
        {
            const unsigned dictbegin = 0x21;
            const unsigned dictend   = get_font_begin();
            
            /* dictword = the string to be displayed when the byte is met */
            ctstring dictword;
            for(unsigned a=0; a<rep_word.size(); ++a)
            {
                ctchar c = rep_word[a];
                if(c >= dictbegin && c < dictend)
                    dictword += dict[c-dictbegin];
                else
                    dictword += c;
            }
            return dictword;
        }
        
        const ctstring MakeReplaceWord(const ctstring& dictword,
                                       const vector<ctstring>& dict)
        {
            const unsigned dictbegin = 0x21;
            
            /* dictword = the string to be displayed when the byte is met */
            ctstring rep_word = dictword;
            for(unsigned d=0; d<dict.size(); ++d)
            {
                ctchar replacement = dictbegin + d;
                rep_word = str_replace(dict[d], replacement, rep_word);
            }
            return rep_word;
        }
        
    public:
        void Do(const PageScriptList& uncompressed_pages,
                const PageScriptList& compressed_pages,
                const double pos, const double scale,
                const vector<ctstring>& dict,
                const Conjugatemap* const Conjugater
               )
        {
            FILE *log = GetLogFile("dictionary", "outputfn");
            
            const bool UseImproveTest = GetConf("dictionary", "use_improve_test");
            
            const unsigned dictend   = get_font_begin();
            
            // extrachars(0x100-0x2FF) ARE allowed in dictionary keys.
            
            const ctchar spacechar = getchronochar(' ', cset_12pix);
            
            fprintf(stderr, " - time left: ");
            for(unsigned a=fprintf(stderr, "%15s",""); a-->0; )putc(8, stderr);

            substringtable substrings;
            
            unsigned pageno=0, pagecount=compressed_pages.size();
            
            /* Find substrings from the compressed script */
            for(PageScriptList::const_iterator
                i = compressed_pages.begin();
                i != compressed_pages.end();
                ++i)
            {
                double totalpos = (pos + pageno++/(double)pagecount) / scale;
                if(totalpos==0.0)totalpos=1e-10;
                
                unsigned diffsec = (unsigned)(difftime(time(NULL),begintime)*(1.0/totalpos - 1));
                for(unsigned a=fprintf(stderr,
                      "%02d:%02d:%02d",
                          diffsec/3600,
                          (diffsec/60)%60,
                          diffsec%60);
                    a-->0; )putc(8, stderr);
            
                const PageScript& page = *i;
                const ctstring& script = i->script;
                
                LoadSubstrings(substrings, page, script, Conjugater);
            }
            
            fprintf(stderr, "\r%8u substrings; ", substrings.size());
            
            ctstring bestword;
            
            /* Now find the substring that has the biggest score */
            if(true)
            {
                substringtable::const_iterator j;
                substringtable::const_iterator bestj = substrings.end();
                
                double bestscore = -1;
                for(j = substrings.begin();
                    j != substrings.end();
                    ++j)
                {
                    const ctstring &word = j->first;

                    // Each instance of the string
                    unsigned oldbytes = CalcSize(word);
                    // Will be replaced by one byte
                    unsigned newbytes = 1;
                    unsigned saving = oldbytes - newbytes;
                    double realscore = saving * j->second;

                    if(realscore > bestscore)
                    {
                        bestj     = j;
                        bestscore = realscore;
                    }
                }
            
                /* bestword = the substring to be replaced with one byte */
                bestword = rep_word = bestj->first;
            }
            
            /* Make it plain text */
            const ctstring dictword = MakeDictWord(bestword, dict);
            
            double bestscore = 0, goodscore = 0;

            if(UseImproveTest)
            {
                fprintf(stderr, "testing '%s'", DispString(dictword).c_str());
                
                substrings.clear();
                
                /* Find substrings from the plaintext script */
                for(PageScriptList::const_iterator
                    i = uncompressed_pages.begin();
                    i != uncompressed_pages.end();
                    ++i)
                {
                    const PageScript& page = *i;
                    const ctstring& script = i->script;
                    
                    for(unsigned a=0;;)
                    {
                        unsigned b = script.find(dictword, a);
                        if(b == script.npos) break;
                        
                        //fprintf(stderr, ".");
                    
                        const unsigned maxbeg = 3;
                        const unsigned maxend = 3;
                    
                        bool begspace = dictword[0] == spacechar;
                        
                        for(unsigned beglen=0; ; ++beglen)
                        {
                            if(beglen > b) break;
                            
                            unsigned begin  = b-beglen;
                            unsigned maxlen = script.size() - (begin + dictword.size());
                            if(maxlen > maxend) maxlen = maxend;
                            
                            if(beglen > 0)
                            {
                                const ctchar ch = script[b-beglen];
                                if(ch < dictend) break;
                                
                                if(Conjugater->IsConjChar(ch)) break;
                                if(ch == spacechar) begspace = true;
                            }
                            
                            if(begspace && beglen >= maxbeg) break;
                            
                            bool endspace = dictword[dictword.size()-1] == spacechar;
                            
                            unsigned length = dictword.size() + beglen;
                            for(unsigned endlen=0; ; ++endlen, ++length)
                            {
                                if(endlen > 0)
                                {
                                    const ctchar ch = script[b+dictword.size()+endlen-1];
                                    if(ch < dictend) break;
                                    if(Conjugater->IsConjChar(ch)) break;
                                    if(ch == spacechar) endspace = true;
                                }
                                
                                if(endspace && endlen >= maxend) break;
                                
                                /* Cumulate the substring usage counter */
                                substrings.Collect(page, begin, length);
                            }
                        }
                        a = b + 1;
                    }
                }
                
                fprintf(stderr, "\r%8u neostrings; ", substrings.size());
                
                list<ctstring> rejected;

    #if 1
                // Don't allow keys that are just substrings of previous dict words
                for(unsigned d=0; d<dict.size(); ++d)
                    rejected.push_back(dict[d]);
    #endif
                
                /* Now find again the substring that has the biggest score */
                double bestscore=-1;
                double goodscore=0;
                
                if(true)
                {
                    substringtable::const_iterator j;
                    substringtable::const_iterator bestj = substrings.end();
                    
                    for(j = substrings.begin();
                        j != substrings.end();
                        ++j)
                    {
                        const ctstring& word = j->first;

        #if 0
                        // Calculate saving from the compressed form
                        ctstring comprword = word;
                        for(unsigned d=0; d<dict.size(); ++d)
                        {
                            ctchar replacement = dictbegin + d;
                            comprword = str_replace(dict[d], replacement, comprword);
                        }
                        // Each instance of the string
                        unsigned oldbytes = CalcSize(comprword);
        #else
                        // Calculate saving from the uncompressed form
                        unsigned oldbytes = CalcSize(word);
        #endif
                        // Will be replaced by one byte
                        unsigned newbytes = 1;
                        unsigned saving = oldbytes - newbytes;
                        
                        double realscore = saving * j->second;

                        if(word == dictword)
                            goodscore = realscore;
                        
                        if(realscore >= bestscore)
                        {
                            bool exists = false;

                            if(word != dictword)
                            {
        #if 0
                                for(unsigned a=0; a<dict.size(); ++a)
                                    if(dict[a] == word)
                                    {
                                        exists = true;
                                        break;
                                    }
        #endif
        #if 1
                                if(!exists)
                                {
                                    list<ctstring>::const_iterator k;
                                    for(k=rejected.begin(); k!=rejected.end(); ++k)
                                        if(k->find(word) != k->npos)
                                        {
                                            exists = true;
                                            break;
                                        }
                                }
        #endif
        #if 0
                                for(unsigned a=0; a<dict.size(); ++a)
                                    if(dict[a].find(word) != dict[a].npos)
                                    {
                                        exists = true;
                                        break;
                                    }
        #endif
                            }
                            if(exists)
                                rejected.push_back(word);
                            else
                            {
                                bestj     = j;
                                bestscore = realscore;
                            }
                        }
                    }
                    if(bestj == substrings.end())
                    {
                        printf(";No neostrings\n");
                    }
                    else
                    {
                        rep_word  = bestj->first;
                    }
                }
            } /* UseImproveTest */

            dict_word = MakeDictWord(rep_word, dict);
            was_extended = dictword != dict_word;
            
            if(was_extended && log)
            {
                fprintf(log, ";Chose '%s'(%.2f) instead of '%s'(%.2f)\n",
                    DispString(dict_word).c_str(),
                    bestscore,
                    DispString(dictword).c_str(),
                    goodscore);
                printf(";Chose '%s'(%.2f) instead of '%s'(%.2f)\n",
                    DispString(dict_word).c_str(),
                    bestscore,
                    DispString(dictword).c_str(),
                    goodscore);
            }
            
            was_previously_effectless = WasEffectless(dict_word);
        }
    };
}

#include <cstdarg>
static void PreloadDict(vector<ctstring>& dict,
   const char *arg, ...)
{
    va_list ap;
    va_start(ap, arg);
    
    while(arg)
    {
        dict.push_back(getctstring(AscToWstr(arg)));
        
        arg = va_arg(ap, const char *);
    }
    va_end(ap);
}

void insertor::RebuildDictionary()
{
    const bool UseRedefine  = GetConf("dictionary", "use_redefine");
    const bool ManyRestarts = GetConf("dictionary", "restart_many_times");
    const bool UseQuickRegen = GetConf("dictionary", "use_quick_regen");

    FILE *log = GetLogFile("dictionary", "outputfn");

    time_t begintime = time(NULL);
    fprintf(stderr,
        "Rebuilding the dictionary. This will take probably a long time!\n"
        "You should take a lunch break or something now.\n"
    );
    
    fprintf(stderr, "Building script for compression tests...\n");
    
    PageScriptList pages, saved_pages;

    set<unsigned> zpages = GetZStringPageList();
    for(set<unsigned>::const_iterator i=zpages.begin(); i!=zpages.end(); ++i)
    {
        stringoffsmap pagestrings = GetZStringList(*i);
        
        pagestrings.GenerateNeederList();
        const stringoffsmap::neederlist_t &neederlist = pagestrings.neederlist;
        
        PageScript tmp;
        
        tmp.pagenum   = *i;
        //tmp.urgency   = 1.0 + (65536 - freespace.Size(tmp.pagenum)) / 65536.0;
        //tmp.urgency   = 1.0 / freespace.Size(tmp.pagenum);
        tmp.urgency   = 1;
        
        //tmp.urgency *= freespace.Count(tmp.pagenum);

        for(unsigned a=0; a<pagestrings.size(); ++a)
            if(neederlist.find(a) == neederlist.end())
            {
                tmp.script += pagestrings[a].str;
                tmp.script += (ctchar)0;
            }
        pages.push_back(tmp);
    }
    
    saved_pages = pages;
    
    fprintf(stderr, "Script to be used in compression tests: %u bytes\n",
        CalcScriptSize(pages));
    
    const unsigned dictbegin = 0x21;
    const unsigned dictend   = get_font_begin();

    const unsigned dict_fullsize = dictend - dictbegin;
    
    Finder Finder;
    Finder.begintime = begintime;
    
    dict.clear();
    
    unsigned num_restarts = 0;
    bool reapply = false;
    
#if PRELOAD_DICTIONARY_TEST
    PreloadDict(dict,
"t� ", "it� ",
"kun","min",": H","is ","'s ","Sateenkaarikuor","et ","you ",
": M","Schal","Lavo","ing ","Kuningas",": T","ell","the ",
"�n ","   Kyll�.","Melchior","Kansleri: ","ta ",
"uningatar","an ","en ","on ","in ",
NULL);
    num_restarts = dict.size();
    PreloadDict(dict, 
"      ","e ","ta","in","a ",": ","is",", ","en","t ","i ",
"an","� ","ll","ka","er","it","t�","   ","va","on","et",
"as","o ","es","tu","te","ik","al","ol","us","ut","ha","ma",
"el","or","ti","ar","si","ou","mm","sa","os","�n","un","ku","d ",
"ki","ko","y ","at","k�","ks","v�","th","to","ei","vi","vo","yt",
"a!","jo","s ","le","ai","om","ur","ni","of ","��","Ozzie","ja",
"!!","ys","li","ne","ow","mi","pa","ke",
NULL);
    reapply = true;
#endif

    vector<ctstring> quick_regen;
    hash_map<ctstring, unsigned> latest_gains;
    
    while(dict.size() < dict_fullsize)
    {
        fprintf(stderr, "Finding substrings");
        
        if(reapply)
        {
        ReReApply:
            pages = saved_pages;
            
            unsigned prev_prev_saving = 0;
            unsigned prev_saving      = 0;
            ctstring prev_word;
            
            for(unsigned d=0; d<dict.size(); ++d)
            {
                const ctstring &dictword = dict[d];
        #if 0
                printf(";Applying:%s\n", DispString(dictword).c_str());
        #endif
                ctstring rep_word = dictword;
                for(unsigned a=0; a<d; ++a)
                {
                    ctchar replacement = dictbegin + a;
                    rep_word = str_replace(dict[a], replacement, rep_word);
                }
                ctchar replacement = dictbegin + d;
                
                unsigned saving = 0;
                PageScriptList::iterator i;
                for(i=pages.begin(); i!=pages.end(); ++i)
                {
                    unsigned size0 = CalcSize(i->script);
                    i->script = str_replace(rep_word, replacement, i->script);
                    unsigned size1 = CalcSize(i->script);
                    saving += size0 - size1;
                }

                latest_gains[dictword] = saving;
                
                if(log)
                    fprintf(log, ";Applying:%s;saving: %u bytes\n",
                        DispString(dictword).c_str(), saving);

                if(saving <= CalcSize(dictword))
                {
                    Finder.MarkAsEffectless(dictword);
                    
                    printf(";Effectless dictionary word:%s;\n",
                        DispString(dictword).c_str());
                    if(log)
                    {
                        fprintf(log, ";Effectless dictionary word:%s;\n",
                            DispString(dictword).c_str());
                    }
                    
                    if(d < num_restarts) --num_restarts;
                    dict.erase(dict.begin() + d);
                    --d;
                }
                
                /* If this word gives way better saving than the previous word */
                if(d >= 2
                && prev_saving < saving/2
                && prev_saving < prev_prev_saving/2
                  )
                {
                    printf(";Effectless previous dictionary word:%s;\n",
                        DispString(prev_word).c_str());
                    if(log)
                    {
                        fprintf(log, ";Effectless previous dictionary word:%s;\n",
                            DispString(prev_word).c_str());
                    }
                    
                    --d;
                    if(d < num_restarts) --num_restarts;
                    Finder.MarkAsEffectless(dict[d]);
                    
                    dict.erase(dict.begin() + d);
                    goto ReReApply;
                }
                
                prev_prev_saving = prev_saving;
                prev_saving = saving;
                prev_word   = dictword;
            }
            reapply = false;
        }
        
        ctstring testee;
        
        if(UseQuickRegen)
            for(unsigned a=0; a<quick_regen.size(); ++a)
            {
                const ctstring& dictword = quick_regen[a];
                const ctstring rep_word = Finder.MakeReplaceWord(dictword, dict);

                ctchar replacement = dictbegin + dict.size();
                unsigned saving = 0;
                
                PageScriptList backup = pages;
                PageScriptList::iterator i;
                for(i=pages.begin(); i!=pages.end(); ++i)
                {
                    const unsigned size0 = CalcSize(i->script);
                    i->script = str_replace(rep_word, replacement, i->script);
                    const unsigned size1 = CalcSize(i->script);
                    saving += size0 - size1;
                }
                
                if(saving < latest_gains[dictword])
                {
                    pages = backup;
                    
                    testee = dictword;
                    
                    printf(";Discarding '%s', saving(%u) doesn't match previous(%u)\n",
                        DispString(dictword).c_str(),
                        saving, latest_gains[dictword]);
                    quick_regen.erase(quick_regen.begin(),
                                      quick_regen.begin() + a + 1);
                    break;
                }

                if(log)
                    fprintf(log, ";Reapplying:%s;saving: %u bytes\n",
                        DispString(dictword).c_str(), saving);

                printf(";Reapplying:%s;saving: %u bytes\n",
                    DispString(dictword).c_str(), saving);

                dict.push_back(dictword);
                latest_gains[dictword] = saving;
            }
        
        fprintf(stderr, "/%u bytes; %u/%u", CalcScriptSize(pages), dict.size(), dict_fullsize);

        Finder.Do(saved_pages, pages, dict.size(), dict_fullsize, dict, Conjugater);
        
        const ctstring& rep_word = Finder.rep_word;
        const ctstring& dictword = Finder.dict_word;
        
        if(!testee.empty())
        {
            if(dictword != testee)
            {
                quick_regen.clear();
            }
            testee.clear();
        }
        
        printf(";Generated:%s;%u bytes\n",
            DispString(dictword).c_str(),
            CalcSize(rep_word));
        
        if(UseRedefine)
        {
            for(unsigned d=1; d<dict.size(); ++d)
                if(dictword.find(dict[d]) != dictword.npos)
                {
                    printf(";Obsoleted:%s;\n", DispString(dict[d]).c_str());
                    if(log)
                        fprintf(log, ";Obsoleted:%s;\n", DispString(dict[d]).c_str());

                    if(ManyRestarts && d < num_restarts)
                    {
                        --num_restarts;
                    }
                    
                    dict.erase(dict.begin() + d);
                    --d;

                    reapply = true;
                }
        }
        
        if(Finder.was_extended)
            reapply = true;
        
        if(reapply)
        {
            if(log)
                fprintf(log, ";Generated:%s;%u bytes\n",
                    DispString(dictword).c_str(),
                    CalcSize(rep_word));
             
            if(Finder.was_previously_effectless)
            {
                dict.push_back(dictword);
            }
            else
            {
                dict.insert(dict.begin(), dictword);
            
                if(ManyRestarts)
                {
                    ++num_restarts;

                    const unsigned restartpos = num_restarts;

                    printf(";Restarted the work at $%02X\n", restartpos+dictbegin);
                    if(log) fprintf(log, ";Restarted the work at %u\n", restartpos);
                    
                    if(UseQuickRegen)
                    {
                        quick_regen.clear();
                        for(unsigned d=restartpos; d<dict.size(); ++d)
                            quick_regen.push_back(dict[d]);
                    }
                    
                    dict.erase(dict.begin() + restartpos, dict.end());
                }
            }
        }
        else
        {
            ctchar replacement = dictbegin + dict.size();
            
            unsigned saving = 0;

            PageScriptList::iterator i;
            for(i=pages.begin(); i!=pages.end(); ++i)
            {
                const unsigned size0 = CalcSize(i->script);
                i->script = str_replace(rep_word, replacement, i->script);
                const unsigned size1 = CalcSize(i->script);
                saving += size0 - size1;
            }
            
            if(log)
                fprintf(log, ";Applying:%s;saving: %u bytes\n",
                    DispString(dictword).c_str(), saving);

            latest_gains[dictword] = saving;
            dict.push_back(dictword);
        }
        
        for(unsigned d=0; d<dict.size(); ++d)
        {
            printf("$%02X:%s;\n",
                dictbegin+d,
                DispString(dict[d]).c_str());
        }
        fflush(stdout);
    }

    time_t endtime = time(NULL);
    fprintf(stderr,
        "\nDictionary generated successfully - took %.2f minutes.\n",
        difftime(endtime, begintime) / 60.0);
}

void insertor::ApplyDictionary()
{
    const bool sort_dictionary = GetConf("dictionary", "sort_before_apply");

    const unsigned dictbegin = 0x21;

    fprintf(stderr,
        "Applying dictionary. This will take some seconds at most.\n"
    );
    if(sort_dictionary)
        sort(dict.begin(), dict.end(), dictsorter);
#if APPLYD_DUMP
    unsigned col=0;
#endif
    for(unsigned d=0; d<dict.size(); ++d)
    {
        const ctstring &dictword = dict[d];
        
        ctstring rep_word = dictword;
        for(unsigned a=0; a<d; ++a)
        {
            ctchar replacement = dictbegin + a;
            rep_word = str_replace(dict[a], replacement, rep_word);
        }
        
#if APPLYD_DUMP
        fprintf(stderr, "'%s%*c",
            DispString(dictword).c_str(),
            dictword.size()-12, '\'');
        if(++col==6){col=0;putc('\n',stderr);}
#endif
        
        ctchar replacement = dictbegin + d;
        
        for(stringlist::iterator i=strings.begin(); i!=strings.end(); ++i)
        {
            if(i->type != stringdata::zptr12) continue;
            i->str = str_replace(rep_word, replacement, i->str);
        }
    }
#if APPLYD_DUMP
    if(col)putc('\n', stderr);
#endif
}

unsigned insertor::CalculateScriptSize() const
{
    set<unsigned> zpages = GetZStringPageList();
    unsigned size = 0;
    for(set<unsigned>::const_iterator i=zpages.begin(); i!=zpages.end(); ++i)
    {
        stringoffsmap pagestrings = GetZStringList(*i);
        
        pagestrings.GenerateNeederList();
        
        const stringoffsmap::neederlist_t &neederlist = pagestrings.neederlist;
        
        for(unsigned a=0; a<pagestrings.size(); ++a)
            if(neederlist.find(a) == neederlist.end())
                size += CalcSize(pagestrings[a].str) + 1;
    }
    return size;
}

/* Build the dictionary. */
void insertor::DictionaryCompress()
{
    const bool rebuild_dict = GetConf("dictionary", "rebuild");
    
    const unsigned dictbegin = 0x21;
    const unsigned dictend   = get_font_begin();

    const unsigned dict_fullsize = dictend - dictbegin;
    
    if(dict_fullsize != dict.size() && !rebuild_dict)
    {
        fprintf(stderr,
            "Warning: Dictionary size should be %u, not %u.\n"
            "         Please rebuild dictionary!\n",
            dict_fullsize, dict.size());
    }
    
    fprintf(stderr, "Calculating script size...\n");
    unsigned origsize = CalculateScriptSize();

    if(rebuild_dict) RebuildDictionary();

    ApplyDictionary();

    unsigned resultsize = CalculateScriptSize();

    unsigned dictbytes = 0;
    for(unsigned a=0; a<dict.size(); ++a)dictbytes += CalcSize(dict[a]) + 1;
    
    fprintf(stderr, "Original script size: %u bytes; new script size: %u bytes\n"
                    "Saved: %u bytes (%.1f%% off); dictionary size: %u bytes\n",
        origsize,
        resultsize,
        origsize-resultsize,
        (origsize-resultsize)*100.0/origsize,
        dictbytes);
}