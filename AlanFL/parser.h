#pragma once

#include <memory>
#include <utility>
#include <vector>
#include "lexer.h"
#include "ast.h"

namespace alanfl {
	/*
	 * This error will be first thrown by some parsing functions when things are not going right,
	 * then abort normal parsing procedure until a high-level function (with enough knowledge about the context).
	 * Then it will be put on the parser error list. And the parser tries to recover after then.
	 */
	struct parse_error {
		source_location pos; // TODO: make position a 'range', not a point, otherwise it's hard to 'underline' the faulty code on screen.
		std::wstring message;
		parse_error(source_location pos, std::wstring message)
			: pos(pos), message(std::move(message)) {}
	};

	/*
	 * The parser, written in recursive-descent algorithm, pretty straight forward
	 */
	class parser {
		std::shared_ptr<lexer> lex; // The lexer where we receive tokens
		token tok; // Current lookahead token
		source_location prev_end; // End of previous token, useful when relating nodes to its covered code in file
		std::vector<parse_error> errors; // Recoverable errors during parsing
		void consume_token();

		/*
		 * Check whether the lookahead token is any of the arguments
		 */
		static bool is() { return false; }
		template <typename Type, typename ...Other>
		bool is(Type t, Other... o) const { return tok.type == t || is(o...); }

		/*
		 * Skip until we encounter any of the arguments
		 */
		template <typename ...Type>
		void skip_until(Type... t) { while (!eof() && !is(t...)) consume_token(); }

		/*
		 * Wrappers for throw parser_error
		 */
		void error(std::wstring msg) const;
		void error_unexpected(std::wstring msg) const;
	public:
		explicit parser(std::shared_ptr<lexer> lex)
			: lex(std::move(lex)), tok(this->lex->next_token()) {}

		bool eof() const { return tok.type == token_type::eof; }
		bool has_error() const { return !errors.empty(); }
		void dump_error() const;

		/*
		 * Parsing functions, each corresponds to a non-terminal
		 */
		std::shared_ptr<identifier_node> identifier();
		std::shared_ptr<integer_node> integer();
		std::shared_ptr<decimal_node> decimal();

		std::shared_ptr<fn_node> fn();
		std::shared_ptr<expr_node> primary();
		std::shared_ptr<expr_node> expr_fn_call();
		std::shared_ptr<expr_node> expr_unop();
		std::shared_ptr<expr_node> expr_mul_div();
		std::shared_ptr<expr_node> expr_add_sub();
		std::shared_ptr<expr_node> expr_assign();
		std::shared_ptr<expr_node> expr_cmp();
		std::shared_ptr<expr_node> expr_eq_cmp();
		std::shared_ptr<expr_node> expr_and();
		std::shared_ptr<expr_node> expr_or();
		std::shared_ptr<expr_node> expr();

		std::shared_ptr<stmt_node> expr_stmt();
		std::shared_ptr<stmt_node> if_stmt();
		std::shared_ptr<stmt_node> while_stmt();
		std::shared_ptr<stmt_node> break_stmt();
		std::shared_ptr<stmt_node> return_stmt();
		std::shared_ptr<stmt_node> block();
		std::shared_ptr<var_init_node> var_init();
		std::shared_ptr<var_decl_node> var_decl();
		std::shared_ptr<stmt_node> stmt();

		std::shared_ptr<module_node> mod();
	};
}
