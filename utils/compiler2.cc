#include <typeinfo>
#include <vector>
#include <deque>
#include <cxxabi.h>

#include "wstring.hh"
#include "ctcset.hh"
#include "hash.hh"
#include "autoptr"

using std::vector;
using std::deque;

namespace
{
    template<typename T>
    const string GetTypeName(const T& p)
    {
        const std::type_info& ti = typeid(p);
        int status;
        char* realname;
        
        realname = abi::__cxa_demangle(ti.name(), 0, 0, &status);
        string tmp = realname;
        free(realname);
        
        unsigned n = tmp.find('_');
        if(n != tmp.npos) tmp = tmp.substr(n+1);
        
        return tmp;
    }
    
    //////////////
    // Tree representation
    //////////////
    typedef autoptr<struct expression> expr_p;
    typedef autoptr<struct codenode> code_p;

    struct expr_callback_const
    {
        virtual ~expr_callback_const() {}
        virtual void operator() (const struct expression *, bool isread=true, bool iswrite=false) = 0;
    };
    
    struct expr_callback
    {
        virtual ~expr_callback() {}
        virtual void operator() (expr_p &, bool isread=true, bool iswrite=false) = 0;
    };
    
    struct expression: public ptrable
    {
        enum output_t { o_default, o_byte, o_word } output_type;

        expression() : output_type(o_default)
        {
        }
        
        virtual ~expression() { }
        
        virtual bool is_lvalue() const { return false; }
        
        virtual void Dump() const = 0;
        void ExprDump() const
        {
            if(output_type==o_byte) fprintf(stderr, ".byte");
            if(output_type==o_word) fprintf(stderr, ".word");
        }

        virtual void ForAllSubExpr(expr_callback_const* ) const { }
        virtual void ForAllSubExpr(expr_callback* ) { }
        
        virtual bool IsConst() const = 0;
        virtual int CalculateConst() const = 0;
        
        bool IsSameType(const expression &b) const
        {
            return typeid(*this) == typeid(b);
        }

        virtual bool operator== (const expression &b) const = 0;
        inline bool operator!= (const expression &b) const { return !operator== (b); }
    };

    struct expression_binary_swappable: public expression
    {
        vector<expr_p> items;
        
        expression_binary_swappable() { }
        expression_binary_swappable(expression *a, expression *b)
        {
            items.push_back(a);
            items.push_back(b);
        }
        
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

        virtual void ForAllSubExpr(expr_callback_const* T) const
        {
            for(unsigned a=0; a<items.size(); ++a)
            {
                (*T)(items[a]);
                if(items[a]) items[a]->ForAllSubExpr(T);
            }
        }
        virtual void ForAllSubExpr(expr_callback* T)
        {
            for(unsigned a=0; a<items.size(); ++a)
            {
                (*T)(items[a]);
                if(items[a]) items[a]->ForAllSubExpr(T);
            }
        }
        
        virtual bool IsConst() const
        {
            for(unsigned a=0; a<items.size(); ++a)
                if(!items[a]->IsConst()) return false;
            return true;
        }
        
        virtual bool operator== (const expression &b) const
        {
            if(!IsSameType(b)) return false;
            
            const expression_binary_swappable &bb
                = reinterpret_cast<const expression_binary_swappable&> (b);
            
            if(items.size() != bb.items.size()) return false;
            for(unsigned a=0; a<items.size(); ++a)
                if(*items[a] != *bb.items[a]) return false;
            
            return true;
        }
    };
    struct expression_sum: public expression_binary_swappable
    {
        expression_sum() { }
        expression_sum(expression *a, expression *b): expression_binary_swappable(a, b) { }
        
        virtual int CalculateConst() const
        {
            int result = 0;
            for(unsigned a=0; a<items.size(); ++a) result += items[a]->CalculateConst();
            return result;
        }
    };
    struct expression_or: public expression_binary_swappable
    {
        expression_or() { }
        expression_or(expression *a, expression *b): expression_binary_swappable(a, b) { }
        
        virtual int CalculateConst() const
        {
            int result = 0;
            for(unsigned a=0; a<items.size(); ++a) result |= items[a]->CalculateConst();
            return result;
        }
    };
    struct expression_and: public expression_binary_swappable
    {
        expression_and() { }
        expression_and(expression *a, expression *b): expression_binary_swappable(a, b) { }
        
