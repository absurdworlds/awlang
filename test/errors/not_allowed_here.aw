foreign import: C {
	// error: not_allowed_here
	module test {
		function f();
	}
}

module test {
	// error: not_allowed_here
	module test;
	function f();
}
