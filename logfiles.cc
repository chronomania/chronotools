#include <map>
#include <cstdlib>
#include <utility>

#include "logfiles.hh"
#include "wstring.hh"
#include "config.hh"

typedef std::pair<std::string, std::string> setup_t;
typedef std::map<setup_t, std::FILE *> handles;

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
            std::fclose(i->second);
    }
    FILE *Get(const char *sect, const char *key)
    {
        setup_t setup(sect, key);
        handles::const_iterator i = data.find(setup);
        if(i != data.end()) return i->second;
        
        std::FILE *result = 0;
        std::wstring fn = GetConf(sect, key);
        if(!fn.empty())
        {
            std::string fn2 = WstrToAsc(fn);
            result = std::fopen(fn2.c_str(), "wt");
            if(!result)
                std::perror(fn2.c_str());
            else
                std::setbuf(result, NULL);
        }
        return data[setup] = result;
    }
} Logfiles;

std::FILE *GetLogFile(const char *sect, const char *key)
{
    return Logfiles.Get(sect, key);
}
