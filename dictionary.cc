#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "ctinsert.hh"
#include "ctcset.hh"
#include "miscfun.hh"
#include "config.hh"
#include "hash.hh"
#include "logfiles.hh"
#include "conjugate.hh"
#include "msginsert.hh"
#include "pageptrlist.hh"
#include "settings.hh"

using namespace std;

#define APPLYD_DUMP       0
#define CAREFUL_TESTING   1

#define PRELOAD_DICTIONARY_TEST 0

/*
Jos selvitet‰‰n N kerralla:
  Kuinka monta erilaista N stringin yhdistelm‰‰ saadaan 1200000:sta...
Valtava m‰‰r‰.
  
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
    public:
        PageScript(): pagenum(), urgency(), script() { }
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
           : data()
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
    public:
        Finder(): rep_word(), dict_word(),
                  was_extended(), was_previously_effectless(),
                  begintime()
        { }
    private:
        hash_set<ctstring> effectless;

        bool WasEffectless(const ctstring& dictword) const
        {
            return effectless.find(dictword) != effectless.end();
        }
        
        size_t CalcSaving(const ctstring& word, const ctstring& where) const
        {
            size_t saving = 0;
            size_t saved_length = CalcSize(word);
            for(size_t a=0; a<where.size(); )
            {
                size_t b = where.find(word, a);
                
                if(b == where.npos)break;
                
                saving += saved_length - 1;

                a = b + word.size();
            }
            return saving;
        }
        size_t CalcSaving(const ctstring& word, const PageScriptList& pages) const
        {
            size_t saving = 0;
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
            
            const ctchar spacechar = getctchar(' ', cset_12pix);
            
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
                str_replace_inplace(rep_word, dict[d], replacement);
            }
            return rep_word;
        }
        
    public:
        void Do(const PageScriptList& uncompressed_pages,
                const PageScriptList& compressed_pages,
                //const double pos, const double scale,
                const vector<ctstring>& dict,
                const Conjugatemap* const Conjugater
               )
        {
            FILE *log = GetLogFile("dictionary", "outputfn");
            
            const bool UseImproveTest = GetConf("dictionary", "use_improve_test");
            
            const unsigned dictend   = get_font_begin();
            
            // extrachars(0x100-0x2FF) ARE allowed in dictionary keys.
            
            const ctchar spacechar = getctchar(' ', cset_12pix);
            
            fprintf(stderr, " - time used: ");
            for(unsigned a=fprintf(stderr, "%15s",""); a-->0; )putc(8, stderr);

            substringtable substrings;
            
            //unsigned pageno=0, pagecount=compressed_pages.size();
            
            /* Find substrings from the compressed script */
            for(PageScriptList::const_iterator
                i = compressed_pages.begin();
                i != compressed_pages.end();
                ++i)
            {
                //double totalpos = (pos + pageno++/(double)pagecount) / scale;
                //if(totalpos==0.0)totalpos=1e-10;
                
                unsigned diffsec = (unsigned)(difftime(time(NULL), begintime));
                // *(1.0/totalpos - 1));
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
            
            fprintf(stderr, "\r>%7u substrings; ", substrings.size());
            
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
                    
                    for(size_t a=0;;)
                    {
                        size_t b = script.find(dictword, a);
                        if(b == script.npos) break;
                        
                        //fprintf(stderr, ".");
                    
                        const size_t maxbeg = 3;
                        const size_t maxend = 3;
                    
                        bool begspace = dictword[0] == spacechar;
                        
                        for(size_t beglen=0; ; ++beglen)
                        {
                            if(beglen > b) break;
                            
                            size_t begin  = b-beglen;
                            size_t maxlen = script.size() - (begin + dictword.size());
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
                            
                            size_t length = dictword.size() + beglen;
                            for(size_t endlen=0; ; ++endlen, ++length)
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
                for(size_t d=0; d<dict.size(); ++d)
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
                        for(size_t d=0; d<dict.size(); ++d)
                        {
                            ctchar replacement = dictbegin + d;
                            str_replace_inplace(comprword, dict[d], replacement);
                        }
                        // Each instance of the string
                        size_t oldbytes = CalcSize(comprword);
        #else
                        // Calculate saving from the uncompressed form
                        size_t oldbytes = CalcSize(word);
        #endif
                        // Will be replaced by one byte
                        size_t newbytes = 1;
                        size_t saving = oldbytes - newbytes;
                        
                        double realscore = saving * j->second;

                        if(word == dictword)
                            goodscore = realscore;
                        
                        if(realscore >= bestscore)
                        {
                            bool exists = false;

                            if(word != dictword)
                            {
        #if 0
                                for(size_t a=0; a<dict.size(); ++a)
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
                                for(size_t a=0; a<dict.size(); ++a)
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
                        fprintf(log, ";No neostrings\n");
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
            }
            
            was_previously_effectless = WasEffectless(dict_word);
        }
    };
}

/*
#include <cstdarg>
static void PreloadDict(vector<ctstring>& dict,
   const wchar_t *arg, ...)
{
    va_list ap;
    va_start(ap, arg);
    
    while(arg)
    {
        dict.push_back(getctstring(arg));
        
        arg = va_arg(ap, const wchar_t *);
    }
    va_end(ap);
}
*/

void insertor::RebuildDictionary()
{
    const bool UseRedefine  = GetConf("dictionary", "use_redefine");
    const bool ManyRestarts = GetConf("dictionary", "restart_many_times");
    const bool UseQuickRegen = GetConf("dictionary", "use_quick_regen");

    FILE *log = GetLogFile("dictionary", "outputfn");

    time_t begintime = time(NULL);
    fprintf(stderr,
        "Rebuilding the dictionary. This will take probably a long time!\n"
        "> You should take a lunch break or something now.\n"
    );
    
    fprintf(stderr, "> Building script for compression tests... ");
    
    PageScriptList pages, saved_pages;

    if(true)
    {
        list<pair<unsigned, ctstring> > tmp = GetScriptByPage();
        for(list<pair<unsigned, ctstring> >::const_iterator
            i = tmp.begin(); i != tmp.end(); ++i)
        {
            PageScript tmp;
            
            tmp.pagenum   = i->first;
            tmp.script    = i->second;
            
            //tmp.urgency   = 1.0 + (65536 - freespace.Size(tmp.pagenum)) / 65536.0;
            //tmp.urgency   = 1.0 / freespace.Size(tmp.pagenum);
            tmp.urgency   = 1;
            
            //tmp.urgency *= freespace.Count(tmp.pagenum);

            pages.push_back(tmp);
        }
    }
    
    saved_pages = pages;
    
    fprintf(stderr, "%u bytes.\n", CalcScriptSize(pages));
    
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
L"t‰ ", L"it‰ ",
L"kun",L"min",L": H",L"is ",L"'s ",L"Sateenkaarikuor",L"et ",L"you ",
L": M",L"Schal",L"Lavo",L"ing ",L"Kuningas",L": T",L"ell",L"the ",
L"‰n ",L"   Kyll‰.",L"Melchior",L"Kansleri: ",L"ta ",
L"uningatar",L"an ",L"en ",L"on ",L"in ",
NULL);
    num_restarts = dict.size();
    PreloadDict(dict, 
L"      ",  L"e ",L"ta",L"in",L"a ",L": ",L"is",L", ",L"en",L"t ",L"i ",
L"an",L"‰ ",L"ll",L"ka",L"er",L"it",L"t‰",L"   ",L"va",L"on",L"et",
L"as",L"o ",L"es",L"tu",L"te",L"ik",L"al",L"ol",L"us",L"ut",L"ha",L"ma",
L"el",L"or",L"ti",L"ar",L"si",L"ou",L"mm",L"sa",L"os",L"‰n",L"un",L"ku",L"d ",
L"ki",L"ko",L"y ",L"at",L"k‰",L"ks",L"v‰",L"th",L"to",L"ei",L"vi",L"vo",L"yt",
L"a!",L"jo",L"s ",L"le",L"ai",L"om",L"ur",L"ni",L"of ",L"‰‰",L"Ozzie",L"ja",
L"!!",L"ys",L"li",L"ne",L"ow",L"mi",L"pa",L"ke",
NULL);
    reapply = true;
#endif

    vector<ctstring> quick_regen;
    hash_map<ctstring, unsigned> latest_gains;
    
    if(log)
    {
        fprintf(log,
            ";------------\n"
            ";The dictionary is found at the end of this file.\n"
            ";If you are satisfied with it, you can copypaste\n"
            ";it to your script file, replacing the old dictionary.\n"
            ";------------\n"
               );
    }
    
    while(dict.size() < dict_fullsize)
    {
        fprintf(stderr, "Finding substrs");
        
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
                    str_replace_inplace(rep_word, dict[a], replacement);
                }
                ctchar replacement = dictbegin + d;
                
                unsigned saving = 0;
                PageScriptList::iterator i;
                for(i=pages.begin(); i!=pages.end(); ++i)
                {
                    unsigned size0 = CalcSize(i->script);
                    str_replace_inplace(i->script, rep_word, replacement);
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
                    str_replace_inplace(i->script, rep_word, replacement);
                    const unsigned size1 = CalcSize(i->script);
                    saving += size0 - size1;
                }
                
                if(saving < latest_gains[dictword])
                {
                    pages = backup;
                    
                    testee = dictword;
                    
                    fprintf(log,
                        ";Discarding '%s', saving(%u) doesn't match previous(%u)\n",
                        DispString(dictword).c_str(),
                        saving, latest_gains[dictword]);
                    
                    quick_regen.erase(quick_regen.begin(),
                                      quick_regen.begin() + a + 1);
                    break;
                }

                if(log)
                    fprintf(log, ";Reapplying:%s;saving: %u bytes\n",
                        DispString(dictword).c_str(), saving);

                dict.push_back(dictword);
                latest_gains[dictword] = saving;
            }
        
        fprintf(stderr, "/%u bytes; %u/%u", (unsigned)CalcScriptSize(pages), (unsigned)dict.size(), (unsigned)dict_fullsize);

        Finder.Do(saved_pages, pages,
                  //dict.size(), dict_fullsize,
                  dict, Conjugater);
        
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
        
        /*
        printf(";Generated:%s;%u bytes\n",
            DispString(dictword).c_str(),
            CalcSize(rep_word));
        */
        
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

                    if(log)
                        fprintf(log, ";Restarted the work at %u ($%02X)\n",
                                     restartpos, restartpos+dictbegin);
                    
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
                str_replace_inplace(i->script, rep_word, replacement);
                const unsigned size1 = CalcSize(i->script);
                saving += size0 - size1;
            }
            
            if(log)
                fprintf(log, ";Applying:%s;saving: %u bytes\n",
                    DispString(dictword).c_str(), saving);

            latest_gains[dictword] = saving;
            dict.push_back(dictword);
        }
        /*
        for(unsigned d=0; d<dict.size(); ++d)
        {
            printf("$%02X:%s;\n",
                dictbegin+d,
                DispString(dict[d]).c_str());
        }
        fflush(stdout);
        */
    }

    fprintf(stderr,
        "\r> %-56s\n",
        "Dictionary generated. Applying it first, then saving...");
}

