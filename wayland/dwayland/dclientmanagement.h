// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DCLIENTMANAGEMENT_H
#define DCLIENTMANAGEMENT_H

#include "global.h"

#include <QRect>
#include <QQueue>
#include <qwindowdefs.h>

struct com_deepin_client_management;
struct wl_array;

DPP_BEGIN_NAMESPACE

class DClientManagement
{
public:
    static DClientManagement *instance();

    bool isValid();
    void showSplitMenu(WId wid, const QRect &buttonRect);
    void hideSplitMenu(WId wid, bool delay);

private:
    DClientManagement() = default;

    bool ensureBound();
    void handleWindowFromPoint(quint32 windowId);

    static void windowStates(void *data, com_deepin_client_management *management,
                             quint32 count, wl_array *states);
    static void windowFromPoint(void *data, com_deepin_client_management *management,
                                quint32 windowId);
    static void allWindowId(void *data, com_deepin_client_management *management,
                            wl_array *idArray);
    static void specificWindowState(void *data, com_deepin_client_management *management,
                                    qint32 pid, quint32 windowId, const char *resourceName,
                                    qint32 x, qint32 y, qint32 width, qint32 height,
                                    qint32 isMinimized, qint32 isFullscreen, qint32 isActive,
                                    qint32 splitable, const char *uuid);

    com_deepin_client_management *m_management = nullptr;
    QRect m_pendingButtonRect;
    bool m_showPending = false;
    quint64 m_requestSerial = 0;
    QQueue<quint64> m_pendingRequests;
    WId m_windowId = 0;
};

DPP_END_NAMESPACE

#endif // DCLIENTMANAGEMENT_H
