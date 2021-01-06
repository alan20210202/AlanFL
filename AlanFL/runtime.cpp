#include "runtime.h"
#include "util.h"

#include <mpirxx.h>

using namespace alanfl;
using namespace std;

object::~object() {
	switch (type) {
	case object_type::integer:
		i_val.~mpz_class();
		break;
	case object_type::decimal:
		d_val.~mpf_class();
		break;
	case object_type::boolean:
	default:
		break; // Do nothing
	}
}

bool scope::exists(const wstring &name) const {
	return vars.find(name) != vars.end();
}

object::ptr_ref scope::get(const wstring &name) {
	auto res = vars.find(name);
	if (res == vars.end())
		throw runtime_error(L"variable \"" + name + L"\" not found");
	return res->second;
}

void scope::set(const wstring &name, const object::ptr &val) {
	vars[name] = val;
}

bool frame::exists(const std::wstring &name) const {
	for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
		if (it->exists(name))
			return true;
	return false;
}

object::ptr_ref frame::get(const std::wstring &name) {
	for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
		auto res = it->vars.find(name);
		if (res != it->vars.end())
			return res->second;
	}
	throw runtime_error(L"variable \"" + name + L"\" not found");
}

void frame::set(const std::wstring &name, const object::ptr &val) {
	for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
		if (it->exists(name)) {
			it->set(name, val);
			return;
		}
	scopes.back().set(name, val);
}

void frame::push() {
	scopes.emplace_back(ctx);
}

void frame::pop() {
	scopes.pop_back();
}

scope &frame::top() const {
	return const_cast<scope&>(scopes.back());
}

wostream & alanfl::operator<<(wostream & out, const object &obj) {
	mp_exp_t expo = 0;
	switch (obj.type) {
	case object_type::integer: 
		return out << to_wstr(obj.i_val.get_str());
	case object_type::decimal:
		return out << to_wstr(obj.d_val.get_str(expo).insert(expo, "."));
	case object_type::boolean:
		return out << (obj.b_val ? "true" : "false");
	default:
		break;
	}
	unreachable("printing object to wostream");
	return out;
}
