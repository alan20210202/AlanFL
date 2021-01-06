#pragma once

#include <iostream>
#include <map>
#include <string>
#include <nana/gui/widgets/form.hpp>
#include <nana/gui/widgets/treebox.hpp>
#include "ast.h"
#include "util.h"

namespace alanfl {
	/*
	 * This is a utility tool that helps present the AST node on GUI
	 */
	class ast_visualizer : public nana::form, public ast_visitor<void, std::wstring> {
		std::map<std::wstring, std::wstring> props; // The 'properties' of nodes
		nana::treebox tree; // The treeview

		void visit_binop_node(const std::shared_ptr<binop_node> &node, std::wstring prefix) override;
		void visit_unop_node(const std::shared_ptr<unop_node> &node, std::wstring prefix) override;
		void visit_bool_node(const std::shared_ptr<bool_node> &node, std::wstring prefix) override;
		void visit_integer_node(const std::shared_ptr<integer_node> &node, std::wstring prefix) override;
		void visit_decimal_node(const std::shared_ptr<decimal_node> &node, std::wstring prefix) override;
		void visit_fn_call_node(const std::shared_ptr<fn_call_node> &node, std::wstring prefix) override;
		void visit_fn_node(const std::shared_ptr<fn_node> &node, std::wstring prefix) override;
		void visit_expr_stmt_node(const std::shared_ptr<expr_stmt_node> &node, std::wstring prefix) override;
		void visit_identifier_node(const std::shared_ptr<identifier_node> &node, std::wstring prefix) override;
		void visit_if_stmt_node(const std::shared_ptr<if_stmt_node> &node, std::wstring prefix) override;
		void visit_while_stmt_node(const std::shared_ptr<while_stmt_node> &node, std::wstring prefix) override;
		void visit_break_stmt_node(const std::shared_ptr<break_stmt_node> &node, std::wstring prefix) override;
		void visit_return_stmt_node(const std::shared_ptr<return_stmt_node> &node, std::wstring prefix) override;
		void visit_block_node(const std::shared_ptr<block_node> &node, std::wstring prefix) override;
		void visit_empty_stmt_node(const std::shared_ptr<empty_stmt_node> &node, std::wstring prefix) override;
		void visit_var_decl_node(const std::shared_ptr<var_decl_node> &node, std::wstring prefix) override;
		void visit_var_init_node(const std::shared_ptr<var_init_node> &node, std::wstring prefix) override;
	public:
		explicit ast_visualizer(const std::shared_ptr<ast_node> n)
			: form(nana::API::make_center(700, 700)),
			tree(*this, { 0, 0, 700, 700 }) {
			caption("Visualizing Node...");
			visit(n, L"");
			for (auto& kv : props)
				tree.insert(to_str(kv.first), to_str(kv.second));
		}
	};
}
