//
// Created by Masashi Fujita on 2016/09/06.
// Copyright (c) 2016 Polyphony Digital Inc. All rights reserved.
//
#include <catch.hpp>

#include <lowio/lowio.hpp>

SCENARIO ("Test `parse_flags`", "[parse_flags]") {
    GIVEN ("mode = \"?\"") {
        const std::string mode { "?" };
    WHEN ("parsing") {
        auto r = LowIO::parse_flags (mode) ;
    THEN ("Should fail") {
        REQUIRE_FALSE (r) ;
    AND_THEN ("Should return LowIO::ErrorCodes::BAD_PARAMETER") {
        REQUIRE (r.getErrorCode () == LowIO::ErrorCode::BAD_PARAMETER) ;
    }}}}

    GIVEN ("mode = \"r\"") {
        const std::string mode { "r" } ;
    WHEN ("parsing") {
        auto r = LowIO::parse_flags (mode) ;
    THEN ("Should success") {
        REQUIRE (r) ;
    AND_THEN ("flags should be `OpenFlags::READ_ONLY`") {
        auto flag = LowIO::OpenFlags::READ_ONLY ;
        REQUIRE (r.getValue () == flag) ;
    }}}}

    GIVEN ("mode = \"w\"") {
        const std::string mode { "w" } ;
    WHEN ("parsing") {
        auto r = LowIO::parse_flags (mode) ;
    THEN ("Should success") {
        REQUIRE (r) ;
    AND_THEN ("flags should be `OpenFlags::WRITE_ONLY`") {
        auto flag = LowIO::OpenFlags::WRITE_ONLY | LowIO::OpenFlags::CREATE | LowIO::OpenFlags::TRUNCATE ;
        REQUIRE (r.getValue () == flag) ;
    }}}}

    GIVEN ("mode = \"r+\"") {
        const std::string mode { "r+" } ;
    WHEN ("parsing") {
        auto r = LowIO::parse_flags (mode) ;
    THEN ("Should success") {
        REQUIRE (r) ;
    AND_THEN ("flags should be `OpenFlags::READ_ONLY`") {
        auto flag = LowIO::OpenFlags::READ_WRITE ;
        REQUIRE (r.getValue () == flag) ;
    }}}}

    GIVEN ("mode = \"w+\"") {
        const std::string mode { "w+" } ;
    WHEN ("parsing") {
        auto r = LowIO::parse_flags (mode) ;
    THEN ("Should success") {
        REQUIRE (r) ;
    AND_THEN ("flags should be `OpenFlags::WRITE_ONLY`") {
        auto flag = LowIO::OpenFlags::READ_WRITE | LowIO::OpenFlags::CREATE | LowIO::OpenFlags::TRUNCATE ;
        REQUIRE (r.getValue () == flag) ;
    }}}}
}
