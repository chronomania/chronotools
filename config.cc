#include <cstdio>
#include <cerrno>

#include "ctcset.hh"
#include "wstring.hh"
#include "confparser.hh"

namespace
{
    static const char DefaultConfig[] =
"\n\
# A simple Chronotools configuration file.\n\
# Complete enough for ctdump to work.\n\
[general]\n\
characterset=\"iso-8859-15\"\n\
[dumper]\n\
font8fn=\"ct8fn.tga\"\n\
font12fn=\"ct16fn.tga\"\n\
[font]\n\
num_characters=96\n\
num_extra=0\n\
font8fn=\"ct8fn.tga\"\n\
font12fn=\"ct16fn.tga\"\n\
use_vwf8=false\n\
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
font12_F0=\"°¶##LRHMP·¶()¶¶_\"\n\
def8sym 0x20 \"[bladesymbol]\"\n\
def8sym 0x21 \"[bowsymbol]\"\n\
def8sym 0x22 \"[gunsymbol]\"\n\
def8sym 0x23 \"[armsymbol]\"\n\
def8sym 0x24 \"[swordsymbol]\"\n\
def8sym 0x25 \"[fistsymbol]\"\n\
def8sym 0x26 \"[scythesymbol]\"\n\
def8sym 0x28 \"[armorsymbol]\"\n\
def8sym 0x27 \"[helmsymbol]\"\n\
def8sym 0x29 \"[ringsymbol]\"\n\
def8sym 0x2E \"[shieldsymbol]\"\n\
def8sym 0x2F \"[starsymbol]\"\n\
def8sym 0x62 \"[handpart1]\"\n\
def8sym 0x63 \"[handpart1]\"\n\
def8sym 0x67 \"[hpmeter0]\"\n\
def8sym 0x68 \"[hpmeter1]\"\n\
def8sym 0x69 \"[hpmeter2]\"\n\
def8sym 0x6A \"[hpmeter3]\"\n\
def8sym 0x6B \"[hpmeter4]\"\n\
def8sym 0x6C \"[hpmeter5]\"\n\
def8sym 0x6D \"[hpmeter6]\"\n\
def8sym 0x6E \"[hpmeter7]\"\n\
def8sym 0x6F \"[hpmeter8]\"\n\
def12sym 0xEE \"[musicsymbol]\"\n\
def12sym 0xF0 \"[heartsymbol]\"\n\
def12sym 0xF1 \"...\"\n\
charcachesize=256\n\
";
    
    ConfParser config;
};

const ConfParser::Field& GetConf(const char *sect, const char *var)
{
    try
    {
        return config[sect][var];
    }
    catch(ConfParser::invalid_section sect_error)
    {
        static const ConfParser::Field error;
        return error;
    }
    catch(ConfParser::invalid_field field_error)
    {
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
            FILE *fp = fopen("ct.cfg", "rt");
            if(!fp)
            {
                if(errno == ENOENT)
                {
                    fprintf(stderr,
                        "The configuration file 'ct.cfg' doesn't exist.\n"
                        "You should have one if you are dumping any other ROMs\n"
                        "than the standard english Chrono Trigger ROM, or if\n"
                        "you are inserting scripts to ROMs.\n"
                        "Ask Bisqwit how to create one.\n"
                        "Using internal defaults for now.\n");
                }
                else
                    perror("ct.cfg");
                
                fp = fopen("ct-cfg.tmp", "wt");
                fwrite(DefaultConfig, strlen(DefaultConfig), 1, fp);
                fclose(fp);
                fp = fopen("ct-cfg.tmp", "rt");
                was_tmp = true;
            }
            
            char Buf[8192];
            setvbuf(fp, Buf, _IOFBF, sizeof Buf);

            config.Parse(fp);

            ucs4string cset = GetConf("general", "characterset");
            setcharset(WstrToAsc(cset).c_str());

            rewind(fp);
            clearerr(fp);
            
            config.Clear();
            config.Parse(fp);
            
            fclose(fp);

            if(was_tmp)remove("ct-cfg.tmp");
            
            fprintf(stderr, "Configuration loaded\n");
        }
    } ConfigLoader;
}
