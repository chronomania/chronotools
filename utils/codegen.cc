#include <vector>
#include <list>
#include <set>
#include <map>

#include "codegen.hh"
#include "macrogenerator.hh"
#include "autoptr"

typedef autoptr<struct CodeNode> CodeNodePtr;

#define THREAD_JUMPS     1
#define COMBINE_NODES    0
#define REORDER_NODES    0
#define NEGATE_JUMPS     1
#define DEBUG_NODES      0
#define ELIMINATE_DEAD   1
#define GENERATE_MACROES 0

static FILE *out = NULL;

struct FlagState
{
    FlagAssumedStateXY XY;
    FlagAssumedStateA  A;
    
    bool IsCompatible(const FlagState& b) const
    {
        return A == b.A && XY == b.XY;
    }
};

struct DefList: public ptrable
{
    std::map<std::string, unsigned> Defines;
    std::map<std::string, unsigned> NonDefines;
    
    autoptr<struct DefList> Alternate;
    
public:
    DefList(): Alternate(NULL)
    {
    }
        
    bool operator==(const DefList& b) const
    {
        if(Defines != b.Defines) return false;
        if(NonDefines != b.NonDefines) return false;
        if(!Alternate != !b.Alternate) return false;
        if(Alternate && *Alternate != *b.Alternate) return false;
        return true;
    }
    bool operator!=(const DefList& b) const { return !operator==(b); }
    bool operator< (const DefList& b) const
    {
        if(Defines != b.Defines) return Defines < b.Defines;
        if(NonDefines != b.NonDefines) return NonDefines < b.NonDefines;
        return false;
    }
    
    void Begin()
    {
        std::string rule = GetRule();
        if(!rule.empty())
        {
            if(!Alternate
            && NonDefines.empty()
            && Defines.size() == 1)
            {
                fprintf(out, "#ifdef %s\n", Defines.begin()->first.c_str());
            }
            else if(!Alternate
            && Defines.empty()
            && NonDefines.size() == 1)
            {
                fprintf(out, "#ifndef %s\n", NonDefines.begin()->first.c_str());
            }
            else
                fprintf(out, "#if %s\n", rule.c_str());
        }
    }
    void End()
    {
        std::string rule = GetRule();
        if(!rule.empty())
            fprintf(out, "#endif\n");
    }
    
    void Assimilate(DefList& b);
    
    void ListDefs(std::set<struct DefList>& b);
    
    void Tidy()
    {
        std::map<std::string, unsigned>::iterator i, j;
        for(j = Defines.begin(); j != Defines.end(); j = i)
        {
            i = j; ++i;
            if(!j->second) Defines.erase(j); else j->second=1;
        }
        for(j = NonDefines.begin(); j != NonDefines.end(); j = i)
        {
            i = j; ++i;
            if(!j->second) NonDefines.erase(j); else j->second=1;
        }
        if(Alternate) Alternate->Tidy();
        while(Defines.empty() && NonDefines.empty() && Alternate)
        {
            Defines    = Alternate->Defines;
            NonDefines = Alternate->NonDefines;
            Alternate  = Alternate->Alternate;
        }
    }
private:
    const std::string GetRule() const
    {
        std::map<std::string, unsigned>::const_iterator i;
        std::string result;
        
        bool ands = false;
        for(i=Defines.begin(); i!=Defines.end(); ++i)
            //if(i->second)
            {
                if(!result.empty()) { ands = true; result += "&&"; }
                result += "defined(";
                result += i->first;
                if(i->second != 1) { char Buf[32];sprintf(Buf,"/*%u*/",i->second);result+=Buf;}
                result += ")";
            }
        for(i=NonDefines.begin(); i!=NonDefines.end(); ++i)
            //if(i->second)
            {
                if(!result.empty()) { ands = true; result += "&&"; }
                result += "!defined(";
                result += i->first;
                if(i->second != 1) { char Buf[32];sprintf(Buf,"/*%u*/",i->second);result+=Buf;}
                result += ")";
            }
        if(Alternate)
        {
            std::string alt = Alternate->GetRule();
            if(!alt.empty())
            {
                if(ands) result = std::string("(") + result + ")";
                result = result + " || " + alt;
            }
            else
                result = result + " || false";
        }
        return result;
    }
};

void DefList::ListDefs(std::set<DefList>& b)
{
    Tidy();
    DefList tmp;
    tmp.Defines    = Defines;
    tmp.NonDefines = NonDefines;
    tmp.Alternate = NULL;
    b.insert(tmp);
    if(Alternate) Alternate->ListDefs(b);
}

void DefList::Assimilate(DefList& b)
{
    std::set<DefList> list;
    ListDefs(list);
    b.ListDefs(list);
    
    struct DefList* target = this;
    for(std::set<DefList>::const_iterator i = list.begin(); ; )
    {
        target->Defines    = i->Defines;
        target->NonDefines = i->NonDefines;
        target->Alternate  = NULL;
        
        if(++i == list.end()) break;
        target->Alternate = new DefList;
        target = target->Alternate;
    }
}


struct AutoRefererLink: public CodeNodePtr
{
    CodeNodePtr   owner;
    
    AutoRefererLink();
    ~AutoRefererLink();
    AutoRefererLink& operator= (CodeNode *value)
    {
        Unlink();
        CodeNodePtr::operator= (value);
        Link();
        return *this;
    }
    void SetOwner(CodeNode *p);
private:
    void Link();
    void Unlink();
    
    AutoRefererLink(const AutoRefererLink& b);
    
