#include "parser/parser.hh"

namespace Frontend
{
Parser::Parser(const char* fn) : lexer(new Lexer(fn))
{
    // Pre-load all the tokens
    lexer->getToken(cur_token);
    lexer->getToken(next_token);

    // Fill the pre-built 
    std::vector<ValueType::Type> arg_types;
    ValueType::Type ret_type = ValueType::Type::VOID;
    FuncRecord record;

    // printVarInt
    arg_types.push_back(ValueType::Type::INT);
    record.ret_type = ret_type;
    record.arg_types = arg_types;
    record.is_built_in = true;
    func_def_tracker.insert({"printVarInt", record});

    // printVarFloat
    arg_types.clear();
    arg_types.push_back(ValueType::Type::FLOAT);
    record.ret_type = ret_type;
    record.arg_types = arg_types;
    record.is_built_in = true;
    func_def_tracker.insert({"printVarFloat", record});

    parseProgram();
}

void Parser::advanceTokens()
{
    cur_token = next_token;
    lexer->getToken(next_token);
}

void Parser::parseProgram()
{
    // Should always be functions to start with since
    // we don't support globals or structures...
    while (!cur_token.isTokenEOF())
    {
        ValueType::Type ret_type;
        std::unique_ptr<Identifier> iden;
        std::vector<FuncStatement::Argument> args;
        std::vector<std::shared_ptr<Statement>> codes;

        // determine return type
        ret_type = ValueType::typeTokenToValueType(cur_token);
        if (ret_type == ValueType::Type::MAX)
        {
	    std::cerr << "[Error] parseProgram: unsupported return type\n"
                      << "[Line] " << cur_token.getLine() << "\n";
	    exit(0);
        }
                
        // function name
        advanceTokens();
        iden = std::make_unique<Identifier>(cur_token);
        if (!next_token.isTokenLP())
        {
            std::cerr << "[Error] Incorrect function defition.\n "
                      << "[Line] " << cur_token.getLine() << "\n";
            exit(0);
	}

        advanceTokens();
        assert(cur_token.isTokenLP());

        // Track local variables
	std::unordered_map<std::string,ValueType::Type> local_vars;
        local_vars_tracker.push_back(&local_vars);

        // extract arguments
        while (!cur_token.isTokenRP())
        {
            advanceTokens();
            if (cur_token.isTokenRP()) break; // no args

            std::string arg_type = cur_token.getLiteral();

            advanceTokens();
            std::unique_ptr<Identifier> arg_iden(new Identifier(cur_token));
            FuncStatement::Argument arg(arg_type, arg_iden);
            args.push_back(arg);

            recordLocalVars(arg);

            advanceTokens();
        }
        assert(cur_token.isTokenRP());

        advanceTokens();
        assert(cur_token.isTokenLBrace());

        // record function def
        recordDefs(iden->getLiteral(), ret_type, args);

        // parse the codes section
        while (true)
        {
            advanceTokens();
            if (cur_token.isTokenRBrace())
                    break;

            parseStatement(iden->getLiteral(), codes);
        }

        std::unique_ptr<Statement> func_proto
            (new FuncStatement(ret_type, 
                               iden, 
                               args, 
                               codes,
                               local_vars));
        local_vars_tracker.pop_back();

        program.addStatement(func_proto);
        
        advanceTokens();
    }
}

void Parser::parseStatement(std::string &cur_func_name, 
                            std::vector<std::shared_ptr<Statement>> &codes)
{
    cur_expr_type = ValueType::Type::MAX;

    // is it an if statement?
    if (cur_token.isTokenIf())
    {
        auto code = parseIfStatement(cur_func_name);
        codes.push_back(std::move(code));
        return;
    }

    if (cur_token.isTokenFor())
    {
        auto code = parseForStatement(cur_func_name);
        codes.push_back(std::move(code));
        return;
    }

    // is it a function call?
    if (auto [is_def, is_built_in] = 
            isFuncDef(cur_token.getLiteral());
        is_def)
    {
        Statement::StatementType call_type = is_built_in ?
            Statement::StatementType::BUILT_IN_CALL_STATEMENT :
            Statement::StatementType::NORMAL_CALL_STATEMENT;

        auto code = parseCall();
        std::unique_ptr<CallStatement> call = 
            std::make_unique<CallStatement>(code, call_type); 

        codes.push_back(std::move(call));

        return;
    }

    // it it a return statement?
    if (cur_token.isTokenReturn())
    {
	advanceTokens();

        cur_expr_type = getFuncRetType(cur_func_name);
        auto ret = parseExpression();

        std::unique_ptr<RetStatement> ret_statement = 
            std::make_unique<RetStatement>(ret);

        codes.push_back(std::move(ret_statement));

        return;
    }

    // is it a variable-assignment?
    if (isTokenTypeKeyword(cur_token) ||
        cur_token.isTokenIden())
    {
        auto code = parseAssnStatement();

        codes.push_back(std::move(code));

        return;
    }
}

std::unique_ptr<Statement> Parser::parseAssnStatement()
{
    // Allocating new variables
    if (isTokenTypeKeyword(cur_token))
    {
        Token type_token = cur_token;

        advanceTokens();
        if (auto [already_defined, type] = isVarAlreadyDefined(cur_token);
            already_defined)
        {
            std::cerr << "[Error] Re-definition of "
                      << cur_token.getLiteral() << "\n";
            std::cerr << "[Line] " << cur_token.getLine() << "\n";
            exit(0);
        }

        bool is_array = (next_token.isTokenLBracket()) ? 
                        true : false;

        recordLocalVars(cur_token, type_token, is_array);

        std::unique_ptr<Expression> iden =
            std::make_unique<LiteralExpression>(cur_token);

	std::unique_ptr<Expression> expr;
        if (!is_array)
        {
            advanceTokens();
            assert(cur_token.isTokenEqual());

            advanceTokens();
            expr = parseExpression();
        }
        else
        {
            expr = parseArrayExpr();
        }
	
        std::unique_ptr<Statement> statement = 
            std::make_unique<AssnStatement>(iden, expr);

        return statement;
    }
    else
    {
        auto [already_defined, type] = isVarAlreadyDefined(cur_token);
        if (!already_defined)
        {
            std::cerr << "[Error] Undefined variable of "
                      << cur_token.getLiteral() << "\n";
            std::cerr << "[Line] " << cur_token.getLine() << "\n";
            exit(0);
        }

        cur_expr_type = ValueType::Type::MAX;
        auto iden = parseExpression();

        assert(cur_token.isTokenEqual());
        advanceTokens();

	std::unique_ptr<Expression> expr;
        if (type == ValueType::Type::INT_ARRAY || 
            type == ValueType::Type::FLOAT_ARRAY)
        {
            cur_expr_type = (type == ValueType::Type::INT_ARRAY)
                            ? type = ValueType::Type::INT
                            : type = ValueType::Type::FLOAT;
            
	}
        else
        {
            cur_expr_type = type;
        }

        expr = parseExpression();
        
        std::unique_ptr<Statement> statement = 
            std::make_unique<AssnStatement>(iden, expr);

        return statement;
    }
}

std::unique_ptr<Expression> Parser::parseArrayExpr()
{
    advanceTokens();
    assert(cur_token.isTokenLBracket());

    advanceTokens();
    // num_ele must be an integer
    auto swap = cur_expr_type;
    cur_expr_type = ValueType::Type::INT;
    auto num_ele = parseExpression();
    cur_expr_type = swap;
    if (!(num_ele->isExprLiteral()))
    {
        std::cerr << "[Error] Number of array elements "
                  << "must be a single integer. \n"
                  << "[Line] " << cur_token.getLine() << "\n";
        exit(0);
    }
    auto num_ele_lit = static_cast<LiteralExpression*>(num_ele.get());
    if (!(num_ele_lit->isLiteralInt()))
    {
        std::cerr << "[Error] Number of array elements "
                  << "must be a single integer. \n"
                  << "[Line] " << cur_token.getLine() << "\n";
        exit(0);
    }
    int num_eles_int = stoi(num_ele_lit->getLiteral());
    if (num_eles_int <= 1)
    {
        std::cerr << "[Error] Number of array elements "
                  << "must be larger than 1. \n"
                  << "[Line] " << cur_token.getLine() << "\n";
        exit(0);
    }

    assert(cur_token.isTokenRBracket());

    advanceTokens();
    assert(cur_token.isTokenEqual());

    advanceTokens();
    assert(cur_token.isTokenLBrace());

    std::vector<std::shared_ptr<Expression>> eles;
    if (!next_token.isTokenRBrace())
    {
        advanceTokens();
        while (!cur_token.isTokenRBrace())
        {
            eles.push_back(parseExpression());
            if (cur_token.isTokenComma())
                advanceTokens();
        }

        // We make sure consistent number of elements
        if (num_eles_int != eles.size())
        {
            std::cerr << "[Error] Accpeted format: "
                      << "(1) pre-allocation style - array<int> x[10] = {} "
                      << "(2) #initials == #elements - "
                      << "array<int> x[2] = {1, 2} \n"
                      << "[Line] " << cur_token.getLine() << "\n";
            exit(0);
        }
    }
    else
    {
        advanceTokens();
    }

    advanceTokens();

    std::unique_ptr<Expression> ret = 
        std::make_unique<ArrayExpression>(num_ele, eles);

    return ret;
}

std::unique_ptr<Expression> Parser::parseIndex()
{
    std::unique_ptr<Identifier> iden(new Identifier(cur_token));

    advanceTokens();
    assert(cur_token.isTokenLBracket());

    advanceTokens();

    // Index must be an integer
    auto swap = cur_expr_type;
    cur_expr_type = ValueType::Type::INT;
    auto idx = parseExpression();
    cur_expr_type = swap;

    std::unique_ptr<Expression> ret = 
        std::make_unique<IndexExpression>(iden, idx);

    assert(cur_token.isTokenRBracket());

    return ret;
}

std::unique_ptr<Expression> Parser::parseCall()
{
    std::unique_ptr<Identifier> def(new Identifier(cur_token));

    advanceTokens();
    assert(cur_token.isTokenLP());

    advanceTokens();
    std::vector<std::shared_ptr<Expression>> args;

    auto &arg_types = getFuncArgTypes(def->getLiteral());
    unsigned idx = 0;
    while (!cur_token.isTokenRP())
    {
        if (cur_token.isTokenRP())
            break;

        auto swap = cur_expr_type;
        cur_expr_type = arg_types[idx++];
        args.push_back(parseExpression());
        cur_expr_type = swap;

        if (cur_token.isTokenRP())
            break;
        advanceTokens();
    }

    std::unique_ptr<Expression> ret = 
        std::make_unique<CallExpression>(def, args);

    return ret;
}

std::unique_ptr<Condition> Parser::parseCondition()
{
    // Left condition
    auto cond_left = parseExpression();

    // Comp operator
    std::string comp_opr_str = cur_token.getLiteral();
    if (next_token.isTokenEqual())
    {
        comp_opr_str += next_token.getLiteral();
        advanceTokens();
    }

    // Right condition
    advanceTokens();
    auto cond_right = parseExpression();

    // Build up the condition object
    std::unique_ptr<Condition> cond = 
        std::make_unique<Condition>(cond_left,
                                    cond_right,
                                    comp_opr_str,
                                    cur_expr_type);
    return cond;
}

std::unique_ptr<Statement> Parser::parseIfStatement(std::string& 
                                                    parent_func_name)
{
    advanceTokens();
    assert(cur_token.isTokenLP());

    advanceTokens();
    auto cond = parseCondition();

    // Parse taken block
    advanceTokens();
    assert(cur_token.isTokenLBrace());

    std::vector<std::shared_ptr<Statement>> taken_block_codes;
    std::unordered_map<std::string,ValueType::Type> taken_block_local_vars;
    local_vars_tracker.push_back(&taken_block_local_vars);
    while (true)
    {
        advanceTokens();
        if (cur_token.isTokenRBrace())
            break;

        parseStatement(parent_func_name, taken_block_codes);
        // We just finished an if/for statement
        if (taken_block_codes.back()->isStatementIf() ||
            taken_block_codes.back()->isStatementFor())
        {
            // This RBrace is from the statement,
            // should not terminate.
            assert(cur_token.isTokenRBrace());
        }
        else
        {
            if (cur_token.isTokenRBrace())
                break;
        }
    }
    assert(cur_token.isTokenRBrace());
    local_vars_tracker.pop_back();

    // Parse else block
    std::vector<std::shared_ptr<Statement>> not_taken_block_codes;
    std::unordered_map<std::string,
                       ValueType::Type> not_taken_block_local_vars;

    if (next_token.isTokenElse())
    {
        advanceTokens();
        local_vars_tracker.push_back(&not_taken_block_local_vars);
        advanceTokens();
        while (true)
        {
            advanceTokens();
            if (cur_token.isTokenRBrace())
                break;

            parseStatement(parent_func_name, not_taken_block_codes);
            // We just finished an if/for statement
            if (not_taken_block_codes.back()->isStatementIf() ||
                not_taken_block_codes.back()->isStatementFor())
            {
                // This RBrace is from the statement,
                // should not terminate.
                assert(cur_token.isTokenRBrace());
            }
            else
            {
                if (cur_token.isTokenRBrace())
                    break;
            }
        }
        assert(cur_token.isTokenRBrace());
        local_vars_tracker.pop_back();
    }

    std::unique_ptr<Statement> if_statement = 
        std::make_unique<IfStatement>(cond, 
                                      taken_block_codes,
                                      not_taken_block_codes,
                                      taken_block_local_vars,
                                      not_taken_block_local_vars);
    
    assert(cur_token.isTokenRBrace());
    return if_statement;
}

std::unique_ptr<Statement> Parser::parseForStatement(std::string& 
                                                     parent_func_name)
{
    std::vector<std::shared_ptr<Statement>> block;
    std::unordered_map<std::string,
                       ValueType::Type> block_local_vars;
    local_vars_tracker.push_back(&block_local_vars);

    advanceTokens();
    assert(cur_token.isTokenLP());

    advanceTokens();
    auto start = parseAssnStatement();

    advanceTokens();
    auto end = parseCondition();

    advanceTokens();
    auto step = parseAssnStatement();
    
    advanceTokens();
    assert(cur_token.isTokenLBrace());

    while (true)
    {
        advanceTokens();
        if (cur_token.isTokenRBrace())
            break;

        parseStatement(parent_func_name, block);
        // We just finished an if/for statement
        if (block.back()->isStatementIf() ||
            block.back()->isStatementFor())
        {
            // This RBrace is from the statement,
            // should not terminate.
            assert(cur_token.isTokenRBrace());
        }
        else
        {
            if (cur_token.isTokenRBrace())
                break;
        }
    }
    
    std::unique_ptr<Statement> for_statement = 
        std::make_unique<ForStatement>(start,
                                       end,
                                       step,
                                       block,
                                       block_local_vars);
                                       	       
    assert(cur_token.isTokenRBrace());
    local_vars_tracker.pop_back();
   
    return for_statement;
}


std::unique_ptr<Expression> Parser::parseExpression()
{
    std::unique_ptr<Expression> left = parseTerm();

    while (true)
    {
        if (cur_token.isTokenPlus() || 
            cur_token.isTokenMinus())
        {
            Expression::ExpressionType expr_type;
            if (cur_token.isTokenPlus())
            {
                expr_type = Expression::ExpressionType::PLUS;
            }
            else
            {
                expr_type = Expression::ExpressionType::MINUS;
            }

            advanceTokens();

            std::unique_ptr<Expression> right;

            // Priority one. ()
            if (cur_token.isTokenLP())
            {
                right = parseTerm();
                left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);
                continue;
            }
            
            // Priority two. *, /
            // check if the next token is a function call
	    std::unique_ptr<Expression> pending_expr = nullptr;
            if (auto [is_def, is_built_in] = 
                    isFuncDef(cur_token.getLiteral());
                    is_def)
            {
                strictTypeCheck(cur_token);
                pending_expr = parseCall();
            }

            // check if the next token is an array index
            // TODO - add deref in the future
            if (bool is_index = (next_token.isTokenLBracket()) ?
                                true : false;
                is_index)
            {
                assert(pending_expr == nullptr);
                strictTypeCheck(cur_token, is_index);
                pending_expr = parseIndex();
            }

            if (next_token.isTokenAsterisk() ||
                next_token.isTokenSlash())
            {
                if (pending_expr != nullptr)
                {
                    advanceTokens();
                    right = parseTerm(std::move(pending_expr));
                }
                else
                {
                    right = parseTerm();
                }
            }
            else
            {
                if (pending_expr != nullptr)
                {
                    right = std::move(pending_expr);
                    advanceTokens();
                }
                else
                {
                    // Could be a literal or unary expression
                    right = parseFactor();
                }
            }

            left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);
        }
        else
        {
            return left;
        }
    }
}

