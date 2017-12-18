#pragma once

#include "Vector4.h"
#include "Vector3.h"
#include "Quaternion.h"

namespace Math {

using namespace DirectX;

class Matrix4
{
public:
    enum Index
    {
        Index00 = 0,
        Index10,
        Index20,
        Index30,
        Index01,
        Index11,
        Index21,
        Index31,
        Index02,
        Index12,
        Index22,
        Index32,
        Index03,
        Index13,
        Index23,
        Index33,
    };
public:
    Matrix4();
    Matrix4(
        float v00, float v01, float v02, float v03,                             // Row 1
        float v10, float v11, float v12, float v13,                             // Row 2
        float v20, float v21, float v22, float v23,                             // Row 3
        float v30, float v31, float v32, float v33                              // Row 4
    );
    Matrix4(const Vector4& row0, const Vector4& row1, const Vector4& row2, const Vector4& row3);
    Matrix4(const XMMATRIX& matrix);

    operator XMMATRIX() const
    {
        return XMMatrixSet(
            m_[0], m_[4], m_[8], m_[12],
            m_[1], m_[5], m_[9], m_[13],
            m_[2], m_[6], m_[10], m_[14],
            m_[3], m_[7], m_[11], m_[15]
        );
    }

    const Vector3 operator *(const Vector3& rhs) const;
    const Vector4 operator *(const Vector4& rhs) const;
    const Matrix4 operator *(const Matrix4& rhs) const;
    const Matrix4 operator *(float rhs) const
    {
        return Matrix4(
            m_[Index00] * rhs,
            m_[Index01] * rhs,
            m_[Index02] * rhs,
            m_[Index03] * rhs,
            m_[Index10] * rhs,
            m_[Index11] * rhs,
            m_[Index12] * rhs,
            m_[Index13] * rhs,
            m_[Index20] * rhs,
            m_[Index21] * rhs,
            m_[Index22] * rhs,
            m_[Index23] * rhs,
            m_[Index30] * rhs,
            m_[Index31] * rhs,
            m_[Index32] * rhs,
            m_[Index33] * rhs
        );
    }

    Matrix4 operator +(const Matrix4& rhs) const
    {
        return Matrix4(
            m_[Index00] + rhs.m_[Index00],
            m_[Index01] + rhs.m_[Index01],
            m_[Index02] + rhs.m_[Index02],
            m_[Index03] + rhs.m_[Index03],
            m_[Index10] + rhs.m_[Index10],
            m_[Index11] + rhs.m_[Index11],
            m_[Index12] + rhs.m_[Index12],
            m_[Index13] + rhs.m_[Index13],
            m_[Index20] + rhs.m_[Index20],
            m_[Index21] + rhs.m_[Index21],
            m_[Index22] + rhs.m_[Index22],
            m_[Index23] + rhs.m_[Index23],
            m_[Index30] + rhs.m_[Index30],
            m_[Index31] + rhs.m_[Index31],
            m_[Index32] + rhs.m_[Index32],
            m_[Index33] + rhs.m_[Index33]
        );
    }
    Matrix4 operator -(const Matrix4& rhs) const
    {
        return Matrix4(
            m_[Index00] - rhs.m_[Index00],
            m_[Index01] - rhs.m_[Index01],
            m_[Index02] - rhs.m_[Index02],
            m_[Index03] - rhs.m_[Index03],
            m_[Index10] - rhs.m_[Index10],
            m_[Index11] - rhs.m_[Index11],
            m_[Index12] - rhs.m_[Index12],
            m_[Index13] - rhs.m_[Index13],
            m_[Index20] - rhs.m_[Index20],
            m_[Index21] - rhs.m_[Index21],
            m_[Index22] - rhs.m_[Index22],
            m_[Index23] - rhs.m_[Index23],
            m_[Index30] - rhs.m_[Index30],
            m_[Index31] - rhs.m_[Index31],
            m_[Index32] - rhs.m_[Index32],
            m_[Index33] - rhs.m_[Index33]
        );
    }

    Matrix4& Translate(const Vector3& v);
    Matrix4& Scale(const Vector3& v);

    Matrix4& RotateX(float ang);
    Matrix4& RotateY(float ang);
    Matrix4& RotateZ(float ang);
    Matrix4& Rotate(const Vector3& axis, float ang);

    /// Get rotation part
    Quaternion Rotation(bool rowNormalize = true) const;
    /// Get translation part
    Vector3 Translation() const;
    /// Get scaling part
    Vector3 Scaling() const;
    void Decompose(Vector3* scale, Quaternion* rotation, Vector3* translation) const
    {
        XMVECTOR s, r, t;
        XMMatrixDecompose(&s, &r, &t, *this);
        scale->x_ = s.m128_f32[0];
        scale->y_ = s.m128_f32[1];
        scale->z_ = s.m128_f32[2];
        rotation->x_ = r.m128_f32[0];
        rotation->y_ = r.m128_f32[1];
        rotation->z_ = r.m128_f32[2];
        rotation->w_ = r.m128_f32[3];
        translation->x_ = t.m128_f32[0];
        translation->y_ = t.m128_f32[1];
        translation->z_ = t.m128_f32[2];
    }

    Matrix4 Transpose() const;
    float Determinant() const;
    Matrix4 Inverse() const;

    static Matrix4 FromFrustum(float left, float right, float bottom, float top, float _near, float _far);
    static Matrix4 FromPerspective(float fovy, float aspect, float _near, float _far);
    static Matrix4 FromOrtho(float left, float right, float bottom, float top, float _near, float _far);
    static Matrix4 FromOrtho(float width, float height, float _near, float _far);
    static Matrix4 FromLookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
    static Matrix4 FromQuaternion(const Quaternion& q);
    static Matrix4 FromAxisAngle(const Vector3& axis, float angle);
    static Matrix4 FromScale(const Vector3& scale);
    static Matrix4 FromTranslation(const Vector3& v);

    static Vector3 UnProject(const Vector3& vec, const Matrix4& view, const Matrix4& proj, const float viewport[]);
    static Vector3 Project(const Vector3& vec, const Matrix4& view, const Matrix4& proj, const float viewport[]);

    /// Column-major
    float m_[16];

    static const Matrix4 Identity;
};

}