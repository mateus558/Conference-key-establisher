#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>
#include <cstdlib>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include<boost/multiprecision/number.hpp>
#include "trevor.h"

using namespace boost::multiprecision;

const QString CONNECT_USER = "dcc075/users/connect";
const QString DISCONNECT_USER = "dcc075/users/disconnect";
const QString NUMBER_USERS = "dcc075/users/number_users";
const QString COMMAND_USER = "dcc075/users/command";
const QString PARAM_ALPHA = "dcc075/params/alpha";
const QString PARAM_BETA = "dcc075/params/beta";
const QString PARAM_Y = "dcc075/params/y_param";
const QString PARAM_DELTA = "dcc075/params/delta";
const QString PARAM_TOTIENTDELTA = "dcc075/params/totient_delta";
const QString SESSION_KEY_BOB = "dcc075/sessionkey/bob";
const QString SESSION_KEY_ALICE = "dcc075/sessionkey/alice";

const int64_t L1D_CACHE_SIZE = 32768;

Trevor::Trevor(const QString host, const quint16 port, const QString username, const QString password)
{
    if(!username.isEmpty()){
        m_mqtt = new MQTTServer(host, port, username, password);
    }else {
        m_mqtt = new MQTTServer(host, port);
    }

    QObject::connect(m_mqtt, &MQTTServer::messageReceived, this, &Trevor::onMessageReceived);
    QObject::connect(m_mqtt, &MQTTServer::connected, this, &Trevor::subscribeToTopics);
    QObject::connect(m_mqtt, &MQTTServer::logMessage, this, &Trevor::sendLogToGUI);
}

void Trevor::dropUsers()
{
    for(auto user: users){
        emit userDisconnected(user);
        qDebug() << user << " disconnected.\n";
    }
    users.erase(std::remove_if(this->users.begin(), this->users.end(), [](QString _user){
        return true;
    }), users.end());
    n_users = 0;
}

void Trevor::connectToHost()
{
    if(m_mqtt->state() == QMqttClient::Connected){
        m_mqtt->disconnectFromBroker();
    }
    m_mqtt->setHost(host);
    m_mqtt->setPort(port);
    m_mqtt->setUsername(username);
    m_mqtt->setPassword(password);
    m_mqtt->connectToBroker();
}

void Trevor::disconnectFromHost()
{
    if(m_mqtt->state() == QMqttClient::Connected)
        m_mqtt->disconnectFromBroker();
}

void Trevor::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString topic_name = topic.name();
    QString message_content = QString::fromUtf8(message.data());

    if(message_content.isEmpty()) return;

    if(topic_name == CONNECT_USER){
        bool already_in = false;
        for(auto user: users){
            if(user == message_content){
                already_in = true;
                break;
            }
        }
        if(!already_in){
            users.push_back(message_content);
            n_users++;
            m_mqtt->publish(NUMBER_USERS, QString::number(n_users), 2);
            qDebug() << message_content << " connected.\n";
            emit userConnected(message_content);

            if(n_users == 1){
                qDebug() << "\nGenerating session parameters...\n";
                generateSessionParameters();
                emit sessionParamsComputed(params);
            }

            if(sess_params_computed){
               qDebug() << "\n Parameters: \n";
               qDebug() << "Y: " << QString::fromStdString(params["y"].str()) << "\n";
               qDebug() << "Alpha: " << QString::fromStdString(params["alpha"].str()) << "\n";
               qDebug() << "Beta: " << QString::fromStdString(params["beta"].str()) << "\n";
               qDebug() << "Delta: " << QString::fromStdString(params["delta"].str()) << "\n";
               qDebug() << "Totient delta: " << QString::fromStdString(params["totient_delta"].str()) << "\n";

               qDebug() << "\nSending session parameters.\n";

               m_mqtt->publish(PARAM_Y, QString::fromStdString(params["y"].str()), 2);
               m_mqtt->publish(PARAM_ALPHA, QString::fromStdString(params["alpha"].str()), 2);
               m_mqtt->publish(PARAM_BETA, QString::fromStdString(params["beta"].str()), 2);
               m_mqtt->publish(PARAM_DELTA, QString::fromStdString(params["delta"].str()), 2);
               m_mqtt->publish(PARAM_TOTIENTDELTA, QString::fromStdString(params["totient_delta"].str()), 2);
            }
        }else {
            qDebug() << message_content << " is already connected.\n";
        }
    }

    if(topic_name == SESSION_KEY_BOB){
        emit sessionKeyComputed(QString("Bob"), message_content);
        qDebug() << "\nBob session key: "  << message_content << "\n";
    }
    if(topic_name == SESSION_KEY_ALICE){
        emit sessionKeyComputed(QString("Alice"), message_content);
        qDebug() << "\nAlice session key: "  << message_content << "\n";
    }
    if(topic_name == DISCONNECT_USER){
        int i;
        for(i = 0; i < users.size(); i++){
            if(users[i] == message_content) break;
        }
        if(i == users.size()){
            qWarning() << "\n" << message_content << "is not an user.\n";
            return;
        }
        std::remove_if(users.begin(), users.end(), [&message_content](const QString user){
            return (user == message_content);
        });
        n_users--;
        emit userDisconnected(message_content);
        qDebug() << message_content << " disconnected.\n";
    }
}

