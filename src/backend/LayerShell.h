#pragma once

#include <QObject>
#include <QWindow>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

class LayerShell : public QObject
{
    Q_OBJECT
    // The QML Window we are attaching to
    Q_PROPERTY(QWindow* window READ window WRITE setWindow NOTIFY windowChanged)

public:
    explicit LayerShell(QObject *parent = nullptr);
    ~LayerShell();

    QWindow* window() const;
    void setWindow(QWindow *window);

    // Call this to activate the shell logic
    Q_INVOKABLE void activate();

signals:
    void windowChanged();

public:
    // Wayland Static Callbacks (must be public for C callback access)
    static void registryHandleGlobal(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    static void registryHandleGlobalRemove(void *data, struct wl_registry *registry, uint32_t name);
    static void layerSurfaceHandleConfigure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t width, uint32_t height);
    static void layerSurfaceHandleClosed(void *data, struct zwlr_layer_surface_v1 *surface);

private:
    void initWayland();
    void createLayerSurface();

    // Member Variables
    QWindow *m_window = nullptr;
    struct wl_display *m_wlDisplay = nullptr;
    struct wl_registry *m_wlRegistry = nullptr;
    struct zwlr_layer_shell_v1 *m_layerShell = nullptr;
    struct zwlr_layer_surface_v1 *m_layerSurface = nullptr;
    struct wl_surface *m_wlSurface = nullptr;
    
    bool m_configured = false;
};
