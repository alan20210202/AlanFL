#pragma once

/*
 * The lexer for AlanFL
 */

#include <string>
#include <iostream>
#include <utility>

namespace alanfl {
	/*
	 * Enum defining token types
	 */
	enum class token_type {
		unknown,
		eof,
		integer,
		identifier,
		add, sub,
		mul, div,
		inc, dec,
		assign,
		left_bracket,
		right_bracket,
		left_parenthesis,
		right_parenthesis,
		left_brace,
		right_brace,
		decimal,
		semicolon,
		comma,
		eq, neq,
		lt, gt,
		lteq, gteq,
		lnot,
		land, lor, // Add l to avoid ISO646
		kw_var,
		kw_true,
		kw_false,
		kw_if,
		kw_else,
		kw_while,
		kw_break,
		kw_fn,
		kw_return
	};

	/*
	 * Convert token type to string
	 */
	std::wstring token_str(token_type tok);

	/*
	 * Records the location in a file
	 */
	struct source_location {
		unsigned line;
		unsigned col;
		// TODO: Add a field to distinguish source_location from different files

		source_location() : line(-1), col(-1) {}
		source_location(const unsigned line, const unsigned col)
			: line(line),
			  col(col) {}

		friend std::wostream& operator<<(std::wostream& out, const source_location& fp);
	};

	/*
	 * The token
	 */
	struct token {
		std::wstring text;
		source_location begin, end;
		token_type type;

		token(std::wstring text, const source_location& start, const source_location& end, const token_type type)
			: text(std::move(text)),
			  begin(start),
			  end(end),
			  type(type) {}

		friend std::wostream& operator<<(std::wostream& out, const token &t);
	};

	/*
	 * Hand-written lexer
	 */
	class lexer {
		void consume_char(bool append_to_text = true);
		void skip_whitespaces();
		void start_token();
		token end_token(token_type type);
		wchar_t ch;
		source_location loc; // Current location of lexer
		source_location begin_loc; // Begin location of current token
		std::wstring text;
	public:
		std::wistream &input; // Hope this is fast enough, remember to manually toggle sync_with_stdio
        bool eof() const { return input.eof(); }
		
		token next_token();
		explicit lexer(std::wistream &input) : ch(0), loc(0, 0), begin_loc(loc), text(L""), input(input) {
			consume_char();
		}
	};
}
