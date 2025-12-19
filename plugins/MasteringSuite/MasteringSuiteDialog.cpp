#include "MasteringSuiteDialog.h"
#include "MasteringSuite.h" 
#include "Knob.h" 
#include "MasteringScope.h" 
#include <QGroupBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QLabel>
#include <vector>
namespace lmms::gui {
MasteringSuiteDialog::MasteringSuiteDialog(MasteringSuiteControls* c) : EffectControlDialog(c) {
    setFixedSize(720, 500);
    setWindowTitle("Mastering Suite");
  
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(40, 42, 48));
    setPalette(pal);
    setAutoFillBackground(true);
    auto scope = new MasteringScope(this);
    scope->setGeometry(10, 10, 700, 220);
  
    MasteringSuiteEffect* effect = static_cast<MasteringSuiteEffect*>(c->parent());
  
    scope->m_ptrInL = &effect->m_scopeInL;
    scope->m_ptrInR = &effect->m_scopeInR;
    scope->m_ptrOutL = &effect->m_scopeOutL;
    scope->m_ptrOutR = &effect->m_scopeOutR;
    CompModule* modules[] = { &effect->m_compLow, &effect->m_compMid, &effect->m_compHigh, &effect->m_compMaster };
    scope->m_activeModule = modules[0]; 
    int btnY = 240;
    const char* labels[] = { "LOW", "MID", "HIGH", "MASTER" };
  
    QStackedWidget* stack = new QStackedWidget(this);
    stack->setGeometry(100, 240, 600, 240);
    // --- BUTTONS & METERS ---
    std::vector<QPushButton*> buttons;
    std::vector<GRMeter*> meters; 
    for(int i=0; i<4; ++i) {
        QPushButton* b = new QPushButton(labels[i], this);
        b->setGeometry(10, btnY + i*45, 80, 40);
        b->setStyleSheet("background-color: #333; color: #AAA; border: 1px solid #555; border-radius: 4px;");
        buttons.push_back(b);
        GRMeter* m = new GRMeter(this);
        m->move(92, btnY + i*45);
        m->m_value = &modules[i]->m_currentGR; 
        meters.push_back(m);
    }
    for(int i=0; i<4; ++i) {
        connect(buttons[i], &QPushButton::clicked, [=](){ 
            stack->setCurrentIndex(i);
            for(size_t j=0; j<buttons.size(); ++j) {
                if(j == (size_t)i) {
                    buttons[j]->setStyleSheet("background-color: #80FF00; color: black; font-weight: bold; border: 1px solid #80FF00; border-radius: 4px;");
                } else {
                    buttons[j]->setStyleSheet("background-color: #333; color: #AAA; font-weight: normal; border: 1px solid #555; border-radius: 4px;");
                }
            }
            scope->m_activeModule = modules[i]; 
            scope->update();
        });
    }
  
    startTimer(30); 
    // --- CONTROLS PAGE ---
    auto createPage = [&](FloatModel* pre, FloatModel* att, FloatModel* rel, 
                          FloatModel* gain, FloatModel* sat, FloatModel* last, 
                          BoolModel* solo, const char* lastLabel = "Width") {
        QGroupBox* gb = new QGroupBox();
        gb->setStyleSheet("border: none;");
        int kx=10, ky=20;
        auto mk = [&](FloatModel* m, const char* n) {
            Knob* k = new Knob(KnobType::Bright26, gb);
            k->setModel(m); k->setLabel(n); k->move(kx, ky); k->show();
            kx += 60;
        };
        mk(pre, "Pre"); kx += 10;
        mk(att, "Attack"); mk(rel, "Release"); kx += 20; 
        mk(gain, "Post");
        if(sat) mk(sat, "Sat"); 
        if(last) mk(last, lastLabel); 
        if(solo) {
            QCheckBox* cb = new QCheckBox("SOLO", gb);
            cb->move(kx + 10, ky + 10);
            cb->setStyleSheet("color: #DDD; font-weight: bold;");
            cb->setChecked(solo->value());
            connect(cb, &QCheckBox::toggled, [=](bool v){ solo->setValue(v); });
            connect(solo, &BoolModel::dataChanged, [=](){ cb->setChecked(solo->value()); });
        }
        return gb;
    };
    stack->addWidget(createPage(&c->m_lowPre, &c->m_lowAtt, &c->m_lowRel, &c->m_lowGain, &c->m_lowSat, &c->m_lowWidth, &c->m_lowSolo));
    stack->addWidget(createPage(&c->m_midPre, &c->m_midAtt, &c->m_midRel, &c->m_midGain, &c->m_midSat, &c->m_midWidth, &c->m_midSolo));
    stack->addWidget(createPage(&c->m_highPre, &c->m_highAtt, &c->m_highRel, &c->m_highGain, &c->m_highSat, &c->m_highWidth, &c->m_highSolo));
    stack->addWidget(createPage(&c->m_mastPre, &c->m_mastAtt, &c->m_mastRel, &c->m_mastGain, nullptr, &c->m_mastCeil, nullptr, "Ceil"));
  
