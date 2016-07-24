#include <imagine/util/math/GLMMat4.hh>
#include <imagine/glm/geometric.hpp>
#include <imagine/glm/gtc/matrix_transform.hpp>
#include <imagine/glm/gtc/matrix_inverse.hpp>

namespace glm
{
	template <typename T, typename U, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> unProjectWithInverse
	(
		tvec3<T, P> const & win,
		tmat4x4<T, P> const & inverse,
		tvec4<U, P> const & viewport
	)
	{
		tvec4<T, P> tmp = tvec4<T, P>(win, T(1));
		tmp.x = (tmp.x - T(viewport[0])) / T(viewport[2]);
		tmp.y = (tmp.y - T(viewport[1])) / T(viewport[3]);
		tmp = tmp * T(2) - T(1);

		tvec4<T, P> obj = inverse * tmp;
		obj /= obj.w;

		return tvec3<T, P>(obj);
	}
}

GLMMat4 GLMMat4::translate(GLMVec3 translation) const
{
	return glm::translate(m, translation.v);
}

GLMMat4 GLMMat4::makeTranslate(GLMVec3 translation)
{
	return glm::translate({}, translation.v);
}

GLMMat4 GLMMat4::makePerspectiveFovRH(float fovy, float aspect, float znear, float zfar)
{
	return glm::perspective(fovy, aspect, znear, zfar);
}

GLMMat4 GLMMat4::scale(GLMVec3 factors) const
{
	return glm::scale(m, factors.v);
}

GLMMat4 GLMMat4::rotate(float angle, GLMVec3 axis) const
{
	return glm::rotate(m, angle, axis.v);
}

GLMMat4 GLMMat4::invert() const
{
	return glm::inverse(m);
}

GLMVec3 GLMMat4::project(IG::Rect2<int> viewport, GLMVec3 obj) const
{
	glm::ivec4 viewportVec {viewport.x, viewport.y, viewport.x2, viewport.y2};
	return glm::project(obj.v, {}, m, viewportVec);
}

GLMVec3 GLMMat4::unproject(IG::Rect2<int> viewport, GLMVec3 win, GLMMat4 inverse) const
{
	glm::ivec4 viewportVec {viewport.x, viewport.y, viewport.x2, viewport.y2};
	return glm::unProjectWithInverse(win.v, inverse.m, viewportVec);
}
