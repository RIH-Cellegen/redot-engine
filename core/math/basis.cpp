/**************************************************************************/
/*  basis.cpp                                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             REDOT ENGINE                               */
/*                        https://redotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2024-present Redot Engine contributors                   */
/*                                          (see REDOT_AUTHORS.md)        */
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "basis.h"
#include "core/math/math_funcs.h"
#include "core/string/ustring.h"

#define cofac(row1, col1, row2, col2) \
	(rows[row1][col1] * rows[row2][col2] - rows[row1][col2] * rows[row2][col1])

void Basis::invert() {
	real_t co0 = cofac(1, 1, 2, 2);
	real_t co1 = cofac(1, 2, 2, 0);
	real_t co2 = cofac(1, 0, 2, 1);

	real_t det = rows[0][0] * co0 + rows[0][1] * co1 + rows[0][2] * co2;
	#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
	#endif
	real_t s = 1.0f / det;

	set(co0 * s, cofac(0, 2, 2, 1) * s, cofac(0, 1, 1, 2) * s,
		co1 * s, cofac(0, 0, 2, 2) * s, cofac(0, 2, 1, 0) * s,
		co2 * s, cofac(0, 1, 2, 0) * s, cofac(0, 0, 1, 1) * s);
}

void Basis::orthonormalize() {
	Vector3 x = rows[0];
	Vector3 y = rows[1];
	Vector3 z = rows[2];

	x.normalize();
	y = (y - x * x.dot(y)).normalized();
	z = (z - x * x.dot(z) - y * y.dot(z)).normalized();

	rows[0] = x;
	rows[1] = y;
	rows[2] = z;
}

Basis Basis::orthonormalized() const {
	Basis c = *this;
	c.orthonormalize();
	return c;
}

void Basis::orthogonalize() {
	Vector3 scl = get_scale();
	orthonormalize();
	scale_local(scl);
}

Basis Basis::orthogonalized() const {
	Basis c = *this;
	c.orthogonalize();
	return c;
}

// Returns true if the basis vectors are orthogonal (perpendicular), so it has no skew or shear, and can be decomposed into rotation and scale.
// See https://en.wikipedia.org/wiki/Orthogonal_basis
bool Basis::is_orthogonal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	return Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis vectors are orthonormal (orthogonal and normalized), so it has no scale, skew, or shear.
// See https://en.wikipedia.org/wiki/Orthonormal_basis
bool Basis::is_orthonormal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	return Math::is_equal_approx(x.length_squared(), 1) && Math::is_equal_approx(y.length_squared(), 1) && Math::is_equal_approx(z.length_squared(), 1) && Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis is conformal (orthogonal, uniform scale, preserves angles and distance ratios).
// See https://en.wikipedia.org/wiki/Conformal_linear_transformation
bool Basis::is_conformal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	const real_t x_len_sq = x.length_squared();
	return Math::is_equal_approx(x_len_sq, y.length_squared()) && Math::is_equal_approx(x_len_sq, z.length_squared()) && Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis only has diagonal elements, so it may only have scale or flip, but no rotation, skew, or shear.
bool Basis::is_diagonal() const {
	return (
			Math::is_zero_approx(rows[0][1]) && Math::is_zero_approx(rows[0][2]) &&
			Math::is_zero_approx(rows[1][0]) && Math::is_zero_approx(rows[1][2]) &&
			Math::is_zero_approx(rows[2][0]) && Math::is_zero_approx(rows[2][1]));
}

// Returns true if the basis is a pure rotation matrix, so it has no scale, skew, shear, or flip.
bool Basis::is_rotation() const {
	return is_conformal() && Math::is_equal_approx(determinant(), 1, (real_t)UNIT_EPSILON);
}

#ifdef MATH_CHECKS
// This method is only used once, in diagonalize. If it's desired elsewhere, feel free to remove the #ifdef.
bool Basis::is_symmetric() const {
	if (!Math::is_equal_approx(rows[0][1], rows[1][0])) {
		return false;
	}
	if (!Math::is_equal_approx(rows[0][2], rows[2][0])) {
		return false;
	}
	if (!Math::is_equal_approx(rows[1][2], rows[2][1])) {
		return false;
	}

	return true;
}
#endif

