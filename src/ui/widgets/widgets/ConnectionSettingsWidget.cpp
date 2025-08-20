#include "ConnectionSettingsWidget.hpp"
#include "core/handler/ConfigHandler.hpp"

ConnectionSettingsWidget::ConnectionSettingsWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
}

void ConnectionSettingsWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange: retranslateUi(this); break;
        default: break;
    }
}

void ConnectionSettingsWidget::LoadFromConnection(const ConnectionId &id)
{
    auto meta = ConnectionManager->GetConnectionMetaObject(id);
    const QString &pref = meta.preferredKernel;
    int idx = connectionPreferredKernelCombo->findText(pref.isEmpty() ? "(inherit global)" : pref, Qt::MatchFixedString);
    if (idx < 0) idx = 0;
    connectionPreferredKernelCombo->setCurrentIndex(idx);
}

void ConnectionSettingsWidget::SaveToConnection(const ConnectionId &id)
{
    auto meta = ConnectionManager->GetConnectionMetaObject(id);
    const auto text = connectionPreferredKernelCombo->currentText();
    const QString value = (text == "(inherit global)") ? QString("") : text;
    if (meta.preferredKernel != value)
    {
        meta.preferredKernel = value;
        // Write back to storage and notify
        // Keep root unchanged; only metadata updates
        ConnectionManager->RenameConnection(id, meta.displayName); // trigger save path
        // Save connections file again with updated meta
        ConnectionManager->SaveConnectionConfig();
    }
}
