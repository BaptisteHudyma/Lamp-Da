/*! \file vector_math.h
    \brief Define vectors and rotation matrices, and the possibility to rotate vectors
*/

#ifndef UTILS_VECTOR_MATH_H
#define UTILS_VECTOR_MATH_H

namespace lampda {
namespace utils {

/**
 * \brief 2d vector in any space
 */
struct vec2d
{
  float x; ///< x coordinate in 2D
  float y; ///< y coordinate in 2D

  /// Default constructor
  vec2d() : x(0), y(0) {}
  /// Construct by copy
  vec2d(const vec2d& other) = default;
  /// Construct by parametersr
  vec2d(const float _x, const float _y) : x(_x), y(_y) {}

  /// Dot product in 2D, representing the arcos of the angle between this vector and another
  float dot(const vec2d& other) const { return this->x * other.x + this->y * other.y; }
  /// Coefficient wise multiplication
  vec2d multiply(const vec2d& other) const { return vec2d(this->x * other.x, this->y * other.y); }
  /// Coefficient wise multiplication by a constant
  vec2d multiply(const float mult) const { return vec2d(this->x * mult, this->y * mult); }
  /// Coefficient wise addition
  vec2d add(const vec2d& other) const { return vec2d(this->x + other.x, this->y + other.y); }
  /// Coefficient wise addition by a constant
  vec2d add(const float mult) const { return vec2d(this->x + mult, this->y + mult); }

  /// Copy operator
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
  float z; ///< z coordinate of a 3D vector

  /// Default constructor
  vec3d() : vec2d(), z(0) {}
  /// Construct by parameters
  vec3d(const float _x, const float _y, const float _z) : vec2d(_x, _y), z(_z) {}
  /// Construct by 2D vector and parameter
  vec3d(const vec2d& res, const float _z) : vec2d(res), z(_z) {}

  /// Dot product in 3D, representing the arcos of the angle between this vector and another
  float dot(const vec3d& other) const { return vec2d::dot(other) + this->z * other.z; }
  /// Coefficient wise multiplication
  vec3d multiply(const vec3d& other) const { return vec3d(vec2d::multiply(other), this->z * other.z); }
  /// Coefficient wise multiplication by a constant
  vec3d multiply(const float mult) const { return vec3d(vec2d::multiply(mult), this->z * mult); }
  /// Coefficient wise addition
  vec3d add(const vec3d& other) const { return vec3d(vec2d::add(other), this->z + other.z); }
  /// Coefficient wise addition by a constant
  vec3d add(const float mult) const { return vec3d(vec2d::add(mult), this->z + mult); }

  /// Copy operator
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
  float w; ///< 4d part of the coordinate

  /// Default constructor
  vec4d() : vec3d(), w(0) {}
  /// Construct from parameters
  vec4d(const float _x, const float _y, const float _z, const float _w) : vec3d(_x, _y, _z), w(_w) {}
  /// Construct from 3D and 4D part
  vec4d(const vec3d& res, const float _w) : vec3d(res), w(_w) {}

  /// Dot product, representing the angle arcos from this vector to another.
  float dot(const vec4d& other) const { return vec3d::dot(other) + this->w * other.w; }
  /// Coefficient wise multiplication
  vec4d multiply(const vec4d& other) const { return vec4d(vec3d::multiply(other), this->w * other.w); }
  /// Coefficient wise multiplication by a constant
  vec4d multiply(const float mult) const { return vec4d(vec3d::multiply(mult), this->w * mult); }
  /// Coefficient wise addition
  vec4d add(const vec4d& other) const { return vec4d(vec3d::add(other), this->w + other.w); }
  /// Coefficient wise addition by a constant
  vec4d add(const float mult) const { return vec4d(vec3d::add(mult), this->w + mult); }

  /// Egality affectation
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
  float R11; ///< row 1, index 1
  float R12; ///< row 1, index 2
  float R13; ///< row 1, index 3

  // second line
  float R21; ///< row 2, index 1
  float R22; ///< row 2, index 2
  float R23; ///< row 2, index 3

  // third line
  float R31; ///< row 3, index 1
  float R32; ///< row 3, index 2
  float R33; ///< row 3, index 3

  vec3d col1() const { return vec3d(R11, R21, R31); } ///< Access the matrix by columns
  vec3d col2() const { return vec3d(R12, R22, R32); } ///< Access the matrix by columns
  vec3d col3() const { return vec3d(R13, R23, R33); } ///< Access the matrix by columns

  vec3d row1() const { return vec3d(R11, R12, R13); } ///< Access the matrix by row
  vec3d row2() const { return vec3d(R21, R22, R23); } ///< Access the matrix by row
  vec3d row3() const { return vec3d(R31, R32, R33); } ///< Access the matrix by row

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
  void from_angles_XYZ(const float x_rad, const float y_rad, const float z_rad);
  /**
   * \brief create a rotation matrix from three angles in radian (XYZ)
   */
  void from_angles_XYZ(const vec3d& angles_rad);

private:
  /**
   * \brief create a rotation matrix from three angles in radian (ZYX)
   */
  void from_angles_ZYX(const vec3d& angles_rad);
};

/**
 * \brief Transform a vector from a space to another
 */
struct TransformationMatrix
{
  /// Rotation part of the matrix
  RotationMatrix rotation;
  /// translation part of the matrix
  vec3d translation;

  /// Constructor
  TransformationMatrix() = default;
  /// Constructor from rotation and translation
  TransformationMatrix(const RotationMatrix& rot, const vec3d& trans);
  /// Constructor from euler angles and translation
  TransformationMatrix(const vec3d& euler, const vec3d& trans);

  /**
   * \brief Transform a vector by this transformation
   * \param[in] vec The vector to transform
   * \return the transformed vector
   */
  vec3d transform(const vec3d& vec) const;
};

} // namespace utils
} // namespace lampda

#endif
