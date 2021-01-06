#include "lexer.h"

#include <cctype>
#include "util.h"

using namespace alanfl;
using namespace std;

wstring alanfl::token_str(token_type tok) {
	switch(tok) { // Convert token into readable string
	case token_type::unknown: return L"unknown";
	case token_type::eof: return L"eof";
	case token_type::add: return L"+";
	case token_type::sub: return L"-";
	case token_type::mul: return L"*";
	case token_type::div: return L"/";
	case token_type::inc: return L"++";
	case token_type::dec: return L"--";
	case token_type::assign: return L"=";
	case token_type::identifier: return L"identifier";
	case token_type::integer: return L"integer";
	case token_type::decimal: return L"decimal";
	case token_type::left_brace: return L"{";
	case token_type::right_brace: return L"}";
	case token_type::left_bracket: return L"[";
	case token_type::right_bracket: return L"]";
	case token_type::left_parenthesis: return L"(";
	case token_type::right_parenthesis: return L")";
	case token_type::kw_var: return L"var";
	case token_type::kw_true: return L"true";
	case token_type::kw_false: return L"false";
	case token_type::kw_if: return L"if";
	case token_type::kw_else: return L"else";
	case token_type::kw_while: return L"while";
	case token_type::kw_break: return L"break";
	case token_type::kw_return: return L"return";
	case token_type::kw_fn: return L"fn";
	case token_type::semicolon: return L";";
	case token_type::lt: return L"<";
	case token_type::lteq: return L"<=";
	case token_type::gt: return L">";
	case token_type::gteq: return L">=";
	case token_type::eq: return L"==";
	case token_type::neq: return L"!=";
	case token_type::lnot: return L"1";
	case token_type::land: return L"&&";
	case token_type::lor: return L"||";
	case token_type::comma: return L",";
	}
	unreachable("unknown token type");
	return wstring();
}

wostream& alanfl::operator<<(wostream& out, const source_location& fp) {
	return out << fp.line << L':' << fp.col;
}

wostream& alanfl::operator<<(wostream& out, const token& t) {
	return out << L'"' << t.text << L"\"(" << t.begin << L'-' << t.end << ")";
}

void lexer::consume_char(const bool append_to_text) {
	input.get(ch);
	if (ch == '\n')
		loc.line++, loc.col = 1;
	else
		loc.col++;
	if (append_to_text)
		text += ch;
}

void lexer::skip_whitespaces() {
	while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
		consume_char(false);
}

void lexer::start_token() {
	begin_loc = loc;
	text.clear();
	text += ch;
}

token lexer::end_token(const token_type type) {
	text.pop_back();
	token ret = token(text, begin_loc, loc, type);
	return ret;
}

#define SINGLE_CHAR_TOKEN(c, tok) \
	if (ch == (c)) \
		return consume_char(), end_token(tok)
#define DOUBLE_CHAR_TOKEN(c1, c2, tok1, tok2) \
	if (ch == (c1)) do { \
		consume_char(); \
		SINGLE_CHAR_TOKEN(c2, tok2); \
		return end_token(tok1); \
	} while (0)
#define DOUBLE_CHAR_TOKEN_SEQ(c1, c2, tok) DOUBLE_CHAR_TOKEN(c1, c2, token_type::unknown, tok)
#define KEYWORD_TOKEN(str, tok) if (s == str) return end_token(tok)

token lexer::next_token() {
	skip_whitespaces();
	start_token();

	if (eof())
		return end_token(token_type::eof);

	if (iswdigit(ch)) {
		while (iswdigit(ch))
			consume_char();
		if (ch == '.') {
			consume_char();
			while (iswdigit(ch))
				consume_char();
			return end_token(token_type::decimal);
		}
		return end_token(token_type::integer);
	}

	if (iswalpha(ch)) {
		while (iswalpha(ch) || iswdigit(ch) || ch == '_')
			consume_char();
		auto s = text; s.pop_back();
		KEYWORD_TOKEN(L"var", token_type::kw_var);
		KEYWORD_TOKEN(L"true", token_type::kw_true);
		KEYWORD_TOKEN(L"false", token_type::kw_false);
		KEYWORD_TOKEN(L"if", token_type::kw_if);
		KEYWORD_TOKEN(L"else", token_type::kw_else);
		KEYWORD_TOKEN(L"while", token_type::kw_while);
		KEYWORD_TOKEN(L"break", token_type::kw_break);
		KEYWORD_TOKEN(L"fn", token_type::kw_fn);
		KEYWORD_TOKEN(L"return", token_type::kw_return);
		return end_token(token_type::identifier);
	}

	DOUBLE_CHAR_TOKEN('+', '+', token_type::add, token_type::inc);
	DOUBLE_CHAR_TOKEN('-', '-', token_type::sub, token_type::dec);

	SINGLE_CHAR_TOKEN('*', token_type::mul);
	SINGLE_CHAR_TOKEN('/', token_type::div);
	SINGLE_CHAR_TOKEN('(', token_type::left_parenthesis);
	SINGLE_CHAR_TOKEN(')', token_type::right_parenthesis);
	SINGLE_CHAR_TOKEN('[', token_type::left_bracket);
	SINGLE_CHAR_TOKEN(']', token_type::right_bracket);
	SINGLE_CHAR_TOKEN('{', token_type::left_brace);
	SINGLE_CHAR_TOKEN('}', token_type::right_brace);
	SINGLE_CHAR_TOKEN(';', token_type::semicolon);
	SINGLE_CHAR_TOKEN(',', token_type::comma);

	DOUBLE_CHAR_TOKEN('<', '=', token_type::lt, token_type::lteq);
	DOUBLE_CHAR_TOKEN('>', '=', token_type::gt, token_type::gteq);
	DOUBLE_CHAR_TOKEN('=', '=', token_type::assign, token_type::eq);
	DOUBLE_CHAR_TOKEN('!', '=', token_type::lnot, token_type::neq);

	DOUBLE_CHAR_TOKEN_SEQ('&', '&', token_type::land);
	DOUBLE_CHAR_TOKEN_SEQ('|', '|', token_type::lor);

	consume_char();
	return end_token(token_type::unknown);
}
