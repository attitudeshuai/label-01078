//
//  httplib.h
//
//  Copyright (c) 2024 Yuji Hirose. All rights reserved.
//  MIT License - https://github.com/yhirose/cpp-httplib
//

#ifndef CPPHTTPLIB_HTTPLIB_H
#define CPPHTTPLIB_HTTPLIB_H

#define CPPHTTPLIB_VERSION "0.15.3"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cctype>
#include <climits>
#include <condition_variable>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#if defined(_MSC_VER)
#if _MSC_VER < 1900
#error Sorry, Visual Studio versions prior to 2015 are not supported
#endif

#pragma comment(lib, "ws2_32.lib")

#ifdef _WIN64
using ssize_t = __int64;
#else
using ssize_t = long;
#endif
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m)&S_IFREG) == S_IFREG)
#endif

#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFDIR) == S_IFDIR)
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef WSA_FLAG_NO_HANDLE_INHERIT
#define WSA_FLAG_NO_HANDLE_INHERIT 0x80
#endif

using socket_t = SOCKET;

#else // not _WIN32

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using socket_t = int;
#define INVALID_SOCKET (-1)
#endif

namespace httplib {

namespace detail {

struct ci {
  bool operator()(const std::string &s1, const std::string &s2) const {
    return std::lexicographical_compare(
        s1.begin(), s1.end(), s2.begin(), s2.end(),
        [](unsigned char c1, unsigned char c2) {
          return ::tolower(c1) < ::tolower(c2);
        });
  }
};

} // namespace detail

using Headers = std::multimap<std::string, std::string, detail::ci>;
using Params = std::multimap<std::string, std::string>;
using Match = std::smatch;

using Progress = std::function<bool(uint64_t current, uint64_t total)>;

struct Response;
using ResponseHandler = std::function<bool(const Response &response)>;

struct MultipartFormData {
  std::string name;
  std::string content;
  std::string filename;
  std::string content_type;
};
using MultipartFormDataItems = std::vector<MultipartFormData>;
using MultipartFormDataMap = std::multimap<std::string, MultipartFormData>;

class DataSink {
public:
  DataSink() : os(&sb_), sb_(*this) {}
  DataSink(const DataSink &) = delete;
  DataSink &operator=(const DataSink &) = delete;
  DataSink(DataSink &&) = delete;
  DataSink &operator=(DataSink &&) = delete;

  std::function<bool(const char *data, size_t data_len)> write;
  std::function<void()> done;
  std::function<bool()> is_writable;
  std::ostream os;

private:
  class data_sink_streambuf : public std::streambuf {
  public:
    explicit data_sink_streambuf(DataSink &sink) : sink_(sink) {}

  protected:
    std::streamsize xsputn(const char *s, std::streamsize n) override {
      sink_.write(s, static_cast<size_t>(n));
      return n;
    }

  private:
    DataSink &sink_;
  };

  data_sink_streambuf sb_;
};

using ContentProvider =
    std::function<bool(size_t offset, size_t length, DataSink &sink)>;

using ContentProviderWithoutLength =
    std::function<bool(size_t offset, DataSink &sink)>;

using ContentProviderResourceReleaser = std::function<void(bool success)>;

struct Request {
  std::string method;
  std::string path;
  Headers headers;
  std::string body;
  std::string remote_addr;
  int remote_port = -1;
  std::string local_addr;
  int local_port = -1;

  Params params;
  MultipartFormDataMap files;
  Match matches;

  bool has_header(const std::string &key) const;
  std::string get_header_value(const std::string &key, size_t id = 0) const;
  size_t get_header_value_count(const std::string &key) const;
  void set_header(const std::string &key, const std::string &val);

  bool has_param(const std::string &key) const;
  std::string get_param_value(const std::string &key, size_t id = 0) const;
  size_t get_param_value_count(const std::string &key) const;
};

struct Response {
  std::string version;
  int status = -1;
  std::string reason;
  Headers headers;
  std::string body;
  std::string location;

  bool has_header(const std::string &key) const;
  std::string get_header_value(const std::string &key, size_t id = 0) const;
  size_t get_header_value_count(const std::string &key) const;
  void set_header(const std::string &key, const std::string &val);

  void set_content(const char *s, size_t n, const std::string &content_type);
  void set_content(const std::string &s, const std::string &content_type);
};

class Server {
public:
  using Handler = std::function<void(const Request &, Response &)>;

  Server();
  virtual ~Server();

  Server &Get(const std::string &pattern, Handler handler);
  Server &Post(const std::string &pattern, Handler handler);
  Server &Put(const std::string &pattern, Handler handler);
  Server &Patch(const std::string &pattern, Handler handler);
  Server &Delete(const std::string &pattern, Handler handler);
  Server &Options(const std::string &pattern, Handler handler);

