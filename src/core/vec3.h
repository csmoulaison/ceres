#ifndef vec3_h_INCLUDED
#define vec3_h_INCLUDED

void v3_normalize(f32* v, f32* res);
void v3_cross(f32* a, f32* b, f32* res);
f32 v3_dot(f32* a, f32* b);

#ifdef CSM_CORE_IMPLEMENTATION

void v3_normalize(f32* v, f32* res) {
	f32 mag = sqrt((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
	res[0] = v[0] / mag;
	res[1] = v[1] / mag;
	res[2] = v[2] / mag;
}

void v3_cross(f32* a, f32* b, f32* res) {
	res[0] = a[1] * b[2] - a[2] * b[1];
	res[1] = a[2] * b[0] - a[0] * b[2];
	res[2] = a[0] * b[1] - a[1] * b[0];
}

f32 v3_dot(f32* a, f32* b) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

f32 v3_distance_squared(f32* a, f32* b) {
	f32 dx = a[0] - b[0];
	f32 dy = a[1] - b[1];
	f32 dz = a[2] - b[2];
	return dx * dx + dy * dy + dz * dz;
}

f32 v3_distance(f32* a, f32* b) {
	return sqrt(v3_distance_squared(a, b));
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // vec3_h_INCLUDED
