#include "quest/qst_parser.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>

namespace ash {
namespace quest {

// ----- helpers ---------------------------------------------------------

namespace {

bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}
bool is_ident_cont(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::int64_t parse_int(const std::string& s, std::size_t& i) {
    bool neg = false;
    if (s[i] == '-') { neg = true; ++i; }
    std::int64_t v = 0;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
        v = v * 10 + (s[i] - '0');
        ++i;
    }
    return neg ? -v : v;
}

void skip_ws(const std::string& s, std::size_t& i, int& line, int& col) {
    while (i < s.size()) {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\r') { ++i; ++col; continue; }
        if (c == '\n') { ++i; ++line; col = 1; continue; }
        if (c == '/' && i + 1 < s.size() && s[i + 1] == '/') {
            while (i < s.size() && s[i] != '\n') { ++i; ++col; }
            continue;
        }
        if (c == '/' && i + 1 < s.size() && s[i + 1] == '*') {
            ++i; ++col; ++i; ++col;
            while (i + 1 < s.size() && !(s[i] == '*' && s[i + 1] == '/')) {
                if (s[i] == '\n') { ++line; col = 1; } else { ++col; }
                ++i;
            }
            if (i + 1 < s.size()) { i += 2; col += 2; }
            continue;
        }
        break;
    }
}

}  // namespace

// ----- Lexer -----------------------------------------------------------

Token Lexer::next() {
    skip_ws(s_, i_, line_, col_);
    if (i_ >= s_.size()) {
        Token t; t.kind = TokenKind::Eof; t.line = line_; t.column = col_;
        return t;
    }

    char c = s_[i_];
    Token t; t.line = line_; t.column = col_;

    if (is_ident_start(c)) {
        std::size_t start = i_;
        while (i_ < s_.size() && is_ident_cont(s_[i_])) { ++i_; ++col_; }
        t.kind = TokenKind::Ident;
        t.text = s_.substr(start, i_ - start);
        return t;
    }
    if (std::isdigit(static_cast<unsigned char>(c)) || (c == '-' && i_ + 1 < s_.size() &&
        std::isdigit(static_cast<unsigned char>(s_[i_ + 1])))) {
        std::size_t start = i_;
        t.number = parse_int(s_, i_);
        // decimal part? skip
        while (i_ < s_.size() && (std::isdigit(static_cast<unsigned char>(s_[i_])) ||
               s_[i_] == '.')) {
            ++i_; ++col_;
        }
        t.kind = TokenKind::Number;
        t.text = s_.substr(start, i_ - start);
        return t;
    }
    if (c == '"') {
        ++i_; ++col_;
        std::string out;
        while (i_ < s_.size() && s_[i_] != '"') {
            if (s_[i_] == '\\' && i_ + 1 < s_.size()) {
                char nx = s_[i_ + 1];
                switch (nx) {
                    case 'n':  out += '\n'; break;
                    case 't':  out += '\t'; break;
                    case 'r':  out += '\r'; break;
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    default:   out += nx;   break;
                }
                i_ += 2; col_ += 2;
            } else {
                out += s_[i_++]; ++col_;
            }
        }
        if (i_ >= s_.size()) throw ParseError(line_, col_, "unterminated string");
        ++i_; ++col_;
        t.kind = TokenKind::String;
        t.text = std::move(out);
        return t;
    }

    // punctuation
    auto single = [&](TokenKind k, std::size_t n) {
        t.kind = k; i_ += n; col_ += static_cast<int>(n); return t;
    };
    auto pairc = [&](char /*a*/, char /*b*/, TokenKind k) -> Token {
        t.kind = k; i_ += 2; col_ += 2; return t;
    };
    switch (c) {
        case ':': return single(TokenKind::Colon, 1);
        case ',': return single(TokenKind::Comma, 1);
        case ';': return single(TokenKind::Semicolon, 1);
        case '.': return single(TokenKind::Dot, 1);
        case '{': return single(TokenKind::LBrace, 1);
        case '}': return single(TokenKind::RBrace, 1);
        case '(': return single(TokenKind::LParen, 1);
        case ')': return single(TokenKind::RParen, 1);
        case '[': return single(TokenKind::LBracket, 1);
        case ']': return single(TokenKind::RBracket, 1);
        case '+': if (i_ + 1 < s_.size() && s_[i_ + 1] == '=') return pairc('+','=',TokenKind::PlusEq); return single(TokenKind::Eof,0);
        case '-': if (i_ + 1 < s_.size() && s_[i_ + 1] == '>') return pairc('-','>',TokenKind::Arrow);
                  if (i_ + 1 < s_.size() && s_[i_ + 1] == '=') return pairc('-','=',TokenKind::MinusEq);
                  return single(TokenKind::Eof,0);
        case '=': if (i_ + 1 < s_.size() && s_[i_ + 1] == '=') return pairc('=','=',TokenKind::Eq);
                  return single(TokenKind::EqAssign, 1);
        case '!': if (i_ + 1 < s_.size() && s_[i_ + 1] == '=') return pairc('!','=',TokenKind::Ne); return single(TokenKind::Eof,0);
        case '>': if (i_ + 1 < s_.size() && s_[i_ + 1] == '=') return pairc('>','=',TokenKind::Ge); return single(TokenKind::Gt, 1);
        case '<': if (i_ + 1 < s_.size() && s_[i_ + 1] == '=') return pairc('<','=',TokenKind::Le); return single(TokenKind::Lt, 1);
    }
    std::string msg = "unexpected character '";
    msg += c;
    msg += "'";
    throw ParseError(line_, col_, msg);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> out;
    for (;;) {
        Token t = next();
        out.push_back(t);
        if (t.kind == TokenKind::Eof) break;
    }
    return out;
}

// ----- Parser ---------------------------------------------------------

const Token& Parser::peek(std::size_t off) const {
    return toks_[std::min(p_ + off, toks_.size() - 1)];
}

bool Parser::eat(TokenKind k) {
    if (toks_[p_].kind == k) { ++p_; return true; }
    return false;
}

bool Parser::is_keyword(const char* kw) const {
    if (toks_[p_].kind != TokenKind::Ident) return false;
    return toks_[p_].text == kw;
}

bool Parser::eat_keyword(const char* kw) {
    if (is_keyword(kw)) { ++p_; return true; }
    return false;
}

bool Parser::is_punct(TokenKind k) const { return toks_[p_].kind == k; }

void Parser::expect(TokenKind k, const char* what) {
    if (toks_[p_].kind != k) {
        throw ParseError(toks_[p_].line, toks_[p_].column,
                         std::string("expected ") + what);
    }
    ++p_;
}

std::string Parser::parse_ident_or_kw() {
    if (toks_[p_].kind != TokenKind::Ident && toks_[p_].kind != TokenKind::String) {
        throw ParseError(toks_[p_].line, toks_[p_].column,
                         "expected identifier or string");
    }
    auto s = toks_[p_].text;
    ++p_;
    return s;
}

void Parser::skip_block(TokenKind open, TokenKind close) {
    if (!eat(open)) return;
    int depth = 1;
    while (p_ < toks_.size() && depth > 0) {
        if (toks_[p_].kind == open) ++depth;
        else if (toks_[p_].kind == close) --depth;
        ++p_;
    }
}

void Parser::parse_meta(ParsedQuest& out) {
    expect(TokenKind::LBrace, "{");
    while (!eat(TokenKind::RBrace)) {
        auto key = parse_ident_or_kw();
        expect(TokenKind::Colon, ":");
        if (key == "name") {
            out.def.meta.name = parse_ident_or_kw();
        } else if (key == "category") {
            out.def.meta.category = parse_ident_or_kw();
        } else if (key == "priority") {
            if (toks_[p_].kind != TokenKind::Number)
                throw ParseError(toks_[p_].line, toks_[p_].column,
                                 "expected number for priority");
            out.def.meta.priority = toks_[p_].number;
            ++p_;
        } else if (key == "giver") {
            out.def.meta.giver = parse_ident_or_kw();
        } else if (key == "sort_index") {
            if (toks_[p_].kind != TokenKind::Number)
                throw ParseError(toks_[p_].line, toks_[p_].column,
                                 "expected number for sort_index");
            out.def.meta.sort_index = toks_[p_].number;
            ++p_;
        } else if (key == "auto_complete") {
            if (is_keyword("true"))      { out.def.meta.auto_complete = true; ++p_; }
            else if (is_keyword("false")){ out.def.meta.auto_complete = false; ++p_; }
            else if (toks_[p_].kind == TokenKind::Number)
                out.def.meta.auto_complete = (toks_[p_++].number != 0);
            else throw ParseError(toks_[p_].line, toks_[p_].column,
                                  "expected bool for auto_complete");
        } else if (key == "hidden_until") {
            Condition c = parse_condition_expr();
            out.def.meta.hidden_until = std::move(c);
        } else {
            // unknown key: skip value
            if (toks_[p_].kind == TokenKind::LBrace) skip_block(TokenKind::LBrace, TokenKind::RBrace);
            else ++p_;
        }
        eat(TokenKind::Semicolon);
    }
}

void Parser::parse_branch_block(std::vector<Branch>& out) {
    expect(TokenKind::LBrace, "{");
    while (!eat(TokenKind::RBrace)) {
        Branch b;
        if (eat_keyword("else")) {
            // bare `else` means fallthrough; `else if (...)` recurses.
            if (!is_keyword("if")) {
                b.cond.op = CondOp::True;
                expect(TokenKind::Arrow, "->");
                b.target_stage = parse_ident_or_kw();
                out.push_back(std::move(b));
                eat(TokenKind::Semicolon);
                continue;
            }
            eat_keyword("if");
            expect(TokenKind::LParen, "(");
            b.cond = parse_condition_expr();
            expect(TokenKind::RParen, ")");
            expect(TokenKind::Arrow, "->");
            b.target_stage = parse_ident_or_kw();
            out.push_back(std::move(b));
        } else if (eat_keyword("if")) {
            expect(TokenKind::LParen, "(");
            b.cond = parse_condition_expr();
            expect(TokenKind::RParen, ")");
            expect(TokenKind::Arrow, "->");
            b.target_stage = parse_ident_or_kw();
            out.push_back(std::move(b));
        } else {
            throw ParseError(toks_[p_].line, toks_[p_].column,
                             "branch: expected if/else");
        }
        eat(TokenKind::Semicolon);
    }
}

void Parser::parse_action_block(std::vector<Action>& out) {
    expect(TokenKind::LBrace, "{");
    while (!eat(TokenKind::RBrace)) {
        Action a = parse_action_line();
        out.push_back(std::move(a));
        eat(TokenKind::Semicolon);
    }
}

Action Parser::parse_action_line() {
    Action a;
    auto kw = parse_ident_or_kw();
    if (kw == "set_flag") {
        a.kind = ActionKind::SetFlag;
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::EqAssign, "=");
        a.args.push_back(parse_ident_or_kw());
        return a;
    }
    if (kw == "inc_flag") {
        a.kind = ActionKind::IncFlag;
        a.args.push_back(parse_ident_or_kw());
        if (eat(TokenKind::Comma)) {
            if (toks_[p_].kind != TokenKind::Number)
                throw ParseError(toks_[p_].line, toks_[p_].column,
                                 "expected int");
            a.args.push_back(std::to_string(toks_[p_].number));
            ++p_;
        } else {
            a.args.push_back("1");
        }
        return a;
    }
    if (kw == "toggle_flag") {
        a.kind = ActionKind::ToggleFlag;
        a.args.push_back(parse_ident_or_kw());
        return a;
    }
    if (kw == "clear_flag") {
        a.kind = ActionKind::ClearFlag;
        a.args.push_back(parse_ident_or_kw());
        return a;
    }
    if (kw == "give_item") {
        a.kind = ActionKind::GiveItem;
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        if (toks_[p_].kind != TokenKind::Number)
            throw ParseError(toks_[p_].line, toks_[p_].column, "expected count");
        a.args.push_back(std::to_string(toks_[p_].number));
        ++p_;
        return a;
    }
    if (kw == "take_item") {
        a.kind = ActionKind::TakeItem;
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        if (toks_[p_].kind != TokenKind::Number)
            throw ParseError(toks_[p_].line, toks_[p_].column, "expected count");
        a.args.push_back(std::to_string(toks_[p_].number));
        ++p_;
        return a;
    }
    if (kw == "give_xp") {
        a.kind = ActionKind::GiveXp;
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        if (toks_[p_].kind != TokenKind::Number)
            throw ParseError(toks_[p_].line, toks_[p_].column, "expected amount");
        a.args.push_back(std::to_string(toks_[p_].number));
        ++p_;
        return a;
    }
    if (kw == "disposition") {
        a.kind = ActionKind::Disposition;
        a.args.push_back(parse_ident_or_kw());
        if (eat(TokenKind::PlusEq))      a.args.push_back("+=");
        else if (eat(TokenKind::EqAssign)) a.args.push_back("=");
        else throw ParseError(toks_[p_].line, toks_[p_].column, "expected = or +=");
        if (toks_[p_].kind != TokenKind::Number)
            throw ParseError(toks_[p_].line, toks_[p_].column, "expected number");
        a.args.push_back(std::to_string(toks_[p_].number));
        ++p_;
        return a;
    }
    if (kw == "reputation") {
        a.kind = ActionKind::Reputation;
        a.args.push_back(parse_ident_or_kw());
        if (eat(TokenKind::PlusEq))        a.args.push_back("+=");
        else if (eat(TokenKind::EqAssign)) a.args.push_back("=");
        else throw ParseError(toks_[p_].line, toks_[p_].column, "expected = or +=");
        if (toks_[p_].kind != TokenKind::Number)
            throw ParseError(toks_[p_].line, toks_[p_].column, "expected number");
        a.args.push_back(std::to_string(toks_[p_].number));
        ++p_;
        return a;
    }
    if (kw == "journal") {
        a.kind = ActionKind::JournalAdd;
        if (!eat_keyword("add")) {
            throw ParseError(toks_[p_].line, toks_[p_].column, "expected 'add'");
        }
        expect(TokenKind::Comma, ",");
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        a.args.push_back(parse_ident_or_kw());
        return a;
    }
    if (kw == "spawn_npc") {
        a.kind = ActionKind::SpawnNpc;
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        if (toks_[p_].kind != TokenKind::Number) throw ParseError(toks_[p_].line, toks_[p_].column, "expected x");
        a.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        expect(TokenKind::Comma, ",");
        if (toks_[p_].kind != TokenKind::Number) throw ParseError(toks_[p_].line, toks_[p_].column, "expected y");
        a.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        return a;
    }
    if (kw == "despawn_npc") {
        a.kind = ActionKind::DespawnNpc;
        a.args.push_back(parse_ident_or_kw());
        return a;
    }
    if (kw == "set_stage") {
        a.kind = ActionKind::SetStage;
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        a.args.push_back(parse_ident_or_kw());
        return a;
    }
    if (kw == "complete_quest") {
        a.kind = ActionKind::CompleteQuest;
        a.args.push_back(parse_ident_or_kw());
        return a;
    }
    if (kw == "fail_quest") {
        a.kind = ActionKind::FailQuest;
        a.args.push_back(parse_ident_or_kw());
        return a;
    }
    if (kw == "teleport") {
        a.kind = ActionKind::Teleport;
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        a.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        if (toks_[p_].kind != TokenKind::Number) throw ParseError(toks_[p_].line, toks_[p_].column, "expected x");
        a.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        expect(TokenKind::Comma, ",");
        if (toks_[p_].kind != TokenKind::Number) throw ParseError(toks_[p_].line, toks_[p_].column, "expected y");
        a.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        return a;
    }
    throw ParseError(toks_[p_].line, toks_[p_].column,
                     "unknown action: " + kw);
}

