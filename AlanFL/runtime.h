#pragma once
#include <string>
#include <mpirxx.h>
#include <memory>
#include <utility>
#include <vector>
#include "unordered_map"

namespace alanfl {
	enum class object_type {
		nothing,
		integer,
		decimal,
		boolean,
		function
	};

	class vm;
	struct fn_node;

	/*
	 * The object in AlanFL, it is designed to be immutable. 
	 */
	struct object {
		using ptr = std::shared_ptr<object>;
		using ptr_ref = std::shared_ptr<object>&;

		struct fn_object {
			std::unordered_map<std::wstring, ptr> captured; // Captured variables
			std::shared_ptr<fn_node> func; // Corresponding AST node
			fn_object(std::shared_ptr<fn_node> func) : func(std::move(func)) {}
		};

		const object_type type;
		union {
			mpz_class i_val;
			mpf_class d_val;
			bool b_val;
			fn_object f_val;
		};

		object() : type(object_type::nothing) {}
		explicit object(mpz_class i) : type(object_type::integer), i_val(std::move(i)) {}
		explicit object(mpf_class d) : type(object_type::decimal), d_val(std::move(d)) {}
		explicit object(const bool b) : type(object_type::boolean), b_val(b) {}
		explicit object(fn_object f) : type(object_type::function), f_val(std::move(f)) {}

		~object();

		friend std::wostream &operator<<(std::wostream &out, const object &obj);
	};

	// A variable scope
	struct scope {
		const vm &ctx;

		std::unordered_map<std::wstring, object::ptr> vars;
		bool exists(const std::wstring &name) const;
		object::ptr_ref get(const std::wstring &name);
		void set(const std::wstring &name, const object::ptr &val);

		scope(const vm &ctx) : ctx(ctx) {}
	};

	// An execution frame
	struct frame {
		const vm &ctx;

		std::vector<scope> scopes;
		bool exists(const std::wstring &name) const;
		object::ptr_ref get(const std::wstring &name);
		void set(const std::wstring &name, const object::ptr &val);
		scope &top() const;

		void push();
		void pop();

		frame(const vm &ctx) : ctx(ctx) {}
	};

	// Though with the same name with std::runtime_error, this only be thrown when the VM runs into an error
	struct runtime_error : std::exception {
		std::wstring message;
		runtime_error(std::wstring message) : message(std::move(message)) {}
	};
}