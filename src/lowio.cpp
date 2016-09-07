//
// Created by Masashi Fujita on 2016/09/08.
// Copyright (c) 2016 Polyphony Digital Inc. All rights reserved.
//

#ifdef HAVE_CONFIG_HPP
#   include "config.hpp"
#endif

#include "lowio/lowio.hpp"

namespace LowIO {
    std::string unparse_flags (uint32_t flags) {
        auto check = [flags](uint32_t f) -> bool {
            return (flags & f) != 0 ;
        };

        std::string result ;
        if (check (OpenFlags::CREATE)) {
            if (check (OpenFlags::APPEND)) {
                result += 'a' ;
            }
            else if (check (OpenFlags::TRUNCATE)) {
                result += 'w' ;
            }
            else {
                // Should never happen.
                result += 'w' ;
            }
            if (check (OpenFlags::READ_WRITE)) {
                result += '+' ;
            }
        }
        else {
            if (check (OpenFlags::READ_WRITE)) {
                result += "r+" ;
            }
            else if (check (OpenFlags::READ_ONLY)) {
                result += 'r' ;
            }
            else {
                // Should never happen.
                result += 'r' ;
            }
        }
        if (check (OpenFlags::EXCLUDE)) {
            result += 'x' ;
        }
        return result ;
    }
}
