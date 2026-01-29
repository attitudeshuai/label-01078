# GCD Solver Service

计算序列中所有不同 gcd(ai, aj) (i ≠ j) 的个数。

## 构建

### Windows (MinGW)
```bash
cd backend
mingw32-make
```

### Windows (CMake)
```bash
cd backend
build.bat
```

### Linux/macOS
```bash
cd backend
make
```

## 运行

```bash
./gcd_server [port]
# 默认端口: 8080
```

## API

### POST /api/gcd/distinct-count

请求:
```json
{
  "array": [2, 4, 6, 8]
}
```

响应:
```json
{
  "success": true,
  "data": {
    "count": 2,
    "distinct_gcds": [2, 4],
    "time_ms": 0.123
  }
}
```

### GET /api/health

响应:
```json
{
  "success": true,
  "data": {
    "status": "ok",
    "service": "gcd-solver",
    "version": "1.0.0"
  }
}
```

## 测试

```bash
curl -X POST http://localhost:8080/api/gcd/distinct-count \
  -H "Content-Type: application/json" \
  -d '{"array": [2, 3, 4, 6, 8, 12]}'
```