// For Div/Mul
std::unique_ptr<Expression> Parser::parseTerm(
    std::unique_ptr<Expression> pending_left)
{   
    std::unique_ptr<Expression> left = 
        (pending_left != nullptr) ? std::move(pending_left) : parseFactor();

    while (true)
    {
        if (cur_token.isTokenAsterisk() || 
            cur_token.isTokenSlash())
        {
            Expression::ExpressionType expr_type;
            if (cur_token.isTokenAsterisk())
            {
                expr_type = Expression::ExpressionType::ASTERISK;
            }
            else
            {
                expr_type = Expression::ExpressionType::SLASH;
            }

            advanceTokens();

            std::unique_ptr<Expression> right;

            // We are trying to mul/div something with higher priority
            if (cur_token.isTokenLP()) 
            {
                right = parseTerm();
            }
            else
            {
                // TODO - add deref in the future
                bool is_index = (next_token.isTokenLBracket()) ?
                                true : false;

                if (is_index)
                {
                    // Make sure the array type is consistent
                    strictTypeCheck(cur_token, is_index);
                    right = parseIndex();
                    advanceTokens();
                }
                else if (auto [is_def, is_built_in] = 
                            isFuncDef(cur_token.getLiteral());
                            is_def)
                {
                    // Make sure the function return type is consistent
                    strictTypeCheck(cur_token, is_index);
                    right = parseCall();
                    advanceTokens();
                }
                else
                {
                    // Could be a literal or unary expression
                    right = parseFactor();
                }
            }

            left = std::make_unique<ArithExpression>(left, 
                       right, 
                       expr_type);

        }
        else
        {
            break;
        }

    }

    return left;
}

