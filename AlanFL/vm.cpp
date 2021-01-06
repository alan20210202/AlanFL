#include <iostream>
#include "parser.h"
#include "vm.h"

using namespace alanfl;
using namespace std;

struct loop_break {
	unsigned cnt;
	explicit loop_break(const unsigned cnt) : cnt(cnt) {}
};

struct function_return {
	object::ptr value;
	explicit function_return(object::ptr value) : value(move(value)) {}
};

void vm::push_frame() {
	call_stack.emplace(*this);
	current_frame = &call_stack.top();
}

void vm::pop_frame() {
	call_stack.pop();
	if (!call_stack.empty())
		current_frame = &call_stack.top();
}

void vm::init_obj_cache() {
	for (auto i = MIN_CACHE_INT; i <= MAX_CACHE_INT; i++)
		int_cache[i - MIN_CACHE_INT] = make_shared<object>(mpz_class(i));
	bool_true = make_shared<object>(true);
	bool_false = make_shared<object>(false);
	nothing = make_shared<object>();
}

void vm::init_intrinsics() {
	push_frame();
	global.set(L"print_line", get_intrinsic(L"fn (val)", [](vm &ctx) {
		const auto val = ctx.get(L"val");
		wcout << *val << endl;
	}));
	global.set(L"read_int", get_intrinsic(L"fn ()", [](vm &ctx) {
		string s; cin >> s;
		throw function_return(ctx.get_int(mpz_class(s)));
	}));
	global.set(L"sqrt", get_intrinsic(L"fn (x)", [](vm &ctx) {
		const auto x = ctx.get(L"x");
		if (x->type != object_type::decimal && x->type != object_type::integer)
			throw runtime_error(L"sqrt accepts only numbers");
		if (x->type == object_type::decimal)
			throw function_return(ctx.get_decimal(sqrt(x->d_val)));
		throw function_return(ctx.get_decimal(sqrt(x->i_val)));
	}));
	pop_frame();
}

void vm::exec(const shared_ptr<ast_node> &node) {
	try {
		visit(node);
	} catch (runtime_error &re) {
		wcout << re.message << endl;
	} catch (logic_error &le) {
		wcout << le.what() << endl;
	}
}

object::ptr_ref vm::get(const wstring &name) {
	try {
		return current_frame->get(name);
	} catch (...) {
		return global.get(name);
	}
}

object::ptr vm::get_nothing() const {
	return nothing;
}

object::ptr vm::get_int(mpz_class z) const {
	if (z.fits_si_p()) {
		const auto si = z.get_si();
		if (-MIN_CACHE_INT <= si && si <= MAX_CACHE_INT)
			return int_cache[si - MIN_CACHE_INT];
	}
	return make_shared<object>(move(z));
}

object::ptr vm::get_decimal(mpf_class f) const {
	return make_shared<object>(move(f));
}

object::ptr vm::get_bool(const bool b) const {
	return b ? bool_true : bool_false;
}

object::ptr vm::get_fn(const shared_ptr<fn_node> &fn) {
	current_frame->push();
	for (auto &vi : fn->captures)
		visit(vi);
	auto &cap = current_frame->top();
	auto ret = make_shared<object>(object::fn_object(fn));
	ret->f_val.captured = move(cap.vars);
	current_frame->pop();
	return ret;
}

object::ptr vm::get_intrinsic(const wstring &sig, function<void(vm &ctx)> body) {
	wstringstream wss; wss << sig << L" {}";
	auto lex = make_shared<lexer>(wss);
	auto par = make_shared<parser>(lex);
	auto fn = par->fn();
	fn->body = make_shared<intrinsic_node>(body);
	return get_fn(fn);
}

void vm::visit_empty_stmt_node(const shared_ptr<empty_stmt_node> &node) {}

void vm::visit_if_stmt_node(const shared_ptr<if_stmt_node> &node) {
	const auto cond = rve.visit(node->cond);
	if (cond->type != object_type::boolean)
		throw runtime_error(L"condition for an if stmt must be boolean!");
	if (cond->b_val)
		visit(node->branch);
	else if (node->else_branch != nullptr)
		visit(node->else_branch);
}

void vm::visit_while_stmt_node(const shared_ptr<while_stmt_node> &node) {
	auto cond = rve.visit(node->cond);
	if (cond->type != object_type::boolean)
		throw runtime_error(L"condition for a while stmt must be boolean!");
	try {
		while (cond->b_val) {
			visit(node->body);
			cond = rve.visit(node->cond);
			if (cond->type != object_type::boolean)
				throw runtime_error(L"condition for a while stmt must be boolean!");
		}
	} catch (loop_break &lb) {
		if (lb.cnt > 1) {
			lb.cnt--;
			throw;
		}
	}
}

