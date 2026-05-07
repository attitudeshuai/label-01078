#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

class ValidationException : public std::runtime_error {
public:
    explicit ValidationException(const std::string& msg) : std::runtime_error(msg) {}
};

class BusinessException : public std::runtime_error {
public:
    explicit BusinessException(const std::string& msg) : std::runtime_error(msg) {}
};

#endif
