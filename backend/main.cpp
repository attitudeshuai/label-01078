#include "httplib.h"
#include "json.hpp"
#include "gcd_solver.hpp"
#include <iostream>
#include <chrono>
#include <stdexcept>

using json = nlohmann::json;

/**
 * GCD 计算服务
 * 
 * API:
 * POST /api/gcd/distinct-count
 * 请求体: {"array": [a1, a2, ..., an]}
 * 响应: {"count": k, "distinct_gcds": [...], "time_ms": t}
 * 
 * GET /api/health
 * 响应: {"status": "ok"}
 */

// ==================== 异常定义 ====================

class ValidationException : public std::runtime_error {
public:
    explicit ValidationException(const std::string& msg) : std::runtime_error(msg) {}
};

class BusinessException : public std::runtime_error {
public:
    explicit BusinessException(const std::string& msg) : std::runtime_error(msg) {}
};

// ==================== 响应工具 ====================

void setup_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

json create_error_response(int code, const std::string& message, const std::string& type = "ERROR") {
    json response;
    response["success"] = false;
    response["error"]["code"] = code;
    response["error"]["message"] = message;
    response["error"]["type"] = type;
    return response;
}

json create_success_response(int count, const std::vector<int>& gcds, double time_ms) {
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

// ==================== 参数校验器 ====================

class RequestValidator {
public:
    struct GcdRequest {
        std::vector<int> array;
    };

    static GcdRequest validate_gcd_request(const std::string& body) {
        // 校验请求体非空
        if (body.empty()) {
            throw ValidationException("Request body cannot be empty");
        }

        // 解析 JSON
        json request_json;
        try {
            request_json = json::parse(body);
        } catch (const std::exception& e) {
            throw ValidationException(std::string("Invalid JSON format: ") + e.what());
        }

        // 校验必填字段 array
        if (!request_json.contains("array")) {
            throw ValidationException("Field 'array' is required");
        }

        if (!request_json["array"].is_array()) {
            throw ValidationException("Field 'array' must be an array");
        }

        GcdRequest req;
        req.array = request_json["array"].get<std::vector<int>>();

        // 校验数组长度
        if (req.array.size() < 2) {
            throw ValidationException("Array must contain at least 2 elements");
        }

        if (req.array.size() > 200000) {
            throw ValidationException("Array size exceeds maximum limit (200000)");
        }

        // 校验数组元素值
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

// ==================== 全局异常处理器 ====================

class GlobalExceptionHandler {
public:
    static void handle(const std::exception& e, httplib::Response& res) {
        setup_cors(res);

        // 处理校验异常
        if (dynamic_cast<const ValidationException*>(&e)) {
            res.status = 400;
            res.set_content(
                create_error_response(400, e.what(), "VALIDATION_ERROR").dump(),
                "application/json"
            );
            std::cerr << "[WARN] Validation error: " << e.what() << std::endl;
            return;
        }

        // 处理业务异常
        if (dynamic_cast<const BusinessException*>(&e)) {
            res.status = 422;
            res.set_content(
                create_error_response(422, e.what(), "BUSINESS_ERROR").dump(),
                "application/json"
            );
            std::cerr << "[WARN] Business error: " << e.what() << std::endl;
            return;
        }

        // 处理未知异常 - 500
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

// ==================== 主程序 ====================

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    httplib::Server svr;

    // Health check endpoint
    svr.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        try {
            setup_cors(res);
            json response;
            response["success"] = true;
            response["data"]["status"] = "ok";
            response["data"]["service"] = "gcd-solver";
            response["data"]["version"] = "1.0.0";
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            GlobalExceptionHandler::handle(e, res);
        } catch (...) {
            GlobalExceptionHandler::handle_unknown(res);
        }
    });

    // CORS preflight
    svr.Options("/api/gcd/distinct-count", [](const httplib::Request&, httplib::Response& res) {
        setup_cors(res);
        res.status = 204;
    });

    // Main GCD computation endpoint
    svr.Post("/api/gcd/distinct-count", [](const httplib::Request& req, httplib::Response& res) {
        try {
            setup_cors(res);

            // 参数校验
            auto validated_req = RequestValidator::validate_gcd_request(req.body);

            // 业务处理
            auto start = std::chrono::high_resolution_clock::now();
            auto result = GcdSolver::solve(validated_req.array);
            auto end = std::chrono::high_resolution_clock::now();

            double time_ms = std::chrono::duration<double, std::milli>(end - start).count();

            // 返回成功响应
            res.set_content(
                create_success_response(result.count, result.distinct_gcds, time_ms).dump(),
                "application/json"
            );

        } catch (const std::exception& e) {
            GlobalExceptionHandler::handle(e, res);
        } catch (...) {
            GlobalExceptionHandler::handle_unknown(res);
        }
    });

    std::cout << "GCD Solver Service starting on port " << port << std::endl;
    std::cout << "Endpoints:" << std::endl;
    std::cout << "  GET  /api/health              - Health check" << std::endl;
    std::cout << "  POST /api/gcd/distinct-count  - Compute distinct GCDs" << std::endl;

    if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return 1;
    }

    return 0;
}
