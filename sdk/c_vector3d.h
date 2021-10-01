#pragma once

#include <cmath>
#include "../utils/intrinsics.h"

class c_vector3d
{
public:
	float x, y, z;



	__forceinline c_vector3d() : c_vector3d(0.f, 0.f, 0.f) { }

	__forceinline c_vector3d(const float x, const float y, const float z) : x(x), y(y), z(z) { }

	__forceinline c_vector3d operator+(const c_vector3d& v) const
	{
		return c_vector3d(x + v.x, y + v.y, z + v.z);
	}

	__forceinline c_vector3d operator+(float fl)
	{
		return c_vector3d(x + fl, y + fl, z + fl);
	}

	__forceinline c_vector3d operator-(float fl)
	{
		return c_vector3d(x - fl, y - fl, z - fl);
	}

	__forceinline c_vector3d operator-(const c_vector3d& v) const
	{
		return c_vector3d(x - v.x, y - v.y, z - v.z);
	}

	__forceinline c_vector3d operator*(const c_vector3d& v) const
	{
		return c_vector3d(x * v.x, y * v.y, z * v.z);
	}

	__forceinline c_vector3d operator/(const c_vector3d& v) const
	{
		return c_vector3d(x / v.x, y / v.y, z / v.z);
	}

	__forceinline c_vector3d operator*(const float v) const
	{
		return c_vector3d(x * v, y * v, z * v);
	}

	__forceinline c_vector3d operator/(const float v) const
	{
		return c_vector3d(x / v, y / v, y / v);
	}
	__forceinline float& operator[](int i) {
		return ((float*)this)[i];
	}

	__forceinline float operator[](int i) const {
		return ((float*)this)[i];
	}

	__forceinline c_vector3d operator+=(const c_vector3d& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}



	__forceinline c_vector3d operator-=(const c_vector3d& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	__forceinline c_vector3d operator*=(const c_vector3d& other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}

	__forceinline c_vector3d operator/=(const c_vector3d& other)
	{
		x /= other.x;
		y /= other.y;
		z /= other.z;
		return *this;
	}

	__forceinline c_vector3d operator*=(const float other)
	{
		x *= other;
		y *= other;
		z *= other;
		return *this;
	}

	__forceinline c_vector3d operator/=(const float other)
	{
		x /= other;
		y /= other;
		z /= other;
		return *this;
	}



	__forceinline bool operator==(const c_vector3d other)
	{
		if (x == other.x && y == other.y && z == other.z)
			return true;
		return false;
	}

	__forceinline float length() const
	{
		m128 tmp;
		tmp.f[0] = x * x + y * y + z * z;
		const auto calc = sqrt_ps(tmp.v);
		return reinterpret_cast<const m128*>(&calc)->f[0];
	}

	inline float lengthsqr(void) const
	{
		return x * x + y * y + z + z;
	}

	__forceinline float length2d() const
	{
		m128 tmp;
		tmp.f[0] = x * x + y * y;
		const auto calc = sqrt_ps(tmp.v);
		return reinterpret_cast<const m128*>(&calc)->f[0];
	}

	__forceinline float length2dsqr() const
	{
		return x * x + y * y;
	}

	__forceinline float dot(const c_vector3d& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	__forceinline float dist_to(const c_vector3d& vOther) const 
	{
		c_vector3d delta;

		delta.x = x - vOther.x;
		delta.y = y - vOther.y;
		delta.z = z - vOther.z;

		return delta.length();
	}

	__forceinline float dot(const float* other) const
	{
		return x * other[0] + y * other[1] + z * other[2];
	}

	__forceinline c_vector3d cross(const c_vector3d& other) const
	{
		return c_vector3d(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		);
	}

	__forceinline c_vector3d normalize()
	{
		const auto l = length();

		if (l > 0)
		{
			x /= l;
			y /= l;
			z /= l;
		}

		return *this;
	}


	__forceinline float normalize_in_place()
	{
		c_vector3d& v = *this;

		float iradius = 1.f / (this->length() + 1.192092896e-07F);

		v.x *= iradius;
		v.y *= iradius;
		v.z *= iradius;

		return v.length();
	}

	__forceinline bool is_valid() const
	{
		return std::isfinite(this->x) && std::isfinite(this->y) && std::isfinite(this->z);
	}
#define CHECK_VALID( _v ) 0
	void Clamp();
	//void Normalize();
	void Sanitize();

	inline c_vector3d Normalize()
	{
		CHECK_VALID(*this);

		c_vector3d vector;
		float length = this->length();

		if (length != 0)
		{
			vector.x = x / length;
			vector.y = y / length;
			vector.z = z / length;
		}
		else
			vector.x = vector.y = 0.0f; vector.z = 1.0f;

		return vector;
	}

	inline c_vector3d clamp_angles()
	{
		//CHECK_VALID(*this);

		if (this->x < -89.0f)
			this->x = -89.0f;

		if (this->x > 89.0f)
			this->x = 89.0f;

		while (this->y < -180.0f)
			this->y += 360.0f;

		while (this->y > 180.0f)
			this->y -= 360.0f;

		this->z = 0.0f;

		return *this;
	}

	inline c_vector3d NormalizeNoClamp()
	{
		//CHECK_VALID(*this);

		this->x -= floorf(this->x / 360.0f + 0.5f) * 360.0f;

		this->y -= floorf(this->y / 360.0f + 0.5f) * 360.0f;

		this->z -= floorf(this->z / 360.0f + 0.5f) * 360.0f;

		return *this;
	}
};

class c_vector3d_aligned : public c_vector3d
{
	float w{};	
};

using c_qangle = c_vector3d;
