/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  TRX Brass LVGL GUI
 *
 *  Copyright (c) 2022-2026 Belousov Oleg aka R1CBU
 */

#include <gio/gio.h>
#include "lvgl/lvgl.h"
#include "bluez.h"
#include "src/dialog_bluetooth.h"
#include "src/msg.h"

static GDBusConnection  *connection = NULL;
static bool             ok = false;
static bool             scan = false;

static const gchar agent_introspection_xml[] =
    "<node>"
    "  <interface name='org.bluez.Agent1'>"
    "    <method name='Release'/>"
    "    <method name='RequestPinCode'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='pincode' direction='out'/>"
    "    </method>"
    "    <method name='DisplayPinCode'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='pincode' direction='in'/>"
    "    </method>"
    "    <method name='RequestPasskey'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='out'/>"
    "    </method>"
    "    <method name='DisplayPasskey'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='in'/>"
    "      <arg type='q' name='entered' direction='in'/>"
    "    </method>"
    "    <method name='RequestConfirmation'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='u' name='passkey' direction='in'/>"
    "    </method>"
    "    <method name='RequestAuthorization'>"
    "      <arg type='o' name='device' direction='in'/>"
    "    </method>"
    "    <method name='AuthorizeService'>"
    "      <arg type='o' name='device' direction='in'/>"
    "      <arg type='s' name='uuid' direction='in'/>"
    "    </method>"
    "    <method name='Cancel'/>"
    "  </interface>"
    "</node>";

