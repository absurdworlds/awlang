#!/usr/bin/env python3
"""
Compiles and runs an .aw test file, then checks that the values named in
its `// expect: <value>` comments appear in the program's stdout, in order.

Usage: run_expect_test.py <awscriptc> <source.aw> <output-binary>
"""
import re
import subprocess
import sys

EXPECT_RE = re.compile(r"//\s*expect:\s*(.*)")


def main() -> int:
	if len(sys.argv) != 4:
		print("usage: run_expect_test.py <awscriptc> <source.aw> <output>", file=sys.stderr)
		return 1

	awscriptc, source, output = sys.argv[1:4]

	with open(source, encoding="utf-8") as f:
		text = f.read()

	expected = [m.group(1).rstrip() for m in EXPECT_RE.finditer(text)]

	if not expected:
		print(f"{source}: no '// expect:' comments found", file=sys.stderr)
		return 1

	compiled = subprocess.run(
		[awscriptc, source, "-o", output],
		capture_output=True, text=True,
	)
	if compiled.returncode != 0:
		print(f"{source}: compile failed:", file=sys.stderr)
		print(compiled.stdout + compiled.stderr, file=sys.stderr)
		return 1

	ran = subprocess.run([output], capture_output=True, text=True)
	actual = ran.stdout

	pos = 0
	for value in expected:
		found = actual.find(value, pos)
		if found == -1:
			print(f"{source}: expected {value!r} not found (in order) in program output", file=sys.stderr)
			print(f"--- actual output ---\n{actual}", file=sys.stderr)
			return 1
		pos = found + len(value)

	return 0


if __name__ == "__main__":
	sys.exit(main())
