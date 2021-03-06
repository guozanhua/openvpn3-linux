//  OpenVPN 3 Linux client -- Next generation OpenVPN client
//
//  Copyright (C) 2017 - 2018  OpenVPN Inc. <sales@openvpn.net>
//  Copyright (C) 2017 - 2018  David Sommerseth <davids@openvpn.net>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, version 3 of the
//  License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef OPENVPN3_DBUS_CONFIGMGR_HPP
#define OPENVPN3_DBUS_CONFIGMGR_HPP

#include <functional>
#include <map>
#include <ctime>

#include "common/core-extensions.hpp"
#include "dbus/core.hpp"
#include "dbus/connection-creds.hpp"
#include "dbus/exceptions.hpp"
#include "log/dbus-log.hpp"

using namespace openvpn;


/**
 * Helper class to tackle signals sent by the configuration manager
 *
 * This mostly just wraps the LogSender class and predefines LogGroup to always
 * be CONFIGMGR.
 */

class ConfigManagerSignals : public LogSender
{
public:
    /**
     *  Declares the ConfigManagerSignals object
     *
     * @param conn        D-Bus connection to use when sending signals
     * @param object_path D-Bus object to use as sender of signals
     */
    ConfigManagerSignals(GDBusConnection *conn, std::string object_path)
        : LogSender(conn, LogGroup::CONFIGMGR,
                    OpenVPN3DBus_interf_configuration, object_path)
    {
    }


    virtual ~ConfigManagerSignals()
    {
    }


    /**
     *  Whenever a FATAL error happens, the process is expected to stop.
     *  (The exit step is not yet implemented)
     *
     * @param msg  Message to sent to the log subscribers
     */
    void LogFATAL(std::string msg)
    {
        Log(log_group, LogCategory::FATAL, msg);
        // FIXME: throw something here, to start shutdown procedures
    }


    /**
     *  Sends a StatusChange signal with a text message
     *
     * @param major  StatusMajor code of the status change
     * @param minor  StatusMinro code of the status change
     * @param msg    String containing a description of the reason for this
     *               status change
     */
    void StatusChange(const StatusMajor major, const StatusMinor minor, std::string msg)
    {
        GVariant *params = g_variant_new("(uus)", (guint) major, (guint) minor, msg.c_str());
        Send("StatusChange", params);
    }


    /**
     *  A simpler StatusChange signal sender, without a text message
     *
     * @param major  StatusMajor code of the status change
     * @param minor  StatusMinro code of the status change
     */
    void StatusChange(const StatusMajor major, const StatusMinor minor)
    {
        GVariant *params = g_variant_new("(uus)", (guint) major, (guint) minor, "");
        Send("StatusChange", params);
    }
};


/**
 *  ConfigurationAlias objects are present under the
 *  /net/openvpn/v3/configuration/alias/$ALIAS_NAME D-Bus path in the
 *  configuration manager service.  They are simpler aliases which can be
 *  used instead of the full D-Bus object path to a single VPN configuration
 *  profile
 */