static void handle_agent_method_call(
    GDBusConnection *connection,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer user_data
) {
    LV_LOG_INFO("[ AGENT ] %s", method_name);

    if (g_strcmp0(method_name, "RequestPinCode") == 0) {
        const gchar *device_path;
        g_variant_get(parameters, "(&o)", &device_path);

        /* ВАЖНО: тут нужен ПРАВИЛЬНЫЙ формат (s), а не NULL.
           "0000" подходит для многих legacy-устройств, но если
           у твоей клавиатуры на корпусе написан свой PIN —
           нужно either спросить пользователя через UI, either
           использовать код, который она ожидает. */
        LV_LOG_INFO("RequestPinCode for %s -> 0000", device_path);
        g_dbus_method_invocation_return_value(invocation,
            g_variant_new("(s)", "0000"));

    } else if (g_strcmp0(method_name, "DisplayPinCode") == 0) {
        const gchar *device_path, *pincode;
        g_variant_get(parameters, "(&o&s)", &device_path, &pincode);

        LV_LOG_INFO("PinCode for %s: %s", device_path, pincode);
        msg_set_text_long_fmt("PIN: %s", pincode);

        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (g_strcmp0(method_name, "RequestPasskey") == 0) {
        /* Редкий кейс (устройство с дисплеем, без ввода).
           Без реального UI для ввода passkey — честно отклоняем,
           чтобы BlueZ не завис в ожидании неверного ответа. */
        const gchar *device_path;
        g_variant_get(parameters, "(&o)", &device_path);
        LV_LOG_WARN("RequestPasskey for %s - rejecting (not implemented)", device_path);
        g_dbus_method_invocation_return_error(invocation,
            G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Not supported");

    } else if (g_strcmp0(method_name, "DisplayPasskey") == 0) {
        const gchar *device_path;
        guint32 passkey;
        guint16 entered;
        g_variant_get(parameters, "(&ouq)", &device_path, &passkey, &entered);

        LV_LOG_INFO("Passkey for %s: %06u (entered: %d)", device_path, passkey, entered);
        msg_set_text_long_fmt("Type PIN %06u and press Enter", passkey);
        g_dbus_method_invocation_return_value(invocation, NULL);

    } else if (g_strcmp0(method_name, "RequestConfirmation") == 0 ||
               g_strcmp0(method_name, "AuthorizeService") == 0 ||
               g_strcmp0(method_name, "RequestAuthorization") == 0) {
        LV_LOG_INFO("Request: Ok (%s)", method_name);
        g_dbus_method_invocation_return_value(invocation, NULL);

    } else {
        /* Release, Cancel */
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

static const GDBusInterfaceVTable agent_vtable = {
    handle_agent_method_call, NULL, NULL
};

bool dbus_bluez_register_agent() {
    GError *error = NULL;
    GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml(agent_introspection_xml, NULL);

    guint reg_id = g_dbus_connection_register_object(
        connection,
        "/org/bluez/agent/custom",
        introspection_data->interfaces[0],
        &agent_vtable,
        NULL, NULL, &error);

    if (reg_id == 0) {
        LV_LOG_ERROR("D-Bus error: %s", error->message);
        g_error_free(error);
        return false;
    }

    GVariant *result = g_dbus_connection_call_sync(
        connection, "org.bluez", "/org/bluez", "org.bluez.AgentManager1",
        "RegisterAgent",
        g_variant_new("(os)", "/org/bluez/agent/custom", "KeyboardDisplay"),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (!result) {
        LV_LOG_ERROR("D-Bus error: %s", error->message);
        g_error_free(error);
        return false;
    }
    g_variant_unref(result);

    result = g_dbus_connection_call_sync(
        connection, "org.bluez", "/org/bluez", "org.bluez.AgentManager1",
        "RequestDefaultAgent",
        g_variant_new("(o)", "/org/bluez/agent/custom"),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (!result) {
        LV_LOG_ERROR("D-Bus error: %s", error->message);
        g_error_free(error);
        return false;
    }
    g_variant_unref(result);

    LV_LOG_INFO("D-Bus agent installed");

    return true;
}

void dbus_bluez_init() {
    GVariant    *result;
    GError      *error = NULL;

    result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        "/org/bluez/hci0",
        "org.freedesktop.DBus.Properties",
        "Set",
        g_variant_new("(ssv)", "org.bluez.Adapter1", "Powered", g_variant_new_boolean(TRUE)),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL, &error
    );

    if (!result) {
        ok = false;
        LV_LOG_ERROR("D-Bus Bluez error: %s", error->message);
        g_error_free(error);
    }
}

void dbus_bluez_check(GDBusConnection *conn) {
    GError          *error = NULL;
    GVariant        *result;

    connection = conn;
    ok = false;

    result = g_dbus_connection_call_sync(
        connection, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error
    );

    if (!result) {
        LV_LOG_ERROR("D-Bus error: %s", error->message);
        g_error_free(error);
        return;
    }

    GVariant *dict_objects = g_variant_get_child_value(result, 0);
    gsize num_objects = g_variant_n_children(dict_objects);

    for (gsize i = 0; i < num_objects; i++) {
        GVariant *object_entry = g_variant_get_child_value(dict_objects, i);

        GVariant *path_val = g_variant_get_child_value(object_entry, 0);
        const gchar *object_path = g_variant_get_string(path_val, NULL);

        GVariant *dict_interfaces = g_variant_get_child_value(object_entry, 1);
        gsize num_interfaces = g_variant_n_children(dict_interfaces);

        for (gsize j = 0; j < num_interfaces; j++) {
            GVariant *iface_entry = g_variant_get_child_value(dict_interfaces, j);

            GVariant *iface_name_val = g_variant_get_child_value(iface_entry, 0);
            const gchar *interface_name = g_variant_get_string(iface_name_val, NULL);

            if (g_strcmp0(interface_name, "org.bluez.Adapter1") == 0) {
                LV_LOG_INFO("Bluez adapter: %s", object_path);
                ok = true;
            }

            g_variant_unref(iface_name_val);
            g_variant_unref(iface_entry);

            if (ok) {
                break;
            }
        }

        g_variant_unref(dict_interfaces);
        g_variant_unref(path_val);
        g_variant_unref(object_entry);

        if (ok) {
            break;
        }
    }

    g_variant_unref(dict_objects);
    g_variant_unref(result);

    if (ok) {
        dbus_bluez_init();
    } else {
        LV_LOG_WARN("Bluez adapter not found");
    }
}

bool dbus_bluez_ok() {
    return ok;
}

bool dbus_bluez_scan(bool on) {
    GError *error = NULL;

    if (connection == NULL || !ok) {
        LV_LOG_ERROR("Bluez scan: adapter is not ready");
        return false;
    }

    LV_LOG_INFO("Bluez scan %s", on ? "start" : "stop");

    GVariant *result = g_dbus_connection_call_sync(
        connection, "org.bluez", "/org/bluez/hci0", "org.bluez.Adapter1",
        on ? "StartDiscovery" : "StopDiscovery", NULL, NULL,
        G_DBUS_CALL_FLAGS_NONE, 5000, NULL, &error
    );

    if (result) {
        g_variant_unref(result);
        scan = on;
        LV_LOG_INFO("Bluez scan %s complete", on ? "start" : "stop");
        return true;
    } else {
        LV_LOG_ERROR("Bluez scan %s failed: %s", on ? "start" : "stop", error->message);
        g_error_free(error);
        return false;
    }
}

bool dbus_bluez_pair(const char *path) {
    GError      *error = NULL;
    GVariant    *result;

    result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        path,
        "org.bluez.Device1",
        "Pair",
        NULL,
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        30000,
        NULL, &error
    );

    if (result) {
        LV_LOG_INFO("Paired ok %s", path);
        return true;
    } else {
        LV_LOG_WARN("Not paired %s - %s", path, error->message);
        g_error_free(error);
        return false;
    }
}

bool dbus_bluez_trust(const char *path) {
    GError      *error = NULL;
    GVariant    *result;

    result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        path,
        "org.freedesktop.DBus.Properties",
        "Set",
        g_variant_new("(ssv)", "org.bluez.Device1", "Trusted", g_variant_new_boolean(TRUE)),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1, NULL, &error
    );

    if (result) {
        g_variant_unref(result);
        return true;
    } else {
        g_error_free(error);
        return false;
    }
}

static void connect_done_cb(GObject *source, GAsyncResult *async_result, gpointer user_data) {
    char *path = user_data;
    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), async_result, &error);

    if (result != NULL) {
        msg_set_text_fmt("Connected Ok");
        g_variant_unref(result);
    } else {
        LV_LOG_WARN("Bluetooth connect failed for %s: %s", path, error->message);
        msg_set_text_fmt("Not connected: %s", error->message);
        g_error_free(error);
    }

    g_free(path);
}

static void connect_device_async(char *path) {
    if (!dbus_bluez_trust(path)) {
        LV_LOG_WARN("Bluetooth trust failed for %s", path);
        msg_set_text_fmt("Bluetooth trust failed");
    }

    msg_set_text_fmt("Connecting");
    g_dbus_connection_call(
        connection, "org.bluez", path, "org.bluez.Device1", "Connect",
        NULL, NULL, G_DBUS_CALL_FLAGS_NONE, 10000, NULL,
        connect_done_cb, path);
}

static void pair_done_cb(GObject *source, GAsyncResult *async_result, gpointer user_data) {
    char *path = user_data;
    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source), async_result, &error);

    if (result == NULL) {
        LV_LOG_WARN("Bluetooth pairing failed for %s: %s", path, error->message);
        msg_set_text_fmt("Pairing failed: %s", error->message);
        g_error_free(error);
        g_free(path);
        return;
    }

    g_variant_unref(result);
    LV_LOG_INFO("Bluetooth paired: %s", path);
    connect_device_async(path);
}

