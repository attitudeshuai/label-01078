// Minimal JSON library for C++
// Simplified version for this project

#ifndef NLOHMANN_JSON_HPP
#define NLOHMANN_JSON_HPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <initializer_list>

namespace nlohmann {

class json {
public:
    enum class value_t { null, object, array, string, number_integer, number_float, boolean };

    json() : type_(value_t::null) {}
    json(std::nullptr_t) : type_(value_t::null) {}
    json(bool v) : type_(value_t::boolean), bool_val_(v) {}
    json(int v) : type_(value_t::number_integer), int_val_(v) {}
    json(long v) : type_(value_t::number_integer), int_val_(v) {}
    json(long long v) : type_(value_t::number_integer), int_val_(v) {}
    json(unsigned int v) : type_(value_t::number_integer), int_val_(static_cast<long long>(v)) {}
    json(unsigned long v) : type_(value_t::number_integer), int_val_(static_cast<long long>(v)) {}
    json(unsigned long long v) : type_(value_t::number_integer), int_val_(static_cast<long long>(v)) {}
    json(double v) : type_(value_t::number_float), float_val_(v) {}
    json(const char* v) : type_(value_t::string), str_val_(v) {}
    json(const std::string& v) : type_(value_t::string), str_val_(v) {}
    
    json(std::initializer_list<std::pair<const std::string, json>> init) : type_(value_t::object) {
        for (const auto& p : init) {
            obj_val_[p.first] = p.second;
        }
    }

    static json object() {
        json j;
        j.type_ = value_t::object;
        return j;
    }

    static json array() {
        json j;
        j.type_ = value_t::array;
        return j;
    }

    static json parse(const std::string& s) {
        size_t pos = 0;
        return parse_value(s, pos);
    }

    json& operator[](const std::string& key) {
        if (type_ == value_t::null) type_ = value_t::object;
        return obj_val_[key];
    }

    json& operator[](size_t idx) {
        if (type_ == value_t::null) type_ = value_t::array;
        if (idx >= arr_val_.size()) arr_val_.resize(idx + 1);
        return arr_val_[idx];
    }

    const json& operator[](const std::string& key) const {
        static json null_json;
        auto it = obj_val_.find(key);
        return it != obj_val_.end() ? it->second : null_json;
    }

    const json& operator[](size_t idx) const {
        static json null_json;
        return idx < arr_val_.size() ? arr_val_[idx] : null_json;
    }

    void push_back(const json& val) {
        if (type_ == value_t::null) type_ = value_t::array;
        arr_val_.push_back(val);
    }

    size_t size() const {
        if (type_ == value_t::array) return arr_val_.size();
        if (type_ == value_t::object) return obj_val_.size();
        return 0;
    }

    bool contains(const std::string& key) const {
        return type_ == value_t::object && obj_val_.find(key) != obj_val_.end();
    }

    bool is_array() const { return type_ == value_t::array; }
    bool is_object() const { return type_ == value_t::object; }
    bool is_null() const { return type_ == value_t::null; }

    template<typename T>
    T get() const {
        if constexpr (std::is_same_v<T, std::string>) {
            return str_val_;
        } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, long long>) {
            return static_cast<T>(int_val_);
        } else if constexpr (std::is_same_v<T, bool>) {
            return bool_val_;
        } else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>) {
            return type_ == value_t::number_float ? static_cast<T>(float_val_) : static_cast<T>(int_val_);
        } else if constexpr (std::is_same_v<T, std::vector<int>>) {
            std::vector<int> result;
            for (const auto& v : arr_val_) {
                result.push_back(static_cast<int>(v.int_val_));
            }
            return result;
        }
        return T{};
    }

    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        dump_impl(oss, indent, 0);
        return oss.str();
    }

    // Iterator support for arrays
    auto begin() { return arr_val_.begin(); }
    auto end() { return arr_val_.end(); }
    auto begin() const { return arr_val_.begin(); }
    auto end() const { return arr_val_.end(); }

