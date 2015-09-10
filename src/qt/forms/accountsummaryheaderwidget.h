#ifndef ACCOUNTSUMMARYHEADERWIDGET_H
#define ACCOUNTSUMMARYHEADERWIDGET_H

#include <QFrame>

namespace Ui
{
    class AccountSummaryHeaderWidget;
}
class WalletModel;

class AccountSummaryHeaderWidget : public QFrame
{
    Q_OBJECT

public:
    explicit AccountSummaryHeaderWidget(QWidget *parent = 0);
    ~AccountSummaryHeaderWidget();
    void update(QString accountLabel, QString accountAddress, QString Balance, QString Available, QString Interest, QString Pending, bool editable);
    void setModel(WalletModel *model);

    void setColumnWidths(int pendingWidth, int interestWidth, int availableWidth);
private slots:
    void on_edit_account_label_button_pressed();
    void on_cancel_edit_account_label_button_pressed();

    void on_accept_edit_account_label_button_pressed();

    void on_account_header_line_edit_lostFocus();

    void on_account_header_line_edit_returnPressed();

private:
    void accept();
    void cancel();
    WalletModel *model;
    Ui::AccountSummaryHeaderWidget *ui;
};

#endif // ACCOUNTSUMMARYHEADERWIDGET_H