SubReq Parser::parse_subreq_one(bool allow_kill) {
    expect(TokenKind::LBrace, "{");
    SubReq r;
    bool dispatched = false;
    while (!eat(TokenKind::RBrace)) {
        auto key = parse_ident_or_kw();
        expect(TokenKind::Colon, ":");
        if (key == "item") {
            ItemReq req;
            req.item = parse_ident_or_kw();
            r = req;
            dispatched = true;
        } else if (key == "count") {
            // Attach count to whichever variant is current.
            if (auto* p = std::get_if<ItemReq>(&r)) {
                if (toks_[p_].kind != TokenKind::Number)
                    throw ParseError(toks_[p_].line, toks_[p_].column, "expected number");
                p->count = toks_[p_].number; ++p_;
            } else if (auto* kp = std::get_if<KillReq>(&r)) {
                if (toks_[p_].kind != TokenKind::Number)
                    throw ParseError(toks_[p_].line, toks_[p_].column, "expected number");
                kp->count = toks_[p_].number; ++p_;
            } else {
                throw ParseError(toks_[p_].line, toks_[p_].column,
                                 "count without item/faction");
            }
        } else if (key == "examine") {
            ExamineReq req;
            req.entity_id = parse_ident_or_kw();
            r = req;
            dispatched = true;
        } else if (key == "talk_to") {
            TalkReq req;
            req.npc_id = parse_ident_or_kw();
            r = req;
            dispatched = true;
        } else if (key == "flag") {
            FlagReq req;
            req.key = parse_ident_or_kw();
            r = req;
            dispatched = true;
        } else if (key == "faction" && allow_kill) {
            KillReq req;
            req.faction = parse_ident_or_kw();
            r = req;
            dispatched = true;
        } else {
            throw ParseError(toks_[p_].line, toks_[p_].column,
                             "unknown subreq key: " + key);
        }
        eat(TokenKind::Comma);
    }
    if (!dispatched) throw ParseError(toks_[p_].line, toks_[p_].column,
                                       "empty subreq");
    return r;
}