    void operator= (const AutoRefererLink& b);
};

static const int CallTypeJSR = 1;
static const int CallTypeWORD= 2;
static const int CallTypeJMPX= 3;
struct Jump
{
    int Is_Call;
    bool Is_C;
    bool Is_SET;
    
    //  Is_C Is_SET Is_Call
    //     x x      1       jsr
    //     x x      2       .word
    //     0 0      0       bne
    //     0 1      0       beq
    //     1 0      0       bcc
    //     1 1      0       bcs

    AutoRefererLink target_ins;
    std::string target_label;
public:
    Jump()
    : Is_Call(0),
      Is_C(false),
      Is_SET(false)
    {
    }
    
    void Negate()
    {
        Is_SET = !Is_SET;
    }
    
    bool Is_NegateOf(const Jump& b) const
    {
        return Is_Call == b.Is_Call
            && Is_SET == !b.Is_SET
            && Is_C == b.Is_C;
    }
    bool Is_SameIns(const Jump& b) const
    {
        return Is_Call == b.Is_Call
            && Is_SET == b.Is_SET
            && Is_C == b.Is_C;
    }
};

struct CodeNode: public ptrable
{
    std::set<std::string> labels;
    std::set<std::string> important_labels;
    
    std::set<CodeNodePtr> referers;
    
    FlagState assumes;
    enum { t_undefined, t_code, t_jump } type;
    std::string code;
    
    Jump jump;
    
    AutoRefererLink next_ins;
    std::string     next_label;
    
    DefList defines;
public:
    CodeNode()
    : type(t_undefined)
    {
        next_ins.SetOwner(this);
        jump.target_ins.SetOwner(this);
    }
    
    ~CodeNode()
    {
        Terminate();
    }
    
    void SetCode(const std::string& s)
    {
        type = t_code;
        code = s;
    }
    
    void SetCall(const std::string& target, int calltype)
    {
        type = t_jump;
        jump.Is_Call      = calltype;
        jump.target_label = target;
    }
    void SetJump(const std::string& op, const std::string& target)
    {
        type = t_jump;
        jump.Is_Call = 0;
        jump.Is_C    = (op=="bcc" || op=="bcs");
        jump.Is_SET  = (op=="bcs" || op=="beq");
        jump.target_label = target;
    }
    
    void SetNextLabel(const std::string& s)
    {
        if(type == t_undefined)
        {
            type = t_code;
            code = "";
        }
        next_label = s;
    }
    
    void OptimizeWithNextNode(CodeNode* next)
    {
        if(type != t_jump) return;
        if(jump.Is_Call) return;
        
        if(jump.target_ins == next)
        {
            // Example:          Becomes:
            //   beq Y:bra X       bne X:bra Y
            // Y:                Y:
            // Where the "bra Y" will vanish to nonexistance.
            
            CodeNode *old_next = next_ins;
            
            next_ins    = NULL;
            jump.Negate();
            jump.target_ins   = old_next;
            jump.target_label = next_label;
            next_ins   = next;
            next_label = "";
        }
    }
    
    void KnownAs(const std::string& s, bool Keep)
    {
        labels.insert(s);
        if(Keep) important_labels.insert(s);
    }
    
    void KnowLabel(const std::string& s, CodeNode* ptr)
    {
        if(next_label == s)
        {
            next_label = "";
            next_ins   = ptr;
        }
        if(type == t_jump)
        {
            if(jump.target_label == s)
            {
                jump.target_label = "";
                jump.target_ins   = ptr;
            }
        }
    }
    
    bool IsFunctionallyEqual(const CodeNode& b, bool firstrun=true) const
    {
        /* NOT THREAD-SAFE! */
        /* This is for loop detection */
        typedef std::map<const CodeNode*, unsigned> loopdetectmap;
        static loopdetectmap left_done, right_done;
        if(firstrun)
        {
            left_done.clear();
            right_done.clear();
        }
        else
        {
            loopdetectmap::const_iterator
               L = left_done.find(this),
               R = right_done.find(&b);
            if(L != left_done.end())
            {
                if(R == right_done.end()) return false;
                return R->second == L->second;
            }
            if(R != right_done.end()) return false;
            left_done[this] = left_done.size();
            right_done[&b]  = right_done.size();
        }
        
        if(type != b.type) return false;
        
        if(!assumes.IsCompatible(b.assumes))
        {
            return false;
        }
        
        if(type == t_code && code != b.code) return false;
        
        if(next_label != b.next_label) return false;
        
        const AutoRefererLink* insA1 = &next_ins;
        const AutoRefererLink* insA2 = &jump.target_ins;
        const AutoRefererLink* insB1 = &b.next_ins;
        const AutoRefererLink* insB2 = &b.jump.target_ins;
        
        if(type == t_jump)
        {
            if(jump.Is_NegateOf(b.jump))
            {
                insA1 = &jump.target_ins;
                insA2 = &next_ins;
            }
            else if(!jump.Is_SameIns(b.jump))
            {
                return false;
            }
        }
        else
        {
            insA2 = NULL;
            insB2 = NULL;
        }
        
        if(insA1)
        {
            if(!*insA1 != !*insB1) return false;
            if(*insA1
            && &**insA1 != &**insB1
            && !(*insA1)->IsFunctionallyEqual(**insB1, false))
            {
                return false;
            }
        }
        if(insA2)
        {
            if(!*insA2 != !*insB2) return false;
            if(*insA2
            && &**insA2 != &**insB2
            && !(*insA2)->IsFunctionallyEqual(**insB2, false))
            {
                return false;
            }
        }
        return true;
    }
    
