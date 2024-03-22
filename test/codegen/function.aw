foreign import: C {
function printf(str: u8*, ...);
}

function void(var i : int)
{
	printf("test, %d\n", i);
}

function int() : int
{
	return 1;
}

function main()
{
	void(int());
}
