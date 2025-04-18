// SPDX-FileCopyrightText: 2015 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DBUSACCOUNT_H_1439464396
#define DBUSACCOUNT_H_1439464396

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface org.deepin.dde.Accounts1
 */
class DBusAccount: public QDBusAbstractInterface
{
    Q_OBJECT

    Q_SLOT void __propertyChanged__(const QDBusMessage &msg)
    {
        QList<QVariant> arguments = msg.arguments();
        if (3 != arguments.count()) {
            return;
        }
        QString interfaceName = msg.arguments().at(0).toString();
        if (interfaceName != "org.deepin.dde.Accounts1") {
            return;
        }
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        QStringList keys = changedProps.keys();
        foreach(const QString & prop, keys) {
            const QMetaObject *self = metaObject();
            for (int i = self->propertyOffset(); i < self->propertyCount(); ++i) {
                QMetaProperty p = self->property(i);
                if (p.name() == prop) {
                    Q_EMIT p.notifySignal().invoke(this);
                }
            }
        }
    }
public:
    static inline const char *staticService()
    { return "org.deepin.dde.Accounts1"; }
    static inline const char *staticInterfacePath()
    { return "/org/deepin/dde/Accounts1"; }
    static inline const char *staticInterfaceName()
    { return "org.deepin.dde.Accounts1"; }

public:
    explicit DBusAccount(QObject *parent = 0);

    ~DBusAccount();

    Q_PROPERTY(QStringList UserList READ userList)
    inline QStringList userList() const
    { return qvariant_cast< QStringList >(property("UserList")); }
};

//namespace org {
//  namespace deepin {
//    namespace dde {
//      typedef ::DBusAccount Accounts;
//    }
//  }
//}
#endif
