#include <exception>
#include "operators.h"
#include "util.h"

using namespace alanfl;
using namespace std;

wstring alanfl::binop_str(const binary_op op) {
	switch (op) {
	case binary_op::add: return L"+";
	case binary_op::sub: return L"-";
	case binary_op::mul: return L"*";
	case binary_op::div: return L"/";
	case binary_op::assign: return L"=";
	case binary_op::land: return L"&&";
	case binary_op::lor: return L"||";
	case binary_op::lt: return L"<";
	case binary_op::lteq: return L"<=";
	case binary_op::gt: return L">";
	case binary_op::gteq: return L">=";
	case binary_op::eq: return L"==";
	case binary_op::neq: return L"!=";
	}
	unreachable("converting binop to string");
	return wstring();
}

wstring alanfl::unop_str(unary_op op) {
	switch (op) {
	case unary_op::neg: return L"negate";
	case unary_op::lnot: return L"not";
	}
}

binary_op alanfl::binop_from(token_type type) {
	switch (type) {
	case token_type::add: return binary_op::add;
	case token_type::sub: return binary_op::sub;
	case token_type::mul: return binary_op::mul;
	case token_type::div: return binary_op::div;
	case token_type::assign: return binary_op::assign;
	case token_type::land: return binary_op::land;
	case token_type::lor: return binary_op::lor;
	case token_type::lt: return binary_op::lt;
	case token_type::lteq: return binary_op::lteq;
	case token_type::gt: return binary_op::gt;
	case token_type::gteq: return binary_op::gteq;
	case token_type::eq: return binary_op::eq;
	case token_type::neq: return binary_op::neq;
	default: unreachable("converting token to binop");
	}
}

unary_op alanfl::unop_from(token_type type) {
	switch (type) {
	case token_type::sub: return unary_op::neg;
	case token_type::lnot: return unary_op::lnot;
	default: unreachable("converting token to unop");
	}
}