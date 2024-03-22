/*
 * Copyright (C) 2023 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_codegen_backend_llvm_h
#define aw_script_codegen_backend_llvm_h

// TODO: put this into a separate static lib

#include "aw/script/ast/middle/module.h"

#include <aw/script/diag/diagnostics_engine.h>

#include <aw/script/codegen/backend.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Value.h>

#include <llvm/Target/TargetMachine.h>
#include <map>

namespace aw::script {
class optimizer_llvm;

class backend_llvm : public backend {
public:
	explicit backend_llvm(diagnostics_engine& diag);

	~backend_llvm() override;

	bool set_target(string_view target_triple) override;
	void set_optimization_level(optimization_level level) override;

	bool create_module(const middle::module& mod) override;

	bool handle_declaration(const middle::declaration& decl) override
	{
		return gen(decl) != nullptr;
	}

	bool write_object_file(string_view out_path) override;

	void dump_ir() override;

	bool optimize_module() override;
	bool optimize_lto() override;

	auto gen(const middle::declaration& decl) -> llvm::Value*;
	auto gen(const middle::function& decl) -> llvm::Value*;
	auto gen(const middle::variable& var) -> llvm::Value*;
	auto gen(const middle::struct_decl& var) -> llvm::Value*;

	auto gen(const std::unique_ptr<middle::statement>& stmt) -> llvm::Value*;
	auto gen_local(const middle::variable& stmt) -> llvm::Value*;
	auto gen(const middle::statement& stmt) -> llvm::Value*;
	auto gen(const middle::decl_statement& stmt) -> llvm::Value*;
	auto gen(const middle::if_else_statement& stmt) -> llvm::Value*;
	auto gen(const middle::while_statement& stmt) -> llvm::Value*;
	auto gen(const middle::statement_block& stmt) -> llvm::Value*;
	auto gen(const middle::return_statement& stmt) -> llvm::Value*;
	auto gen(const middle::empty_statement& stmt) -> llvm::Value*;

	auto gen(const value_ptr<middle::expression>& expr) -> llvm::Value*;
	auto gen(const std::unique_ptr<middle::expression>& expr) -> llvm::Value*;
	auto gen(const middle::expression& expr) -> llvm::Value*;
	auto gen_lvalue(const std::unique_ptr<middle::expression>& expr) -> llvm::Value*;
	auto gen_lvalue(const middle::expression& expr) -> llvm::Value*;
	auto gen(const middle::cast_expression& expr) -> llvm::Value*;
	auto gen(const middle::numeric_literal& expr) -> llvm::Constant*;
	auto gen(const middle::float_literal& expr) -> llvm::ConstantFP*;
	auto gen(const middle::bool_literal& expr) -> llvm::ConstantInt*;
	auto gen(const middle::string_literal& expr) -> llvm::Value*;
	auto gen(const middle::list_literal& expr, std::string_view name = {}) -> llvm::Value*;
	auto gen(const middle::numbered_list_literal& expr, std::string_view name = {}) -> llvm::Value*;
	auto gen(const middle::struct_literal& expr, std::string_view name = {}) -> llvm::Value*;
	auto gen(const middle::value_expression& expr) -> llvm::Value*;
	auto gen(const middle::binary_expression& expr) -> llvm::Value*;
	auto gen(const middle::unary_expression& expr) -> llvm::Value*;
	auto gen(const middle::call_expression& expr) -> llvm::Value*;
	auto gen(const middle::field_expression& expr) -> llvm::Value*;
	auto gen(const middle::subscript_expression& expr) -> llvm::Value*;
	auto gen(const middle::if_expression& expr) -> llvm::Value*;

	template<typename T>
	auto gen(const T&) -> llvm::Value*
	{
		assert(!"Unimplemented visitor");
		return nullptr;
	}

	auto gen_if_condition(const std::unique_ptr<middle::expression>& expr) -> llvm::Value*;

	void gen_global_ctors();

	void create_global_ctor_body(llvm::Function* func, llvm::GlobalVariable* var, const middle::expression& expr);
	auto create_global_ctor(llvm::GlobalVariable* var, const middle::variable& decl) -> llvm::Function*;

private:
	diagnostics_engine& diag;
	llvm::LLVMContext context;
	llvm::IRBuilder<> builder{context};
	llvm::TargetMachine* target_machine = nullptr;

	std::unique_ptr<optimizer_llvm> optimizer;
	std::unique_ptr<llvm::Module> cur_module;

	std::map<std::string, llvm::Value*, std::less<>> globals;
	std::map<std::string, llvm::Value*, std::less<>> symtab;
	std::map<std::string_view, llvm::Value*, std::less<>> strings;

	struct global_ctor_entry {
		int proprity = 0;
		llvm::Function* ctor = nullptr;
	};

	std::vector<global_ctor_entry> global_ctors;
};

} // namespace aw::script
#endif//aw_script_codegen_backend_llvm_h
