#include "device.h"
#include <boost/random.hpp>

const QString CONNECT_USER = "dcc075/users/connect";
const QString DISCONNECT_USER = "dcc075/users/disconnect";
const QString NUMBER_USERS = "dcc075/users/number_users";
const QString COMMAND_USER = "dcc075/users/command";
const QString PARAM_ALPHA = "dcc075/params/alpha";
const QString PARAM_BETA = "dcc075/params/beta";
const QString PARAM_Y = "dcc075/params/y_param";
const QString PARAM_DELTA = "dcc075/params/delta";
const QString PARAM_GAMMA = "dcc075/params/gamma";
const QString PARAM_TOTIENTDELTA = "dcc075/params/totient_delta";
const QString SESSION_KEY = "dcc075/sessionkey";

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
    m_mqtt->connectToBroker();

    QObject::connect(m_mqtt, &MQTTServer::messageReceived, this, &Device::onMessageReceived);
    QObject::connect(m_mqtt, &MQTTServer::connected, this, &Device::subscribeToTopics);
}

Device::~Device()
{

}

void Device::compute_gamma()
{
    mpz_int xb_beta, mod;
    mt19937 mt;
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
;
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

    prod_gamma = 1;
    for(int i = 0; i < users.size(); i++){
        if(users[i] == id_mqtt) continue;
        prod_gamma *= gammas[i] % alpha;
    }
    z = (prod_gamma*xb) % alpha;
    session_key = powm(y, z, delta);
    m_mqtt->publish(SESSION_KEY, this->id_mqtt + QString("_") + QString::fromStdString(session_key.str()), 2);
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
    QString message_content = QString::fromUtf8(message.data());

    if(topic_name == NUMBER_USERS){
        n_users = message_content.toInt();
        if(gamma_computed) m_mqtt->publish(PARAM_GAMMA, id_mqtt + QString("_") + QString::fromStdString(gamma.str()), 2);
        if(server_id == 0) server_id = n_users;
    }
    if(!gamma_computed){
        if(y == 0 && topic_name == PARAM_Y){
            y = mpz_int(message_content.toStdString());
        }
        if(alpha == 0 && topic_name == PARAM_ALPHA){
            alpha = mpz_int(message_content.toStdString());
        }
        if(beta == 0 && topic_name == PARAM_BETA){
            beta = mpz_int(message_content.toStdString());
        }
        if(delta == 0 && topic_name == PARAM_DELTA){
            delta = mpz_int(message_content.toStdString());
        }
        if(totient_delta == 0 && topic_name == PARAM_TOTIENTDELTA){
            totient_delta = mpz_int(message_content.toStdString());
        }

        if(y > 0 && alpha > 0 && beta > 0 && delta > 0 && totient_delta > 0){
            compute_gamma();
            std::string msg_gamma(gamma.str());
            msg_gamma = id_mqtt.toStdString() + std::string("_") + msg_gamma;
            m_mqtt->publish(PARAM_GAMMA, QString::fromStdString(msg_gamma), 2);
        }
    }
    if(topic_name == PARAM_GAMMA){
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
    }

}

void Device::subscribeToTopics()
{
    m_mqtt->subscribe(NUMBER_USERS, 2);
    m_mqtt->subscribe(COMMAND_USER, 2);
    m_mqtt->subscribe(PARAM_Y, 2);
    m_mqtt->subscribe(PARAM_ALPHA, 2);
    m_mqtt->subscribe(PARAM_BETA, 2);
    m_mqtt->subscribe(PARAM_DELTA, 2);
    m_mqtt->subscribe(PARAM_TOTIENTDELTA, 2);
    m_mqtt->subscribe(PARAM_GAMMA, 2);
    m_mqtt->publish(CONNECT_USER, id_mqtt);
}
