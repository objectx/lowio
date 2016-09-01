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
#include <string>
#include <fcntl.h>

#if defined (_WIN64) || defined (_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif

namespace LowIO {

    enum class ErrorCode { OPEN_FAILED
                         , CREATE_FAILED
                         , CLOSE_FAILED
                         , BAD_HANDLE
                         , READ_FAILED
                         , WRITE_FAILED
                         , SEEK_FAILED
                         , TRUNCATE_FAILED
                         , DUPLICATE_FAILED } ;

    class Error final : public std::runtime_error {
    private:
        ErrorCode	code_ ;
    public:
        Error (ErrorCode code, const std::string &msg)
                : std::runtime_error { msg }, code_ { code } {
            /* NO-OP */
        }

        ErrorCode   code () const {
            return code_ ;
        }
    } ;

    enum class SeekOrigin { BEGIN, END, CURRENT };

    enum class OpenFlags : uint32_t {
#if ! (defined (_WIN32) || defined (_WIN64))
        // For the UN?X like systems
          READ_ONLY  = O_RDONLY
        , WRITE_ONLY = O_WRONLY
        , READ_WRITE = O_RDWR
        , APPEND     = O_APPEND
        , CREATE     = O_CREAT
        , TRUNCATE   = O_TRUNC
        , EXCLUDE    = O_EXCL
#else
          READ_ONLY  = _O_RDONLY
        , WRITE_ONLY = _O_WRONLY
        , READ_WRITE = _O_RDWR
        , APPEND     = _O_APPEND
        , CREATE     = _O_CREAT
        , TRUNCATE   = _O_TRUNC
        , EXCLUDE    = _O_EXCL
#endif
    } ;
    // FileHandle aliases...
#if defined (_WIN32) || defined (_WIN64)
    using handle_t = HANDLE ;
    const handle_t	BAD_HANDLE = INVALID_HANDLE_VALUE ;
    inline bool	valid_handle_p (handle_t H) {
        return (H != BAD_HANDLE) ;
    }
#else
    using handle_t = int ;
    const handle_t	BAD_HANDLE = static_cast<handle_t> (-1) ;

    inline bool	valid_handle_p (handle_t H) {
        return (0 <= H) ;
    }
#endif

    /**
     * Gets OS's native handle for input.
     *
     * @return handle for input
     */
    inline handle_t	GetSTDIN () {
#if defined (_WIN32) || defined (_WIN64)
        return GetStdHandle (STD_INPUT_HANDLE) ;
#else
        return 0 ;
#endif
    }

    /**
     * Gets OS's native handle for output.
     *
     * @return handle for output
     */
    inline handle_t	GetSTDOUT () {
#if defined (_WIN32) || defined (_WIN64)
        return GetStdHandle (STD_OUTPUT_HANDLE) ;
#else
        return 1 ;
#endif
    }

    /**
     * Gets OS's native handle for error output.
     *
     * @return handle for error output
     */
    inline handle_t	GetSTDERR () {
#if defined (_WIN32) || defined (_WIN64)
        return GetStdHandle (STD_ERROR_HANDLE) ;
#else
        return 2 ;
#endif
    }

    //! @brief Opens a file handle.
    handle_t open (const std::string &path, OpenFlags flag, uint32_t mode) ;
    //! @brief Closes a file handle.
    void close (handle_t &&h) ;

    /** Wrapper class for using OS's native file handle.  */
    class _os_handle_t final {
    private:
        handle_t	value_ ;
    public:
        ~_os_handle_t () ;

        _os_handle_t () : value_ { BAD_HANDLE } {/* NO-OP */}

        explicit _os_handle_t (handle_t h) : value_ { h } {/* NO-OP */}

        _os_handle_t (_os_handle_t &&src) : value_ { src.detach () } { /* NO-OP */ }

        operator handle_t () const {
            return value_ ;
        }

        _os_handle_t &	operator = (_os_handle_t &&src) {
            attach (src.detach ()) ;
            return *this ;
        }

