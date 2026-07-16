foreign import: C {
func puts(str: u8*);
}

func main() : int
{
	var i: int = 0;
	var count: int = 0;
	while i < 5 {
		++i;
		if i == 3 {
			continue;
		}
		++count;
	}

	if count == 4 {
		puts("passed!");
	}

	return 0;
}