    // --- GLOBAL KNOBS ---
    auto createLabeledKnob = [&](FloatModel* m, const char* name, int x) {
        Knob* k = new Knob(KnobType::Bright26, this);
        k->setModel(m); k->setLabel(name); k->move(x, 430);
        QLabel* lbl = new QLabel(this);
        lbl->setGeometry(x, 480, 50, 15);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color: #80FF00; font-size: 10px;");
        auto updateLbl = [=]() {
            float v = m->value();
            if(v >= 1000) lbl->setText(QString::asprintf("%.1fk", v/1000.f));
            else lbl->setText(QString::asprintf("%.0fHz", v));
        };
        updateLbl();
        connect(m, &FloatModel::dataChanged, updateLbl);
    };
    createLabeledKnob(&c->m_lowSplit, "Split 1", 10);
    createLabeledKnob(&c->m_highSplit, "Split 2", 60);
    createLabeledKnob(&c->m_lowCut, "LoCut", 120);
    createLabeledKnob(&c->m_highCut, "HiCut", 170);
    QCheckBox* cbRMS = new QCheckBox("RMS Mode", this);
    cbRMS->move(230, 440);
    cbRMS->setStyleSheet("color: #AAA; font-weight: bold;");
    cbRMS->setChecked(c->m_rmsMode.value());
    connect(cbRMS, &QCheckBox::toggled, [=](bool v){ c->m_rmsMode.setValue(v); });
    // --- FACTORY PRESETS SECTION ---
    QLabel* lblFactory = new QLabel("FACTORY SETTINGS", this);
    lblFactory->setGeometry(400, 430, 200, 20);
    lblFactory->setStyleSheet("color: #80FF00; font-weight: bold; font-size: 12px;");
    const char* presets[] = { "A", "B", "C", "D" };
    for(int k=0; k<4; ++k) {
        QPushButton* b = new QPushButton(presets[k], this);
        b->setGeometry(400 + k*50, 455, 40, 30);
        b->setStyleSheet("background-color: #444; color: white; font-weight: bold; font-size: 14px; border: 1px solid #666; border-radius: 4px;");
      
        connect(b, &QPushButton::clicked, [=](){
            // RESET ALL KNOBS
            c->m_lowGain.setValue(0); c->m_midGain.setValue(0); c->m_highGain.setValue(0);
            c->m_lowSat.setValue(0); c->m_midSat.setValue(0); c->m_highSat.setValue(0);
            c->m_mastGain.setValue(0);
            // RESET ALL CURVES
            auto resetCurve = [](CompModule& m) { m.m_points = {{0,0}, {1,1}}; m.updateLUT(); };
            resetCurve(effect->m_compLow); resetCurve(effect->m_compMid); 
            resetCurve(effect->m_compHigh); resetCurve(effect->m_compMaster);
            // FIXED: Added 'f' suffix to all floating point constants to prevent C4305 truncation error
            if(k == 0) { // A: ALL ROUND
                 effect->m_compLow.m_points = {{0,0}, {0.5f, 0.55f}, {1,1}};
                 effect->m_compMid.m_points = {{0,0}, {0.5f, 0.45f}, {1,1}};
                 effect->m_compHigh.m_points = {{0,0}, {0.5f, 0.55f}, {1,1}};
                 effect->m_compMaster.m_points = {{0,0}, {0.8f, 0.85f}, {1,1}};
                 c->m_lowSat.setValue(0.1f); 
            }
            else if(k == 1) { // B: BASS & PUNCH
                 effect->m_compLow.m_points = {{0,0}, {0.7f, 0.8f}, {1,1}};
                 c->m_lowGain.setValue(2.0f); 
                 effect->m_compMaster.m_points = {{0,0}, {0.9f, 0.9f}, {1,1}};
            }
            else if(k == 2) { // C: CRISP
                 effect->m_compLow.m_points = {{0,0}, {0.5f, 0.45f}, {1,1}};
                 effect->m_compHigh.m_points = {{0,0}, {0.4f, 0.5f}, {1,1}};
                 c->m_highGain.setValue(1.5f);
                 c->m_highSat.setValue(0.25f);
            }
            else if(k == 3) { // D: DEPTH
                 effect->m_compMid.m_points = {{0,0}, {0.4f, 0.6f}, {1,1}};
                 effect->m_compMaster.m_points = {{0,0}, {0.6f, 0.75f}, {1,1}};
                 c->m_mastGain.setValue(2.0f);
            }
            // FORCE UPDATE
            effect->m_compLow.updateLUT(); effect->m_compMid.updateLUT();
            effect->m_compHigh.updateLUT(); effect->m_compMaster.updateLUT();
            scope->update();
        });
    }
    buttons[0]->click();
}
void MasteringSuiteDialog::timerEvent(QTimerEvent* e) {
    EffectControlDialog::timerEvent(e); 
    QList<GRMeter*> meters = findChildren<GRMeter*>();
    for(auto m : meters) m->update();
}
} // namespace lmms::gui