void Parser::parse_subreq_list(std::vector<SubReq>& out, bool allow_kill) {
    expect(TokenKind::LBracket, "[");
    while (!eat(TokenKind::RBracket)) {
        out.push_back(parse_subreq_one(allow_kill));
        eat(TokenKind::Comma);
    }
}

Condition Parser::parse_condition_expr() {
    Condition c;
    auto kw = parse_ident_or_kw();
    if (kw == "AND") {
        c.op = CondOp::And;
        expect(TokenKind::LParen, "(");
        while (!eat(TokenKind::RParen)) {
            c.children.push_back(parse_condition_expr());
            eat(TokenKind::Comma);
        }
        return c;
    }
    if (kw == "OR") {
        c.op = CondOp::Or;
        expect(TokenKind::LParen, "(");
        while (!eat(TokenKind::RParen)) {
            c.children.push_back(parse_condition_expr());
            eat(TokenKind::Comma);
        }
        return c;
    }
    if (kw == "NOT") {
        c.op = CondOp::Not;
        c.children.push_back(parse_condition_expr());
        return c;
    }
    if (kw == "true") {
        c.op = CondOp::True;
        return c;
    }
    if (kw == "in_combat") {
        c.op = CondOp::InCombat;
        return c;
    }
    if (kw == "weather") {
        c.op = CondOp::Weather;
        c.args.push_back(parse_ident_or_kw());
        return c;
    }
    if (kw == "time") {
        c.op = CondOp::TimeOfDay;
        // time:hour or time:day
        expect(TokenKind::Colon, ":");
        auto unit = parse_ident_or_kw();
        c.args.push_back(unit);
        return c;
    }
    if (kw == "reputation") {
        c.op = CondOp::RepAtLeast;
        expect(TokenKind::LParen, "(");
        c.args.push_back(parse_ident_or_kw());
        expect(TokenKind::RParen, ")");
        if (eat(TokenKind::Ge)) {
            if (toks_[p_].kind != TokenKind::Number)
                throw ParseError(toks_[p_].line, toks_[p_].column, "expected number");
            c.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        }
        return c;
    }
    if (kw == "disposition") {
        c.op = CondOp::DispositionAtLeast;
        expect(TokenKind::LParen, "(");
        c.args.push_back(parse_ident_or_kw());
        expect(TokenKind::RParen, ")");
        if (eat(TokenKind::Ge)) {
            if (toks_[p_].kind != TokenKind::Number)
                throw ParseError(toks_[p_].line, toks_[p_].column, "expected number");
            c.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        }
        return c;
    }
    if (kw == "has_item") {
        expect(TokenKind::LParen, "(");
        c.args.push_back(parse_ident_or_kw());
        if (eat(TokenKind::Comma)) {
            if (toks_[p_].kind != TokenKind::Number)
                throw ParseError(toks_[p_].line, toks_[p_].column, "expected count");
            c.args.push_back(std::to_string(toks_[p_].number)); ++p_;
            c.op = CondOp::HasItemCount;
        } else {
            c.op = CondOp::HasItem;
        }
        expect(TokenKind::RParen, ")");
        return c;
    }
    if (kw == "skill") {
        c.op = CondOp::SkillAtLeast;
        expect(TokenKind::Colon, ":");
        c.args.push_back(parse_ident_or_kw());
        if (eat(TokenKind::Ge)) {
            if (toks_[p_].kind != TokenKind::Number)
                throw ParseError(toks_[p_].line, toks_[p_].column, "expected number");
            c.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        }
        return c;
    }
    if (kw == "completed_quest") {
        c.op = CondOp::CompletedQuest;
        expect(TokenKind::LParen, "(");
        c.args.push_back(parse_ident_or_kw());
        expect(TokenKind::RParen, ")");
        return c;
    }
    if (kw == "active_quest") {
        c.op = CondOp::ActiveQuest;
        expect(TokenKind::LParen, "(");
        c.args.push_back(parse_ident_or_kw());
        expect(TokenKind::RParen, ")");
        return c;
    }
    if (kw == "at") {
        c.op = CondOp::AtLocation;
        expect(TokenKind::LParen, "(");
        c.args.push_back(parse_ident_or_kw());
        expect(TokenKind::Comma, ",");
        if (toks_[p_].kind != TokenKind::Number) throw ParseError(toks_[p_].line, toks_[p_].column, "expected x");
        c.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        expect(TokenKind::Comma, ",");
        if (toks_[p_].kind != TokenKind::Number) throw ParseError(toks_[p_].line, toks_[p_].column, "expected y");
        c.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        expect(TokenKind::RParen, ")");
        return c;
    }
    if (kw == "flag") {
        c.op = CondOp::HasFlag;
        expect(TokenKind::Colon, ":");
        c.args.push_back(parse_ident_or_kw());
        if (eat(TokenKind::Eq)) {
            c.op = CondOp::FlagEquals;
            c.args.push_back(parse_ident_or_kw());
        } else if (eat(TokenKind::Ge)) {
            c.op = CondOp::FlagAtLeast;
            if (toks_[p_].kind != TokenKind::Number)
                throw ParseError(toks_[p_].line, toks_[p_].column, "expected number");
            c.args.push_back(std::to_string(toks_[p_].number)); ++p_;
        }
        return c;
    }
    // unknown -> treat as FlagEquals with key=arg
    c.op = CondOp::HasFlag;
    c.args.push_back(kw);
    return c;
}

