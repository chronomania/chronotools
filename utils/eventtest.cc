#include <vector>
#include <cctype>
#include <string>
#include <list>
#include <set>
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
        if(!origsize) Data.clear();
    }
};


/*


  Goto constructs:
  
     
   A:                          loop {
     x                            x
     <no ifs reaching beyond A>
     goto_loop(A)              }
   
   D:                          loop {
     x                            x
     goto_if(E)(rule)             break if(rule)
     goto_loop(D)                    
   E:                          }
   
   D:                          loop {
     x                            x
     goto_else(E)                 break
     goto_loop(D)              
   E:                          }
   
     goto_if(B)(rule)          if(rule) {
     x                           x
   B:                          }
   
     goto_if(B)(rule)          if(rule) {
     x                           x
     goto_else(C)              } else {
   B:                          
     y                           y
   C:                          }
  
   
   
$L20:	[LetB:7F021C:7E2980]
	 [LetB:7F021E:7E2981]
	 [LetB:7F0220:7E2982]
	 [Unknown24:7F0210:00]
	 [Unknown22:00:09:0A]
	 [Goto:L50 [UnlessB:7F0212 >= 19]]
	  [Goto:L50 [UnlessB:7F0214 >= 30]]
	   [Goto:L50 [IfStorylinePoint:33]]
	    [Goto:L30 [IfStorylinePoint:2E]]
	     [Goto:L50]
$L30:	     [Goto:L40 [UnlessB:7E00A2 & 08]]
	      [Goto:L50]
$L40:	      [SetBit:7E00A2:03]
$L50:	[Goto:L20]



$L20: [loop [

      [If:7F0212 >= 19 [
       [If:7F0214 >= 30 [
        [If:Storyline  33 [
         [If:StoryLine 2E [
         ]else[
          [If:7E00A2 & 8 [
          ]else[
           [Setbit]
          ]]
         ]]
        ]]
       ]]
      ]]
      ]]
   
   

 */

class EventRecord
{
public:
    class LineRecord
    {
    public:
        std::wstring Command;
        
        EventCode::gototype goto_type;
        unsigned            goto_target;
        
    public:
        LineRecord()
        {
        }
    };
    typedef std::map<unsigned, LineRecord> linemap;
    linemap lines;
public:
    void Dump(const std::vector<unsigned>& pointers)
    {
        DumpUsing(pointers);
    }
    void OptimizeIfs()
    {
        //return;
        
        /* This function attempts to find sequences of following type:
            
            A if(rule)goto B
              goto C
            B
           
           And convert them to:
           
            A ifnot(rule)goto C
         */
        for(linemap::iterator i = lines.begin(); i != lines.end(); ++i)
        {
        retry:
            EventCode::gototype tmp = i->second.goto_type;
            EventCode::gototype complement;
            if(tmp == EventCode::goto_if)
                complement = EventCode::goto_ifnot;
            else if(tmp == EventCode::goto_ifnot)
                complement = EventCode::goto_if;
            else
                continue;
        
            linemap::iterator j = i; ++j;
            if(j == lines.end()) continue;
            if(j->second.goto_type != EventCode::goto_forward) continue;
            
            linemap::iterator k = j; ++k;
            if(k == lines.end()) continue;
            if(i->second.goto_target == k->first)
            {
                i->second.goto_type = complement;
                i->second.goto_target = j->second.goto_target;
                lines.erase(j);
                goto retry;
            }
        }
    }
private:
    enum contexttype { ctx_default, ctx_loop, ctx_if };

    struct DepthData
    {
        linemap::const_iterator cur;
        unsigned endpos;
        
        unsigned indent;
        
        contexttype context;
        
        bool was_terminal;
        
    public:
        DepthData(linemap::const_iterator i,
                  unsigned end, unsigned ind,
                  contexttype ctx)
         : cur(i), endpos(end), indent(ind), context(ctx),
           was_terminal(false) { }
    };
    
    typedef std::map<unsigned, std::string> gotomap;
    gotomap gotos;
    
    const std::string& CreateLabel(unsigned target)
    {
        gotomap::const_iterator i = gotos.find(target);
        if(i != gotos.end()) return i->second;
        
        return gotos[target] = format("%04X", target/2);
    }

