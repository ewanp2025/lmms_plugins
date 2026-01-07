#ifndef RNG6_H
#define RNG6_H
#include <vector>
#include <memory>
#include <atomic>
#include <QWidget>
#include <QPushButton>
#include <QDomElement>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include "Instrument.h"
#include "InstrumentView.h"
#include "AutomatableModel.h"
#include "Knob.h"
#include "LedCheckBox.h"
namespace lmms
{
class SimpleADSR {
public:
    enum Stage { ATTACK, DECAY, SUSTAIN, RELEASE, OFF };
    SimpleADSR() : m_stage(OFF), m_level(0.0f) {}
    void trigger() { m_stage = ATTACK; }
    void release() { if (m_stage != OFF) m_stage = RELEASE; }
    float process(float a, float d, float s, float r, float samplerate) {
        switch (m_stage) {
            case ATTACK: m_level += 1.0f / (std::max(0.001f, a) * samplerate);
                if (m_level >= 1.0f) { m_level = 1.0f; m_stage = DECAY; } break;
            case DECAY: m_level -= (1.0f - s) / (std::max(0.001f, d) * samplerate);
                if (m_level <= s) { m_level = s; m_stage = SUSTAIN; } break;
            case SUSTAIN: m_level = s; break;
            case RELEASE: m_level -= s / (std::max(0.001f, r) * samplerate);
                if (m_level <= 0.0f) { m_level = 0.0f; m_stage = OFF; } break;
            case OFF: m_level = 0.0f; break;
        }
        return m_level;
    }
private:
    Stage m_stage;
    float m_level;
};
struct ChaosPatch {
    struct VoiceParams {
        float detune; float cutoffOffset; float resOffset;
        float pan; float shape; float fmAmount;
    };
    VoiceParams voices[6];
    float modulationCurve[1024]; 
};
namespace gui { class RNG6View; }
class ChaosVoice {

public:
    ChaosVoice(std::shared_ptr<const ChaosPatch> patch, NotePlayHandle* _nph,

             const sample_rate_t _sample_rate, float _chaosAmt,

             float _masterCutoff, float _masterRes, int _activeVoices,

             float _masterSpread, float _envA, float _envD, float _envS,

             float _envR, float _lfoSpeed);

    void nextSample(float* outL, float* outR);

    void release() { m_adsr.release(); }

private:

    float getModulation(float phase);

    float getOscillator(float phase, float shape, float fm);

    std::shared_ptr<const ChaosPatch> m_patch;

    NotePlayHandle* m_nph;

    sample_rate_t m_sampleRate;

    float m_chaosAmount, m_cutoff, m_res;

    int m_numVoices; float m_spread, m_lfoSpeed;

    struct VoiceState { float phase; float currentPanL; float currentPanR; };

    std::vector<VoiceState> m_voiceStates;

    float m_lfoPhase;

    float m_filterStateL[2], m_filterStateR[2];

    SimpleADSR m_adsr; 

    float m_envA, m_envD, m_envS, m_envR;

};

class RNG6Instrument : public Instrument {

    Q_OBJECT

public:

    RNG6Instrument(InstrumentTrack* _instrument_track);

    void playNote(NotePlayHandle* _n, SampleFrame* _working_buffer) override;

    void deleteNotePluginData(NotePlayHandle* _n) override;

    void saveSettings(QDomDocument& _doc, QDomElement& _parent) override;

    void loadSettings(const QDomElement& _this) override;

    QString nodeName() const override;

    gui::PluginView* instantiateView(QWidget* _parent) override;

    std::shared_ptr<const ChaosPatch> currentPatch() const { return m_currentPatch; }

protected slots:

    void updateSeed(); 

private:

    FloatModel m_seed, m_chaosAmount;

    BoolModel m_lockPitch, m_lockFilter, m_lockShape;

    FloatModel m_uniVoices, m_uniSpread, m_filterCutoff, m_filterRes, m_gain;

    FloatModel m_envA, m_envD, m_envS, m_envR, m_lfoSpeed;

    std::shared_ptr<ChaosPatch> m_currentPatch;

    friend class gui::RNG6View; // Now correctly identifies the class

};

namespace gui {

class ChaosVisualizer : public QWidget {

    Q_OBJECT

public:

    ChaosVisualizer(QWidget* parent) : QWidget(parent) {}

    void setPatch(std::shared_ptr<const ChaosPatch> patch) { m_patch = patch; update(); }

protected:

    void paintEvent(QPaintEvent* event) override;

private:

    std::shared_ptr<const ChaosPatch> m_patch;

};

class RNG6View : public InstrumentViewFixedSize {

    Q_OBJECT

public:

    RNG6View(Instrument* _instrument, QWidget* _parent);

private slots:

    void showPageMain(); void showPageSettings(); void refreshVisualizer();

private:

    void modelChanged() override; void updateVisibleWidgets();

    QPushButton *m_btnTabMain, *m_btnTabSettings;

    int m_currentTab;

    Knob *m_seedKnob, *m_chaosKnob, *m_cutoffKnob, *m_resKnob, *m_voicesKnob, 

         *m_spreadKnob, *m_envAKnob, *m_envDKnob, *m_envSKnob, *m_envRKnob, *m_gainKnob;

    LedCheckBox *m_btnLockPitch, *m_btnLockFilter, *m_btnLockShape;

    ChaosVisualizer* m_visualizer;

    QTimer* m_refreshTimer;

};

} // namespace gui

} // namespace lmms

#endif
