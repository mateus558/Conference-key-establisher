#include "device.h"
#include <boost/random.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <random>

using namespace boost::multiprecision;
using namespace boost::random;


Device::Device(const QString &host, const int port, const QString &user, const QString &password)
{
    if(!user.isEmpty()){
        m_mqtt = new MQTTServer(host, port, user, password);
    }else {
        m_mqtt = new MQTTServer(host, port);
    }

    std::string id(10, 0);
    generate_random_id(id, 10);
    id_mqtt = QString::fromStdString(id);
    qDebug() << "MQTT id: " << id_mqtt << "\n";
    m_mqtt->setIdMqtt(id_mqtt);
    m_mqtt->connectToBroker();

    QObject::connect(m_mqtt, &MQTTServer::messageReceived, this, &Device::onMessageReceived);
    QObject::connect(m_mqtt, &MQTTServer::connected, this, &Device::subscribeToTopics);
}

Device::~Device()
{
    m_mqtt->publish(DISCONNECT_USER, id_mqtt, 2);
    m_mqtt->disconnectFromBroker();
}

void Device::compute_gamma()
{
    mpz_int xb_beta, mod;
    std::random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<mpz_int> uid(0, delta);
    uniform_int_distribution<mpz_int> uit(0, totient_delta);

    xa = uid(mt);
    xb = uit(mt);
    while(xb*beta % totient_delta == 0){
        xb = uit(mt);
    }
    gamma = boost::multiprecision::pow(xa, 2) * alpha + xb * beta;
    users.push_back(id_mqtt);
    gammas.push_back(gamma);
    gamma_computed = true;
}

mpz_int fastExp(mpz_int b, mpz_int e, mpz_int m)
{
    mpz_int result = e & 1 ? b : 1;
    while (e) {
        e >>= 1;
        b = (b * b) % m;
        if (e & 1)
            result = (result * b) % m;
    }
    return result;
}

void Device::compute_session_key()
{
    mpz_int prod_gamma, z;

    if(y > 0 && alpha > 0 && beta > 0 && delta > 0 && totient_delta > 0){
        prod_gamma = 1;
        for(int i = 0; i < users.size(); i++){
            if(users[i] == id_mqtt) continue;
            prod_gamma *= gammas[i] % alpha;
        }
        z = (prod_gamma*xb) % alpha;
        session_key = powm(y, z, delta);
        cpp_dec_float_50 bits = boost::multiprecision::log2(session_key.convert_to<cpp_dec_float_50>());
        qDebug() << "number of bits: " << QString::fromStdString(bits.convert_to<mpz_int>().str()) << "\n";
        m_mqtt->publish(SESSION_KEY, this->id_mqtt + QString("_") + QString::fromStdString(session_key.str()), 2);
        session_key_computed = true;
    }
}

std::string Device::generate_random_id(std::string &str, size_t length)
{
    auto randchar = []() -> char{
        const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::generate_n(str.begin(), length, randchar);
    return str;
}

void Device::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString topic_name = topic.name();
    timer.start();
    QString message_content = QString::fromUtf8(message.data());
    total_time += timer.elapsed();

    if(topic_name == COMMAND_USER){
        QStringList pieces = message_content.split('_');
        if(pieces[0] != id_mqtt) return;
        if(pieces[1] == QString("accepted")){
            accepted = true;
        }
        if(pieces[1] == QString("sendgamma")){
            if(gamma_computed) m_mqtt->publish(PARAM_GAMMA, id_mqtt + QString("_") + QString::fromStdString(gamma.str()), 2);
        }
    }

    if(accepted) qDebug() << id_mqtt << " " << message << " " << topic;

    if(topic_name == NUMBER_USERS){
        n_users = message_content.toInt();
        timer.start();
        if(gamma_computed) m_mqtt->publish(PARAM_GAMMA, id_mqtt + QString("_") + QString::fromStdString(gamma.str()), 2);
        total_time += timer.elapsed();
        if(server_id == 0) server_id = n_users;
    }
    if(topic_name == DISCONNECT_USER){
        auto it = users.begin();
        size_t i;
        for(i = 0; it != users.end(); it++, i++){
            if((*it) == message_content) break;
        }
        users.erase(it);
        gammas.erase(gammas.begin()+i);
        n_users--;
        if(n_users > 1 && n_users == gammas.size()) compute_session_key();
        qDebug() << QString("[") + id_mqtt + QString("]: ") + message_content << " disconnected.\n";
    }

    if(!gamma_computed){
        total_time += timer.elapsed();
        if(y == 0 && topic_name == PARAM_Y){
            y = mpz_int(message_content.toStdString());
        }else if(alpha == 0 && topic_name == PARAM_ALPHA){
            alpha = mpz_int(message_content.toStdString());
        }else if(beta == 0 && topic_name == PARAM_BETA){
            beta = mpz_int(message_content.toStdString());
        }else if(delta == 0 && topic_name == PARAM_DELTA){
            delta = mpz_int(message_content.toStdString());
        }else if(totient_delta == 0 && topic_name == PARAM_TOTIENTDELTA){
            totient_delta = mpz_int(message_content.toStdString());
        }

        if(y > 0 && alpha > 0 && beta > 0 && delta > 0 && totient_delta > 0){
            compute_gamma();
            std::string msg_gamma(gamma.str());
            msg_gamma = id_mqtt.toStdString() + std::string("_") + msg_gamma;
            m_mqtt->publish(PARAM_GAMMA, QString::fromStdString(msg_gamma), 2);
        }
        total_time += timer.elapsed();
    }
    if(topic_name == PARAM_GAMMA && accepted){
        timer.start();
        QStringList pieces = message_content.split('_');
        bool compute = true;
        QString user_id = pieces[0], user_gamma = pieces[1];
        int i;
        for(i = 0; i < users.size(); i++){
            if(users[i] == user_id){
                compute = false;
                break;
            }
        }

        if(i < users.size()){
            gammas[i].assign(user_gamma.toStdString());
        }else{
            gammas.push_back(mpz_int(user_gamma.toStdString()));
        }
        if(i == users.size() || users.size() == 0) users.push_back(user_id);

        if(n_users > 1 && n_users == gammas.size() && compute) compute_session_key();
        total_time += timer.elapsed();
    }
    if(n_users == n_cobaias && session_key_computed && on_experimentation){
        emit emitTotalTime(total_time);
    }
}

void Device::subscribeToTopics()
{
    PARAM_Y += QString("_")+id_mqtt;
    PARAM_ALPHA += QString("_")+id_mqtt;
    PARAM_BETA += QString("_")+id_mqtt;
    PARAM_DELTA += QString("_")+id_mqtt;
    PARAM_TOTIENTDELTA += QString("_")+id_mqtt;

    m_mqtt->subscribe(NUMBER_USERS, 2);
    m_mqtt->subscribe(COMMAND_USER, 2);
    m_mqtt->subscribe(PARAM_Y, 2);
    m_mqtt->subscribe(PARAM_ALPHA, 2);
    m_mqtt->subscribe(PARAM_BETA, 2);
    m_mqtt->subscribe(PARAM_DELTA, 2);
    m_mqtt->subscribe(PARAM_TOTIENTDELTA, 2);
    m_mqtt->subscribe(PARAM_GAMMA, 2);
    m_mqtt->subscribe(DISCONNECT_USER, 2);
    m_mqtt->publish(CONNECT_USER, id_mqtt);
}

void Device::setN_cobaias(const size_t &value)
{
    on_experimentation = true;
    n_cobaias = value;
}

QString Device::getId_mqtt() const
{
    return id_mqtt;
}