  bool listen(const std::string &host, int port, int socket_flags = 0);
  bool is_running() const;
  void stop();

private:
  class ServerImpl;
  std::unique_ptr<ServerImpl> impl_;
};

// Implementation details below

inline bool Request::has_header(const std::string &key) const {
  return headers.find(key) != headers.end();
}

inline std::string Request::get_header_value(const std::string &key,
                                              size_t id) const {
  auto rng = headers.equal_range(key);
  auto it = rng.first;
  std::advance(it, static_cast<ssize_t>(id));
  if (it != rng.second) { return it->second; }
  return std::string();
}

inline size_t Request::get_header_value_count(const std::string &key) const {
  return headers.count(key);
}

inline void Request::set_header(const std::string &key,
                                 const std::string &val) {
  headers.emplace(key, val);
}

inline bool Request::has_param(const std::string &key) const {
  return params.find(key) != params.end();
}

inline std::string Request::get_param_value(const std::string &key,
                                             size_t id) const {
  auto rng = params.equal_range(key);
  auto it = rng.first;
  std::advance(it, static_cast<ssize_t>(id));
  if (it != rng.second) { return it->second; }
  return std::string();
}

inline size_t Request::get_param_value_count(const std::string &key) const {
  return params.count(key);
}

inline bool Response::has_header(const std::string &key) const {
  return headers.find(key) != headers.end();
}

inline std::string Response::get_header_value(const std::string &key,
                                               size_t id) const {
  auto rng = headers.equal_range(key);
  auto it = rng.first;
  std::advance(it, static_cast<ssize_t>(id));
  if (it != rng.second) { return it->second; }
  return std::string();
}

inline size_t Response::get_header_value_count(const std::string &key) const {
  return headers.count(key);
}

inline void Response::set_header(const std::string &key,
                                  const std::string &val) {
  headers.emplace(key, val);
}

inline void Response::set_content(const char *s, size_t n,
                                   const std::string &content_type) {
  body.assign(s, n);
  set_header("Content-Type", content_type);
}

inline void Response::set_content(const std::string &s,
                                   const std::string &content_type) {
  body = s;
  set_header("Content-Type", content_type);
}

// Server Implementation
class Server::ServerImpl {
public:
  using Handler = Server::Handler;

  ServerImpl() : is_running_(false), svr_sock_(INVALID_SOCKET) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
  }

  ~ServerImpl() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
  }

  void Get(const std::string &pattern, Handler handler) {
    get_handlers_.emplace_back(std::regex(pattern), std::move(handler));
  }

  void Post(const std::string &pattern, Handler handler) {
    post_handlers_.emplace_back(std::regex(pattern), std::move(handler));
  }

  void Put(const std::string &pattern, Handler handler) {
    put_handlers_.emplace_back(std::regex(pattern), std::move(handler));
  }

  void Patch(const std::string &pattern, Handler handler) {
    patch_handlers_.emplace_back(std::regex(pattern), std::move(handler));
  }

  void Delete(const std::string &pattern, Handler handler) {
    delete_handlers_.emplace_back(std::regex(pattern), std::move(handler));
  }

  void Options(const std::string &pattern, Handler handler) {
    options_handlers_.emplace_back(std::regex(pattern), std::move(handler));
  }

  bool listen(const std::string &host, int port, int socket_flags) {
    svr_sock_ = create_server_socket(host, port, socket_flags);
    if (svr_sock_ == INVALID_SOCKET) { return false; }

    is_running_ = true;

    while (is_running_) {
      auto client_sock = accept(svr_sock_, nullptr, nullptr);
      if (client_sock == INVALID_SOCKET) {
        if (is_running_) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        continue;
      }

      std::thread([this, client_sock]() {
        process_request(client_sock);
        close_socket(client_sock);
      }).detach();
    }

    return true;
  }

  bool is_running() const { return is_running_; }

  void stop() {
    is_running_ = false;
    if (svr_sock_ != INVALID_SOCKET) {
      close_socket(svr_sock_);
      svr_sock_ = INVALID_SOCKET;
    }
  }

