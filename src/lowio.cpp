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

#if defined (_WIN32) || defined (_WIN64)
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
#endif

namespace LowIO {

    _os_handle_t::~_os_handle_t () {
        if (! valid ()) {
            return ;
        }
        try {
            this->close () ;
        }
        catch (const Error &) {
            value_ = BAD_HANDLE ;
        }
    }

    void    _os_handle_t::close () {
        if (! valid ()) {
            return ;
        }
        handle_t  H = value_ ;
        value_ = BAD_HANDLE ;
#if defined (_WIN32) || defined (_WIN64)
        if (! CloseHandle (H)) {
            throw Error { ErrorCode::CLOSE_FAILED
                        , std::string { "Close failed (" }.append (get_last_error_message ()).append (").") } ;
        }
#else	/* not (_WIN32 || _WIN64) */
        if (::close (H) != 0) {
            throw Error { ErrorCode::CLOSE_FAILED, std::string { "Close failed." } } ;
        }
#endif	/* not (_WIN32 || _WIN64) */
    }

    size_t _os_handle_t::seek (int64_t offset, SeekOrigin origin) {
        if (! valid ()) {
            throw Error { ErrorCode::BAD_HANDLE, std::string { "Invalid handle for seek." } } ;
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
        if (tmp.LowPart == 0xFFFFFFFFu && err != NO_ERROR)
            throw Error { ErrorCode::SEEK_FAILED
                        , std::string { "Seek failed." }.append (get_last_error_message (err)).append (").") } ;
        return static_cast <uint64_t> (tmp.QuadPart) ;
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
            throw Error { ErrorCode::SEEK_FAILED, "Seek failed." } ;
        }
        return static_cast <uint64_t> (off) ;
#endif	/* not (_WIN32 || _WIN64) */
    }


    size_t  _os_handle_t::read (void *data, size_t size) {
        if (! valid ()) {
            throw Error { ErrorCode::BAD_HANDLE, std::string { "Invalid handle for read." } } ;
        }
#if defined (_WIN32) || defined (_WIN64)
        DWORD n_read = 0 ;
        if (! ReadFile (value_, data, static_cast <DWORD>(size), &n_read, NULL)) {
            throw Error { ErrorCode::READ_FAILED
                        , std::string { "Read failed (" }.append (get_last_error_message ()).append (").") } ;
        }
        auto    sz_read = static_cast <size_t> (n_read) ;
#else	/* not (_WIN32 || _WIN64) */
        long  n = ::read (value_, data, size) ;
        if (n < 0) {
            throw Error { ErrorCode::READ_FAILED, std::string { "Read failed." } } ;
        }
        auto sz_read = static_cast <size_t> (n) ;
#endif	/* not (_WIN32 || _WIN64) */
        return sz_read ;
    }

    void	_os_handle_t::write (const void *data, size_t size) {
        if (! valid ()) {
            throw Error { ErrorCode::BAD_HANDLE, "Invalid handle for write." } ;
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

            if (! WriteFile (value_, static_cast<const char *> (data) + sz_written, sz, &cnt, NULL) ||
                (cnt != sz))
                throw Error { ErrorCode::WRITE_FAILED
                            , std::string { "Write failed (" }.append (get_last_error_message ()).append (").") } ;
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
                throw Error { ErrorCode::WRITE_FAILED, std::string { "Write failed." } } ;
            }
            sz_written += sz ;
        }
#endif	/* not (_WIN32 || _WIN64) */
    }

    void	_os_handle_t::truncate () {
        if (! valid ()) {
            throw Error { ErrorCode::BAD_HANDLE, "Invalid handle for truncate." } ;
        }
#if defined (_WIN32) || defined (_WIN64)
        if (! SetEndOfFile (value_)) {
            throw Error { ErrorCode::TRUNCATE_FAILED
                        , std::string { "Truncate failed (" }.append (get_last_error_message ()).append (").") } ;
        }
#else	/* not (_WIN32 || _WIN64) */
        if (::write (value_, "", 0) < 0) {
            throw Error { ErrorCode::TRUNCATE_FAILED, "Truncate failed." } ;
        }
#endif	/* not (_WIN32 || _WIN64) */
    }

    void	_os_handle_t::duplicate (handle_t h) {
#if defined (_WIN32) || defined (_WIN64)
        HANDLE	h_new ;
        HANDLE	h_proc = GetCurrentProcess () ;
        if (! DuplicateHandle (h_proc, h, h_proc, &h_new, 0, false, DUPLICATE_SAME_ACCESS)) {
            throw Error { ErrorCode::DUPLICATE_FAILED
                        , std::string { "Duplicate handle failed (" }.append (get_last_error_message ()).append (").") } ;
        }
        this->attach (h_new) ;
#else	/* not (_WIN32 || _WIN64) */
        int   fd = dup (h) ;
        if (fd < 0) {
            throw Error { ErrorCode::DUPLICATE_FAILED, "Duplicate handle failed." } ;
        }
        this->attach (fd) ;
#endif	/* not (_WIN32 || _WIN64) */
    }

    Input & Input::open (const std::string &file) {
#if defined (_WIN32) || defined (_WIN64)
        HANDLE    H = CreateFileA ( file.c_str ()
                                  , GENERIC_READ
                                  , FILE_SHARE_READ
                                  , nullptr
                                  , OPEN_EXISTING
                                  , FILE_ATTRIBUTE_NORMAL
                                  , 0) ;
        if (H == INVALID_HANDLE_VALUE) {
            throw Error { ErrorCode::OPEN_FAILED
                        , std::string { "Failed to open file [" }.append (file).append ("] (").append (get_last_error_message ()).append (").") } ;
        }
        h_.attach (static_cast <handle_t> (H)) ;
#else	/* not (_WIN32 || _WIN64) */
        int	fd = ::open (file.c_str (), O_RDONLY) ;
        if (fd < 0) {
            throw Error { ErrorCode::OPEN_FAILED
                        , std::string { "Failed to open file [" }.append (file).append ("].") } ;
        }
        h_.attach (static_cast <handle_t> (fd)) ;
#endif	/* not (_WIN32 || _WIN64) */
        return *this ;
    }

    void	Output::open (const std::string &file) {
#if defined (_WIN32) || defined (_WIN64)
        HANDLE    H = CreateFile(file.c_str (), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0) ;
        if (H == INVALID_HANDLE_VALUE) {
            throw Error { ErrorCode::CREATE_FAILED
                        , std::string { "Failed to create file [" }.append (file).append ("] (").append (get_last_error_message ()).append (").") } ;
        }
        h_.attach (static_cast <handle_t> (H)) ;
#else	/* not (_WIN32 || _WIN64) */

        int	fd = ::open (file.c_str (), O_CREAT | O_WRONLY | O_TRUNC, 0666) ;

        if (fd < 0) {
            throw Error (ErrorCode::CREATE_FAILED, std::string ("Failed to create file [").append (file).append ("].")) ;
        }
        h_.attach (static_cast <handle_t> (fd)) ;
#endif	/* not (_WIN32 || _WIN64) */
    }
}
