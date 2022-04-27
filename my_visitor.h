#pragma once
#include "CParserBaseVisitor.h"
using namespace antlrcpp;

class my_visitor : public CParserBaseVisitor {
public:
    std::any visitProg(CParser::ProgContext *ctx) override { return visit(ctx->expr()); }

    std::any visitAddSub(CParser::AddSubContext *ctx) override {
        int left = std::any_cast<int>(visit(ctx->expr()));
        int right = atoi(ctx->INT()->getText().c_str());
        if (ctx->Plus()) {
            return left + right;
        } else {
            return left - right;
        }
    }

    std::any visitNum(CParser::NumContext *ctx) override {
        return atoi(ctx->INT()->getText().c_str());
    }
};