    void DumpUsing(const std::vector<unsigned>& pointers)
    {
        typedef std::list<DepthData> StackType;
        StackType stack;
        
        gotos.clear();
        
        unsigned current_function = 256;
        bool     found_return = false;
        
        /* Initialize the stack. */
        stack.push_front(DepthData
            (lines.begin(),
             lines.rbegin()->first+1,
             0, /* indent */
             ctx_default));

        CreateLabel(stack.begin()->cur->first);

        while(!stack.empty())
        {
            DepthData& front_ref = *stack.begin();
            const DepthData front = front_ref;
            
            if(front.cur == lines.end()
            || front.cur->first >= front.endpos)
            {
                stack.pop_front();
                
                switch(front.context)
                {
                    case ctx_default:
                    {
                        break;
                    }
                    case ctx_loop:
                    {
                        DepthData& parent = *stack.begin();
                        OutputLine("[Loop]", parent.indent); //endloop
                        parent.cur = front.cur;
                        break;
                    }
                    case ctx_if:
                    {
                        DepthData& parent = *stack.begin();
                        OutputLine("[EndIf]", parent.indent); //endif
                        parent.cur = front.cur;
                        break;
                    }
                }
                continue;
            }
            
            const LineRecord& line = front.cur->second;
            const unsigned addr    = front.cur->first;
            
            if(UpdateFunctionNumber(addr, pointers, current_function))
            {
                if(current_function == 0) found_return = false;
            }
            
            bool is_referred = DumpPointers(addr, pointers);
            if(front.was_terminal && !is_referred)
            {
                /* Just in case this causes errors, only apply
                 * the optimization to excess 'else's
                 */
                if(line.goto_type == EventCode::goto_forward
                && front.context == ctx_if)
                {
                    OutputLine("; ignoring goto", front.indent);
                    /* Ignore an unused statement */
                    ++front_ref.cur;
                    continue;
                }
            }
            
            front_ref.was_terminal = false;
            switch(line.goto_type)
            {
                case EventCode::goto_none:
                {
                    if(!line.Command.empty())
                    {
                        OutputLine(line.Command, front.indent);
                    }
                    
                    if(line.Command.substr(0,8) == "[Return]")
                    {
                        if(current_function == 0 && !found_return)
                        {
                            /* In function #0, first 'return' is not a terminal. */
                            found_return = true;
                        }
                        else
                            front_ref.was_terminal = true;
                    }
                    /*
                      - disabling... It might not be safe.                    
                    if(line.Command.size() > 10
                    && line.Command.substr(line.Command.size()-10,9) == "[forever]")
                    {
                        front_ref.was_terminal = true;
                    } 
                    */                   
                    break;
                }
                
                case EventCode::goto_loopbegin:
                {
                    /* If the loop extends beyond the bounds
                     * of the current scope, it's too difficult
                     * to handle. In that case, turn it into gotos.
                     */
                    bool is_ok = true;
                    for(StackType::const_iterator
                        i = stack.begin();
                        i != stack.end();
                        ++i)
                    {
                        if(line.goto_target > i->endpos)
                        {
                            is_ok = false;
                            break; // Didn't match this loop
                        }
                    }
                    
                    if(is_ok)
                    {
                        DepthData child = front_ref;
                        ++child.cur;
                        child.endpos = line.goto_target;
                        child.indent += 2;
                        child.context = ctx_loop;
                        
                        if(child.cur->second.goto_target == child.endpos
                        && child.cur->second.goto_type == EventCode::goto_if)
                        {
                            // until
                            std::wstring cmd = format("[Until%s]", child.cur->second.Command.c_str());
                            OutputLine(cmd, front.indent);
                            ++child.cur;
                        }
                        else if(child.cur->second.goto_target == child.endpos
                        && child.cur->second.goto_type == EventCode::goto_ifnot)
                        {
                            // while
                            std::wstring cmd = format("[While%s]", child.cur->second.Command.c_str());
                            OutputLine(cmd, front.indent);
                            ++child.cur;
                        }
                        else
                        {
                            OutputLine("[LoopBegin]", front.indent);
                        }
                        
                        /*  FIXME: turn ifX { ifY {a} } else {b}
                         *         into unlessX {b} elseifY {a}
                         */
                        
                        stack.push_front(child);
                        continue;
                    }
                    /* Ok, bad thing. */
                    linemap::iterator i = lines.find(line.goto_target);
                    --i;
                    i->second.goto_type = EventCode::goto_forward;
                    
                    std::wstring label = CreateLabel(addr);
                    OutputLabel(label);
                    OutputLine(";loop begin", front.indent);
                    
                    break;
                }
                
                case EventCode::goto_backward:
                {
                    // don't dump. It's a loop-end.
                    
                    // But mark it a "terminal".
                    front_ref.was_terminal = true;
                    break;
                }
                    
                case EventCode::goto_forward:
                {
                    front_ref.was_terminal = true;
                    
                    /* Find out if we're inside a loop
                     * and the target is the end position
                     * of the said loop */
                    bool found_loop = false;
                    for(StackType::const_iterator
                        i = stack.begin();
                        i != stack.end();
                        ++i)
                    {
                        if(i->context == ctx_loop)
                        {
                            if(line.goto_target == i->endpos)
                            {
                                found_loop = true;
                            }
                            break; // Didn't match this loop
                        }
                    }
                    if(found_loop)
                    {
                        OutputLine("[Break]", front.indent);
                        break;
                    }
                    enum { not_else, is_else, is_redundant_else,
                           is_elseif, is_elseifnot }
                        elsetype = not_else;
                    
                    /* Well then. Is this an "else" then? */
                    if(front.context == ctx_if
                    && !front.was_terminal)
                    {
                        /* It is an else, if the next line
                         * is the current "if"'s end. */
                        linemap::const_iterator next = front.cur; ++next;
                        //while(next != lines.end()
                        //&& next->second.goto_type == EventCode::goto_none
                        //&& next->second.Command.empty()) ++next;
                        
                        if(next != lines.end())
                        {
                            if(front.endpos == next->first)
                            {
                                elsetype = is_else;
                                if(front.endpos == line.goto_target)
                                {
                                    elsetype = is_redundant_else;
                                }
                                else
                                {
                                    /* If the 'else' branch
                                     * begins with an 'if' that ends
                                     * where this 'else' ends
                                     */
                                    unsigned next_goal = next->second.goto_target;
                                    if(next_goal == line.goto_target)
                                    {
                                    yes_elseif:
                                        if(next->second.goto_type == EventCode::goto_if)
                                        {
                                            elsetype = is_elseif;
                                        }
                                        else if(next->second.goto_type == EventCode::goto_ifnot)
                                        {
                                            elsetype = is_elseifnot;
                                        }
                                    }
                                    else
                                    {
                                        /* If the 'if' inside this 'else'
                                         * ends in another 'else' that ends
                                         * where this 'else' ends
                                         */
                                        linemap::const_iterator i = lines.find(next_goal); --i;
                                        const LineRecord& further = i->second;
                                        if(further.goto_type == EventCode::goto_forward
                                        && further.goto_target == line.goto_target)
                                        {
                                            goto yes_elseif;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    front_ref.was_terminal = false;
                    switch(elsetype)
                    {
                        case is_else:
                        {
                            /* Find the parent's indent level */
                            StackType::const_iterator i = stack.begin(); ++i;
                            const DepthData& parent = *i;
                            
                            OutputLine("[Else]", parent.indent);
                            
                            /* Extend the "end" of the "if". */
                            front_ref.endpos = line.goto_target;
                            break;
                        }
                        case is_redundant_else:
                        {
                            OutputLine("; redundant 'else' removed", front.indent);
                            break;
                        }
                        case is_elseif:
                        case is_elseifnot:
                        {
                            /* Find the parent's indent level */
                            StackType::const_iterator i = stack.begin(); ++i;
                            const DepthData& parent = *i;
                            ++front_ref.cur;
                            
                            const char* posi_if = "If";
                            const char* nega_if = "Unless";
                            if(elsetype == is_elseifnot)
                            {
                                std::swap(posi_if, nega_if);
                            }
                            
                            std::wstring elseifcmd =
                                format("[Else%s%s]", posi_if, front_ref.cur->second.Command.c_str());

                            OutputLine(elseifcmd, parent.indent);
                            
                            /* Extend the "end" of the "if" to point to
                             * the end of the 'elseif'.
                             */
                            front_ref.endpos = front_ref.cur->second.goto_target;
                            break;
                        }
                        case not_else:
                        {
                            /* Still don't know what was it.
                             * Treat it as a goto.
                             * Since it is a forward goto, we'll definitely
                             * find it eventually.
                             */
                            std::wstring label = CreateLabel(line.goto_target);
                            OutputLine("[Goto:"+label+"]", front.indent);
                            front_ref.was_terminal = true;
                            break;
                        }
                    }
                    break;
                }
                    
                case EventCode::goto_if:
                case EventCode::goto_ifnot:
                {
                    const char* posi_if = "If";
                    const char* nega_if = "Unless";
                    if(line.goto_type == EventCode::goto_ifnot)
                    {
                        std::swap(posi_if, nega_if);
                    }
                    /* Find out if we're inside a loop
                     * and the target is the end position
                     * of the said loop */
                    bool found_loop = false;
                    for(StackType::const_iterator
                        i = stack.begin();
                        i != stack.end();
                        ++i)
                    {
                        if(i->context == ctx_loop)
                        {
                            if(line.goto_target == i->endpos)
                            {
                                found_loop = true;
                            }
                            break; // Didn't match this loop
                        }
                    }
                    if(found_loop)
                    {
                        std::wstring break_cmd =
                            format("[Break [%s%s]]", posi_if, line.Command.c_str());

                        OutputLine(break_cmd, front.indent);
                        break;
                    }
                    
                    /* If the goto target is smaller than current
                     * scope's end, construct an "if" statement.
                     */
                    bool is_if = line.goto_target <= front.endpos;
                    
                    if(is_if)
                    {
                        std::wstring if_cmd =
                            format("[%s%s]", posi_if, line.Command.c_str());
                        DepthData child = front_ref;
                        ++child.cur;
                        child.endpos = line.goto_target;
                        child.indent += 2;
                        child.context = ctx_if;
                        OutputLine(if_cmd, front.indent);
                        stack.push_front(child);
                        continue;
                    }
                    
                    /* Still don't know what was it.
                     * Treat it as a goto.
                     * Since it is a forward goto, we'll definitely
                     * find it eventually.
                     */
                    
                    std::wstring label = CreateLabel(line.goto_target);
                    std::wstring goto_cmd =
                        format("[Goto:%s [%s%s]]",
                               label.c_str(),
                               nega_if, line.Command.c_str());

                    OutputLine(goto_cmd, front.indent);
                    break;
                }
            }
            
            ++front_ref.cur;
        }
    }
    
    bool UpdateFunctionNumber(unsigned addr, const std::vector<unsigned>& pointers, unsigned& no)
    {
        for(unsigned f=0; f<16; ++f)
            for(unsigned ptrno=f; ptrno<pointers.size(); ptrno+=16)
                if(addr == pointers[ptrno])
                {
                    no = f;
                    return true;
                }
        return false;
    }
    
    void OutputLabel(const std::wstring& label)
    {
        printf("$%s:", label.c_str());
    }
    
    void FlushObjectsAndFunctions
        (const std::set<unsigned>& objects,
         const std::set<unsigned>& functions)
    {
        if(!objects.empty())
        {
            std::string list;
            for(std::set<unsigned>::const_iterator
                i = objects.begin(); i != objects.end(); ++i)
            {
                if(!list.empty()) list += ':';
                list += format("%X", *i);
            }
            printf("[OBJECT:%s]", list.c_str());
        }
        if(!functions.empty())
        {
            std::string list;
            for(std::set<unsigned>::const_iterator
                i = functions.begin(); i != functions.end(); ++i)
            {
                if(!list.empty()) list += ':';
                list += format("%X", *i);
            }
            printf("[FUNCTION:%s]", list.c_str());
        }
        printf("\n");
    }

    bool DumpPointers(unsigned addr, const std::vector<unsigned>& pointers)
    {
        bool is_referred = false;
        
        gotomap::const_iterator i = gotos.find(addr);
        if(i != gotos.end())
        {
            OutputLabel(i->second);
            is_referred = true;
        }
        
        std::set<unsigned> objects;
        std::set<unsigned> functions;
    
        unsigned n_objs = pointers.size() / 16;
        for(unsigned a=0; a<n_objs; ++a)
        {
            unsigned ptrno = a*16;
            
            std::string funclist;
            for(unsigned b=0; b<16; ++b)
            {
                unsigned ptr = pointers[ptrno++];
                if(ptr != addr)continue;
                
                if(!functions.empty() && !objects.empty())
                {
                    /* Ok if
                     *   1obj  && obj=a
                     *or 1func && func=b
                     */
                    if((functions.size()!=1 || *functions.begin()!=b)
                    && (objects.size()!=1 || *objects.begin()!=a))
                    {
                        FlushObjectsAndFunctions(objects, functions);
                        objects.clear(); functions.clear();
                    }
                }
                
                objects.insert(a);
                functions.insert(b);
                
                is_referred = true;
            }
        }
        if(!functions.empty() || !objects.empty())
            FlushObjectsAndFunctions(objects, functions);
        
        return is_referred;
    }
    
    void OutputLine(const std::string& s, unsigned indent)
    {
        printf("\t%*s%s\n", indent,"", s.c_str());
    }
};

static void DumpEvent(vector<Byte> Data)
{
    #define GetByte() Data[offs++]
    #define GetWord() (offs+=2, (Data[offs-2]|(Data[offs-1]<<8)))
    
    unsigned n_actors = Data[0];
    Data.erase(Data.begin()); // erase first byte.
    
    unsigned offs = 0;
    EventCode decoder;
    EventRecord record;
    
    std::vector<unsigned> pointers(n_actors * 16);
    
    const unsigned max_loops = 10;

    for(unsigned a=0; a<n_actors; ++a)
        for(unsigned b=0; b<16; ++b)
            pointers[a*16+b] = GetWord()*max_loops;
    
    printf("- Number of objects: %u - size: %04X\n", n_actors, Data.size());
    
    while(offs < Data.size())
    {
        const unsigned address = offs;

        std::string text;
        
        EventCode::DecodeResult tmp;
        tmp = decoder.DecodeBytes(address, &Data[address], Data.size()-address);
        
        text += tmp.code;
        offs += tmp.nbytes;
        
        /*
        text += "(*";
        text += format("{%04X}", address);
        for(unsigned a=0; a<tmp.nbytes; ++a) text += format(" %02X", Data[address+a]);
        text += " *)";
        */
        
        EventRecord::LineRecord& line = record.lines[address*max_loops];
        line.Command     = text;
        line.goto_type   = tmp.goto_type;
        line.goto_target = tmp.goto_target*max_loops;
        
        if(line.goto_type == EventCode::goto_backward)
        {
            unsigned beginpos = line.goto_target;
            // Move the loop beginning 1 forward and insert a loopbegin
            for(unsigned n=max_loops; --n>0; )
            {
                EventRecord::linemap::iterator
                    i = record.lines.find(beginpos + n-1);
                if(i == record.lines.end())  continue;
                record.lines[beginpos+n] = i->second;
            }
            EventRecord::LineRecord& begin = record.lines[beginpos];
            
            begin.goto_type   = EventCode::goto_loopbegin;
            begin.goto_target = offs*max_loops;
        }
    }
    record.OptimizeIfs();
    record.Dump(pointers);
    
    fflush(stdout);
    fflush(stderr);
}

static void LoadEvent(unsigned evno, vector<Byte>& Data)
{
    Decompressor de;
    
    unsigned addr = 0x3CF9F0 + evno*3;
    unsigned addr2 = ROM[addr] | (ROM[addr+1]<<8) | (ROM[addr+2]<<16);
    
    addr2 &= 0x3FFFFF;
    
    std::printf("*** %06X: %02X %02X %02X...\n", addr2,
        ROM[addr2], ROM[addr2+1], ROM[addr2+2]);
    
    Data.clear();
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
//        if(a >= 0x66 && a <= 0x6E) continue;
//        if(a >= 0xE7 && a <= 0xEB) continue;
//        if(a >=0x140 && a <=0x142) continue;
        vector<Byte> Data;
        LoadEvent(a, Data);

        std::printf("*** Event %03X (%u bytes): %s\n",
            a, Data.size(), LocationEventNames[a]);
        if(Data.empty()) continue;  
        
        DumpEvent(Data);
        
        std::printf("\n\n");
    }

    return 0;
}
