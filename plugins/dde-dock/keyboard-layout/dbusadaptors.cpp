// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "utils.h"
#include "dbusadaptors.h"
#include "fcitxinputmethoditem.h"
#include "plugins-logging-category.h"

#include <DDBusSender>
#include <DSysInfo>

#include <QDebug>
#include <QtDBus/QDBusConnection>

// switch kdb layout key in gsettings
const QString KDB_LAYOUT_KEYBINDING_KEY = "switchNextKbdLayout";

// dcc keyboard layout key in gsettings
const QString KDB_LAYOUT_DCC_NAME = "keyboardLayout";

// because not allowd to use libfcitx-qt, use org.fcitx.Fcitx to
// get fcitx status and data
const QString FCITX_ADDRESSS = "org.fcitx.Fcitx";

Q_LOGGING_CATEGORY(KEYBOARD_LAYOUT, "org.deepin.dde.dock.keyboardLayout")

DBusAdaptors::DBusAdaptors(QObject *parent)
    : QDBusAbstractAdaptor(parent),
      m_keyboard(new Keyboard("org.deepin.dde.InputDevices1",
                              "/org/deepin/dde/InputDevice1/Keyboard",
                              QDBusConnection::sessionBus(), this)),
    m_menu(new QMenu()),
    m_fcitxRunning(false),
    m_inputmethod(nullptr)
{
    m_keyboard->setSync(false);

    connect(m_keyboard, &Keyboard::CurrentLayoutChanged, this, &DBusAdaptors::onCurrentLayoutChanged);
    connect(m_keyboard, &Keyboard::UserLayoutListChanged, this, &DBusAdaptors::onUserLayoutListChanged);
    connect(m_menu, &QMenu::triggered, this, &DBusAdaptors::handleActionTriggered);

    // init data
    initAllLayoutList();
    onCurrentLayoutChanged(m_keyboard->currentLayout());
    onUserLayoutListChanged(m_keyboard->userLayoutList());

    // deepin show fcitx lang code,while fcitx is running
    if (Dtk::Core::DSysInfo::isCommunityEdition()) {
        initFcitxWatcher();
    }

}

DBusAdaptors::~DBusAdaptors()
{
}

QString DBusAdaptors::layout() const
{
    if (m_userLayoutList.size() < 2) {
        // do NOT show keyboard indicator
        return QString();
    }

    if (m_currentLayout.isEmpty()) {
        // refetch data
        QTimer::singleShot(1000, m_keyboard, &Keyboard::currentLayout);
    }

    return m_currentLayout;
}

void DBusAdaptors::setLayout(const QString &str)
{
    m_currentLayout = str;
    emit layoutChanged(str);
}

Keyboard *DBusAdaptors::getCurrentKeyboard()
{
    return m_keyboard;
}

void DBusAdaptors::onClicked(int button, int x, int y)
{
//    button value means(XCB_BUTTON_INDEX):
//    0, Any of the following (or none)
//    1, The left mouse button.
//    2, The right mouse button.
//    3, The middle mouse button.
//    4, Scroll wheel. TODO: direction?
//    5, Scroll wheel. TODO: direction?

    Q_UNUSED(button);

    if (m_menu && m_userLayoutList.size() >= 2 && !m_fcitxRunning) {
        m_menu->exec(QPoint(x, y));
    }
}

void DBusAdaptors::onCurrentLayoutChanged(const QString &value)
{
    m_currentLayoutRaw = value;
    m_currentLayout = value.split(';').first();

    refreshMenuSelection();

    emit layoutChanged(layout());
}

void DBusAdaptors::onUserLayoutListChanged(const QStringList &value)
{
    m_userLayoutList = value;

    initAllLayoutList();

    emit layoutChanged(layout());
}

void DBusAdaptors::initAllLayoutList()
{
    QDBusPendingCall call = m_keyboard->LayoutList();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [=] {
        if (call.isError()) {
            qCWarning(KEYBOARD_LAYOUT) << "Failed to get all keyboard list, error: " << call.error().message();
        } else {
            QDBusReply<KeyboardLayoutList> reply = call.reply();
            m_allLayoutList = reply.value();
            refreshMenu();
        }

        watcher->deleteLater();
    });
}

