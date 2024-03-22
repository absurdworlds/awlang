aw_add_test(awscript_test_lang
	SOURCES
		lexer/lexer.c++
		parser/parser.c++
		semantic/resolve_deps.c++
)

target_link_libraries(awscript_test_lang
	PRIVATE
		awscript-lang
		aw::string
		aw::test
		aw::test-runner)