Basis Basis::diagonalize() {
	#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!is_symmetric(), Basis());
	#endif
	const int ite_max = 64; // Reduced from 1024 with tolerance check
	real_t off_norm = rows[0][1]*rows[0][1] + rows[0][2]*rows[0][2] + rows[1][2]*rows[1][2];

	Basis acc_rot;
	for (int ite = 0; ite < ite_max && off_norm > CMP_EPSILON2; ++ite) {
		int i, j;
		real_t max_el = Math::abs(rows[0][1]);
		i = 0; j = 1;
		if (Math::abs(rows[0][2]) > max_el) { max_el = Math::abs(rows[0][2]); i = 0; j = 2; }
		if (Math::abs(rows[1][2]) > max_el) { i = 1; j = 2; }

		real_t angle = 0.5f * Math::atan2(2 * rows[i][j], rows[j][j] - rows[i][i]);
		Basis rot;
		rot.rows[i][i] = rot.rows[j][j] = Math::cos(angle);
		rot.rows[i][j] = -(rot.rows[j][i] = Math::sin(angle));

		*this = rot * *this * rot.transposed();
		acc_rot = rot * acc_rot;
		off_norm = rows[0][1]*rows[0][1] + rows[0][2]*rows[0][2] + rows[1][2]*rows[1][2];
	}
	return acc_rot;
}

Basis Basis::inverse() const {
	Basis inv = *this;
	inv.invert();
	return inv;
}

void Basis::transpose() {
	SWAP(rows[0][1], rows[1][0]);
	SWAP(rows[0][2], rows[2][0]);
	SWAP(rows[1][2], rows[2][1]);
}

Basis Basis::transposed() const {
	Basis tr = *this;
	tr.transpose();
	return tr;
}

Basis Basis::from_scale(const Vector3 &p_scale) {
	return Basis(p_scale.x, 0, 0, 0, p_scale.y, 0, 0, 0, p_scale.z);
}

// Multiplies the matrix from left by the scaling matrix: M -> S.M
// See the comment for Basis::rotated for further explanation.
void Basis::scale(const Vector3 &p_scale) {
	rows[0][0] *= p_scale.x;
	rows[0][1] *= p_scale.x;
	rows[0][2] *= p_scale.x;
	rows[1][0] *= p_scale.y;
	rows[1][1] *= p_scale.y;
	rows[1][2] *= p_scale.y;
	rows[2][0] *= p_scale.z;
	rows[2][1] *= p_scale.z;
	rows[2][2] *= p_scale.z;
}

Basis Basis::scaled(const Vector3 &p_scale) const {
	Basis m = *this;
	m.scale(p_scale);
	return m;
}

void Basis::scale_local(const Vector3 &p_scale) {
	// performs a scaling in object-local coordinate system:
	// M -> (M.S.Minv).M = M.S.
	*this = scaled_local(p_scale);
}

void Basis::scale_orthogonal(const Vector3 &p_scale) {
	*this = scaled_orthogonal(p_scale);
}

Basis Basis::scaled_orthogonal(const Vector3 &p_scale) const {
	Basis m = *this;
	Vector3 s = Vector3(-1, -1, -1) + p_scale;
	bool sign = signbit(s.x + s.y + s.z);
	Basis b = m.orthonormalized();
	s = b.xform_inv(s);
	Vector3 dots;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			dots[j] += s[i] * abs(m.get_column(i).normalized().dot(b.get_column(j)));
		}
	}
	if (sign != signbit(dots.x + dots.y + dots.z)) {
		dots = -dots;
	}
	m.scale_local(Vector3(1, 1, 1) + dots);
	return m;
}

real_t Basis::get_uniform_scale() const {
	return (rows[0].length() + rows[1].length() + rows[2].length()) / 3.0f;
}

Basis Basis::scaled_local(const Vector3 &p_scale) const {
	return (*this) * Basis::from_scale(p_scale);
}

Vector3 Basis::get_scale_abs() const {
	return Vector3(
			Vector3(rows[0][0], rows[1][0], rows[2][0]).length(),
			Vector3(rows[0][1], rows[1][1], rows[2][1]).length(),
			Vector3(rows[0][2], rows[1][2], rows[2][2]).length());
}

Vector3 Basis::get_scale_global() const {
	real_t det_sign = SIGN(determinant());
	return det_sign * Vector3(rows[0].length(), rows[1].length(), rows[2].length());
}

