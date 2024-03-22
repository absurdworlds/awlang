module subscript;

foreign import: C {
function printf(str: u8*, var...args);
function puts(str: u8*);
}

function main()
{
	var x: int[3] = { 1,2,3 };

	// expect: 1
	printf("%d\n", x[0]);
	// expect: 2
	printf("%d\n", x[1]);
	// expect: 3
	printf("%d\n", x[2]);

	// expect: 1, 2, 3
	var i = 0;
	while i < 3 {
		printf("%s%d", if i == 0 "" else ", ", x[i]);
		i += 1;
	}
	puts("");
}