void dbus_bluez_connect(const char *path) {
    if (connection == NULL || !ok || path == NULL) {
        msg_set_text_fmt("Bluetooth adapter is not ready");
        return;
    }

    if (scan) dbus_bluez_scan(false);

    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_sync(
        connection, "org.bluez", path, "org.freedesktop.DBus.Properties", "Get",
        g_variant_new("(ss)", "org.bluez.Device1", "Paired"),
        G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 5000, NULL, &error);

    bool paired = false;
    if (result != NULL) {
        GVariant *value = NULL;
        g_variant_get(result, "(v)", &value);
        paired = g_variant_get_boolean(value);
        g_variant_unref(value);
        g_variant_unref(result);
    } else {
        LV_LOG_WARN("Unable to read Bluetooth pairing state for %s: %s", path, error->message);
        g_error_free(error);
    }

    char *path_copy = g_strdup(path);
    if (path_copy == NULL) {
        msg_set_text_fmt("Bluetooth: out of memory");
        return;
    }

    if (paired) {
        connect_device_async(path_copy);
        return;
    }

    msg_set_text_fmt("Pairing...");
    g_dbus_connection_call(
        connection, "org.bluez", path, "org.bluez.Device1", "Pair",
        NULL, NULL, G_DBUS_CALL_FLAGS_NONE, 60000, NULL,
        pair_done_cb, path_copy);
}

bool dbus_bluez_remove(const char *path) {
    GError *error = NULL;

    if (connection == NULL || !ok || path == NULL) return false;

    GVariant *result = g_dbus_connection_call_sync(
        connection,
        "org.bluez",
        "/org/bluez/hci0",
        "org.bluez.Adapter1",
        "RemoveDevice",
        g_variant_new("(o)", path),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        5000,
        NULL,
        &error
    );

    if (result != NULL) {
        g_variant_unref(result);
        LV_LOG_INFO("Bluez device removed: %s", path);
        return true;
    }

    LV_LOG_ERROR("Bluez remove failed for %s: %s", path, error->message);
    g_error_free(error);
    return false;
}

