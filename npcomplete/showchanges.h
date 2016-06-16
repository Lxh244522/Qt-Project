#ifndef SHOWCHANGES_H
#define SHOWCHANGES_H

#include <QObject>

class ShowChanges : public QObject
{
    Q_OBJECT
public:
    explicit ShowChanges(QObject *parent = 0);
    ~ShowChanges();
signals:

public slots:
    //槽函数，接收value变化信号
    void RecvValue(double v);
    //槽函数，接收nickName变化信号
    void RecvNickName(const QString& strNewName);
    //槽函数，接收cout变化
    void RecvCount(int nNewCount);
};

#endif // SHOWCHANGES_H
