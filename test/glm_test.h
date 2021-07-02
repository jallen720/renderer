#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ctk/math.h"

static bool use_glm = true;

namespace test {

union Matrix {
    ctk::Matrix ctk;
    glm::mat4 glm;
    f32 data[16];

    f32 *operator[](u32 row) { return data + (row * 4); }
};

union Vec3 {
    ctk::Vec3<f32> ctk;
    glm::vec3 glm;
    struct { f32 x, y, z; };

    Vec3 &operator+=(const Vec3 &r);
};

static Matrix default_matrix() {
    if (use_glm)
        return { .glm = glm::mat4(1.0f) };
    else
        return { .ctk = ctk::MATRIX_ID };
}

static Matrix operator*(Matrix &l, Matrix &r) {
    if (use_glm)
        return { .glm = l.glm * r.glm };
    else
        return { .ctk = l.ctk * r.ctk };
}

Matrix perspective_matrix(PerspectiveInfo info) {
    if (use_glm)
        return { .glm = glm::perspective(glm::radians(info.vertical_fov), info.aspect, info.z_near, info.z_far) };
    else
        return { .ctk = ctk::perspective_matrix(info) };
}

Matrix rotate(Matrix matrix, f32 degrees, Axis axis) {
    if (use_glm) {
        if (axis == Axis::X) return { .glm = glm::rotate(matrix.glm, glm::radians(degrees), { 1, 0, 0 }) };
        if (axis == Axis::Y) return { .glm = glm::rotate(matrix.glm, glm::radians(degrees), { 0, 1, 0 }) };
        if (axis == Axis::Z) return { .glm = glm::rotate(matrix.glm, glm::radians(degrees), { 0, 0, 1 }) };
    }
    else {
        return { .ctk = ctk::rotate(matrix.ctk, degrees, axis) };
    }
}

Matrix translate(Matrix matrix, Vec3 translation) {
    if (use_glm)
        return { .glm = glm::translate(matrix.glm, translation.glm) };
    else
        return { .ctk = ctk::translate(matrix.ctk, translation.ctk) };
}

Matrix look_at(Vec3 position, Vec3 point, Vec3 up) {
    if (use_glm)
        return { .glm = glm::lookAt(position.glm, point.glm, up.glm) };
    else
        return { .ctk = ctk::look_at(position.ctk, point.ctk, up.ctk) };
}

static Vec3 operator*(f32 r, const Vec3 &l) {
    return {
        l.x * r,
        l.y * r,
        l.z * r,
    };
}

static Vec3 operator+(const Vec3 &l, const Vec3 &r) {
    return {
        l.x + r.x,
        l.y + r.y,
        l.z + r.z,
    };
}

Vec3 &Vec3::operator+=(const Vec3 &r) {
    this->x += r.x;
    this->y += r.y;
    this->z += r.z;
    return *this;
}

}
