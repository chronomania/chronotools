#ifndef bqtTristateHH
#define bqtTristateHH

/*
 *  This module defines a bool-like type,
 *  that has three different values:
 *
 *    true, false and maybe.
 *
 *  Operator charts:
 *
 *           !     bool 
 *    true   false true    
 *    false  true  false
 *    maybe  maybe false
 *
 *           ==true ==false ==maybe !=true !=false !=maybe
 *    true   true   false   maybe   false  true    maybe
 *    false  false  true    maybe   true   false   maybe
 *    maybe  maybe  maybe   true    maybe  maybe   false
 *
 *           ||true ||false ||maybe &&true &&false &&maybe
 *    true   true   true    true    true   false   maybe
 *    false  true   false   maybe   false  false   false
 *    maybe  true   maybe   maybe   maybe  false   maybe
 *
 * Note:
 *   b || !b  doesn't equal to true, if b is "maybe".
 *
 * Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
 *
 */

class tristate
{
    // states: 0=false, 1=true, 2=unknown
public:
    tristate(bool b) : state(b?1:0) {}
    tristate(unsigned char v) : state(v) {}
    
    operator bool() const { return state != 0; }
    tristate operator! () const { return state==0?1:state==1?0:state; }
    tristate operator== (bool b) const { return state==2 ? 2 : state==b; }
    tristate operator!= (bool b) const { return state==2 ? 2 : state!=b; }
    tristate operator== (tristate b) const
    {
        if(b.state != state && (b.state > 1 || state > 1)) return 2;
        return b.state == state;
    }
    tristate operator!= (tristate b) const
    {
        if(b.state != state && (b.state > 1 || state > 1)) return 2;
        return b.state != state;
    }
    tristate operator|| (tristate b) const
    {
        if(b.state == 1 || state == 1) return 1; // certainly
        if(b.state > 1 || state > 1) return 2; // possibly
        return 0; // not
    }
    tristate operator&& (tristate b) const
    {
        if(b.state == 1 && state == 1) return 1; // certainly
        if(b.state == 0 || state == 0) return 0; // never
        return 2; // possibly
    }
    tristate operator&& (bool b) const { return b ? state : false; }
    tristate operator|| (bool b) const { return b ? true : state; }
    
    bool is_true() const { return state==1; }
    bool is_false() const { return state==0; }
    bool is_maybe() const { return state>1; }
    
    /*
    const tristate& operator&&= (tristate& b) { return *this = *this && b; }
    const tristate& operator||= (tristate& b) { return *this = *this || b; }
    const tristate& operator&&= (bool b) { return *this = *this && b; }
    const tristate& operator||= (bool b) { return *this = *this || b; }
    */
    
private:
    unsigned char state;
    
    // unimplemented operators:
    void operator< (tristate b) {}
    void operator> (tristate b) {}
    void operator<= (tristate b) {}
    void operator>= (tristate b) {}
    void operator- (tristate b) {}
    void operator+ (tristate b) {}

    tristate(int v) : state(v) {}
};

namespace {
const tristate maybe = (unsigned char)2;
}

#endif
