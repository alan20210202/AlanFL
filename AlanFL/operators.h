#pragma once

#include <string>
#include "lexer.h"

namespace alanfl {
	enum class binary_op {
		add, sub, mul, div, assign,
		land, lor,
		lt, lteq, gt, gteq, eq, neq
	};

	enum class unary_op {
		neg, lnot
	};

	std::wstring binop_str(binary_op op);

	std::wstring unop_str(unary_op op);

	binary_op binop_from(token_type type);

	unary_op unop_from(token_type type);
}