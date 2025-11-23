#include "vector_math.h"
#include <cmath>

vec3d RotationMatrix::transform(const vec3d& vec) const
{
  vec3d res;
  res.x = vec.dot(this->row1());
  res.y = vec.dot(this->row2());
  res.z = vec.dot(this->row3());
  return res;
}

RotationMatrix RotationMatrix::compose(const RotationMatrix& other) const
{
  RotationMatrix res;
  res.R11 = this->row1().dot(other.col1());
  res.R12 = this->row1().dot(other.col2());
  res.R13 = this->row1().dot(other.col3());

  res.R21 = this->row2().dot(other.col1());
  res.R22 = this->row2().dot(other.col2());
  res.R23 = this->row2().dot(other.col3());

  res.R31 = this->row3().dot(other.col1());
  res.R32 = this->row3().dot(other.col2());
  res.R33 = this->row3().dot(other.col3());
  return res;
}

void RotationMatrix::from_angles_XYZ(const float x_rad, const float y_rad, const float z_rad)
{
  //                               Z         Y        X
  from_angles_ZYX(vec3d(z_rad, y_rad, x_rad));
}
void RotationMatrix::from_angles_XYZ(const vec3d& angles_rad)
{
  //                               Z         Y        X
  from_angles_ZYX(vec3d(angles_rad.z, angles_rad.y, angles_rad.x));
}

void RotationMatrix::from_angles_ZYX(const vec3d& angles_rad)
{
  auto si = sinf(angles_rad.x);
  auto sj = sinf(angles_rad.y);
  auto sk = sinf(angles_rad.z);

  auto ci = cosf(angles_rad.x);
  auto cj = cosf(angles_rad.y);
  auto ck = cosf(angles_rad.z);

  R11 = cj * cj;
  R12 = ci * sj * sk - si * ck;
  R13 = ci * sj * ck + si * sk;

  R21 = sj * ck;
  R22 = si * sj * sk + ci * ck;
  R23 = si * sj * ck - ci * sk;

  R31 = -sj;
  R32 = cj * sj;
  R33 = cj * ck;
}

TransformationMatrix::TransformationMatrix(const RotationMatrix& rot, const vec3d& trans) :
  rotation(rot),
  translation(trans)
{
}
TransformationMatrix::TransformationMatrix(const vec3d& euler, const vec3d& trans) : translation(trans)
{
  rotation.from_angles_XYZ(euler);
}
vec3d TransformationMatrix::transform(const vec3d& vec) const { return rotation.transform(vec).add(translation); }
