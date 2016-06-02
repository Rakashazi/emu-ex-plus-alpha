/// @ref gtx_norm
/// @file glm/gtx/norm.inl

namespace glm
{
	template <typename T>
	GLM_FUNC_QUALIFIER T length2
	(
		T const & x
	)
	{
		return x * x;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T length2
	(
		tvec2<T, P> const & x
	)
	{
		return dot(x, x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T length2
	(
		tvec3<T, P> const & x
	)
	{
		return dot(x, x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T length2
	(
		tvec4<T, P> const & x
	)
	{
		return dot(x, x);
	}

	template <typename T>
	GLM_FUNC_QUALIFIER T distance2
	(
		T const & p0,
		T const & p1
	)
	{
		return length2(p1 - p0);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T distance2
	(
		tvec2<T, P> const & p0,
		tvec2<T, P> const & p1
	)
	{
		return length2(p1 - p0);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T distance2
	(
		tvec3<T, P> const & p0,
		tvec3<T, P> const & p1
	)
	{
		return length2(p1 - p0);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T distance2
	(
		tvec4<T, P> const & p0,
		tvec4<T, P> const & p1
	)
	{
		return length2(p1 - p0);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T l1Norm
	(
		tvec3<T, P> const & a,
		tvec3<T, P> const & b
	)
	{
		return abs(b.x - a.x) + abs(b.y - a.y) + abs(b.z - a.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T l1Norm
	(
		tvec3<T, P> const & v
	)
	{
		return abs(v.x) + abs(v.y) + abs(v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T l2Norm
	(
		tvec3<T, P> const & a,
		tvec3<T, P> const & b
	)
	{
		return length(b - a);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T l2Norm
	(
		tvec3<T, P> const & v
	)
	{
		return length(v);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T lxNorm
	(
		tvec3<T, P> const & x,
		tvec3<T, P> const & y,
		unsigned int Depth
	)
	{
		return pow(pow(y.x - x.x, T(Depth)) + pow(y.y - x.y, T(Depth)) + pow(y.z - x.z, T(Depth)), T(1) / T(Depth));
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER T lxNorm
	(
		tvec3<T, P> const & v,
		unsigned int Depth
	)
	{
		return pow(pow(v.x, T(Depth)) + pow(v.y, T(Depth)) + pow(v.z, T(Depth)), T(1) / T(Depth));
	}

}//namespace glm
