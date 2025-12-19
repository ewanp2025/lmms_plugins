#pragma once
#include "Effect.h"
#include "EffectControls.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include "MasteringSuiteControls.h"

namespace lmms {

struct CurvePoint {
    float x, y; 
    bool operator<(const CurvePoint& other) const { return x < other.x; }
};

// --- ROBUST BIQUAD ENGINE ---
class Biquad {
public:
    float b0=0, b1=0, b2=0, a1=0, a2=0;
    float x1=0, x2=0, y1=0, y2=0;
    
    // Copy Constructor: Only copy coefficients, NOT state (x1,y1)
    Biquad& operator=(const Biquad& other) {
        b0=other.b0; b1=other.b1; b2=other.b2; a1=other.a1; a2=other.a2;
        return *this;
    }

    void calc(float sr, float freq, int type) {
        if (freq < 10.0f) freq = 10.0f;
        if (freq > sr * 0.49f) freq = sr * 0.49f;
        float w0 = 2.0f * 3.14159265f * freq / sr;
        float alpha = std::sin(w0) / (2.0f * 0.7071f);
        float c = std::cos(w0);
        float norm = 1.0f / (1.0f + alpha);
        if (type == 0) { // LP
            b0 = ((1.0f - c) * 0.5f) * norm; b1 = (1.0f - c) * norm; b2 = ((1.0f - c) * 0.5f) * norm;
            a1 = (-2.0f * c) * norm; a2 = (1.0f - alpha) * norm;
        } else { // HP
            b0 = ((1.0f + c) * 0.5f) * norm; b1 = -(1.0f + c) * norm; b2 = ((1.0f + c) * 0.5f) * norm;
            a1 = (-2.0f * c) * norm; a2 = (1.0f - alpha) * norm;
        }
    }
    inline float process(float in) {
        float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        if (!std::isfinite(out) || std::abs(out) < 1e-20f) out = 0.0f;
        x2 = x1; x1 = in; y2 = y1; y1 = out;
        return out;
    }
    void reset() { x1=0; x2=0; y1=0; y2=0; }
};

// --- STEREO LINKWITZ-RILEY 4 (24dB/oct) ---
class StereoLR4 {
public:
    Biquad l1, l2, r1, r2; 
    void calc(float sr, float freq, int type) { 
        l1.calc(sr, freq, type); l2 = l1; r1 = l1; r2 = l1; 
    }
    void process(float& l, float& r) { 
        l = l2.process(l1.process(l)); 
        r = r2.process(r1.process(r)); 
    }
    void reset() { l1.reset(); l2.reset(); r1.reset(); r2.reset(); }
};

// --- COMPRESSOR MODULE ---
class CompModule {
public:
    std::vector<CurvePoint> m_points;
    std::vector<float> m_lut; 
    float m_sr=44100;
    float m_env=0;
    float m_rmsSum=0;
    float m_currentGR = 0.0f; 

    CompModule() {
        m_points = { {0.0f, 0.0f}, {1.0f, 1.0f} };
        m_lut.resize(4096);
        updateLUT();
    }

    void init(float sr) { m_sr = sr; m_env = 0; m_rmsSum = 0; m_currentGR = 0; }

    void updateLUT() {
        if(m_points.empty()) return;
        std::sort(m_points.begin(), m_points.end());
        for(size_t i=0; i < m_lut.size(); ++i) {
            float x = (float)i / (m_lut.size() - 1);
            CurvePoint p0 = m_points[0], p1 = m_points[0];
            bool found = false;
            for(const auto& p : m_points) {
                if (p.x >= x) { p1 = p; found = true; break; }
                p0 = p;
            }
            if(!found) p1 = p0; 
            float range = p1.x - p0.x;
            if (range < 0.0001f) {
                m_lut[i] = p1.y;
            } else {
                if (m_points.size() <= 2) {
                    float t = (x - p0.x) / range;
                    m_lut[i] = p0.y + t * (p1.y - p0.y);
                } else {
                    float t = (x - p0.x) / range;
                    float ft = t * 3.14159265f;
                    float f = (1.0f - std::cos(ft)) * 0.5f;
                    m_lut[i] = p0.y * (1.0f - f) + p1.y * f;
                }
            }
        }
    }

    void process(float& l, float& r, FloatModel& att, FloatModel& rel, FloatModel& gain, bool useRMS) {
        for(int k=0; k<2; ++k) { 
            float inL = l; float inR = r; 
            float level = 0.0f;
            if(useRMS) {
                m_rmsSum = m_rmsSum * 0.999f + (inL*inL + inR*inR) * 0.001f;
                level = std::sqrt(m_rmsSum);
            } else {
                level = std::max(std::abs(inL), std::abs(inR));
            }

            float inDB = 20.f * std::log10(level + 1e-6f);
            float x = (inDB + 60.0f) / 60.0f;
            
            // FIX: Use std::clamp logic to avoid -Wmisleading-indentation error
            x = std::max(0.0f, std::min(x, 1.0f));

            int idx = (int)(x * (m_lut.size() - 1));
            float targetY = m_lut[idx];
            float targetDB = (targetY * 60.0f) - 60.0f;
            float gainChangeDB = targetDB - inDB;
            
            float effSr = m_sr * 2.0f; 
            float atC = std::exp(-1.f/(att.value()*0.001f*effSr));
            float reC = std::exp(-1.f/(rel.value()*0.001f*effSr));
            
            if(gainChangeDB < m_env) m_env = atC * m_env + (1-atC)*gainChangeDB;
            else                     m_env = reC * m_env + (1-reC)*gainChangeDB;
            
            m_currentGR = m_env; 

            float totalGain = std::pow(10.f, (m_env + gain.value()) / 20.f);
            l *= totalGain; r *= totalGain;
        }
    }
};

class MasteringSuiteEffect : public Effect {
public:
    MasteringSuiteEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
    virtual ~MasteringSuiteEffect() = default;
    
    EffectControls* controls() override { return &m_controls; }
    ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;

    CompModule m_compLow, m_compMid, m_compHigh, m_compMaster;
    
    StereoLR4 m_globalLoCut, m_globalHiCut; 
    StereoLR4 m_crLowLP, m_crLowHP;
    StereoLR4 m_crHighLP, m_crHighHP;
    
    std::vector<float> m_scopeInL, m_scopeInR;
    std::vector<float> m_scopeOutL, m_scopeOutR;

    MasteringSuiteControls m_controls;

    std::vector<float> m_lookaheadBufL, m_lookaheadBufR;
    int m_bufferPos = 0;
    float m_limiterEnv = 1.0f;
};

} // namespace lmms