// Deal with () here
std::unique_ptr<Expression> Parser::parseFactor()
{
    std::unique_ptr<Expression> left;

    if (cur_token.isTokenPlus() ||
        cur_token.isTokenMinus())
    {
        Expression::ExpressionType expr_type;
        if (cur_token.isTokenPlus())
        {
            expr_type = Expression::ExpressionType::PLUS;
        }
        else
        {
            expr_type = Expression::ExpressionType::MINUS;
        }
	
        Token::TokenType tok_type = (cur_expr_type == ValueType::Type::INT) ? 
                                    Token::TokenType::TOKEN_INT : 
                                    Token::TokenType::TOKEN_FLOAT;
        std::string tok_lit = (cur_expr_type == ValueType::Type::INT) ? 
                              "0" : "0.0";
        Token zero_tok(tok_type, tok_lit);

        std::unique_ptr<Expression> left_expr = 
            std::make_unique<LiteralExpression>(zero_tok);
        advanceTokens();

        std::unique_ptr<Expression> right_expr;

        if (cur_token.isTokenInt() || 
            cur_token.isTokenFloat())
        {
            right_expr = 
                std::make_unique<LiteralExpression>(cur_token);

            advanceTokens();
        }
        else
        {
            right_expr = parseFactor();
        }

        left = std::make_unique<ArithExpression>(left_expr, 
                                                 right_expr,
                                                 expr_type);
        return left;
    }

    if (cur_token.isTokenLP())
    {
        advanceTokens();
        left = parseExpression();
        assert(cur_token.isTokenRP()); // Error checking
        advanceTokens();
        return left;
    }
    
    // TODO - add deref in the future
    bool is_index = (next_token.isTokenLBracket()) ?
                    true : false;

    strictTypeCheck(cur_token, is_index);
    
    if (is_index)
        left = parseIndex();
    else if (auto [is_def, is_built_in] = 
                 isFuncDef(cur_token.getLiteral());
                 is_def)
        left = parseCall();
    else
        left = std::make_unique<LiteralExpression>(cur_token);

    advanceTokens();

    return left;
}