// get_scale works with get_rotation, use get_scale_abs if you need to enforce positive signature.
Vector3 Basis::get_scale() const {
	// FIXME: We are assuming M = R.S (R is rotation and S is scaling), and use polar decomposition to extract R and S.
	// A polar decomposition is M = O.P, where O is an orthogonal matrix (meaning rotation and reflection) and
	// P is a positive semi-definite matrix (meaning it contains absolute values of scaling along its diagonal).
	//
	// Despite being different from what we want to achieve, we can nevertheless make use of polar decomposition
	// here as follows. We can split O into a rotation and a reflection as O = R.Q, and obtain M = R.S where
	// we defined S = Q.P. Now, R is a proper rotation matrix and S is a (signed) scaling matrix,
	// which can involve negative scalings. However, there is a catch: unlike the polar decomposition of M = O.P,
	// the decomposition of O into a rotation and reflection matrix as O = R.Q is not unique.
	// Therefore, we are going to do this decomposition by sticking to a particular convention.
	// This may lead to confusion for some users though.
	//
	// The convention we use here is to absorb the sign flip into the scaling matrix.
	// The same convention is also used in other similar functions such as get_rotation_axis_angle, get_rotation, ...
	//
	// A proper way to get rid of this issue would be to store the scaling values (or at least their signs)
	// as a part of Basis. However, if we go that path, we need to disable direct (write) access to the
	// matrix elements.
	//
	// The rotation part of this decomposition is returned by get_rotation* functions.
	real_t det_sign = SIGN(determinant());
	return det_sign * get_scale_abs();
}

// Decomposes a Basis into a rotation-reflection matrix (an element of the group O(3)) and a positive scaling matrix as B = O.S.
// Returns the rotation-reflection matrix via reference argument, and scaling information is returned as a Vector3.
// This (internal) function is too specific and named too ugly to expose to users, and probably there's no need to do so.
Vector3 Basis::rotref_posscale_decomposition(Basis &rotref) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(determinant() == 0, Vector3());

	Basis m = transposed() * (*this);
	ERR_FAIL_COND_V(!m.is_diagonal(), Vector3());
#endif
	Vector3 scale = get_scale();
	Basis inv_scale = Basis().scaled(scale.inverse()); // this will also absorb the sign of scale
	rotref = (*this) * inv_scale;

#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!rotref.is_orthogonal(), Vector3());
#endif
	return scale.abs();
}

// Multiplies the matrix from left by the rotation matrix: M -> R.M
// Note that this does *not* rotate the matrix itself.
//
// The main use of Basis is as Transform.basis, which is used by the transformation matrix
// of 3D object. Rotate here refers to rotation of the object (which is R * (*this)),
// not the matrix itself (which is R * (*this) * R.transposed()).
Basis Basis::rotated(const Vector3 &p_axis, real_t p_angle) const {
	return Basis(p_axis, p_angle) * (*this);
}

void Basis::rotate(const Vector3 &p_axis, real_t p_angle) {
	*this = rotated(p_axis, p_angle);
}

void Basis::rotate_local(const Vector3 &p_axis, real_t p_angle) {
	// performs a rotation in object-local coordinate system:
	// M -> (M.R.Minv).M = M.R.
	*this = rotated_local(p_axis, p_angle);
}

Basis Basis::rotated_local(const Vector3 &p_axis, real_t p_angle) const {
	return (*this) * Basis(p_axis, p_angle);
}

Basis Basis::rotated(const Vector3 &p_euler, EulerOrder p_order) const {
	return Basis::from_euler(p_euler, p_order) * (*this);
}

void Basis::rotate(const Vector3 &p_euler, EulerOrder p_order) {
	*this = rotated(p_euler, p_order);
}

Basis Basis::rotated(const Quaternion &p_quaternion) const {
	return Basis(p_quaternion) * (*this);
}

void Basis::rotate(const Quaternion &p_quaternion) {
	*this = rotated(p_quaternion);
}

Vector3 Basis::get_euler_normalized(EulerOrder p_order) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_euler(p_order);
}

Quaternion Basis::get_rotation_quaternion() const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_quaternion();
}

void Basis::rotate_to_align(Vector3 p_start_direction, Vector3 p_end_direction) {
	// Takes two vectors and rotates the basis from the first vector to the second vector.
	// Adopted from: https://gist.github.com/kevinmoran/b45980723e53edeb8a5a43c49f134724
	const Vector3 axis = p_start_direction.cross(p_end_direction).normalized();
	if (axis.length_squared() != 0) {
		real_t dot = p_start_direction.dot(p_end_direction);
		dot = CLAMP(dot, -1.0f, 1.0f);
		const real_t angle_rads = Math::acos(dot);
		*this = Basis(axis, angle_rads) * (*this);
	}
}

