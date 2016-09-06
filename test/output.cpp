
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
        }}
        AND_WHEN ("Open for append") {
            LowIO::handle_t h { LowIO::open ( path.string ()
                                            , ( LowIO::OpenFlags::CREATE
                                              | LowIO::OpenFlags::APPEND
                                              | LowIO::OpenFlags::READ_WRITE)
                                            , 0666) } ;
        THEN ("Should success") {
            REQUIRE (h.valid ()) ;
        AND_WHEN ("Append \"Hello World\".") {
            REQUIRE (h.write (hello.c_str (), hello.size ())) ;
            REQUIRE (h.close ()) ;
        THEN ("File contains \"Hello World!HelloWorld!\"") {
            std::ifstream in { path.string (), std::ios::in | std::ios::binary } ;
            REQUIRE (in.good ()) ;
            char    tmp [64] ;
            in.read (tmp, sizeof (tmp)) ;
            auto sz = in.gcount () ;
            REQUIRE (sz == 2 * hello.size ()) ;
            std::string actual { tmp, tmp + sz } ;
            std::string expected { hello + hello } ;
            REQUIRE (expected == actual) ;
        }}}}
    }}}}}}
}