void DBusAdaptors::refreshMenu()
{
    if (!m_menu || m_userLayoutList.size() < 2) {
        return;
    }

    // all action object will be deleted
    m_menu->clear();

    for (const QString &layoutRawName : m_userLayoutList) {
        const QString layoutName = duplicateCheck(layoutRawName);
        const QString layoutLocalizedName = m_allLayoutList.value(layoutRawName);
        const QString text = QString("%1 (%2)").arg(layoutLocalizedName, layoutName);

        QAction *action = new QAction(text, m_menu);
        action->setObjectName(layoutRawName);
        action->setCheckable(true);
        action->setChecked(layoutRawName == m_currentLayoutRaw);
        m_menu->addAction(action);
    }

    m_menu->addSeparator();

    // will be deleted after QMenu->clear() above
    m_addLayoutAction = new QAction(tr("Add keyboard layout"), m_menu);

    m_menu->addAction(m_addLayoutAction);
}

void DBusAdaptors::refreshMenuSelection()
{
    const auto &actions = m_menu->actions();
    for (QAction *action : actions) {
        action->setChecked(action->objectName() == m_currentLayoutRaw);
    }
}

void DBusAdaptors::handleActionTriggered(QAction *action)
{
    if (action == m_addLayoutAction) {
        DDBusSender()
                .service("org.deepin.dde.ControlCenter1")
                .interface("org.deepin.dde.ControlCenter1")
                .path("/org/deepin/dde/ControlCenter1")
                .method("ShowPage")
                .arg(QString("keyboard"))
                .arg(QString("Keyboard Layout/Add Keyboard Layout"))
                .call();
    }

    const QString layout = action->objectName();
    if (m_userLayoutList.contains(layout)) {
        m_keyboard->setCurrentLayout(layout);
    }
}

void DBusAdaptors::onGSettingsChanged(const QString &key)
{
    Q_UNUSED(key);

#if 0 // TODO origin gsettings: "com.deepin.dde.dock.module.keyboard"
    // 键盘布局插件处显示的内容就是QLabel中的内容，有文字了就显示，没有文字就不显示了
    if (m_gsettings && m_gsettings->keys().contains("enable")) {
        const bool enable = m_gsettings->get("enable").toBool();
        QString layoutStr = getCurrentKeyboard()->currentLayout().split(';').first();
        setLayout(enable ? layoutStr : "");
    }
#endif
}

QString DBusAdaptors::duplicateCheck(const QString &kb)
{
    QStringList list;
    const auto &kbs = kb.split(";");
    const QString &kbFirst = kbs.first();
    for (const QString &data : m_userLayoutList) {
        if (data.split(";").first() == kbFirst) {
            list << data;
        }
    }

    const QString kblayout = kb.split(";").first().mid(0, 2);

    return kblayout + (list.count() > 1 ? QString::number(list.indexOf(kb) + 1) : "");
}

void DBusAdaptors::onFcitxConnected(const QString &service)
{
    Q_UNUSED(service)
    if (m_fcitxRunning)
        return;

    // fcitx from closed to running
    m_fcitxRunning = true;
    setKeyboardLayoutGsettings();
    if (m_inputmethod) {
        delete m_inputmethod;
        m_inputmethod = nullptr;
    }
    // fcitx from off to on will create this, free it on fcitx closing.
    m_inputmethod = new FcitxInputMethodProxy(
        FCITX_ADDRESSS,
        "/inputmethod",
        QDBusConnection::sessionBus(),
        this);

    if (QDBusConnection::sessionBus().connect(FCITX_ADDRESSS, "/inputmethod",
        "org.freedesktop.DBus.Properties", "PropertiesChanged", this,
        SLOT(onPropertyChanged(QString, QVariantMap, QStringList)))) {
    } else {
        qCWarning(KEYBOARD_LAYOUT) << "Fcitx's properties changed signal connection was not successful";
    }

    Q_EMIT fcitxStatusChanged(m_fcitxRunning);
}

void DBusAdaptors::onFcitxDisconnected(const QString &service)
{
    Q_UNUSED(service)
    if (!m_fcitxRunning)
        return;

    // fcitx from running to close
    m_fcitxRunning = false;
    setKeyboardLayoutGsettings();
    QDBusConnection::sessionBus().disconnect(FCITX_ADDRESSS, "/inputmethod",
        "org.freedesktop.DBus.Properties", "PropertiesChanged", this,
        SLOT(onPropertyChanged(QString, QVariantMap, QStringList)));
    // fcitx is closing, free it.
    if (m_inputmethod) {
        delete m_inputmethod;
        m_inputmethod = nullptr;
    }

    Q_EMIT fcitxStatusChanged(m_fcitxRunning);

}

