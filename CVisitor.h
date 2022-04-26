#include "CLexer.h"
#include "CParser.h"
#include "CParserBaseVisitor.h"
#include "CParserBaseListener.h"
#include "antlr4-runtime.h"

#include <iostream>

class CVisitor : public CParserBaseVisitor
{
public:
    CVisitor(){}
    virtual ~CVisitor(){}

    std::any visitProg(CParser::ProgContext *ctx) override
    {
        std::any ret;
        std::cout << visitExpr(ctx->ExprContext) << std::endl;
        return ret;
    }

    std::any visitParen(CParser::ExprContext *ctx) override 
    {
        
    }

private:



};