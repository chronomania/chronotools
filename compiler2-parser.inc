const program ParseFile(class infile& inf, const ucs4string& main_name);

class infile
{
private:
    ucs4string data;
    vector<ucs4string> tokens;
    
    unsigned ptr;
public:
    void Dump()
    {
        wstringOut conv(getcharset());
        for(unsigned a=0; a<tokens.size(); ++a)
        {
            fprintf(stderr, "%u: %s\n",
                a,
                conv.puts(tokens[a]).c_str());
        }
    }
    
    struct State { unsigned p; };
    
    const State SaveState() const { State tmp; tmp.p = ptr; return tmp; }
    void RestoreState(const State& s) { ptr = s.p; }
    
    bool IsLast() const { return (ptr+1) == tokens.size(); }
    const ucs4string& Get() { return tokens[ptr++]; }
    const ucs4string& Peek() const { return tokens[ptr]; }
    
    void Read(FILE *fp)
    {
        wstringIn conv(getcharset());
        
        for(;;)
        {
            char Buf[2048];
            if(!fgets(Buf, sizeof Buf, fp))break;
            data += conv.puts(Buf);
        }
        
        enum { stSpace, stWord, stComment } state = stSpace;
        
        ucs4string curtoken;
        for(unsigned a=0; a<data.size(); ++a)
        {
            char c = WcharToAsc(data[a]);
            
            bool isspace = c == ' ' || c == '\t'
                        || c == '\n' || c=='\r';
            bool isdelim = c == '(' || c == ')'
                        || c == '[' || c == ']'
                        || c == ':' || c == '.';
            switch(state)
            {
                case stSpace:
                {
                    
                    if(c == '{') { state=stComment; break; }
                    if(isspace)break;
                    state=stWord;
                    // passthru
                }
                case stWord:
                {
                    if(isspace || isdelim)
                    {
                        if(!curtoken.empty())
                        {
                            tokens.push_back(curtoken);
                            CLEARSTR(curtoken);
                        }
                        if(isdelim)
                        {
                            curtoken += data[a]; 
                            tokens.push_back(curtoken);
                            CLEARSTR(curtoken);
                        }
                        state = stSpace;
                    }
                    else
                        curtoken += data[a];
                    break;
                }
                case stComment:
                {
                    if(c != '}') break;
                    state = stSpace;
                    break;
                }
            }
        }
        if(!curtoken.empty())
        {
            tokens.push_back(curtoken);
            CLEARSTR(curtoken);
        }
    }
};

