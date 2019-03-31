#include "dusttcpsock.h"

#include <QDebug>

#ifdef DUSTTCP_H
DustTcpSock::DustTcpSock(QObject *parent , QString *ip, quint16 *port) : QObject (parent)

{


    m_sock = new QTcpSocket(this);

    connect(m_sock, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(m_sock, SIGNAL(bytesWritten(qint64)), this, SLOT(writes()));

    connect(m_sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
    //connect(this, SIGNAL(dataReady(QByteArray&)), this, SLOT(setData(QByteArray&)) );

    changeInterface(*ip, *port);
    m_sock->setSocketOption(QAbstractSocket::LowDelayOption, 0);
     //qDebug() << "Socket " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);
   // m_sock->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024);
   // qDebug() << "Socket next " << m_sock->socketOption(QAbstractSocket::SendBufferSizeSocketOption);

    m_sock->setSocketOption(QAbstractSocket::TypeOfServiceOption, 64);

    measure = new  QMap<QString, int>;
    measure->insert("PM1", 0);
    measure->insert("PM2.5", 0);
    measure->insert("PM4", 0);
    measure->insert("PM10", 0);
    measure->insert("PM", 0);

    is_read = false;
    status = "";
    qDebug() << "Dust measure equipment handling has been initialized.";

}
#endif

#ifdef DUSTUDP_H
DustTcpSock::DustTcpSock(QObject *parent , QString *ip, quint16 *port) : QObject (parent)

{


    m_sock = new QUdpSocket(this);

    connect(m_sock, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(m_sock, SIGNAL(bytesWritten(qint64)), this, SLOT(writes()));

    connect(m_sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
    //connect(this, SIGNAL(dataReady(QByteArray&)), this, SLOT(setData(QByteArray&)) );

    changeInterface(*ip, *port);
    m_sock->setSocketOption(QAbstractSocket::LowDelayOption, 0);

    measure = new  QMap<QString, int>;
    measure->insert("PM1", 0);
    measure->insert("PM2.5", 0);
    measure->insert("PM4", 0);
    measure->insert("PM10", 0);
    measure->insert("PM", 0);

    is_read = false;
    status = "";
    qDebug() << "Dust measure equipment handling has been initialized.";

}
#endif

DustTcpSock::~DustTcpSock()
{
    m_sock->disconnectFromHost();
}



void DustTcpSock::changeInterface(const QString &address, quint16 portNbr)
{
    m_sock->connectToHost(address, portNbr);
}





void DustTcpSock::on_cbEnabled_clicked(bool checked)
{
    if (checked)
    {
    }
    else {
        m_sock->disconnectFromHost();
    }
    //emit tcpPortActive(checked);
}


void DustTcpSock::readData()
{

    QStringList list;
    int ind;
    int running;
    QRegExp xRegExp("(-?\\d+(?:[\\.,]\\d+(?:e\\d+)?)?)");

    QByteArray data = m_sock->readAll();
    switch (last_command) {
    case RMMEAS :
        list = QString(QString(data).remove(QRegExp("[\r\n{\r\n}]"))).split(QRegExp(","));
        running = QString(data).indexOf("Running");
        ind = xRegExp.indexIn(list.at(0));

        if (running != -1) status = "Running";

        if ((list.at(0).mid(ind).toUInt() - sample_t ) >0) // Idle status detecting
        {
            sample_t = list.at(0).mid(ind).toUInt();
            measure->insert("PM1", int(list.at(1).toFloat()*1000));
            measure->insert("PM2.5", int(list.at(2).toFloat()*1000));
            measure->insert("PM4", int(list.at(3).toFloat()*1000));
            measure->insert("PM10", int(list.at(4).toFloat()*1000));
            measure->insert("PM", int(list.at(5).toFloat()*1000));
        }
        else
        {
            measure->insert("PM1", 0);
            measure->insert("PM2.5", 0);
            measure->insert("PM4", 0);
            measure->insert("PM10", 0);
            measure->insert("PM", 0);
        }
            break;

    case MSTATUS :
                status = QString(data).remove(QRegExp("[\r\n{\r\n}]"));
                break;

            case RDMN :
                model = QString(data).remove(QRegExp("[\r\n{\r\n}]"));
                break;

            default: break;
        }

        qDebug() << "Dust measure equipment data: " << data << " lenght - " << data.length() << " \n";

        this->is_read = true;

        emit (dataReady(data));


        blockSize = 0;

    }

    void DustTcpSock::displayError(QAbstractSocket::SocketError socketError)
    {
        switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
            qDebug()<<   ("Dust measure equipment handling error: The host was not found. Please check the "
                          "host name and port settings.");
            break;
        case QAbstractSocket::ConnectionRefusedError:
            qDebug()<< ("Dust measure equipment handling error: The connection was refused by the peer. "
                        "Make sure the fortune server is running, "
                        "and check that the host name and port "
                        "settings are correct.");
            break;
        default:
            qDebug()<< ("Dust measure equipment handling error: ") << (m_sock->errorString());
        }

    }

    void DustTcpSock::sendData( char *data)
    {
        if (strcmp(data, "RDMN")==0)
            last_command = RDMN;

        if (strcmp(data, "MSTART")==0)
            last_command = MSTART;

        if (strcmp(data, "MSTOP")==0)
            last_command = MSTOP;

        if (strcmp(data, "MSTATUS")==0)
            last_command = MSTATUS;

        if (strcmp(data, "RMMEAS")==0)
            last_command = RMMEAS;

        char *str = (char*)(malloc(strlen(data) * sizeof(char) + 1));
        *str = '\0';
        strcat(str, data);
        strcat(str,  "\r");
        qint64 lnt = qint64(strlen(str));

        lnt = m_sock->write(str, lnt);
        // lnt = m_sock->flush();

        qDebug()<< "Dust command: " << data ;
    }

    void DustTcpSock::writes()
    {
        qDebug()<< "written " ;
    }
