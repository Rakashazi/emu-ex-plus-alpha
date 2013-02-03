#pragma once

namespace Gfx
{

void setTransformTarget(TransformTargetEnum target)
{
	glcMatrixMode(target == TARGET_TEXTURE ? GL_TEXTURE : GL_MODELVIEW);
}

void applyTranslate(TransformCoordinate x, TransformCoordinate y, TransformCoordinate z)
{
	glTranslatef(x, y, z);
}

void applyScale(TransformCoordinate sx, TransformCoordinate sy, TransformCoordinate sz)
{
	glScalef(sx, sy, sz);
}

void applyPitchRotate(Angle t)
{
	glRotatef(t, (Angle)1, (Angle)0, (Angle)0);
}

void applyRollRotate(Angle t)
{
	glRotatef(t, (Angle)0, (Angle)0, (Angle)1);
}

void applyYawRotate(Angle t)
{
	glRotatef(t, (Angle)0, (Angle)1, (Angle)0);
}

void loadTranslate(TransformCoordinate x, TransformCoordinate y, TransformCoordinate z)
{
	Matrix4x4<float> mat;
	mat.translate(x, y, z);
	glLoadMatrixf((GLfloat *)&mat.v[0]);
}

void loadIdentTransform()
{
	glLoadIdentity();
}

}
