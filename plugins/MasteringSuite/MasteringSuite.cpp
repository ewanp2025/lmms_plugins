#include "MasteringSuite.h"
#include "embed.h"
#include "plugin_export.h"
#include <cmath>
#include <algorithm>
namespace lmms {
extern "C" {
    Plugin::Descriptor PLUGIN_EXPORT MasteringSuite_plugin_descriptor = {
        LMMS_STRINGIFY(PLUGIN_NAME),
        "MasteringSuite",
        QT_TRANSLATE_NOOP("PluginBrowser", "Professional Mastering Channel"),
        "Ewan Pettigrew",
        0x0100,
        Plugin::Type::Effect,
        new PixmapLoader("lmms-plugin-logo"),
        nullptr,
        nullptr,
    };
}
MasteringSuiteEffect::MasteringSuiteEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) 
    : Effect(&MasteringSuite_plugin_descriptor, parent, key)
    , m_controls(this)
{
    float sr = Engine::audioEngine()->outputSampleRate();
    if(sr < 1.0f) sr = 44100.0f; 
    m_compLow.init(sr); m_compMid.init(sr); m_compHigh.init(sr); m_compMaster.init(sr);
 
    m_scopeInL.resize(512, 0.0f); m_scopeInR.resize(512, 0.0f);
    m_scopeOutL.resize(512, 0.0f); m_scopeOutR.resize(512, 0.0f);
    int lookaheadSize = 8192; 
    m_lookaheadBufL.resize(lookaheadSize, 0.0f); m_lookaheadBufR.resize(lookaheadSize, 0.0f);
 
    m_globalLoCut.reset(); m_globalHiCut.reset();
}
Effect::ProcessStatus MasteringSuiteEffect::processImpl(SampleFrame* buf, const fpp_t frames) {
    float sr = Engine::audioEngine()->outputSampleRate();
    if(sr < 44100.0f) sr = 44100.0f; 
    // UPDATE FILTER COEFFICIENTS
    m_globalLoCut.calc(sr, m_controls.m_lowCut.value(), 1); 
    m_globalHiCut.calc(sr, m_controls.m_highCut.value(), 0); 
    // Calculate Crossover Filters (0=LP, 1=HP)
    // Split 1: Low vs MidHigh
    m_crLowLP.calc(sr, m_controls.m_lowSplit.value(), 0);
    m_crLowHP.calc(sr, m_controls.m_lowSplit.value(), 1);
 
    // Split 2: Mid vs High
    m_crHighLP.calc(sr, m_controls.m_highSplit.value(), 0);
    m_crHighHP.calc(sr, m_controls.m_highSplit.value(), 1);
    auto applyWidth = [](float& l, float& r, float width) {
        if (std::abs(width - 1.0f) < 0.001f) return;
        float mid = (l + r) * 0.5f; float side = (l - r) * 0.5f;
        side *= width; l = mid + side; r = mid - side;
    };
    auto checkNaN = [&](float& val) { if (!std::isfinite(val)) val = 0.0f; };
    auto dbToLin = [](float db) { return std::pow(10.f, db/20.f); };
    const int bufSize = (int)m_lookaheadBufL.size();
    int lookaheadDist = (int)(0.005f * sr); 
    if (lookaheadDist > bufSize - 10) lookaheadDist = bufSize - 10;
  
    float ceilingLin = dbToLin(m_controls.m_mastCeil.value());
    if(ceilingLin > 0.99f) ceilingLin = 0.99f; 
    float preLow = dbToLin(m_controls.m_lowPre.value());
    float preMid = dbToLin(m_controls.m_midPre.value());
    float preHigh = dbToLin(m_controls.m_highPre.value());
    float preMast = dbToLin(m_controls.m_mastPre.value());
    bool soloL = m_controls.m_lowSolo.value();
    bool soloM = m_controls.m_midSolo.value();
    bool soloH = m_controls.m_highSolo.value();
    bool anySolo = soloL || soloM || soloH;
    bool useRMS = m_controls.m_rmsMode.value();
    for(fpp_t i = 0; i < frames; ++i) {
        float l = buf[i][0]; float r = buf[i][1];
        checkNaN(l); checkNaN(r);
        if(i < 512) { m_scopeInL[i] = l; m_scopeInR[i] = r; }
        m_globalLoCut.process(l, r); 
        m_globalHiCut.process(l, r); 
        // --- IIR CROSSOVER SPLITTING (Linkwitz-Riley) ---
        // 1. Copy signal for splitting
        float l_low = l, r_low = r;
        float l_midhigh = l, r_midhigh = r;
      
        // 2. Process first split (Low vs MidHigh)
        m_crLowLP.process(l_low, r_low);        // Becomes LOW
        m_crLowHP.process(l_midhigh, r_midhigh); // Becomes MID+HIGH
        // 3. Process second split (Mid vs High) using the MidHigh result
        float l_mid = l_midhigh, r_mid = r_midhigh;
        float l_high = l_midhigh, r_high = r_midhigh;
        m_crHighLP.process(l_mid, r_mid);   // Becomes MID
        m_crHighHP.process(l_high, r_high); // Becomes HIGH
        // --- PROCESSING ---
        l_low *= preLow; r_low *= preLow;
        l_mid *= preMid; r_mid *= preMid;
        l_high *= preHigh; r_high *= preHigh;
        // DYNAMICS
        m_compLow.process(l_low, r_low, m_controls.m_lowAtt, m_controls.m_lowRel, m_controls.m_lowGain, useRMS);
        m_compMid.process(l_mid, r_mid, m_controls.m_midAtt, m_controls.m_midRel, m_controls.m_midGain, useRMS);
        m_compHigh.process(l_high, r_high, m_controls.m_highAtt, m_controls.m_highRel, m_controls.m_highGain, useRMS);
        if(m_controls.m_lowSat.value() > 0) { float s=1+m_controls.m_lowSat.value(); l_low=std::tanh(l_low*s); r_low=std::tanh(r_low*s); }
        if(m_controls.m_midSat.value() > 0) { float s=1+m_controls.m_midSat.value(); l_mid=std::tanh(l_mid*s); r_mid=std::tanh(r_mid*s); }
        if(m_controls.m_highSat.value() > 0) { float s=1+m_controls.m_highSat.value(); l_high=std::tanh(l_high*s); r_high=std::tanh(r_high*s); }
        applyWidth(l_low, r_low, m_controls.m_lowWidth.value());
        applyWidth(l_mid, r_mid, m_controls.m_midWidth.value());
        applyWidth(l_high, r_high, m_controls.m_highWidth.value());
        if (anySolo) { if (!soloL) {l_low=0; r_low=0;} if (!soloM) {l_mid=0; r_mid=0;} if (!soloH) {l_high=0; r_high=0;} }
        // SUM
        l = l_low + l_mid + l_high; r = r_low + r_mid + r_high;
    
        // Note: Linkwitz-Riley filters sum perfectly flat in amplitude, 
        // but have a phase shift of 360 degrees. This is perfectly normal for plugins.
     
        l *= preMast; r *= preMast;
        m_compMaster.process(l, r, m_controls.m_mastAtt, m_controls.m_mastRel, m_controls.m_mastGain, useRMS);
        // LIMITER
        m_lookaheadBufL[m_bufferPos] = l; m_lookaheadBufR[m_bufferPos] = r;
        float maxPeak = 0.0f;
        int scanIdx = m_bufferPos;
        for(int k=0; k < lookaheadDist; k+=4) { 
            if (scanIdx < 0) scanIdx += bufSize;
            float p = std::max(std::abs(m_lookaheadBufL[scanIdx]), std::abs(m_lookaheadBufR[scanIdx]));
            if(p > maxPeak) maxPeak = p;
            scanIdx -= 4;
        }
        float targetGain = 1.0f;
        if(maxPeak > ceilingLin) targetGain = ceilingLin / maxPeak;
        if(targetGain < m_limiterEnv) m_limiterEnv = targetGain; 
        else m_limiterEnv = m_limiterEnv * 0.999f + targetGain * 0.001f; 
        int readPos = m_bufferPos - lookaheadDist;
        while(readPos < 0) readPos += bufSize;
        float outL = m_lookaheadBufL[readPos]; float outR = m_lookaheadBufR[readPos];
        outL *= m_limiterEnv; outR *= m_limiterEnv;
        checkNaN(outL); checkNaN(outR);
        m_bufferPos++; if(m_bufferPos >= bufSize) m_bufferPos = 0;
    
        buf[i][0] = outL; buf[i][1] = outR;
        if(i < 512) { m_scopeOutL[i] = outL; m_scopeOutR[i] = outR; }
    }
    return ProcessStatus::Continue;
}
extern "C" {
    PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data) {
        return new MasteringSuiteEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
    }
}
} // namespace lmms
