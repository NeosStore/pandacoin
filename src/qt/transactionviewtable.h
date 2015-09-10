#ifndef TRANSACTIONVIEWTABLE_H
#define TRANSACTIONVIEWTABLE_H

#include <QTableView>

class QResizeEvent;

class TransactionViewTable : public QTableView
{
    Q_OBJECT
public:
    explicit TransactionViewTable(QWidget *parent = 0);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void setModel(QAbstractItemModel *model);

signals:

public slots:
    void updateColumns();

};

#endif // TRANSACTIONVIEWTABLE_H
