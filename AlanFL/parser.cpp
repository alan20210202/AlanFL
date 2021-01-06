#include <iostream>
#include "operators.h"
#include "parser.h"

using namespace alanfl;
using namespace std;

/*
 * To provide users accurate indication of where their code goes wrong (either in syntax or runtime),
 * we have to record the region an AST node corresponds to.
 * Since we are writing an Recursive Descent Parser, the span starts from where the 'function' starts,
 * and ends when the function returns. These macros and template functions help to do this...
 * Just ENTER; at the start of the function, and use RETURN() instead of keyword return when we return
 */
template <typename Ret>
shared_ptr<Ret> locate(shared_ptr<Ret> node, source_location begin, source_location end) {
	node->begin = begin, node->end = end;
	return node;
}
#define ENTER auto begin_loc = tok.begin
#define RETURN(val) return locate(val, begin_loc, prev_end)

/*
 * Some commonly used functions
 */
void parser::consume_token() {
	prev_end = tok.end;
	tok = lex->next_token();
}

/*
 * Wrappers for 'throw'
 */
void parser::error(const wstring msg) const {
	throw parse_error(tok.begin, msg);
}

void parser::error_unexpected(const wstring msg) const {
	throw parse_error(tok.begin, L"unexpected token: " + token_str(tok.type) + L", " + msg);
}

/*
 * Debug function, simply dump all the errors onto stdout
 */
void parser::dump_error() const {
	for (auto &e : errors)
		wcout << e.pos << '\t' << e.message << endl;
}

/*
 * Expression parsing
 * All binary ops except assigns are left-associative.
 * Precedence:
 * Primary
 * Function Call
 * Unary op
 * * /
 * + -
 * =
 * < > <= >=
 * == !=
 * &&
 * ||
 */
shared_ptr<identifier_node> parser::identifier() {
	ENTER;
	const auto ret = make_shared<identifier_node>(tok.text);
	consume_token();
	RETURN(ret);
}

shared_ptr<integer_node> parser::integer() {
	ENTER;
	const auto s = to_str(tok.text);
	consume_token();
	RETURN(make_shared<integer_node>(mpz_class(s)));
}

shared_ptr<decimal_node> parser::decimal() {
	ENTER;
	const auto s = to_str(tok.text);
	consume_token();
	RETURN(make_shared<decimal_node>(mpf_class(s)));
}

/*
 * Lambda syntax:
 * lambda	::= fn [captures] [params] block_stmt
 * captures	::= '[' var_init (',' var_init)* ']'
 * params	::= '(' var_init (',' var_init)* ')'
 * 
 * The introduction of function lambdas make parsing harder because one possible brace mismatch
 * will ruin parsing since now its hard to recover using skip-until strategy!
 * TODO: Figure out a better strategy for error recovery!
 */
shared_ptr<fn_node> parser::fn() {
	ENTER;
	if (!is(token_type::kw_fn))
		error_unexpected(L"lambda function should start with 'fn'");
	consume_token();
	auto ret = make_shared<fn_node>();
	if (is(token_type::left_bracket)) {
		consume_token();
		if (!is(token_type::right_bracket)) {
			do {
				if (is(token_type::comma))
					consume_token();
				ret->captures.emplace_back(var_init());
			} while (is(token_type::comma));
		}
		if (!is(token_type::right_bracket))
			error_unexpected(L"lambda captures should be enclosed by []");
		consume_token();
	}
	if (is(token_type::left_parenthesis)) {
		consume_token();
		if (!is(token_type::right_parenthesis)) {
			do {
				if (is(token_type::comma))
					consume_token();
				ret->params.emplace_back(var_init());
			} while (is(token_type::comma));
		}
		if (!is(token_type::right_parenthesis))
			error_unexpected(L"parameter definition should be enclosed by ()");
		consume_token();
	}
	ret->body = block();
	RETURN(ret);
}

shared_ptr<expr_node> parser::primary() {
	if (is(token_type::left_parenthesis)) {
		ENTER;
		consume_token();
		const auto ret = expr();
		if (!is(token_type::right_parenthesis))
			error_unexpected(L"expecting ')' while parsing parentheses surrounded expression");
		consume_token();
		RETURN(ret);
	}
	if (is(token_type::integer))
		return integer();
	if (is(token_type::identifier))
		return identifier();
	if (is(token_type::decimal))
		return decimal();
	if (is(token_type::kw_fn))
		return fn();
	if (is(token_type::kw_true)) {
		ENTER;
		consume_token();
		RETURN(make_shared<bool_node>(true));
	}
	if (is(token_type::kw_false)) {
		ENTER;
		consume_token();
		RETURN(make_shared<bool_node>(false));
	}
	
	error_unexpected(L"expecting integer, decimal, identifier, true, false or '()' while parsing primary expression");
}

