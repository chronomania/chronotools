#include "expr.hh"

#if 0
void ReplaceLabelWithValue(const std::string&s, long value, expression*& e)
{
    if(expr_label *l = dynamic_cast<expr_label*> (e))
    {
        if(l->GetName() == s)
        {
            delete e;
            e = new expr_number(value);
            return;
        }
    }
    e->DefineLabel(s, value);
    if(e->IsConst())
    {
        value = e->GetConst();
        delete e;
        e = new expr_number(value);
    }
}
#endif
