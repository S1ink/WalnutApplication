#pragma once


template<typename scalar = float>
struct vec2_ {
	scalar x, y;
};
template<typename scalar = float>
struct vec3_ {
	scalar x, y, z;
};
typedef vec2_<>	vec2;
typedef vec3_<> vec3;

template<typename scalar = float>
struct ray2_ {
	vec2_<scalar> origin, direction;
};
template<typename scalar = float>
struct ray3_ {
	vec3_<scalar> origin, direction;
};
typedef ray2_<> ray2;
typedef ray3_<> ray3;

template<typename scalar = float>
struct circle_ {
	scalar x, y, radius;
};
template<typename scalar = float>
struct sphere_ {
	scalar x, y, z, radius;
};
typedef circle_<> circle;
typedef sphere_<> sphere;