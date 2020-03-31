#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <QFile>
#include<QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    trevor(nullptr),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    params_ui = new ParamsGUI();
    users_model = new QStringListModel(this);
    users_model->setStringList(users_list);
    ui->listView_users->setModel(users_model);

    sesskey_model = new QStringListModel(this);
    sesskey_model->setStringList(sesskey_list);
    ui->listView->setModel(sesskey_model);

    log_model = new QStringListModel(this);
    log_model->setStringList(log_list);
    ui->listView_log->setModel(log_model);

    QFile file("mqtt.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream in(&file);
        size_t i = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            switch (i) {
            case 0:
                this->host = line;
                ui->lineEdit_host->setText(line);
                break;
            case 1:
                this->port = line.toInt();
                ui->lineEdit_port->setText(line);
                break;
            case 2:
                this->username = line;
                ui->lineEdit_username->setText(line);
                break;
            case 3:
                this->password = line;
                ui->lineEdit_password->setText(line);
                break;
            }
            i++;
        }
        file.close();
    }
    trevor = new Trevor(host, port.toInt(), username, password);
    trevor->setM(3);
    trevor->setN(3);
    ui->lineEdit_m->setText(QString::number(3));
    ui->lineEdit_n->setText(QString::number(3));
    QObject::connect(trevor, &Trevor::userConnected, this, &MainWindow::addUserToListView);
    QObject::connect(trevor, &Trevor::sessionKeyComputed, this, &MainWindow::addSessionKeyToView);
    QObject::connect(trevor, &Trevor::emitLogMessage, this, &MainWindow::addLogMessageToView);
    QObject::connect(trevor, &Trevor::sessionParamsComputed, params_ui, &ParamsGUI::receiveParameters);
    QObject::connect(trevor, &Trevor::userDisconnected, params_ui, [this](const QString user){
        size_t row = 0;
        for(auto _user: users_list){
            if(_user == user) break;
            row++;
        }
        this->ui->listView_users->model()->removeRow(row);
        users_list.erase(std::remove_if(this->users_list.begin(), this->users_list.end(), [&user](QString _user){
            return (user == _user);
        }), users_list.end());
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_connect_clicked()
{
    if(trevor->getMqtt()->state() == QMqttClient::Disconnected){
        this->host = ui->lineEdit_host->text();
        this->port = ui->lineEdit_port->text();
        if(ui->lineEdit_host->text().isEmpty() || ui->lineEdit_port->text().isEmpty()){
            QMessageBox msgbox;
            msgbox.setIcon(QMessageBox::Warning);
            msgbox.setWindowTitle("IOT Key Agreement");
            msgbox.addButton(QMessageBox::Ok);
            msgbox.setText("Warning: you need to provide a host and a port to connect.");
            msgbox.exec();
            return;
        }
        this->username = ui->lineEdit_username->text();
        this->password = ui->lineEdit_password->text();

        QFile file("mqtt.txt");
        if (!file.open(QIODevice::WriteOnly)) {
            return;
        }

        QTextStream out(&file);
        out << host.toUtf8() << "\n";
        out << port.toUtf8() << "\n";
        out << username.toUtf8() << "\n";
        out << password.toUtf8() << "\n";

        file.close();

        if(!trevor){
            trevor = new Trevor(host, port.toInt(), username, password);
            trevor->setM(7);
            trevor->setN(7);
        }else{
            trevor->setHost(host);
            trevor->setPort(port.toInt());
            trevor->setUsername(username);
            trevor->setPassword(password);
        }
        trevor->connectToHost();
        ui->pushButton_connect->setText("Disconnect");
    }else{
        trevor->dropUsers();
        trevor->setNewSession();
        trevor->disconnectFromHost();
        std::for_each(devices.begin(), devices.end(), [](Device *d){
            delete d;
        });
        devices.clear();
        ui->pushButton_connect->setText("Connect");
    }
}

void MainWindow::addUserToListView(const QString user)
{
    users_list << user;
    users_model->setStringList(users_list);
    ui->listView_users->setModel(users_model);
}

void MainWindow::addSessionKeyToView(const QString user, const QString session_key)
{
    QString content = user + "_" + session_key;
    sesskey_list << content;
    sesskey_model->setStringList(sesskey_list);
    ui->listView->setModel(sesskey_model);
}

void MainWindow::addLogMessageToView(const QString &msg)
{
    log_list << msg;
    log_model->setStringList(log_list);
    ui->listView_log->setModel(log_model);
}

void MainWindow::on_pushButton_parameters_clicked()
{
    params_ui->show();
}

void MainWindow::on_pushButton_clear_clicked()
{
    ui->listView->model()->removeRows(0, ui->listView->model()->rowCount());
    sesskey_list.erase(sesskey_list.begin(), sesskey_list.end());
}

void MainWindow::on_pushButton_clicked()
{
    int m = ui->lineEdit_m->text().toInt();
    int n = ui->lineEdit_n->text().toInt();

    trevor->setM(m);
    trevor->setN(n);
}

void MainWindow::on_pushButton_new_device_clicked()
{
    devices.push_back(new Device(this->host, this->port.toInt(), this->username, this->password));
}

void MainWindow::on_pushButton_remove_device_clicked()
{
    QModelIndex index = ui->listView_users->currentIndex();
    QString device = index.data(Qt::DisplayRole).toString();
    auto it = devices.begin();
    size_t i;
    for(i = 0; it != devices.end(); it++, i++){
        if((*it)->getId_mqtt() == device) break;
    }
    delete devices[i];
    devices.erase(it);
}


