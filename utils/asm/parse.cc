#include <cctype>

#include "parse.hh"
#include "expr.hh"
#include "insdata.hh"

const std::string ParseToken(ParseData& data)
{
    std::string token;
    data.SkipSpace();
    char c = data.PeekC();
    if(c == '$')
    {
    Hex:
        token += data.GetC();
        for(;;)
        {
            c = data.PeekC();
            if( (c >= '0' && c <= '9')
             || (c >= 'A' && c <= 'F')
             || (c >= 'a' && c <= 'f')
             || (c == '-' && token.size()==1)
              )
                token += data.GetC();
            else
                break;
        }
    }
    else if(c == '-' || (c >= '0' && c <= '9'))
    {
        if(c == '-')
        {
            ParseData::StateType state = data.SaveState();
            data.GetC();
            char c = data.PeekC();
            data.LoadState(state);
            if(c == '$')
            {
                token += c;
                goto Hex;
            }
            bool ok = c >= '0' && c <= '9';
            if(!ok) return token;
        }
        for(;;)
        {
            token += data.GetC();
            if(data.EOF()) break;
            c = data.PeekC();
            if(c < '0' || c > '9') break;
        }
    }
    if(isalpha(c) || c == '_')
    {
        for(;;)
        {
            token += data.GetC();
            if(data.EOF()) break;
            c = data.PeekC();
            if(c != '_' && !isalnum(c)) break;
        }
    }
    return token;
}

expression* RealParseExpression(ParseData& data, int prio=0)
{
    std::string s = ParseToken(data);
    
    expression* left = NULL;
    
    if(s.empty()) /* If no number or symbol */
    {
        char c = data.PeekC();
        if(c == '-') // negation
        {
            ParseData::StateType state = data.SaveState();
            left = RealParseExpression(data, prio_negate);
            data.GetC(); // eat
            if(!left) { data.LoadState(state); return left; }
            left = new expr_negate(left);
        }
        if(c == '~')
        {
            ParseData::StateType state = data.SaveState();
            data.GetC(); // eat
            left = RealParseExpression(data, prio_bitnot);
            if(!left) { data.LoadState(state); return left; }
            left = new expr_bitnot(left);
        }
        else if(c == '(')
        {
            ParseData::StateType state = data.SaveState();
            data.GetC(); // eat
            left = RealParseExpression(data, 0);
            data.SkipSpace();
            if(data.PeekC() == ')') data.GetC();
            else if(left) { delete left; left = NULL; }
            if(!left) { data.LoadState(state); return left; }
        }
        else
        {
            // invalid char
            return left;
        }
    }
    else if(s[0] == '-' || s[0] == '$' || (s[0] >= '0' && s[0] <= '9'))
    {
        long value = 0;
        bool negative = false;
        // Number.
        if(s[0] == '$')
        {
            unsigned pos = 1;
            if(s[1] == '-') { ++pos; negative = true; }
            
            for(; pos < s.size(); ++pos)
            {
                value = value*16;
                if(s[pos] >= 'A' && s[pos] <= 'F') value += 10+s[pos]-'A';
                if(s[pos] >= 'a' && s[pos] <= 'f') value += 10+s[pos]-'a';
                if(s[pos] >= '0' && s[pos] <= '9') value +=    s[pos]-'0';
            }
        }
        else
        {
            unsigned pos = 0;
            if(s[0] == '-') { negative = true; ++pos; }
            for(; pos < s.size(); ++pos)
                value = value*10 + (s[pos] - '0');
        }
        
        if(negative) value = -value;
        left = new expr_number(value);
    }
    else
    {
        if(IsReservedWord(s))
        {
            /* Attempt to use a reserved as variable name */
            return left;
        }
        
        left = new expr_label(s);
    }

    data.SkipSpace();
    if(!left) return left;
    
Reop:
    if(!data.EOF())
    {
        #define op2(reqprio, c1,c2, exprtype) \
            if(prio < reqprio && data.PeekC() == c1) \
            { \
                ParseData::StateType state = data.SaveState(); \
                bool ok = true; \
                if(c2 != 0) \
                { \
                    data.GetC(); char c = data.PeekC(); \
                    data.LoadState(state); \
                    ok = c == c2; \
                    if(ok) data.GetC(); \
                } \
                if(ok) \
                { \
                    data.GetC(); \
                    expression *right = RealParseExpression(data, reqprio); \
                    if(!right) \
                    { \
                        data.LoadState(state); \
                        return left; \
                    } \
                    left = new exprtype(left, right); \
                    if(left->IsConst()) \
                    { \
                        right = new expr_number(left->GetConst()); \
                        delete left; \
                        left = right; \
                    } \
                    goto Reop; \
            }   }
        
        op2(prio_addsub, '+',   0, expr_plus);
        op2(prio_addsub, '-',   0, expr_minus);
        op2(prio_divmul, '*',   0, expr_mul);
        op2(prio_divmul, '/',   0, expr_div);
        op2(prio_shifts, '<', '<', expr_shl);
        op2(prio_shifts, '>', '>', expr_shr);
        op2(prio_bitand, '&',   0, expr_bitand);
        op2(prio_bitor,  '|',   0, expr_bitor);
        op2(prio_bitxor, '^',   0, expr_bitxor);
    }
    return left;
}

