#include <cstdio>

#include "ctcset.hh"
#include "wstring.hh"
#include "confparser.hh"

ConfParser config;

const ConfParser::Field& GetConf(const char *sect, const char *var)
{
    return config[sect][var];
}

namespace
{
    /* Can't be embedded as static in GetConf,
     * because "characterset" must be loaded in start.
     */
    class ConfigLoader
    {
    public:
        ConfigLoader()
        {
            FILE *fp = fopen("ct.cfg", "rt");
            char Buf[8192];
            setbuffer(fp,Buf,sizeof Buf);

            config.Parse(fp);

            ucs4string cset = GetConf("general", "characterset");
            setcharset(WstrToAsc(cset).c_str());

            rewind(fp);
            clearerr(fp);
            
            config.Clear();
            config.Parse(fp);
            
            fclose(fp);
            
            fprintf(stderr, "Configuration loaded\n");
        }
    } ConfigLoader;
}
