/*
 * lowio.cpp:
 *
 * Copyright (c) 2016 Masashi Fujita
 */

#ifdef HAVE_CONFIG_HPP
#   include "config.hpp"
#endif

#include "lowio/lowio.hpp"

#ifdef HAVE_SYS_STAT_H
#   include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
#   include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif

#if ! (defined (_WIN64) || defined (_WIN32))

namespace LowIO {
    native_handle_t open (const std::string &path, uint32_t flags, uint32_t mode) {
        auto result = ::open (path.c_str (), static_cast<int> (flags), mode) ;
        if (result < 0) {
            return BAD_HANDLE ;
        }
        return static_cast<native_handle_t> (result) ;
    }

    result_t close (native_handle_t &&h) {
        if (valid_handle_p (h)) {

            native_handle_t H = BAD_HANDLE;
            std::swap (H, h);
            if (::close (H) != 0) {
                return { ErrorCode::CLOSE_FAILED, std::string { "Close failed." } };
            }
        }
        return {} ;
    }

    result_t_<size_t>   handle_t::seek (int64_t offset, SeekOrigin origin) {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, std::string { "Invalid handle for seek." } } ;
        }
        int64_t   off ;
        switch (origin) {
        case SeekOrigin::BEGIN :
            off = ::lseek (value_, static_cast <int64_t> (offset), SEEK_SET) ;
            break ;
        case SeekOrigin::CURRENT:
            off = ::lseek (value_, static_cast <int64_t> (offset), SEEK_CUR) ;
            break ;
        case SeekOrigin::END:
            off = ::lseek (value_, static_cast <int64_t> (offset), SEEK_END) ;
            break ;
        }
        if (off == -1L) {
            return { ErrorCode::SEEK_FAILED, "Seek failed." } ;
        }
        return result_t_<size_t> { static_cast<size_t> (off) } ;
    }


    result_t_<size_t>   handle_t::read (void *data, size_t size) {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, std::string { "Invalid handle for read." } } ;
        }
        long  n = ::read (value_, data, size) ;
        if (n < 0) {
            return { ErrorCode::READ_FAILED, std::string { "Read failed." } } ;
        }
        auto sz_read = static_cast <size_t> (n) ;
        return result_t_<size_t> { static_cast<size_t> (sz_read) } ;
    }

    result_t	handle_t::write (const void *data, size_t size) {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, "Invalid handle for write." } ;
        }
        // It's redundant to UN?X like systems...
        const size_t  MAXIMUM_SEGMENT_SIZE = 32 * 1024 * 1024 ;

        size_t    sz_written = 0 ;

        while (sz_written < size) {
            size_t    sz = size - sz_written ;
            if (MAXIMUM_SEGMENT_SIZE < sz) {
                sz = MAXIMUM_SEGMENT_SIZE ;
            }
            long  cnt = ::write (value_, static_cast<const char *> (data) + sz_written, sz) ;
            if (cnt < 0 || static_cast<size_t> (cnt) != sz) {
                return { ErrorCode::WRITE_FAILED, std::string { "Write failed." } } ;
            }
            sz_written += sz ;
        }
        return {} ;
    }

    result_t	handle_t::truncate () {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, "Invalid handle for truncate." } ;
        }
        if (::write (value_, "", 0) < 0) {
            return { ErrorCode::TRUNCATE_FAILED, "Truncate failed." } ;
        }
        return {} ;
    }

    result_t	handle_t::duplicate (native_handle_t h) {
        int   fd = dup (h) ;
        if (fd < 0) {
            return { ErrorCode::DUPLICATE_FAILED, "Duplicate handle failed." } ;
        }
        return this->attach (fd) ;
    }
}

#endif  /* ! (_WIN64 || _WIN32) */
