/*
 * lowio.h: Performs I/O via low-level system dependent API.
 *
 * Copyright (c) 2016 Masashi Fujita
 */

#pragma once

#ifndef	lowio_hpp__96145883_4CE0_42BE_83E5_CD4D75A540A0
#define	lowio_hpp__96145883_4CE0_42BE_83E5_CD4D75A540A0 1

#include <stdint.h>
#include <cassert>

#include <cctype>

#include <stdexcept>
#include <memory>
#include <string>
#include <iterator>
#include <vector>

#include <fcntl.h>

#if defined (_WIN64) || defined (_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif

namespace LowIO {

    // FileHandle aliases...
#if defined (_WIN32) || defined (_WIN64)
    using native_handle_t = HANDLE ;
    const native_handle_t	BAD_HANDLE = INVALID_HANDLE_VALUE ;
    inline bool	valid_handle_p (native_handle_t H) {
        return (H != BAD_HANDLE) ;
    }
#else
    using native_handle_t = int ;
    const native_handle_t	BAD_HANDLE = static_cast<native_handle_t> (-1) ;

    inline bool	valid_handle_p (native_handle_t H) {
        return (0 <= H) ;
    }
#endif

    //! Error codes.
    enum class ErrorCode { SUCCESS
                         , OPEN_FAILED
                         , CREATE_FAILED
                         , CLOSE_FAILED
                         , BAD_HANDLE
                         , READ_FAILED
                         , WRITE_FAILED
                         , SEEK_FAILED
                         , TRUNCATE_FAILED
                         , DUPLICATE_FAILED
                         , BAD_PARAMETER } ;


    //! Represents a operation result.
    class result_t {
        ErrorCode code_ = ErrorCode::SUCCESS ;
        std::unique_ptr<std::string>    message_ ;
    public:
        result_t () { /* NO-OP */ }

        result_t (ErrorCode code, const std::string &msg)
                : code_ { code }
                , message_ { std::make_unique<std::string> (msg) } {
            /* NO-OP */
        }

        result_t (ErrorCode code, std::string &&msg)
                : code_ { code }
                , message_ { std::make_unique<std::string> (std::move (msg)) } {
            /* NO-OP */
        }

        result_t (result_t &&src) = default ;

        ErrorCode getErrorCode () const {
            return code_ ;
        }

        operator bool () const {
            return code_ == ErrorCode::SUCCESS ;
        }

        bool    operator ! () const {
            return code_ != ErrorCode::SUCCESS ;
        }

        //! Retrieves a error message.
        std::string getMessage () const {
            if (message_) {
                return *message_ ;
            }
            return std::string {} ;
        }
    } ;

    //! Represents a result or an error.
    template <typename T_>
        class result_t_ final : public result_t {
            T_  value_ ;
        public:
            explicit result_t_ (const T_ &value) : value_ { value } {
                /* NO-OP */
            }

            result_t_ (ErrorCode code, const std::string &msg)
                    : result_t { code, msg } {
                /* NO-OP */
            }

            result_t_ (ErrorCode code, std::string &&msg)
                    : result_t { code, std::move (msg) } {
                /* NO-OP */
            }

            explicit operator T_ () const {
                return value_ ;
            }

            const T_ &  getValue () const {
                return value_ ;
            }
        } ;


    //! Seek origins.
    enum class SeekOrigin { BEGIN, END, CURRENT };


    //! Constants for opening a file.
    struct OpenFlags final {
#if ! (defined (_WIN32) || defined (_WIN64))
        // For the UN?X like systems
        static const uint32_t  READ_ONLY  = O_RDONLY ;
        static const uint32_t  WRITE_ONLY = O_WRONLY ;
        static const uint32_t  READ_WRITE = O_RDWR ;
        static const uint32_t  APPEND     = O_APPEND ;
        static const uint32_t  CREATE     = O_CREAT ;
        static const uint32_t  TRUNCATE   = O_TRUNC ;
        static const uint32_t  EXCLUDE    = O_EXCL ;
#else
        static const uint32_t  READ_ONLY  = _O_RDONLY ;
        static const uint32_t  WRITE_ONLY = _O_WRONLY ;
        static const uint32_t  READ_WRITE = _O_RDWR ;
        static const uint32_t  APPEND     = _O_APPEND ;
        static const uint32_t  CREATE     = _O_CREAT ;
        static const uint32_t  TRUNCATE   = _O_TRUNC ;
        static const uint32_t  EXCLUDE    = _O_EXCL ;
#endif
    } ;

