#include "ReverseBeat.h"#include "Engine.h"#include "AudioEngine.h"#include "Song.h"#include "embed.h"#include "plugin_export.h"#include "Knob.h"#include <QGridLayout>#include <cmath>namespace lmms{extern "C"{
Plugin::Descriptor PLUGIN_EXPORT ReverseBeat_plugin_descriptor ={	LMMS_STRINGIFY(PLUGIN_NAME),
	"ReverseBeat",
	QT_TRANSLATE_NOOP("PluginBrowser", "4th beat reverse"),
	"Ewan Pettigrew",
	0x0100,
	Plugin::Type::Effect,
	new PixmapLoader("lmms-plugin-logo"),
	nullptr, nullptr,
} ;
}namespace gui{
ReverseBeatControlDialog::ReverseBeatControlDialog(ReverseBeatControls* controls) :
    EffectControlDialog(controls)
{
    QGridLayout* layout = new QGridLayout(this);
    auto makeKnob = [&](const QString& label, FloatModel* model) {
        Knob* k = new Knob(KnobType::Bright26, label, this);
        k->setModel(model);
        return k;
    };    layout->addWidget(makeKnob(tr("TRIG"), &controls->m_triggerModel), 0, 0, Qt::AlignCenter);
    layout->addWidget(makeKnob(tr("LEN"), &controls->m_lengthModel), 0, 1, Qt::AlignCenter);
    layout->addWidget(makeKnob(tr("BIT"), &controls->m_crunchModel), 0, 2, Qt::AlignCenter);}
}ReverseBeatControls::ReverseBeatControls(ReverseBeatEffect* effect) :
	EffectControls(effect),
	m_effect(effect),
	m_triggerModel(0.0f, 0.0f, 1.0f, 1.0f, this, tr("Trigger")),
	m_lengthModel(1.0f, 0.05f, 2.0f, 0.05f, this, tr("Length")),
	m_crunchModel(0.0f, 0.0f, 1.0f, 1.0f, this, tr("Crunch"))
{}
gui::EffectControlDialog* ReverseBeatControls::createView() { return new gui::ReverseBeatControlDialog(this); }
void ReverseBeatControls::loadSettings(const QDomElement& p) {
	m_triggerModel.loadSettings(p, "trigger");
	m_lengthModel.loadSettings(p, "length");
	m_crunchModel.loadSettings(p, "crunch");
}

void ReverseBeatControls::saveSettings(QDomDocument& d, QDomElement& p) {
	m_triggerModel.saveSettings(d, p, "trigger");
	m_lengthModel.saveSettings(d, p, "length");
	m_crunchModel.saveSettings(d, p, "crunch");
}
ReverseBeatEffect::ReverseBeatEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key) :
	Effect(&ReverseBeat_plugin_descriptor, parent, key),
	m_controls(this), m_writePos(0), m_lastTrigger(0.0f), m_isReversing(false), m_playbackTimer(0)
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
	const int loopSize = static_cast<int>((60.0f / bpm) * sampleRate * m_controls.m_lengthModel.value());
	if (trigger >= 0.5f && m_lastTrigger < 0.5f) 
	{		m_isReversing = true;
		m_playbackTimer = 0;
		m_reverseStartPos = (m_writePos > 0) ? (m_writePos - 1) : (m_bufferL.size() - 1);
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
			// LOOPING LOGIC: Use modulo to restart the reverse segment
			int offset = m_playbackTimer % loopSize;
			int readPos = (m_reverseStartPos - offset);
			while (readPos < 0) readPos += m_bufferL.size();
			float outL = m_bufferL[readPos];
			float outR = m_bufferR[readPos];
			if (useCrunch) {
				outL = std::floor(outL * 2048.0f) / 2048.0f;
				outR = std::floor(outR * 2048.0f) / 2048.0f;
			}

			buf[f][0] = outL;
			buf[f][1] = outR;
			m_playbackTimer++;
		}
	}
	return ProcessStatus::Continue;
}
extern "C" {
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model* parent, void* data) {
	return new ReverseBeatEffect(parent, static_cast<const Plugin::Descriptor::SubPluginFeatures::Key*>(data));
}
}
} // namespace lmms