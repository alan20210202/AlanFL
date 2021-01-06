#pragma once

#include <mpirxx.h>
#include <utility>
#include <memory>
#include <string>
#include <functional>
#include <vector>
#include "lexer.h"
#include "operators.h"
#include "util.h"
#include "runtime.h"

namespace alanfl {
	/*
	 * RTTI is always a problem since we want to use visitors, though other parser may not care,
	 * the VM here directly runs the code on ASTs!
	 * Therefore, we want to minimize the overhead as much as we can.
	 * It is tested that typeid is 3x faster than dynamic_pointer_cast,
	 * and v table is 10x faster than typeid,
	 * and member variable is 2x faster than v table...
	 * 
	 * The key is to assign each node a UNIQUE 'type id' manually,
	 * Here we use __LINE__ / 4, because it is simple and clear,
	 * but it also implied that we should AVOID writing node definitions in OTHER FILES.
	 */
#define IMPL_TYPEID static const int TYPE_ID = __LINE__ / 4; 
#define INIT_TYPEID type_id = TYPE_ID;

	struct ast_node {
		virtual ~ast_node() = default;
		int type_id; // Manually implemented typeid
		source_location begin, end; // The corresponding covered code span of the node
	};

	/*
	 * Base for all expression nodes
	 */
	struct expr_node : ast_node {
		IMPL_TYPEID
		expr_node() { INIT_TYPEID }
	};

	/*
	 * Literals: bool, integer, decimal(float), identifier
	 * TODO: string, char
	 */
	struct bool_node : expr_node {
		IMPL_TYPEID
		bool value;
		explicit bool_node(const bool value) : value(value) { INIT_TYPEID }
	};

	struct integer_node : expr_node {
		IMPL_TYPEID
		mpz_class value;
		object::ptr value_obj;
		explicit integer_node(mpz_class value) : value(std::move(value)), value_obj(std::make_shared<object>(this->value)) { INIT_TYPEID }
	};

	struct decimal_node : expr_node {
		IMPL_TYPEID
		mpf_class value;
		object::ptr value_obj;
		explicit decimal_node(mpf_class value) : value(std::move(value)), value_obj(std::make_shared<object>(this->value)) { INIT_TYPEID }
	};

	struct identifier_node : expr_node {
		IMPL_TYPEID
		std::wstring id;
		explicit identifier_node(std::wstring id) : id(std::move(id)) { INIT_TYPEID }
	};

	/*
	 * Arithmetic operations, see operators in "operators.h"
	 */
	struct binop_node : expr_node {
		IMPL_TYPEID
		std::shared_ptr<expr_node> lhs, rhs;
		binary_op op;
		binop_node(std::shared_ptr<expr_node> lhs, std::shared_ptr<expr_node> rhs, const binary_op op)
			: lhs(std::move(lhs)), rhs(std::move(rhs)), op(op) {
			INIT_TYPEID
		}
	};

	struct unop_node : expr_node {
		IMPL_TYPEID
		std::shared_ptr<expr_node> operand;
		unary_op op;
		unop_node(std::shared_ptr<expr_node> operand, const unary_op op)
			: operand(std::move(operand)), op(op) {
			INIT_TYPEID
		}
	};

	/*
	 * Function call
	 */
	struct fn_call_node : expr_node {
		IMPL_TYPEID
		std::shared_ptr<expr_node> callee;
		std::vector<std::shared_ptr<expr_node>> args;
		fn_call_node(std::shared_ptr<expr_node> callee) : callee(std::move(callee)) { INIT_TYPEID }
	};

	/*
	 * Base for all statement nodes
	 */
	struct stmt_node : ast_node {
		IMPL_TYPEID
		stmt_node() { INIT_TYPEID }
	};


	struct expr_stmt_node : stmt_node {
		IMPL_TYPEID
		std::shared_ptr<expr_node> expr;
		expr_stmt_node(std::shared_ptr<expr_node> expr)
			: expr(std::move(expr)) {
			INIT_TYPEID
		}
	};

	struct break_stmt_node : stmt_node {
		IMPL_TYPEID
		unsigned cnt;
		break_stmt_node(const unsigned cnt) : cnt(cnt) { INIT_TYPEID }
	};

	struct return_stmt_node : stmt_node {
		IMPL_TYPEID;
		std::shared_ptr<expr_node> val;
		return_stmt_node(std::shared_ptr<expr_node> val) : val(std::move(val)) { INIT_TYPEID }
	};

	struct empty_stmt_node : stmt_node {
		IMPL_TYPEID
		empty_stmt_node() { INIT_TYPEID }
	};

	struct if_stmt_node : stmt_node {
		IMPL_TYPEID
		std::shared_ptr<expr_node> cond;
		std::shared_ptr<stmt_node> branch, else_branch;
		if_stmt_node(std::shared_ptr<expr_node> cond, 
				std::shared_ptr<stmt_node> branch,
				std::shared_ptr<stmt_node> else_branch) 
		: cond(std::move(cond)), branch(std::move(branch)), else_branch(std::move(else_branch))
		{ INIT_TYPEID }
	};

	struct while_stmt_node : stmt_node {
		IMPL_TYPEID
		std::shared_ptr<expr_node> cond;
		std::shared_ptr<stmt_node> body;
		while_stmt_node(std::shared_ptr<expr_node> cond, 
			std::shared_ptr<stmt_node> body)
			: cond(std::move(cond)), body(std::move(body))
		{ INIT_TYPEID }
	};

	struct block_node : stmt_node {
		IMPL_TYPEID
		std::vector<std::shared_ptr<stmt_node>> stmts;
		block_node() { INIT_TYPEID }
	};