class ConfigurationAlias : public DBusObject,
                           public ConfigManagerSignals
{
public:
    /**
     * Initializes a Configuration Alias
     *
     * @param dbuscon    D-Bus connection to use for this object
     * @param aliasname  A string containing the alias name
     * @param cfgpath    An object path pointing at an existing D-Bus path
     */
    ConfigurationAlias(GDBusConnection *dbuscon, std::string aliasname, std::string cfgpath)
        : DBusObject(OpenVPN3DBus_rootp_configuration + "/aliases/" + aliasname),
          ConfigManagerSignals(dbuscon, OpenVPN3DBus_rootp_configuration + "/aliases/" + aliasname),
          cfgpath(cfgpath)
    {
        alias = aliasname;
        std::string new_obj_path = OpenVPN3DBus_rootp_configuration + "/aliases/"
                                   + aliasname;

        if (!g_variant_is_object_path(new_obj_path.c_str()))
        {
            THROW_DBUSEXCEPTION("ConfigurationAlias",
                                "Specified alias is invalid");
        }

        std::string introsp_xml ="<node name='" + new_obj_path + "'>"
            "    <interface name='" + OpenVPN3DBus_interf_configuration + "'>"
            "        <property  type='o' name='config_path' access='read'/>"
            "    </interface>"
            "</node>";
        ParseIntrospectionXML(introsp_xml);
    }


    /**
     *  Returns a C string (char *) of the configured alias name

     * @return Returns a const char * pointing at the alias name
     */
    const char * GetAlias()
    {
        return alias.c_str();
    }


    /**
     *  Callback method which is called each time a D-Bus method call occurs
     *  on this ConfigurationAlias object.
     *
     *  This object does not have any methods, so it is not expected that
     *  this callback will be called.  If it is, throw an exception.
     *
     * @param conn       D-Bus connection where the method call occurred
     * @param sender     D-Bus bus name of the sender of the method call
     * @param obj_path   D-Bus object path of the target object.
     * @param intf_name  D-Bus interface of the method call
     * @param method_name D-Bus method name to be executed
     * @param params     GVariant Glib2 object containing the arguments for
     *                   the method call
     * @param invoc      GDBusMethodInvocation where the response/result of
     *                   the method call will be returned.
     */
    void callback_method_call(GDBusConnection *conn,
                              const std::string sender,
                              const std::string obj_path,
                              const std::string intf_name,
                              const std::string method_name,
                              GVariant *params,
                              GDBusMethodInvocation *invoc)
    {
        THROW_DBUSEXCEPTION("ConfigManagerAlias", "method_call not implemented");
    }


    /**
     *   Callback which is called each time a ConfigurationAlias D-Bus
     *   property is being read.
     *
     * @param conn           D-Bus connection this event occurred on
     * @param sender         D-Bus bus name of the requester
     * @param obj_path       D-Bus object path to the object being requested
     * @param intf_name      D-Bus interface of the property being accessed
     * @param property_name  The property name being accessed
     * @param error          A GLib2 GError object if an error occurs
     *
     * @return  Returns a GVariant Glib2 object containing the value of the
     *          requested D-Bus object property.  On errors, NULL must be
     *          returned and the error must be returned via a GError
     *          object.
     */
    GVariant * callback_get_property(GDBusConnection *conn,
                                     const std::string sender,
                                     const std::string obj_path,
                                     const std::string intf_name,
                                     const std::string property_name,
                                     GError **error)
    {
        GVariant *ret = NULL;

        if ("config_path" == property_name)
        {
            ret = g_variant_new_string (cfgpath.c_str());
        }
        else
        {
            ret = NULL;
            g_set_error (error,
                         G_IO_ERROR,
                         G_IO_ERROR_FAILED,
                         "Unknown property");
        }
        return ret;
    };

    /**
     *  Callback method which is used each time a ConfigurationAlias
     *  property is being modified over the D-Bus.
     *
     *  This will always fail with an exception, as there exists no properties
     *  which can be modified in a ConfigurationAlias object.
     *
     * @param conn           D-Bus connection this event occurred on
     * @param sender         D-Bus bus name of the requester
     * @param obj_path       D-Bus object path to the object being requested
     * @param intf_name      D-Bus interface of the property being accessed
     * @param property_name  The property name being accessed
     * @param value          GVariant object containing the value to be stored
     * @param error          A GLib2 GError object if an error occurs
     *
     * @return Will always throw an exception as there are no properties to
     *         modify.
     */
    GVariantBuilder * callback_set_property(GDBusConnection *conn,
                                            const std::string sender,
                                            const std::string obj_path,
                                            const std::string intf_name,
                                            const std::string property_name,
                                            GVariant *value,
                                            GError **error)
    {
        THROW_DBUSEXCEPTION("ConfigManagerAlias", "set property not implemented");
    }


private:
    std::string alias;
    std::string cfgpath;
};


/**
 *  A ConfigurationObject contains information about a specific VPN
 *  configuration profile.  This object is then exposed on the D-Bus through
 *  its own unique object path.
 *
 *  The configuration manager is responsible for maintaining the
 *  life cycle of these configuration objects.
 */
