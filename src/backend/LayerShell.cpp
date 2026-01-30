#include "LayerShell.h"
#include <QDebug>
#include <QGuiApplication>
#include <QWindow>

// Include the actual header for QPlatformNativeInterface
// This is a private Qt header but necessary for Wayland native access
#include <qpa/qplatformnativeinterface.h>

// Static Listener Structs
static const struct wl_registry_listener registry_listener = {
    LayerShell::registryHandleGlobal,
    LayerShell::registryHandleGlobalRemove
};

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    LayerShell::layerSurfaceHandleConfigure,
    LayerShell::layerSurfaceHandleClosed
};

LayerShell::LayerShell(QObject *parent) : QObject(parent)
{
}

LayerShell::~LayerShell()
{
    if (m_layerSurface) zwlr_layer_surface_v1_destroy(m_layerSurface);
    if (m_layerShell) zwlr_layer_shell_v1_destroy(m_layerShell);
    // Do NOT destroy m_wlDisplay or m_wlRegistry; Qt owns those.
}

QWindow* LayerShell::window() const { return m_window; }

void LayerShell::setWindow(QWindow *window) {
    if (m_window != window) {
        m_window = window;
        emit windowChanged();
    }
}

void LayerShell::activate()
{
    if (!m_window) {
        qWarning() << "LayerShell: No window set!";
        return;
    }
    initWayland();
}

void LayerShell::initWayland()
{
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    if (!native) {
        qWarning() << "LayerShell: Failed to get QPlatformNativeInterface";
        return;
    }

    // Get Qt's Wayland Display
    m_wlDisplay = static_cast<struct wl_display *>(
        native->nativeResourceForIntegration("wl_display"));
    if (!m_wlDisplay) {
        qWarning() << "LayerShell: Could not retrieve wl_display. Are you running on Wayland?";
        return;
    }

    // Get the Registry manually to find the Layer Shell global
    // We create our own registry wrapper to avoid interfering with Qt's
    m_wlRegistry = wl_display_get_registry(m_wlDisplay);
    wl_registry_add_listener(m_wlRegistry, &registry_listener, this);
    
    // Roundtrip to ensure we get the globals
    wl_display_roundtrip(m_wlDisplay);

    if (m_layerShell) {
        createLayerSurface();
    } else {
        qWarning() << "LayerShell: Compositor does not support zwlr_layer_shell_v1!";
    }
}

void LayerShell::registryHandleGlobal(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    (void)version; // Unused parameter
    LayerShell *self = static_cast<LayerShell*>(data);
    if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        self->m_layerShell = (struct zwlr_layer_shell_v1 *)wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    }
}

void LayerShell::registryHandleGlobalRemove(void *data, struct wl_registry *registry, uint32_t name)
{
    (void)data;     // Unused parameter
    (void)registry; // Unused parameter
    (void)name;     // Unused parameter
    // Handle global removal if necessary
}

void LayerShell::createLayerSurface()
{
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    if (!native) {
        qWarning() << "LayerShell: Failed to get QPlatformNativeInterface";
        return;
    }

    // Get the underlying wl_surface from the QWindow
    // Note: The window must be visible/created for this to exist!
    m_wlSurface = static_cast<struct wl_surface *>(
        native->nativeResourceForWindow("surface", m_window));

    if (!m_wlSurface) {
        qWarning() << "LayerShell: Could not get wl_surface for QWindow. Make sure window is visible first.";
        return;
    }

    // Create the layer surface
    // Layer: OVERLAY (4) or BACKGROUND (0). We use OVERLAY to sit on top of everything.
    // Scope: "login"
    m_layerSurface = zwlr_layer_shell_v1_get_layer_surface(m_layerShell, m_wlSurface, nullptr, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "login");
    
    // Configure behavior
    zwlr_layer_surface_v1_set_size(m_layerSurface, 0, 0); // 0,0 means match anchor size
    
    // Anchor to all 4 edges (Top|Bottom|Left|Right = 1|2|4|8 = 15)
    zwlr_layer_surface_v1_set_anchor(m_layerSurface, 15);
    
    // Exclusive zone: -1 means "I don't care, just cover". 
    // Positive means "reserve this space". For a greeter, -1 (auto) or 0 usually works fine if anchoring fully.
    zwlr_layer_surface_v1_set_exclusive_zone(m_layerSurface, -1);
    
    // Interaction: Keyboard Exclusive (1). We need this to type the password!
    zwlr_layer_surface_v1_set_keyboard_interactivity(m_layerSurface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);

    zwlr_layer_surface_v1_add_listener(m_layerSurface, &layer_surface_listener, this);
    
    // Initial commit to apply changes
    wl_surface_commit(m_wlSurface);
    wl_display_roundtrip(m_wlDisplay);
}

void LayerShell::layerSurfaceHandleConfigure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t width, uint32_t height)
{
    LayerShell *self = static_cast<LayerShell*>(data);
    
    // We must ACK the configure event or the window won't show
    zwlr_layer_surface_v1_ack_configure(surface, serial);
    
    // Resize the QWindow to match if the compositor enforces a size
    // For fullscreen (0x0), we usually don't need to force resize unless we are doing something specific.
    if (width > 0 && height > 0) {
        self->m_window->resize(width, height);
    }
    
    if (!self->m_configured) {
        self->m_configured = true;
        // Sometimes a second commit is needed to force the render
        // wl_surface_commit(self->m_wlSurface); 
    }
}

void LayerShell::layerSurfaceHandleClosed(void *data, struct zwlr_layer_surface_v1 *surface)
{
    (void)data;    // Unused parameter
    (void)surface; // Unused parameter
    // Compositor closed us
    QCoreApplication::quit();
}
