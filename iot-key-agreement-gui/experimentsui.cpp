#include "experimentsui.h"
#include "ui_experimentsui.h"

#include <QMessageBox>
#include <QThread>

ExperimentsUI::ExperimentsUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExperimentsUI)
{
    ui->setupUi(this);
}

ExperimentsUI::~ExperimentsUI()
{
    delete ui;
}

void ExperimentsUI::on_pushButton_runExperiment_clicked()
{
    if(ui->lineEdit_host->text().isEmpty() || ui->lineEdit_port->text().isEmpty()){
        QMessageBox msgbox;
        msgbox.setIcon(QMessageBox::Warning);
        msgbox.setWindowTitle("IOT Key Agreement - Experiments");
        msgbox.addButton(QMessageBox::Ok);
        msgbox.setText("Warning: you need to provide a host and a port to connect.");
        msgbox.exec();
        return;
    }
    if(ui->lineEdit_ndevices->text().isEmpty()){
        QMessageBox msgbox;
        msgbox.setIcon(QMessageBox::Warning);
        msgbox.setWindowTitle("IOT Key Agreement - Experiments");
        msgbox.addButton(QMessageBox::Ok);
        msgbox.setText("Warning: you need to provide the number of devices used in the experiment.");
        msgbox.exec();
        return;
    }

    n_devices = ui->lineEdit_ndevices->text().toInt();
    host = ui->lineEdit_host->text();
    port = ui->lineEdit_port->text().toInt();
    username = ui->lineEdit_username->text();
    password = ui->lineEdit_password->text();
    /*trevor->init(host, port, username, password);
    if(trevor->getMqtt()->state() == QMqttClient::Disconnected){
        trevor->setM(7);
        trevor->setN(7);
        trevor->connectToHost();
    }*/
   devices.resize(n_devices);
   for(size_t i = 0; i < this->n_devices; i++){
        this->devices[i] = new Device(this->host, this->port, this->username, this->password);
        this->devices[i]->setN_cobaias(this->n_devices);
        QObject::connect(devices[i], &Device::emitTotalTime, this, &ExperimentsUI::receiveComputationTime);
   }
}

void ExperimentsUI::receiveComputationTime(int time)
{
    total_comp_time += time;
    n_finished += 1;
    if(n_finished == n_devices){
        ui->label->setText(QString::number(total_comp_time)+QString(" ms"));
        qDebug() << "Total time to session key computation: " << total_comp_time << "ms\n";
    }
}
