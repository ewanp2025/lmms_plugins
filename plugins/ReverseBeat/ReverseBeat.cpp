#include "ReverseBeat.h"

#include "Engine.h"

#include "AudioEngine.h"

#include "Song.h"

#include "embed.h"

#include "plugin_export.h"

#include "Knob.h"

#include <QGridLayout>

#include <cmath>

extern "C"

{

    PLUGIN_EXPORT lmms::Plugin::Descriptor ReverseBeat_plugin_descriptor =

    {

        LMMS_STRINGIFY(PLUGIN_NAME),

        "ReverseBeat",

        QT_TRANSLATE_NOOP("PluginBrowser", "Reverse Buffer"),

        "Ewan Pettigrew",

        0x0100,

        lmms::Plugin::Type::Effect,

        new lmms::PixmapLoader("lmms-plugin-logo"),

        nullptr, nullptr,

    };

}

namespace lmms

{

namespace gui

{

ReverseBeatControlDialog::ReverseBeatControlDialog(ReverseBeatControls* controls) :

    EffectControlDialog(controls)

{

    QGridLayout* layout = new QGridLayout(this);

    auto makeKnob = [&](const QString& label, FloatModel* model) {

        Knob* k = new Knob(KnobType::Bright26, label, this);

        k->setModel(model);

        return k;

    };

    layout->addWidget(makeKnob(tr("TRIG"), &controls->m_triggerModel), 0, 0, Qt::AlignCenter);

    layout->addWidget(makeKnob(tr("LEN"), &controls->m_lengthModel), 0, 1, Qt::AlignCenter);

    layout->addWidget(makeKnob(tr("BIT"), &controls->m_crunchModel), 0, 2, Qt::AlignCenter);

    layout->addWidget(makeKnob(tr("PITCH"), &controls->m_pitchModel), 1, 0, Qt::AlignCenter);

    layout->addWidget(makeKnob(tr("LPF"), &controls->m_filterModel), 1, 1, Qt::AlignCenter);

}

} // namespace gui

ReverseBeatControls::ReverseBeatControls(ReverseBeatEffect* effect) :

    EffectControls(effect),

    m_effect(effect),

    m_triggerModel(0.0f, 0.0f, 1.0f, 1.0f, this, tr("Trigger")),

    m_lengthModel(1.0f, 0.05f, 2.0f, 0.05f, this, tr("Length")),

    m_crunchModel(0.0f, 0.0f, 1.0f, 1.0f, this, tr("Crunch")),

    m_pitchModel(1.0f, 0.1f, 2.0f, 0.01f, this, tr("Pitch")),

    m_filterModel(0.0f, 0.0f, 0.99f, 0.01f, this, tr("Filter"))

{}

gui::EffectControlDialog* ReverseBeatControls::createView() 

{ 

    return new gui::ReverseBeatControlDialog(this); 

}



void ReverseBeatControls::loadSettings(const QDomElement& p) {

    m_triggerModel.loadSettings(p, "trigger");

    m_lengthModel.loadSettings(p, "length");

    m_crunchModel.loadSettings(p, "crunch");

    m_pitchModel.loadSettings(p, "pitch");

    m_filterModel.loadSettings(p, "filter");

}



void ReverseBeatControls::saveSettings(QDomDocument& d, QDomElement& p) {

    m_triggerModel.saveSettings(d, p, "trigger");

    m_lengthModel.saveSettings(d, p, "length");

    m_crunchModel.saveSettings(d, p, "crunch");

    m_pitchModel.saveSettings(d, p, "pitch");

    m_filterModel.saveSettings(d, p, "filter");

}

ReverseBeatEffect::ReverseBeatEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :

    // Reference the global descriptor defined at the top

    Effect(&ReverseBeat_plugin_descriptor, parent, key),

    m_controls(this), m_writePos(0), m_lastTrigger(0.0f), m_isReversing(false), 

    m_playbackTimer(0.0), m_filterLP_L(0.0f), m_filterLP_R(0.0f)

{

    m_bufferL.assign(192000, 0.0f);

    m_bufferR.assign(192000, 0.0f);

}


Effect::ProcessStatus ReverseBeatEffect::processImpl(SampleFrame* buf, const fpp_t frames)

{

    const float bpm = Engine::getSong()->getTempo();

    const float sampleRate = Engine::audioEngine()->baseSampleRate();

    const float trigger = m_controls.m_triggerModel.value();

    const bool useCrunch = m_controls.m_crunchModel.value() >= 0.5f;

    const float pitch = m_controls.m_pitchModel.value();

    const float lpf = m_controls.m_filterModel.value();

    const int loopSize = static_cast<int>((60.0f / bpm) * sampleRate * m_controls.m_lengthModel.value());

    bool isPlaying = Engine::getSong()->isPlaying();

    if (!isPlaying) { m_isReversing = false; }

    if (trigger >= 0.5f && m_lastTrigger < 0.5f && isPlaying) 

    {

        m_isReversing = true;

        m_playbackTimer = 0.0;

        m_reverseStartPos = (m_writePos > 0) ? (m_writePos - 1) : (m_bufferL.size() - 1);

        m_filterLP_L = 0.0f; 

        m_filterLP_R = 0.0f; 

    } 

    else if (trigger < 0.5f) { m_isReversing = false; }

    m_lastTrigger = trigger;



    for (fpp_t f = 0; f < frames; ++f) 

    {

        if (!m_isReversing) 

        {

            m_bufferL[m_writePos] = buf[f][0];

            m_bufferR[m_writePos] = buf[f][1];

            m_writePos = (m_writePos + 1) % m_bufferL.size();

        } 

        else 

        {

            double scaledTimer = m_playbackTimer * pitch;

            double offset = fmod(scaledTimer, (double)loopSize);
           
            double readPosExact = (double)m_reverseStartPos - offset;

            while (readPosExact < 0) readPosExact += (double)m_bufferL.size();

            int index1 = (int)readPosExact;

            int index2 = (index1 == 0) ? (int)(m_bufferL.size() - 1) : (index1 - 1);

            float fraction = (float)(readPosExact - (double)index1);

            float outL = m_bufferL[index1] + fraction * (m_bufferL[index2] - m_bufferL[index1]);

            float outR = m_bufferR[index1] + fraction * (m_bufferR[index2] - m_bufferR[index1]);

            m_filterLP_L = outL + lpf * (m_filterLP_L - outL);

            m_filterLP_R = outR + lpf * (m_filterLP_R - outR);

            outL = m_filterLP_L; outR = m_filterLP_R;

            if (offset < FADE_SAMPLES) {

                float fadeIdx = (float)offset / (float)FADE_SAMPLES;

                outL *= fadeIdx; outR *= fadeIdx;

            } else if (offset > (loopSize - FADE_SAMPLES)) {

                float fadeIdx = (float)(loopSize - offset) / (float)FADE_SAMPLES;

                outL *= fadeIdx; outR *= fadeIdx;

            }

            if (useCrunch) {

                outL = std::round(outL * 127.0f) / 127.0f;

                outR = std::round(outR * 127.0f) / 127.0f;

            }

            buf[f][0] = outL;

            buf[f][1] = outR;

            m_playbackTimer += 1.0;

        }

    }

    return ProcessStatus::Continue;

}

extern "C" {

    // 2. Export the main entry point

    PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data) {

        return new ReverseBeatEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));

    }

}



} // namespace lmms


