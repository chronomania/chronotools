#include <list>
#include <string>
#include <cstdarg>

#include "snescode.hh"
#include "config.hh"
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

void insertor::ObsoleteCode(unsigned address, unsigned bytes, bool barrier)
{
    const bool ClearSpace = GetConf("patch", "clear_free_space");
    
    address &= 0x3FFFFF;
    
    // 0x80 = BRA
    // 0xEA = NOP
    if(barrier)
    {
        if(bytes > 0)
        {
            freespace.Add(address, bytes);
            if(ClearSpace)
            {
                vector<unsigned char> empty(bytes, 0);
                PlaceData(empty, address, "free space");
            }
            bytes = 0;
        }
    }
    else
    {
        while(bytes > 2)
        {
            bytes -= 2;
            unsigned skipbytes = bytes;
            if(skipbytes > 127) skipbytes = 127;
            
            PlaceByte(0x80,      address++, "bra"); // BRA over the space.
            PlaceByte(skipbytes, address++, "bra");
            
            freespace.Add(address, skipbytes);
            if(ClearSpace)
            {
                vector<unsigned char> empty(skipbytes, 0);
                PlaceData(empty, address, "free space");
            }
            
            bytes -= skipbytes;
        }
        while(bytes > 0) { PlaceByte(0xEA, address++, "nop"); --bytes; }
    }
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
        
        address += 4;
        bool barrier = false;
        if(add_rts)
        {
            // 0x60 = RTS
            PlaceByte(0x60, address++, "rts"); // rts
            barrier = true;
        }
        
        ObsoleteCode(address, nopcount, barrier);
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
