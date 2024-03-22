foreign import: C {
function printf(str: u8*, var...args);
}

struct vec3 {
	x: f64;
	y: f64;
	z: f64;
}

function main()
{
	const v: vec3 = {
		.x = 1.0,
		.y = 2.0,
		.z = 3.0,
	};

	// expect: 1.0, 2.0, 3.0
	printf("%.1f, %.1f, %.1f\n", v.x, v.y, v.z);
}
