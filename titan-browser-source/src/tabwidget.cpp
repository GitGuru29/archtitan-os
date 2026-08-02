#include "tabwidget.h"
#include <QTabBar>
#include <QWebEngineView>
#include <QWebEngineProfile>

TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent)
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
    setElideMode(Qt::ElideRight);

    connect(this, &QTabWidget::tabCloseRequested,
            this, &TabWidget::onTabCloseRequested);
    connect(this, &QTabWidget::currentChanged, this, [this](int idx) {
        if (auto *v = qobject_cast<QWebEngineView *>(widget(idx))) {
            emit urlChanged(v->url());
            emit titleChanged(v->title());
        }
    });
}

QWebEngineView *TabWidget::newTab(const QUrl &url)
{
    auto *view = new QWebEngineView(this);
    view->setUrl(url);

    int idx = addTab(view, QStringLiteral("New Tab"));
    setCurrentIndex(idx);
    connectView(view);

    return view;
}

QWebEngineView *TabWidget::currentView() const
{
    return qobject_cast<QWebEngineView *>(currentWidget());
}

void TabWidget::connectView(QWebEngineView *view)
{
    connect(view, &QWebEngineView::urlChanged, this, [this, view](const QUrl &url) {
        if (view == currentWidget()) emit urlChanged(url);
    });
    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &title) {
        int idx = indexOf(view);
        if (idx >= 0) {
            setTabText(idx, title.left(28));
            setTabToolTip(idx, title);
        }
        if (view == currentWidget()) emit titleChanged(title);
    });
    connect(view, &QWebEngineView::loadProgress, this, [this, view](int p) {
        if (view == currentWidget()) emit loadProgress(p);
    });
    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        if (view == currentWidget()) emit loadFinished(ok);
    });
}

void TabWidget::onTabCloseRequested(int index)
{
    if (count() <= 1) return; // keep at least one tab
    auto *v = qobject_cast<QWebEngineView *>(widget(index));
    removeTab(index);
    delete v;
}
