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
#if defined (_WIN32) || defined (_WIN64)
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
#endif
}

namespace LowIO {

    native_handle_t open (const std::string &path, uint32_t flags, uint32_t mode) {
#if defined (_WIN64) || defined (_WIN32)
        auto make_access = [](uint32_t f) -> DWORD {
            if ((f & OpenFlags::WRITE_ONLY) != 0) {
                return GENERIC_WRITE ;
            }
            if ((f & OpenFlags::READ_WRITE) != 0) {
                return GENERIC_WRITE | GENERIC_READ ;
            }
            return GENERIC_READ ;
        } ;
        auto make_creation = [](uint32_t f) -> DWORD {
            DWORD result = 0 ;
            if ((f & OpenFlags::CREATE) != 0) {
                if ((f & OpenFlags::EXCLUDE) != 0) {
                    result |= CREATE_NEW ;
                }
                else {
                    result |= OPEN_ALWAYS ;
                }
            }
            if ((f, OpenFlags::TRUNCATE) != 0) {
                result |= TRUNCATE_EXISTING ;
            }
            return result ;
        } ;
        HANDLE    H = CreateFileA (path.c_str ()
                                  , make_access (flags)
                                  , FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE
                                  , nullptr
                                  , make_creation (flags)
                                  , FILE_ATTRIBUTE_NORMAL
                                  , 0) ;
        if (H == INVALID_HANDLE_VALUE) {
            return static_cast<native_handle_t> (BAD_HANDLE) ;
        }
#else
        auto result = ::open (path.c_str (), static_cast<int> (flags), mode) ;
        if (result < 0) {
            return BAD_HANDLE ;
        }
        return static_cast<native_handle_t> (result) ;
#endif
    }

    result_t close (native_handle_t &&h) {
        if (valid_handle_p (h)) {

            native_handle_t H = BAD_HANDLE;
            std::swap (H, h);
#if defined (_WIN64) || defined (_WIN32)
            if (!CloseHandle (H)) {
                return { ErrorCode::CLOSE_FAILED
                       , std::string { "Close failed (" }.append (get_last_error_message ()).append (").") } ;
            }
#else
            if (::close (H) != 0) {
                return { ErrorCode::CLOSE_FAILED, std::string { "Close failed." } };
            }
#endif
        }
        return {} ;
    }

    handle_t::~handle_t () {
        this->close () ;
    }

    result_t_<size_t>   handle_t::seek (int64_t offset, SeekOrigin origin) {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, std::string { "Invalid handle for seek." } } ;
        }
#if defined (_WIN32) || defined (_WIN64)
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
#else	/* not (_WIN32 || _WIN64) */
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
#endif	/* not (_WIN32 || _WIN64) */
    }


    result_t_<size_t>   handle_t::read (void *data, size_t size) {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, std::string { "Invalid handle for read." } } ;
        }
#if defined (_WIN32) || defined (_WIN64)
        DWORD n_read = 0 ;
        if (! ReadFile (value_, data, static_cast <DWORD>(size), &n_read, NULL)) {
            return result_t_<size_t> { ErrorCode::READ_FAILED
                                     , std::string { "Read failed (" }.append (get_last_error_message ()).append (").") } ;
        }
        auto    sz_read = static_cast <size_t> (n_read) ;
#else	/* not (_WIN32 || _WIN64) */
        long  n = ::read (value_, data, size) ;
        if (n < 0) {
            return { ErrorCode::READ_FAILED, std::string { "Read failed." } } ;
        }
        auto sz_read = static_cast <size_t> (n) ;
#endif	/* not (_WIN32 || _WIN64) */
        return result_t_<size_t> { static_cast<size_t> (sz_read) } ;
    }

    result_t	handle_t::write (const void *data, size_t size) {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, "Invalid handle for write." } ;
        }
#if defined (_WIN32) || defined (_WIN64)
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
#else	/* not (_WIN32 || _WIN64) */
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
#endif	/* not (_WIN32 || _WIN64) */
        return {} ;
    }

    result_t	handle_t::truncate () {
        if (! valid ()) {
            return { ErrorCode::BAD_HANDLE, "Invalid handle for truncate." } ;
        }
#if defined (_WIN32) || defined (_WIN64)
        if (! SetEndOfFile (value_)) {
            return result_t { ErrorCode::TRUNCATE_FAILED
                            , std::string { "Truncate failed (" }.append (get_last_error_message ()).append (").") } ;
        }
#else	/* not (_WIN32 || _WIN64) */
        if (::write (value_, "", 0) < 0) {
            return { ErrorCode::TRUNCATE_FAILED, "Truncate failed." } ;
        }
#endif	/* not (_WIN32 || _WIN64) */
        return {} ;
    }

    result_t	handle_t::duplicate (native_handle_t h) {
#if defined (_WIN32) || defined (_WIN64)
        HANDLE	h_new ;
        HANDLE	h_proc = GetCurrentProcess () ;
        if (! DuplicateHandle (h_proc, h, h_proc, &h_new, 0, false, DUPLICATE_SAME_ACCESS)) {
            return { ErrorCode::DUPLICATE_FAILED
                   , std::string { "Duplicate handle failed (" }.append (get_last_error_message ()).append (").") } ;
        }
        return this->attach (h_new) ;
#else	/* not (_WIN32 || _WIN64) */
        int   fd = dup (h) ;
        if (fd < 0) {
            return { ErrorCode::DUPLICATE_FAILED, "Duplicate handle failed." } ;
        }
        return this->attach (fd) ;
#endif	/* not (_WIN32 || _WIN64) */
    }

    result_t    Input::open (const std::string &file) {
        return h_.attach (LowIO::open (file, OpenFlags::READ_ONLY, 0666)) ;
    }

    result_t	Output::open (const std::string &file) {
        return h_.attach (LowIO::open (file, OpenFlags::WRITE_ONLY | OpenFlags::CREATE | OpenFlags::TRUNCATE, 0666)) ;
    }
}