class ConfigurationObject : public DBusObject,
                            public ConfigManagerSignals,
                            public DBusCredentials
{
public:
    /**
     *  Constructor creating a new ConfigurationObject
     *
     * @param dbuscon  D-Bus connection this object is tied to
     * @param remove_callback  Callback function which must be called when
     *                 destroying this configuration object.
     * @param objpath  D-Bus object path of this object
     * @param creator  An uid reference of the owner of this object.  This is
     *                 typically the uid of the front-end user importing this
     *                 VPN configuration profile.
     * @param params   Pointer to a GLib2 GVariant object containing both
     *                 meta data as well as the configuration profile itself
     *                 to use when initializing this object
     */
    ConfigurationObject(GDBusConnection *dbuscon,
                        std::function<void()> remove_callback,
                        std::string objpath,
                        uid_t creator, GVariant *params)
        : DBusObject(objpath),
          ConfigManagerSignals(dbuscon, objpath),
          DBusCredentials(dbuscon, creator),
          remove_callback(remove_callback),
          name(""),
          import_tstamp(std::time(nullptr)),
          last_use_tstamp(0),
          used_count(0),
          valid(false),
          readonly(false),
          single_use(false),
          persistent(false),
          persist_tun(false),
          alias(nullptr)
    {
        gchar *cfgstr;
        gchar *cfgname_c;
        g_variant_get (params, "(ssbb)",
                       &cfgname_c, &cfgstr,
                       &single_use, &persistent);
        name = std::string(cfgname_c);

        // Parse the options from the imported configuration
        OptionList::Limits limits("profile is too large",
				  ProfileParseLimits::MAX_PROFILE_SIZE,
				  ProfileParseLimits::OPT_OVERHEAD,
				  ProfileParseLimits::TERM_OVERHEAD,
				  ProfileParseLimits::MAX_LINE_SIZE,
				  ProfileParseLimits::MAX_DIRECTIVE_SIZE);
        options.parse_from_config(cfgstr, &limits);

        // FIXME:  Validate the configuration file, ensure --ca/--key/--cert/--dh/--pkcs12
        //         contains files
        valid = true;

        std::string introsp_xml ="<node name='" + objpath + "'>"
            "    <interface name='net.openvpn.v3.configuration'>"
            "        <method name='Fetch'>"
            "            <arg direction='out' type='s' name='config'/>"
            "        </method>"
            "        <method name='FetchJSON'>"
            "            <arg direction='out' type='s' name='config_json'/>"
            "        </method>"
            "        <method name='SetOption'>"
            "            <arg direction='in' type='s' name='option'/>"
            "            <arg direction='in' type='s' name='value'/>"
            "        </method>"
            "        <method name='AccessGrant'>"
            "            <arg direction='in' type='u' name='uid'/>"
            "        </method>"
            "        <method name='AccessRevoke'>"
            "            <arg direction='in' type='u' name='uid'/>"
            "        </method>"
            "        <method name='Seal'/>"
            "        <method name='Remove'/>"
            "        <property type='u' name='owner' access='read'/>"
            "        <property type='au' name='acl' access='read'/>"
            "        <property type='s' name='name' access='readwrite'/>"
            "        <property type='t' name='import_timestamp' access='read' />"
            "        <property type='t' name='last_used_timestamp' access='read' />"
            "        <property type='u' name='used_count' access='read' />"
            "        <property type='b' name='valid' access='read'/>"
            "        <property type='b' name='readonly' access='read'/>"
            "        <property type='b' name='single_use' access='read'/>"
            "        <property type='b' name='persistent' access='read'/>"
            "        <property type='b' name='locked_down' access='readwrite'/>"
            "        <property type='b' name='public_access' access='readwrite'/>"
            "        <property type='b' name='persist_tun' access='readwrite' />"
            "        <property type='s' name='alias' access='readwrite'/>"
            "    </interface>"
            "</node>";
        ParseIntrospectionXML(introsp_xml);

        g_free(cfgname_c);
        g_free(cfgstr);
    }


    ~ConfigurationObject()
    {
        remove_callback();
        LogVerb2("Configuration removed");
        IdleCheck_RefDec();
    };


    /**
     *  Callback method which is called each time a D-Bus method call occurs
     *  on this ConfigurationObject.
     *
     * @param conn        D-Bus connection where the method call occurred
     * @param sender      D-Bus bus name of the sender of the method call
     * @param obj_path    D-Bus object path of the target object.
     * @param intf_name   D-Bus interface of the method call
     * @param method_name D-Bus method name to be executed
     * @param params      GVariant Glib2 object containing the arguments for
     *                    the method call
     * @param invoc       GDBusMethodInvocation where the response/result of
     *                    the method call will be returned.
     */
    void callback_method_call(GDBusConnection *conn,
                              const std::string sender,
                              const std::string obj_path,
                              const std::string intf_name,
                              const std::string method_name,
                              GVariant *params,
                              GDBusMethodInvocation *invoc)
    {
        IdleCheck_UpdateTimestamp();
        if ("Fetch" == method_name)
        {
            try
            {
                if (!locked_down)
                {
                    CheckACL(sender, true);
                }
                else
                {
                    // If the configuration is locked down, restrict any
                    // read-operations to anyone except the backend VPN client
                    // process (root user) or the configuration profile owner
                    CheckOwnerAccess(sender, true);
                }
                g_dbus_method_invocation_return_value(invoc,
                                                      g_variant_new("(s)",
                                                                    options.string_export().c_str()));

                // If the fetching user is root, we consider this
                // configuration to be "used"
                if (GetUID(sender) == 0)
                {
                    // If this config is tagged as single-use only then we delete this
                    // config from memory.
                    if (single_use)
                    {
                        LogVerb2("Single-use configuration fetched");
                        RemoveObject(conn);
                        delete this;
                        return;
                    }
                    used_count++;
                    last_use_tstamp = std::time(nullptr);
                }
                return;
            }
            catch (DBusCredentialsException& excp)
            {
                LogWarn(excp.err());
                excp.SetDBusError(invoc);
            }
        }
        else if ("FetchJSON" == method_name)
        {
            try
            {
                if (!locked_down)
                {
                    CheckACL(sender);
                }
                else
                {
                    // If the configuration is locked down, restrict any
                    // read-operations to the configuration profile owner
                    CheckOwnerAccess(sender);
                }
                g_dbus_method_invocation_return_value(invoc,
                                                      g_variant_new("(s)",
                                                                    options.json_export().c_str()));

                // Do not remove single-use object with this method.
                // FetchJSON is only used by front-ends, never backends.  So
                // it still needs to be available when the backend calls Fetch.
                //
                // single-use configurations are an automation convenience,
                // not a security feature.  Security is handled via ACLs.
                return;
            }
            catch (DBusCredentialsException& excp)
            {
                LogWarn(excp.err());
                excp.SetDBusError(invoc);
            }
        }
        else if ("SetOption" == method_name)
        {
            if (readonly)
            {
                g_dbus_method_invocation_return_dbus_error (invoc,
                                                            "net.openvpn.v3.error.ReadOnly",
                                                            "Configuration is sealed and readonly");
                return;
            }
            try
            {
                CheckOwnerAccess(sender);
                // TODO: Implement SetOption
                g_dbus_method_invocation_return_value(invoc, NULL);
                return;
            }
            catch (DBusCredentialsException& excp)
            {
                LogWarn(excp.err());
                excp.SetDBusError(invoc);
            }
        }
        else if ("AccessGrant" == method_name)
        {
            if (readonly)
            {
                g_dbus_method_invocation_return_dbus_error (invoc,
                                                            "net.openvpn.v3.error.ReadOnly",
                                                            "Configuration is sealed and readonly");
                return;
            }

            try
            {
                CheckOwnerAccess(sender);

                uid_t uid = -1;
                g_variant_get(params, "(u)", &uid);
                GrantAccess(uid);
                g_dbus_method_invocation_return_value(invoc, NULL);

                LogVerb1("Access granted to UID " + std::to_string(uid)
                         + " by UID " + std::to_string(GetUID(sender)));
                return;
            }
            catch (DBusCredentialsException& excp)
            {
                LogWarn(excp.err());
                excp.SetDBusError(invoc);
            }
        }
        else if ("AccessRevoke" == method_name)
        {
            if (readonly)
            {
                g_dbus_method_invocation_return_dbus_error (invoc,
                                                            "net.openvpn.v3.error.ReadOnly",
                                                            "Configuration is sealed and readonly");
                return;
            }

            try
            {
                CheckOwnerAccess(sender);

                uid_t uid = -1;
                g_variant_get(params, "(u)", &uid);
                RevokeAccess(uid);
                g_dbus_method_invocation_return_value(invoc, NULL);

                LogVerb1("Access revoked for UID " + std::to_string(uid)
                         + " by UID " + std::to_string(GetUID(sender)));
                return;
            }
            catch (DBusCredentialsException& excp)
            {
                LogWarn(excp.err());
                excp.SetDBusError(invoc);
            }
        }
        else if ("Seal" == method_name)
        {
            try
            {
                CheckOwnerAccess(sender);

                if (valid) {
                    readonly = true;
                    g_dbus_method_invocation_return_value(invoc, NULL);
                }
                else
                {
                    g_dbus_method_invocation_return_dbus_error (invoc,
                                                                "net.openvpn.v3.error.InvalidData",
                                                                "Configuration is not currently valid");
                }
                return;
            }
            catch (DBusCredentialsException& excp)
            {
                LogWarn(excp.err());
                excp.SetDBusError(invoc);
            }
        }
        else if ("Remove" == method_name)
        {
            try
            {
                CheckOwnerAccess(sender);
                RemoveObject(conn);
                g_dbus_method_invocation_return_value(invoc, NULL);
                delete this;
                return;
            }
            catch (DBusCredentialsException& excp)
            {
                LogWarn(excp.err());
                excp.SetDBusError(invoc);
            }
        }
    };


    /**
     *   Callback which is used each time a ConfigurationObject D-Bus
     *   property is being read.
     *
     *   Only the 'owner' is accessible by anyone, otherwise it must either
     *   be the configuration owner or UIDs granted access to this profile.
     *
     * @param conn           D-Bus connection this event occurred on
     * @param sender         D-Bus bus name of the requester
     * @param obj_path       D-Bus object path to the object being requested
     * @param intf_name      D-Bus interface of the property being accessed
     * @param property_name  The property name being accessed
     * @param error          A GLib2 GError object if an error occurs
     *
     * @return  Returns a GVariant Glib2 object containing the value of the
     *          requested D-Bus object property.  On errors, NULL must be
     *          returned and the error must be returned via a GError
     *          object.
     */
    GVariant * callback_get_property(GDBusConnection *conn,
                                     const std::string sender,
                                     const std::string obj_path,
                                     const std::string intf_name,
                                     const std::string property_name,
                                     GError **error)
    {
        IdleCheck_UpdateTimestamp();

        // Properties available for everyone
        if ("owner" == property_name)
        {
            return GetOwner();
        }

        // Properties available for root
        bool allow_root = false;
        if ("persist_tun" == property_name)
        {
            allow_root = true;
        }


        // Properties only available for approved users
        try {
            CheckACL(sender, allow_root);

            GVariant *ret = NULL;

            if ("single_use" == property_name)
            {
                ret = g_variant_new_boolean (single_use);
            }
            else if ("persistent" == property_name)
            {
                ret = g_variant_new_boolean (persistent);
            }
            else if ("valid" == property_name)
            {
                ret = g_variant_new_boolean (valid);
            }
            else if ("readonly" == property_name)
            {
                ret = g_variant_new_boolean (readonly);
            }
            else if ("name"  == property_name)
            {
                ret = g_variant_new_string (name.c_str());
            }
            else if( "import_timestamp" == property_name)
            {
                return g_variant_new_uint64 (import_tstamp);
            }
            else if( "last_used_timestamp" == property_name)
            {
                return g_variant_new_uint64 (last_use_tstamp);
            }
            else if( "used_count" == property_name)
            {
                return g_variant_new_uint32 (used_count);
            }
            else if ("alias" == property_name)
            {
                ret = g_variant_new_string(alias ? alias->GetAlias() : "");
            }
            else if ("locked_down" == property_name)
            {
                ret = g_variant_new_boolean (locked_down);
            }
            else if ("public_access" == property_name)
            {
                ret = GetPublicAccess();
            }
            else if ("persist_tun" == property_name)
            {
                ret = g_variant_new_boolean (persist_tun);
            }
            else if ("acl" == property_name)
            {
                    ret = GetAccessList();
            }
            else
            {
                g_set_error (error,
                             G_IO_ERROR,
                             G_IO_ERROR_FAILED,
                             "Unknown property");
            }

            return ret;
        }
        catch (DBusCredentialsException& excp)
        {
            LogWarn(excp.err());
            excp.SetDBusError(error, G_IO_ERROR, G_IO_ERROR_FAILED);
            return NULL;
        }
    };


    /**
     *  Callback method which is used each time a ConfigurationObject property
     *  is being modified over D-Bus.
     *
     * @param conn           D-Bus connection this event occurred on
     * @param sender         D-Bus bus name of the requester
     * @param obj_path       D-Bus object path to the object being requested
     * @param intf_name      D-Bus interface of the property being accessed
     * @param property_name  The property name being accessed
     * @param value          GVariant object containing the value to be stored
     * @param error          A GLib2 GError object if an error occurs
     *
     * @return Returns a GVariantBuilder object which will be used by the
     *         D-Bus library to issue the required PropertiesChanged signal.
     *         If an error occurres, the DBusPropertyException is thrown.
     */
    GVariantBuilder * callback_set_property(GDBusConnection *conn,
                                            const std::string sender,
                                            const std::string obj_path,
                                            const std::string intf_name,
                                            const std::string property_name,
                                            GVariant *value,
                                            GError **error)
    {
        IdleCheck_UpdateTimestamp();
        if (readonly)
        {
            throw DBusPropertyException(G_IO_ERROR, G_IO_ERROR_READ_ONLY,
                                        obj_path, intf_name, property_name,
                                        "Configuration object is read-only");
        }

        try
        {
            CheckOwnerAccess(sender);

            GVariantBuilder * ret = NULL;
            if (("alias" == property_name) && conn)
            {
                if (nullptr != alias)
                {
                    alias->RemoveObject(conn);
                    delete alias;
                    alias = nullptr;
                }

                try
                {
                    gsize len = 0;
                    alias =  new ConfigurationAlias(conn,
                                                    std::string(g_variant_get_string(value, &len)),
                                                    GetObjectPath());
                    alias->RegisterObject(conn);
                    ret = build_set_property_response(property_name, alias->GetAlias());
                }
                catch (DBusException& err)
                {
                    delete alias;
                    alias = nullptr;
                    throw DBusPropertyException(G_IO_ERROR, G_IO_ERROR_EXISTS,
                                                obj_path, intf_name, property_name,
                                                err.getRawError().c_str());
                }
            }
            else if (("name" == property_name) && conn)
            {
                gsize len = 0;
                name = std::string(g_variant_get_string(value, &len));
                ret = build_set_property_response(property_name, name);
            }
            else if (("locked_down" == property_name) && conn)
            {
                locked_down = g_variant_get_boolean(value);
                ret = build_set_property_response(property_name, locked_down);
                LogVerb1("Configuration lock-down flag set to "
                         + (locked_down ? std::string("true") : std::string("false"))
                         + " by UID " + std::to_string(GetUID(sender)));
            }
            else if (("public_access" == property_name) && conn)
            {
                bool acl_public = g_variant_get_boolean(value);
                SetPublicAccess(acl_public);
                ret = build_set_property_response(property_name, acl_public);
                LogVerb1("Public access set to "
                         + (acl_public ? std::string("true") : std::string("false"))
                         + " by UID " + std::to_string(GetUID(sender)));
            }
            else if (("persist_tun" == property_name) && conn)
            {
                persist_tun = g_variant_get_boolean(value);
                ret = build_set_property_response(property_name, persist_tun);
            }
            else
            {
                throw DBusPropertyException(G_IO_ERROR, G_IO_ERROR_FAILED,
                                            obj_path, intf_name, property_name,
                                            "Denied");
            };

            return ret;
        }
        catch (DBusCredentialsException& excp)
        {
            LogWarn(excp.err());
            throw DBusPropertyException(G_IO_ERROR, G_IO_ERROR_FAILED,
                                        obj_path, intf_name, property_name,
                                        excp.getUserError());
        }
    };


private:
    std::function<void()> remove_callback;
    std::string name;
    std::time_t import_tstamp;
    std::time_t last_use_tstamp;
    unsigned int used_count;
    bool valid;
    bool readonly;
    bool single_use;
    bool persistent;
    bool locked_down;
    bool persist_tun;
    ConfigurationAlias *alias;
    OptionListJSON options;
};


