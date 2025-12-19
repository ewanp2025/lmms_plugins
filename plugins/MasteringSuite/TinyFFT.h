#pragma once
#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>

namespace lmms {

class TinyFFT {
public:
    using Complex = std::complex<float>;
    
    // The "Cooly-Tukey" FFT Algorithm (Recursive)
    static void fft(std::vector<Complex>& x) {
        const size_t N = x.size();
        if (N <= 1) return;

        // Divide
        std::vector<Complex> even(N / 2);
        std::vector<Complex> odd(N / 2);
        for (size_t i = 0; i < N / 2; ++i) {
            even[i] = x[i * 2];
            odd[i]  = x[i * 2 + 1];
        }

        // Conquer
        fft(even);
        fft(odd);

        // Combine
        for (size_t k = 0; k < N / 2; ++k) {
            Complex t = std::polar(1.0f, -2.0f * (float)M_PI * k / N) * odd[k];
            x[k] = even[k] + t;
            x[k + N / 2] = even[k] - t;
        }
    }

    // Window Function (Hanning) to smooth the edges
    static float window(int i, int N) {
        return 0.5f * (1.0f - std::cos(2.0f * (float)M_PI * i / (N - 1)));
    }
};

}