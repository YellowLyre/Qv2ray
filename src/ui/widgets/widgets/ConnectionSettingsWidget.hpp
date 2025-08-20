#pragma once

#include "ui_ConnectionSettingsWidget.h"
#include "core/handler/ConfigHandler.hpp"

class ConnectionSettingsWidget
    : public QWidget
    , private Ui::ConnectionSettingsWidget
{
    Q_OBJECT

  public:
    explicit ConnectionSettingsWidget(QWidget *parent = nullptr);
    void LoadFromConnection(const ConnectionId &id);
    void SaveToConnection(const ConnectionId &id);

  protected:
    void changeEvent(QEvent *e);
};
