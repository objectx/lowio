/*
 * Copyright (c) 2016 Masashi Fujita
 */

#if defined (_WIN64) || defined (_WIN32)

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

namespace {
    std::string	get_last_error_message (DWORD code) {
        void *	msg = nullptr ;
        FormatMessageA ((FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS),
                        NULL,
                        code,
                        MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                        (LPTSTR)&msg,
                        0,
                        NULL) ;
        std::string	result { static_cast<char *>(msg) } ;
        LocalFree (msg) ;
        return result ;
    }

    std::string	get_last_error_message () {
        return get_last_error_message (GetLastError ()) ;
    }
}

namespace LowIO {

    native_handle_t open (const std::string &path, uint32_t flags, uint32_t mode) {
        DWORD access = 0 ;
        switch (flags & (OpenFlags::READ_ONLY | OpenFlags::WRITE_ONLY | OpenFlags::READ_WRITE)) {
        case OpenFlags::READ_ONLY:
            access = GENERIC_READ ;
            break ;
        case OpenFlags::WRITE_ONLY:
            access = GENERIC_WRITE ;
            break ;
        case OpenFlags::READ_WRITE:
            access = GENERIC_READ | GENERIC_WRITE ;
            break ;
        default:
            assert (false) ;    // Should not reached.
        }

        DWORD creation = 0 ;

        if ((flags & OpenFlags::CREATE) != 0) {
            if ((flags & OpenFlags::EXCLUDE) != 0) {
                creation |= CREATE_NEW ;
            }
            else {
                if ((flags & OpenFlags::TRUNCATE) != 0) {
                    creation |= CREATE_ALWAYS ;
                }
                else {
                    creation |= OPEN_ALWAYS ;
                }
            }
        }
        else if ((flags & OpenFlags::TRUNCATE) != 0) {
            creation |= OPEN_EXISTING | TRUNCATE_EXISTING ;
        }

        HANDLE    H = CreateFileA (path.c_str ()
                                  , access
                                  , FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE
                                  , nullptr
                                  , creation
                                  , FILE_ATTRIBUTE_NORMAL
                                  , 0) ;
        if (H == INVALID_HANDLE_VALUE) {
            auto msg = get_last_error_message () ;
            return static_cast<native_handle_t> (BAD_HANDLE) ;
        }
        return static_cast<native_handle_t> (H) ;
    }

    result_t close (native_handle_t &&h) {
        if (valid_handle_p (h)) {

            native_handle_t H = BAD_HANDLE;
            std::swap (H, h);
            if (!CloseHandle (H)) {
                return { ErrorCode::CLOSE_FAILED
                       , std::string { "Close failed (" }.append (get_last_error_message ()).append (").") } ;
            }
        }
        return {} ;
    }

    result_t_<size_t>   handle_t::seek (int64_t offset, SeekOrigin origin) {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, std::string { "Invalid handle for seek." } } ;
        }
        LARGE_INTEGER	tmp ;
        tmp.QuadPart = offset ;
        switch (origin) {
        case SeekOrigin::BEGIN:
            tmp.LowPart = ::SetFilePointer (value_, tmp.LowPart, &tmp.HighPart, FILE_BEGIN) ;
            break ;
        case SeekOrigin::CURRENT:
            tmp.LowPart = ::SetFilePointer (value_, tmp.LowPart, &tmp.HighPart, FILE_CURRENT) ;
            break ;
        case SeekOrigin::END:
            tmp.LowPart = ::SetFilePointer (value_, tmp.LowPart, &tmp.HighPart, FILE_END) ;
            break ;
        default:
            /* NOTREACHED */
            assert (false) ;
            break ;
        }
        DWORD	err = ::GetLastError () ;
        if (tmp.LowPart == 0xFFFFFFFFu && err != NO_ERROR) {
            return { ErrorCode::SEEK_FAILED
                   , std::string { "Seek failed." }.append (get_last_error_message (err)).append (").") } ;
        }
        return result_t_<size_t> { static_cast<size_t> (tmp.QuadPart) } ;
    }


    result_t_<size_t>   handle_t::read (void *data, size_t size) {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, std::string { "Invalid handle for read." } } ;
        }

        DWORD n_read = 0 ;
        if (! ReadFile (value_, data, static_cast <DWORD>(size), &n_read, NULL)) {
            return result_t_<size_t> { ErrorCode::READ_FAILED
                                     , std::string { "Read failed (" }.append (get_last_error_message ()).append (").") } ;
        }
        auto    sz_read = static_cast <size_t> (n_read) ;
        return result_t_<size_t> { static_cast<size_t> (sz_read) } ;
    }

    result_t	handle_t::write (const void *data, size_t size) {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, "Invalid handle for write." } ;
        }
        // Due to kernel restriction, Writing over 64MBytes data to Network drive cause WriteFile failure.
        const size_t  MAXIMUM_SEGMENT_SIZE = 32 * 1024 * 1024 ;

        DWORD sz_written = 0 ;

        while (sz_written < static_cast<DWORD> (size)) {
            DWORD sz = static_cast<DWORD> (size) - sz_written ;

            if (MAXIMUM_SEGMENT_SIZE < sz) {
                sz = MAXIMUM_SEGMENT_SIZE ;
            }
            DWORD	cnt = 0 ;

            if (!WriteFile (value_, static_cast<const char *> (data) + sz_written, sz, &cnt, NULL) ||
                (cnt != sz)) {
                return result_t { ErrorCode::WRITE_FAILED
                                , std::string { "Write failed (" }.append (get_last_error_message ()).append (").") } ;
            }
            sz_written += sz ;
        }
        return {} ;
    }

    result_t	handle_t::truncate () {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, "Invalid handle for truncate." } ;
        }
        if (! SetEndOfFile (value_)) {
            return result_t { ErrorCode::TRUNCATE_FAILED
                            , std::string { "Truncate failed (" }.append (get_last_error_message ()).append (").") } ;
        }
        return {} ;
    }

    result_t	handle_t::duplicate (native_handle_t h) {
        HANDLE	h_new ;
        HANDLE	h_proc = GetCurrentProcess () ;
        if (! DuplicateHandle (h_proc, h, h_proc, &h_new, 0, false, DUPLICATE_SAME_ACCESS)) {
            return { ErrorCode::DUPLICATE_FAILED
                   , std::string { "Duplicate handle failed (" }.append (get_last_error_message ()).append (").") } ;
        }
        return this->attach (h_new) ;
    }

    native_handle_t	handle_t::duplicate () {
        HANDLE	h_new ;
        HANDLE	h_proc = GetCurrentProcess () ;
        if (! DuplicateHandle (h_proc, h, h_proc, &h_new, 0, false, DUPLICATE_SAME_ACCESS)) {
            return BAD_HANDLE ;
        }
        return h_new ;
    }
}

#endif /* _WIN64 || _WIN32 */