namespace
{
    expr_p ParseExpression(infile& inf)
    {
        const ucs4string& key = inf.Get();
        const string asc = WstrToAsc(key);
        
        expr_p result = NULL;
        expression_binary_swappable *swappable = NULL;
        expression_binary *binary = NULL;
        if(asc == "sum")
        {
            swappable = new expression_sum;
        ParseSwappable:
            string Sep = WstrToAsc(inf.Get());
            if(Sep != "(")
                fprintf(stderr, "Error: In '%s', '(' expected; got '%s'\n",
                    asc.c_str(), Sep.c_str());

            while(!inf.IsLast())
            {
                Sep = WstrToAsc(inf.Peek());
                if(Sep == ")") { inf.Get(); break; }
                
                swappable->items.push_back(ParseExpression(inf));
            }
            result = swappable; goto Finish;
        }
        if(asc == "or") { swappable = new expression_or; goto ParseSwappable; }
        if(asc == "and") { swappable = new expression_and; goto ParseSwappable; }
        if(asc == "minus")
        {
            binary = new expression_minus;
        ParseBinary:
            string Sep = WstrToAsc(inf.Get());
            if(Sep != "(")
                fprintf(stderr, "Error: In '%s', '(' expected; got '%s'\n",
                    asc.c_str(), Sep.c_str());

            binary->left = ParseExpression(inf);
            binary->right = ParseExpression(inf);

            Sep = WstrToAsc(inf.Get());
            if(Sep != ")")
            {
                fprintf(stderr, "Error: In '%s', ')' expected; got '%s'\n",
                    asc.c_str(), Sep.c_str());

                fprintf(stderr, "Got so far: ");
                binary->Dump();
                fprintf(stderr, "\n\n");
            }

            result = binary; goto Finish;
        }
        if(asc == "shl") { binary = new expression_shl; goto ParseBinary; }
        if(asc == "shr") { binary = new expression_shr; goto ParseBinary; }
        if(asc[0] == '$')
        {
            expression_const *value = new expression_const;
            value->value = atoi(key.c_str()+1, 16);
            result = value; goto Finish;
        }
        if(asc[0] == '-')
        {
            expression_const *value = new expression_const;
            if(asc[1] == '$')
            {
                fprintf(stderr, "Error: Use $-12 for negative hex, not -$12\n");
                return NULL;
            }
            value->value = atoi(key.c_str()+1, 10);
            result = value; goto Finish;
        }
        if(asc[0] >= '0' && asc[0] <= '9')
        {
            expression_const *value = new expression_const;
            value->value = atoi(key.c_str(), 10);
            result = value; goto Finish;
        }
        if(asc == "[")
        {
            expression_mem *mem = new expression_mem;
            mem->page = ParseExpression(inf);
            
            string Sep = WstrToAsc(inf.Get());
            if(Sep != ":")
            {
                fprintf(stderr, "Error: In mem notation, ':' expected; got '%s'\n",
                    Sep.c_str());

                fprintf(stderr, "Got so far: ");
                mem->Dump();
                fprintf(stderr, "\n\n");
            }
            
            mem->offset = ParseExpression(inf);
            Sep = WstrToAsc(inf.Get());
            if(Sep != "]")
            {
                fprintf(stderr, "Error: In mem notation, ']' expected; got '%s'\n",
                    Sep.c_str());

                fprintf(stderr, "Got so far: ");
                mem->Dump();
                fprintf(stderr, "\n\n");
            }
            
            result = mem; goto Finish;
        }
        if(true)
        {
            if(asc == "let" || asc == "add"
            || asc == "and" || asc == "or"
            || asc == "do" || asc == "if_eq"
            || asc == "if_gte" || asc == "break_if_eq"
            || asc == "break_if_gte" || asc == "call")
            {
                fprintf(stderr, "Error: '%s' is not an allowed variable name\n",
                    asc.c_str());
            }
            
            expression_var *var = new expression_var;
            var->varname = key;
            
            if(WstrToAsc(inf.Peek()) == "[")
            {
                infile::State sav = inf.SaveState();
                
                inf.Get(); // eat "["
                
                expr_p index = ParseExpression(inf);
                
                const string Sep = WstrToAsc(inf.Get());
                if(Sep == ":")
                {
                    // Oops, looks like a memory address!
                    // We weren't supposed to read this as an index!
                    inf.RestoreState(sav);
                }
                else
                {
                    var->index = index;
                    if(Sep != "]")
                    {
                        fprintf(stderr, "Error: In var index, ']' expected; got '%s'\n",
                            Sep.c_str());

                        fprintf(stderr, "Got so far: ");
                        var->Dump();
                        fprintf(stderr, "\n\n");
                    }
                }
            }
            result = var; goto Finish;
        }
    Finish:
        while(WstrToAsc(inf.Peek()) == ".")
        {
            inf.Get(); // eat "."
            const ucs4string& tok = inf.Get();
            const string key = WstrToAsc(tok);
            if(key == "byte")
                result->output_type = expression::o_byte;
            else if(key == "word")
                result->output_type = expression::o_word;
            else if(key == "lo")
                result->suffix = expression::s_lo;
            else if(key == "hi")
                result->suffix = expression::s_hi;
        }
        return result;
    }

    const codelump ParseCode(infile& inf);