    void LabelLinks(CodeNode* ptr, const std::string& name)
    {
        if(next_ins == ptr) next_label = name;
        if(type == t_jump)
        {
            if(jump.target_ins == ptr) jump.target_label = name;
        }
    }
    
    void RedirectReferers(CodeNode* newlink)
    {
        typedef std::set<CodeNodePtr>::iterator ref_it;
        for(ref_it next, i = referers.begin();
                   i != referers.end();
                   i = next)
        {
            next = i; ++next;
            (*i)->UpdateLinks(this, newlink);
        }
        if(IsDead()) Terminate();
    }
    
    void UpdateLinks(CodeNode* oldlink, CodeNode* newlink)
    {
/*
        std::printf("next_ins=%p, old=%p, new=%p\n",
            (const void *)next_ins,
            oldlink,
            newlink);
*/
        if(next_ins == oldlink)
        {
            next_ins = newlink;
        }
        
        if(type == t_jump)
        {
            if(jump.target_ins == oldlink)
            {
                jump.target_ins = newlink;
            }
        }
    }
    
    void Assimilate(CodeNode& b)
    {
        typedef std::set<std::string>::const_iterator lab_it;
        
        /* Assimilate labels */
        for(lab_it i = b.important_labels.begin();
                   i != b.important_labels.end();
                   ++i)
        {
            important_labels.insert(*i);
        }
        b.important_labels.clear();
        
        if(defines != b.defines)
        {
            defines.Assimilate(b.defines);
        }
        
        std::set<const CodeNode *> seen;
        CodeNode *l1=next_ins,  *l2=b.next_ins;
        while(l1 && l2)
        {
            if(seen.find(l1) != seen.end()) break;
            seen.insert(l1);
            
            l1->defines.Assimilate(l2->defines);
            
            l1 = l1->next_ins;
            l2 = l2->next_ins;
        }

        //code += ";assimilated("+b.code+")";
    }
    
    void AssimilateChain(AutoRefererLink& link)
    {
        std::list<CodeNodePtr> AssimilateList;
        while(link && link->IsNop()) 
        {
            //link->code += ";assimilated by ("+code+")";
            AssimilateList.push_back(link);
            if(!link->next_ins) break;
            
            CodeNodePtr next = link->next_ins;
            
            link->RedirectReferers(link->next_ins);
            
            link = next;
        }
        for(std::list<CodeNodePtr>::const_iterator
            i = AssimilateList.begin();
            i != AssimilateList.end();
            ++i)
        {
            link->Assimilate(**i);
        }
    }
    
    void ThreadJumps()
    {
        AssimilateChain(next_ins);
        if(type == t_jump) AssimilateChain(jump.target_ins);
        
        if(IsNop())
        {
            // oops, we are to be assimilated...
            // by whom?
            if(next_ins)
            {
                // haa!
                next_ins->Assimilate(*this);
            }
        }
    }
    
    bool IsNop() const
    {
        if(type == t_code && code[0] == ';') return true;
        return type == t_undefined;
    }
    
    bool IsDead() const
    {
        return referers.empty()
            && important_labels.empty();
    }
    
    bool IsRootNode() const
    {
        /* Node is a possible rootnode if it's never used
         * with a shortjump.
         */
        typedef std::set<CodeNodePtr>::const_iterator ref_it;

        for(ref_it j = referers.begin();
                   j != referers.end();
                   ++j)
        {
            const CodeNode& node = **j;
            if(node.next_ins == this) return false;
            
            if(node.type == t_jump
            && node.jump.target_ins == this
            && !node.jump.Is_Call)
            {
                return false;
            }
        }
        return true;
    }
    
    bool IsJustAFollowup() const
    {
        /* Node is just a followup if the only way it's
         * ever referred is by being the next to something.
         */
        typedef std::set<CodeNodePtr>::const_iterator ref_it;

        for(ref_it j = referers.begin();
                   j != referers.end();
                   ++j)
        {
            const CodeNode& node = **j;
            /* Some other way of referring - not a followup */
            if(node.next_ins != this) return false;
        }
        return true;
    }
    
    bool IsCombinable() const
    {
        // Everything else is combinable but data.
        if(jump.Is_Call == CallTypeWORD) return false;
        return true;
    }
    
    void Terminate()
    {
        // Ensure nobody will be referred by this one.
        
        /*
        std::fprintf(out, ";%p killed (", this);
        Dump();
        std::fprintf(out, ")\n");
        */
        
        next_ins        = NULL;
        jump.target_ins = NULL;
    }
    
    unsigned InsLen() const
    {
        unsigned size = 0;
        if((next_ins && next_ins->referers.size() > 1)
        || (jump.target_ins && jump.target_ins->referers.size() > 1))
        {
            size += 2; // by average
        }
        if(code == "rts" || code == "asl"
        || code == "sec" || code == "clc"
        || code == "phx" || code == "plx"
        || code == "phy" || code == "ply"
        || code == "pha" || code == "pla") return size+1;
        
        if(code.find('#') != code.npos)
        {
            if(code.find('x') != code.npos
            || code.find('y') != code.npos)
            {
                return size + (assumes.XY == Assume16bitXY ? 3 : 2);
            }
            return size + (assumes.A == Assume16bitA ? 3 : 2);
        }
        if(code.find("jsr") != code.npos
        || code.find("brl") != code.npos) return size+3;
        return size+2; // good guess
    }
    