void vm::visit_break_stmt_node(const shared_ptr<break_stmt_node> &node) {
	throw loop_break(node->cnt); // Yeah!!!!
}

void vm::visit_return_stmt_node(const shared_ptr<return_stmt_node> &node) {
	const auto val = rve.visit(node->val);
	throw function_return(val);
}

void vm::visit_intrinsic_node(const shared_ptr<intrinsic_node> &node) {
	node->body(*this);
}

void vm::visit_block_node(const shared_ptr<block_node> &node) {
	try {
		current_frame->push();
		for (auto &s : node->stmts)
			visit(s);
		current_frame->pop();
	} catch(...) { // Clean up scope
		current_frame->pop();
		throw;
	}
}

void vm::visit_expr_stmt_node(const shared_ptr<expr_stmt_node> &node) {
#ifdef EXPR_STMT_PRINT_RESULT
	const auto res = rve.visit(node->expr);
	wcout << *res << endl;
#else
	rve.visit(node->expr);
#endif
}

void vm::visit_var_decl_node(const shared_ptr<var_decl_node> &node) {
	for (auto &vi : node->vars)
		visit(vi);
}

void vm::visit_var_init_node(const shared_ptr<var_init_node> &node) {
	const auto init = rve.visit(node->init);
	current_frame->top().set(node->id->id, init);
}

void vm::visit_module_node(const shared_ptr<module_node> &node) {
	for (auto &decl : node->decls) { // Initialize global variables in order
		for (auto &vi : decl->vars) {
			const auto init = rve.visit(vi->init);
			global.set(vi->id->id, init);
		}
	}
	const auto entry = global.get(L"entry");
	if (entry->type != object_type::function)
		throw runtime_error(L"entry should be a function to call");
	push_frame();
	visit(entry->f_val.func->body);
	pop_frame();
}

object::ptr_ref vm::rvalue_evaluator::visit_lvalue(const shared_ptr<expr_node> &node) const {
	return ctx.lve.visit(node);
}

object::ptr vm::rvalue_evaluator::visit_binop_node(const shared_ptr<binop_node> &node) {
	switch (node->op) {
#define ARITH_BINOP(op_enum, op) case op_enum: { \
		const auto lhs = visit(node->lhs), rhs = visit(node->rhs); \
		if (lhs->type == object_type::integer && rhs->type == object_type::integer) \
			return ctx.get_int(lhs->i_val op rhs->i_val); \
		if (lhs->type == object_type::integer && rhs->type == object_type::decimal) \
			return ctx.get_decimal(lhs->i_val op rhs->d_val); \
		if (lhs->type == object_type::decimal && rhs->type == object_type::integer) \
			return ctx.get_decimal(lhs->d_val op rhs->i_val); \
		if (lhs->type == object_type::decimal && rhs->type == object_type::decimal) \
			return ctx.get_decimal(lhs->d_val op rhs->d_val); \
		throw runtime_error(L"cannot perform arithmetic operation on non-numeric type"); \
		}
#define COMPARE_BINOP(op_enum, op) case op_enum: { \
		const auto lhs = visit(node->lhs), rhs = visit(node->rhs); \
		if (lhs->type == object_type::integer && rhs->type == object_type::integer) \
			return ctx.get_bool(lhs->i_val op rhs->i_val); \
		if (lhs->type == object_type::integer && rhs->type == object_type::decimal) \
			return ctx.get_bool(lhs->i_val op rhs->d_val); \
		if (lhs->type == object_type::decimal && rhs->type == object_type::integer) \
			return ctx.get_bool(lhs->d_val op rhs->i_val); \
		if (lhs->type == object_type::decimal && rhs->type == object_type::decimal) \
			return ctx.get_bool(lhs->d_val op rhs->d_val); \
		throw runtime_error(L"cannot perform arithmetic comparison on non-numeric type"); \
		}
