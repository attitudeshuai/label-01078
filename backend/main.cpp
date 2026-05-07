#include "httplib.h"
#include "json.hpp"
#include "gcd_solver.hpp"
#include "exceptions.hpp"
#include "response_utils.hpp"
#include "request_validator.hpp"
#include "exception_handler.hpp"
#include <iostream>
#include <chrono>
#include <cstdlib>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    httplib::Server svr;

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

    svr.Options("/api/gcd/distinct-count", [](const httplib::Request&, httplib::Response& res) {
        setup_cors(res);
        res.status = 204;
    });

    svr.Post("/api/gcd/distinct-count", [](const httplib::Request& req, httplib::Response& res) {
        try {
            setup_cors(res);

            auto validated_req = RequestValidator::validate_gcd_request(req.body);

            auto start = std::chrono::high_resolution_clock::now();
            auto result = GcdSolver::solve(validated_req.array);
            auto end = std::chrono::high_resolution_clock::now();

            double time_ms = std::chrono::duration<double, std::milli>(end - start).count();

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
