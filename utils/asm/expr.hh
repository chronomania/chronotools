#ifndef bqt65asmExprHH
#define bqt65asmExprHH

#include <string>

class expression
{
public:
    virtual ~expression() { }
    virtual bool IsConst() const = 0;
    virtual long GetConst() const { return 0; }
    
    virtual const std::string Dump() const = 0;
};
class expr_number: public expression
{
protected:
    long value;
public:
    expr_number(long v): value(v) { }
    
    virtual bool IsConst() const { return true; }
    virtual long GetConst() const { return value; }

    virtual const std::string Dump() const
    {
        char Buf[512];
        if(value < 0)
            std::sprintf(Buf, "$-%lX", -value);
        else
            std::sprintf(Buf, "$%lX", value);
        return Buf;
    }
};
class expr_label: public expression
{
protected:
    std::string name;
public:
    expr_label(const std::string& s): name(s) { }
    
    virtual bool IsConst() const { return false; }

    virtual const std::string Dump() const { return name; }
    const std::string& GetName() const { return name; }
};
class expr_unary: public expression
{
public:
    expression* sub;
public:
    expr_unary(expression *s): sub(s) { }
    virtual bool IsConst() const { return sub->IsConst(); }

    virtual ~expr_unary() { delete sub; }
private:
    expr_unary(const expr_unary &b);
    void operator= (const expr_unary &b);
};
class expr_binary: public expression
{
public:
    expression* left;
    expression* right;
public:
    expr_binary(expression *l, expression *r): left(l), right(r) { }
    virtual bool IsConst() const { return left->IsConst() && right->IsConst(); }

    virtual ~expr_binary() { delete left; delete right; }
    
private:
    expr_binary(const expr_binary &b);
    void operator= (const expr_binary &b);
};

#define unary_class(classname, op, stringop) \
    class classname: public expr_unary \
    { \
    public: \
        classname(expression *s): expr_unary(s) { } \
        virtual long GetConst() const { return op sub->GetConst(); } \
    \
        virtual const std::string Dump() const \
        { return std::string(stringop) + sub->Dump(); } \
    };

#define binary_class(classname, op, stringop) \
    class classname: public expr_binary \
    { \
    public: \
        classname(expression *l, expression *r): expr_binary(l, r) { } \
        virtual long GetConst() const { return left->GetConst() op right->GetConst(); } \
    \
        virtual const std::string Dump() const \
        { return std::string("(") + left->Dump() + stringop + right->Dump() + ")"; } \
    };

unary_class(expr_bitnot, ~, "~")
unary_class(expr_negate, -, "-")

binary_class(expr_plus,   +, "+")
binary_class(expr_minus,  -, "-")
binary_class(expr_mul,    *, "*")
binary_class(expr_div,    /, "/")
binary_class(expr_shl,   <<, " shl ")
binary_class(expr_shr,   >>, " shr ")
binary_class(expr_bitand, &, " and")
binary_class(expr_bitor,  |, " or ")
binary_class(expr_bitxor, ^, " xor ")

#endif