    void Dump()
    {
        typedef std::set<CodeNodePtr>::const_iterator ref_it;
        typedef std::set<std::string>::const_iterator lab_it;
        
        for(ref_it j = referers.begin();
                   j != referers.end();
                   ++j)
        {
            std::fprintf(out, "[%p]", (const void *)*j);
        }
        for(lab_it j = important_labels.begin();
                   j != important_labels.end();
                   ++j)
        {
            std::fprintf(out, "%s: ", j->c_str());
        }
        switch(type)
        {
            case CodeNode::t_code:
                std::fprintf(out, "%s", code.c_str());
                break;
            case CodeNode::t_jump:
            {
                const char *ins = "???";
                switch(jump.Is_Call)
                {
                    case CallTypeJSR:  ins = "jsr"; break;
                    case CallTypeWORD: ins = ".word"; break;
                    case CallTypeJMPX: ins = "jmp ("; break;
                    default:
                        ins = jump.Is_C
                            ? jump.Is_SET ? "bcs" : "bcc"
                            : jump.Is_SET ? "beq" : "bne";
                }
                std::fprintf(out, "%s ", ins);
                if(!jump.target_label.empty())
                    std::fprintf(out, "%s", jump.target_label.c_str());
                else
                    std::fprintf(out, "%p", (const void *)jump.target_ins);
                if(jump.Is_Call == CallTypeJMPX)
                {
                    std::fprintf(out, ",x)");
                }
                break;
            }
            default:
                break;
        }
        if(!next_label.empty())
            std::fprintf(out, " -> %s", next_label.c_str());
        else if(next_ins)
            std::fprintf(out, " -> %p", (const void *)next_ins);
    }
    
    void FinishNode()
    {
        // Remove redudant bitness information.
        // This helps the combiner.
        
        bool need_x = false;
        bool need_a = false;
        if(code.find('#') != code.npos)
        {
            std::string ins = code.substr(0, 3);
            
            if(ins == "cpx" || ins == "cpy" 
            || ins == "ldx" || ins == "ldy") need_x = true;
            if(ins == "adc" || ins == "and"
            || ins == "bit" || ins == "cmp"
            || ins == "eor" || ins == "lda"
            || ins == "ora" || ins == "sbc") need_a = true;
        }
        if(!need_x) assumes.XY = UnknownXY;
        else if(assumes.XY == UnknownXY)
            code += "; unknown XY";
        if(!need_a) assumes.A  = UnknownA;
        else if(assumes.A == UnknownA)
            code += "; unknown A";
    }
private:
    CodeNode(const CodeNode &b);
    void operator= (const CodeNode &b);
};

AutoRefererLink::AutoRefererLink()
{
}
AutoRefererLink::~AutoRefererLink()
{
    Unlink();
}
void AutoRefererLink::SetOwner(CodeNode* p)
{
    owner = p;
}
void AutoRefererLink::Unlink()
{
    CodeNode *link = *this;
    if(link)
    {    
        if(link->referers.find(owner)
        == link->referers.end())
        {
            std::fprintf(out, ";Unlink error\n");
        }
        link->referers.erase(owner);
    }
}
void AutoRefererLink::Link()
{
    CodeNode *link = *this;
    if(link)
    {
        if(link->referers.find(owner)
        != link->referers.end())
        {
            std::fprintf(out, ";Link error\n");
        }
        link->referers.insert(owner);
    }
}

struct CodeChain
{
    typedef std::vector<CodeNodePtr> nodelist_t;
    typedef std::set<CodeNode*> seenlist_t;
    
    typedef nodelist_t::iterator nod_it;
    typedef nodelist_t::const_iterator nod_cit;
    typedef seenlist_t::const_iterator cand_it;
    nodelist_t nodes;
    seenlist_t contents;
    
    double EvaluateHappiness(const nodelist_t& nodes) const
    {
        double happiness = 0;
        
        std::map<const CodeNode*, int> nodepositions;
        
        int pos=0;
        for(nod_cit i = nodes.begin(); i != nodes.end(); ++i)
            nodepositions[*i] = pos++;
        
        pos=0;
        double count = 1;
        for(nod_cit i = nodes.begin(); i != nodes.end(); ++i)
        {
            const CodeNode& node = **i;
            if(node.next_ins)
            {
                int next_pos = nodepositions[node.next_ins];
                if(next_pos != pos+1)
                {
                    int offset = next_pos-pos; if(offset < 0)offset=-offset;
                    happiness -= 2;
                    ++count;
                    /*
                    if(offset >= 40)
                        happiness -= (offset-40.0)*(offset-40.0) /  5.0;
                    */
                }
            }
            if(node.jump.target_ins && !node.jump.Is_Call)
            {
                int target_pos = nodepositions[node.jump.target_ins];

                if(target_pos == pos+1)
                {
                    happiness += 2;
                }
                else
                {
                    int offset = target_pos-pos; if(offset < 0)offset=-offset;
                    happiness -= 2;
                    
                    if(offset >= 40)
                    {
                        double fac = (offset - 40);
                        happiness -= fac;
                        ++count;
                    }
                }
            }
            ++pos;
        }
        return happiness * count;
    }
    
    /*
0505Warp= Jaa se koodi sellaisiin lohkoihin, joissa on vain peräkkäistä
          koodia (mutta ei hyppykäskyjä, paitsi viimeinen komento lohkossa)
          ja kokeile kaikkien näiden lohkojen järjestyksiä. Tosin sekin on
          hidasta paitsi jos lohkoja on luokkaa alle 20 :)
    */
    
