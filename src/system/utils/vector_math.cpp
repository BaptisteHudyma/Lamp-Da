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

void RotationMatrix::from_angles(const float roll_rad, const float pitch_rad, const float yaw_rad)
{
  //                                X             Y           Z
  from_angles(vec3d(pitch_rad, roll_rad, yaw_rad));
}

void RotationMatrix::from_angles(const vec3d& angles_rad)
{
  auto si = sinf(angles_rad.x);
  auto sj = sinf(angles_rad.y);
  auto sk = sinf(angles_rad.z);

  auto ci = cosf(angles_rad.x);
  auto cj = cosf(angles_rad.y);
  auto ck = cosf(angles_rad.z);

  auto cc = ci * ck;
  auto cs = ci * sk;
  auto sc = si * ck;
  auto ss = si * sk;

  R11 = cj * ck;
  R12 = sj * sc - cs;
  R13 = sj * cc + ss;

  R21 = cj * sk;
  R22 = sj * ss + cc;
  R23 = sj * cs - sc;

  R31 = -sj;
  R32 = cj * si;
  R33 = cj * ci;
}

TransformationMatrix::TransformationMatrix(const RotationMatrix& rot, const vec3d& trans) :
  rotation(rot),
  translation(trans)
{
}
TransformationMatrix::TransformationMatrix(const vec3d& euler, const vec3d& trans) : translation(trans)
{
  rotation.from_angles(euler);
}
vec3d TransformationMatrix::transform(const vec3d& vec) const { return rotation.transform(vec).add(translation); }
