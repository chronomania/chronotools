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
        enum suffix_t { s_none,    s_lo,   s_hi   } suffix;

        expression()
        : output_type(o_default), suffix(s_none)
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

        virtual void ForAllSubExpr(expr_callback_const* ) const { }
        virtual void ForAllSubExpr(expr_callback* ) { }
        
        virtual bool IsConst() const = 0;
        virtual int CalculateConst() const = 0;
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
    };

    struct expression_minus: public expression_binary
    {
        virtual int CalculateConst() const
        {
            return left->CalculateConst() - right->CalculateConst();
        }
    };
    struct expression_shl: public expression_binary
    {
        virtual int CalculateConst() const
        {
            return left->CalculateConst() << right->CalculateConst();
        }
    };
    struct expression_shr: public expression_binary
    {
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

        virtual bool IsConst() const { return true; }
        virtual int CalculateConst() const { return value; }
    };
    struct expression_register: public expression_nonconst
    {
        expression_register()
        {
            output_type = o_word;
        }
        expression_register(int s)
        {
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
    struct expression_reg_B: public expression_nonconst
    {
        expression_reg_B()
        {
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
        }
        
        virtual void ForAllExpr(expr_callback_const*, bool recursive=true) const = 0;
        virtual void ForAllExpr(expr_callback*, bool recursive=true) = 0;
    };

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

    // Variable name -> size-expr
    typedef hash_map<ucs4string, expr_p> vardefmap_t;
    // Variable name translations
    typedef hash_map<ucs4string, ucs4string> transmap_t;
    // Variable name -> contents
    typedef hash_map<ucs4string, expr_p> varstatemap_t;

    struct program
    {
        codelump code;
        vardefmap_t vars;
    };

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
    
    struct context
    {
        // Variable name -> variable content
        varstatemap_t state;
        // Variable name -> variable size
        vardefmap_t   vars;
        
        void OptimizeExpressions()
        {
            for(;;)
            {
                bool changes = false;
                for(varstatemap_t::iterator i = state.begin(); i != state.end(); ++i)
                    if(OptimizeExpression(i->second, state))
                        changes = true;

                if(!changes) break;
            }
        }
    };

    class Compiler
    {
    private:
        struct program& prog;
    public:
        codelump result;
        
        Compiler(struct program& p): prog(p) { }
    private:
        transmap_t newnames;
    private:
        const ucs4string GenerateVarName() const
        {
            static unsigned counter = 0;
            char Buf[64];
            sprintf(Buf, "@fly_%u@", counter++);
            return AscToWstr(Buf);
        }
        
        unsigned NumVarReadsBeforeAWrite
                 (const vector<code_p>& lines,
                  unsigned firstline,
                  const ucs4string& varname) const
        {
            /*
               To calculate how many times the variable is
               read before it's overwritten the next time
            */
            const unsigned num_assumed_loop_executions = 999;
            
            struct tester: public expr_callback_const
            {
                const ucs4string& name;
                unsigned num_reads;
                
                tester(const ucs4string& n): name(n) { }
                
                virtual void operator() (const expression* p, bool isread, bool iswrite)
                {
                    if(!p) return;
                    if(!isread) return;
                    
                    if(const expression_var *v = dynamic_cast<const expression_var *> (&*p))
                    {
                        const ucs4string& varname = v->varname;
                        if(varname != name) return;
                        
                        ++num_reads;
                    }
                }
            } tester(varname);
            
            for(unsigned a=firstline; a<lines.size(); ++a)
            {
                codenode* line = lines[a];
                
                tester.num_reads = 0;
                line->ForAllExpr(&tester);
                unsigned n = tester.num_reads;
            }
            return 3;
        }
        
        void SetVarContent(context& ctx, const ucs4string& varname, expr_p value)
        {
            const ucs4string newname = GenerateVarName();
        
            ctx.state[newname] = value;
            newnames[varname]  = newname;
        }
        
        void CheckVarAccess(context& ctx,
                            const ucs4string& varname,
                            const vector<code_p>& lines,
                            unsigned firstline)
        {
            // Check if the variable should be loaded.
            
            varstatemap_t::iterator i = ctx.state.find(varname);
            
            if(i == ctx.state.end())
            {
                // Probably already loaded.
                return;
            }
            
            expr_p& value = i->second;
        
            bool isconst = false;
            if(dynamic_cast<expression_const *> (&*value))
            {
                isconst = true;
            }
            else if(value->IsConst())
            {
                value = new expression_const(value->CalculateConst());
                isconst = true;
            }
            
            //if(isconst) return;
            
            unsigned num = NumVarReadsBeforeAWrite(lines, firstline, varname);
            if(num > 1)
            {
                codenode_let *let = new codenode_let;
                
                let->target = new expression_var(varname);
                let->value = value;
                
                result.nodes.push_back(let);
                
                ctx.state.erase(i);
            }
        }
    public:
        void Compile(context& ctx, codelump& lump)
        {
            vector<code_p>& lines = lump.nodes;
            
            fprintf(stderr, "--Compiling\n");
            lump.Dump();
            fprintf(stderr, "--State:\n");
            for(varstatemap_t::const_iterator i=ctx.state.begin(); i!=ctx.state.end(); ++i)
            {
                fprintf(stderr, "- %s: ", WstrToAsc(i->first).c_str());
                i->second->Dump();
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "--Aliases:\n");
            for(transmap_t::const_iterator i=newnames.begin(); i!=newnames.end(); ++i)
                fprintf(stderr, "- %s -> %s\n",
                    WstrToAsc(i->first).c_str(),
                    WstrToAsc(i->second).c_str());
            fprintf(stderr, "----\n");

            for(unsigned linenumber=0; linenumber<lines.size(); ++linenumber)
            {
                code_p line = lines[linenumber];
                
                struct expr_name_converter: public expr_callback
                {
                    context& ctx;
                    transmap_t& newnames;
                    codelump& result;
                    hash_set<ucs4string> vars_read;
                    
                    expr_name_converter(context& c, transmap_t& n, codelump& r)
                    : ctx(c), newnames(n), result(r) { }
                    
                    virtual void operator() (expr_p& p, bool isread, bool iswrite)
                    {
                        if(isread)
                        {
                            if(expression_var *v = dynamic_cast<expression_var *> (&*p))
                            {
                                ucs4string& varname = v->varname;

                                transmap_t::const_iterator i = newnames.find(varname);
                                
                                if(i != newnames.end())
                                {
                                    /*
                                    fprintf(stderr,
                                            "Debug: Transformed '%s' to '%s'\n",
                                            WstrToAsc(varname).c_str(),
                                            WstrToAsc(i->second).c_str());
                                    */
                                    varname = i->second;
                                }
                                
                                vars_read.insert(varname);
                            }
                        }
                    }
                } expr_name_converter(ctx, newnames, result);
                
                line->ForAllExpr(&expr_name_converter);
                
                hash_set<ucs4string>::const_iterator i;
                for(i=expr_name_converter.vars_read.begin(); 
                    i!=expr_name_converter.vars_read.end();
                    ++i)
                {
                    CheckVarAccess(ctx, *i, lines, linenumber);
                }

                struct expr_converter: public expr_callback
                {
                    context& ctx;
                    
                    expr_converter(context &c): ctx(c) { }
                    
                    virtual void operator() (expr_p& p, bool isread, bool iswrite)
                    {
                        if(isread)
                        {
                            if(expression_var *v = dynamic_cast<expression_var *> (&*p))
                            {
                                varstatemap_t::const_iterator i = ctx.state.find(v->varname);
                                if(i != ctx.state.end())
                                {
                                    p = i->second;
                                }
                            }
                            
                            //OptimizeExpression(p, ctx.state);
                        }
                    }
                } expr_converter(ctx);
                
                expression_binary_swappable* swappable = NULL;
                codenode_alter_var* altervar = NULL;
                
                if(codenode_let *cmd = dynamic_cast<codenode_let *> (&*line))
                  if(dynamic_cast<expression_var *> (&*cmd->target))
                {
                    altervar = cmd;
                    //goto HandleAlterVar;
                }
                if(codenode_add *cmd = dynamic_cast<codenode_add *> (&*line))
                  if(dynamic_cast<expression_var *> (&*cmd->target))
                {
                    altervar = cmd;
                    swappable = new expression_sum;
                    //goto HandleAlterVar;
                }
                if(codenode_and *cmd = dynamic_cast<codenode_and *> (&*line))
                  if(dynamic_cast<expression_var *> (&*cmd->target))
                {
                    altervar = cmd;
                    swappable = new expression_and;
                    //goto HandleAlterVar;
                }
                if(codenode_or *cmd = dynamic_cast<codenode_or *> (&*line))
                  if(dynamic_cast<expression_var *> (&*cmd->target))
                {
                    altervar = cmd;
                    swappable = new expression_or;
                    //goto HandleAlterVar;
                }

                if(altervar)
                {
                    expr_p target = altervar->target;
                    expr_p value  = altervar->value;
                    if(expression_var *var = dynamic_cast<expression_var *> (&*target))
                    {
                        expr_converter(value, true, false);
                        value->ForAllSubExpr(&expr_converter);
                        if(swappable)
                        {
                            swappable->items.push_back(target);
                            swappable->items.push_back(value);
                            value = swappable;
                            expr_converter(value, true, false);
                            value->ForAllSubExpr(&expr_converter);
                        }

                        SetVarContent(ctx, var->varname, value);
                    }
                    continue;
                }
                
                line->ForAllExpr(&expr_converter, false);
                
                if(codenode_do *cmd = dynamic_cast<codenode_do *> (&*line))
                {
                    struct var_write_finder: public expr_callback_const
                    {
                        const transmap_t& newnames;
                        hash_set<ucs4string> vars_written;
                        
                        var_write_finder(const transmap_t& n): newnames(n) { }
                        
                        virtual void operator() (const expression* p, bool isread, bool iswrite)
                        {
                            if(iswrite)
                            {
                                if(const expression_var *v = dynamic_cast<const expression_var *> (p))
                                {
                                    ucs4string varname = v->varname;
                                    
                                    transmap_t::const_iterator i = newnames.find(varname);
                                    if(i != newnames.end()) varname = i->second;
                                    
                                    vars_written.insert(varname);
                                }
                            }
                        }
                    } var_write_finder(newnames);
                    
                    cmd->content.ForAllExpr(&var_write_finder, true);
                    
                    hash_set<ucs4string>::const_iterator i;
                    for(i=var_write_finder.vars_written.begin(); 
                        i!=var_write_finder.vars_written.end();
                        ++i)
                    {
                        fprintf(stderr, "Checking var '%s' write: ",
                            WstrToAsc(*i).c_str());
                        if(ctx.state.find(*i) != ctx.state.end())
                            ctx.state[*i]->Dump();
                        fprintf(stderr, "\n");
                        CheckVarAccess(ctx, *i, cmd->content.nodes, 0);
                    }
                    
                    const codelump outer = result; result.nodes.clear();

                    Compile(ctx, cmd->content);

                    fprintf(stderr, "--Aliases AFTER:\n");
                    for(transmap_t::const_iterator i=newnames.begin(); i!=newnames.end(); ++i)
                        fprintf(stderr, "- %s -> %s\n",
                            WstrToAsc(i->first).c_str(),
                            WstrToAsc(i->second).c_str());
                    fprintf(stderr, "----\n");

                    
                    codenode_do *newcmd = new codenode_do(*cmd);
                    newcmd->content = result;
                    
                    result = outer;
                    result.nodes.push_back(newcmd);
                    
                    continue;
                }

                if(codenode_if_eq *cmd = dynamic_cast<codenode_if_eq *> (&*line))
                {
                    const codelump outer = result; result.nodes.clear();
                    
                    Compile(ctx, cmd->content);
                    
                    codenode_if_eq *newcmd = new codenode_if_eq(*cmd);
                    newcmd->content = result;
                    
                    result = outer;
                    result.nodes.push_back(newcmd);
                    
                    continue;
                }

                if(codenode_if_gte *cmd = dynamic_cast<codenode_if_gte *> (&*line))
                {
                    const codelump outer = result; result.nodes.clear();
                    
                    Compile(ctx, cmd->content);
                    
                    codenode_if_gte *newcmd = new codenode_if_gte(*cmd);
                    newcmd->content = result;
                    
                    result = outer;
                    result.nodes.push_back(newcmd);
                    
                    continue;
                }
                result.nodes.push_back(line);
            }
        }
    };

    void TestRun(program& prog)
    {
        Compiler comp(prog);
        
        context ctx;
        
        #define def_global(name, expr) SetVarContent(ctx, AscToWstr(name), expr)
        

        parammap_t params;
        def_global("TILEBASE_SEG",  new expression_const(0x11));
        def_global("TILEBASE_OFFS", new expression_const(0x2233));
        def_global("WIDTH_SEG",     new expression_const(0x11));
        def_global("WIDTH_OFFS",    new expression_const(0x2233));
        def_global("BITNESS",       new expression_const(4));
        def_global("Length",        new expression_reg_A(0)); //lo
        def_global("OsoiteSeg",     new expression_reg_A(1)); //hi
        def_global("OsoiteOffs",    new expression_reg_Y);
        def_global("attr",          new expression_mem
                     (new expression_const(0x00),
                      new expression_const(0x27E),
                      expression::o_word));
        def_global("tiledest_offs", new expression_reg_X);
        def_global("tiledest_seg",  new expression_reg_B);

        def_global("VRAMaddr",      new expression_const(0x0000));
        def_global("tilenum",       new expression_const(0x100));

        def_global("attr",          new expression_const(0x15));
        
        comp.Compile(ctx, prog.code);
        
        comp.result.Dump();
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
    
    const ucs4string main_name = AscToWstr("draw_str");
    
    program prog = ParseFile(inf, main_name);
    
    TestRun(prog);
}
