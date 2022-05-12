#pragma once

#include <filesystem>
#include <fmt/format.h>

namespace fs = std::filesystem;

struct Expr;

class ASTPrinter {
public:
    std::shared_ptr<Expr> AST;
    void ToPNG(fs::path const &exe_path, fs::path const &filename);
    // void ToPNG(const char *exe_path, const std::string &filename);

    ASTPrinter(std::shared_ptr<Expr> ast) : AST{std::move(ast)} {}

private:
    void sexp_fmt(const Expr &e);
    fmt::memory_buffer buffer; // buf for graphviz file output
};
