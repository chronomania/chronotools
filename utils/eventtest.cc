#include <vector>
#include <cctype>
#include <string>
#include <map>

#include "compress.hh"
#include "eventdata.hh"
#include "miscfun.hh"

typedef unsigned char Byte;

static Byte *ROM;

static void LoadROM(std::FILE *fp)
{
    std::fprintf(stderr, "Loading ROM...");
    unsigned hdrskip = 0;

    char HdrBuf[512];
    std::fread(HdrBuf, 1, 512, fp);
    if(HdrBuf[1] == 2)
    {
        hdrskip = 512;
    }
    std::fseek(fp, 0, SEEK_END);
    unsigned romsize = std::ftell(fp) - hdrskip;
    std::fprintf(stderr, " $%X bytes...", romsize);
    ROM = new unsigned char [romsize];
    std::fseek(fp, hdrskip, SEEK_SET);
    std::fread(ROM, 1, romsize, fp);
    std::fprintf(stderr, " done\n");
}

class Decompressor
{
public:
    unsigned origsize;
    
    void Decompress(unsigned addr, std::vector<Byte>& Data)
    {
        origsize = Uncompress(ROM+(addr&0x3FFFFF), Data, ROM+0x400000);
        std::printf("*** Data $%06X: %u / %u\n", addr|0xC00000, Data.size(), origsize);
    }
};

class EventRecord
{
public:
    class LineRecord
    {
    public:
        std::string Name;
        std::string Command;
        bool ForceLabel;
        
        std::string Jump;
        
        typedef std::map<unsigned/*addr*/, std::string/*label*/> referrermap;
        referrermap Referrers;
    public:
        LineRecord() : ForceLabel(false)
        {
        }
    
        void RenameLabel(const std::string& oldname, const std::string& newname)
        {
            str_replace_inplace(Command, oldname, newname);
            Jump = newname;
        }
    };
    typedef std::map<unsigned, LineRecord> linemap;
    linemap lines;

public:
    void CreateLabels()
    {
        bool ContainsEmpties = false;
        for(linemap::const_iterator i=lines.begin(); i!=lines.end(); ++i)
            if(i->second.Command.empty())
                { ContainsEmpties = true; break; }
        
        unsigned labelcounter = 0;
        for(linemap::iterator i=lines.begin(); i!=lines.end(); ++i)
        {
            LineRecord& l = i->second;
            
            if(!l.Referrers.empty() || l.ForceLabel || l.Command.empty())
            {
                /* Create a label */
                labelcounter += 10;
                l.Name = format("L%u", labelcounter);
            }
            
            if(ContainsEmpties)
            {
                l.Name = format("%04X", i->first);
            }
            
            if(!l.Referrers.empty())
            {
                for(LineRecord::referrermap::iterator
                    j = l.Referrers.begin();
                    j != l.Referrers.end();
                    ++j)
                {
                    lines[j->first].RenameLabel(j->second, l.Name);
                }
            }
        }
    }
    
    void Dump()
    {
        bool ContainsEmpties = false;
        for(linemap::const_iterator i=lines.begin(); i!=lines.end(); ++i)
            if(i->second.Command.empty())
                { ContainsEmpties = true; break; }
        
        std::map<std::string, unsigned> n_waiting_for_goto;
        std::map<std::string, unsigned> n_waiting_for_label;
        
        unsigned indent = 0;
        
        for(linemap::const_iterator i=lines.begin(); i!=lines.end(); ++i)
        {
            const LineRecord& l = i->second;
            if(!l.Name.empty())
            {
                printf("$%s:", l.Name.c_str());
            }
            printf("\t");
            
            unsigned add_indent = 0;
            
            if(!l.Referrers.empty())
            {
                /* Decrease indent for each pending goto. */
                unsigned n = n_waiting_for_label[l.Name];
                n_waiting_for_label.erase(l.Name);
                indent -= n;
                
                n = l.Referrers.size() - n;
                if(n > 0)
                {
                    unsigned n = l.Referrers.size();
                    n_waiting_for_goto[l.Name] += n;
                    add_indent += n;
                }
            }
            
            if(!l.Jump.empty())
            {
                /* We're jumping. */
                if(n_waiting_for_goto[l.Jump] > 0)
                {
                    /* Backwards: Decrease indent now. */
                    unsigned n = n_waiting_for_goto[l.Jump];
                    n_waiting_for_goto.erase(l.Jump);
                    indent -= n;
                }
                else
                {
                    /* Jump forwards. Increase indent after command. */
                    ++n_waiting_for_label[l.Jump];
                    ++add_indent;
                }
            }
            
            printf("%*s", indent, "");
            
            indent += add_indent;

            if(!l.Command.empty())
                printf("%s", l.Command.c_str());
            else
                printf("???");
            
            if(ContainsEmpties && !l.Referrers.empty())
            {
                printf("\t;%u refs: ", l.Referrers.size());
                for(LineRecord::referrermap::const_iterator
                    j = l.Referrers.begin();
                    j != l.Referrers.end();
                    ++j)
                    printf(" %04X", j->first);
            }
            printf("\n");
        }
    }
};

