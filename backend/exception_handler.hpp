#ifndef EXCEPTION_HANDLER_HPP
#define EXCEPTION_HANDLER_HPP

#include "httplib.h"
#include "response_utils.hpp"
#include "exceptions.hpp"
#include <iostream>

class GlobalExceptionHandler {
public:
    static void handle(const std::exception& e, httplib::Response& res) {
        setup_cors(res);

        if (dynamic_cast<const ValidationException*>(&e)) {
            res.status = 400;
            res.set_content(
                create_error_response(400, e.what(), "VALIDATION_ERROR").dump(),
                "application/json"
            );
            std::cerr << "[WARN] Validation error: " << e.what() << std::endl;
            return;
        }

        if (dynamic_cast<const BusinessException*>(&e)) {
            res.status = 422;
            res.set_content(
                create_error_response(422, e.what(), "BUSINESS_ERROR").dump(),
                "application/json"
            );
            std::cerr << "[WARN] Business error: " << e.what() << std::endl;
            return;
        }

        res.status = 500;
        res.set_content(
            create_error_response(500, "Internal server error", "INTERNAL_ERROR").dump(),
            "application/json"
        );
        std::cerr << "[ERROR] Internal error: " << e.what() << std::endl;
    }

    static void handle_unknown(httplib::Response& res) {
        setup_cors(res);
        res.status = 500;
        res.set_content(
            create_error_response(500, "Unknown internal error", "INTERNAL_ERROR").dump(),
            "application/json"
        );
        std::cerr << "[ERROR] Unknown internal error" << std::endl;
    }
};

#endif // EXCEPTION_HANDLER_HPP