    code_p ParseStatement(infile &inf)
    {
        const ucs4string& key = inf.Get();
        const string asc = WstrToAsc(key);
        
        codenode *result = NULL;
        
        codenode_alter_var *alter_var = NULL;
        codenode_any_if    *any_if    = NULL;
        codenode_any_break *any_break = NULL;
        if(asc == "let")
        {
            alter_var = new codenode_let;
        ParseAlterVar:
            expr_p target = ParseExpression(inf);
            alter_var->target = target;

            if(!target->is_lvalue())
            {
                fprintf(stderr, "Error: Expression '");
                target->Dump();
                fprintf(stderr, "' used as an lvalue\n");
            }

            alter_var->value  = ParseExpression(inf);
            result=alter_var; goto Finish;
        }
        if(asc == "add") { alter_var = new codenode_add; goto ParseAlterVar; }
        if(asc == "and") { alter_var = new codenode_and; goto ParseAlterVar; }
        if(asc == "or")  { alter_var = new codenode_or;  goto ParseAlterVar; }
        if(asc == "do")
        {
            codenode_do *tmp = new codenode_do;
            tmp->content = ParseCode(inf);
            result=tmp; goto Finish;
        }
        if(asc == "if_eq")
        {
            any_if = new codenode_if_eq;
        ParseAnyIf:
            any_if->a = ParseExpression(inf);
            any_if->b = ParseExpression(inf);
            any_if->content = ParseCode(inf);
            result=any_if; goto Finish;
        }
        if(asc == "if_gte") { any_if = new codenode_if_gte; goto ParseAnyIf; }
        
        if(asc == "break_if_eq")
        {
            any_break = new codenode_break_if_eq;
        ParseAnyBreak:
            any_break->a = ParseExpression(inf);
            any_break->b = ParseExpression(inf);
            result=any_break; goto Finish;
        }
        if(asc == "break_if_gte") { any_break = new codenode_break_if_gte; goto ParseAnyBreak; }
        
        if(asc == "call")
        {
            codenode_call *call = new codenode_call;
            call->funcname = inf.Get();

            string Sep = WstrToAsc(inf.Get());
            if(Sep != "(")
            {
                fprintf(stderr, "Error: In call, '(' expected; got '%s'\n",
                    Sep.c_str());

                fprintf(stderr, "Got so far: ");
                call->Dump();
                fprintf(stderr, "\n\n");
            }
            
            while(!inf.IsLast())
            {
                const ucs4string& key = inf.Get();
                if(WstrToAsc(key) == ")")break;
                
                string Sep = WstrToAsc(inf.Get());
                if(Sep != ":")
                {
                    fprintf(stderr, "Error: In call, ':' expected after '%s'; got '%s'\n",
                        WstrToAsc(key).c_str(), Sep.c_str());

                    fprintf(stderr, "Got so far: ");
                    call->Dump();
                    fprintf(stderr, "\n\n");
                }
                
                call->params[key] = ParseExpression(inf);
            }
            result=call; goto Finish;
        }
        
        fprintf(stderr, "ParseStatement: Expected "
            "'let' or 'add' or 'and' or 'or' or 'do'"
            "or 'if_eq' or 'if_gte' or 'break_if_eq' or 'break_if_gte'"
            "or 'call'"
            "; got '%s'\n",
            asc.c_str());
    Finish:
        //if(result) result->CollectVars();
        return result;
    }

    const codelump ParseCode(infile &inf)
    {
        codelump result;
        while(!inf.IsLast() && WstrToAsc(inf.Peek()) != "end")
        {
            code_p stmt = ParseStatement(inf);

            if(stmt)
                result.nodes.push_back(stmt);
            else
            {
                fprintf(stderr, "Got so far: ");
                result.Dump();
                fprintf(stderr, "\n\n");
            }
        }
        if(!inf.IsLast()) inf.Get(); // eat "end"
        return result;
    }

    const ucs4string TransformLocal(const ucs4string& varname,
                                    const ucs4string& funcname)
    {
        ucs4string result = funcname;
        char Buf[64];
        sprintf(Buf, "@");
        result += AscToWstr(Buf);
        result += varname;
        return result;
    }

    const ucs4string TransformGlobal(const ucs4string& varname)
    {
        char Buf[64];
        sprintf(Buf, "@global@");
        ucs4string result = AscToWstr(Buf);
        result += varname;
        return result;
    }

    void TransformVariables(codelump& code,
                            const ucs4string& funcname,
                            const vardefmap_t& globalvars,
                            const vardefmap_t& localvars)
    {
        vector<code_p>& lines = code.nodes;
        for(unsigned a=0; a<lines.size(); ++a)
        {
            codenode* code = lines[a];

    #if 0
            if(codenode_call *call = dynamic_cast<codenode_call *> (code))
            {
                parammap_t newparammap;
                parammap_t::const_iterator i;
                for(i=call->params.begin(); i!=call->params.end(); ++i)
                {
                    ucs4string varname = i->first;
                    if(globalvars.find(varname) == globalvars.end())
                        varname = TransformLocal(varname, call->funcname);
                    else
                    {
                        varname = TransformGlobal(varname);
                        // Huh? Assigning params to globals?
                    }
                    newparammap[varname] = i->second;
                }
                call->params = newparammap;
            }
    #endif
            
            struct transformer: public expr_callback
            {
                const vardefmap_t& globals;
                const vardefmap_t& locals;
                const ucs4string& funcname;
                transformer(const vardefmap_t& g,
                            const vardefmap_t& l,
                            const ucs4string& f)
                    : globals(g), locals(l), funcname(f) { }
                virtual void operator() (expr_p& p, bool isread, bool iswrite)
                {
                    if(!p) return;
                    if(expression_var *var = dynamic_cast<expression_var *> (&*p))
                    {
                        ucs4string& varname = var->varname;
                        vardefmap_t::const_iterator i;
                        if((i = locals.find(varname)) != locals.end())
                            varname = TransformLocal(varname, funcname);
                        else if((i = globals.find(varname)) != globals.end())
                            varname = TransformGlobal(varname);
                    }
                }
            } transformer(globalvars, localvars, funcname);
            code->ForAllExpr(&transformer);
        }
    }

