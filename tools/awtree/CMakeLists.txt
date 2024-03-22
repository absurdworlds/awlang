
add_executable(awtree
	awtree.c++
)

target_link_libraries(awtree
	PRIVATE
		aw::io
		aw::utility
		awscript-lang)
