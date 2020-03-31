#ifndef DEVICE_H
#define DEVICE_H

#include "iot-key-agreement-core_global.h"
#include "mqttserver.h"
#include <QObject>
#include <QElapsedTimer>
#include <boost/multiprecision/gmp.hpp>

class IOTKEYAGREEMENTCORE_EXPORT Device: public QObject
{
    Q_OBJECT
public:
    Device(const QString &host, const int port, const QString &user, const QString &password);
    ~Device();
    QString getId_mqtt() const;

    void setN_cobaias(const size_t &value);

private:
    void compute_gamma();

    void compute_session_key();

    std::string generate_random_id(std::string &str, size_t len);

signals:
    void emitTotalTime(int totalTime);

private slots:
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);
    void subscribeToTopics();

private:
    MQTTServer *m_mqtt;
    QString id_mqtt;
    QVector<QString> users;
    std::vector<boost::multiprecision::mpz_int> gammas;
    boost::multiprecision::mpz_int session_key, alpha, beta, xa, xb;
    boost::multiprecision::mpz_int y, gamma, delta, totient_delta;
    size_t n_users = 0, server_id = 0, n_cobaias = 0;
    bool gamma_computed = false, session_key_computed = false, on_experimentation = false;
    QElapsedTimer timer;
    int total_time = 0;
};

#endif // DEVICE_H