	struct intrinsic_node : stmt_node {
		IMPL_TYPEID
		std::function<void(vm &ctx)> body;
		intrinsic_node(std::function<void(vm &ctx)> body) : body(std::move(body)) { INIT_TYPEID }
	};

	struct var_init_node : ast_node {
		IMPL_TYPEID
		std::shared_ptr<identifier_node> id;
		std::shared_ptr<expr_node> init;

		var_init_node(std::shared_ptr<identifier_node> id, std::shared_ptr<expr_node> init)
			: id(std::move(id)), init(std::move(init)) {
			INIT_TYPEID
		}
	};

	struct fn_node : expr_node {
		IMPL_TYPEID
		std::vector<std::shared_ptr<var_init_node>> params;
		std::vector<std::shared_ptr<var_init_node>> captures;
		std::shared_ptr<stmt_node> body;
		fn_node() { INIT_TYPEID }
	};

	struct var_decl_node : stmt_node {
		IMPL_TYPEID
		std::vector<std::shared_ptr<var_init_node>> vars;
		var_decl_node() { INIT_TYPEID }
	};

	struct module_node : ast_node {
		IMPL_TYPEID
		std::vector<std::shared_ptr<var_decl_node>> decls;
		module_node() { INIT_TYPEID }
	};

	/*
	 * The visitor for AST, but it is not implemented in the traditional visit / accept style. 
	 * The traditional visit-accept style needs an 'accept' method defined in AST nodes, which helps
	 * to dispatch visit calls to certain function with the help of v-table, but as long as we use it 
	 * we are not able to customize extra parameters and return values...
	 * So here we dispatch different functions manually
	 */
#ifdef _MSC_VER
	#pragma warning(disable : 4716) // Disable MSVC warning that function has no return
#endif
	template <typename Ret = void, typename ...Args>
	class ast_visitor {
		/*
		 * When the visitor visits a node that isn't expected to be visited,
		 * ex: statement visitor visits expression node, this will be called.
		 */
		virtual void unexpected_visit() { unimplemented("visitor function not implemented"); }

		// Define a visitor function for a certain type of node
#define VISITOR_FUNCTION(subtype) \
	virtual Ret visit_##subtype(const std::shared_ptr<subtype> &node, Args... args) { unexpected_visit(); }
		// Visitor function with fallback
#define VISITOR_FUNCTION_FALLBACK(subtype, super) \
	virtual Ret visit_##subtype(const std::shared_ptr<subtype> &node, Args... args) { return visit_##super(node, std::forward<Args>(args)...); }
		VISITOR_FUNCTION(expr_node)
		VISITOR_FUNCTION_FALLBACK(bool_node, expr_node)
		VISITOR_FUNCTION_FALLBACK(integer_node, expr_node)
		VISITOR_FUNCTION_FALLBACK(decimal_node, expr_node)
		VISITOR_FUNCTION_FALLBACK(identifier_node, expr_node)
		VISITOR_FUNCTION_FALLBACK(binop_node, expr_node)
		VISITOR_FUNCTION_FALLBACK(unop_node, expr_node)
		VISITOR_FUNCTION_FALLBACK(fn_call_node, expr_node)
		VISITOR_FUNCTION_FALLBACK(fn_node, expr_node)

		VISITOR_FUNCTION(stmt_node)
		VISITOR_FUNCTION_FALLBACK(empty_stmt_node, stmt_node)
		VISITOR_FUNCTION_FALLBACK(expr_stmt_node, stmt_node)
		VISITOR_FUNCTION_FALLBACK(if_stmt_node, stmt_node)
		VISITOR_FUNCTION_FALLBACK(while_stmt_node, stmt_node)
		VISITOR_FUNCTION_FALLBACK(break_stmt_node, stmt_node)
		VISITOR_FUNCTION_FALLBACK(return_stmt_node, stmt_node)
		VISITOR_FUNCTION_FALLBACK(block_node, stmt_node)
		VISITOR_FUNCTION_FALLBACK(intrinsic_node, stmt_node)
		VISITOR_FUNCTION_FALLBACK(var_decl_node, stmt_node)

		VISITOR_FUNCTION(var_init_node)
		VISITOR_FUNCTION(module_node)
#undef VISITOR_FUNCTION
#undef VISITOR_FUNCTION_FALLBACK
	public:
		virtual ~ast_visitor() = default;

		// Manual dispatch
		Ret visit(const std::shared_ptr<ast_node> &node, Args... args) {
#define DISPATCH(subtype) case subtype::TYPE_ID: \
	return visit_##subtype(std::static_pointer_cast<subtype>(node), std::forward<Args>(args)...)
			switch (node->type_id) {
				DISPATCH(bool_node);
				DISPATCH(integer_node);
				DISPATCH(decimal_node);
				DISPATCH(identifier_node);
				DISPATCH(binop_node);
				DISPATCH(unop_node);
				DISPATCH(fn_call_node);
				DISPATCH(fn_node);

				DISPATCH(empty_stmt_node);
				DISPATCH(expr_stmt_node);
				DISPATCH(if_stmt_node);
				DISPATCH(while_stmt_node);
				DISPATCH(break_stmt_node);
				DISPATCH(return_stmt_node);
				DISPATCH(block_node);
				DISPATCH(intrinsic_node);
				DISPATCH(var_decl_node);
				DISPATCH(var_init_node);

				DISPATCH(module_node);
#ifdef _MSC_VER
			default: __assume(0);
#else
			default: unreachable("unknown node type");
#endif
			}
		}
	};
}
