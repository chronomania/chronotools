#include <vector>
#include <cxxabi.h>

#include "wstring.hh"
#include "ctcset.hh"
#include "hash.hh"
#include "autoptr"

using std::vector;

namespace
{
    template<typename T>
    const string GetTypeName(const T& p)
    {
        const type_info& ti = typeid(p);
        int status;
        char* realname;
        
        realname = abi::__cxa_demangle(ti.name(), 0, 0, &status);
        string tmp = realname;
        free(realname);
        
        unsigned n = tmp.find('_');
        if(n != tmp.npos) tmp = tmp.substr(n+1);
        
        return tmp;
    }
    
    struct callback
    {
        virtual ~callback() {}
        virtual void operator() (const struct expression *, bool=false) = 0;
    };
    
    //////////////
    // Tree representation
    //////////////
    struct expression: public ptrable
    {
        enum input_t  { i_unknown, i_byte, i_word } input_type;
        enum output_t { o_default, o_byte, o_word } output_type;
        enum suffix_t { s_none,    s_lo,   s_hi   } suffix;

        expression()
        : input_type(i_unknown), output_type(o_default), suffix(s_none)
        { }
        
        virtual ~expression() { }
        
        virtual bool is_lvalue() const { return false; }
        
        virtual void Dump() const = 0;
        void ExprDump() const
        {
            if(output_type==o_byte) fprintf(stderr, ".byte");
            if(output_type==o_word) fprintf(stderr, ".word");
            if(suffix==s_lo) fprintf(stderr, ".lo");
            if(suffix==s_hi) fprintf(stderr, ".hi");
        }

        virtual void ForAllSubExpr(callback* ) const { }
        
    };
    typedef autoptr<expression> expr_p;

    struct expression_binary_swappable: public expression
    {
        vector<expr_p> items;
        
        virtual void Dump() const
        {
            fprintf(stderr, "%s(", GetTypeName(*this).c_str());
            for(unsigned a=0; a<items.size(); ++a)
            {
                if(a)fprintf(stderr, " ");
                if(items[a])items[a]->Dump();
                else fprintf(stderr, "(null)");
            }
            fprintf(stderr, ")");
            ExprDump();
        }

        virtual void ForAllSubExpr(callback* T) const
        {
            for(unsigned a=0; a<items.size(); ++a)
            {
                (*T)(items[a]);
                if(items[a]) items[a]->ForAllSubExpr(T);
            }
        }
    };
    struct expression_sum: public expression_binary_swappable
    {
    };
    struct expression_or: public expression_binary_swappable
    {
    };
    struct expression_and: public expression_binary_swappable
    {
    };

    struct expression_binary: public expression
    {
        expr_p left, right;