void RetStatement::printStatement()
{
    std::cout << "    {\n";
    std::cout << "      [Return]\n";
    if (ret->getType() == Expression::ExpressionType::LITERAL)
    {
        std::cout << "      " << ret->print(4);
    }
    else
    {
        std::cout << ret->print(4);
    }
    std::cout << "    }\n";
}

void AssnStatement::printStatement()
{
    std::cout << "    {\n";
    if (iden->getType() == Expression::ExpressionType::LITERAL)
    {
        std::cout << "      " << iden->print(4);
    }
    else
    {
        std::cout << iden->print(4);
    }

    std::cout << "      =\n";
    if (expr->getType() == Expression::ExpressionType::LITERAL)
    {
        std::cout << "      " << expr->print(4);
    }
    else
    {
        std::cout << expr->print(4);
    }

    std::cout << "    }\n";
}

void FuncStatement::printStatement()
{
    std::cout << "{\n";
    std::cout << "  Function Name: " << iden->print() << "\n";
    std::cout << "  Return Type: ";
    if (func_type == ValueType::Type::VOID)
    {
        std::cout << "void\n";
    }
    else if (func_type == ValueType::Type::INT)
    {
        std::cout << "int\n";
    }
    else if (func_type == ValueType::Type::FLOAT)
    {
        std::cout << "float\n";
    }

    std::cout << "  Arguments\n";
    for (auto &arg : args)
    {
        std::cout << "    " << arg.print() << "\n";
    }
    if (!args.size()) std::cout << "    NONE\n";

    std::cout << "  Codes\n";
    std::cout << "  {\n";
    for (auto &code : codes)
    {
        code->printStatement();
    }
    std::cout << "  }\n";
    std::cout << "}\n";
}