        virtual int CalculateConst() const
        {
            int result = -1;
            for(unsigned a=0; a<items.size(); ++a) result &= items[a]->CalculateConst();
            return result;
        }
    };

    struct expression_binary: public expression
    {
        expr_p left, right;
        
        expression_binary() { }
        expression_binary(expression *a, expression *b): left(a),right(b) { }

        virtual void Dump() const
        {
            fprintf(stderr, "%s(", GetTypeName(*this).c_str());
            if(left)left->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, " ");
            if(right)right->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, ")");
            ExprDump();
        }

        virtual void ForAllSubExpr(expr_callback_const* T) const
        {
            (*T)(left);  if(left) left->ForAllSubExpr(T);
            (*T)(right); if(right) right->ForAllSubExpr(T);
        }
        virtual void ForAllSubExpr(expr_callback* T)
        {
            (*T)(left);  if(left) left->ForAllSubExpr(T);
            (*T)(right); if(right) right->ForAllSubExpr(T);
        }
        
        virtual bool IsConst() const
        {
            return left->IsConst() && right->IsConst();
        }

        virtual bool operator== (const expression &b) const
        {
            if(!IsSameType(b)) return false;
            
            const expression_binary &bb
                = reinterpret_cast<const expression_binary&> (b);
            
            return *left == *bb.left && *right == *bb.right;
        }
    };

    struct expression_minus: public expression_binary
    {
        expression_minus() { }
        expression_minus(expression *a, expression *b): expression_binary(a, b) { }

        virtual int CalculateConst() const
        {
            return left->CalculateConst() - right->CalculateConst();
        }
    };
    struct expression_shl: public expression_binary
    {
        expression_shl() { }
        expression_shl(expression *a, expression *b): expression_binary(a, b) { }

        virtual int CalculateConst() const
        {
            return left->CalculateConst() << right->CalculateConst();
        }
    };
    struct expression_shr: public expression_binary
    {
        expression_shr() { }
        expression_shr(expression *a, expression *b): expression_binary(a, b) { }

        virtual int CalculateConst() const
        {
            return left->CalculateConst() >> right->CalculateConst();
        }
    };
    
    struct expression_nonconst: public expression
    {
        virtual bool IsConst() const { return false; }
        virtual int CalculateConst() const { return 0; }
    };

    struct expression_var: public expression_nonconst
    {
        ucs4string varname;
        expr_p index;
        
        expression_var() { }
        expression_var(const ucs4string& name): varname(name) { }
        
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

        virtual void ForAllSubExpr(expr_callback_const* T) const
        {
            (*T)(index); if(index) index->ForAllSubExpr(T);
        }
        virtual void ForAllSubExpr(expr_callback* T)
        {
            (*T)(index); if(index) index->ForAllSubExpr(T);
        }

        virtual bool operator== (const expression &b) const
        {
            if(!IsSameType(b)) return false;
            
            if(const expression_var *bp = dynamic_cast<const expression_var*> (&b))
            {
                if(varname != bp->varname) return false;
                if(!index != !bp->index) return false;
                if(!index) return true;
                return *index == *bp->index;
            }
            return false;
        }
    };
    struct expression_mem: public expression_nonconst
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

        virtual void ForAllSubExpr(expr_callback_const* T) const
        {
            (*T)(page);  if(page) page->ForAllSubExpr(T);
            (*T)(offset);if(offset)offset->ForAllSubExpr(T);
        }

        virtual void ForAllSubExpr(expr_callback* T)
        {
            (*T)(page);  if(page) page->ForAllSubExpr(T);
            (*T)(offset);if(offset)offset->ForAllSubExpr(T);
        }
        
        virtual bool operator== (const expression &b) const
        {
            if(!IsSameType(b)) return false;
            
            if(const expression_mem *bp = dynamic_cast<const expression_mem*> (&b))
            {
                return *page == *bp->page && *offset == *bp->offset;
            }
            return false;
        }
    };
    struct expression_const: public expression
    {
        int value;
        
        expression_const(int v): value(v)
        {
            //output_type = value<256 ? o_byte : o_word;
        }
        
        virtual void Dump() const
        {
            if(value < 0)
                fprintf(stderr, "$-%X", -value);
            else
                fprintf(stderr, "$%X", value);
            ExprDump();
        }

        virtual bool IsConst() const { return true; }
        virtual int CalculateConst() const { return value; }

        virtual bool operator== (const expression &b) const
        {
            return b.IsConst() && b.CalculateConst() == value;
        }
    private:
        expression_const() : value(0) { }
    };
    
    struct register_16bit
    {
    private:
        expr_p value_lo;
        expr_p value_hi;
        expr_p value_16bit;
    public:
        void Assign_Lo(expression *lo)
        {
            value_lo    = lo;
            value_16bit = NULL; value_16bit = Get_16bit();
        }
        void Assign_Hi(expression *hi)
        {
            value_hi    = hi;
            value_16bit = NULL; value_16bit = Get_16bit();
        }
        void Assign_16bit(expression *e)
        {
            value_16bit = e;
            value_lo = NULL; value_lo = Get_Lo();
            value_hi = NULL; value_hi = Get_Hi();
        }
        const expr_p Get_Lo() const
        {
            if(value_lo) return value_lo;
            expression *result = new expression_and
               (value_16bit,
                new expression_const(0x00FF)
               );
            result->output_type = expression::o_byte;
            return result;
        }
        const expr_p Get_Hi() const
        {
            if(value_hi) return value_hi;
            expression *result = new expression_shr
               (value_16bit,
                new expression_const(8)
               );
            result->output_type = expression::o_byte;
            return result;
        }
        const expr_p Get_16bit() const
        {
            if(value_16bit) return value_16bit;
            expression *result = new expression_or
               (value_lo,
                new expression_shl
                (
                   value_hi,
                   new expression_const(8)
                )
               );
            result->output_type = expression::o_word;
            return result;
        }
    };

    struct register_8bit
    {
    private:
        expr_p value;
    public:
        void Assign(expression *e)
        {
            value = e;
            value->output_type = expression::o_byte;
        }
        const expr_p Get() const
        {
            return value;
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
        }
        
        virtual void ForAllExpr(expr_callback_const*, bool recursive=true) const = 0;
        virtual void ForAllExpr(expr_callback*, bool recursive=true) = 0;
    };

    struct codelump
    {
        deque<code_p> nodes;
        
        void Dump() const
        {
            for(unsigned a=0; a<nodes.size(); ++a)
            {
                if(nodes[a])nodes[a]->Dump();else fprintf(stderr, "(null)\n");
            }
        }

        void ForAllExpr(expr_callback_const* T, bool recursive) const
        {
            if(!recursive) return;
            for(unsigned a=0; a<nodes.size(); ++a)
                nodes[a]->ForAllExpr(T, recursive);
        }

        void ForAllExpr(expr_callback* T, bool recursive)
        {
            if(!recursive) return;
            for(unsigned a=0; a<nodes.size(); ++a)
                nodes[a]->ForAllExpr(T, recursive);
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

        virtual void ForAllExpr(expr_callback_const* T, bool) const
        {
            // assume "target" is both read and written
            (*T)(target, true, true); if(target) target->ForAllSubExpr(T);
            (*T)(value);              if(value) value->ForAllSubExpr(T);
        }

        virtual void ForAllExpr(expr_callback* T, bool)
        {
            // assume "target" is both read and written
            (*T)(target, true, true); if(target) target->ForAllSubExpr(T);
            (*T)(value);              if(value) value->ForAllSubExpr(T);
        }
    };

    struct codenode_let: public codenode_alter_var
    {
        virtual void ForAllExpr(expr_callback_const* T, bool) const
        {
            // "target" is only written
            (*T)(target, false, true); if(target) target->ForAllSubExpr(T);
            (*T)(value);               if(value) value->ForAllSubExpr(T);
        }
        virtual void ForAllExpr(expr_callback* T, bool)
        {
            // "target" is only written
            (*T)(target, false, true); if(target) target->ForAllSubExpr(T);
            (*T)(value);               if(value) value->ForAllSubExpr(T);
        }
    };

    struct codenode_add: public codenode_alter_var { };
    struct codenode_and: public codenode_alter_var { };
    struct codenode_or: public codenode_alter_var { };

    struct codenode_minus: public codenode_alter_var { };
    struct codenode_shl: public codenode_alter_var { };
    struct codenode_shr: public codenode_alter_var { };

    struct codenode_do: public codenode
    {
        codelump content;
        
        virtual void Dump() const
        {
            fprintf(stderr, ":loop\n");
            content.Dump();
            fprintf(stderr, ":endloop\n");
            CodeDump();
        }
        virtual void ForAllExpr(expr_callback_const* T, bool recursive) const
        {
            if(recursive) content.ForAllExpr(T, recursive);
        }
        virtual void ForAllExpr(expr_callback* T, bool recursive)
        {
            if(recursive) content.ForAllExpr(T, recursive);
        }
    };
    
    struct codenode_any_if: public codenode
    {
        expr_p a, b;
        codelump content;
        
        virtual void Dump() const
        {
            fprintf(stderr, ":%s ", GetTypeName(*this).c_str());
            if(a)a->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, " ");
            if(b)b->Dump();else fprintf(stderr, "(null)");
            fprintf(stderr, ":\n");
            content.Dump();
            fprintf(stderr, ":endif\n");
            CodeDump();
        }

        virtual void ForAllExpr(expr_callback_const* T, bool recursive) const
        {
            (*T)(a); if(a) a->ForAllSubExpr(T);
            (*T)(b); if(b) b->ForAllSubExpr(T);
            if(recursive) content.ForAllExpr(T, recursive);
        }

        virtual void ForAllExpr(expr_callback* T, bool recursive)
        {
            (*T)(a); if(a) a->ForAllSubExpr(T);
            (*T)(b); if(b) b->ForAllSubExpr(T);
            if(recursive) content.ForAllExpr(T, recursive);
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

        virtual void ForAllExpr(expr_callback_const* T, bool) const
        {
            (*T)(a); if(a) a->ForAllSubExpr(T);
            (*T)(b); if(b) b->ForAllSubExpr(T);
        }

        virtual void ForAllExpr(expr_callback* T, bool)
        {
            (*T)(a); if(a) a->ForAllSubExpr(T);
            (*T)(b); if(b) b->ForAllSubExpr(T);
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

        virtual void ForAllExpr(expr_callback_const* T, bool) const
        {
            parammap_t::const_iterator i;
            for(i=params.begin(); i!=params.end(); ++i)
            {
                (*T)(i->second);
                if(i->second) i->second->ForAllSubExpr(T);
            }
        }

        virtual void ForAllExpr(expr_callback* T, bool)
        {
            parammap_t::iterator i;
            for(i=params.begin(); i!=params.end(); ++i)
            {
                (*T)(i->second);
                if(i->second) i->second->ForAllSubExpr(T);
            }
        }
    };

    // Variable name -> size-expr
    typedef hash_map<ucs4string, expr_p> vardefmap_t;
    // Variable name translations
    typedef hash_map<ucs4string, ucs4string> transmap_t;
    // Variable name -> contents
    typedef hash_map<ucs4string, expr_p> varstatemap_t;

    bool OptimizeExpression
        (expr_p &p, const varstatemap_t& states)
    {
        // return value: true = changed, false = didn't change
        
        struct tester: public expr_callback
        {
            const varstatemap_t& states;
            bool changes;
            
            tester(const varstatemap_t& s) : states(s) { }
            
            virtual void operator() (expr_p& p, bool isread, bool iswrite)
            {
                if(!p)return;
                
                isread=isread;
                iswrite=iswrite;
                
                if(expression_const *v = dynamic_cast<expression_const *> (&*p))
                {
                    v=v;
                    // Already a const
                    return;
                }
                
                if(p->IsConst())
                {
                    p = new expression_const(p->CalculateConst());
                    changes = true;
                }
                
                if(expression_var *v = dynamic_cast<expression_var *> (&*p))
                {
                    const ucs4string& varname = v->varname;
                    
                    varstatemap_t::const_iterator i = states.find(varname);
                    if(i != states.end()
                    && i->second
                    && i->second->IsConst())
                    {
                        p = new expression_const(p->CalculateConst());
                        changes = true;
                    }
                }
            }
        } tester(states);
        
        tester.changes = false;
        tester(p, true, false);
        return tester.changes;
    }

    const ucs4string CreateVarName()
    {
        static unsigned counter = 0;
        char Buf[128];
        sprintf(Buf, "@FLY@%u", counter++);
        return AscToWstr(Buf);
    }
    
    expression *CreateTarget()
    {
        return new expression_var(CreateVarName());
    }
    
    struct program
    {
        codelump code;
        vardefmap_t vars;
        
        void Dump() const
        {
            fprintf(stderr, "--Code(%u)--\n", code.nodes.size());
            code.Dump();
            fprintf(stderr, "--Variables(%u)--\n", vars.size());
            for(vardefmap_t::const_iterator i=vars.begin(); i!=vars.end(); ++i)
            {
                fprintf(stderr, "  %s: ", WstrToAsc(i->first).c_str());
                i->second->Dump();
                fprintf(stderr, "\n");
            }
        }
        
        struct TargetVar
        {
            bool needs_word, only_bytes;
            ucs4string Name;
            expr_p target;
            
            const program &prog;
            
        public:
            TargetVar(const program &p)
            : needs_word(false), only_bytes(true),
              Name(CreateVarName()), target(new expression_var(Name)),
              prog(p)
            {
            }
            
            void Update(expression *e)
            {
                if(e->output_type == expression::o_word) needs_word = true;
                if(e->output_type == expression::o_byte) return;
                
                if(e->IsConst())
                {
                    int value = e->CalculateConst();
                    if(value & 0xFF00) needs_word = true;
                    return;
                }
                
                if(expression_var *var = dynamic_cast<expression_var*> (e))
                {
                    const ucs4string& varname = var->varname;
                    vardefmap_t::const_iterator i = prog.vars.find(varname);
                    if(i != prog.vars.end())
                    {
                        expression *varsize = i->second;
                        if(varsize->IsConst())
                        {
                            unsigned size = varsize->CalculateConst();
                            if(size == 2) needs_word = true;
                            if(size == 1) return;
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Undefined var: '%s'\n",
                            WstrToAsc(varname).c_str());
                    }
                }

                only_bytes = false;
            }
            
            void Set16bit()
            {
                only_bytes = false;
            }
            
            operator const expr_p& () const { return target; }
            operator const ucs4string& () const { return Name; }
            
            unsigned GetSize() const
            {
                return needs_word ? 2 : only_bytes ? 1 : 2;
            }
        };

        void Flatten(expr_p& expr, codelump& code, unsigned& line)
        {
            if(expr->IsConst())
            {
                expr = new expression_const(expr->CalculateConst());
            }
            
            deque<code_p>& lines = code.nodes;
            if(expression_binary_swappable *e = dynamic_cast<expression_binary_swappable *> (&*expr))
            {
                TargetVar target(*this);
                
                for(unsigned a = 0; a < e->items.size(); ++a)
                {
                    expr_p& expr2 = e->items[a];
                    Flatten(expr2, code, line);
                    
                    target.Update(expr2);
                    
                    codenode_alter_var *c;
                    
                    if(!a)
                        c = new codenode_let;
                    else if(dynamic_cast<expression_sum *> (e))
                        c = new codenode_add;
                    else if(dynamic_cast<expression_or *> (e))
                        c = new codenode_or;
                    else if(dynamic_cast<expression_and *> (e))
                        c = new codenode_and;
                    else
                    {
                        c = NULL;
                        
                        fprintf(stderr, "Internal error: "
                            "Expression type '%s' not handled in binary-swappable\n",
                            GetTypeName(*e).c_str());
                    }
                    
                    c->target = target;
                    c->value  = expr2;
                    
                    lines.insert(lines.begin() + line++, c);
                }
                
                if(dynamic_cast<expression_sum *> (e)) target.Set16bit();
                
                vars[target] = new expression_const(target.GetSize());
                
                expr = target;
                return;
            }

            if(expression_binary *e = dynamic_cast<expression_binary *> (&*expr))
            {
                Flatten(e->left, code, line);
                Flatten(e->right, code, line);
                
                TargetVar target(*this);
                
                target.Update(e->left);
                target.Update(e->right);

                codenode_alter_var *c = NULL;
                
                c = new codenode_let;
                c->target = target;
                c->value  = e->left;
                lines.insert(lines.begin() + line++, c);
                
                if(dynamic_cast<expression_minus *> (e))
                    c = new codenode_minus;
                else if(dynamic_cast<expression_shl *> (e))
                    c = new codenode_shl;
                else if(dynamic_cast<expression_shr *> (e))
                    c = new codenode_shr;
                else
                {
                    c = NULL;
              
                    fprintf(stderr, "Internal error: "
                        "Expression type '%s' not handled in binary\n",
                        GetTypeName(*e).c_str());
                }
                
                c->target = target;
                c->value  = e->right;
                lines.insert(lines.begin() + line++, c);
                
                target.Set16bit();
                vars[target] = new expression_const(target.GetSize());
                
                expr = target;
                return;
            }
            
            if(expression_var *e = dynamic_cast<expression_var *> (&*expr))
            {
                /* Index can be NULL too */
                if(e->index)
                {
                    Flatten(e->index, code, line);
                }
                return;
            }
            
            if(expression_mem *e = dynamic_cast<expression_mem *> (&*expr))
            {
                Flatten(e->page, code, line);
                Flatten(e->offset, code, line);
                return;
            }

            if(expression_const *e = dynamic_cast<expression_const *> (&*expr))
            {
                return;
            }

            fprintf(stderr, "Internal error: "
                        "Expression type '%s' not handled in flatten\n",
                        GetTypeName(*&*expr).c_str());
        }

        void Flatten(codelump& code)
        {
            deque<code_p>& lines = code.nodes;
            for(unsigned a=0; a<lines.size(); ++a)
            {
                codenode* stmt = lines[a];
                
                if(codenode_alter_var *p = dynamic_cast<codenode_alter_var *> (stmt))
                {
                    Flatten(p->value, code, a);
                    Flatten(p->target, code, a);
                    continue;
                }
                if(codenode_do *p = dynamic_cast<codenode_do *> (stmt))
                {
                    Flatten(p->content);
                    continue;
                }
                if(codenode_any_if *p = dynamic_cast<codenode_any_if *> (stmt))
                {
                    Flatten(p->a, code, a);
                    Flatten(p->b, code, a);
                    Flatten(p->content);
                    continue;
                }
                if(codenode_any_break *p = dynamic_cast<codenode_any_break *> (stmt))
                {
                    Flatten(p->a, code, a);
                    Flatten(p->b, code, a);
                    continue;
                }
            }
        }
        
        void Flatten()
        {
            Flatten(code);
        }
    }; /* struct program */

    struct context
    {
        register_16bit A, X, Y;
        register_16bit D;
        register_8bit B;
    };
    
    void Compile(const codelump& code, context& ctx, vardefmap_t& vars)
    {
        
    }
    
    void Compile(const program& prog, context& ctx)
    {    
        //Compile(prog.code, ctx, prog.vars);
    }

    void TestRun(program& prog)
    {
        #define assign_var(name, expr) \
          do { \
            codenode_alter_var *p = new codenode_let; \
            p->target = new expression_var(AscToWstr(name)); \
            p->value = expr; \
            prog.code.nodes.push_front(p); \
          } while(0)

        assign_var("TILEBASE_SEG",  new expression_const(0x11));
        assign_var("TILEBASE_OFFS", new expression_const(0x2233));
        assign_var("WIDTH_SEG",     new expression_const(0x11));
        assign_var("WIDTH_OFFS",    new expression_const(0x2233));
        assign_var("BITNESS",       new expression_const(4));

        assign_var("VRAMaddr",      new expression_const(0x0000));
        assign_var("tilenum",       new expression_const(0x100));

        assign_var("attr",          new expression_const(0x15));
        
        prog.Flatten();
        
        prog.Dump();
        
/*
        def_global("Length",        new expression_reg_A(0)); //lo
        def_global("OsoiteSeg",     new expression_reg_A(1)); //hi
        def_global("OsoiteOffs",    new expression_reg_Y);
        def_global("attr",          new expression_mem
                     (new expression_const(0x00),
                      new expression_sum(
                       new expression_const(0x7E),
                       new expression_reg_D
                                        ),
                      expression::o_word));
        def_global("tiledest_offs", new expression_reg_X);
        def_global("tiledest_seg",  new expression_reg_B);
*/
    }


    //////////////
    // For parser
    //////////////
    
    #include "compiler2-parser.inc"
}

int main(void)
{
    std::set_terminate (__gnu_cxx::__verbose_terminate_handler);

    infile inf;
    
    FILE *fp = fopen("ct.code2", "rt");
    inf.Read(fp);
    fclose(fp);
    
    ucs4string main_name = AscToWstr("draw_str");
    
    program prog = ParseFile(inf, main_name);
    
    TestRun(prog);
}
