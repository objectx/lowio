#pragma once
#ifndef fixture_hpp__f14dde33fe28494e9826c96b0432e3b8
#define fixture_hpp__f14dde33fe28494e9826c96b0432e3b8   1

#include <stdint.h>
#include <boost/filesystem.hpp>

namespace Fixture {
    namespace FS = boost::filesystem ;

    struct TestDirHolder {

        FS::path dir_ ;

        ~TestDirHolder () {
            FS::remove_all (dir_) ;
        }

        TestDirHolder () {
            createTestDir () ;
        }

        void createTestDir () {
            auto t = FS::temp_directory_path () /= FS::unique_path ("lowiotest-%%%%%%%%%%%%%%%%.tmp") ;
            if (!FS::create_directories (t)) {
                throw std::runtime_error { std::string { "Failed to create directory \"" }
                .append (t.string ()).append ("\".") } ;
            }
            dir_ = t ;
        }
    } ;

    struct DirectoryFixture {

        static TestDirHolder    testDir_ ;

        DirectoryFixture () {
            /* NO-OP */
        }

        FS::path    makeTestPath (const FS::path &file) {
            return FS::path { testDir_.dir_ } /= file ;
        }
    } ;
}

#endif /* fixture_hpp__f14dde33fe28494e9826c96b0432e3b8 */