void IfStatement::printStatement()
{
    std::cout << "  {\n";
    std::cout << "  [IF Statement] \n";
    std::cout << "  [Condition]\n";
    cond->printStatement();
    std::cout << "  [Taken Block]\n";
    std::cout << "  {\n";
    for (auto &code : taken_block)
    {
        code->printStatement();
    }
    std::cout << "  }\n";
    if (not_taken_block.size() == 0)
    {
        std::cout << "  }\n";
        return;
    }
    std::cout << "  [Not Taken Block]\n";
    std::cout << "  {\n";
    for (auto &code : not_taken_block)
    {
        code->printStatement();
    }
    std::cout << "  }\n";
    std::cout << "  }\n";

}

void ForStatement::printStatement()
{
    std::cout << "  {\n";
    std::cout << "  [For Statement] \n";
    std::cout << "  [Start]\n";
    start->printStatement();
    std::cout << "  [End]\n";
    end->printStatement();
    std::cout << "  [Step]\n";
    step->printStatement();

    std::cout << "  [Block]\n";
    std::cout << "  {\n";
    for (auto &code : block)
    {
        code->printStatement();
    }
    std::cout << "  }\n";
    std::cout << "  }\n";

}

void Condition::printStatement()
{
    std::cout << "  {\n";
    std::cout << "    [Left]\n";
    if (left->getType() == Expression::ExpressionType::LITERAL)
        std::cout << "      " << left->print(3) << "\n";
    else
        std::cout << left->print(3) << "\n";
    std::cout << "    [COMP] " << opr_type_str << "\n\n";
    std::cout << "    [Right]\n";
    if (right->getType() == Expression::ExpressionType::LITERAL) 
        std::cout << "      " << right->print(3) << "\n";
    else
        std::cout << right->print(3) << "\n";
    std::cout << "  }\n";
}
}