shared_ptr<expr_node> parser::expr_fn_call() {
	ENTER;
	shared_ptr<expr_node> ret = primary();
	while (is(token_type::left_parenthesis)) {
		consume_token();
		ret = make_shared<fn_call_node>(ret);
		if (is(token_type::right_parenthesis)) {
			consume_token();
			continue;
		}
		do {
			if (is(token_type::comma))
				consume_token();
			static_pointer_cast<fn_call_node>(ret)->args.emplace_back(expr());
		} while (is(token_type::comma));
		if (is(token_type::right_parenthesis))
			consume_token();
	}
	RETURN(ret);
}

shared_ptr<expr_node> parser::expr_unop() {
	ENTER;
	if (is(token_type::sub, token_type::lnot)) {
		auto op = unop_from(tok.type);
		consume_token();
		return make_shared<unop_node>(expr_unop(), op);
	}
	RETURN(expr_fn_call());
}

shared_ptr<expr_node> parser::expr_mul_div() {
	ENTER;
	auto ret = expr_unop();
	while (is(token_type::mul, token_type::div)) {
		auto op = binop_from(tok.type);
		consume_token();
		ret = make_shared<binop_node>(ret, expr_unop(), op);
	}
	RETURN(ret);
}

shared_ptr<expr_node> parser::expr_add_sub() {
	ENTER;
	auto ret = expr_mul_div();
	while (is(token_type::add, token_type::sub)) {
		auto op = binop_from(tok.type);
		consume_token();
		ret = make_shared<binop_node>(ret, expr_mul_div(), op);
	}
	RETURN(ret);
}

shared_ptr<expr_node> parser::expr_assign() {
	ENTER;
	auto ret = expr_add_sub();
	if (is(token_type::assign)) {
		consume_token();
		ret = make_shared<binop_node>(ret, expr_assign(), binary_op::assign);
	}
	RETURN(ret);
}

shared_ptr<expr_node> parser::expr_cmp() {
	ENTER;
	auto ret = expr_assign();
	while (is(token_type::lt, token_type::lteq, token_type::gt, token_type::gteq)) {
		auto op = binop_from(tok.type);
		consume_token();
		ret = make_shared<binop_node>(ret, expr_assign(), op);
	}
	RETURN(ret);
}

shared_ptr<expr_node> parser::expr_eq_cmp() {
	ENTER;
	auto ret = expr_cmp();
	while (is(token_type::eq, token_type::neq)) {
		auto op = binop_from(tok.type);
		consume_token();
		ret = make_shared<binop_node>(ret, expr_cmp(), op);
	}
	RETURN(ret);
}

shared_ptr<expr_node> parser::expr_and() {
	ENTER;
	auto ret = expr_eq_cmp();
	while (is(token_type::land)) {
		auto op = binop_from(tok.type);
		consume_token();
		ret = make_shared<binop_node>(ret, expr_eq_cmp(), op);
	}
	RETURN(ret);
}

shared_ptr<expr_node> parser::expr_or() {
	ENTER;
	auto ret = expr_and();
	while (is(token_type::lor)) {
		auto op = binop_from(tok.type);
		consume_token();
		ret = make_shared<binop_node>(ret, expr_and(), op);
	}
	RETURN(ret);
}

shared_ptr<expr_node> parser::expr() {
	return expr_or();
}

/*
 * Statement parsing
 */

/*
 * When we catch errors thrown by lower-level parse functions, we first put the caught error onto the 
 * 'errors' list, then we adopt 'panic recovery'.
 * Error recoveries are all done on statement level.
 * 
 * These are some common tokens that we skip to when the parser 'panics'
 */
#define COMMON_ERR_REC_TOKS token_type::kw_return, token_type::kw_break, token_type::kw_if, token_type::kw_else, token_type::kw_var, token_type::semicolon, token_type::right_brace

shared_ptr<stmt_node> parser::expr_stmt() {
	ENTER;
	shared_ptr<stmt_node> ret = nullptr;
	try {
		ret = make_shared<expr_stmt_node>(expr());
		if (!is(token_type::semicolon))
			error_unexpected(L"expecting ';' after an expression statement");
		consume_token();
		RETURN(ret);
	} catch (parse_error &e) {
		errors.emplace_back(e);
		skip_until(COMMON_ERR_REC_TOKS);
		if (ret == nullptr)
			RETURN(make_shared<empty_stmt_node>());
		RETURN(ret);
	}
}

shared_ptr<stmt_node> parser::if_stmt() {
	ENTER;
	try {
		if (!is(token_type::kw_if))
			error_unexpected(L"expecting 'if' at the beginning of an if statement");
		consume_token();
		auto cond = expr();
		auto branch = stmt();
		shared_ptr<stmt_node> else_branch = nullptr;
		if (is(token_type::kw_else))
			consume_token(), else_branch = stmt();
		RETURN(make_shared<if_stmt_node>(cond, branch, else_branch));
	} catch (parse_error &e) {
		errors.emplace_back(e);
		skip_until(COMMON_ERR_REC_TOKS);
		RETURN(make_shared<empty_stmt_node>());
	}
}

