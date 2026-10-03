// Maintains M = K_A^{-1} for a set A of indices of a fixed SPD matrix K under single-element
// insertions and deletions (O(|A|^2) each, via the block-inverse / Schur-complement formulas).
// Used by the log-determinant and Gaussian mutual-information objectives:
//   cond_var(v) = K_vv - K_vA K_A^{-1} K_Av        for v not in A   ( = det K_{A+v} / det K_A )
//   inv_diag(v) = [K_A^{-1}]_vv                     for v in A       ( = det K_{A-v} / det K_A )
// To bound floating-point drift from long add/remove sequences (high_cap_U performs O(n^2)),
// M is recomputed from scratch by Cholesky every `refresh_every` updates.
#pragma once
#include <cmath>
#include <stdexcept>
#include <vector>

namespace dual {

struct Dense {
    int n = 0;
    std::vector<double> a;
    Dense() = default;
    explicit Dense(int n_) : n(n_), a((size_t)n_ * n_, 0.0) {}
    double& operator()(int i, int j) { return a[(size_t)i * n + j]; }
    double operator()(int i, int j) const { return a[(size_t)i * n + j]; }
};

// log det of the principal submatrix K_A (Cholesky); -inf if not positive definite
inline double logdet_sub(const Dense& K, const std::vector<int>& A) {
    int s = A.size();
    std::vector<double> L((size_t)s * s, 0.0);
    double ld = 0;
    for (int i = 0; i < s; i++)
        for (int j = 0; j <= i; j++) {
            double v = K(A[i], A[j]);
            for (int t = 0; t < j; t++) v -= L[(size_t)i * s + t] * L[(size_t)j * s + t];
            if (i == j) {
                if (v <= 0) return -INFINITY;
                L[(size_t)i * s + i] = std::sqrt(v);
                ld += std::log(v);
            } else {
                L[(size_t)i * s + j] = v / L[(size_t)j * s + j];
            }
        }
    return ld;
}

class InverseSet {
public:
    InverseSet() = default;
    InverseSet(const Dense* K, int refresh_every = 256)
        : K_(K), cap_(K->n), pos_(K->n, -1), M_((size_t)K->n * K->n, 0.0), refresh_every_(refresh_every) {}

    bool contains(int v) const { return pos_[v] >= 0; }
    int size() const { return (int)items_.size(); }
    const std::vector<int>& items() const { return items_; }

    double cond_var(int v) const {
        int s = items_.size();
        double c = (*K_)(v, v);
        if (s == 0) return c;
        // c -= k^T M k with k = K_{A,v}
        for (int p = 0; p < s; p++) {
            double kp = (*K_)(items_[p], v);
            if (kp == 0) continue;
            const double* row = &M_[(size_t)p * cap_];
            double acc = 0;
            for (int q = 0; q < s; q++) acc += row[q] * (*K_)(items_[q], v);
            c -= kp * acc;
        }
        return c;
    }

    double inv_diag(int v) const { return M_[(size_t)pos_[v] * cap_ + pos_[v]]; }

    void add(int v) {
        int s = items_.size();
        std::vector<double> b(s, 0.0);  // b = M k
        for (int p = 0; p < s; p++) {
            const double* row = &M_[(size_t)p * cap_];
            double acc = 0;
            for (int q = 0; q < s; q++) acc += row[q] * (*K_)(items_[q], v);
            b[p] = acc;
        }
        double c = (*K_)(v, v);
        for (int p = 0; p < s; p++) c -= (*K_)(items_[p], v) * b[p];
        if (!(c > 0)) throw std::runtime_error("InverseSet::add: matrix not positive definite");
        for (int p = 0; p < s; p++)
            for (int q = 0; q < s; q++) M_[(size_t)p * cap_ + q] += b[p] * b[q] / c;
        for (int p = 0; p < s; p++) {
            M_[(size_t)p * cap_ + s] = -b[p] / c;
            M_[(size_t)s * cap_ + p] = -b[p] / c;
        }
        M_[(size_t)s * cap_ + s] = 1.0 / c;
        pos_[v] = s;
        items_.push_back(v);
        tick();
    }

    void remove(int v) {
        int s = items_.size(), p0 = pos_[v], last = s - 1;
        // move v to the last slot (swap rows/cols p0 <-> last), then drop it
        if (p0 != last) {
            for (int q = 0; q < s; q++) std::swap(M_[(size_t)p0 * cap_ + q], M_[(size_t)last * cap_ + q]);
            for (int q = 0; q < s; q++) std::swap(M_[(size_t)q * cap_ + p0], M_[(size_t)q * cap_ + last]);
            int u = items_[last];
            items_[p0] = u;
            pos_[u] = p0;
        }
        double d = M_[(size_t)last * cap_ + last];
        for (int p = 0; p < last; p++) {
            double bp = M_[(size_t)p * cap_ + last];
            if (bp == 0) continue;
            for (int q = 0; q < last; q++) M_[(size_t)p * cap_ + q] -= bp * M_[(size_t)last * cap_ + q] / d;
        }
        items_.pop_back();
        pos_[v] = -1;
        tick();
    }

    // recompute M = K_A^{-1} from scratch (Cholesky, then invert)
    void refresh() {
        int s = items_.size();
        if (s == 0) return;
        std::vector<double> L((size_t)s * s, 0.0);
        for (int i = 0; i < s; i++)
            for (int j = 0; j <= i; j++) {
                double v = (*K_)(items_[i], items_[j]);
                for (int t = 0; t < j; t++) v -= L[(size_t)i * s + t] * L[(size_t)j * s + t];
                if (i == j) {
                    if (!(v > 0)) throw std::runtime_error("InverseSet::refresh: not positive definite");
                    L[(size_t)i * s + i] = std::sqrt(v);
                } else {
                    L[(size_t)i * s + j] = v / L[(size_t)j * s + j];
                }
            }
        // Linv (lower triangular)
        std::vector<double> Li((size_t)s * s, 0.0);
        for (int i = 0; i < s; i++) {
            Li[(size_t)i * s + i] = 1.0 / L[(size_t)i * s + i];
            for (int j = 0; j < i; j++) {
                double acc = 0;
                for (int t = j; t < i; t++) acc += L[(size_t)i * s + t] * Li[(size_t)t * s + j];
                Li[(size_t)i * s + j] = -acc / L[(size_t)i * s + i];
            }
        }
        // M = Linv^T Linv
        for (int p = 0; p < s; p++)
            for (int q = 0; q <= p; q++) {
                double acc = 0;
                for (int t = p; t < s; t++) acc += Li[(size_t)t * s + p] * Li[(size_t)t * s + q];
                M_[(size_t)p * cap_ + q] = M_[(size_t)q * cap_ + p] = acc;
            }
        updates_ = 0;
    }

private:
    void tick() {
        if (refresh_every_ > 0 && ++updates_ >= refresh_every_) refresh();
    }

    const Dense* K_ = nullptr;
    int cap_ = 0;
    std::vector<int> items_, pos_;
    std::vector<double> M_;
    int refresh_every_ = 256, updates_ = 0;
};

}  // namespace dual