#define LOGICAL_BINOP(op_enum, op) case op_enum: { \
		const auto lhs = visit(node->lhs), rhs = visit(node->rhs); \
		if (lhs->type == object_type::boolean && rhs->type == object_type::boolean) \
			return ctx.get_bool(lhs->b_val op rhs->b_val); \
		throw runtime_error(L"cannot perform logical operation on non-boolean type"); \
		}
	ARITH_BINOP(binary_op::add, +)
	ARITH_BINOP(binary_op::sub, -)
	ARITH_BINOP(binary_op::mul, *)
	ARITH_BINOP(binary_op::div, /)

	LOGICAL_BINOP(binary_op::land, &&)
	LOGICAL_BINOP(binary_op::lor, ||)

	COMPARE_BINOP(binary_op::lt, <)
	COMPARE_BINOP(binary_op::lteq, <=)
	COMPARE_BINOP(binary_op::gt, >)
	COMPARE_BINOP(binary_op::gteq, >=)
	COMPARE_BINOP(binary_op::eq, ==)
	COMPARE_BINOP(binary_op::neq, !=)

	case binary_op::assign:
		return visit_lvalue(node->lhs) = visit(node->rhs);
	}
	unreachable("evaluating binary expression");
	return nullptr;
}

object::ptr vm::rvalue_evaluator::visit_fn_node(const shared_ptr<fn_node> &node) {
	return ctx.get_fn(node);
}

object::ptr vm::rvalue_evaluator::visit_fn_call_node(const shared_ptr<fn_call_node> &node) {
	const auto callee = visit(node->callee); // Calculate the callee
	if (callee->type != object_type::function) // If callee is not a function
		throw runtime_error(L"can not \"call\" a non-function object");

	const auto &fn = callee->f_val.func;
	if (node->args.size() > fn->params.size()) // If we have more arguments than callee is expected to receive
		throw runtime_error(L"too many arguments to call function");

	vector<object::ptr> args;
	for (auto &arg : node->args)
		args.emplace_back(visit(arg)); // Evaluate args before new frame pushed

	ctx.push_frame(); // New frame on stack
	ctx.current_frame->push(); // Push captured variables
	for (auto &it : callee->f_val.captured)
		ctx.current_frame->set(it.first, it.second);

	try {
		for (auto i = 0; i < node->args.size(); i++) { // Put arguments
			auto &vi = fn->params[i];
			ctx.current_frame->top().set(vi->id->id, args[i]);
		}
		for (auto i = node->args.size(); i < fn->params.size(); i++) { // Put default arguments, if any
			auto &vi = fn->params[i];
			if (vi->init == nullptr)
				throw runtime_error(L"unprovided call argument \"" + vi->id->id + L"\" must have its default value");
			visit(vi);
		}
		ctx.visit(fn->body); // Execute function body
		ctx.current_frame->pop();
		ctx.pop_frame(); // Clear stack	
	} catch (runtime_error&) {
		ctx.current_frame->pop();
		ctx.pop_frame(); // Clear stack	
		throw;
	} catch (function_return &fr) { // We use errors to return values
		ctx.current_frame->pop();
		ctx.pop_frame(); // Clear stack	
		return fr.value;
	}
	return ctx.get_nothing();
}

object::ptr vm::rvalue_evaluator::visit_unop_node(const shared_ptr<unop_node> &node) {
	switch (node->op) {
	case unary_op::neg: {
		const auto val = visit(node->operand);
		if (val->type == object_type::integer)
			return ctx.get_int(-val->i_val);
		if (val->type == object_type::decimal)
			return ctx.get_decimal(-val->d_val);
		throw runtime_error(L"cannot perform numeric negation on non-numeric type");
	}
	case unary_op::lnot: {
		const auto val = visit(node->operand);
		if (val->type == object_type::boolean)
			return ctx.get_bool(!val->b_val);
		throw runtime_error(L"cannot perform logical negation on non-boolean type");
	}
	}
	unreachable("evaluating unary expression");
	return nullptr;
}

object::ptr vm::rvalue_evaluator::visit_bool_node(const shared_ptr<bool_node> &node) {
	return ctx.get_bool(node->value);
}

object::ptr vm::rvalue_evaluator::visit_decimal_node(const shared_ptr<decimal_node> &node) {
	return node->value_obj;
}

object::ptr vm::rvalue_evaluator::visit_integer_node(const shared_ptr<integer_node> &node) {
	return node->value_obj;
}

object::ptr vm::rvalue_evaluator::visit_identifier_node(const shared_ptr<identifier_node> &node) {
	const auto val = ctx.get(node->id);
	return val;
}

void vm::rvalue_evaluator::unexpected_visit() {
	unreachable("evaluating rvalue");
}

object::ptr vm::lvalue_evaluator::visit_rvalue(const shared_ptr<expr_node> &node) const {
	return ctx.rve.visit(node);
}

object::ptr_ref vm::lvalue_evaluator::visit_identifier_node(const shared_ptr<identifier_node> &node) {
	return ctx.get(node->id);
}

void vm::lvalue_evaluator::unexpected_visit() {
	throw runtime_error(L"expression cannot be used as lvalue!");
}