    //! @brief Composes `OpenFlags` from the supplied sequence.
    //! @param it_beg Start of the sequence
    //! @param it_end End of the sequence
    //! @return Composed flags
    template <typename Iter_>
        result_t_<uint32_t> parse_flags (Iter_ it_beg, Iter_ it_end) {
            uint32_t result = 0 ;

            auto invalid_param = [it_beg, it_end](char ch) {
                std::string msg { "Invalid open mode '" } ;
                msg.push_back (ch) ;
                msg.append ("' found in \"")
                   .append (it_beg, it_end)
                   .append ("\".") ;
                return result_t_<uint32_t> { ErrorCode::BAD_PARAMETER, msg } ;
            } ;

            auto it = it_beg ;
            char ch = std::tolower (*it++) ;
            switch (ch) {
            case 'r':
                result = 0 ;
                if (it != it_end && *it == '+') {
                    ++it ;
                    // "r+"
                    result |= OpenFlags::READ_WRITE ;
                }
                else {
                    result |= OpenFlags::READ_ONLY ;
                }
                break ;
            case 'w':
                result = OpenFlags::CREATE | OpenFlags::TRUNCATE ;
                if (it != it_end && *it == '+') {
                    ++it ;
                    // "r+"
                    result |= OpenFlags::READ_WRITE ;
                }
                else {
                    result |= OpenFlags::WRITE_ONLY ;
                }
                break ;
            case 'a':
                result = OpenFlags::CREATE | OpenFlags::APPEND ;
                if (it != it_end && *it == '+') {
                    ++it ;
                    // "r+"
                    result |= OpenFlags::READ_WRITE ;
                }
                else {
                    result |= OpenFlags::WRITE_ONLY ;
                }
                break ;
            default:
                return invalid_param (ch) ;
            }
            for ( ; it != it_end ; ++it) {
                switch (std::tolower (*it)) {
                case 'b':
                    break ;
                case 'x':
                    result |= OpenFlags::EXCLUDE ;
                    break ;
                default:
                    return invalid_param (*it) ;
                }
            }
            return result_t_<uint32_t> { result } ;
        }

    template <typename T_>
        result_t_<uint32_t>  parse_flags (const T_ &s) {
            using std::begin ;
            using std::end ;
            return parse_flags (begin (s), end (s)) ;
        }

    // Retrieves STD(IN|OUT|ERR) handles.
#if (! defined (_WIN64)) && (! defined (_WIN32))
    //! Gets OS's native handle for the input.
    //! @return Handle for input
    inline native_handle_t	GetSTDIN () {
        return 0 ;
    }

    //! Gets OS's native handle for output.
    //! @return Handle for output
    inline native_handle_t	GetSTDOUT () {
        return 1 ;
    }

    //! Gets OS's native handle for error output.
    //! @return Handle for error output
    inline native_handle_t	GetSTDERR () {
        return 2 ;
    }
#else   /* _WIN64 OR _WIN32 */
    inline native_handle_t	GetSTDIN () {
        return GetStdHandle (STD_INPUT_HANDLE) ;
    }

    inline native_handle_t	GetSTDOUT () {
        return GetStdHandle (STD_OUTPUT_HANDLE) ;
    }

    inline native_handle_t	GetSTDERR () {
        return GetStdHandle (STD_ERROR_HANDLE) ;
    }
#endif  /* _WIN64 OR _WIN32 */

