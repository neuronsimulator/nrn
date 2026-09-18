#pragma once

#include <Eigen/Eigen>

/*  3-D rotation matrix */

class Rotation3d final: public Resource {
  public:
    Rotation3d();

    Eigen::Vector3f rotate(const Eigen::Vector3f&) const;

    void identity();

    // relative transformation of the transformation
    void rotate_x(float radians);
    void rotate_y(float radians);
    void rotate_z(float radians);
    void origin(float x, float y, float z);
    void offset(float x, float y);
    std::array<float, 2> x_axis() const;
    std::array<float, 2> y_axis() const;
    std::array<float, 2> z_axis() const;

  private:
    Eigen::Matrix3f mat;
    Eigen::Transform<float, 3, Eigen::Affine> tr;
    Eigen::Transform<float, 3, Eigen::Affine> orig;
};
