foreign import: C {
func puts(str: u8*);
}

func main() : int
{
	var i: int = 0;
	var count: int = 0;
	while i < 10 {
		if i == 3 {
			break;
		}
		++count;
		++i;
	}

	if count == 3 {
		puts("passed!");
	}

	return 0;
}
