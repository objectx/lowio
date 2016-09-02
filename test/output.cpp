
#include <catch.hpp>

#include <iostream>

#include "fixture.hpp"

#include <lowio/lowio.hpp>

namespace FS = boost::filesystem ;

TEST_CASE_METHOD (Fixture::DirectoryFixture, "Test: LowIO::Output", "[output]") {
    GIVEN ("A path") {
        auto path = makeTestPath (FS::unique_path ("tmp-%%%%%%%%%%%%%%%%.txt")) ;
    WHEN ("Create output") {
        LowIO::Output   output { path.string () } ;
    THEN ("Should be valid.") {
        REQUIRE (output.valid ()) ;
    AND_WHEN ("Write \"Hello World!\"") {
        const std::string hello { "Hello World!" } ;
        auto r = output.write (hello.data (), hello.size ()) ;
    THEN ("Should success") {
        REQUIRE (!!r) ;
    AND_WHEN ("Close should success") {
        auto cr = output.close () ;
        REQUIRE (!!cr) ;
    AND_WHEN ("Read contents") {
        std::ifstream   in { path.string (), std::ios::in | std::ios::binary } ;
        REQUIRE (in.good ()) ;
        char    tmp [64] ;
        in.read (tmp, sizeof (tmp)) ;
        auto sz = in.gcount () ;
    THEN ("Contents should match") {
        REQUIRE (sz == hello.size ()) ;
        REQUIRE (memcmp (hello.data (), tmp, hello.size ()) == 0) ;
    }}}}}}}}
}
