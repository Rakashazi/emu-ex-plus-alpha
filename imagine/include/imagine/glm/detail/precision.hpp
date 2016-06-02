/// @ref core
/// @file glm/detail/precision.hpp

#pragma once

namespace glm
{
	enum precision
	{
		highp,
		mediump,
		lowp,
		aligned_highp,
		aligned_mediump,
		aligned_lowp,
		aligned = aligned_highp,
#		ifdef GLM_FORCE_ALIGNED
			defaultp = aligned_highp
#		else
			defaultp = highp
#		endif
	};

namespace detail
{
	template <precision P>
	struct is_aligned
	{
		static const bool value = false;
	};

	template<>
	struct is_aligned<aligned_lowp>
	{
		static const bool value = true;
	};

	template<>
	struct is_aligned<aligned_mediump>
	{
		static const bool value = true;
	};

	template<>
	struct is_aligned<aligned_highp>
	{
		static const bool value = true;
	};
}//namespace detail
}//namespace glm
