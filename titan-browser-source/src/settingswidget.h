#pragma once
#include <QWidget>
#include <QStackedWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>

class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget *parent = nullptr);

signals:
    void themeChanged(const QString &theme);

private slots:
    void onNavChanged(int row);
    void onThemeSelected(const QString &theme);
    void onClearDataClicked();

private:
    void setupUi();
    QWidget *createGeneralPage();
    QWidget *createAppearancePage();
    QWidget *createSearchPage();
    QWidget *createPrivacyPage();
    QWidget *createPerformancePage();
    QWidget *createAIPage();
    QWidget *createShortcutsPage();
    QWidget *createAboutPage();

    QListWidget    *m_navList  = nullptr;
    QStackedWidget *m_pages    = nullptr;
};
