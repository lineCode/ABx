#include "stdafx.h"
#include <catch.hpp>

#include "BoundingBox.h"

TEST_CASE("BoundingBox Basic", "[boundingbox]")
{
    SECTION("Size")
    {
        Math::BoundingBox bb(-2.0f, 2.0f);
        Math::Vector3 size = bb.Size();
        REQUIRE(size.x_ == 4.0f);
        REQUIRE(size.y_ == 4.0f);
        REQUIRE(size.z_ == 4.0f);
    }
    SECTION("Center")
    {
        Math::BoundingBox bb(-2.0f, 2.0f);
        Math::Vector3 center = bb.Center();
        REQUIRE(center.x_ == 0.0f);
        REQUIRE(center.y_ == 0.0f);
        REQUIRE(center.z_ == 0.0f);
    }
    SECTION("Extends")
    {
        Math::BoundingBox bb(-2.0f, 2.0f);
        Math::Vector3 extends = bb.Extends();
        REQUIRE(extends.x_ == 2.0f);
        REQUIRE(extends.y_ == 2.0f);
        REQUIRE(extends.z_ == 2.0f);
    }

}

TEST_CASE("BoundingBox Collisions", "[boundingbox]")
{
    SECTION("BoundingBox inside")
    {
        Math::BoundingBox bb1(-2.0f, 2.0f);
        Math::BoundingBox bb2(-1.0f, 1.0f);
        Math::Vector3 move;
        REQUIRE(bb1.Collides(bb2, move));
        REQUIRE(move.x_ == 3.0f);
        REQUIRE(move.y_ == 0.0f);
        REQUIRE(move.z_ == 0.0f);
    }
    SECTION("BoundingBox outside")
    {
        Math::BoundingBox bb1(-4.0f, -1.0f);
        Math::BoundingBox bb2(0.0f, 2.0f);
        Math::Vector3 move;
        REQUIRE(!bb1.Collides(bb2, move));
        REQUIRE(move.x_ == 0.0f);
        REQUIRE(move.y_ == 0.0f);
        REQUIRE(move.z_ == 0.0f);
    }
}

TEST_CASE("XMath::BoundingBox Collisions", "[boundingbox]")
{
    SECTION("BoundingBox inside")
    {
        Math::BoundingBox bb1(-2.0f, 2.0f);
        Math::BoundingBox bb2(-1.0f, 1.0f);
        XMath::BoundingBox xbb1 = (XMath::BoundingBox)bb1;
        XMath::BoundingBox xbb2 = (XMath::BoundingBox)bb2;
        REQUIRE(xbb1.Intersects(bb2));
    }
    SECTION("BoundingBox outside")
    {
        Math::BoundingBox bb1(-4.0f, -1.0f);
        Math::BoundingBox bb2(0.0f, 2.0f);
        XMath::BoundingBox xbb1 = (XMath::BoundingBox)bb1;
        XMath::BoundingBox xbb2 = (XMath::BoundingBox)bb2;
        REQUIRE(!xbb1.Intersects(bb2));
    }
}

TEST_CASE("BoundingBox Intersections", "[boundingbox]")
{
    SECTION("Vector3 inside")
    {
        Math::BoundingBox bb1(-2.0f, 2.0f);
        Math::Vector3 inside(1.0f, 1.0f, 1.0f);
        Math::Intersection in = bb1.IsInside(inside);
        REQUIRE(in == Math::INSIDE);
    }
    SECTION("Vector3 outside")
    {
        Math::BoundingBox bb1(-2.0f, 2.0f);
        Math::Vector3 inside(3.0f, 1.0f, 1.0f);
        Math::Intersection in = bb1.IsInside(inside);
        REQUIRE(in == Math::OUTSIDE);
    }
}