    void Reorder()
    {
        std::random_shuffle(nodes.begin(), nodes.end());
        
        /* TODO: Verify that all nodes have their
         * next_ins and jump.target_ins listed here!
         */
        
        for(nod_it i = nodes.begin(); i != nodes.end(); ++i)
            contents.insert(*i);
        
        /* FIXME: rootnodes are not the only good thing to start from.
         * For example, this is possible and often even good:
         *
         *    L1:   clc
         *          rts
         *    func: cmp #1
         *          bcc L1
         *          sec
         *          rts
         */
        
        double best_happiness = 0;
        double base_happiness = 0;
        bool first = true;
        nodelist_t result;
        for(nod_it i = nodes.begin(); i != nodes.end(); ++i)
        {
            if((*i)->IsRootNode()
            || !(*i)->IsJustAFollowup())
            {
                nodelist_t tmpresult;
                seenlist_t tmpseen;
                
                FindMoreNodes(tmpresult, tmpseen, *i);

                double happiness = EvaluateHappiness(tmpresult);
                
                if(first || happiness > best_happiness)
                {
                    result = tmpresult;
                    first  = false;
                    best_happiness = happiness;
                }
                if(best_happiness >= base_happiness) break; // enough
            }
        }
        
        nodes = result;
    }
private:
    void FindMoreNodes(nodelist_t& result, seenlist_t& seen, CodeNode* node)
    {
        typedef std::set<CodeNodePtr>::const_iterator ref_it;
    MoreNodes:
        result.push_back(node);
        seen.insert(node);
        
        seenlist_t candidates;
        
        /* The next instruction is a logical choice */
        if(node->next_ins)
        {
            candidates.insert(node->next_ins);

            /* But if the next instruction is a very common
             * target, anything else could be considerable
             * as well... */
            if(node->next_ins->referers.size() != 1)
            {
                for(nod_it i = nodes.begin(); i != nodes.end(); ++i)
                    if(!(*i)->IsJustAFollowup())
                        candidates.insert(*i);
            }
            CancelSeen(candidates, seen);
        }
        
        if(node->jump.target_ins && !node->jump.Is_Call)
        {
            /* Branch target is also an option */
            candidates.insert(node->jump.target_ins);
            CancelSeen(candidates, seen);
        }
        
        /* If we still have nothing... */
        
        if(candidates.empty())
        {
            for(nod_it i = nodes.begin(); i != nodes.end(); ++i)
                if(!(*i)->IsJustAFollowup())
                    candidates.insert(*i);
            CancelSeen(candidates, seen);
        }
        
        if(candidates.empty())
        {
            for(nod_it i = nodes.begin(); i != nodes.end(); ++i)
                if((*i)->IsRootNode())
                    candidates.insert(*i);

            CancelSeen(candidates, seen);
        }

        if(candidates.empty())
        {
            for(ref_it i = node->referers.begin();
                       i != node->referers.end();
                       ++i)
            {
                if((*i)->next_ins == node || !(*i)->jump.Is_Call)
                {
                    candidates.insert(*i);
                }
            }
            CancelSeen(candidates, seen);
        }
            
        if(candidates.empty())
        {
            for(nod_it i = nodes.begin(); i != nodes.end(); ++i)
                candidates.insert(*i);
            CancelSeen(candidates, seen);
        }
        
        if(candidates.size() == 1)
        {
            node = *candidates.begin();
            candidates.clear();
            goto MoreNodes;
        }
        
        if(candidates.empty())
        {
            // Nothing left!
            return;
        }
        
        /* Handle all candidates and see which of them is best. */
        
        double   best_happiness = 0;
        unsigned n_seen = 0;
        
        double base_happiness = EvaluateHappiness(result);
        
        fprintf(stderr, "Evaluating %u candidates...\n", candidates.size());
        for(cand_it j = candidates.begin();
                    j != candidates.end();
                    ++j)
        {
            nodelist_t tmpresult = result;
            seenlist_t tmpseen   = seen;
            
            FindMoreNodes(tmpresult, tmpseen, *j);
            
            double happiness = EvaluateHappiness(tmpresult);
            
            if(!n_seen || happiness > best_happiness)
            {
                best_happiness = happiness;
                result = tmpresult;
                seen   = tmpseen;
            }
            ++n_seen;
            if(n_seen >= 1)break; // enough
            if(best_happiness >= base_happiness) break; // Enough!
        }
        fprintf(stderr, "...done\n");
    }
    
    void CancelSeen(seenlist_t& candidates, const seenlist_t& seen)
    {
        for(cand_it j,i = candidates.begin(); i != candidates.end(); i=j)
        {
            j = i; ++j;
            if(seen.find(*i) != seen.end()
            || contents.find(*i) == contents.end())
            {
                candidates.erase(i);
            }
        }
    }
};

static CodeNode* CurNode = NULL;
static std::list<CodeNodePtr> AllNodes;
static bool NodeFinished = false;
static std::set<std::string> KeepList;
static FlagState assumes;

static DefList defines;
static std::list<std::pair<std::string, bool> > defhistory;

static void GenerateRootNode()
{
    CurNode = new CodeNode;
    AllNodes.push_back(CurNode);
    NodeFinished = false;
}

