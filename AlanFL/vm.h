#pragma once
#include <stack>
#include "runtime.h"
#include "ast.h"

namespace alanfl {
	/*
	 * The node-based VM for AlanFL
	 * This is often passes as reference as a context of the language
	 */
	class vm : public ast_visitor<> {
		static const int MAX_CACHE_INT = 127; // Upper bound for caching int
		static const int MIN_CACHE_INT = -127; // Lower bound for caching int
		static const int CACHE_SIZE = MAX_CACHE_INT - MIN_CACHE_INT + 1;
		object::ptr int_cache[CACHE_SIZE]; // Int cache
		object::ptr bool_true, bool_false; // Bool cache
		object::ptr nothing; // Nothing cache

		scope global; // Global scope
		std::stack<frame> call_stack; // Call stack

		/*
		 * The evaluator for right values
		 */
		class rvalue_evaluator : public ast_visitor<object::ptr> {
			vm &ctx;
			void unexpected_visit() override;

			object::ptr_ref visit_lvalue(const std::shared_ptr<expr_node> &node) const;
			object::ptr visit_bool_node(const std::shared_ptr<bool_node> &node) override;
			object::ptr visit_identifier_node(const std::shared_ptr<identifier_node> &node) override;
			object::ptr visit_integer_node(const std::shared_ptr<integer_node> &node) override;
			object::ptr visit_decimal_node(const std::shared_ptr<decimal_node> &node) override;
			object::ptr visit_fn_node(const std::shared_ptr<fn_node> &node) override;
			object::ptr visit_fn_call_node(const std::shared_ptr<fn_call_node> &node) override;
			object::ptr visit_binop_node(const std::shared_ptr<binop_node> &node) override;
			object::ptr visit_unop_node(const std::shared_ptr<unop_node> &node) override;
		public:
			explicit rvalue_evaluator(vm &ctx) : ctx(ctx) {};
		} rve;

		/*
		 * For left values
		 */
		class lvalue_evaluator : public ast_visitor<object::ptr_ref> {
			vm &ctx;
			void unexpected_visit() override;

			object::ptr visit_rvalue(const std::shared_ptr<expr_node> &node) const;
			object::ptr_ref visit_identifier_node(const std::shared_ptr<identifier_node> &node) override;
		public:
			explicit lvalue_evaluator(vm &ctx) : ctx(ctx) {};
		} lve;

		void init_obj_cache();
		void init_intrinsics();

		object::ptr_ref get(const std::wstring &name);
		object::ptr get_nothing() const;
		object::ptr get_int(mpz_class z) const;
		object::ptr get_decimal(mpf_class f) const;
		object::ptr get_bool(bool b) const;
		object::ptr get_fn(const std::shared_ptr<fn_node> &fn);
		object::ptr get_intrinsic(const std::wstring &sig, std::function<void(vm &ctx)> body);

		void visit_empty_stmt_node(const std::shared_ptr<empty_stmt_node> &node) override;
		void visit_if_stmt_node(const std::shared_ptr<if_stmt_node> &node) override;
		void visit_while_stmt_node(const std::shared_ptr<while_stmt_node> &node) override;
		void visit_break_stmt_node(const std::shared_ptr<break_stmt_node> &node) override;
		void visit_return_stmt_node(const std::shared_ptr<return_stmt_node> &node) override;
		void visit_intrinsic_node(const std::shared_ptr<intrinsic_node> &node) override;
		void visit_expr_stmt_node(const std::shared_ptr<expr_stmt_node> &node) override;
		void visit_var_decl_node(const std::shared_ptr<var_decl_node> &node) override;
		void visit_var_init_node(const std::shared_ptr<var_init_node> &node) override;
		void visit_block_node(const std::shared_ptr<block_node> &node) override;

		void visit_module_node(const std::shared_ptr<module_node> &node) override;
	public:
		frame *current_frame;
		void push_frame();
		void pop_frame();
		void exec(const std::shared_ptr<ast_node> &node);
		vm() : global(*this), rve(*this), lve(*this), current_frame(nullptr) {
			init_obj_cache();
			init_intrinsics();
			push_frame(); // Push a dummy frame
		}
	};
}
