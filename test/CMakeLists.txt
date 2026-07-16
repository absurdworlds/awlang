file(GLOB_RECURSE SOURCES *.aw)

add_custom_target(testfiles SOURCES ${SOURCES})

set(TESTS
	syntax/comments.aw
	syntax/operators.aw
	syntax/precedence.aw
	errors/invalid_decls.aw
	errors/not_allowed_here.aw
	errors/not_implemented_yet.aw
	errors/recovery.aw
	codegen/array.aw
	codegen/array_of_structs.aw
	codegen/function.aw
	modules/main.aw
)

# Tests whose .aw source carries `// expect: <value>` comments documenting
# what the program should print. run_expect_test.py compiles and runs the
# program and checks those values show up in its stdout, in order -- these
# comments used to be undocumented notes nothing ever verified.
set(EXPECT_TESTS
	syntax/as.aw
	syntax/if.aw
	syntax/struct.aw
	syntax/subscript.aw
	codegen/chain.aw
)

set(NEGATIVE_TESTS
	errors/invalid_decls.aw
	errors/not_allowed_here.aw
	errors/not_implemented_yet.aw
	errors/recovery.aw
)

set(RUNTIME_TESTS
	codegen/pointer.aw
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

find_package(Python3 COMPONENTS Interpreter)
if (Python3_Interpreter_FOUND)
	foreach (test ${EXPECT_TESTS})
		get_filename_component(name ${test} NAME_WE)
		add_test (
			NAME ${test}
			COMMAND ${Python3_EXECUTABLE}
				${CMAKE_CURRENT_SOURCE_DIR}/run_expect_test.py
				$<TARGET_FILE:awscriptc>
				${CMAKE_CURRENT_SOURCE_DIR}/${test}
				${CMAKE_CURRENT_BINARY_DIR}/${name}_expect
		)
		set_property(TEST ${test} PROPERTY TIMEOUT 5)
	endforeach()
endif()

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
