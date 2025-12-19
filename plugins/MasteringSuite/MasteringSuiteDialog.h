#pragma once
#include "EffectControlDialog.h"
#include <QWidget>
#include <QPainter>
namespace lmms {
class MasteringSuiteControls;
namespace gui {
// --- GAIN REDUCTION METER WIDGET ---
class GRMeter : public QWidget {
    Q_OBJECT
public:
    float* m_value = nullptr; // Pointer to CompModule::m_currentGR
    GRMeter(QWidget* parent) : QWidget(parent) {
        setFixedSize(6, 40);
        setAttribute(Qt::WA_OpaquePaintEvent);
    }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(20, 20, 20));
        if(m_value) {
            float val = *m_value; // dB change (negative)
            if(val > 0) val = 0;
            float h = (-val / 30.0f) * height();
            if(h > height()) h = height();
         
            // Turn bright red if heavily compressing
            QColor c = (h < 5) ? QColor(150, 0, 0) : QColor(255, 0, 0);
            p.fillRect(0, 0, width(), (int)h, c);
        }
    }
};
class MasteringSuiteDialog : public EffectControlDialog {
    Q_OBJECT
public:
    MasteringSuiteDialog(MasteringSuiteControls* controls);
protected:
    void timerEvent(QTimerEvent* e) override; 
};
} // namespace gui
} // namespace lmms
