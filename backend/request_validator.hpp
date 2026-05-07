#ifndef REQUEST_VALIDATOR_HPP
#define REQUEST_VALIDATOR_HPP

#include "json.hpp"
#include "exceptions.hpp"
#include <vector>
#include <string>

using json = nlohmann::json;

class RequestValidator {
public:
    struct GcdRequest {
        std::vector<int> array;
    };

    static GcdRequest validate_gcd_request(const std::string& body) {
        if (body.empty()) {
            throw ValidationException("Request body cannot be empty");
        }

        json request_json;
        try {
            request_json = json::parse(body);
        } catch (const std::exception& e) {
            throw ValidationException(std::string("Invalid JSON format: ") + e.what());
        }

        if (!request_json.contains("array")) {
            throw ValidationException("Field 'array' is required");
        }

        if (!request_json["array"].is_array()) {
            throw ValidationException("Field 'array' must be an array");
        }

        GcdRequest req;
        req.array = request_json["array"].get<std::vector<int>>();

        if (req.array.size() < 2) {
            throw ValidationException("Array must contain at least 2 elements");
        }

        if (req.array.size() > 200000) {
            throw ValidationException("Array size exceeds maximum limit (200000)");
        }

        for (size_t i = 0; i < req.array.size(); i++) {
            int val = req.array[i];
            if (val < 1) {
                throw ValidationException("Array element at index " + std::to_string(i) +
                    " is invalid: value must be >= 1, got " + std::to_string(val));
            }
            if (val > 10000000) {
                throw ValidationException("Array element at index " + std::to_string(i) +
                    " is invalid: value must be <= 10000000, got " + std::to_string(val));
            }
        }

        return req;
    }
};

#endif // REQUEST_VALIDATOR_HPP
