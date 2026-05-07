#ifndef RESPONSE_UTILS_HPP
#define RESPONSE_UTILS_HPP

#include "httplib.h"
#include "json.hpp"
#include <vector>

using json = nlohmann::json;

inline void setup_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

inline json create_error_response(int code, const std::string& message, const std::string& type = "ERROR") {
    json response;
    response["success"] = false;
    response["error"]["code"] = code;
    response["error"]["message"] = message;
    response["error"]["type"] = type;
    return response;
}

inline json create_success_response(int count, const std::vector<int>& gcds, double time_ms) {
    json response;
    response["success"] = true;
    response["data"]["count"] = count;
    response["data"]["time_ms"] = time_ms;
    
    json gcd_array = json::array();
    for (int g : gcds) {
        gcd_array.push_back(g);
    }
    response["data"]["distinct_gcds"] = gcd_array;
    
    return response;
}

#endif
