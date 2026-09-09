#pragma once
#include <QWidget>
#include <QUrl>
#include <QLineEdit>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QList>

struct SpeedDial {
    QString name;
    QString url;
    QString color;
};

class NewTabWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NewTabWidget(QWidget *parent = nullptr);

signals:
    void navigateRequested(const QUrl &url);
    void openAIAssistantRequested();

public slots:
    void focusSearch();

private slots:
    void updateClockAndGreeting();
    void onSearchSubmitted();
    void onSearchEngineClicked();
    void onAddShortcutClicked();
    void onOpenToolClicked(const QString &toolName);
    void onSpaceSelected(const QString &spaceName);

private:
    void setupUi();
    void setupHeader(QVBoxLayout *layout);
    void setupSearchBar(QVBoxLayout *layout);
    void setupSpaces(QVBoxLayout *layout);
    void setupSpeedDials(QVBoxLayout *layout);
    void setupDevTools(QVBoxLayout *layout);

    void loadShortcuts();
    void saveShortcuts();
    void renderShortcuts();

    QLabel      *m_clockLabel    = nullptr;
    QLabel      *m_greetingLabel = nullptr;
    QLineEdit   *m_searchInput   = nullptr;
    QPushButton *m_engineBtn     = nullptr;
    QGridLayout *m_dialsLayout   = nullptr;
    QWidget     *m_dialsContainer= nullptr;

    QList<SpeedDial> m_shortcuts;
    QList<QPushButton*> m_spaceButtons;
    QString m_activeSpace = QStringLiteral("Development");

    int m_currentEngineIdx = 0;
};
