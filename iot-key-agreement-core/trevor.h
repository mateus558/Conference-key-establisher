#ifndef TREVOR_H
#define TREVOR_H

#include "iot-key-agreement-core_global.h"
#include "mqttserver.h"

#include <QObject>
#include <QMap>
#include <QVector>
#include <boost/multiprecision/gmp.hpp>

class IOTKEYAGREEMENTCORE_EXPORT Trevor: public QObject
{
    Q_OBJECT
public:
    Trevor(const QString host, const quint16 port, const QString username = 0, const QString password = 0);

    void dropUsers();

    void connectToHost();

    void disconnectFromHost();

    void generateSessionParameters();

    void setNewSession();

    std::vector<int64_t> segmented_sieve(int64_t limit);

    void setHost(const QString &value);

    void setUsername(const QString &value);

    void setPassword(const QString &value);

    void setPort(const quint16 &value);

    void setM(const size_t &value);

    void setN(const size_t &value);

    virtual ~Trevor(){ };

    QMap<QString, boost::multiprecision::mpz_int> getParams() const;

    MQTTServer *getMqtt() const;

public slots:
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);

    void subscribeToTopics();

    void sendLogToGUI(const QString &msg);

signals:
    void userConnected(const QString user);

    void userDisconnected(const QString user);

    void sessionKeyComputed(const QString user, const QString session_key);

    void emitLogMessage(const QString &msg);

    void sessionParamsComputed(QMap<QString, boost::multiprecision::mpz_int>);

private:
    QString host, username, password;
    quint16 port;
    size_t n_users = 0, m = 0, n = 0;
    bool sess_params_computed = false;
    MQTTServer *m_mqtt;
    QVector<QString> users;
    QMap<QString, boost::multiprecision::mpz_int> params;
};

#endif // TREVOR_H
