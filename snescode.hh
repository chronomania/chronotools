#ifndef bqtSNEScodeHH
#define bqtSNEScodeHH

#include <vector>
#include <list>
#include <set>
#include <utility>
#include <map>
#include "wstring.hh"

using namespace std;

class SNEScode: public vector<unsigned char>
{
    typedef unsigned char byte;
    unsigned address;
    bool located;
    
    set<unsigned> callfrom;
    set<unsigned> longptrfrom;
    set<unsigned> offsptrfrom;
    set<unsigned> pageptrfrom;
    
#if 0
    list<class FarToNearCallCode *> farcalls;
    
    void Add(unsigned n...);
    struct BitnessData
    {
        enum { StateUnknown, StateSet, StateUnset, StateAnything }
            StateX, StateM,
            WantedX, WantedM;
    public:
        BitnessData() { StateX=StateM=WantedX=WantedM=StateUnknown; }
        void Update(const BitnessData &b)
        {
            if(b.StateX != StateAnything && b.StateX != StateX) StateX = StateUnknown;
            if(b.StateM != StateAnything && b.StateM != StateM) StateM = StateUnknown;
        }
    } bits;
    friend class RelativeBranch;
    friend class RelativeLongBranch;
    
    void CheckStates();
#endif

public:
#if 0
    class FarToNearCall
    {
        SNEScode &code;
        class FarToNearCallCode *ptr;
    public:
        FarToNearCall(SNEScode &c, class FarToNearCallCode *p);
        void Proceed(unsigned addr);
    };
    class RelativeBranch
    {
        SNEScode &code;
        set<unsigned> from;
        unsigned target;
        BitnessData bits;
    public:
        RelativeBranch(SNEScode &c);
        void FromHere();
        void ToHere();
        void Proceed();
    };
    class RelativeLongBranch
    {
        SNEScode &code;
        set<unsigned> from;
        unsigned target;
        BitnessData bits;
    public:
        RelativeLongBranch(SNEScode &c);
        void FromHere();
        void ToHere();
        void Proceed();
    };

    void EmitCode(byte a);
    void EmitCode(byte a,byte b);
    void EmitCode(byte a,byte b,byte c);
    void EmitCode(byte a,byte b,byte c,byte d);
    void EmitCode(byte a,byte b,byte c,byte d,byte e);
    void EmitCode(byte a,byte b,byte c,byte d,byte e,byte f);
    
    void Set16bit_M();
    void Set8bit_M();
    void Set16bit_X();
    void Set8bit_X();
    void BitnessUnknown();
    void BitnessAnything();

    const FarToNearCall PrepareFarToNearCall();

    const RelativeBranch PrepareRelativeBranch();
    const RelativeLongBranch PrepareRelativeLongBranch();
#endif
    
    explicit SNEScode(const vector<unsigned char>&);
    SNEScode();
    ~SNEScode();
    void AddCallFrom(unsigned address);
    void AddLongPtrFrom(unsigned address);
    void AddOffsPtrFrom(unsigned address);
    void AddPagePtrFrom(unsigned address);
    
    /*
        Use this routine to call near-routines from your routines.
        Usage:
            ... your code here ...
           FarToNearCall call = code.PrepareFarToNearCall()....
           ... last things before call to the routine ...
           call.Proceed(function_address);
           ... rest of your code ...
        
        Note: PrepareFarCall() OVERWRITES the following:
              - Register A (you want to reset it before proceeding)
              - Stack (don't pop anything before "proceed"!)
              - M-flag (do rep/sep $20 before proceeding!)
    */
    
    // If you don't locate the code anywhere, it won't work!
    void YourAddressIs(unsigned addr);
    
public:
    // These are for ROM::AddPatch() to use
    unsigned GetAddress() const { return address; }
    bool HasAddress() const { return located; }
    const set<unsigned>& GetCalls() const { return callfrom; }
    const set<unsigned>& GetLongPtrs() const { return longptrfrom; }
    const set<unsigned>& GetOffsPtrs() const { return offsptrfrom; }
    const set<unsigned>& GetPagePtrs() const { return pageptrfrom; }
};

struct SubRoutine
{
    // code
    SNEScode code;
    
    // funcname -> positions where called
    typedef map<ucs4string, set<unsigned> > requires_t;
    requires_t requires;
    
#if 0
    void CallFar(unsigned address);
    void JmpFar(unsigned address);
    void CallSub(const ucs4string &name);
#endif
};

#if 0
struct FunctionList
{
    // funcname -> pair<function, requiredflag>
    typedef map<ucs4string, pair<SubRoutine, bool> > functions_t;
    functions_t functions;

    void Define(const ucs4string &name, const SubRoutine &sub);
    void RequireFunction(const ucs4string &name);
};
#endif

#endif
