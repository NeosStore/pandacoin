#ifndef RICHTEXTCOMBO_H
#define RICHTEXTCOMBO_H

#include <QComboBox>

class QLabel;

class RichTextCombo : public QComboBox
{
    Q_OBJECT
public:
    explicit RichTextCombo(QWidget *parent = 0);
    void paintEvent(QPaintEvent *e);

signals:

public slots:
private:
    QLabel* renderControl;
};

#endif // RICHTEXTCOMBO_H