/**
 *  The ConfigManagerObject is the main object for the Configuration Manager
 *  D-Bus service.  Whenever any user (D-Bus clients) accesses the main
 *  object path, this object is invoked.
 *
 *  This object basically handles the Import method, which takes a
 *  configuration profile as input and creates a ConfigurationObject.  There
 *  will exist a ConfigurationObject for each imported profile, having its
 *  own unique D-Bus object path.  Whenever a user (D-Bus client) accesses
 *  an object, the ConfigurationObject is invoked directly by the D-Bus
 *  implementation.
 */
class ConfigManagerObject : public DBusObject,
                            public ConfigManagerSignals,
                            public RC<thread_safe_refcount>
{
public:
    typedef  RCPtr<ConfigManagerObject> Ptr;

    /**
     *  Constructor initializing the ConfigManagerObject to be registered on
     *  the D-Bus.
     *
     * @param dbuscon  D-Bus this object is tied to
     * @param objpath  D-Bus object path to this object
     */
    ConfigManagerObject(GDBusConnection *dbusc, const std::string objpath)
        : DBusObject(objpath),
          ConfigManagerSignals(dbusc, objpath),
          dbuscon(dbusc),
          creds(dbusc)
    {
        std::stringstream introspection_xml;
        introspection_xml << "<node name='" + objpath + "'>"
                          << "    <interface name='" + OpenVPN3DBus_interf_configuration + "'>"
                          << "        <method name='Import'>"
                          << "          <arg type='s' name='name' direction='in'/>"
                          << "          <arg type='s' name='config_str' direction='in'/>"
                          << "          <arg type='b' name='single_use' direction='in'/>"
                          << "          <arg type='b' name='persistent' direction='in'/>"
                          << "          <arg type='o' name='config_path' direction='out'/>"
                          << "        </method>"
                          << "        <method name='FetchAvailableConfigs'>"
                          << "          <arg type='ao' name='paths' direction='out'/>"
                          << "        </method>"
                          << GetLogIntrospection()
                          << "    </interface>"
                          << "</node>";
        ParseIntrospectionXML(introspection_xml);

        Debug("ConfigManagerObject registered on '" + OpenVPN3DBus_interf_configuration + "':" + objpath);
    }

    ~ConfigManagerObject()
    {
        LogInfo("Shutting down");
        RemoveObject(dbuscon);
    }


    /**
     * Enables logging to file in addition to the D-Bus Log signal events
     *
     * @param filename  String containing the name of the log file
     */
    void OpenLogFile(std::string filename)
    {
        OpenLogFile(filename);
    }


    /**
     *  Callback method called each time a method in the
     *  ConfigurationManagerObject is called over the D-Bus.
     *
     * @param conn        D-Bus connection where the method call occurred
     * @param sender      D-Bus bus name of the sender of the method call
     * @param obj_path    D-Bus object path of the target object.
     * @param intf_name   D-Bus interface of the method call
     * @param method_name D-Bus method name to be executed
     * @param params      GVariant Glib2 object containing the arguments for
     *                    the method call
     * @param invoc       GDBusMethodInvocation where the response/result of
     *                    the method call will be returned.
     */
    void callback_method_call(GDBusConnection *conn,
                              const std::string sender,
                              const std::string obj_path,
                              const std::string intf_name,
                              const std::string method_name,
                              GVariant *params,
                              GDBusMethodInvocation *invoc)
    {
        IdleCheck_UpdateTimestamp();
        if ("Import" == method_name)
        {
            // Import the configuration
            std::string cfgpath = generate_path_uuid(OpenVPN3DBus_rootp_configuration, 'x');

            auto *cfgobj = new ConfigurationObject(dbuscon,
                                                   [self=Ptr(this), cfgpath]()
                                                   {
                                                       self->remove_config_object(cfgpath);
                                                   },
                                                   cfgpath,
                                                   creds.GetUID(sender),
                                                   params);
            IdleCheck_RefInc();
            cfgobj->IdleCheck_Register(IdleCheck_Get());
            cfgobj->RegisterObject(conn);
            config_objects[cfgpath] = cfgobj;

            Debug(std::string("ConfigurationObject registered on '")
                         + intf_name + "': " + cfgpath
                         + " (owner uid " + std::to_string(creds.GetUID(sender)) + ")");
            g_dbus_method_invocation_return_value(invoc, g_variant_new("(o)", cfgpath.c_str()));
        }
        else if ("FetchAvailableConfigs" == method_name)
        {
            // Build up an array of object paths to available config objects
            GVariantBuilder *bld = g_variant_builder_new(G_VARIANT_TYPE("ao"));
            for (auto& item : config_objects)
            {
                try {
                    // We check if the caller is allowed to access this
                    // configuration object.  If not, an exception is thrown
                    // and we will just ignore that exception and continue
                    item.second->CheckACL(sender);
                    g_variant_builder_add(bld, "o", item.first.c_str());
                }
                catch (DBusCredentialsException& excp)
                {
                    // Ignore credentials exceptions.  It means the
                    // caller does not have access this configuration object
                }
            }

            // Wrap up the result into a tuple, which GDBus expects and
            // put it into the invocation response
            GVariantBuilder *ret = g_variant_builder_new(G_VARIANT_TYPE_TUPLE);
            g_variant_builder_add_value(ret, g_variant_builder_end(bld));
            g_dbus_method_invocation_return_value(invoc,
                                                  g_variant_builder_end(ret));

            // Clean-up
            g_variant_builder_unref(bld);
            g_variant_builder_unref(ret);
        }
    };


    /**
     *  Callback which is used each time a ConfigManagerObject D-Bus
     *  property is being read.
     *
     *  For the ConfigManagerObject, this method will just return NULL
     *  with an error set in the GError return pointer.  The
     *  ConfigManagerObject does not use properties at all.
     *
     * @param conn           D-Bus connection this event occurred on
     * @param sender         D-Bus bus name of the requester
     * @param obj_path       D-Bus object path to the object being requested
     * @param intf_name      D-Bus interface of the property being accessed
     * @param property_name  The property name being accessed
     * @param error          A GLib2 GError object if an error occurs
     *
     * @return  Returns always NULL, as there are no properties in the
     *          ConfigManagerObject.
     */
    GVariant * callback_get_property(GDBusConnection *conn,
                                     const std::string sender,
                                     const std::string obj_path,
                                     const std::string intf_name,
                                     const std::string property_name,
                                     GError **error)
    {
        IdleCheck_UpdateTimestamp();
        GVariant *ret = NULL;
        g_set_error (error,
                     G_IO_ERROR,
                     G_IO_ERROR_FAILED,
                     "Unknown property");
        return ret;
    };


    /**
     *  Callback method which is used each time a ConfigManagerObject
     *  property is being modified over the D-Bus.
     *
     *  This will always fail with an exception, as there exists no properties
     *  which can be modified in a ConfigManagerObject.
     *
     * @param conn           D-Bus connection this event occurred on
     * @param sender         D-Bus bus name of the requester
     * @param obj_path       D-Bus object path to the object being requested
     * @param intf_name      D-Bus interface of the property being accessed
     * @param property_name  The property name being accessed
     * @param value          GVariant object containing the value to be stored
     * @param error          A GLib2 GError object if an error occurs
     *
     * @return Will always throw an exception as there are no properties to
     *         modify.
     */
    GVariantBuilder * callback_set_property(GDBusConnection *conn,
                                            const std::string sender,
                                            const std::string obj_path,
                                            const std::string intf_name,
                                            const std::string property_name,
                                            GVariant *value,
                                            GError **error)
    {
        THROW_DBUSEXCEPTION("ConfigManagerObject", "get property not implemented");
    }


private:
    GDBusConnection *dbuscon;
    DBusConnectionCreds creds;
    std::map<std::string, ConfigurationObject *> config_objects;

    /**
     * Callback function used by ConfigurationObject instances to remove
     * its object path from the main registry of configuration objects
     *
     * @param cfgpath  std::string containing the object path to the object
     *                 to remove
     *
     */
    void remove_config_object(const std::string cfgpath)
    {
        config_objects.erase(cfgpath);
    }
};