    //! @brief Opens/Creates a file.
    //! @param path Path to open/create.
    //! @param flag Flags
    //! @param mode Access mode
    //! @return File handle
    native_handle_t open (const std::string &path, uint32_t flag, uint32_t mode) ;

    //! @brief Closes a file handle.
    //! @param h Handle to close
    //! @remaks After calling this, passed `h` is invalidated.
    result_t close (native_handle_t &&h) ;


    //! Wrapper class for using OS's native file handle.
    class handle_t final {
        native_handle_t	value_ = BAD_HANDLE ;
    public:
        ~handle_t () {
            this->close () ;
        }

        handle_t () = default ;

        explicit handle_t (native_handle_t h) : value_ { h } {/* NO-OP */}

        handle_t (handle_t &&src) : value_ { src.detach () } { /* NO-OP */ }

        explicit operator native_handle_t () const {
            return value_ ;
        }

        handle_t &	operator = (handle_t &&src) {
            attach (src.detach ()) ;
            return *this ;
        }

        native_handle_t	handle () const {
            return value_ ;
        }

        bool		valid () const {
            return valid_handle_p (value_) ;
        }

        operator bool () const {
            return valid () ;
        }

        bool operator ! () const {
            return ! valid () ;
        }

        result_t    close () {
            return LowIO::close (std::move (value_)) ;
        }

        result_t    attach (native_handle_t h) {
            if (valid ()) {
                auto r = this->close () ;
                if (! r) {
                    return r ;
                }
            }
            value_ = h ;
            return {} ;
        }

        native_handle_t	detach () {
            native_handle_t	result = BAD_HANDLE ;
            std::swap (result, value_) ;
            return result ;
        }

        //! Moving file-pointer to the specified position.
        //! @return byte offset from the beginning
        //! @param offset delta value
        //! @param origin origin for computing file-pointer
        result_t_<size_t>   seek (int64_t offset, SeekOrigin origin) ;

        //! Reads <code>size</code> bytes data into <code>data</code>
        //! @param data buffer to store data
        //! @param size buffer size
        //! @param sz_read actually read bytes
        result_t_<size_t>   read (void *data, size_t maxsize) ;

        //! Writes <code>size</code> bytes data from <code>data</code>.
        //! @param data the source
        //! @param size size to write
        result_t    write (const void *data, size_t size) ;

        //! Truncates a opened file at the current position.
        result_t    truncate () ;

        //! Duplicates supplied handle and attatch it.
        //! @param h The handle to duplicate
        result_t    duplicate (native_handle_t h) ;

        //! Duplicate internal handle.
        native_handle_t duplicate () const ;
    } ;


    //! Input handle abstraction.
    class Input {
        static const size_t DEFAULT_BUFFER_SIZE = 4 * 1024 * 1024 ;
        handle_t	h_;
    public:
        ~Input () = default ;
        Input () = default ;

        //! Construct & open `file`
        //! @param file file to read
        Input (const std::string &file) {
            this->open (file) ;
        }

        explicit Input (native_handle_t h) : h_ { h } { /* NO-OP */ }

        Input (Input &&src) : h_ { src.detach () } {
            /* NO-OP */
        }

        bool valid () const {
            return h_.valid () ;
        }

        operator bool () const {
            return h_.valid () ;
        }

        bool operator ! () const {
            return ! h_.valid () ;
        }
        //! Opens `file` for reading.
        //! @param file File to read
        result_t	open (const std::string &file) {
            auto fd = LowIO::open (file, OpenFlags::READ_ONLY, 0666) ;
            if (valid_handle_p (fd)) {
                return h_.attach (fd) ;
            }
            return { ErrorCode::OPEN_FAILED
                   , std::string { "Failed to open a file \"" }.append (file).append ("\" for reading.") } ;
        }

        //! Closes attached handle
        result_t	close() {
            return h_.close();
        }

        result_t    attach (native_handle_t h) {
            return h_.attach (h) ;
        }

        native_handle_t	detach () {
            return h_.detach ();
        }

