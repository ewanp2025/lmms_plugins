#include "MasteringSuiteControls.h"
#include "MasteringSuiteDialog.h"
#include "MasteringSuite.h" 
#include <QDomDocument> 
#include <QDomElement> 
#include <QDomNode>
namespace lmms {
MasteringSuiteControls::MasteringSuiteControls(MasteringSuiteEffect* effect) 
    : EffectControls(effect)
    , m_lowCut(20.f, 10.f, 150.f, 1.f, this, tr("LoCut"))
    , m_highCut(20000.f, 10000.f, 20000.f, 10.f, this, tr("HiCut"))
    , m_lowSplit(200.f, 60.f, 800.f, 1.f, this, tr("Low Split"))
    , m_highSplit(3000.f, 800.f, 10000.f, 10.f, this, tr("High Split"))
    , m_rmsMode(false, this, tr("RMS Mode"))
    // LOW
    , m_lowPre(0.f, -12.f, 12.f, 0.1f, this, tr("L-Pre"))
    , m_lowAtt(30.f, 1.f, 200.f, 1.f, this, tr("L-Att"))
    , m_lowRel(100.f, 10.f, 1000.f, 1.f, this, tr("L-Rel"))
    , m_lowGain(0.f, -12.f, 12.f, 0.1f, this, tr("L-Post"))
    , m_lowSat(0.f, 0.f, 1.f, 0.01f, this, tr("L-Sat"))
    , m_lowWidth(1.0f, 0.f, 2.f, 0.01f, this, tr("L-Wid"))
    , m_lowSolo(false, this, tr("L-Solo"))
    // MID
    , m_midPre(0.f, -12.f, 12.f, 0.1f, this, tr("M-Pre"))
    , m_midAtt(30.f, 1.f, 200.f, 1.f, this, tr("M-Att"))
    , m_midRel(100.f, 10.f, 1000.f, 1.f, this, tr("M-Rel"))
    , m_midGain(0.f, -12.f, 12.f, 0.1f, this, tr("M-Post"))
    , m_midSat(0.f, 0.f, 1.f, 0.01f, this, tr("M-Sat"))
    , m_midWidth(1.0f, 0.f, 2.f, 0.01f, this, tr("M-Wid"))
    , m_midSolo(false, this, tr("M-Solo"))
    // HIGH
    , m_highPre(0.f, -12.f, 12.f, 0.1f, this, tr("H-Pre"))
    , m_highAtt(30.f, 1.f, 200.f, 1.f, this, tr("H-Att"))
    , m_highRel(100.f, 10.f, 1000.f, 1.f, this, tr("H-Rel"))
    , m_highGain(0.f, -12.f, 12.f, 0.1f, this, tr("H-Post"))
    , m_highSat(0.f, 0.f, 1.f, 0.01f, this, tr("H-Sat"))
    , m_highWidth(1.0f, 0.f, 2.f, 0.01f, this, tr("H-Wid"))
    , m_highSolo(false, this, tr("H-Solo"))
    // MASTER
    , m_mastPre(0.f, -12.f, 12.f, 0.1f, this, tr("Mas-Pre"))
    , m_mastAtt(30.f, 1.f, 200.f, 1.f, this, tr("Mas-Att"))
    , m_mastRel(100.f, 10.f, 1000.f, 1.f, this, tr("Mas-Rel"))
    , m_mastGain(0.f, -12.f, 12.f, 0.1f, this, tr("Mas-Post"))
    , m_mastCeil(0.f, -6.f, 0.f, 0.01f, this, tr("Ceiling"))
{}
void MasteringSuiteControls::saveSettings(QDomDocument& doc, QDomElement& parent) {
    m_lowCut.saveSettings(doc, parent, "lowCut");
    m_highCut.saveSettings(doc, parent, "highCut");
    m_lowSplit.saveSettings(doc, parent, "lowSplit");
    m_highSplit.saveSettings(doc, parent, "highSplit");
    m_rmsMode.saveSettings(doc, parent, "rmsMode");
    auto saveBand = [&](const char* prefix, FloatModel& p, FloatModel& a, FloatModel& r, FloatModel& g, FloatModel& s, FloatModel& w, BoolModel& solo) {
        p.saveSettings(doc, parent, QString(prefix) + "Pre");
        a.saveSettings(doc, parent, QString(prefix) + "Att");
        r.saveSettings(doc, parent, QString(prefix) + "Rel");
        g.saveSettings(doc, parent, QString(prefix) + "Gain");
        s.saveSettings(doc, parent, QString(prefix) + "Sat");
        w.saveSettings(doc, parent, QString(prefix) + "Wid");
        solo.saveSettings(doc, parent, QString(prefix) + "Solo");
    };
    saveBand("low", m_lowPre, m_lowAtt, m_lowRel, m_lowGain, m_lowSat, m_lowWidth, m_lowSolo);
    saveBand("mid", m_midPre, m_midAtt, m_midRel, m_midGain, m_midSat, m_midWidth, m_midSolo);
    saveBand("high", m_highPre, m_highAtt, m_highRel, m_highGain, m_highSat, m_highWidth, m_highSolo);
    m_mastPre.saveSettings(doc, parent, "mastPre");
    m_mastAtt.saveSettings(doc, parent, "mastAtt");
    m_mastRel.saveSettings(doc, parent, "mastRel");
    m_mastGain.saveSettings(doc, parent, "mastGain");
    m_mastCeil.saveSettings(doc, parent, "mastCeil");
    MasteringSuiteEffect* effect = static_cast<MasteringSuiteEffect*>(parentModel());
 
    auto saveCurve = [&](const char* name, const std::vector<CurvePoint>& pts) {
        QDomElement root = doc.createElement(name);
        for(const auto& p : pts) {
            QDomElement pt = doc.createElement("pt");
            pt.setAttribute("x", p.x);
            pt.setAttribute("y", p.y);
            root.appendChild(pt);
        }
        parent.appendChild(root);
    };
    saveCurve("curveLow", effect->m_compLow.m_points);
    saveCurve("curveMid", effect->m_compMid.m_points);
    saveCurve("curveHigh", effect->m_compHigh.m_points);
    saveCurve("curveMast", effect->m_compMaster.m_points);
}
void MasteringSuiteControls::loadSettings(const QDomElement& parent) {
    m_lowCut.loadSettings(parent, "lowCut");
    m_highCut.loadSettings(parent, "highCut");
    m_lowSplit.loadSettings(parent, "lowSplit");
    m_highSplit.loadSettings(parent, "highSplit");
    m_rmsMode.loadSettings(parent, "rmsMode");
    auto loadBand = [&](const char* prefix, FloatModel& p, FloatModel& a, FloatModel& r, FloatModel& g, FloatModel& s, FloatModel& w, BoolModel& solo) {
        p.loadSettings(parent, QString(prefix) + "Pre");
        a.loadSettings(parent, QString(prefix) + "Att");
        r.loadSettings(parent, QString(prefix) + "Rel");
        g.loadSettings(parent, QString(prefix) + "Gain");
        s.loadSettings(parent, QString(prefix) + "Sat");
        w.loadSettings(parent, QString(prefix) + "Wid");
        solo.loadSettings(parent, QString(prefix) + "Solo");
    };
    loadBand("low", m_lowPre, m_lowAtt, m_lowRel, m_lowGain, m_lowSat, m_lowWidth, m_lowSolo);
    loadBand("mid", m_midPre, m_midAtt, m_midRel, m_midGain, m_midSat, m_midWidth, m_midSolo);
    loadBand("high", m_highPre, m_highAtt, m_highRel, m_highGain, m_highSat, m_highWidth, m_highSolo);
    m_mastPre.loadSettings(parent, "mastPre");
    m_mastAtt.loadSettings(parent, "mastAtt");
    m_mastRel.loadSettings(parent, "mastRel");
    m_mastGain.loadSettings(parent, "mastGain");
    m_mastCeil.loadSettings(parent, "mastCeil");
    MasteringSuiteEffect* effect = static_cast<MasteringSuiteEffect*>(parentModel());
  
    auto loadCurve = [&](const char* name, CompModule& mod) {
        QDomElement root = parent.firstChildElement(name);
        if(!root.isNull()) {
            mod.m_points.clear();
            QDomNode n = root.firstChild();
            while(!n.isNull()) {
                QDomElement e = n.toElement();
                if(!e.isNull()) {
                    CurvePoint pt; 
                    pt.x = e.attribute("x").toFloat();
                    pt.y = e.attribute("y").toFloat();
                    mod.m_points.push_back(pt);
                }
                n = n.nextSibling();
            }
            mod.updateLUT();
        }
    };
    loadCurve("curveLow", effect->m_compLow);
    loadCurve("curveMid", effect->m_compMid);
    loadCurve("curveHigh", effect->m_compHigh);
    loadCurve("curveMast", effect->m_compMaster);
}
gui::EffectControlDialog* MasteringSuiteControls::createView() {
    return new gui::MasteringSuiteDialog(this);
}
} // namespace lmms
