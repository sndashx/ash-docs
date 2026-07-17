#pragma once
/// Phase 09: .qst DSL parser (per appendix C).
///
/// The parser is a small recursive-descent over a hand-rolled lexer.
/// It produces a ParsedQuest from a .qst source string or file path.
///
/// Supported syntax (see appendices/C-quest-format.txt):
///
///   quest <id> {
///     meta { name: "..."; category: "..."; priority: 100;
///            giver: npc_id; sort_index: 1;
///            hidden_until: <cond>; auto_complete: true; }
///     entry: <stage_id>;
///     stage <id> { description: "..."; objective: "...";
///                  objective_all: [...]; objective_count: {...};
///                  objective_any: [...]; duration: "300s";
///                  escort_npc: ...; destination: ...;
///                  on_enter { ... }; on_exit { ... };
///                  on_complete { ... }; on_fail { ... };
///                  on_arrive { ... }; on_timeout { ... };
///                  condition { ... };
///                  branch { if (...) -> s; else if (...) -> s; else -> s; };
///                  next: <stage_id>; }
///   }
///
/// Errors are reported with a (line, column, message) triple.

#include "quest/qst_ast.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace ash {
namespace quest {

struct ParseError : public std::runtime_error {
    int line = 0;
    int column = 0;
    ParseError(int l, int c, const std::string& msg)
        : std::runtime_error("line " + std::to_string(l) + ":" +
                             std::to_string(c) + ": " + msg),
          line(l), column(c) {}
};

enum class TokenKind {
    Ident, String, Number,
    Colon, Comma, Semicolon, Dot,
    LBrace, RBrace, LParen, RParen, LBracket, RBracket,
    Arrow, Eq, Ne, Ge, Le, Gt, Lt,
    PlusEq, MinusEq, EqAssign,
    Eof,
};

struct Token {
    TokenKind kind = TokenKind::Eof;
    std::string text;
    std::int64_t number = 0;
    int line = 0;
    int column = 0;
};

class Lexer {
public:
    explicit Lexer(const std::string& src) : s_(src) {}
    std::vector<Token> tokenize();

private:
    Token next();
    const std::string& s_;
    std::size_t i_ = 0;
    int line_ = 1;
    int col_ = 1;
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& toks) : toks_(toks), p_(0) {}

    ParsedQuest parse_quest();

private:
    const Token& peek(std::size_t off = 0) const;
    bool eat(TokenKind k);
    bool eat_keyword(const char* kw);
    bool is_keyword(const char* kw) const;
    bool is_punct(TokenKind k) const;
    void expect(TokenKind k, const char* what);

    void parse_meta(ParsedQuest& out);
    void parse_stage(ParsedQuest& out);
    void parse_branch_block(std::vector<Branch>& out);

    void parse_stage_body(Stage& st);
    void parse_action_block(std::vector<Action>& out);
    void parse_condition_block(Condition& out);
    Condition parse_condition_expr();
    Action    parse_action_line();
    void      parse_subreq_list(std::vector<SubReq>& out, bool allow_kill);
    SubReq    parse_subreq_one(bool allow_kill);

    std::string parse_ident_or_kw();
    void        skip_block(TokenKind open, TokenKind close);

    const std::vector<Token>& toks_;
    std::size_t p_;
};

ParsedQuest parse_qst_string(const std::string& src);
ParsedQuest parse_qst_file(const std::string& path);

}  // namespace quest
}  // namespace ash