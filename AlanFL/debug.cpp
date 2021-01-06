#include <nana/gui.hpp>
#include <nana/gui/widgets/form.hpp>
#include <nana/gui/widgets/treebox.hpp>

#include "debug.h"
#include "operators.h"

using namespace std;
using namespace alanfl;
using namespace nana;

/*
 * 'VALUE' is used when we want to show a 'value', or something that can be directly printed out
 * 'TREE' is used when we want to recursively show a subtree of the node
 */
#define VALUE(key, value) props[prefix + L"/" + key] = wstring(key) + L": " + value
#define TREE(key, value) if (value != nullptr) \
							props[prefix + L"/" + key] = wstring(key) + L": " + to_wstr(typeid(*value).name()),\
							visit(value, prefix + L"/" + key); \
						else \
							VALUE(key, L"nullptr")

void ast_visualizer::visit_binop_node(const shared_ptr<binop_node> &node, const wstring prefix) {
	TREE(L"lhs", node->lhs);
	TREE(L"rhs", node->rhs);
	VALUE(L"op", binop_str(node->op));
}

void ast_visualizer::visit_unop_node(const shared_ptr<unop_node> &node, const wstring prefix) {
	TREE(L"operand", node->operand);
	VALUE(L"op", unop_str(node->op));
}

void ast_visualizer::visit_bool_node(const std::shared_ptr<bool_node> &node, const wstring prefix) {
	VALUE(L"value", to_wstr(node->value));
}

void ast_visualizer::visit_integer_node(const shared_ptr<integer_node> &node, const wstring prefix) {
	VALUE(L"value", to_wstr(node->value.get_str()));
}

void ast_visualizer::visit_decimal_node(const shared_ptr<decimal_node> &node, const wstring prefix) {
	mp_exp_t expo;
	VALUE(L"value", to_wstr(node->value.get_str(expo).insert(expo, ".")));
}

void ast_visualizer::visit_fn_call_node(const std::shared_ptr<fn_call_node> &node, const std::wstring prefix) {
	TREE(L"callee", node->callee);
	for (auto i = 0; i < node->args.size(); i++)
		TREE(L"arg " + to_wstr(i), node->args[i]);
}

void ast_visualizer::visit_fn_node(const std::shared_ptr<fn_node> &node, const std::wstring prefix) {
	TREE(L"body", node->body);
	for (auto i = 0; i < node->params.size(); i++)
		TREE(L"param " + to_wstr(i), node->params[i]);
}

void ast_visualizer::visit_expr_stmt_node(const shared_ptr<expr_stmt_node> &node, const wstring prefix) {
	TREE(L"expr", node->expr);
}

void ast_visualizer::visit_if_stmt_node(const shared_ptr<if_stmt_node> &node, const wstring prefix) {
	TREE(L"cond", node->cond);
	TREE(L"branch", node->branch);
	TREE(L"else", node->else_branch);
}

void ast_visualizer::visit_while_stmt_node(const std::shared_ptr<while_stmt_node> &node, const wstring prefix) {
	TREE(L"cond", node->cond);
	TREE(L"body", node->body);
}

void ast_visualizer::visit_break_stmt_node(const std::shared_ptr<break_stmt_node> &node, const wstring prefix) {
	VALUE(L"count", to_wstr(node->cnt));
}

void ast_visualizer::visit_return_stmt_node(const std::shared_ptr<return_stmt_node> &node, const wstring prefix) {
	TREE(L"value", node->val);
}

void ast_visualizer::visit_block_node(const shared_ptr<block_node> &node, const wstring prefix) {
	for (auto i = 0; i < node->stmts.size(); i++)
		TREE(L"stmt " + to_wstr(i), node->stmts[i]);
}

void ast_visualizer::visit_empty_stmt_node(const shared_ptr<empty_stmt_node> &node, const wstring prefix) {}

void ast_visualizer::visit_identifier_node(const shared_ptr<identifier_node> &node, const wstring prefix) {
	VALUE(L"id", node->id);
}

void ast_visualizer::visit_var_decl_node(const shared_ptr<var_decl_node> &node, const wstring prefix) {
	for (auto i = 0; i < node->vars.size(); i++)
		TREE(L"var " + to_wstr(i), node->vars[i]);
}

void ast_visualizer::visit_var_init_node(const shared_ptr<var_init_node> &node, const wstring prefix) {
	TREE(L"id", node->id);
	TREE(L"init", node->init);
}

