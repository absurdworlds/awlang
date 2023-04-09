#ifndef aw_script_ir_type_h
#define aw_script_ir_type_h

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// TODO: think about it
namespace aw::script::middle {
struct struct_decl;
struct initializer;
}

namespace aw::script::ir {

struct type;

enum class simple_type {
	nothing,
	string_literal,
	numeric_literal,
	float_literal,
	boolean,
	byte,
	character,
};

struct integer_type {
	int size = 0;
	bool is_signed = false;
};

struct fp_type {
	int size = 0;
};

struct pointer_type {
	type* base_type = nullptr;
	bool is_mutable = false;
};

struct reference_type {
	type* base_type = nullptr;
	bool is_mutable = false;
};

struct alias_type {
	type* base_type = nullptr;
	bool strict = false;
};

struct variant_type {
	std::vector<type*> alternatives;
};

struct tuple_type {
	std::vector<type*> members;
};

struct struct_type {
	middle::struct_decl* decl;

	struct field {
		ir::type* type = nullptr;
		middle::initializer const* init;
	};

	std::unordered_map<std::string, field> fields;
};

using type_kind = std::variant<
	simple_type,
	integer_type,
	fp_type,
	pointer_type,
	reference_type,
	alias_type,
	variant_type,
	tuple_type,
	struct_type
>;

struct type {
	std::string name;
	type_kind kind;
};

} // namespace aw::script::ir

#endif // aw_script_ir_type_h
