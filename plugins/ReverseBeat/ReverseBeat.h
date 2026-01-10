#ifndef LMMS_REVERSEBEAT_H
#define LMMS_REVERSEBEAT_H

#include "Effect.h"
#include "EffectControls.h"
#include "EffectControlDialog.h"
#include <vector>

namespace lmms
{

class ReverseBeatEffect;
class ReverseBeatControls;

namespace gui
{
    class ReverseBeatControlDialog : public EffectControlDialog
    {
        Q_OBJECT
    public:
        ReverseBeatControlDialog(ReverseBeatControls* controls);
        ~ReverseBeatControlDialog() override = default;
    };
} // namespace gui

class ReverseBeatControls : public EffectControls
{
	Q_OBJECT
public:
	ReverseBeatControls(ReverseBeatEffect* effect);
	~ReverseBeatControls() override = default;

	void saveSettings(QDomDocument& doc, QDomElement& parent) override;
	void loadSettings(const QDomElement& parent) override;

	inline QString nodeName() const override { return "ReverseBeatControls"; }
	gui::EffectControlDialog* createView() override;
	int controlCount() override { return 3; }

	ReverseBeatEffect* m_effect;
	FloatModel m_triggerModel;
	FloatModel m_lengthModel;
	FloatModel m_crunchModel;
private:
	friend class ReverseBeatEffect;
    friend class gui::ReverseBeatControlDialog;
};

class ReverseBeatEffect : public Effect
{
public:
	ReverseBeatEffect(Model* parent, const Descriptor::SubPluginFeatures::Key* key);
	~ReverseBeatEffect() override = default;

	ProcessStatus processImpl(SampleFrame* buf, const fpp_t frames) override;
	EffectControls* controls() override { return &m_controls; }

private:
	ReverseBeatControls m_controls;
	
	std::vector<float> m_bufferL, m_bufferR;
	size_t m_writePos;
	float m_lastTrigger;
	bool m_isReversing;
	int m_reverseStartPos; // Changed to track the 'anchor' point
	int m_playbackTimer;   // Counts samples since trigger started

	friend class ReverseBeatControls;
};

} // namespace lmms


#endif
