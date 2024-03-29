#pragma once

#include "Vector3.h"
#include "Vector4.h"
#include <string>

namespace Math {

class Matrix4;

class Quaternion
{
public:
    Quaternion() noexcept :
        w_(1.0f),
        x_(0.0f),
        y_(0.0f),
        z_(0.0f)
    {}
    Quaternion(float w, float x, float y, float z) noexcept :
        w_(w),
        x_(x),
        y_(y),
        z_(z)
    {}
    Quaternion(float x, float y, float z);
#if defined(HAVE_DIRECTX_MATH)
    Quaternion(const XMath::XMVECTOR& q) :
        w_(XMath::XMVectorGetW(q)),
        x_(XMath::XMVectorGetX(q)),
        y_(XMath::XMVectorGetY(q)),
        z_(XMath::XMVectorGetZ(q))
    { }
#endif
    explicit Quaternion(const Vector3& eulerAngles) :
        // x = Pitch, y = Yaw, z = Roll
        Quaternion(eulerAngles.x_, eulerAngles.y_, eulerAngles.z_)
    {}
    /// Parse from string
    explicit Quaternion(const std::string& str);

    /// Create a Quaternion representing the rotation between two 3D vectors
    static Quaternion FromTwoVectors(const Vector3& u, const Vector3& v);
    /// Create a Quaternion from Axis and Angle. Angle is Rad
    static Quaternion FromAxisAngle(const Vector3& axis, float angle);

#if defined(HAVE_DIRECTX_MATH)
    /// Cast to XMVECTOR
    operator XMath::XMVECTOR() const
    {
        return XMath::XMVectorSet(x_, y_, z_, w_);
    }
    operator XMath::XMFLOAT4() const
    {
        return { x_, y_, z_, w_ };
    }
#endif

    /// Test for equality
    bool operator ==(const Quaternion& rhs) const {
        return Equals(rhs);
    }
    /// Test for inequality
    bool operator !=(const Quaternion& rhs) const {
        return !Equals(rhs);
    }

    Quaternion& operator+=(const Quaternion& v);
    Quaternion& operator-=(const Quaternion& v);

    const Quaternion operator+(const Quaternion& v) const;
    const Quaternion operator-(const Quaternion& v) const;

    friend Quaternion operator*(const Quaternion& v, float n);
    friend Quaternion operator*(float n, const Quaternion& v);
    /// Multiply Quaternions
    /// https://www.3dgep.com/understanding-quaternions/#Quaternion_Products
    friend Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);
    Quaternion& operator*=(const Quaternion& rhs);

    friend Quaternion operator/(const Quaternion& v, float n);
    friend Quaternion operator/(float n, const Quaternion& v);

    Vector3 operator *(const Vector3& rhs) const
    {
        const Vector3 qVec(x_, y_, z_);
        const Vector3 cross1(qVec.CrossProduct(rhs));
        const Vector3 cross2(qVec.CrossProduct(cross1));

        return rhs + 2.0f * (cross1 * w_ + cross2);
    }
    /// Test for equality with another vector with epsilon.
    bool Equals(const Quaternion& rhs) const
    {
        return Math::Equals(x_, rhs.x_) && Math::Equals(y_, rhs.y_) && Math::Equals(z_, rhs.z_) && Math::Equals(w_, rhs.w_);
    }

    Quaternion Inverse() const;
    Quaternion Conjugate() const;

    /// Return Axis and Angle
    /// x, y, z -> Axis
    /// w -> Angle
    Vector4 AxisAngle() const;
    /// Convert to Euler angles
    /// x = roll
    /// y = pitch
    /// z = yaw
    Vector3 EulerAngles() const;
    const Quaternion Normal() const;
    void Normalize();
    float LengthSqr() const;
    float Length() const;
    /// Return rotation matrix
    Matrix4 GetMatrix() const;

    std::string ToString() const
    {
        std::stringstream ss;
        ss << w_ << " " << x_ << " " << y_ << " " << z_;
        return ss.str();
    }

    float w_;
    float x_;
    float y_;
    float z_;

    static const Quaternion Identity;
};

}
