
#include <catch.hpp>

#include <lowio/lowio.hpp>
#include "fixture.hpp"

namespace FS = boost::filesystem ;

TEST_CASE_METHOD (Fixture::DirectoryFixture, "Test LowIO::handle_t", "[handle]") {
    GIVEN ("A path") {
        auto const &path = makeTestPath (FS::unique_path ("tmp-%%%%%%%%%%%%%%%%.txt")) ;
    WHEN ("Create a file") {
        auto h = LowIO::open ( path.string ()
                             , ( LowIO::OpenFlags::CREATE
                               | LowIO::OpenFlags::WRITE_ONLY
                               | LowIO::OpenFlags::TRUNCATE), 0666) ;
    THEN ("Should success") {
        REQUIRE (LowIO::valid_handle_p (h)) ;
    AND_WHEN ("Capture it to the `handle_t`") {
        LowIO::handle_t H { h } ;
    THEN ("Should valid and match.") {
        REQUIRE (H.valid ()) ;
        REQUIRE (H.handle () == h) ;
        AND_WHEN ("Explicitly close") {
            H.close () ;
        THEN ("Handle is invalidated") {
            REQUIRE_FALSE (H.valid ()) ;
        }}
        AND_WHEN ("Pass to another `handle_t` instance") {
            LowIO::handle_t H2 { std::move (H) } ;
        THEN ("Old handle should be invalidated.") {
            REQUIRE (H2.valid ()) ;
            REQUIRE_FALSE (H.valid ()) ;
        }}
    }}}}}
}
