#include "backend_llvm.h"

#include <aw/script/diag/error_t.h>
#include <aw/script/symtab/scope.h>

#include <llvm/IR/Constant.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <iostream>

using namespace llvm;

namespace aw::script {

static error_t error_undefined_variable(diagnostics_engine& diag, string_view name)
{
	return error(diag, diagnostic_id::undefined_variable, location(), name);
}

static error_t error_not_implemented_yet(diagnostics_engine& diag)
{
	return error(diag, diagnostic_id::not_implemented_yet, location());
}

static error_t error_is_not_declared(diagnostics_engine& diag, string_view name)
{
	return error(diag, diagnostic_id::is_not_declared, location(), name);
}

backend_llvm::backend_llvm(diagnostics_engine& diag)
	: diag(diag)
{
	cur_module = std::make_unique<Module>("awscript", context);

	InitializeAllTargetInfos();
	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmParsers();
	InitializeAllAsmPrinters();
}

bool backend_llvm::set_target(string_view request_triple)
{
	using namespace std::string_view_literals;

	const std::string target_triple = request_triple.empty() ?
		sys::getDefaultTargetTriple() :
		std::string(request_triple);

	std::string error;
	auto target = TargetRegistry::lookupTarget(target_triple, error);
	if (!target) {
		errs() << error;
		return false;
	}

	auto cpu = "generic"sv;
	auto features = string_view{};

	TargetOptions opt;

	auto reloc_model = Optional<Reloc::Model>();

	target_machine = target->createTargetMachine(target_triple, cpu, features, opt, reloc_model);

	cur_module->setDataLayout(target_machine->createDataLayout());

	// TODO: this is temporary I implement external functions
	std::vector<Type*> args(1, Type::getInt64Ty(context));
	auto signature = FunctionType::get(Type::getVoidTy(context), args, false);
	auto func = Function::Create(signature, Function::ExternalLinkage, "putchar", cur_module.get());
	func->args().begin()->setName("i");

	return true;
}


bool backend_llvm::write_object_file(string_view out_path)
{
	std::error_code EC;
	raw_fd_ostream dest(out_path, EC, sys::fs::OF_None);

	if (EC) {
		errs() << "Could not open file: " << EC.message();
		return false;
	}

	legacy::PassManager pass;
	auto FileType = CGFT_ObjectFile;

	if (target_machine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
		errs() << "TheTargetMachine can't emit a file of this type";
		return false;
	}

	pass.run(*cur_module);
	dest.flush();

	return true;
}

void backend_llvm::dump_ir()
{
	cur_module->print(errs(), nullptr);
}

auto backend_llvm::gen(const ast::declaration& decl) -> llvm::Value*
{
	switch (decl.kind())
	{
	case ast::decl_kind::type:
	case ast::decl_kind::alias_type:
	case ast::decl_kind::class_type:
		break;
	case ast::decl_kind::function:
		return gen(decl.as<ast::function>());
	case ast::decl_kind::variable:
		break;
	}
	return nullptr;
}

auto backend_llvm::gen(const ast::function& decl) -> llvm::Value*
{
	std::vector<Type*> args(decl.args.size(), Type::getInt64Ty(context));
	auto signature = FunctionType::get(Type::getInt64Ty(context), args, false);
	auto func = Function::Create(signature, Function::ExternalLinkage, decl.name(), cur_module.get());

	// Set names for all arguments.
	unsigned i = 0;
	for (auto& arg : func->args())
		arg.setName(decl.args[i++]->name());

	// Create a new basic block to start insertion into.
	auto* bb = BasicBlock::Create(context, "entry", func);
	builder.SetInsertPoint(bb);

	symtab.clear();
	for (auto& arg : func->args())
		symtab[std::string(arg.getName())] = &arg;

	if (Value* res = gen(decl.body)) {
		builder.CreateRet(res);

		llvm::verifyFunction(*func);

		return func;
	}

	func->eraseFromParent();
	return nullptr;
}

auto backend_llvm::gen(const ast::variable& decl) -> llvm::Value*
{
	return nullptr;
}

auto backend_llvm::gen(const std::unique_ptr<ast::statement>& stmt) -> llvm::Value*
{
	return stmt ? gen(*stmt) : nullptr;
}


auto backend_llvm::gen(const ast::statement& stmt) -> llvm::Value*
{
	return std::visit([this] (auto&& stmt) { return gen(stmt); }, stmt);
}


auto backend_llvm::gen_if_condition(const std::unique_ptr<ast::expression>& expr) -> llvm::Value*
{
	auto* cond_v = gen(expr);
	if (!cond_v)
		return nullptr;

	if (cond_v->getType() == Type::getInt64Ty(context))
		cond_v = builder.CreateICmpNE(cond_v, ConstantInt::get(context, APInt(64, 0, true)), "ifcond");

	return cond_v;
}

auto backend_llvm::gen(const ast::if_else_statement& stmt) -> llvm::Value*
{
	auto* cond_v = gen_if_condition(stmt.if_expr);
	if (!cond_v)
		return nullptr;

	auto* function = builder.GetInsertBlock()->getParent();

	auto* then_bb  = BasicBlock::Create(context, "then", function);
	auto* merge_bb = BasicBlock::Create(context, "ifcont");
	auto* else_bb  = !stmt.else_body ? merge_bb : BasicBlock::Create(context, "else");

	builder.CreateCondBr(cond_v, then_bb, else_bb);

	builder.SetInsertPoint(then_bb);

	auto then_v = gen(stmt.if_body);
	if (!then_v)
		return nullptr;
	builder.CreateBr(merge_bb);
	// Codegen can change the current block, update then_bb for the PHI.
	then_bb = builder.GetInsertBlock();

	if (stmt.else_body) {
		function->getBasicBlockList().push_back(else_bb);
		builder.SetInsertPoint(else_bb);
		auto else_v = gen(stmt.else_body);
		if (!else_v)
			return nullptr;
		builder.CreateBr(merge_bb);
		else_bb = builder.GetInsertBlock();
	}

	function->getBasicBlockList().push_back(merge_bb);
	builder.SetInsertPoint(merge_bb);

	return UndefValue::get(Type::getVoidTy(context));
}

auto backend_llvm::gen(const ast::statement_block& list) -> llvm::Value*
{
	Value* ret = nullptr;
	for (const auto& stmt : list)
		ret = gen(stmt);
	return ret;
}

auto backend_llvm::gen(const ast::empty_statement& stmt) -> llvm::Value*
{
	return UndefValue::get(Type::getVoidTy(context));
}

auto backend_llvm::gen(const ast::numeric_literal& expr) -> llvm::Value*
{
	return ConstantInt::get(context, APInt(64, expr.value, 10));
}

auto backend_llvm::gen(const ast::value_expression& expr) -> llvm::Value*
{
	auto it = symtab.find(expr.name);
	if (it == symtab.end())
		return error_undefined_variable(diag, expr.name);
	return it->second;
}

auto backend_llvm::gen(const std::unique_ptr<ast::expression>& expr) -> llvm::Value*
{
	return expr ? gen(*expr) : nullptr;
}


auto backend_llvm::gen(const ast::expression& expr) -> llvm::Value*
{
	return std::visit([this] (auto&& expr) { return gen(expr); }, expr);
}


auto backend_llvm::gen(const ast::binary_expression& expr) -> llvm::Value*
{
	auto* L = gen(expr.lhs);
	auto* R = gen(expr.rhs);
	if (!L || !R)
		return nullptr;

	switch (expr.op) {
	case ast::binary_operator::minus:
		return builder.CreateSub(L, R, "subtmp");
	case ast::binary_operator::plus:
		return builder.CreateAdd(L, R, "addtmp");
	case ast::binary_operator::multiply:
		return builder.CreateMul(L, R, "multmp");
	case ast::binary_operator::less:
		return builder.CreateICmpSLT(L, R, "lttmp");
	case ast::binary_operator::greater:
		return builder.CreateICmpSGT(L, R, "gttmp");
	case ast::binary_operator::divide:
	case ast::binary_operator::assignment:
		return error_not_implemented_yet(diag);
	}
	return nullptr;
}


auto backend_llvm::gen(const ast::unary_expression& expr) -> llvm::Value*
{
	return nullptr;
}


auto backend_llvm::gen(const ast::call_expression& expr) -> llvm::Value*
{
	Function* callee = cur_module->getFunction(expr.func);
	if (!callee)
		return error_is_not_declared(diag, expr.func);

	if (callee->arg_size() != expr.args.size())
		return error_not_implemented_yet(diag); // argument mismatch

	std::vector<Value *> argv;
	for (const auto& arg : expr.args)
	{
		auto* res = gen(arg);
		if (!res)
			return nullptr;
		argv.push_back(res);
	}

	return builder.CreateCall(callee, argv, "calltmp");
}

auto backend_llvm::gen(const ast::if_expression& expr) -> llvm::Value*
{
	auto* cond_v = gen_if_condition(expr.if_expr);
	if (!cond_v)
		return nullptr;

	auto* function = builder.GetInsertBlock()->getParent();

	auto* then_bb  = BasicBlock::Create(context, "ifethen", function);
	auto* else_bb  = BasicBlock::Create(context, "ifeelse");
	auto* merge_bb = BasicBlock::Create(context, "ifecont");

	builder.CreateCondBr(cond_v, then_bb, else_bb);

	builder.SetInsertPoint(then_bb);
	auto* then_v = gen(*expr.if_body);
	if (!then_v)
		return nullptr;
	builder.CreateBr(merge_bb);
	then_bb = builder.GetInsertBlock();

	function->getBasicBlockList().push_back(else_bb);
	builder.SetInsertPoint(else_bb);
	auto* else_v = gen(expr.else_body);
	if (!else_v)
		return nullptr;
	builder.CreateBr(merge_bb);
	else_bb = builder.GetInsertBlock();

	function->getBasicBlockList().push_back(merge_bb);
	builder.SetInsertPoint(merge_bb);

	auto* phi = builder.CreatePHI(then_v->getType(), 2, "ifetmp");

	phi->addIncoming(then_v, then_bb);
	phi->addIncoming(else_v, else_bb);

	return phi;
}


auto backend_llvm::gen(const ast::string_literal& expr) -> llvm::Value*
{
	return nullptr;
}


} // namespace aw::script