        //! Reads at most `size` bytes data into the `data`.
        //! @param data buffer to store data
        //! @param size buffer size
        //!
        //! @return # of bytes read or error
        result_t_<size_t>	fetch (void *data, size_t size) {
            return h_.read (data, size) ;
        }

        //! Reads exact `size` bytes data into `data`.
        //! @param data buffer to store data
        //! @param size size to read
        result_t	read (void *data, size_t size) {
            auto r = h_.read (data, size);
            if (! r) {
                return { std::move (r) } ;
            }
            if (r.getValue () != size) {
                return { ErrorCode::READ_FAILED, std::string { "Premature EOF" } } ;
            }
            return {} ;
        }

        template <typename Iter_>
            result_t    readAll (Iter_ it, size_t buffer_size = 0) {
                std::vector<uint8_t>    buffer ;
                if (buffer_size == 0) {
                    buffer_size = DEFAULT_BUFFER_SIZE ;
                }
                buffer.resize (buffer_size) ;

                while (true) {
                    auto r = h_.read (buffer.data (), buffer.size ()) ;
                    if (! r) {
                        return result_t { std::move (r) } ;
                    }
                    auto sz = r.getValue () ;
                    for (size_t i = 0 ; i < sz ; ++i) {
                        *it++ = buffer [i] ;
                    }
                    if (sz < buffer.size ()) {
                        return result_t { std::move (r) } ;
                    }
                }
            }

        //! Moving file-pointer to the specified position.
        //! @param offset delta value
        //! @param origin origin for computing file-pointer
        //! @return byte offset from the beginning or error
        result_t_<size_t>	seek (int64_t offset, SeekOrigin origin) {
            return h_.seek (offset, origin) ;
        }

        //! Duplicates `h` and attach it.
        //! @param h Handle to duplicate
        result_t    duplicate (native_handle_t h) {
            return h_.duplicate (h) ;
        }
    };


    //! Output handle abstraction.
    class Output {
        handle_t	h_ ;
    public:
        ~Output () = default ;

        Output () = default ;

        Output (const std::string &file) {
            this->open (file) ;
        }

        explicit Output (native_handle_t h) : h_ { h } { /* NO-OP */ }

        Output (Output &&src) : h_ { src.detach() } { /* NO-OP */ }

        bool valid () const {
            return h_.valid () ;
        }

        operator bool () const {
            return h_.valid () ;
        }

        bool operator ! () const {
            return ! h_.valid () ;
        }

        //! Opens & Creates <code>file</code> for writing.
        //! @param file file to read
        result_t	open (const std::string &file) {
            auto fd = LowIO::open (file, OpenFlags::WRITE_ONLY | OpenFlags::CREATE | OpenFlags::TRUNCATE, 0666) ;
            if (valid_handle_p (fd)) {
                return h_.attach (fd) ;
            }
            return { ErrorCode::OPEN_FAILED
                   , std::string { "Failed to open a file \"" }.append (file).append ("\" for writing.") } ;
        }

        result_t	close () {
            return h_.close() ;
        }

        Output &	attach (native_handle_t h) {
            h_.attach(h) ;
            return *this ;
        }

        native_handle_t	detach () {
            return h_.detach() ;
        }

        //! Writes `size` bytes data from `data`.
        //! @param data   the source
        //! @param size   size to write
        result_t	write (const void *data, size_t size) {
            return h_.write(data, size) ;
        }

        //! Moving file-pointer to specified position.
        //! @param offset delta value
        //! @param origin origin for computing file-pointer
        //! @return byte offset from the beginning
        result_t_<size_t>   seek (int64_t offset, SeekOrigin origin) {
            return h_.seek (offset, origin) ;
        }

        result_t	duplicate (native_handle_t h) {
            return h_.duplicate(h) ;
        }

        //! Truncate file at current position.
        result_t	truncate () {
            return h_.truncate() ;
        }
    } ;
}

#endif	/* lowio_hpp__96145883_4CE0_42BE_83E5_CD4D75A540A0 */