void Parser::parse_condition_block(Condition& out) {
    expect(TokenKind::LBrace, "{");
    out = parse_condition_expr();
    eat(TokenKind::Semicolon);
    expect(TokenKind::RBrace, "}");
}

void Parser::parse_stage_body(Stage& st) {
    expect(TokenKind::LBrace, "{");
    while (!eat(TokenKind::RBrace)) {
        auto key = parse_ident_or_kw();
        // Block-style keys (no colon): on_*, condition, branch,
        // objective_all/any/count.
        if (key == "on_enter")     { parse_action_block(st.on_enter); }
        else if (key == "on_exit")   { parse_action_block(st.on_exit); }
        else if (key == "on_complete"){ parse_action_block(st.on_complete); }
        else if (key == "on_fail")   { parse_action_block(st.on_fail); }
        else if (key == "on_arrive") { parse_action_block(st.on_arrive); }
        else if (key == "on_timeout"){ parse_action_block(st.on_timeout); }
        else if (key == "condition") { parse_condition_block(st.completion); }
        else if (key == "branch")    { parse_branch_block(st.branches); st.kind = StageKind::Branch; }
        else if (key == "objective_all") {
            parse_subreq_list(st.objective_all, /*allow_kill=*/false);
            st.kind = StageKind::Simultaneous;
        }
        else if (key == "objective_any") {
            parse_subreq_list(st.objective_any, /*allow_kill=*/false);
            st.kind = StageKind::Investigate;
        }
        else if (key == "objective_count") {
            expect(TokenKind::LBrace, "{");
            KillReq req;
            while (!eat(TokenKind::RBrace)) {
                auto inner_key = parse_ident_or_kw();
                expect(TokenKind::Colon, ":");
                if (inner_key == "faction") {
                    req.faction = parse_ident_or_kw();
                } else if (inner_key == "count") {
                    if (toks_[p_].kind != TokenKind::Number)
                        throw ParseError(toks_[p_].line, toks_[p_].column, "expected count");
                    req.count = toks_[p_].number; ++p_;
                } else {
                    throw ParseError(toks_[p_].line, toks_[p_].column,
                                     "unknown objective_count key: " + inner_key);
                }
                eat(TokenKind::Comma);
            }
            st.objective_count = req;
            st.kind = StageKind::Kill;
        }
        else {
            expect(TokenKind::Colon, ":");
            if (key == "description") {
                st.description = parse_ident_or_kw();
            } else if (key == "objective") {
                st.objective = parse_ident_or_kw();
            } else if (key == "duration") {
                st.duration = parse_ident_or_kw();
                st.kind = StageKind::Timed;
                if (!st.duration.empty() && st.duration.back() == 's') {
                    std::string num = st.duration.substr(0, st.duration.size() - 1);
                    std::size_t i = 0;
                    bool neg = false;
                    if (i < num.size() && num[i] == '-') { neg = true; ++i; }
                    std::int64_t v = 0;
                    bool any = false;
                    while (i < num.size() && std::isdigit(static_cast<unsigned char>(num[i]))) {
                        v = v * 10 + (num[i] - '0');
                        ++i;
                        any = true;
                    }
                    if (any) st.duration_secs = neg ? -v : v;
                }
            } else if (key == "escort_npc") {
                st.escort_npc = parse_ident_or_kw();
                st.kind = StageKind::Escort;
            } else if (key == "destination") {
                st.destination = parse_ident_or_kw();
            } else if (key == "next") {
                if (!eat(TokenKind::Semicolon)) {
                    st.next = parse_ident_or_kw();
                }
            } else {
                if (toks_[p_].kind == TokenKind::LBrace)
                    skip_block(TokenKind::LBrace, TokenKind::RBrace);
                else if (toks_[p_].kind == TokenKind::LBracket)
                    skip_block(TokenKind::LBracket, TokenKind::RBracket);
                else ++p_;
            }
        }
        eat(TokenKind::Semicolon);
    }
}

