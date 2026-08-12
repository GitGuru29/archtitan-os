#include "tabwidget.h"
#include <QTabBar>
#include <QStackedWidget>
#include <QWebEngineView>
#include <QIcon>

TabWidget::TabWidget(QWidget *parent) : QObject(parent)
{
    m_tabBar = new QTabBar(parent);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setExpanding(false);

    m_stack = new QStackedWidget(parent);

    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &TabWidget::onTabCloseRequested);
    connect(m_tabBar, &QTabBar::currentChanged, this, &TabWidget::onCurrentTabChanged);
}

QWebEngineView *TabWidget::newTab(const QUrl &url)
{
    auto *view = new QWebEngineView(m_stack);
    if (!url.isEmpty()) {
        view->setUrl(url);
    }

    int stackIndex = m_stack->addWidget(view);
    int tabIndex = m_tabBar->addTab(QIcon(QStringLiteral(":/icons/home.svg")), QStringLiteral("New Tab"));

    m_tabBar->setCurrentIndex(tabIndex);
    m_stack->setCurrentIndex(stackIndex);

    connectView(view);
    return view;
}

QWebEngineView *TabWidget::currentView() const
{
    return qobject_cast<QWebEngineView *>(m_stack->currentWidget());
}

void TabWidget::connectView(QWebEngineView *view)
{
    connect(view, &QWebEngineView::urlChanged, this, [this, view](const QUrl &url) {
        if (view == m_stack->currentWidget()) emit urlChanged(url);
    });

    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &title) {
        int idx = m_stack->indexOf(view);
        if (idx >= 0 && idx < m_tabBar->count()) {
            QString displayTitle = title.isEmpty() ? QStringLiteral("New Tab") : title;
            m_tabBar->setTabText(idx, displayTitle.left(22));
            m_tabBar->setTabToolTip(idx, title);
        }
        if (view == m_stack->currentWidget()) emit titleChanged(title);
    });

    connect(view, &QWebEngineView::loadProgress, this, [this, view](int p) {
        if (view == m_stack->currentWidget()) emit loadProgress(p);
    });

    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        if (view == m_stack->currentWidget()) emit loadFinished(ok);
    });

    connect(view, &QWebEngineView::iconChanged, this, [this, view](const QIcon &icon) {
        int idx = m_stack->indexOf(view);
        if (idx >= 0 && idx < m_tabBar->count()) {
            if (!icon.isNull()) {
                m_tabBar->setTabIcon(idx, icon);
            }
        }
    });
}

void TabWidget::onCurrentTabChanged(int index)
{
    if (index >= 0 && index < m_stack->count()) {
        m_stack->setCurrentIndex(index);
        if (auto *v = qobject_cast<QWebEngineView *>(m_stack->widget(index))) {
            emit urlChanged(v->url());
            emit titleChanged(v->title());
        }
    }
}

void TabWidget::onTabCloseRequested(int index)
{
    if (m_tabBar->count() <= 1) return; // Keep at least one tab

    auto *v = qobject_cast<QWebEngineView *>(m_stack->widget(index));
    m_tabBar->removeTab(index);
    if (v) {
        m_stack->removeWidget(v);
        delete v;
    }
}

void TabWidget::removeTab(int index)
{
    onTabCloseRequested(index);
}

QUrl TabWidget::tabUrl(int index) const
{
    if (index >= 0 && index < m_stack->count()) {
        if (auto *v = qobject_cast<QWebEngineView *>(m_stack->widget(index)))
            return v->url();
    }
    return QUrl();
}