private:
    value_t type_;
    std::map<std::string, json> obj_val_;
    std::vector<json> arr_val_;
    std::string str_val_;
    long long int_val_ = 0;
    double float_val_ = 0.0;
    bool bool_val_ = false;

    void dump_impl(std::ostringstream& oss, int indent, int depth) const {
        std::string ind = indent >= 0 ? std::string(depth * indent, ' ') : "";
        std::string ind2 = indent >= 0 ? std::string((depth + 1) * indent, ' ') : "";
        std::string nl = indent >= 0 ? "\n" : "";
        std::string sep = indent >= 0 ? ": " : ":";

        switch (type_) {
            case value_t::null: oss << "null"; break;
            case value_t::boolean: oss << (bool_val_ ? "true" : "false"); break;
            case value_t::number_integer: oss << int_val_; break;
            case value_t::number_float: oss << float_val_; break;
            case value_t::string: oss << "\"" << escape_string(str_val_) << "\""; break;
            case value_t::array: {
                oss << "[" << nl;
                bool first = true;
                for (const auto& v : arr_val_) {
                    if (!first) oss << "," << nl;
                    first = false;
                    oss << ind2;
                    v.dump_impl(oss, indent, depth + 1);
                }
                oss << nl << ind << "]";
                break;
            }
            case value_t::object: {
                oss << "{" << nl;
                bool first = true;
                for (const auto& [k, v] : obj_val_) {
                    if (!first) oss << "," << nl;
                    first = false;
                    oss << ind2 << "\"" << escape_string(k) << "\"" << sep;
                    v.dump_impl(oss, indent, depth + 1);
                }
                oss << nl << ind << "}";
                break;
            }
        }
    }

    static std::string escape_string(const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }

    static void skip_whitespace(const std::string& s, size_t& pos) {
        while (pos < s.size() && std::isspace(s[pos])) pos++;
    }

    static json parse_value(const std::string& s, size_t& pos) {
        skip_whitespace(s, pos);
        if (pos >= s.size()) return json();

        char c = s[pos];
        if (c == '{') return parse_object(s, pos);
        if (c == '[') return parse_array(s, pos);
        if (c == '"') return parse_string(s, pos);
        if (c == 't' || c == 'f') return parse_bool(s, pos);
        if (c == 'n') return parse_null(s, pos);
        if (c == '-' || std::isdigit(c)) return parse_number(s, pos);
        
        throw std::runtime_error("Invalid JSON");
    }

    static json parse_object(const std::string& s, size_t& pos) {
        json obj = json::object();
        pos++; // skip '{'
        skip_whitespace(s, pos);
        
        if (s[pos] == '}') { pos++; return obj; }
        
        while (true) {
            skip_whitespace(s, pos);
            std::string key = parse_string(s, pos).get<std::string>();
            skip_whitespace(s, pos);
            if (s[pos] != ':') throw std::runtime_error("Expected ':'");
            pos++;
            obj[key] = parse_value(s, pos);
            skip_whitespace(s, pos);
            if (s[pos] == '}') { pos++; break; }
            if (s[pos] != ',') throw std::runtime_error("Expected ',' or '}'");
            pos++;
        }
        return obj;
    }

    static json parse_array(const std::string& s, size_t& pos) {
        json arr = json::array();
        pos++; // skip '['
        skip_whitespace(s, pos);
        
        if (s[pos] == ']') { pos++; return arr; }
        
        while (true) {
            arr.push_back(parse_value(s, pos));
            skip_whitespace(s, pos);
            if (s[pos] == ']') { pos++; break; }
            if (s[pos] != ',') throw std::runtime_error("Expected ',' or ']'");
            pos++;
        }
        return arr;
    }

    static json parse_string(const std::string& s, size_t& pos) {
        pos++; // skip '"'
        std::string result;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\' && pos + 1 < s.size()) {
                pos++;
                switch (s[pos]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: result += s[pos]; break;
                }
            } else {
                result += s[pos];
            }
            pos++;
        }
        pos++; // skip closing '"'
        return json(result);
    }

    static json parse_number(const std::string& s, size_t& pos) {
        size_t start = pos;
        bool is_float = false;
        if (s[pos] == '-') pos++;
        while (pos < s.size() && std::isdigit(s[pos])) pos++;
        if (pos < s.size() && s[pos] == '.') {
            is_float = true;
            pos++;
            while (pos < s.size() && std::isdigit(s[pos])) pos++;
        }
        if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
            is_float = true;
            pos++;
            if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) pos++;
            while (pos < s.size() && std::isdigit(s[pos])) pos++;
        }
        std::string num_str = s.substr(start, pos - start);
        if (is_float) return json(std::stod(num_str));
        return json(std::stoll(num_str));
    }

    static json parse_bool(const std::string& s, size_t& pos) {
        if (s.substr(pos, 4) == "true") { pos += 4; return json(true); }
        if (s.substr(pos, 5) == "false") { pos += 5; return json(false); }
        throw std::runtime_error("Invalid boolean");
    }

    static json parse_null(const std::string& s, size_t& pos) {
        if (s.substr(pos, 4) == "null") { pos += 4; return json(); }
        throw std::runtime_error("Invalid null");
    }
};

} // namespace nlohmann

#endif // NLOHMANN_JSON_HPP