static CodeNode* GetCurrentNode()
{
    if(!CurNode)
    {
        GenerateRootNode();
    }
    if(NodeFinished)
    {
        CodeNode* oldnode = CurNode;
        
        if(oldnode) oldnode->FinishNode();
        GenerateRootNode();
        
        oldnode->next_ins = CurNode;
        
        NodeFinished = false;
    }
    
    if(CurNode) CurNode->assumes = assumes;
    if(CurNode) CurNode->defines = defines;
    return CurNode;
}

static void CloseChain()
{
    if(CurNode) CurNode->FinishNode();
    CurNode = NULL;
}

static void FinishNode()
{
    if(CurNode && CurNode->type != CodeNode::t_undefined)
    {
        NodeFinished = true;
    }
}



static void CheckKeepList()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    typedef std::set<std::string>::const_iterator lab_it;
    
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        for(lab_it j = (*i)->labels.begin();
                   j != (*i)->labels.end();
                   ++j)
        {
            if(KeepList.find(*j) != KeepList.end())
                (*i)->important_labels.insert(*j);
        }
    }
    KeepList.clear();
}

static void LinkNodes()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    typedef std::set<std::string>::const_iterator lab_it;
    
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        for(lab_it j = (*i)->labels.begin();
                   j != (*i)->labels.end();
                   ++j)
        {
            for(vec_it k = AllNodes.begin();
                       k != AllNodes.end();
                       ++k)
            {
                (*k)->KnowLabel(*j, *i);
            }
        }
        (*i)->labels.clear();
    }
}

static void ThreadJumps()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        (*i)->ThreadJumps();
    }
}

static void CombineNodes()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;

Restart:
    bool DidCombines = false;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        CodeNode* chain = *i;
        if(!chain->IsCombinable())
        {
            continue;
        }
        
        if(chain->IsJustAFollowup())
        {
            unsigned chain_length = 0;
            const unsigned required_length = 3;
            
            std::set<CodeNode*> seen;
            
            /* This is slow. */
            while(chain)
            {
                if(seen.find(chain) != seen.end())
                {
                    // loop detected
                    chain_length = 10; // long enough
                    break;
                }
                seen.insert(chain);
                
                chain_length += chain->InsLen(); // average ins len
                if(chain_length >= required_length)
                {
                    // Ok it's long enough
                    break;
                }
                chain = chain->next_ins;
            }
            
            if(chain_length < required_length) continue;
        }
        
        vec_it first = i; ++first;
        for(vec_it next, j=first; j != AllNodes.end(); j = next)
        {
            next = j; ++next;

            if(j == i) continue;
            
            if((*i)->IsFunctionallyEqual(**j))
            {
                /*
                std::printf("One combine - %p replaced with %p\n",
                    (const void *)*j,
                    (const void *)*i);
                */
                (*i)->Assimilate(**j);

                /* Everyone who linked to this instruction,
                 * is now urged to link to the new instruction
                 * instead.
                 */
                for(vec_it k = AllNodes.begin();
                           k != AllNodes.end();
                           ++k)
                {
                    (*k)->UpdateLinks(*j, *i);
                }
                
                (*j)->Terminate();
                
                AllNodes.erase(j);
                
                DidCombines = true;
            }
        }
    }
    if(DidCombines) goto Restart;
}

static void ReorderNodes()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    typedef std::set<CodeNodePtr>::const_iterator ref_it;
    typedef std::list<CodeChain>::iterator cha_it;
    typedef CodeChain::nod_it nod_it;
    
    std::list<CodeChain> chains;
    
    std::set<CodeNode *> done;
    
    while(!AllNodes.empty())
    {
        /* Find a root node. */
        
        CodeNode *rootnode = NULL;
        for(vec_it i = AllNodes.begin();
                   i != AllNodes.end();
                   ++i)
        {
            if((*i)->IsRootNode())
            {
                rootnode = *i;
                break;
            }
        }
        
        if(!rootnode)
        {
            fprintf(out,
                    ";Ouch, none of %u nodes were rootnodes!\n", AllNodes.size());
            return;
        }
        
        CodeChain chain;

        std::set<CodeNode *> todo;
        todo.insert(rootnode);
        
        while(!todo.empty())
        {
            CodeNode& node = **todo.begin();
            chain.nodes.push_back(&node);
            todo.erase(todo.begin());
            done.insert(&node);
            
            if(node.next_ins
            && !node.next_ins->IsRootNode()
            && done.find(node.next_ins) == done.end())
            {
                todo.insert(node.next_ins);
            }
            if(node.jump.target_ins
            && !node.jump.Is_Call
            && !node.jump.target_ins->IsRootNode()
            && done.find(node.jump.target_ins) == done.end())
            {
                todo.insert(node.jump.target_ins);
            }
            
            for(ref_it i = node.referers.begin();
                       i != node.referers.end();
                       ++i)
            {
                CodeNode& refnode = **i;
                if(refnode.next_ins == &node
                || refnode.jump.target_ins == &node && !refnode.jump.Is_Call)
                {
                    if(done.find(&refnode) == done.end())
                        todo.insert(&refnode);
                }
            }
        }
        
        chains.push_back(chain);

    ReFind:
        for(vec_it i = AllNodes.begin();
                   i != AllNodes.end();
                   ++i)
        {
            if(done.find(*i) != done.end()) { AllNodes.erase(i); goto ReFind; }
        }
    }
    /* AllNodes is empty now */
    
    for(cha_it i = chains.begin(); i != chains.end(); ++i)
    {
        CodeNode *tmp = new CodeNode;
        tmp->SetCode(";new chain");
        AllNodes.push_back(tmp);
        
        i->Reorder();
        
        unsigned roots = 0;
        CodeChain::nodelist_t& vec = i->nodes;
        for(nod_it j = vec.begin(); j != vec.end(); ++j)
        {
            if((*j)->IsRootNode())
            {
                CodeNode *tmp = new CodeNode;
                char Buf[64]; std::sprintf(Buf, ";rootnode #%u", ++roots);
                tmp->SetCode(Buf);
                tmp->next_ins = *j;
                AllNodes.push_back(tmp);
            }
            AllNodes.push_back(*j);
        }
    }
}
static void OptimizeWithNextNodes()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        vec_it next = i; ++next;
        (*i)->OptimizeWithNextNode(*next);
    }
}

