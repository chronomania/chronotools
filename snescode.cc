#include <list>
#include <string>
#include <cstdarg>

#include "snescode.hh"
#include "refer.hh"

using std::list;
using std::string;

typedef unsigned char byte;
SNEScode::SNEScode(const vector<unsigned char>& data)
  : vector<unsigned char> (data),
    located(false)
{
}

SNEScode::SNEScode(const string& n): name(n), located(false)
{
}
SNEScode::SNEScode(): located(false)
{
}
SNEScode::~SNEScode()
{
}

void SNEScode::YourAddressIs(unsigned addr)
{
    address = addr;
    located = true;
    
    //fprintf(stderr, "%u-bytes long subroutine... placing\n", size());
}

#include "ctinsert.hh"
#include "logfiles.hh"

void insertor::PlaceData(const vector<unsigned char>& data,
                         unsigned address,
                         const string& what)
{
    SNEScode blob(data);
    blob.SetName(what);
    blob.YourAddressIs(address);
    codes.push_back(blob);
}

void insertor::PlaceByte(unsigned char byte,
                         unsigned address,
                         const string& what)
{
    SNEScode blob(what);
    //blob.EmitCode(byte);
    blob.push_back(byte);
    blob.YourAddressIs(address & 0x3FFFFF);
    codes.push_back(blob);
}

void insertor::AddReference(const ReferMethod& reference,
                            unsigned target,
                            const string& what)
{
    SNEScode tmp(what);
    tmp.YourAddressIs(target);
    tmp.Add(reference);
    codes.push_back(tmp);
}

#include "o65.hh"
#include "config.hh"
bool insertor::LinkCalls(const string& section)
{
    const ConfParser::ElemVec& elems = GetConf(section.c_str(), "add_call_of").Fields();
    if(elems.empty()) return false;
    for(unsigned a=0; a<elems.size(); a += 4)
    {
        const ucs4string& funcname = elems[a];
        unsigned address           = elems[a+1];
        unsigned nopcount          = elems[a+2];
        bool add_rts               = elems[a+3];
        
        objects.AddReference(WstrToAsc(funcname), CallFrom(address));
        
        // 0x60 = RTS
        // 0x80 = BRA
        // 0xEA = NOP
        
        address += 4;
        if(add_rts)
        {
            PlaceByte(0x60, address++, "rts"); // rts
            if(nopcount > 0)
            {
                freespace.Add((address >> 16) & 0x3F,
                              address & 0xFFFF,
                              nopcount);
                
                // Don't initialize, or it will overwrite whatever
                // uses that space!
                //while(skipbytes-- > 0) { PlaceByte(0, address++); --nopcount; }
                nopcount = 0;
            }
        }
        else
        {
            while(nopcount > 2)
            {
                nopcount -= 2;
                unsigned skipbytes = nopcount;
                if(skipbytes > 127) skipbytes = 127;
                PlaceByte(0x80,      address++, "bra"); // BRA over the space.
                PlaceByte(skipbytes, address++, "bra");
                
                freespace.Add((address >> 16) & 0x3F,
                              address & 0xFFFF,
                              skipbytes);
                
                // Don't initialize, or it will overwrite whatever
                // uses that space!
                //while(skipbytes-- > 0) { PlaceByte(0, address++); --nopcount; }
                nopcount -= skipbytes;
            }
            while(nopcount > 0) { PlaceByte(0xEA, address++, "nop"); --nopcount; }
        }
    }
    return true;
}

#include <cerrno>
const O65 LoadObject(const string& filename, const string& what)
{
    O65 result;
    FILE *fp = fopen(filename.c_str(), "rb");
    if(!fp)
    {
        if(errno != ENOENT)
        {
            string message = "> ";
            message += what;
            message += " disabled: ";
            message += filename;
            perror(message.c_str());
        }
        result.SetError();
        return result;
    }
    result.Load(fp);
    fclose(fp);
    return result;
}

const O65 CreateObject(const vector<unsigned char>& code, const string& name)
{
    O65 result;
    result.LoadCodeFrom(code);
    result.DeclareCodeGlobal(name, 0);
    return result;
}