static void DumpEvent(vector<Byte> Data)
{
    #define GetByte() Data[offs++]
    #define GetWord() (offs+=2, (Data[offs-2]|(Data[offs-1]<<8)))
    
    unsigned n_actors = Data[0];
    Data.erase(Data.begin()); // erase first byte.
    
    unsigned offs = 0;
    printf("- Number of actors: %u\n", n_actors);
    
    EventCode decoder;
    EventRecord record;
    
    for(unsigned a=0; a<n_actors; ++a)
    {
        std::vector<unsigned> pointers;
        for(unsigned b=0; b<16; ++b) pointers.push_back(GetWord());
        
        unsigned b=16;
        while(b>=2 && pointers[b-1] == pointers[b-2]) --b;
        for(unsigned c=0; c<b; ++c)
        {
            /*
               0: initialize
               1: talk/use
               
               3: automatic action
            */
        
            std::string text;
            unsigned actor = a;
            unsigned code  = c;
            char Buf[64];
            std::sprintf(Buf, "[ACTOR:%u:%u]", actor, code);
            text += Buf;
            
            EventRecord::LineRecord& line = record.lines[pointers[c]];
            line.ForceLabel = true;
            line.Command += text;
        }
    }
    
    while(offs < Data.size())
    {
        const unsigned address = offs;
        Byte command = GetByte();

        EventRecord::LineRecord& line = record.lines[address];

        std::string text;
        
        //{ char Buf[64]; sprintf(Buf, "{%02X}", command); text += Buf; }
        /*unsigned size = tmp.nbytes;
        printf("%s", text.c_str());
        printf("\t\t;%02X", Data[offs-1]);
        for(unsigned a=0; a<size; ++a) printf(" %02X", Data[offs+a]);
        printf("\n");*/
        
        decoder.InitDecode(offs, command);
        
        EventCode::DecodeResult tmp;
        tmp = decoder.DecodeBytes(&Data[offs], Data.size()-offs);
        
        text += tmp.code;
        offs += tmp.nbytes;
        
        line.Command += text;
        if(!tmp.label_name.empty())
        {
            record.lines[tmp.label_value].Referrers[address] = tmp.label_name;
        }
    }
    
    record.CreateLabels();
    
    record.Dump();
}

static void LoadEvent(unsigned evno, vector<Byte>& Data)
{
    Decompressor de;
    
    unsigned addr = 0x3CF9F0 + evno*3;
    unsigned addr2 = ROM[addr] | (ROM[addr+1]<<8) | (ROM[addr+2]<<16);
    de.Decompress(addr2, Data);
}

int main(void)
{
    FILE *fp = fopen("FIN/chrono-nohdr.smc", "rb");
    LoadROM(fp);
    fclose(fp);
    
//vector<Byte> Data; LoadEvent(0x1B, Data); DumpEvent(Data); return 0;

    for(unsigned a=0x000; a<=0x200; ++a)
    {
        if(a >= 0x66 && a <= 0x6E) continue;
        if(a >= 0xE7 && a <= 0xEB) continue;
        if(a >=0x140 && a <=0x142) continue;
        vector<Byte> Data;
        LoadEvent(a, Data);

        std::printf("*** Event %03X: %s\n", a, LocationEventNames[a]);
        if(Data.empty()) continue;  
        
        DumpEvent(Data);
        
        std::printf("\n\n");
    }

    return 0;
}