static void EliminateDeadInstructions()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;
FindMore:
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        CodeNode& node = **i;
        if(node.IsDead())
        {
            // Dead, erase it.

            node.Terminate();
            AllNodes.erase(i);
            
            // Restart scanning, as this might have
            // created more dead statements.
            goto FindMore;
        }
    }
}

static void DumpNodes()
{
    typedef std::list<CodeNodePtr>::const_iterator vec_it;
    typedef std::set<CodeNodePtr>::const_iterator ref_it;
    typedef std::set<std::string>::const_iterator lab_it;

    std::fprintf(out, ";-------------\n");
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        std::fprintf(out, ";%p ", (const void *)*i);
        (*i)->Dump();
        std::fprintf(out, "\n");
    }
    std::fprintf(out, ";-------------\n");
}

static void TidyDefines()
{
    typedef std::list<CodeNodePtr>::iterator vec_it;

    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        (*i)->defines.Tidy();
    }
}

static void AssignLabels()
{
    typedef std::list<CodeNodePtr>::const_iterator vec_it;
    typedef std::set<CodeNodePtr>::const_iterator ref_it;

    CodeNode *prev = NULL;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               prev = *i++)
    {
        CodeNode& node = **i;

        if(node.referers.empty())
        {
            // This node was not referred - thus it doesn't need new labels.
            continue;
        }
        
        if(node.referers.size() == 1
        && *node.referers.begin() == prev
        && prev->next_ins == &node)
        {
            // Only one referer.
            // And the only referer was the previous instruction.
            // No label needed
            continue;
        }

        if(node.important_labels.empty())
        {
            // Didn't have labels, create one.
            node.important_labels.insert(GenLabel());
        }
        
        const std::string& label = *node.important_labels.begin();
        
        for(ref_it j = node.referers.begin();
                   j != node.referers.end();
                   ++j)
        {
            if(*j != prev || prev->next_ins != &node)
            {
                (*j)->LabelLinks(&node, label);
            }

        }
    }
}

static void OutputCode(const std::list<std::string>& code)
{
    for(std::list<std::string>::const_iterator
        i = code.begin();
        i != code.end();
        ++i)
    {
        std::fprintf(out, "%s\n", i->c_str());
    }
}

static void OutputNode(const CodeNode& node, std::list<std::string>& target)
{
    typedef std::set<std::string>::const_iterator lab_it;
    
    if(node.defines != defines)
    {
#if GENERATE_MACROES
        FindPatterns(target);
#endif
        OutputCode(target);
        target.clear();

        defines.End();
        defines = node.defines;
        defines.Begin();
        assumes.XY = UnknownXY;
        assumes.A  = UnknownA;
    }
    
    for(lab_it j = node.important_labels.begin();
               j != node.important_labels.end();
               ++j)
    {
        target.push_back(*j + ":");
    }
    
    std::string line = "\t";

    if(node.assumes.XY != assumes.XY)
        switch(node.assumes.XY)
        {
            case Assume8bitXY:
                assumes.XY = node.assumes.XY;
                line += ".xs : ";
                break;
            case Assume16bitXY:
                assumes.XY = node.assumes.XY;
                line += ".xl : ";
                break;
            case UnknownXY:
                // What here?
                break;
        }
    if(node.assumes.A != assumes.A)
        switch(node.assumes.A)
        {
            case Assume8bitA:
                assumes.A = node.assumes.A;
                line += ".as : ";
                break;
            case Assume16bitA:
                assumes.A = node.assumes.A;
                line += ".al : ";
                break;
            case UnknownA:
                // What here?
                break;
        }

    switch(node.type)
    {
        case CodeNode::t_code:
            line += node.code;
            break;
        case CodeNode::t_jump:
        {
            const char *ins = "???";
            switch(node.jump.Is_Call)
            {
                case CallTypeJSR:  ins = "jsr"; break;
                case CallTypeWORD: ins = ".word"; break;
                case CallTypeJMPX: ins = "jmp ("; break;
                default:
                    ins = node.jump.Is_C
                     ? node.jump.Is_SET ? "bcs" : "bcc"
                     : node.jump.Is_SET ? "beq" : "bne";
            }
            line += ins;
            line += ' ';
            line += node.jump.target_label.empty()
                ?    "???"
                :    node.jump.target_label;
            if(node.jump.Is_Call == CallTypeJMPX)
            {
                line += ",x)";
            }
            break;
        }
        default:
            break;
    }
    
    target.push_back(line);
    
    if(!node.next_label.empty())
    {
        line = "\t";
        line += node.next_ins ? "bra" : "brl";
        line += ' ';
        line += node.next_label;
        target.push_back(line);
    }
}

