# GCD Solver Service

## How to Run

```bash
# 使用 Docker Compose 启动
docker-compose up -d

# 或本地编译运行
cd backend
make
./gcd_server 8080
```

## Services

| 服务 | 端口 | 说明 |
|------|------|------|
| gcd-solver | 8080 | GCD 计算服务 |

## 测试账号

本服务为无状态 API，无需登录认证。

## 题目内容

给定一个长度为 n 的序列 {a₁, a₂, ..., aₙ}，求有多少个不同的 gcd(aᵢ, aⱼ) (i ≠ j)

**数据范围：**
- 1 ≤ n ≤ 2×10⁵
- 1 ≤ aᵢ ≤ 10⁷

## 项目介绍

基于 C++ 实现的高性能 GCD 计算后端服务，提供 RESTful API 接口。

### 算法复杂度

- 时间复杂度：O(M log M) 平均，最坏情况 O(M log M + n²) 取决于输入分布
- 空间复杂度：O(M)，M 为数组最大值

**内存说明：** 当 max_val 接近 10⁷ 时，内存占用约 80-120 MB（cnt/mul/can 数组各约 40MB）。

### API 接口

**POST /api/gcd/distinct-count**

请求：
```json
{
  "array": [2, 4, 6, 8]
}
```

响应：
```json
{
  "success": true,
  "data": {
    "count": 2,
    "distinct_gcds": [2, 4],
    "time_ms": 0.15
  }
}
```

**GET /api/health**

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

### 测试示例

**示例 1：基本测试**
```bash
curl -X POST http://localhost:8080/api/gcd/distinct-count \
  -H "Content-Type: application/json" \
  -d '{"array": [2, 4, 6, 8]}'
```
响应：
```json
{"success":true,"data":{"count":2,"distinct_gcds":[2,4],"time_ms":0.002}}
```
说明：gcd(2,4)=2, gcd(2,6)=2, gcd(2,8)=2, gcd(4,6)=2, gcd(4,8)=4, gcd(6,8)=2，不同值为 {2, 4}
