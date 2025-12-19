#pragma once
#include "EffectControls.h"
#include "MasteringSuiteDialog.h"
namespace lmms {
class MasteringSuiteEffect;
class MasteringSuiteControls : public EffectControls {
    Q_OBJECT
public:
    MasteringSuiteControls(MasteringSuiteEffect* effect);
    // FILTERS
    FloatModel m_lowCut, m_highCut; 
    FloatModel m_lowSplit, m_highSplit;
 
    // GLOBAL
    BoolModel m_rmsMode;
    // LOW
    FloatModel m_lowPre, m_lowAtt, m_lowRel, m_lowGain, m_lowSat, m_lowWidth;
    BoolModel  m_lowSolo;
    // MID
    FloatModel m_midPre, m_midAtt, m_midRel, m_midGain, m_midSat, m_midWidth;
    BoolModel  m_midSolo;
    // HIGH
    FloatModel m_highPre, m_highAtt, m_highRel, m_highGain, m_highSat, m_highWidth;
    BoolModel  m_highSolo;
    // MASTER
    FloatModel m_mastPre, m_mastAtt, m_mastRel, m_mastGain, m_mastCeil;
    int controlCount() override { return 31; } 
 
    // SAVING AND LOADING
    void saveSettings(QDomDocument& doc, QDomElement& parent) override;
    void loadSettings(const QDomElement& parent) override;
    QString nodeName() const override { return "MasteringSuite"; }
    gui::EffectControlDialog* createView() override;
};
} // namespace lmms