/**
 *  Main D-Bus service implementation of the Configuration Manager.
 *
 *  This object will register the configuration manager service (destination)
 *  on the D-Bus and create a main manager object (ConfigManagerObject)
 *  which will be invoked whenever a D-Bus client (proxy) accesses this
 *  service.
 */
class ConfigManagerDBus : public DBus
{
public:
    /**
     * Constructor creating a D-Bus service for the Configuration Manager.
     *
     * @param bustype   GBusType, which defines if this service should be
     *                  registered on the system or session bus.
     */
    ConfigManagerDBus(GBusType bustype)
        : DBus(bustype,
               OpenVPN3DBus_name_configuration,
               OpenVPN3DBus_rootp_configuration,
               OpenVPN3DBus_interf_configuration),
          cfgmgr(nullptr),
          procsig(nullptr),
          logfile("")
    {
    };

    ~ConfigManagerDBus()
    {
        procsig->ProcessChange(StatusMinor::PROC_STOPPED);
        delete procsig;
    }


    /**
     *  Prepares logging to file.  This happens in parallel with the
     *  D-Bus Log events which will be sent with Log events.
     *
     * @param filename  Filename of the log file to save the log events.
     */
    void SetLogFile(std::string filename)
    {
        logfile = filename;
    }


    /**
     *  This callback is called when the service was successfully registered
     *  on the D-Bus.
     */
    void callback_bus_acquired()
    {
        cfgmgr.reset(new ConfigManagerObject(GetConnection(), GetRootPath()));
        if (!logfile.empty())
        {
            cfgmgr->OpenLogFile(logfile);
        }
        cfgmgr->RegisterObject(GetConnection());

        procsig = new ProcessSignalProducer(GetConnection(),
                                            OpenVPN3DBus_interf_configuration,
                                            "ConfigurationManager");
        procsig->ProcessChange(StatusMinor::PROC_STARTED);

        if (nullptr != idle_checker)
        {
            cfgmgr->IdleCheck_Register(idle_checker);
        }
    };


    /**
     *  This is called each time the well-known bus name is successfully
     *  acquired on the D-Bus.
     *
     *  This is not used, as the preparations already happens in
     *  callback_bus_acquired()
     *
     * @param conn     Connection where this event happened
     * @param busname  A string of the acquired bus name
     */
    void callback_name_acquired(GDBusConnection *conn, std::string busname)
    {
    };


    /**
     *  This is called each time the well-known bus name is removed from the
     *  D-Bus.  In our case, we just throw an exception and starts shutting
     *  down.
     *
     * @param conn     Connection where this event happened
     * @param busname  A string of the lost bus name
     */
    void callback_name_lost(GDBusConnection *conn, std::string busname)
    {
        THROW_DBUSEXCEPTION("ConfigManagerDBus", "Configuration D-Bus name not registered: '" + busname + "'");
    };

private:
    ConfigManagerObject::Ptr cfgmgr;
    ProcessSignalProducer * procsig;
    std::string logfile;
};

#endif // OPENVPN3_DBUS_CONFIGMGR_HPP
