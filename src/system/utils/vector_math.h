#ifndef UTILS_VECTOR_MATH_H
#define UTILS_VECTOR_MATH_H

/**
 * Define vectors and rotation matrices, and the possibility to rotate vectors
 */

/**
 * \brief 2d vector in any space
 */
struct vec2d
{
  float x;
  float y;

  vec2d() : x(0), y(0) {}
  vec2d(const vec2d& other) = default;
  vec2d(const float _x, const float _y) : x(_x), y(_y) {}

  float dot(const vec2d& other) const { return this->x * other.x + this->y * other.y; }
  vec2d multiply(const vec2d& other) const { return vec2d(this->x * other.x, this->y * other.y); }
  vec2d multiply(const float mult) const { return vec2d(this->x * mult, this->y * mult); }
  vec2d add(const vec2d& other) const { return vec2d(this->x + other.x, this->y + other.y); }
  vec2d add(const float mult) const { return vec2d(this->x + mult, this->y + mult); }

  vec2d& operator=(const vec2d& other)
  {
    // Guard self assignment
    if (this == &other)
      return *this;

    this->x = other.x;
    this->y = other.y;
    return *this;
  }
};

/**
 * \brief 3d vector in any space
 */
struct vec3d : public vec2d
{
  float z;

  vec3d() : vec2d(), z(0) {}
  vec3d(const float _x, const float _y, const float _z) : vec2d(_x, _y), z(_z) {}
  vec3d(const vec2d& res, const float _z) : vec2d(res), z(_z) {}

  float dot(const vec3d& other) const { return vec2d::dot(other) + this->z * other.z; }
  vec3d multiply(const vec3d& other) const { return vec3d(vec2d::multiply(other), this->z * other.z); }
  vec3d multiply(const float mult) const { return vec3d(vec2d::multiply(mult), this->z * mult); }
  vec3d add(const vec3d& other) const { return vec3d(vec2d::add(other), this->z + other.z); }
  vec3d add(const float mult) const { return vec3d(vec2d::add(mult), this->z + mult); }

  vec3d& operator=(const vec3d& other)
  {
    // Guard self assignment
    if (this == &other)
      return *this;

    this->x = other.x;
    this->y = other.y;
    this->z = other.z;
    return *this;
  }
};

/**
 * \brief 4d vector in any space
 */
struct vec4d : public vec3d
{
  float w;

  vec4d() : vec3d(), w(0) {}
  vec4d(const float _x, const float _y, const float _z, const float _w) : vec3d(_x, _y, _z) {}
  vec4d(const vec3d& res, const float _w) : vec3d(res), w(_w) {}

  float dot(const vec4d& other) const { return vec3d::dot(other) + this->w * other.w; }
  vec4d multiply(const vec4d& other) const { return vec4d(vec3d::multiply(other), this->w * other.w); }
  vec4d multiply(const float mult) const { return vec4d(vec3d::multiply(mult), this->w * mult); }
  vec4d add(const vec4d& other) const { return vec4d(vec3d::add(other), this->w + other.w); }
  vec4d add(const float mult) const { return vec4d(vec3d::add(mult), this->w + mult); }

  vec4d& operator=(const vec4d& other)
  {
    // Guard self assignment
    if (this == &other)
      return *this;

    this->x = other.x;
    this->y = other.y;
    this->z = other.z;
    this->w = other.w;
    return *this;
  }
};

/**
 * \brief Represent an XYZ rotation matrix
 */
struct RotationMatrix
{
  // first line
  float R11;
  float R12;
  float R13;

  // second line
  float R21;
  float R22;
  float R23;

  // third line
  float R31;
  float R32;
  float R33;

  vec3d col1() const { return vec3d(R11, R21, R31); }
  vec3d col2() const { return vec3d(R12, R22, R32); }
  vec3d col3() const { return vec3d(R13, R23, R33); }

  vec3d row1() const { return vec3d(R11, R12, R13); }
  vec3d row2() const { return vec3d(R21, R22, R23); }
  vec3d row3() const { return vec3d(R31, R32, R33); }

  /**
   * \brief transform a vector by this rotation
   */
  vec3d transform(const vec3d& vec) const;

  /**
   * \brief Compose this roation with another
   */
  RotationMatrix compose(const RotationMatrix& other) const;

  /**
   * \brief create a rotation matrix from three angles in radian (XYZ)
   */
  void from_angles(const float roll_rad, const float pitch_rad, const float yaw_rad);

  /**
   * \brief create a rotation matrix from three angles in radian (XYZ)
   */
  void from_angles(const vec3d& angles_rad);
};

/**
 * \brief Transform a vector from a space to another
 */
struct TransformationMatrix
{
  RotationMatrix rotation;
  vec3d translation;

  TransformationMatrix() = default;
  TransformationMatrix(const RotationMatrix& rot, const vec3d& trans);
  TransformationMatrix(const vec3d& euler, const vec3d& trans);

  vec3d transform(const vec3d& vec) const;
};

#endif
