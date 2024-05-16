#ifndef CHATBOT_H
#define CHATBOT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ChatBot : public QObject
{
    Q_OBJECT

public:
    ChatBot(QObject *parent = nullptr);

    void reply(const QMap<QString, QString> &map);
signals:
    void replyReceived(QString);
    void finish();
private slots:
    void readResponseData();

private:
    QNetworkAccessManager *manager;
    QNetworkReply *m_reply;
};

#endif // CHATBOT_H
