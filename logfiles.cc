#include <map>
#include <cstdlib>
#include <utility>

#include "logfiles.hh"
#include "wstring.hh"
#include "config.hh"

using std::map;
using std::pair;
using std::perror;
using std::fopen;

typedef pair<string, string> setup_t;
typedef map<setup_t, FILE *> handles;

static class Logs
{
    handles data;
public:
    Logs()
    {
    }
    ~Logs()
    {
        handles::const_iterator i;
        for(i=data.begin(); i!=data.end(); ++i)
            fclose(i->second);
    }
    FILE *Get(const char *sect, const char *key)
    {
        setup_t setup(sect, key);
        handles::const_iterator i = data.find(setup);
        if(i != data.end()) return i->second;
        
        FILE *result = 0;
        wstring fn = GetConf(sect, key);
        if(!fn.empty())
        {
            string fn2 = WstrToAsc(fn);
            result = fopen(fn2.c_str(), "wt");
            if(!result)
                perror(fn2.c_str());
            else
                setbuf(result, NULL);
        }
        return data[setup] = result;
    }
} Logfiles;

FILE *GetLogFile(const char *sect, const char *key)
{
    return Logfiles.Get(sect, key);
}
