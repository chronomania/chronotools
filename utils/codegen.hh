#include <string>
#include <cstdio>

enum FlagWantedStateXY
{
    Want8bitXY,
    Want16bitXY,
    DontCareXY
};
enum FlagWantedStateA
{
    Want8bitA,
    Want16bitA,
    DontCareA
};

enum FlagAssumedStateXY
{
    Assume8bitXY,
    Assume16bitXY,
    UnknownXY
};
enum FlagAssumedStateA
{
    Assume8bitA,
    Assume16bitA,
    UnknownA
};

class FlagAssumption
{
    FlagAssumedStateA Astate;   
    FlagAssumedStateXY XYstate;
    bool A_specified, XY_specified;
public:
    FlagAssumption()
       : Astate(UnknownA),XYstate(UnknownXY),A_specified(false),XY_specified(false) { }
    FlagAssumption(FlagAssumedStateA A)
       : Astate(A),XYstate(UnknownXY),A_specified(true),XY_specified(false) { }
    FlagAssumption(FlagAssumedStateXY XY)
       : Astate(UnknownA),XYstate(XY),A_specified(false),XY_specified(true) { }
    FlagAssumption(FlagAssumedStateA A, FlagAssumedStateXY XY)
       : Astate(A), XYstate(XY), A_specified(true),XY_specified(true) { }
    FlagAssumption(FlagAssumedStateXY XY, FlagAssumedStateA A)
       : Astate(A), XYstate(XY), A_specified(true),XY_specified(true) { }
    void Combine(const FlagAssumption& b)
    {
        if(!A_specified) { Astate=b.Astate; } else if(Astate!=b.Astate) { Astate=UnknownA; }
        if(!XY_specified){XYstate=b.XYstate;}else if(XYstate!=b.XYstate){XYstate=UnknownXY;}
        A_specified=XY_specified=true;
    }
    FlagAssumedStateA GetA() const { return Astate; }
    FlagAssumedStateXY GetXY() const { return XYstate; }
};

/* Emit any code */
void Emit(const std::string& s, FlagWantedStateA, FlagWantedStateXY);

void EmitIfDef(const std::string& s);
void EmitIfNDef(const std::string& s);
void EmitEndIfDef();

/* Utilities */
inline void Emit(const std::string& s, FlagWantedStateXY XY, FlagWantedStateA A)
 { Emit(s, A, XY); }
inline void Emit(const std::string& s, FlagWantedStateXY XY)
 { Emit(s, XY, DontCareA); }
inline void Emit(const std::string& s, FlagWantedStateA A)
 { Emit(s, A, DontCareXY); }
inline void Emit(const std::string& s)
 { Emit(s, DontCareA, DontCareXY); }

void Assume(FlagAssumedStateA, FlagAssumedStateXY);

/* Utilities */
inline void Assume(FlagAssumedStateXY XY, FlagAssumedStateA A)
 { Assume(A, XY); }
inline void Assume(FlagAssumedStateXY XY)
 { Assume(XY, UnknownA); }
inline void Assume(FlagAssumedStateA A)
 { Assume(A, UnknownXY); }

const FlagAssumption GetAssumption();

/* Define a barrier - a point where the
 * execution never continues on the next line.
 */
void EmitBarrier();

/* Define a branch to label.
 * Supported branch types:
 *   bra bcc bcs beq bne jsr
 */
void EmitBranch(const std::string& s, const std::string& target);

/* Generate a label */
const std::string GenLabel();

/* Define a label */
void EmitLabel(const std::string& label);
void KeepLabel(const std::string& label);

void BeginCode(std::FILE *);
void EndCode();