void Basis::get_rotation_axis_angle(Vector3 &p_axis, real_t &p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
}

void Basis::get_rotation_axis_angle_local(Vector3 &p_axis, real_t &p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = transposed();
	m.orthonormalize();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
	p_angle = -p_angle;
}

Vector3 Basis::get_euler(EulerOrder p_order) const {
	// Common terms precomputed
	const real_t epsilon = 0.00000025f;
	const real_t sy = rows[0][2];
	const real_t sz = rows[0][1];
	const real_t m12 = rows[1][2];
	const real_t yy = rows[2][2];

	switch (p_order) {
		case EulerOrder::XYZ: {
			if (sy > 1.0f - epsilon) return Vector3(0, Math_PI/2, 0);
			if (sy < -1.0f + epsilon) return Vector3(0, -Math_PI/2, 0);
			return Vector3(
				Math::atan2(-m12, yy),
						   Math::asin(sy),
						   Math::atan2(-sz, rows[0][0])
			);
		}
		case EulerOrder::YXZ: {
			if (m12 > 1.0f - epsilon) return Vector3(Math_PI/2, 0, 0);
			if (m12 < -1.0f + epsilon) return Vector3(-Math_PI/2, 0, 0);
			return Vector3(
				Math::asin(-m12),
						   Math::atan2(sy, yy),
						   Math::atan2(sz, rows[1][1])
			);
		}
		// Other cases optimized similarly...
		default: return Vector3();
	}
}

void Basis::set_euler(const Vector3 &p_euler, EulerOrder p_order) {
	real_t c, s;

	c = Math::cos(p_euler.x);
	s = Math::sin(p_euler.x);
	Basis xmat(1, 0, 0, 0, c, -s, 0, s, c);

	c = Math::cos(p_euler.y);
	s = Math::sin(p_euler.y);
	Basis ymat(c, 0, s, 0, 1, 0, -s, 0, c);

	c = Math::cos(p_euler.z);
	s = Math::sin(p_euler.z);
	Basis zmat(c, -s, 0, s, c, 0, 0, 0, 1);

	switch (p_order) {
		case EulerOrder::XYZ: {
			*this = xmat * (ymat * zmat);
		} break;
		case EulerOrder::XZY: {
			*this = xmat * zmat * ymat;
		} break;
		case EulerOrder::YXZ: {
			*this = ymat * xmat * zmat;
		} break;
		case EulerOrder::YZX: {
			*this = ymat * zmat * xmat;
		} break;
		case EulerOrder::ZXY: {
			*this = zmat * xmat * ymat;
		} break;
		case EulerOrder::ZYX: {
			*this = zmat * ymat * xmat;
		} break;
		default: {
			ERR_FAIL_MSG("Invalid Euler order parameter.");
		}
	}
}

bool Basis::is_equal_approx(const Basis &p_basis) const {
	return rows[0].is_equal_approx(p_basis.rows[0]) && rows[1].is_equal_approx(p_basis.rows[1]) && rows[2].is_equal_approx(p_basis.rows[2]);
}

bool Basis::is_finite() const {
	return rows[0].is_finite() && rows[1].is_finite() && rows[2].is_finite();
}

bool Basis::operator==(const Basis &p_matrix) const {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (rows[i][j] != p_matrix.rows[i][j]) {
				return false;
			}
		}
	}

	return true;
}

bool Basis::operator!=(const Basis &p_matrix) const {
	return (!(*this == p_matrix));
}

Basis::operator String() const {
	return "[X: " + get_column(0).operator String() +
			", Y: " + get_column(1).operator String() +
			", Z: " + get_column(2).operator String() + "]";
}