shared_ptr<stmt_node> parser::while_stmt() {
	ENTER;
	try {
		if (!is(token_type::kw_while))
			error_unexpected(L"expecting 'while' at the beginning of a while statement");
		consume_token();
		auto cond = expr();
		auto body = stmt();
		RETURN(make_shared<while_stmt_node>(cond, body));
	} catch (parse_error &e) {
		errors.emplace_back(e);
		skip_until(COMMON_ERR_REC_TOKS);
		RETURN(make_shared<empty_stmt_node>());
	}
}

shared_ptr<stmt_node> parser::break_stmt() {
	ENTER;
	shared_ptr<stmt_node> ret = nullptr;
	try {
		if (!is(token_type::kw_break))
			error_unexpected(L"expecting 'break' at the beginning of a break statement");
		consume_token();
		unsigned cnt = 1;
		if (is(token_type::integer)) {
			try {
				cnt = stoi(tok.text);
			} catch (out_of_range&) {
				cnt = 1;
				errors.emplace_back(tok.begin, L"how can you break so many loops?");
			}
			consume_token();
		}
		ret = make_shared<break_stmt_node>(cnt);
		if (!is(token_type::semicolon))
			error_unexpected(L"expecting ';' after a break statement");
		consume_token();
		RETURN(ret);
	} catch (parse_error &e) {
		errors.emplace_back(e);
		skip_until(COMMON_ERR_REC_TOKS);
		if (ret == nullptr)
			RETURN(make_shared<empty_stmt_node>());
		RETURN(ret);
	}
}

shared_ptr<stmt_node> parser::return_stmt() {
	ENTER;
	shared_ptr<stmt_node> ret = nullptr;
	try {
		if (!is(token_type::kw_return))
			error_unexpected(L"expecting 'return' at the beginning of a return statement");
		consume_token();
		ret = make_shared<return_stmt_node>(expr());
		if (!is(token_type::semicolon))
			error_unexpected(L"expecting ';' after a return statement");
		consume_token();
		RETURN(ret);
	} catch (parse_error &e) {
		errors.emplace_back(e);
		skip_until(COMMON_ERR_REC_TOKS);
		if (ret == nullptr)
			RETURN(make_shared<empty_stmt_node>());
		RETURN(ret);
	}
}

shared_ptr<stmt_node> parser::block() {
	ENTER;
	try {
		if (!is(token_type::left_brace))
			error_unexpected(L"expecting '{' at the beginning of a code block");
		consume_token();
		auto ret = make_shared<block_node>();
		while (!is(token_type::right_brace))
			ret->stmts.emplace_back(stmt());
		consume_token();
		RETURN(ret);
	} catch (parse_error &e) {
		errors.emplace_back(e);
		skip_until(token_type::right_brace);
		consume_token();
		RETURN(make_shared<empty_stmt_node>());
	}
}

shared_ptr<var_init_node> parser::var_init() {
	ENTER;
	auto id = identifier();
	std::shared_ptr<expr_node> init = nullptr;
	if (is(token_type::assign)) {
		consume_token();
		init = expr();
	}
	RETURN(make_shared<var_init_node>(id, init));
}

shared_ptr<var_decl_node> parser::var_decl() {
	ENTER;
	try {
		if (!is(token_type::kw_var))
			error_unexpected(L"expecting 'var' at the beginning of variable declaration");
		consume_token();
		auto ret = make_shared<var_decl_node>();
		do {
			if (is(token_type::comma))
				consume_token();
			ret->vars.emplace_back(var_init());
		} while (is(token_type::comma));
		if (!is(token_type::semicolon))
			error_unexpected(L"expecting ';' at the end of variable declaration");
		consume_token();
		RETURN(ret);
	} catch (parse_error &e) {
		errors.emplace_back(e);
		skip_until(COMMON_ERR_REC_TOKS);
		RETURN(make_shared<var_decl_node>());
	}
}

shared_ptr<stmt_node> parser::stmt() {
	if (is(token_type::semicolon)) {
		ENTER;
		consume_token();
		RETURN(make_shared<empty_stmt_node>());
	}
	if (is(token_type::left_brace))
		return block();
	if (is(token_type::kw_var))
		return var_decl();
	if (is(token_type::kw_if))
		return if_stmt();
	if (is(token_type::kw_while))
		return while_stmt();
	if (is(token_type::kw_break))
		return break_stmt();
	if (is(token_type::kw_return))
		return return_stmt();
	return expr_stmt();
}

/*
 * Module
 */
shared_ptr<module_node> parser::mod() {
	ENTER;
	auto ret = make_shared<module_node>();
	while (!eof()) {
		try {
			while (is(token_type::semicolon))
				consume_token();
			ret->decls.emplace_back(var_decl());
		} catch (parse_error &e) {
			errors.emplace_back(e);
			skip_until(token_type::semicolon, token_type::kw_var);
		}
	}
	RETURN(ret);
}