    struct func
    {
        // name -> size-expr
        vardefmap_t vars;

        codelump code;
        
        void Dump()
        {
            code.Dump();
        }
    };

    typedef hash_map<ucs4string, func> funcmap_t;

    struct file
    {
        // name -> size-expr
        vardefmap_t vars;
        
        // name -> function
        funcmap_t funcs;
    };

    void TransformCalls(const file& f, codelump& code, codelump& result)
    {
        vector<code_p>& lines = code.nodes;
        for(unsigned a=0; a<lines.size(); ++a)
        {
            code_p& line = lines[a];
            if(const codenode_do* cmd = dynamic_cast<const codenode_do*> (&*line))
            {
                codelump tmpresult;
                TransformCalls(f, cmd->content, tmpresult);
                codenode_do *tmp = new codenode_do(*cmd);
                tmp->content = tmpresult;
                result.nodes.push_back(tmp);
                continue;
            }
            if(const codenode_if_eq* cmd = dynamic_cast<const codenode_if_eq*> (&*line))
            {
                codelump tmpresult;
                TransformCalls(f, cmd->content, tmpresult);
                codenode_if_eq *tmp = new codenode_if_eq(*cmd);
                tmp->content = tmpresult;
                result.nodes.push_back(tmp);
                continue;
            }
            if(const codenode_if_gte* cmd = dynamic_cast<const codenode_if_gte*> (&*line))
            {
                codelump tmpresult;
                TransformCalls(f, cmd->content, tmpresult);
                codenode_if_gte *tmp = new codenode_if_gte(*cmd);
                tmp->content = tmpresult;
                result.nodes.push_back(tmp);
                continue;
            }
            if(const codenode_call* cmd = dynamic_cast<const codenode_call*> (&*line))
            {
                parammap_t::const_iterator i;
                
                for(i=cmd->params.begin(); i!=cmd->params.end(); ++i)
                {
                    codenode_let *tmp = new codenode_let;
                    const ucs4string varname = TransformLocal(i->first, cmd->funcname);
                    tmp->target = new expression_var(varname);
                    tmp->value  = i->second;
                    result.nodes.push_back(tmp);
                }
                TransformCalls(f, f.find(cmd->funcname)->code, result);
                continue;
            }
            result.push_back(line);
        }
    }
}

const program ParseFile(infile& inf, const ucs4string& main_name)
{
    file tmpfile;
    while(!inf.IsLast())
    {
        const ucs4string& key = inf.Get();
        const string asc = WstrToAsc(key);
        if(asc == "params" || asc == "vars")
        {
            while(!inf.IsLast())
            {
                const ucs4string& name = inf.Get();
                
                if(WstrToAsc(name) == "end") break;
                
                expr_p size = ParseExpression(inf);
                
                tmpfile.vars.insert(make_pair(name, size));
            }
            continue;
        }
        if(asc == "macro")
        {
            func tmpfun;
            const ucs4string& funcname = inf.Get();
            
            while(!inf.IsLast())
            {
                const ucs4string& word = inf.Peek();
                const string word_asc = WstrToAsc(word);
                if(word_asc != "vars" && word_asc != "params") break;

                inf.Get(); // eat the word
                
                while(!inf.IsLast())
                {
                    const ucs4string& varname = inf.Get();
                    
                    if(WstrToAsc(varname) == "end") break;
                    
                    expr_p size = ParseExpression(inf);
                    
                    tmpfun.vars.insert(make_pair(varname, size));
                }
            }
            tmpfun.code = ParseCode(inf);
            
            TransformVariables(tmpfun.code, funcname, tmpfile.vars, tmpfun.vars);
            
            tmpfile.funcs.insert(make_pair(funcname, tmpfun));
            
            fprintf(stderr, "--%s--\n", WstrToAsc(funcname).c_str());
            tmpfun.Dump();
            fprintf(stderr, "\n\n");

            continue;
        }
        fprintf(stderr, "ParseFile: Expected "
            "'params' or 'vars' or 'macro'"
            "; got '%s'\n",
            asc.c_str());
    }
    
    program result;
    
    TransformCalls(file, funcs[main_name].code, result.code);
    
    for(vardefmap_t::const_iterator i=file.vars.begin(); i!=file.vars.end(); ++i)
        result.vars[TransformGlobal(i->first)] = i->second;
    
    for(funcmap_t::const_iterator j=file.funcs.begin(); j!=file.funcs.end(); ++j)
    {
        const func& func = j->second;
        for(vardefmap_t::const_iterator i=func.vars.begin(); i!=func.vars.end(); ++i)
            result.vars[TransformLocal(i->first, j->first)] = i->second;
    }
    
    return result;
}
