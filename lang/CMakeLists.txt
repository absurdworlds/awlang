
aw_add_library(awscript-lang STATIC
	HEADERS
		include/aw/script/utility/location.h
		include/aw/script/lexer/lexer.h
		include/aw/script/lexer/source_buffer.h
		include/aw/script/lexer/source_manager.h
		include/aw/script/lexer/token.h
		include/aw/script/lexer/tokens.h
		include/aw/script/utility/ast_printer_default.h
		include/aw/script/utility/print_token.h
		include/aw/script/utility/type_name.h
		include/aw/script/utility/wrap.h
		include/aw/script/utility/hard_alias.h
		include/aw/script/utility/value_ptr.h
		include/aw/script/utility/map.h
		include/aw/script/ast/mutability.h
		include/aw/script/ast/number_base.h
		include/aw/script/ast/declaration.h
		include/aw/script/ast/expression.h
		include/aw/script/ast/identifier.h
		include/aw/script/ast/linkage.h
		include/aw/script/ast/operators.h
		include/aw/script/ast/statement.h
		include/aw/script/ast/type.h
		include/aw/script/ast/middle/declaration.h
		include/aw/script/ast/middle/expression.h
		include/aw/script/ast/middle/module.h
		include/aw/script/ast/middle/statement.h
		include/aw/script/diag/diagnostic.h
		include/aw/script/diag/diagnostics_engine.h
		include/aw/script/diag/error_t.h
		include/aw/script/diag/messages.h
		include/aw/script/parser/parser.h
		include/aw/script/parser/operators.h
		include/aw/script/symtab/symbol_name.h
		include/aw/script/ir/type.h
		include/aw/script/semantic/dependency_resolver.h
		include/aw/script/semantic/scope.h
		include/aw/script/semantic/semantic_analyzer.h
		include/aw/script/ir/operators.h
		include/aw/script/codegen/backend.h
		include/aw/script/codegen/optimization_level.h
		include/aw/script/driver/main.h
		include/aw/script/driver/options.h
		source/driver/linker.h
		source/parser/errors.h
		source/semantic/context.h
		source/semantic/convert_to_middle.h
		source/semantic/dependency_scanner.h
		source/semantic/resolve_references.h
		source/semantic/type_inference.h
		source/codegen/llvm/backend_llvm.h
		source/codegen/llvm/optimizer_llvm.h
		source/codegen/llvm/llvm_helpers.h
	SOURCES
		source/lexer/lexer.c++
		source/diag/messages.c++
		source/diag/diagnostics_engine.c++
		source/driver/linker.c++
		source/driver/main.c++
		source/ast/statement.c++
		source/parser/parser.c++
		source/parser/parser_decl.c++
		source/parser/operators.c++
		source/semantic/context.c++
		source/semantic/convert_to_middle.c++
		source/semantic/dependency_resolver.c++
		source/semantic/resolve_references.c++
		source/semantic/scope.c++
		source/semantic/semantic_analyzer.c++
		source/semantic/type_inference.c++
		source/codegen/backend.c++
		source/codegen/llvm/backend_llvm.c++
		source/codegen/llvm/optimizer_llvm.c++
		source/codegen/llvm/gen_variable.c++
		source/codegen/llvm/gen_cast.c++
		source/utility/ast_printer_default.c++
	)

target_link_libraries(awscript-lang
	PUBLIC
		aw::io
		aw::string
		aw::platform)

set (AWSCRIPT_USE_LLVM ON)
if (AWSCRIPT_USE_LLVM)
	find_package(LLVM REQUIRED CONFIG)

	option (AWSCRIPT_USE_SYSTEM_LLVM "Use LLVM installed in the system" OFF)
	if (AWSCRIPT_USE_SYSTEM_LLVM)
		execute_process(COMMAND llvm-config --libs all
			        OUTPUT_VARIABLE LLVM_LIBRARIES
				OUTPUT_STRIP_TRAILING_WHITESPACE)
	else()
		message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
		message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

		separate_arguments(LLVM_DEFINITIONS)

		llvm_map_components_to_libnames(LLVM_LIBRARIES
			support
			core
			target
			analysis
			passes
			x86desc
			x86asmparser
			x86codegen)

		message(STATUS "LLVM_LIBRARIES: ${LLVM_LIBRARIES}")
		message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
		message(STATUS "LLVM_TOOLS_BINARY_DIR: ${LLVM_TOOLS_BINARY_DIR}")
		#message(STATUS "LLVM_AVAILABLE_LIBS: ${LLVM_AVAILABLE_LIBS}")
		#message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")

		target_include_directories(awscript-lang
			PRIVATE
				${LLVM_INCLUDE_DIRS})

		target_compile_definitions(awscript-lang
			PRIVATE
			        -DAWSCRIPT_DEBUG_LLVM=1)
	endif()

	target_link_libraries(awscript-lang
			PUBLIC
				${LLVM_LIBRARIES})
endif()


if (AWSCRIPT_MAKE_TESTS)
	add_subdirectory(tests)
endif()