void DBusAdaptors::onPropertyChanged(QString name, QVariantMap map, QStringList list)
{
    // fcitx uniquename start with fcitx-keyboard- which contains keyboard layout.
    QString fcitxUniqueName("fcitx-keyboard-");
    qCDebug(KEYBOARD_LAYOUT) << QString("Properties of interface %1 changed").arg(name);

    if (list.isEmpty() || "CurrentIM" != list[0])
        return;

    if (m_inputmethod == nullptr)
        return;

    QString currentIM = m_inputmethod ->GetCurrentIM();
    if (currentIM.startsWith(fcitxUniqueName)) {
        // fcitx uniquename contains keyboard layout, keyboard is after fcitx-keyboard-
        // such as fcitx-keyboard-ara-uga, keyboard layout is ara;uga
        // fcitx-keyboard-us keyboard is us;
        // fcitx-keyboard-am-phonetic-alt keyboard layout is am;phonetic-alt
        QString layout = currentIM.right(currentIM.size() - fcitxUniqueName.size());
        int splitLoc = layout.indexOf('-');
        if (splitLoc > 0) {
            layout =  layout.replace(splitLoc, 1, ';');
        } else {
            layout.append(';');
        }
        m_keyboard->setCurrentLayout(layout);
    } else {
        // sunpinyin sogounpinyin uniquename not contains keyboard-layout. using lang code only for display.
        FcitxQtInputMethodItemList lists = m_inputmethod -> iMList();
        for (FcitxQtInputMethodItem item : lists) {
            if (currentIM == item.uniqueName()) {
                // zh_CN display as cn
                if (0 == QString::compare("zh_CN", item.langCode())) {
                    item.setLangCode("cn");
                }
                QString layout = item.langCode();
                layout.append(';');
                m_keyboard->setCurrentLayout(layout);
            }
        }
    }

}

void DBusAdaptors::setKeyboardLayoutGsettings()
{
    // TODO origin gsettings: "com.deepin.dde.keybinding.system.enable"
#if 0
    // while fcitx is running, disable keyboard switch shortcut, enable it after fcitx stopped
    if (m_keybingEnabled && m_keybingEnabled->keys().contains(KDB_LAYOUT_KEYBINDING_KEY)) {
        m_keybingEnabled->set(KDB_LAYOUT_KEYBINDING_KEY, QVariant(!m_fcitxRunning));
    }

    // TODO origin gsettings: "com.deepin.dde.control-center"
    // hide keyboard layout setttings in dde-control-center, resume it after fcitx stopped
    if (m_dccSettings && m_dccSettings->keys().contains(KDB_LAYOUT_DCC_NAME)) {
        m_dccSettings->set(KDB_LAYOUT_DCC_NAME, QVariant(!m_fcitxRunning));
    }
#endif
}

bool DBusAdaptors::isFcitxRunning() const
{
    return m_fcitxRunning;
}

void DBusAdaptors::initFcitxWatcher()
{
    qCDebug(KEYBOARD_LAYOUT) << "Init fcitx status watcher";
    FcitxQtInputMethodItem::registerMetaType();
    // init dbusSewrviceWatcher to see fcitx status
    m_fcitxWatcher = new QDBusServiceWatcher(this);
    m_fcitxWatcher->setConnection(QDBusConnection::sessionBus());
    m_fcitxWatcher->addWatchedService(FCITX_ADDRESSS);
    // send fcitx on or off signal, when fcitx is starting or closing.
    connect(m_fcitxWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(onFcitxConnected(QString)));
    connect(m_fcitxWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(onFcitxDisconnected(QString)));

    // get fcitx current status
    QDBusReply<bool> registered = m_fcitxWatcher ->connection().interface()->isServiceRegistered(FCITX_ADDRESSS);

    if (registered.isValid() && registered.value()) {
        // fcitx is alerdy running,
        onFcitxConnected(QString());
    }
    setKeyboardLayoutGsettings();
}

