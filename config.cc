#include <cstdio>
#include <cerrno>
#include <set>
#include <cstring>

#include "ctcset.hh"
#include "wstring.hh"
#include "confparser.hh"

namespace
{
    const char DefaultConfig[] =
"\n\
# A simple Chronotools configuration file.\n\
# Complete enough for only ctdump to work.\n\
[general]\n\
characterset=\"iso-8859-15\"\n\
romsize=32\n\
[dumper]\n\
font8fn=\"ct8fn.tga\"\n\
font12fn=\"ct16fn.tga\"\n\
log_map=\"ct_map.log\"\n\
log_ptr=\"ct_ptr.log\"\n\
attempt_unwrap=true\n\
[mem]\n\
log_addrs=\"ct_addr.log\"\n\
[font]\n\
begin=$A0\n\
font8fn=\"ct8fn.tga\"\n\
font12fn=\"ct16fn.tga\"\n\
nonchar=\"¶\"\n\
font12_00=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_10=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_20=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_30=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_40=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_50=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_60=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_70=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_80=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_90=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_A0=\"ABCDEFGHIJKLMNOP\"\n\
font12_B0=\"QRSTUVWXYZabcdef\"\n\
font12_C0=\"ghijklmnopqrstuv\"\n\
font12_D0=\"wxyz0123456789!?\"\n\
font12_E0=\"/«»:&()'.,=-+%# \"\n\
font12_F0=\"°¶##¶¶¶¶¶·¶¶¶¶¶_\"\n\
font12_100=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_110=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_120=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_130=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_140=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_150=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_160=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_170=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_180=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_190=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_1A0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_1B0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_1C0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_1D0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_1E0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_1F0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_200=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_210=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_220=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_230=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_240=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_250=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_260=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_270=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_280=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_290=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_2A0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_2B0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_2C0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_2D0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_2E0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font12_2F0=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_00=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_10=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_20=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_30=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_40=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_50=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_60=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_70=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_80=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_90=\"¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶¶\"\n\
font8_A0=\"ABCDEFGHIJKLMNOP\"\n\
font8_B0=\"QRSTUVWXYZabcdef\"\n\
font8_C0=\"ghijklmnopqrstuv\"\n\
font8_D0=\"wxyz0123456789!?\"\n\
font8_E0=\"/«»:&()'.,=-+%# \"\n\
font8_F0=\"°¶##LRHMP·¶()¶¶_\"\n\
def8sym $20 \"[bladesymbol]\"\n\
def8sym $21 \"[bowsymbol]\"\n\
def8sym $22 \"[gunsymbol]\"\n\
def8sym $23 \"[armsymbol]\"\n\
def8sym $24 \"[swordsymbol]\"\n\
def8sym $25 \"[fistsymbol]\"\n\
def8sym $26 \"[scythesymbol]\"\n\
def8sym $28 \"[armorsymbol]\"\n\
def8sym $27 \"[helmsymbol]\"\n\
def8sym $29 \"[ringsymbol]\"\n\
def8sym $2E \"[shieldsymbol]\"\n\
def8sym $2F \"[starsymbol]\"\n\
def8sym $62 \"[handpart1]\"\n\
def8sym $63 \"[handpart1]\"\n\
def8sym $67 \"[hpmeter0]\"\n\
def8sym $68 \"[hpmeter1]\"\n\
def8sym $69 \"[hpmeter2]\"\n\
def8sym $6A \"[hpmeter3]\"\n\
def8sym $6B \"[hpmeter4]\"\n\
def8sym $6C \"[hpmeter5]\"\n\
def8sym $6D \"[hpmeter6]\"\n\
def8sym $6E \"[hpmeter7]\"\n\
def8sym $6F \"[hpmeter8]\"\n\
def12sym $EE \"[musicsymbol]\"\n\
def12sym $F0 \"[heartsymbol]\"\n\
def12sym $F1 \"...\"\n\
charcachesize=256\n\
dummy=0\n\
";
    
    ConfParser config;

    const char cfgfilename[] = "ct.cfg";
    const char cfgtmpname[] = "ct-cfg.tmp";
};

const ConfParser::Field& GetConf(const char *sect, const char *var)
{
    try
    {
        return config[sect][var];
    }
    catch(ConfParser::invalid_section sect_error)
    {
        typedef std::string errortype;
        static std::set<errortype> displayed;
        
        const errortype ErrorName(sect_error.GetSection());
        
        if(displayed.find(ErrorName) == displayed.end())
        {
            fprintf(stderr,
                "> %s: Missing section [%s]\n",
                cfgfilename,
                ErrorName.c_str()
                   );
            displayed.insert(ErrorName);
        }

        static const ConfParser::Field error;
        return error;
    }
    catch(ConfParser::invalid_field field_error)
    {
        typedef std::pair<std::string, std::string> errortype;
        static std::set<errortype> displayed;
        
        const errortype ErrorName(field_error.GetField().c_str(),
                                  field_error.GetSection().c_str());
        
        if(displayed.find(ErrorName) == displayed.end())
        {
            fprintf(stderr,
                "> %s: Missing \"%s\" in [%s]\n",
               cfgfilename,
               ErrorName.first.c_str(),/* fieldname */
               ErrorName.second.c_str()/* section name */
                   );

            displayed.insert(ErrorName);
        }

        static const ConfParser::Field error;
        return error;
    }
}

namespace
{
    /* Can't be embedded to be instantified under GetConf,
     * because "characterset" must be loaded in program start.
     */
    class ConfigLoader
    {
    public:
        ConfigLoader()
        {
            bool was_tmp = false;
            
            fprintf(stderr, "Reading %s...", cfgfilename);
            
            FILE *fp = fopen(cfgfilename, "rt");
            if(!fp)
            {
                if(errno == ENOENT)
                {
                    fprintf(stderr,
                        "The configuration file '%s' doesn't exist.\n"
                        "You should have one if you are dumping any other ROMs\n"
                        "than the standard english Chrono Trigger ROM, or if\n"
                        "you are inserting scripts to ROMs.\n"
                        "Ask Bisqwit how to create one.\n"
                        "Using internal defaults for now.\n",
                        cfgfilename);
                }
                else
                    perror(cfgfilename);
                
                fp = fopen(cfgtmpname, "wt");
                fwrite(DefaultConfig, std::strlen(DefaultConfig), 1, fp);
                fclose(fp);
                fp = fopen(cfgtmpname, "rt");
                was_tmp = true;
            }
            
            char Buf[8192];
            setvbuf(fp, Buf, _IOFBF, sizeof Buf);

            config.Parse(fp);

            std::wstring cset = GetConf("general", "characterset");
            setcharset(WstrToAsc(cset).c_str());

            rewind(fp);
            clearerr(fp);
            
            config.Clear();
            config.Parse(fp);
            
            fclose(fp);

            if(was_tmp)remove(cfgtmpname);
            
            fprintf(stderr, " done\n");
        }
    } ConfigLoader;
}
