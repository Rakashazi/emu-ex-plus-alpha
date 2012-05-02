#pragma once

namespace Gfx
{

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
	glcMatrixMode(GL_MODELVIEW);
	Matrix4x4<float> mat;
	mat.translate(x, y, z);
	glLoadMatrixf((GLfloat *)&mat.v[0]);
}

void loadIdentTransform()
{
	glcMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

}
