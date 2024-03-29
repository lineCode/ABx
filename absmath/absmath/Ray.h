#pragma once

#include "Vector3.h"

namespace Math {

class BoundingBox;
class Sphere;
class Matrix4;

/// Infinite straight line in three-dimensional space.
class Ray
{
public:
    Ray() {}
    Ray(const Vector3& origin, const Vector3& direction)
    {
        Define(origin, direction);
    }
    Ray(const Ray& other) :
        origin_(other.origin_),
        direction_(other.direction_)
    {}

    /// Assign from another ray.
    Ray& operator =(const Ray& rhs)
    {
        origin_ = rhs.origin_;
        direction_ = rhs.direction_;
        return *this;
    }

    /// Check for equality with another ray.
    bool operator ==(const Ray& rhs) const { return origin_ == rhs.origin_ && direction_ == rhs.direction_; }

    /// Check for inequality with another ray.
    bool operator !=(const Ray& rhs) const { return origin_ != rhs.origin_ || direction_ != rhs.direction_; }

    /// Define from origin and direction. The direction will be normalized.
    void Define(const Vector3& origin, const Vector3& direction)
    {
        origin_ = origin;
        direction_ = direction.Normal();
    }

    /// Project a point on the ray.
    Vector3 Project(const Vector3& point) const
    {
        const Vector3 offset = point - origin_;
        return origin_ + offset.DotProduct(direction_) * direction_;
    }

    /// Return distance of a point from the ray.
    float Distance(const Vector3& point) const
    {
        const Vector3 projected = Project(point);
        return (point - projected).Length();
    }

    /// Return closest point to another ray.
    Vector3 ClosestPoint(const Ray& ray) const;
    /// Return hit distance to a bounding box, or infinity if no hit.
    float HitDistance(const BoundingBox& box) const;
    /// Return hit distance to a sphere, or infinity if no hit.
    float HitDistance(const Sphere& sphere) const;

    /// Return transformed by a 3x4 matrix. This may result in a non-normalized direction.
    Ray Transformed(const Matrix4& transform) const;
    bool IsDefined() const { return origin_ != Vector3::Zero && direction_ != Vector3::Zero; }

    /// Ray origin.
    Vector3 origin_;
    /// Ray direction.
    Vector3 direction_;
};

}