        virtual void Dump() const
        {
            fprintf(stderr, "%s(", GetTypeName(*this).c_str());
            if(left)left->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, " ");
            if(right)right->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, ")");
            ExprDump();
        }

        virtual void ForAllSubExpr(callback* T) const
        {
            (*T)(left);  if(left) left->ForAllSubExpr(T);
            (*T)(right); if(right) right->ForAllSubExpr(T);
        }
    };

    struct expression_minus: public expression_binary { };
    struct expression_shl: public expression_binary { };
    struct expression_shr: public expression_binary { };

    struct expression_var: public expression
    {
        ucs4string varname;
        expr_p index;
        
        virtual bool is_lvalue() const { return true; }
        
        virtual void Dump() const
        {
            fprintf(stderr, "%s", WstrToAsc(varname).c_str());
            if(index)
            {
                fprintf(stderr, "[");
                index->Dump();
                fprintf(stderr, "]");
            }
            ExprDump();
        }

        virtual void ForAllSubExpr(callback* T) const
        {
            (*T)(index); if(index) index->ForAllSubExpr(T);
        }
    };
    struct expression_mem: public expression
    {
        expr_p page;
        expr_p offset;
        
        expression_mem() { }

        expression_mem(expression *p, expression *o,
                       expression::output_t t)
        : page(p), offset(o)
        {
            output_type = t;
        }

        virtual bool is_lvalue() const { return true; }
        
        virtual void Dump() const
        {
            fprintf(stderr, "[");
            if(page)page->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, ":");
            if(offset)offset->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, "]");
            ExprDump();
        }

        virtual void ForAllSubExpr(callback* T) const
        {
            (*T)(page);  if(page) page->ForAllSubExpr(T);
            (*T)(offset);if(offset)offset->ForAllSubExpr(T);
        }
    };
    struct expression_const: public expression
    {
        int value;
        
        expression_const() : value(0) { }
        
        expression_const(int v): value(v) { }
        
        virtual void Dump() const
        {
            if(value < 0)
                fprintf(stderr, "$-%X", -value);
            else
                fprintf(stderr, "$%X", value);
            ExprDump();
        }
    };
    struct expression_register: public expression
    {
        expression_register()
        {
            input_type  = i_word;
            output_type = o_word;
        }
        expression_register(int s)
        {
            input_type  = i_byte;
            output_type = o_byte;
            suffix = s==1 ? s_hi : s_lo;
        }
        
        virtual void Dump() const
        {
            fprintf(stderr, "%s", GetTypeName(*this).c_str());
            ExprDump();
        }
    };
    struct expression_reg_A: public expression_register
    {
        expression_reg_A() { }
        expression_reg_A(int s) : expression_register(s) { }
    };
    struct expression_reg_X: public expression_register
    {
        expression_reg_X() { }
        expression_reg_X(int s) : expression_register(s) { }
    };
    struct expression_reg_Y: public expression_register
    {
        expression_reg_Y() { }
        expression_reg_Y(int s) : expression_register(s) { }
    };
    struct expression_reg_B: public expression
    {
        expression_reg_B()
        {
            input_type  = i_byte;
            output_type = o_byte;
        }
        
        virtual void Dump() const
        {
            fprintf(stderr, "B");
            ExprDump();
        }
    };
    
    typedef hash_map<ucs4string, expr_p> parammap_t;

    struct codenode: public ptrable
    {
        codenode() { }
        virtual ~codenode() { }
        
        virtual void Dump() const = 0;
        
        void CodeDump() const
        {
            hash_set<ucs4string>::const_iterator i;
            bool first=true;
            for(i=vars_read.begin(); i!=vars_read.end(); ++i)
            {
                if(first){first=false;fprintf(stderr, "- reads:");}
                fprintf(stderr, " %s", WstrToAsc(*i).c_str());
            }
            if(!first)fprintf(stderr, "\n");
            first=true;
            for(i=vars_written.begin(); i!=vars_written.end(); ++i)
            {
                if(first){first=false;fprintf(stderr, "- writes:");}
                fprintf(stderr, " %s", WstrToAsc(*i).c_str());
            }
            if(!first)fprintf(stderr, "\n");
        }
        
        hash_set<ucs4string> vars_read;
        hash_set<ucs4string> vars_written;
        
        // List of variables that have been read since the last write to them
        hash_set<ucs4string> vars_accessed;
        // List of how many reads are still pending for each variable
        hash_map<ucs4string, unsigned> num_reads_left;
        
        virtual void ForAllExpr(callback* ) const { }
        
        void CollectVars()
        {
            struct tester: public callback
            {
                codenode* code;
                virtual void operator() (const expression *p, bool is_target)
                {
                    if(!p)return;
                    //fprintf(stderr, "cv:expr %p, target=%d\n", p, (int)is_target);
                    p->ForAllSubExpr(this);
                    if(const expression_var *v = dynamic_cast<const expression_var *> (p))
                    {
                        if(is_target) code->vars_written.insert(v->varname);
                        else code->vars_read.insert(v->varname);
                    }
                }
            } tester;
            tester.code = this;
            ForAllExpr(&tester);
        }
        
        bool ReadsVar(const ucs4string& name) const
        {
            struct tester: public callback
            {
                ucs4string name;
                bool value;
                virtual void operator() (const expression *p, bool is_target)
                {
                    fprintf(stderr, "rv:expr %p, target=%d\n", p, (int)is_target);
                    if(is_target) return; // Doesn't read
                    if(value) return;
                    p->ForAllSubExpr(this);
                    if(const expression_var *v = dynamic_cast<const expression_var *> (p))
                        if(v->varname == name)
                            value = true;
                }
            } tester;
            tester.name = name;
            tester.value = false;
            ForAllExpr(&tester);
            return tester.value;
        }
        
        bool WritesVar(const ucs4string& name) const
        {
            struct tester: public callback
            {
                ucs4string name;
                bool value;
                virtual void operator() (const expression *p, bool is_target)
                {
                    fprintf(stderr, "wv:expr %p, target=%d\n", p, (int)is_target);
                    if(!is_target) return; // Doesn't write
                    if(value) return;
                    
                    p->ForAllSubExpr(this);
                    if(const expression_var *v = dynamic_cast<const expression_var *> (p))
                        if(v->varname == name)
                            value = true;
                }
            } tester;
            tester.name = name;
            tester.value = false;
            ForAllExpr(&tester);
            return tester.value;
        }
    };
    typedef autoptr<codenode> code_p;

    struct codelump
    {
        vector<code_p> nodes;
        
        void Dump() const
        {
            for(unsigned a=0; a<nodes.size(); ++a)
            {
                if(nodes[a])nodes[a]->Dump();else fprintf(stderr, "(null)\n");
            }
        }

        void ForAllExpr(callback* T) const
        {
            for(unsigned a=0; a<nodes.size(); ++a)
                nodes[a]->ForAllExpr(T);
        }
    };

    struct codenode_alter_var: public codenode
    {
        expr_p target;
        expr_p value;
        
        virtual void Dump() const
        {
            fprintf(stderr, "%s ", GetTypeName(*this).c_str());
            if(target)target->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, " ");
            if(value)value->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, "\n");
            CodeDump();
        }

        virtual void ForAllExpr(callback* T) const
        {
            // assume "target" is both read and written
            (*T)(target, true);
            (*T)(target);
            (*T)(value);
        }
    };

    struct codenode_let: public codenode_alter_var
    {
        virtual void ForAllExpr(callback* T) const
        {
            // "target" is only written
            (*T)(target, true);
            (*T)(value);
        }
    };
    struct codenode_add: public codenode_alter_var { };
    struct codenode_and: public codenode_alter_var { };
    struct codenode_or: public codenode_alter_var { };

    struct codenode_do: public codenode
    {
        codelump content;
        
        virtual void Dump() const
        {
            fprintf(stderr, "do:\n");
            content.Dump();
            fprintf(stderr, "end\n");
            CodeDump();
        }
        virtual void ForAllExpr(callback* T) const
        {
            content.ForAllExpr(T);
        }
    };
    
    struct codenode_any_if: public codenode
    {
        expr_p a, b;
        codelump content;
        
        virtual void Dump() const
        {
            fprintf(stderr, "%s ", GetTypeName(*this).c_str());
            if(a)a->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, " ");
            if(b)b->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, ":\n");
            content.Dump();
            fprintf(stderr, "end\n");
            CodeDump();
        }

        virtual void ForAllExpr(callback* T) const
        {
            (*T)(a);
            (*T)(b);
            content.ForAllExpr(T);
        }
    };
    
    struct codenode_if_eq: public codenode_any_if { };
    struct codenode_if_gte: public codenode_any_if { };
    
    struct codenode_any_break: public codenode
    {
        expr_p a, b;
        
        virtual void Dump() const
        {
            fprintf(stderr, "%s ", GetTypeName(*this).c_str());
            if(a)a->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, " ");
            if(b)b->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, "\n");
            CodeDump();
        }

        virtual void ForAllExpr(callback* T) const
        {
            (*T)(a);
            (*T)(b);
        }
    };
    struct codenode_break_if_eq: public codenode_any_break { };
    struct codenode_break_if_gte: public codenode_any_break { };
    
    struct codenode_call: public codenode
    {
        ucs4string funcname;
        parammap_t params;
        
        virtual void Dump() const
        {
            fprintf(stderr, "call %s(", WstrToAsc(funcname).c_str());
            parammap_t::const_iterator i;
            for(i=params.begin(); i!=params.end(); ++i)
            {
                fprintf(stderr, " %s:", WstrToAsc(i->first).c_str());
                if(i->second)i->second->Dump();else fprintf(stderr, "(null)");
            }
            fprintf(stderr, " )\n");
            CodeDump();
        }

        virtual void ForAllExpr(callback* T) const
        {
            parammap_t::const_iterator i;
            for(i=params.begin(); i!=params.end(); ++i)
                (*T)(i->second);
        }
    };

    struct var_state
    {
        bool is_const;
        unsigned char const_value;
        
        var_state() : is_const(false)
        {
        }
        void SetConst(unsigned char n)
        {
            is_const    = true;
            const_value = n;
        }
    };

    struct var
    {
        ucs4string name;
        expr_p     bytesize;
        
        vector<var_state> state;
        
        var(const ucs4string& n, expr_p bs)
        : name(n), bytesize(bs), state(bs)
        {
        }
        
        void SetValue(unsigned value)
        {
            /* FIXME
            for(unsigned n=0; n<bytesize; ++n)
                state[n].SetConst((value >> (n*8)) & 255);
            */
        }
    };

    struct func
    {
        ucs4string name;
        hash_map<ucs4string, var> vars;
        codelump code;
        
        void Dump()
        {
            fprintf(stderr, "--%s--\n", WstrToAsc(name).c_str());
            code.Dump();
        }
    };

    struct file
    {
        hash_map<ucs4string, var> vars;
        hash_map<ucs4string, func> funcs;
    };

    void Compile(file& f, const ucs4string& func, const parammap_t& params)
    {
        // build scope
        for(hash_map<ucs4string, var>::const_iterator
            i = f.vars.begin(); i != f.vars.end(); ++i)
        {
            
        }
        
        parammap_t::const_iterator i;
    }
    
    void TestRun(file& f)
    {
        parammap_t params;
        params[AscToWstr("TILEBASE_SEG")]  = new expression_const(0x11);
        params[AscToWstr("TILEBASE_OFFS")] = new expression_const(0x2233);
        params[AscToWstr("WIDTH_SEG")]     = new expression_const(0x11);
        params[AscToWstr("WIDTH_OFFS")]    = new expression_const(0x2233);
        params[AscToWstr("BITNESS")]       = new expression_const(4);
        params[AscToWstr("Length")]        = new expression_reg_A(0); //lo
        params[AscToWstr("OsoiteSeg")]     = new expression_reg_A(1); //hi
        params[AscToWstr("OsoiteOffs")]    = new expression_reg_Y;
        params[AscToWstr("attr")]          = new expression_mem
                     (new expression_const(0x00),
                      new expression_const(0x27E),
                      expression::o_word);
        params[AscToWstr("tiledest_offs")] = new expression_reg_X;
        params[AscToWstr("tiledest_seg")]  = new expression_reg_B;

        params[AscToWstr("VRAMaddr")]      = new expression_const(0x0000);
        params[AscToWstr("tilenum")]       = new expression_const(0x100);
        
        Compile(f, AscToWstr("draw_str"), params);
    }


    //////////////
    // For parser
    //////////////
    
    #include "compiler2-parser.inc"
}

int main(void)
{
    FILE *fp = fopen("ct.code2", "rt");
    inf.Read(fp);
    fclose(fp);
    
    file tmp = ParseFile();
    
    TestRun(tmp);
}
