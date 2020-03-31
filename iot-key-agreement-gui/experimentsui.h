#ifndef EXPERIMENTSUI_H
#define EXPERIMENTSUI_H

#include <QWidget>
#include <QObject>

#include "trevor.h"
#include "device.h"

namespace Ui {
class ExperimentsUI;
}

class ExperimentsUI : public QWidget
{
    Q_OBJECT

public:
    explicit ExperimentsUI(QWidget *parent = nullptr);
    ~ExperimentsUI();

private slots:
    void on_pushButton_runExperiment_clicked();

    void receiveComputationTime(int time);

private:
    Ui::ExperimentsUI *ui;
    size_t n_devices, n_finished = 0;
    QVector<Device *> devices;
    QVector<QThread *> threads;
    QString host, username, password;
    quint16 port;
    int total_comp_time = 0;
};

#endif // EXPERIMENTSUI_H
