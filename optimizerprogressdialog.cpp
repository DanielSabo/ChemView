#include "optimizerprogressdialog.h"

#include <QVBoxLayout>

OptimizerProgressDialog::OptimizerProgressDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Calculation Progress");
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    auto l = new QVBoxLayout(this);
    message = new QLabel;
    cancelButton = new QPushButton("Cancel");
    progressBar = new QProgressBar;
    progressBar->setRange(0, 0);

    this->setLayout(l);
    l->addWidget(message);
    l->addWidget(progressBar);
    l->addWidget(cancelButton);

    connect(cancelButton, &QPushButton::clicked, this, [this](bool){
        this->close();
    });
}

void OptimizerProgressDialog::setText(QString text)
{
    message->setText(text);
}

QString OptimizerProgressDialog::getText()
{
    return message->text();
}