void Parser::parse_stage(ParsedQuest& out) {
    auto sid = parse_ident_or_kw();
    Stage st;
    st.id = sid;
    parse_stage_body(st);
    out.def.stages.push_back(std::move(st));
}

ParsedQuest Parser::parse_quest() {
    ParsedQuest out;
    if (!eat_keyword("quest")) {
        throw ParseError(toks_[p_].line, toks_[p_].column, "expected 'quest'");
    }
    out.def.id = parse_ident_or_kw();
    expect(TokenKind::LBrace, "{");
    while (!eat(TokenKind::RBrace)) {
        auto key = parse_ident_or_kw();
        // `meta` and `stage` are followed by a `{ ... }` block directly
        // (no colon). `entry` and `next` use colon syntax.
        if (key == "meta") {
            parse_meta(out);
        } else if (key == "stage") {
            parse_stage(out);
        } else {
            expect(TokenKind::Colon, ":");
            if (key == "entry") {
                if (!eat(TokenKind::Semicolon)) {
                    out.def.entry = parse_ident_or_kw();
                }
            } else {
                // unknown: skip value
                if (toks_[p_].kind == TokenKind::LBrace)
                    skip_block(TokenKind::LBrace, TokenKind::RBrace);
                else ++p_;
            }
        }
        eat(TokenKind::Semicolon);
    }
    if (toks_[p_].kind != TokenKind::Eof) {
        throw ParseError(toks_[p_].line, toks_[p_].column, "trailing tokens");
    }
    out.stage_index.reserve(out.def.stages.size());
    for (std::size_t i = 0; i < out.def.stages.size(); ++i) {
        out.stage_index.push_back({out.def.stages[i].id, i});
    }
    std::sort(out.stage_index.begin(), out.stage_index.end(),
              [](auto& a, auto& b) { return a.first < b.first; });
    for (std::size_t i = 1; i < out.stage_index.size(); ++i) {
        if (out.stage_index[i].first == out.stage_index[i - 1].first) {
            throw ParseError(0, 0,
                             "duplicate stage id: " + out.stage_index[i].first);
        }
    }
    return out;
}

ParsedQuest parse_qst_string(const std::string& src) {
    Lexer lex(src);
    auto toks = lex.tokenize();
    Parser par(toks);
    return par.parse_quest();
}

ParsedQuest parse_qst_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open file: " + path);
    std::stringstream ss; ss << f.rdbuf();
    return parse_qst_string(ss.str());
}

}  // namespace quest
}  // namespace asm