#ifndef GCD_SOLVER_HPP
#define GCD_SOLVER_HPP

#include <vector>
#include <algorithm>

/**
 * 高效求解：给定序列中所有不同 gcd(ai, aj) (i != j) 的个数
 * 
 * 算法思路（优化版）：
 * 1. 统计每个数的出现次数 cnt[x]
 * 2. 计算每个 g 的倍数出现次数 mul[g] = Σ cnt[k*g]
 * 3. 使用容斥原理：如果 mul[g] >= 2，且不存在更大的 g' (g'是g的倍数) 使得 mul[g'] >= 2 且所有 g 的倍数的数的 gcd > g
 * 4. 从大到小枚举 g，用 DP 方式判断 g 是否为有效 gcd
 * 
 * 时间复杂度：O(M log M)
 * 空间复杂度：O(M)
 */
class GcdSolver {
public:
    struct Result {
        int count;
        std::vector<int> distinct_gcds;
    };

    static Result solve(const std::vector<int>& arr) {
        if (arr.size() < 2) {
            return {0, {}};
        }

        int max_val = *std::max_element(arr.begin(), arr.end());

        // cnt[i] = 值 i 在数组中出现的次数
        std::vector<int> cnt(max_val + 1, 0);
        for (int x : arr) {
            if (x >= 1 && x <= max_val) {
                cnt[x]++;
            }
        }

        // mul[g] = g 的倍数在数组中出现的总次数
        std::vector<int> mul(max_val + 1, 0);
        for (int g = 1; g <= max_val; g++) {
            for (int m = g; m <= max_val; m += g) {
                mul[g] += cnt[m];
            }
        }

        // f[g] = 所有是 g 的倍数的数的 gcd 除以 g 后的值
        // 如果 f[g] == 1，说明存在两个数的 gcd 恰好为 g
        // 使用 DP：从大到小计算，f[g] = gcd(所有 cnt[k*g] > 0 的 k 值)
        
        std::vector<int> result_gcds;
        
        // 对于每个 g，计算是否存在两个数的 gcd 恰好为 g
        // 方法：计算所有 g 的倍数除以 g 后的 gcd
        // 如果这个 gcd 为 1，则 g 是一个有效的 gcd 值
        
        for (int g = 1; g <= max_val; g++) {
            if (mul[g] < 2) continue;
            
            // 计算所有 g 的倍数（在数组中出现的）除以 g 后的 gcd
            int overall_gcd = 0;
            int total_count = 0;
            
            for (int m = g; m <= max_val; m += g) {
                if (cnt[m] > 0) {
                    int k = m / g;
                    // 如果同一个数出现多次，gcd(k, k) = k
                    // 但我们需要的是不同位置的数
                    if (cnt[m] >= 2) {
                        // 同一个数出现两次，gcd = k
                        overall_gcd = gcd(overall_gcd, k);
                        total_count += cnt[m];
                    } else {
                        overall_gcd = gcd(overall_gcd, k);
                        total_count += cnt[m];
                    }
                }
            }
            
            // 如果只有一种数（但出现多次），gcd 就是那个数除以 g
            // 如果有多种数，gcd 是它们除以 g 后的 gcd
            // 只有当 overall_gcd == 1 时，g 才是有效的 gcd
            
            // 特殊情况：如果某个数 m = g 出现了至少 2 次
            // 那么 gcd(m, m) = m = g，所以 g 是有效的
            if (cnt[g] >= 2) {
                result_gcds.push_back(g);
                continue;
            }
            
            // 否则需要检查是否存在两个不同的数，它们的 gcd 恰好为 g
            // 即：存在 m1, m2 (m1 != m2)，gcd(m1, m2) = g
            // 等价于：gcd(m1/g, m2/g) = 1
            
            // 收集所有出现的 k = m/g
            std::vector<int> ks;
            for (int m = g; m <= max_val; m += g) {
                if (cnt[m] > 0) {
                    ks.push_back(m / g);
                }
            }
            
            if (ks.size() < 2 && cnt[g] < 2) continue;
            
            // 检查是否存在两个 k 互质
            bool found = false;
            
            // 优化：如果 ks 中有 1，则一定存在互质对
            for (int k : ks) {
                if (k == 1 && mul[g] >= 2) {
                    found = true;
                    break;
                }
            }
            
            if (!found && ks.size() >= 2) {
                // 检查所有 k 的 gcd 是否为 1
                int g_all = 0;
                for (int k : ks) {
                    g_all = gcd(g_all, k);
                    if (g_all == 1) {
                        found = true;
                        break;
                    }
                }
            }
            
            if (found) {
                result_gcds.push_back(g);
            }
        }

        std::sort(result_gcds.begin(), result_gcds.end());
        return {static_cast<int>(result_gcds.size()), result_gcds};
    }

private:
    static int gcd(int a, int b) {
        while (b) {
            a %= b;
            std::swap(a, b);
        }
        return a;
    }
};

#endif // GCD_SOLVER_HPP
