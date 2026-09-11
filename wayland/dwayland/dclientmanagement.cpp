// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dclientmanagement.h"

#include "dwaylandintegration.h"
#include "wayland-deepin-client-management-client-protocol.h"

#define private public
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#undef private

#include <QLoggingCategory>

#include <wayland-client-core.h>

DPP_BEGIN_NAMESPACE

#ifndef QT_DEBUG
Q_LOGGING_CATEGORY(lcWaylandSplitMenu, "dtk.qpa.wayland.splitmenu", QtInfoMsg)
#else
Q_LOGGING_CATEGORY(lcWaylandSplitMenu, "dtk.qpa.wayland.splitmenu")
#endif

DClientManagement *DClientManagement::instance()
{
    static DClientManagement *management = new DClientManagement;
    return management;
}

bool DClientManagement::ensureBound()
{
    if (m_management)
        return true;

    auto *display = DWaylandIntegration::instance()->display();
    if (!display) {
        qCDebug(lcWaylandSplitMenu) << "Cannot bind client management without a Wayland display";
        return false;
    }

    const auto globals = display->globals();
    for (const auto &global : globals) {
        if (global.interface != QLatin1String("com_deepin_client_management"))
            continue;

        m_management = static_cast<com_deepin_client_management *>(
            wl_registry_bind(global.registry, global.id,
                             &com_deepin_client_management_interface,
                             qMin(global.version, 1u)));
        if (!m_management) {
            qCWarning(lcWaylandSplitMenu) << "Failed to bind com_deepin_client_management";
            return false;
        }

        static const com_deepin_client_management_listener listener = {
            DClientManagement::windowStates,
            DClientManagement::windowFromPoint,
            DClientManagement::allWindowId,
            DClientManagement::specificWindowState,
        };
        com_deepin_client_management_add_listener(m_management, &listener, this);
        qCDebug(lcWaylandSplitMenu) << "Bound com_deepin_client_management, registry id:"
                                   << global.id << "version:" << qMin(global.version, 1u);
        return true;
    }

    qCDebug(lcWaylandSplitMenu) << "com_deepin_client_management is not advertised";
    return false;
}

bool DClientManagement::isValid()
{
    return ensureBound();
}

void DClientManagement::showSplitMenu(WId wid, const QRect &buttonRect)
{
    if (!wid || !buttonRect.isValid()) {
        qCDebug(lcWaylandSplitMenu) << "Ignore invalid SplitMenu show request, wid:" << wid
                                   << "button rect:" << buttonRect;
        return;
    }

    if (!ensureBound()) {
        qCDebug(lcWaylandSplitMenu) << "Cannot show SplitMenu: client management is unavailable";
        return;
    }

    m_pendingButtonRect = buttonRect;
    m_showPending = true;
    m_windowId = wid;
    m_pendingRequests.enqueue(++m_requestSerial);
    com_deepin_client_management_get_window_from_point(m_management);
    wl_display_flush(DWaylandIntegration::instance()->display()->wl_display());
    qCDebug(lcWaylandSplitMenu) << "Requested window id for SplitMenu, Qt wid:" << wid
                               << "button rect:" << buttonRect;
}

void DClientManagement::hideSplitMenu(WId wid, bool delay)
{
    if (!wid || wid != m_windowId)
        return;
    m_showPending = false;
    ++m_requestSerial;
    if (!ensureBound()) {
        qCDebug(lcWaylandSplitMenu) << "Cannot hide SplitMenu: client management is unavailable";
        return;
    }

    com_deepin_client_management_hide_split_menu(m_management, delay);
    wl_display_flush(DWaylandIntegration::instance()->display()->wl_display());
    qCDebug(lcWaylandSplitMenu) << "Sent SplitMenu hide request, Qt wid:" << wid
                               << "delay:" << delay;
}

void DClientManagement::handleWindowFromPoint(quint32 windowId)
{
    if (m_pendingRequests.isEmpty() || m_pendingRequests.dequeue() != m_requestSerial)
        return;
    if (!m_showPending || !windowId || !m_management) {
        qCDebug(lcWaylandSplitMenu) << "Ignore window-from-point result, pending:" << m_showPending
                                   << "window id:" << windowId
                                   << "management valid:" << bool(m_management);
        return;
    }

    m_showPending = false;
    com_deepin_client_management_show_split_menu(
        m_management,
        m_pendingButtonRect.x(), m_pendingButtonRect.y(),
        m_pendingButtonRect.width(), m_pendingButtonRect.height(),
        windowId);
    wl_display_flush(DWaylandIntegration::instance()->display()->wl_display());
    qCDebug(lcWaylandSplitMenu) << "Sent SplitMenu show request, compositor window id:"
                               << windowId << "button rect:" << m_pendingButtonRect;
}

void DClientManagement::windowStates(void *data, com_deepin_client_management *management,
                                     quint32 count, wl_array *states)
{
    Q_UNUSED(data)
    Q_UNUSED(management)
    Q_UNUSED(count)
    Q_UNUSED(states)
}

void DClientManagement::windowFromPoint(void *data, com_deepin_client_management *management,
                                        quint32 windowId)
{
    Q_UNUSED(management)
    static_cast<DClientManagement *>(data)->handleWindowFromPoint(windowId);
}

void DClientManagement::allWindowId(void *data, com_deepin_client_management *management,
                                    wl_array *idArray)
{
    Q_UNUSED(data)
    Q_UNUSED(management)
    Q_UNUSED(idArray)
}

void DClientManagement::specificWindowState(
    void *data, com_deepin_client_management *management,
    qint32 pid, quint32 windowId, const char *resourceName,
    qint32 x, qint32 y, qint32 width, qint32 height,
    qint32 isMinimized, qint32 isFullscreen, qint32 isActive,
    qint32 splitable, const char *uuid)
{
    Q_UNUSED(data)
    Q_UNUSED(management)
    Q_UNUSED(pid)
    Q_UNUSED(windowId)
    Q_UNUSED(resourceName)
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(width)
    Q_UNUSED(height)
    Q_UNUSED(isMinimized)
    Q_UNUSED(isFullscreen)
    Q_UNUSED(isActive)
    Q_UNUSED(splitable)
    Q_UNUSED(uuid)
}

DPP_END_NAMESPACE