private:
  socket_t create_server_socket(const std::string &host, int port, int) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    auto port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0) {
      return INVALID_SOCKET;
    }

    auto sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
      freeaddrinfo(res);
      return INVALID_SOCKET;
    }

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(yes));

    if (bind(sock, res->ai_addr, static_cast<int>(res->ai_addrlen)) != 0) {
      close_socket(sock);
      freeaddrinfo(res);
      return INVALID_SOCKET;
    }

    freeaddrinfo(res);

    if (::listen(sock, SOMAXCONN) != 0) {
      close_socket(sock);
      return INVALID_SOCKET;
    }

    return sock;
  }

  void close_socket(socket_t sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
  }

  void process_request(socket_t client_sock) {
    char buf[8192];
    std::string request_data;

    while (true) {
      auto n = recv(client_sock, buf, sizeof(buf), 0);
      if (n <= 0) break;
      request_data.append(buf, n);
      if (request_data.find("\r\n\r\n") != std::string::npos) break;
    }

    if (request_data.empty()) return;

    Request req;
    Response res;
    res.status = 200;

    // Parse request line
    auto line_end = request_data.find("\r\n");
    if (line_end == std::string::npos) return;

    auto request_line = request_data.substr(0, line_end);
    std::istringstream iss(request_line);
    iss >> req.method >> req.path;

    // Parse headers
    auto headers_start = line_end + 2;
    auto headers_end = request_data.find("\r\n\r\n");
    if (headers_end != std::string::npos) {
      auto headers_str = request_data.substr(headers_start, headers_end - headers_start);
      std::istringstream hss(headers_str);
      std::string header_line;
      while (std::getline(hss, header_line) && !header_line.empty()) {
        if (header_line.back() == '\r') header_line.pop_back();
        auto colon = header_line.find(':');
        if (colon != std::string::npos) {
          auto key = header_line.substr(0, colon);
          auto val = header_line.substr(colon + 1);
          while (!val.empty() && val[0] == ' ') val.erase(0, 1);
          req.headers.emplace(key, val);
        }
      }

      // Get body
      auto body_start = headers_end + 4;
      if (body_start < request_data.size()) {
        auto content_length_it = req.headers.find("Content-Length");
        if (content_length_it != req.headers.end()) {
          auto content_length = std::stoul(content_length_it->second);
          while (request_data.size() - body_start < content_length) {
            auto n = recv(client_sock, buf, sizeof(buf), 0);
            if (n <= 0) break;
            request_data.append(buf, n);
          }
        }
        req.body = request_data.substr(body_start);
      }
    }

    // Route request
    bool handled = false;
    std::vector<std::pair<std::regex, Handler>> *handlers = nullptr;

    if (req.method == "GET") handlers = &get_handlers_;
    else if (req.method == "POST") handlers = &post_handlers_;
    else if (req.method == "PUT") handlers = &put_handlers_;
    else if (req.method == "PATCH") handlers = &patch_handlers_;
    else if (req.method == "DELETE") handlers = &delete_handlers_;
    else if (req.method == "OPTIONS") handlers = &options_handlers_;

    if (handlers) {
      for (auto &h : *handlers) {
        std::smatch m;
        if (std::regex_match(req.path, m, h.first)) {
          req.matches = m;
          h.second(req, res);
          handled = true;
          break;
        }
      }
    }

    if (!handled) {
      res.status = 404;
      res.body = "Not Found";
    }

    // Send response
    std::ostringstream oss;
    oss << "HTTP/1.1 " << res.status << " ";
    switch (res.status) {
      case 200: oss << "OK"; break;
      case 400: oss << "Bad Request"; break;
      case 404: oss << "Not Found"; break;
      case 500: oss << "Internal Server Error"; break;
      default: oss << "Unknown"; break;
    }
    oss << "\r\n";

    res.set_header("Content-Length", std::to_string(res.body.size()));
    res.set_header("Connection", "close");

    for (auto &h : res.headers) {
      oss << h.first << ": " << h.second << "\r\n";
    }
    oss << "\r\n";
    oss << res.body;

    auto response_str = oss.str();
    send(client_sock, response_str.c_str(), static_cast<int>(response_str.size()), 0);
  }

  std::atomic<bool> is_running_;
  socket_t svr_sock_;
  std::vector<std::pair<std::regex, Handler>> get_handlers_;
  std::vector<std::pair<std::regex, Handler>> post_handlers_;
  std::vector<std::pair<std::regex, Handler>> put_handlers_;
  std::vector<std::pair<std::regex, Handler>> patch_handlers_;
  std::vector<std::pair<std::regex, Handler>> delete_handlers_;
  std::vector<std::pair<std::regex, Handler>> options_handlers_;
};

inline Server::Server() : impl_(new ServerImpl()) {}
inline Server::~Server() = default;

inline Server &Server::Get(const std::string &pattern, Handler handler) {
  impl_->Get(pattern, std::move(handler));
  return *this;
}

inline Server &Server::Post(const std::string &pattern, Handler handler) {
  impl_->Post(pattern, std::move(handler));
  return *this;
}

inline Server &Server::Put(const std::string &pattern, Handler handler) {
  impl_->Put(pattern, std::move(handler));
  return *this;
}

inline Server &Server::Patch(const std::string &pattern, Handler handler) {
  impl_->Patch(pattern, std::move(handler));
  return *this;
}

inline Server &Server::Delete(const std::string &pattern, Handler handler) {
  impl_->Delete(pattern, std::move(handler));
  return *this;
}

inline Server &Server::Options(const std::string &pattern, Handler handler) {
  impl_->Options(pattern, std::move(handler));
  return *this;
}

inline bool Server::listen(const std::string &host, int port, int socket_flags) {
  return impl_->listen(host, port, socket_flags);
}

inline bool Server::is_running() const { return impl_->is_running(); }

inline void Server::stop() { impl_->stop(); }

} // namespace httplib

#endif // CPPHTTPLIB_HTTPLIB_H
