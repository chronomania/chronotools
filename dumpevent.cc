#include <string>
#include <map>

#include "dumpevent.hh"

#include "miscfun.hh" // str_replace_inplace
#include "compress.hh" // decompression
#include "eventdata.hh"
#include "rommap.hh"  // ROM[]
#include "romaddr.hh" // address conversions
#include "base62.hh"  // base62 functions
#include "scriptfile.hh"

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
        LineRecord(): ForceLabel(false)
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
                PutAscii(AscToWstr(format("$%s:", l.Name.c_str())));
            }
            PutAscii(L"\t");

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
            
            PutAscii(wformat(L"%*s", indent, L""));
            
            indent += add_indent;

            if(!l.Command.empty())
            {
                PutAscii(AscToWstr(l.Command));
            }
            else
                PutAscii(L"???");
            /*
            if(ContainsEmpties && !l.Referrers.empty())
            {
                printf("\t;%u refs: ", l.Referrers.size());
                for(LineRecord::referrermap::const_iterator
                    j = l.Referrers.begin();
                    j != l.Referrers.end(); 
                    ++j)
                    printf(" %04X", j->first);
            }
            */
            PutAscii(L"\n");
        }
    }
};

void DumpEvent(const unsigned ptroffs, const std::wstring& what)
{
    unsigned offs = (ROM[ptroffs+2 ] << 16)
                  | (ROM[ptroffs+1 ] << 8)
                  | (ROM[ptroffs+0 ]);
    offs = SNES2ROMaddr(offs);

    std::vector<unsigned char> Data;
    unsigned orig_bytes = Uncompress(ROM+offs, Data, ROM+GetROMsize());
    unsigned new_bytes = Data.size();
    
    if(Data.empty()) return;
    
    MarkProt(ptroffs, 3, L"event(e) pointers");
    MarkFree(offs, orig_bytes, what + L" data");
    
    StartBlock(AscToWstr("e"+EncodeBase62(ptroffs)), what);

    #define GetByte() Data[offs++]
    #define GetWord() (offs+=2, (Data[offs-2]|(Data[offs-1]<<8)))
    
    unsigned n_actors = Data[0];
    Data.erase(Data.begin()); // erase first byte.

    PutAscii(wformat(L"; Number of actors: %u\n", n_actors));
    
    offs = 0;

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
            std::string text;
            unsigned actor = a;
            unsigned code  = c;
            text += format("[ACTOR:%u:%u]", actor, code);
            
            EventRecord::LineRecord& line = record.lines[pointers[c]];
            line.ForceLabel = true;
            line.Command += text;  
        }
    }
     
    while(offs < Data.size())
    {
        const unsigned address = offs;
        unsigned char command = GetByte();

        EventRecord::LineRecord& line = record.lines[address];

        std::string text;
        
        // text += format("{%02X}", command);
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