static void OutputNodes()
{
    AssignLabels();
    
    assumes.XY = UnknownXY;
    assumes.A  = UnknownA;
    
    defines = DefList();
    defines.Begin();
    
    std::list<std::string> code;
    
    typedef std::list<CodeNodePtr>::const_iterator vec_it;
    for(vec_it i = AllNodes.begin();
               i != AllNodes.end();
               ++i)
    {
        vec_it next = i; ++next;
        OutputNode(**i, code);
    }
    
#if GENERATE_MACROES
    FindPatterns(code);
#endif
    OutputCode(code);
    code.clear();

    defines.End();
}

void BeginCode(std::FILE *output)
{
    out = output ? output : stdout;
    
    EmitSegment("code");
    
    CurNode = NULL;
    AllNodes.clear();
    NodeFinished = false;
    assumes.XY = UnknownXY;
    assumes.A  = UnknownA;
    
    defines = DefList();
    defhistory.clear();
    
}

void EndCode()
{
    while(!defhistory.empty()) EmitEndIfDef();
    
    FinishNode();
    CheckKeepList();
    CloseChain();
    LinkNodes();
#if THREAD_JUMPS
    ThreadJumps();
#endif
#if COMBINE_NODES
    CombineNodes();
#endif
#if ELIMINATE_DEAD
    EliminateDeadInstructions();
#endif
#if REORDER_NODES
    ReorderNodes();
#endif
#if NEGATE_JUMPS
    OptimizeWithNextNodes();
#endif
    TidyDefines();
#if DEBUG_NODES
    DumpNodes();
#endif
    OutputNodes();
    
    //AllNodes.clear();
}


void Emit(const std::string& s, FlagWantedStateA A, FlagWantedStateXY XY)
{
    std::string code = s;
    
    int sep = 0;
    int rep = 0;
    if(A == Want8bitA && assumes.A != Assume8bitA) { sep |= 0x20; assumes.A = Assume8bitA; }
    if(A ==Want16bitA && assumes.A !=Assume16bitA) { rep |= 0x20; assumes.A =Assume16bitA; }
    if(XY== Want8bitXY&& assumes.XY!= Assume8bitXY){ sep |= 0x10; assumes.XY= Assume8bitXY;}
    if(XY==Want16bitXY&& assumes.XY!=Assume16bitXY){ rep |= 0x10; assumes.XY=Assume16bitXY;}
    
    // sep and rep will be given separate nodes.
    // This could help the optimizer.
    if(sep != 0)
    {
        if(code == "sec") sep |= 1; // set carry too
        
        char Buf[32];
        sprintf(Buf, "sep #$%X", sep);
        FinishNode(); // Finish the previous node.
        GetCurrentNode()->SetCode(Buf);
    }
    if(rep != 0)
    {
        if(code == "clc") rep |= 1; // clear carry too
        
        char Buf[32];
        sprintf(Buf, "rep #$%X", rep);
        FinishNode(); // Finish the previous node.
        GetCurrentNode()->SetCode(Buf);
    }
    
    if((sep&1) || (rep&1))
    {
        // Already handled a clc/sec, nothing to do.
        return;
    }
    
    FinishNode(); // Finish the previous node.
    GetCurrentNode()->SetCode(code);
    
    if(code == "rts") { EmitBarrier(); }
}
void EmitLabel(const std::string& label)
{
    // Make sure this label will not be assigned to the PREVIOUS ins.
    FinishNode();
    
    GetCurrentNode()->KnownAs(label, false);
    CurNode->SetCode(";label");
}
void KeepLabel(const std::string& label)
{
    KeepList.insert(label);
}
void EmitBarrier()
{
    // Note that calling this function repetitively isn't an error,
    // and shouldn't increase memory usage.
    
    CloseChain();
}
void EmitBranch(const std::string& s, const std::string& target)
{
    if(s == "bra")
    {
        // No FinishNode here.
        // The followup label will be attached to the previous instruction.
        GetCurrentNode()->SetNextLabel(target);
        EmitBarrier();
    }
    else if(s == "jsr")
    {
        FinishNode(); // Finish the previous node.
        GetCurrentNode()->SetCall(target, CallTypeJSR);
    }
    else if(s == ".word")
    {
        FinishNode(); // Finish the previous node.
        GetCurrentNode()->SetCall(target, CallTypeWORD);
    }
    else if(s == ".jmpx")
    {
        FinishNode(); // Finish the previous node.
        GetCurrentNode()->SetCall(target, CallTypeJMPX);
        EmitBarrier();
    }
    else
    {
        FinishNode(); // Finish the previous node.
        GetCurrentNode()->SetJump(s, target);
    }
}

const std::string GenLabel()
{
    static unsigned counter = 1;
    char Buf[64];
    sprintf(Buf, "L%u", counter++);
    return Buf;
}

void Assume(FlagAssumedStateA A, FlagAssumedStateXY XY)
{
    assumes.A  = A;
    assumes.XY = XY;
}

const FlagAssumption GetAssumption()
{
    return FlagAssumption(assumes.A, assumes.XY);
}

void EmitIfDef(const std::string& s)
{
    ++defines.Defines[s];
    defhistory.push_front(make_pair(s, true));
}
void EmitIfNDef(const std::string& s)
{
    ++defines.NonDefines[s];
    defhistory.push_front(make_pair(s, false));
}
void EmitEndIfDef()
{
    if(!defhistory.empty())
    {
        const std::string& s = defhistory.begin()->first;
        bool           isdef = defhistory.begin()->second;
        
        if(isdef)
            --defines.Defines[s];
        else
            --defines.NonDefines[s];
        
        defhistory.pop_front();
    }
}

void EmitSegment(const std::string& )
{
    /* Segments are ignored */
}
