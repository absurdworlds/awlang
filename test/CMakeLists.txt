file(GLOB_RECURSE SOURCES *.aw)

add_custom_target(testfiles SOURCES ${SOURCES})

set(TESTS
	syntax/as.aw
	syntax/comments.aw
	syntax/if.aw
	syntax/operators.aw
	syntax/precedence.aw
	syntax/struct.aw
	syntax/subscript.aw
	errors/invalid_decls.aw
	errors/not_allowed_here.aw
	errors/not_implemented_yet.aw
	errors/recovery.aw
	codegen/array.aw
	codegen/array_of_structs.aw
	codegen/chain.aw
	codegen/function.aw
	modules/main.aw
)

set(NEGATIVE_TESTS
	errors/invalid_decls.aw
	errors/not_allowed_here.aw
	errors/not_implemented_yet.aw
	errors/recovery.aw
)

set(RUNTIME_TESTS
	codegen/pointer.aw
	codegen/break.aw
	codegen/continue.aw
)

option(AWSCRIPT_NIY_TESTS "Run tests for NIY functionality" OFF)
if (AWSCRIPT_NIY_TESTS)
	list(APPEND TESTS
		errors/linebreaks.aw)
	list(APPEND NEGATIVE_TESTS
		errors/linebreaks.aw)
endif()

foreach (test ${RUNTIME_TESTS})
	get_filename_component(name ${test} NAME_WE)
	add_test (
		NAME ${test}
		COMMAND awscriptc --eval ${test} -o ${CMAKE_CURRENT_BINARY_DIR}/${name}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
	set_property(TEST ${test} PROPERTY TIMEOUT 3)
endforeach()

set_property(TEST codegen/pointer.aw PROPERTY PASS_REGULAR_EXPRESSION "pass;passed!")
set_property(TEST codegen/break.aw PROPERTY PASS_REGULAR_EXPRESSION "passed!")
set_property(TEST codegen/continue.aw PROPERTY PASS_REGULAR_EXPRESSION "passed!")

foreach (test ${TESTS})
	add_test (
		NAME ${test}
		COMMAND awscriptc -n ${test}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
	set_property(TEST ${test} PROPERTY TIMEOUT 3)
endforeach ()

foreach (test ${NEGATIVE_TESTS})
	set_property(TEST ${test} PROPERTY WILL_FAIL TRUE)
endforeach ()