void dbus_bluez_interface_added(
    GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data)
{
    GVariant        *path_val = g_variant_get_child_value(parameters, 0);
    const gchar     *child_path = g_variant_get_string(path_val, NULL);

    if (g_str_has_prefix(child_path, "/org/bluez/hci0/dev_")) {
        GVariant    *dict_interfaces = g_variant_get_child_value(parameters, 1);
        gsize       num_interfaces = g_variant_n_children(dict_interfaces);

        const gchar *address = NULL;
        const gchar *name = "<Noname>";
        GVariant    *address_val = NULL;
        GVariant    *name_val = NULL;

        for (gsize i = 0; i < num_interfaces; i++) {
            GVariant *iface_entry = g_variant_get_child_value(dict_interfaces, i);
            GVariant *iface_name_val = g_variant_get_child_value(iface_entry, 0);
            const gchar *iface_name = g_variant_get_string(iface_name_val, NULL);

            if (g_strcmp0(iface_name, "org.bluez.Device1") == 0) {
                GVariant *dict_properties = g_variant_get_child_value(iface_entry, 1);
                gsize num_properties = g_variant_n_children(dict_properties);

                for (gsize j = 0; j < num_properties; j++) {
                    GVariant *prop_entry = g_variant_get_child_value(dict_properties, j);
                    GVariant *prop_key_val = g_variant_get_child_value(prop_entry, 0);
                    const gchar *prop_key = g_variant_get_string(prop_key_val, NULL);

                    GVariant *prop_variant_wrapper = g_variant_get_child_value(prop_entry, 1);
                    GVariant *prop_value = g_variant_get_variant(prop_variant_wrapper);

                    if (g_strcmp0(prop_key, "Address") == 0) {
                        address_val = prop_value;
                        address = g_variant_get_string(address_val, NULL);
                    } else if (g_strcmp0(prop_key, "Name") == 0) {
                        name_val = prop_value;
                        name = g_variant_get_string(name_val, NULL);
                    } else {
                        g_variant_unref(prop_value);
                    }

                    g_variant_unref(prop_variant_wrapper);
                    g_variant_unref(prop_key_val);
                    g_variant_unref(prop_entry);
                }
                g_variant_unref(dict_properties);
            }
            g_variant_unref(iface_name_val);
            g_variant_unref(iface_entry);
        }

        if (address != NULL) {
            dialog_bluetooth_interface_added(address, name, child_path);
        }

        if (address_val) g_variant_unref(address_val);
        if (name_val) g_variant_unref(name_val);

        g_variant_unref(dict_interfaces);
    }

    g_variant_unref(path_val);
}

void dbus_bluez_cached() {
    GError *error = NULL;
    GVariant *result;

    result = g_dbus_connection_call_sync(
        connection, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (!result) {
        g_error_free(error);
        return;
    }

    GVariant *dict_objects = g_variant_get_child_value(result, 0);
    gsize num_objects = g_variant_n_children(dict_objects);

    for (gsize i = 0; i < num_objects; i++) {
        GVariant *object_entry = g_variant_get_child_value(dict_objects, i);
        GVariant *path_val = g_variant_get_child_value(object_entry, 0);
        const gchar *object_path = g_variant_get_string(path_val, NULL);

        if (g_str_has_prefix(object_path, "/org/bluez/hci0/dev_")) {
            GVariant *dict_interfaces = g_variant_get_child_value(object_entry, 1);
            gsize num_interfaces = g_variant_n_children(dict_interfaces);

            const gchar *address = NULL;
            const gchar *name = "<NoName>";
            GVariant *address_val = NULL;
            GVariant *name_val = NULL;

            for (gsize j = 0; j < num_interfaces; j++) {
                GVariant *iface_entry = g_variant_get_child_value(dict_interfaces, j);
                GVariant *iface_name_val = g_variant_get_child_value(iface_entry, 0);
                const gchar *iface_name = g_variant_get_string(iface_name_val, NULL);

                if (g_strcmp0(iface_name, "org.bluez.Device1") == 0) {
                    GVariant *dict_properties = g_variant_get_child_value(iface_entry, 1);
                    gsize num_properties = g_variant_n_children(dict_properties);

                    for (gsize k = 0; k < num_properties; k++) {
                        GVariant *prop_entry = g_variant_get_child_value(dict_properties, k);
                        GVariant *prop_key_val = g_variant_get_child_value(prop_entry, 0);
                        const gchar *prop_key = g_variant_get_string(prop_key_val, NULL);
                        GVariant *prop_variant_wrapper = g_variant_get_child_value(prop_entry, 1);
                        GVariant *prop_value = g_variant_get_variant(prop_variant_wrapper);

                        if (g_strcmp0(prop_key, "Address") == 0) {
                            address_val = prop_value;
                            address = g_variant_get_string(address_val, NULL);
                        } else if (g_strcmp0(prop_key, "Name") == 0) {
                            name_val = prop_value;
                            name = g_variant_get_string(name_val, NULL);
                        } else {
                            g_variant_unref(prop_value);
                        }
                        g_variant_unref(prop_variant_wrapper);
                        g_variant_unref(prop_key_val);
                        g_variant_unref(prop_entry);
                    }
                    g_variant_unref(dict_properties);
                }
                g_variant_unref(iface_name_val);
                g_variant_unref(iface_entry);
            }

            if (address != NULL) {
                dialog_bluetooth_interface_added(address, name, object_path);
            }

            if (address_val) g_variant_unref(address_val);
            if (name_val) g_variant_unref(name_val);
            g_variant_unref(dict_interfaces);
        }
        g_variant_unref(path_val);
        g_variant_unref(object_entry);
    }

    g_variant_unref(dict_objects);
    g_variant_unref(result);
}
