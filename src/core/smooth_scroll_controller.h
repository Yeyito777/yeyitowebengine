// Copyright (C) 2026 Yeyito. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef SMOOTH_SCROLL_CONTROLLER_H
#define SMOOTH_SCROLL_CONTROLLER_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace QtWebEngineCore {

class RenderWidgetHostViewQt;

class SmoothScrollController : public QObject
{
    Q_OBJECT
public:
    explicit SmoothScrollController(RenderWidgetHostViewQt *rwhv, QObject *parent = nullptr);
    ~SmoothScrollController();

    void scrollBy(int dx, int dy, double factor);
    void stop();

private Q_SLOTS:
    void tick();

private:
    void sendGestureScrollBegin();
    void sendGestureScrollUpdate(int stepX, int stepY);
    void sendGestureScrollEnd();

    RenderWidgetHostViewQt *m_rwhv;
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    double m_dx = 0.0;
    double m_dy = 0.0;
    double m_subPixelX = 0.0;
    double m_subPixelY = 0.0;
    double m_factor = 0.3;
    bool m_scrolling = false;
};

} // namespace QtWebEngineCore

#endif // SMOOTH_SCROLL_CONTROLLER_H