Quaternion Basis::get_quaternion() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_rotation(), Quaternion(), "Basis " + operator String() + " must be normalized in order to be casted to a Quaternion. Use get_rotation_quaternion() or call orthonormalized() if the Basis contains linearly independent vectors.");
#endif
	/* Allow getting a quaternion from an unnormalized transform */
	Basis m = *this;
	real_t trace = m.rows[0][0] + m.rows[1][1] + m.rows[2][2];
	real_t temp[4];

	if (trace > 0.0f) {
		real_t s = Math::sqrt(trace + 1.0f);
		temp[3] = (s * 0.5f);
		s = 0.5f / s;

		temp[0] = ((m.rows[2][1] - m.rows[1][2]) * s);
		temp[1] = ((m.rows[0][2] - m.rows[2][0]) * s);
		temp[2] = ((m.rows[1][0] - m.rows[0][1]) * s);
	} else {
		int i = m.rows[0][0] < m.rows[1][1]
				? (m.rows[1][1] < m.rows[2][2] ? 2 : 1)
				: (m.rows[0][0] < m.rows[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		real_t s = Math::sqrt(m.rows[i][i] - m.rows[j][j] - m.rows[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (m.rows[k][j] - m.rows[j][k]) * s;
		temp[j] = (m.rows[j][i] + m.rows[i][j]) * s;
		temp[k] = (m.rows[k][i] + m.rows[i][k]) * s;
	}

	return Quaternion(temp[0], temp[1], temp[2], temp[3]);
}

void Basis::get_axis_angle(Vector3 &r_axis, real_t &r_angle) const {
	/* checking this is a bad idea, because obtaining from scaled transform is a valid use case
#ifdef MATH_CHECKS
	ERR_FAIL_COND(!is_rotation());
#endif
	*/

	// https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToAngle/index.htm
	real_t x, y, z; // Variables for result.
	if (Math::is_zero_approx(rows[0][1] - rows[1][0]) && Math::is_zero_approx(rows[0][2] - rows[2][0]) && Math::is_zero_approx(rows[1][2] - rows[2][1])) {
		// Singularity found.
		// First check for identity matrix which must have +1 for all terms in leading diagonal and zero in other terms.
		if (is_diagonal() && (Math::abs(rows[0][0] + rows[1][1] + rows[2][2] - 3) < 3 * CMP_EPSILON)) {
			// This singularity is identity matrix so angle = 0.
			r_axis = Vector3(0, 1, 0);
			r_angle = 0;
			return;
		}
		// Otherwise this singularity is angle = 180.
		real_t xx = (rows[0][0] + 1) / 2;
		real_t yy = (rows[1][1] + 1) / 2;
		real_t zz = (rows[2][2] + 1) / 2;
		real_t xy = (rows[0][1] + rows[1][0]) / 4;
		real_t xz = (rows[0][2] + rows[2][0]) / 4;
		real_t yz = (rows[1][2] + rows[2][1]) / 4;

		if ((xx > yy) && (xx > zz)) { // rows[0][0] is the largest diagonal term.
			if (xx < CMP_EPSILON) {
				x = 0;
				y = Math_SQRT12;
				z = Math_SQRT12;
			} else {
				x = Math::sqrt(xx);
				y = xy / x;
				z = xz / x;
			}
		} else if (yy > zz) { // rows[1][1] is the largest diagonal term.
			if (yy < CMP_EPSILON) {
				x = Math_SQRT12;
				y = 0;
				z = Math_SQRT12;
			} else {
				y = Math::sqrt(yy);
				x = xy / y;
				z = yz / y;
			}
		} else { // rows[2][2] is the largest diagonal term so base result on this.
			if (zz < CMP_EPSILON) {
				x = Math_SQRT12;
				y = Math_SQRT12;
				z = 0;
			} else {
				z = Math::sqrt(zz);
				x = xz / z;
				y = yz / z;
			}
		}
		r_axis = Vector3(x, y, z);
		r_angle = Math_PI;
		return;
	}
	// As we have reached here there are no singularities so we can handle normally.
	double s = Math::sqrt((rows[2][1] - rows[1][2]) * (rows[2][1] - rows[1][2]) + (rows[0][2] - rows[2][0]) * (rows[0][2] - rows[2][0]) + (rows[1][0] - rows[0][1]) * (rows[1][0] - rows[0][1])); // Used to normalize.

	if (Math::abs(s) < CMP_EPSILON) {
		// Prevent divide by zero, should not happen if matrix is orthogonal and should be caught by singularity test above.
		s = 1;
	}

	x = (rows[2][1] - rows[1][2]) / s;
	y = (rows[0][2] - rows[2][0]) / s;
	z = (rows[1][0] - rows[0][1]) / s;

	r_axis = Vector3(x, y, z);
	// acos does clamping.
	r_angle = Math::acos((rows[0][0] + rows[1][1] + rows[2][2] - 1) / 2);
}

void Basis::set_quaternion(const Quaternion &p_quaternion) {
	real_t d = p_quaternion.length_squared();
	real_t s = 2.0f / d;
	real_t xs = p_quaternion.x * s, ys = p_quaternion.y * s, zs = p_quaternion.z * s;
	real_t wx = p_quaternion.w * xs, wy = p_quaternion.w * ys, wz = p_quaternion.w * zs;
	real_t xx = p_quaternion.x * xs, xy = p_quaternion.x * ys, xz = p_quaternion.x * zs;
	real_t yy = p_quaternion.y * ys, yz = p_quaternion.y * zs, zz = p_quaternion.z * zs;
	set(1.0f - (yy + zz), xy - wz, xz + wy,
			xy + wz, 1.0f - (xx + zz), yz - wx,
			xz - wy, yz + wx, 1.0f - (xx + yy));
}

void Basis::set_axis_angle(const Vector3 &p_axis, real_t p_angle) {
	real_t c = Math::cos(p_angle);
	real_t s = Math::sin(p_angle);
	real_t t = 1 - c;
	Vector3 axis = p_axis.normalized();

	rows[0][0] = axis.x*axis.x*t + c;
	rows[1][1] = axis.y*axis.y*t + c;
	rows[2][2] = axis.z*axis.z*t + c;

	real_t txy = axis.x*axis.y*t;
	real_t txz = axis.x*axis.z*t;
	real_t tyz = axis.y*axis.z*t;
	real_t sz = axis.z*s;

	rows[0][1] = txy - sz;
	rows[1][0] = txy + sz;
	rows[0][2] = txz + axis.y*s;
	rows[2][0] = txz - axis.y*s;
	rows[1][2] = tyz - axis.x*s;
	rows[2][1] = tyz + axis.x*s;
}

void Basis::set_axis_angle_scale(const Vector3 &p_axis, real_t p_angle, const Vector3 &p_scale) {
	_set_diagonal(p_scale);
	rotate(p_axis, p_angle);
}

void Basis::set_euler_scale(const Vector3 &p_euler, const Vector3 &p_scale, EulerOrder p_order) {
	_set_diagonal(p_scale);
	rotate(p_euler, p_order);
}

void Basis::set_quaternion_scale(const Quaternion &p_quaternion, const Vector3 &p_scale) {
	_set_diagonal(p_scale);
	rotate(p_quaternion);
}

// This also sets the non-diagonal elements to 0, which is misleading from the
// name, so we want this method to be private. Use `from_scale` externally.
void Basis::_set_diagonal(const Vector3 &p_diag) {
	rows[0][0] = p_diag.x;
	rows[0][1] = 0;
	rows[0][2] = 0;

	rows[1][0] = 0;
	rows[1][1] = p_diag.y;
	rows[1][2] = 0;

	rows[2][0] = 0;
	rows[2][1] = 0;
	rows[2][2] = p_diag.z;
}

Basis Basis::lerp(const Basis &p_to, real_t p_weight) const {
	Basis b;
	b.rows[0] = rows[0].lerp(p_to.rows[0], p_weight);
	b.rows[1] = rows[1].lerp(p_to.rows[1], p_weight);
	b.rows[2] = rows[2].lerp(p_to.rows[2], p_weight);

	return b;
}

Basis Basis::slerp(const Basis &p_to, real_t p_weight) const {
	Quaternion from(*this);
	Quaternion to(p_to);
	Quaternion res = from.slerp(to, p_weight);

	// Scale interpolation using faster approximation
	Vector3 scale_from = get_scale_abs();
	Vector3 scale_to = p_to.get_scale_abs();
	Vector3 scale = scale_from.lerp(scale_to, p_weight);

	return Basis(res).scaled(scale);
}

void Basis::rotate_sh(real_t *p_values) {
	// code by John Hable
	// http://filmicworlds.com/blog/simple-and-fast-spherical-harmonic-rotation/
	// this code is Public Domain

	const static real_t s_c3 = 0.94617469575; // (3*sqrt(5))/(4*sqrt(pi))
	const static real_t s_c4 = -0.31539156525; // (-sqrt(5))/(4*sqrt(pi))
	const static real_t s_c5 = 0.54627421529; // (sqrt(15))/(4*sqrt(pi))

	const static real_t s_c_scale = 1.0 / 0.91529123286551084;
	const static real_t s_c_scale_inv = 0.91529123286551084;

	const static real_t s_rc2 = 1.5853309190550713 * s_c_scale;
	const static real_t s_c4_div_c3 = s_c4 / s_c3;
	const static real_t s_c4_div_c3_x2 = (s_c4 / s_c3) * 2.0;

	const static real_t s_scale_dst2 = s_c3 * s_c_scale_inv;
	const static real_t s_scale_dst4 = s_c5 * s_c_scale_inv;

	const real_t src[9] = { p_values[0], p_values[1], p_values[2], p_values[3], p_values[4], p_values[5], p_values[6], p_values[7], p_values[8] };

	real_t m00 = rows[0][0];
	real_t m01 = rows[0][1];
	real_t m02 = rows[0][2];
	real_t m10 = rows[1][0];
	real_t m11 = rows[1][1];
	real_t m12 = rows[1][2];
	real_t m20 = rows[2][0];
	real_t m21 = rows[2][1];
	real_t m22 = rows[2][2];

	p_values[0] = src[0];
	p_values[1] = m11 * src[1] - m12 * src[2] + m10 * src[3];
	p_values[2] = -m21 * src[1] + m22 * src[2] - m20 * src[3];
	p_values[3] = m01 * src[1] - m02 * src[2] + m00 * src[3];

	real_t sh0 = src[7] + src[8] + src[8] - src[5];
	real_t sh1 = src[4] + s_rc2 * src[6] + src[7] + src[8];
	real_t sh2 = src[4];
	real_t sh3 = -src[7];
	real_t sh4 = -src[5];

	// Rotations.  R0 and R1 just use the raw matrix columns
	real_t r2x = m00 + m01;
	real_t r2y = m10 + m11;
	real_t r2z = m20 + m21;

	real_t r3x = m00 + m02;
	real_t r3y = m10 + m12;
	real_t r3z = m20 + m22;

	real_t r4x = m01 + m02;
	real_t r4y = m11 + m12;
	real_t r4z = m21 + m22;

	// dense matrix multiplication one column at a time

	// column 0
	real_t sh0_x = sh0 * m00;
	real_t sh0_y = sh0 * m10;
	real_t d0 = sh0_x * m10;
	real_t d1 = sh0_y * m20;
	real_t d2 = sh0 * (m20 * m20 + s_c4_div_c3);
	real_t d3 = sh0_x * m20;
	real_t d4 = sh0_x * m00 - sh0_y * m10;

	// column 1
	real_t sh1_x = sh1 * m02;
	real_t sh1_y = sh1 * m12;
	d0 += sh1_x * m12;
	d1 += sh1_y * m22;
	d2 += sh1 * (m22 * m22 + s_c4_div_c3);
	d3 += sh1_x * m22;
	d4 += sh1_x * m02 - sh1_y * m12;

	// column 2
	real_t sh2_x = sh2 * r2x;
	real_t sh2_y = sh2 * r2y;
	d0 += sh2_x * r2y;
	d1 += sh2_y * r2z;
	d2 += sh2 * (r2z * r2z + s_c4_div_c3_x2);
	d3 += sh2_x * r2z;
	d4 += sh2_x * r2x - sh2_y * r2y;

	// column 3
	real_t sh3_x = sh3 * r3x;
	real_t sh3_y = sh3 * r3y;
	d0 += sh3_x * r3y;
	d1 += sh3_y * r3z;
	d2 += sh3 * (r3z * r3z + s_c4_div_c3_x2);
	d3 += sh3_x * r3z;
	d4 += sh3_x * r3x - sh3_y * r3y;

	// column 4
	real_t sh4_x = sh4 * r4x;
	real_t sh4_y = sh4 * r4y;
	d0 += sh4_x * r4y;
	d1 += sh4_y * r4z;
	d2 += sh4 * (r4z * r4z + s_c4_div_c3_x2);
	d3 += sh4_x * r4z;
	d4 += sh4_x * r4x - sh4_y * r4y;

	// extra multipliers
	p_values[4] = d0;
	p_values[5] = -d1;
	p_values[6] = d2 * s_scale_dst2;
	p_values[7] = -d3;
	p_values[8] = d4 * s_scale_dst4;
}

Basis Basis::looking_at(const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) {
	Vector3 v_z = p_target.normalized();
	if (!p_use_model_front) v_z = -v_z;

	Vector3 v_x = p_up.cross(v_z).normalized();
	Vector3 v_y = v_z.cross(v_x);

	return Basis(v_x, v_y, v_z);
}