        handle_t	handle () const {
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

        void		close () ;

        void		attach (handle_t h) {
            if (valid ()) {
                this->close () ;
            }
            value_ = h ;
        }

        handle_t	detach () {
            handle_t	result = value_ ;
            value_ = BAD_HANDLE ;
            return result ;
        }

        /**
         * Moving file-pointer to specified position.
         *
         * @return byte offset from the beginning
         * @param offset delta value
         * @param origin origin for computing file-pointer
         */
        size_t	seek (int64_t offset, SeekOrigin origin) ;

        /**
         * Reads <code>size</code> bytes data into <code>data</code>
         *
         * @param data buffer to store data
         * @param size buffer size
         * @param sz_read actually read bytes
         */
        size_t  read (void *data, size_t maxsize) ;

        /**
         * Writes <code>size</code> bytes data from <code>data</code>.
         *
         * @param data the source
         * @param size size to write
         */
        void    write (const void *data, size_t size) ;

        /**
         * Truncates file at current position.
         */
        void    truncate () ;

        /**
         * Duplicates supplied handle.
         *
         * @param h the handle to duplicate
         */
        void    duplicate (handle_t h) ;
    } ;

    class Input {
    private:
        _os_handle_t	h_;
    public:
        ~Input () { /* NO-OP */ }
        Input () { /* NO-OP */ }
        /**
         * Construct & open <code>file</code>
         *
         * @param file file to read
         */
        Input (const std::string &file) {
            this->open (file) ;
        }

        explicit Input (handle_t h) : h_ (h) { /* NO-OP */ }

        Input (Input &&src) : h_ (src.detach ()) {
            /* NO-OP */
        }
        /**
         * Opens <code>file</code> for reading.
         *
         * @param file file to read
         */
        Input &	open (const std::string &file);

        /** Closes attached handle.  */
        void	close() {
            h_.close();
        }

        Input & attach (handle_t h) {
            h_.attach (h);
            return *this ;
        }

        handle_t	detach () {
            return h_.detach ();
        }

        /**
         * Reads at most <code>size</code> bytes data into <code>data</code>
         *
         * @return # of bytes read
         * @param data buffer to store data
         * @param size buffer size
         */
        size_t	fetch (void *data, size_t size) {
            return h_.read (data, size) ;
        }
        /**
         * Reads <code>size</code> bytes data into <code>data</code>.
         * Raising exception if failed to read exactly <code>size</code> bytes.
         *
         * @param data buffer to store data
         * @param size size to read
         */
        void	read (void *data, size_t size) {
            auto sz_read = h_.read (data, size);
            if (sz_read != size) {
                throw Error { ErrorCode::READ_FAILED, std::string { "Premature EOF" } } ;
            }
        }
        /**
         * Moving file-pointer to specified position (for giant file).
         *
         * @return byte offset from the beginning
         * @param offset delta value
         * @param origin origin for computing file-pointer
         */
        uint64_t	seek (int64_t offset, SeekOrigin origin) {
            return static_cast<uint64_t> (h_.seek (offset, origin)) ;
        }

        Input & duplicate (handle_t h) {
            h_.duplicate (h) ;
            return *this ;
        }
    };

    class Output {
    private:
        _os_handle_t	h_ ;
    public:
        ~Output () { /* NO-OP */ }
        Output () { /* NO-OP */ }

        Output (const std::string &file) {
            this->open (file) ;
        }

        explicit Output (handle_t h) : h_ { h } { /* NO-OP */ }
        Output (Output &&src) : h_ { src.detach() } { /* NO-OP */ }

        /**
         * Opens & Creates <code>file</code> for writing.
         *
         * @param file file to read
         */
        void	open (const std::string &file) ;

        void	close () {
            h_.close() ;
        }

        Output &	attach (handle_t h) {
            h_.attach(h) ;
            return *this ;
        }

        handle_t	detach () {
            return h_.detach() ;
        }

        /**
         * Writes <code>size</code> bytes data from <code>data</code>.
         *
         * @param data   the source
         * @param size   size to write
         */
        void	write (const void *data, size_t size) {
            h_.write(data, size) ;
        }

        /**
         * Moving file-pointer to specified position (for giant file).
         *
         * @return byte offset from the beginning
         * @param offset delta value
         * @param origin origin for computing file-pointer
         */
        uint64_t	seek (int64_t offset, SeekOrigin origin) {
            return static_cast<uint64_t> (h_.seek (offset, origin)) ;
        }

        Output &	duplicate (handle_t h) {
            h_.duplicate(h) ;
            return *this ;
        }

        /** Truncate file at current position.  */
        Output &	truncate () {
            h_.truncate() ;
            return *this ;
        }
    } ;
}

#endif	/* lowio_hpp__96145883_4CE0_42BE_83E5_CD4D75A540A0 */
