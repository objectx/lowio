#include <catch.hpp>
#include <string>

#include <lowio/lowio.hpp>

#if defined (_WIN64) || defined (_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "fixture.hpp"

namespace FS = boost::filesystem ;

TEST_CASE_METHOD (Fixture::DirectoryFixture, "Test: LowIO::open", "[openclose]") {
    GIVEN ("A Path") {
        auto path = makeTestPath (FS::unique_path ("tmp-%%%%%%%%%%%%%%%%.txt")) ;
    WHEN ("Create") {
        auto h = LowIO::open ( path.string ()
                             , ( LowIO::OpenFlags::CREATE
                               | LowIO::OpenFlags::WRITE_ONLY
                               | LowIO::OpenFlags::TRUNCATE), 0666) ;
    THEN ("Should success.") {
        CAPTURE (h) ;
        CAPTURE (path.string ()) ;
        REQUIRE (LowIO::valid_handle_p (h)) ;
    AND_WHEN ("Close it") {
        auto r = LowIO::close (std::move (h)) ;
    THEN ("Should success") {
        REQUIRE (r.getErrorCode () == LowIO::ErrorCode::SUCCESS) ;
    AND_THEN ("Handle should be invalidated") {
        CAPTURE (h) ;
        REQUIRE (! LowIO::valid_handle_p (h)) ;
    }}}}}}
}