void insertor::ApplyDictionary()
{
    const bool sort_dictionary = GetConf("dictionary", "sort_before_apply");

    const unsigned dictbegin = 0x21;

    MessageApplyingDictionary();

    if(sort_dictionary)
        sort(dict.begin(), dict.end(), dictsorter);
#if APPLYD_DUMP
    unsigned col=0;
#endif
    for(unsigned d=0; d<dict.size(); ++d)
    {
        MessageWorking();
        const ctstring &dictword = dict[d];
        
        ctstring rep_word = dictword;
        for(unsigned a=0; a<d; ++a)
        {
            ctchar replacement = dictbegin + a;
            str_replace_inplace(rep_word, dict[a], replacement);
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
            switch(i->type)
            {
                /* Only zptr12 strings are compressed */
                case stringdata::zptr12:
                    str_replace_inplace(i->str, rep_word, replacement);
                    break;
                default:
                    continue;
            }
        }
    }
#if APPLYD_DUMP
    if(col)putc('\n', stderr);
#endif
    MessageDone();
}

/* Build the dictionary. */
void insertor::DictionaryCompress()
{
    const bool rebuild_dict = GetConf("dictionary", "rebuild");
    
    const unsigned dictbegin = 0x21;
    const unsigned dictend   = get_font_begin();

    const size_t dict_fullsize = dictend - dictbegin;
    
    if(dict_fullsize != dict.size() && !rebuild_dict)
    {
        fprintf(stderr,
            "Warning: Dictionary size should be %u, not %u.\n"
            "         Please rebuild dictionary!\n",
            (unsigned) dict_fullsize,
            (unsigned) dict.size());
    }
    
    size_t origsize = CalculateScriptSize();

    if(rebuild_dict) RebuildDictionary();
    
    const bool apply_dictionary = GetConf("dictionary", "apply");
    if(apply_dictionary)
    {
        ApplyDictionary();
    }

    size_t resultsize = CalculateScriptSize();

    size_t dictbytes = 0;
    for(size_t a=0; a<dict.size(); ++a)dictbytes += CalcSize(dict[a]) + 1;
    
    if(apply_dictionary)
    {
        fprintf(stderr, "> Original script size: %u bytes; new script size: %u bytes\n"
                        "> Saved: %u bytes (%.1f%% off); dictionary size: %u bytes\n",
            (unsigned) origsize,
            (unsigned) resultsize,
            (unsigned) (origsize-resultsize),
            (origsize-resultsize)*100.0/origsize,
            (unsigned) dictbytes);
    }

    if(rebuild_dict)
    {
        if(!apply_dictionary) resultsize = CalculateScriptSize();
        
        FILE *log = GetLogFile("dictionary", "outputfn");

        if(log)
        {
            fprintf(log,
                "\n"
                "\n"
                "*d;dictionary\n"
                ";Original script size: %u bytes; new script size: %u bytes\n"
                ";Saved: %u bytes (%.1f%% off); dictionary size: %u bytes\n"
                ";-----------------\n"
                ";dictionary, used for compression. don't try to translate it.\n"
                ";-----------------\n",
                (unsigned) origsize,
                (unsigned) resultsize,
                (unsigned) (origsize-resultsize),
                (origsize-resultsize)*100.0/origsize,
                (unsigned) dictbytes
                   );

            for(size_t d=0; d<dict.size(); ++d)
            {
                fprintf(log, "$%02X:%s;\n",
                    (unsigned)(dictbegin+d),
                    DispString(dict[d]).c_str());
            }
            fflush(log);
            
            fprintf(stderr,
                "Dictionary saved.\n"
                "* If you want to reuse the generated dictionary, you can copypaste\n"
                "* it from the log file to your script file.\n");
        }
        else
        {
            fprintf(stderr,
                "* The generated dictionary was not saved in a file.\n"
                "* This is because you had \"outputfn\" setting empty.\n");
        }
    }
}

void insertor::WriteDictionary()
{
    MessageWritingDict();

    PagePtrList tmp;
    
    for(size_t a=0; a<dict.size(); ++a)
    {
        const string s = GetString(dict[a]);
        vector<unsigned char> Buf(s.size() + 1);
        std::copy(s.begin(), s.begin()+s.size(), Buf.begin()+1);
        Buf[0] = s.size();
        
        tmp.AddItem(Buf, a*2);
    }
    
    tmp.Create(*this, "dict", "DICT_TABLE");
    
    objects.AddReference("DICT_TABLE", OffsPtrFrom(GetConst(DICT_OFFSET)));
    objects.AddReference("DICT_TABLE", PagePtrFrom(GetConst(DICT_SEGMENT1)));
    objects.AddReference("DICT_TABLE", PagePtrFrom(GetConst(DICT_SEGMENT2)));
    
    MessageDone();
}