bool ParseExpression(ParseData& data, ins_parameter& result)
{
    data.SkipSpace();
    
    if(data.EOF()) return false;
    
    char prefix = data.PeekC();
    if(prefix == FORCE_LOBYTE
    || prefix == FORCE_HIBYTE
    || prefix == FORCE_ABSWORD
    || prefix == FORCE_LONG
    || prefix == FORCE_SEGBYTE
      )
    {
        // good prefix
        data.GetC();
    }
    else
    {
        // no prefix
        prefix = 0;
    }
    
    expression* e = RealParseExpression(data);
    
    if(e)
    {
        if(expr_minus *m = dynamic_cast<expr_minus *> (e))
        {
            // The rightmost element must be constant.
            if(m->right->IsConst())
            {
                // Convert into a sum.
                long value = m->right->GetConst();
                e = new expr_plus(m->left, new expr_number(-value));
                m->left = NULL; // so it won't be deleted
                delete m;
            }
        }
        
        if(!dynamic_cast<expr_number *> (e)
        && !dynamic_cast<expr_label *> (e))
        {
            if(expr_plus *p = dynamic_cast<expr_plus *> (e))
            {
                bool left_const = p->left->IsConst();
                bool right_const = p->right->IsConst();
                
                if(left_const && right_const)
                {
                    /* This should have been optimized */
                    std::fprintf(stderr, "Internal error\n");
                }
                else if((left_const && dynamic_cast<expr_label *> (p->right))
                    ||  (right_const && dynamic_cast<expr_label *> (p->left))
                       )
                {
                    if(left_const)
                    {
                        /* Reorder them so that the constant is always on right */
                        expression *tmp = p->left;
                        p->left = p->right;
                        p->right = tmp;
                    }
                    /* ok */
                }
                else
                    goto InvalidMath;
            }
            else
            {
    InvalidMath:
                /* Invalid pointer arithmetic */
                std::fprintf(stderr, "Invalid pointer arithmetic: '%s'\n",
                    e->Dump().c_str());
                delete e;
                e = NULL;
            }
        }
    }

    //std::fprintf(stderr, "ParseExpression returned: '%s'\n", result.Dump().c_str());
    
    result.prefix = prefix;
    result.exp    = e;
    
    return e != NULL;
}

tristate ParseAddrMode(ParseData& data, unsigned modenum,
                       ins_parameter& p1, ins_parameter& p2)
{
    #define ParseReq(s) \
        for(const char *q = s; *q; ++q, data.GetC()) { \
            data.SkipSpace(); if(data.PeekC() != *q) return false; }
    #define ParseNotAllow(c) \
        data.SkipSpace(); if(data.PeekC() == c) return false
    #define ParseOptional(c) \
        
    #define ParseExpr(p) \
        if(!ParseExpression(data, p)) return false

    if(modenum >= AddrModeCount) return false;
    
    const AddrMode& modedata = AddrModes[modenum];
    
    if(modedata.forbid) { ParseNotAllow(modedata.forbid); }
    ParseReq(modedata.prereq);
    if(modedata.p1 != AddrMode::tNone) { ParseExpr(p1); }
    if(modedata.p2 != AddrMode::tNone) { ParseOptional(','); ParseExpr(p2); }
    ParseReq(modedata.postreq);
    
    data.SkipSpace();
    tristate result = data.EOF();
    switch(modedata.p1)
    {
        case AddrMode::tByte: result=result && p1.is_byte(); break;
        case AddrMode::tWord: result=result && p1.is_word(); break;
        case AddrMode::tLong: result=result && p1.is_long(); break;
        case AddrMode::tA: result=result && A_16bit ? p1.is_word() : p1.is_byte(); break;
        case AddrMode::tX: result=result && X_16bit ? p1.is_word() : p1.is_byte(); break;
        case AddrMode::tRel8: ;
        case AddrMode::tRel16: ;
        case AddrMode::tNone: ;
    }
    switch(modedata.p2)
    {
        case AddrMode::tByte: result=result && p2.is_byte(); break;
        case AddrMode::tWord: result=result && p2.is_word(); break;
        case AddrMode::tLong: result=result && p2.is_long(); break;
        case AddrMode::tA: result=result && A_16bit ? p2.is_word() : p2.is_byte(); break;
        case AddrMode::tX: result=result && X_16bit ? p2.is_word() : p2.is_byte(); break;
        case AddrMode::tRel8: ;
        case AddrMode::tRel16: ;
        case AddrMode::tNone: ;
    }

    return result;
}

bool IsDelimiter(char c)
{
    return c == ':' || c == '\r' || c == '\n';
}