void Trevor::subscribeToTopics()
{
    m_mqtt->publish(CONNECT_USER, "", 2, true);
    m_mqtt->subscribe(CONNECT_USER, 2);
    m_mqtt->publish(DISCONNECT_USER, "", 2, true);
    m_mqtt->subscribe(DISCONNECT_USER, 2);
    m_mqtt->publish(SESSION_KEY_BOB, "", 2, true);
    m_mqtt->subscribe(SESSION_KEY_BOB, 2);
    m_mqtt->publish(SESSION_KEY_ALICE, "", 2, true);
    m_mqtt->subscribe(SESSION_KEY_ALICE, 2);
}

void Trevor::sendLogToGUI(const QString &msg)
{
    emit emitLogMessage(msg);
}

MQTTServer *Trevor::getMqtt() const
{
    return m_mqtt;
}

QMap<QString, boost::multiprecision::mpz_int> Trevor::getParams() const
{
    return params;
}

int64_t S64(const char *s) {
    int64_t i;
    char c ;
    int scanned = sscanf(s, "%" SCNd64 "%c", &i, &c);
  if (scanned == 1) return i;
  if (scanned > 1) {
    // TBD about extra data found
    return i;
    }
  // TBD failed to scan;
  return 0;
}

mpz_int phi(mpz_int n)
{
    mpz_int ret = 1,i,pow;
    for (i = 2; n != 1; i++)
    {
        pow = 1;
        if(i>boost::multiprecision::sqrt(n))break;
        while (!(n%i))
        {
            n /= i;
            pow *= i;
        }
        ret *= (pow - (pow/i));
    }
    if(n!=1)ret*=(n-1);
    return ret;
}

void Trevor::generateSessionParameters()
{
    params["delta"] = 1;
    params["alpha"] = 1;
    params["beta"] = 1;
    params["totient_delta"] = 0;
    params["y"] = 0;

    std::vector<int64_t> pm, pn;

    int64_t limit = 1000;
    auto primes = segmented_sieve(limit);

    while(primes.size() < (m+n)) primes = segmented_sieve(limit);

    std::vector<size_t> primes_ids(primes.size());

    std::iota(primes_ids.begin(), primes_ids.end(), 0);
    std::random_shuffle(primes_ids.begin(), primes_ids.end());

    size_t i;
    mpz_int temp_q = 1;
    for(i = 0; i < m; i++){
        int64_t p = primes[primes_ids[i]];
        pm.push_back(p);
        temp_q *= p;
    }

    for(; i < (m + n); i++){
        int64_t p = primes[primes_ids[i]];
        pn.push_back(p);
        mpz_int p_power = 1;
        for(size_t j = 0; j < 3; j++){
            p_power *= p;
        }
        params["delta"] *= p_power;
        params["alpha"] *= (p_power / p) * (p-1);
        params["beta"] *= p*(p-1)*temp_q;
    }

    std::random_device rd;     // only used once to initialise (seed) engine
    std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
    std::uniform_int_distribution<int64_t> uni(1, S64(params["delta"].str().c_str())); // guaranteed unbiased
    params["y"] = uni(rng);
    mpz_int g;
    g = boost::multiprecision::gcd(params["y"], params["delta"]);
    while(g != 1){
        g = boost::multiprecision::gcd(params["y"], params["delta"]);
        params["y"] = uni(rng);
    }
    params["totient_delta"] = phi(params["delta"]);
    sess_params_computed = true;
}

void Trevor::setNewSession()
{
    sess_params_computed = false;
}

std::vector<int64_t> Trevor::segmented_sieve(int64_t limit)
{
    int64_t sqrt = (int64_t) std::sqrt(limit);
    int64_t segment_size = std::max(sqrt, L1D_CACHE_SIZE);
    int64_t count = (limit < 2) ? 0 : 1;

    // we sieve primes >= 3
    int64_t i = 3;
    int64_t n = 3;
    int64_t s = 3;

    std::vector<char> sieve(segment_size);
    std::vector<char> is_prime(sqrt + 1, true);
    std::vector<int64_t> primes;
    std::vector<int64_t> multiples;

    for (int64_t low = 0; low <= limit; low += segment_size)
    {
    std::fill(sieve.begin(), sieve.end(), true);

    // current segment = [low, high]
    int64_t high = low + segment_size - 1;
    high = std::min(high, limit);

    // generate sieving primes using simple sieve of Eratosthenes
    for (; i * i <= high; i += 2)
      if (is_prime[i])
        for (int64_t j = i * i; j <= sqrt; j += i)
          is_prime[j] = false;

    // initialize sieving primes for segmented sieve
    for (; s * s <= high; s += 2)
    {
      if (is_prime[s])
      {
           primes.push_back(s);
        multiples.push_back(s * s - low);
      }
    }

    // sieve the current segment
    for (std::size_t i = 0; i < primes.size(); i++)
    {
      int64_t j = multiples[i];
      for (int64_t k = primes[i] * 2; j < segment_size; j += k)
        sieve[j] = false;
      multiples[i] = j - segment_size;
    }
    primes.clear();
    for (; n <= high; n += 2)
      if (sieve[n - low]) // n is a prime
        primes.push_back(n);
    }

    return primes;
}

void Trevor::setHost(const QString &value)
{
    host = value;
}

void Trevor::setUsername(const QString &value)
{
    username = value;
}

void Trevor::setPassword(const QString &value)
{
    password = value;
}

void Trevor::setPort(const quint16 &value)
{
    port = value;
}


void Trevor::setN(const size_t &value)
{
    n = value;
}

void Trevor::setM(const size_t &value)
{
    m = value;
}
