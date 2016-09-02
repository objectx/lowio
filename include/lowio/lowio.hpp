/*
 * lowio.h: Performs I/O via lowlevel system dependent API
 *
 * AUTHOR(S): objectx
 */
#pragma once

#ifndef	lowio_hpp__96145883_4CE0_42BE_83E5_CD4D75A540A0
#define	lowio_hpp__96145883_4CE0_42BE_83E5_CD4D75A540A0 1

#include <sys/types.h>
#include <stdint.h>
#include <assert.h>

#include <stdexcept>
#include <memory>
#include <string>

#include <fcntl.h>


#if defined (_WIN64) || defined (_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif

namespace LowIO {

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
                         , DUPLICATE_FAILED } ;

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

        std::string getMessage () const {
            if (message_) {
                return *message_ ;
            }
            return std::string {} ;
        }
    } ;


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


    enum class SeekOrigin { BEGIN, END, CURRENT };

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

    //! @brief Opens/Creates `path`
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
    private:
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
                if (auto r = this->close ()) {
                    /* NO-OP */
                }
                else {
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

        //! Duplicates supplied handle.
        //! @param h The handle to duplicate
        result_t    duplicate (native_handle_t h) ;
    } ;


    class Input {
    private:
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
        //! Opens `file` for reading.
        //! @param file File to read
        result_t	open (const std::string &file) {
            return h_.attach (LowIO::open (file, OpenFlags::READ_ONLY, 0666)) ;
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
        //! @return # of bytes read
        //! @param data buffer to store data
        //! @param size buffer size
        result_t_<size_t>	fetch (void *data, size_t size) {
            return h_.read (data, size) ;
        }

        //! Reads `size` bytes data into `data`.
        //! Raising exception if failed to read exactly <code>size</code> bytes.
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

        //! Moving file-pointer to the specified position.
        //! @return byte offset from the beginning
        //! @param offset delta value
        //! @param origin origin for computing file-pointer
        result_t_<size_t>	seek (int64_t offset, SeekOrigin origin) {
            return h_.seek (offset, origin) ;
        }

        result_t    duplicate (native_handle_t h) {
            return h_.duplicate (h) ;
        }
    };

    class Output {
    private:
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

        //! Opens & Creates <code>file</code> for writing.
        //! @param file file to read
        result_t	open (const std::string &file) {
            return h_.attach (LowIO::open (file, OpenFlags::WRITE_ONLY | OpenFlags::CREATE | OpenFlags::TRUNCATE, 0666)) ;
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

        //! Moving file-pointer to specified position (for giant file).
        //! @return byte offset from the beginning
        //! @param offset delta value
        //! @param origin origin for computing file-pointer
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
