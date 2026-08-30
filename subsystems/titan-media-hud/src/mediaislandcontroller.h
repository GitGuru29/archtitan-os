#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

/**
 * MediaIslandController — State machine for the Titan Media Island.
 *
 * States:
 *   Hidden    — overlay not visible, no animations running
 *   Opening   — emerge-from-Waybar animation in progress
 *   Compact   — small pill visible
 *   Expanding — compact→expanded transition
 *   Expanded  — full island visible
 *   Collapsing— expanded→compact transition
 *   Closing   — retract-into-Waybar animation in progress
 */
class MediaIslandController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool isVisible READ isVisible NOTIFY stateChanged)
    Q_PROPERTY(bool isHidden READ isHidden NOTIFY stateChanged)
    Q_PROPERTY(bool isOpening READ isOpening NOTIFY stateChanged)
    Q_PROPERTY(bool isCompact READ isCompact NOTIFY stateChanged)
    Q_PROPERTY(bool isExpanding READ isExpanding NOTIFY stateChanged)
    Q_PROPERTY(bool isExpanded READ isExpanded NOTIFY stateChanged)
    Q_PROPERTY(bool isCollapsing READ isCollapsing NOTIFY stateChanged)
    Q_PROPERTY(bool isClosing READ isClosing NOTIFY stateChanged)
    Q_PROPERTY(bool autoPopupEnabled READ autoPopupEnabled WRITE setAutoPopupEnabled NOTIFY autoPopupEnabledChanged)

public:
    enum State {
        Hidden,
        Opening,
        Compact,
        Expanding,
        Expanded,
        Collapsing,
        Closing
    };
    Q_ENUM(State)

    explicit MediaIslandController(QObject *parent = nullptr);

    State state() const { return m_state; }
    bool isVisible() const { return m_state != Hidden; }
    bool isHidden() const { return m_state == Hidden; }
    bool isOpening() const { return m_state == Opening; }
    bool isCompact() const { return m_state == Compact; }
    bool isExpanding() const { return m_state == Expanding; }
    bool isExpanded() const { return m_state == Expanded; }
    bool isCollapsing() const { return m_state == Collapsing; }
    bool isClosing() const { return m_state == Closing; }

    bool autoPopupEnabled() const { return m_autoPopupEnabled; }
    void setAutoPopupEnabled(bool enabled);

public slots:
    /** Toggle: Hidden→Opening or (Compact|Expanded)→Closing */
    void toggle();

    /** Dismiss (ESC): any visible state → Closing */
    void dismiss();

    /** Compact→Expanding */
    void expand();

    /** Expanded→Collapsing */
    void collapse();

    /** Called by QML when an opening/expanding animation finishes */
    void onOpenAnimationFinished();

    /** Called by QML when a closing/collapsing animation finishes */
    void onCloseAnimationFinished();

    /** Called by QML when expand animation finishes */
    void onExpandAnimationFinished();

    /** Called by QML when collapse animation finishes */
    void onCollapseAnimationFinished();

    /** Auto-popup on track change (debounced) */
    void autoPopup();

signals:
    void stateChanged();
    void autoPopupEnabledChanged();

    // QML animation triggers
    void requestOpen();
    void requestClose();
    void requestExpand();
    void requestCollapse();

private:
    void setState(State newState);

    State m_state{Hidden};
    bool m_autoPopupEnabled{true};
    QTimer m_autoHideTimer;
    QTimer m_debounceTimer;
};
