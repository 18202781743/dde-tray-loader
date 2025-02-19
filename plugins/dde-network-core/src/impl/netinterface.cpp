// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netinterface.h"
#include "networkdevicebase.h"
#include "objectmanager.h"

#include <wireddevice.h>

namespace dde {
namespace network {

NetworkProcesser::NetworkProcesser(QObject *parent)
    : QObject (parent)
{
}

NetworkProcesser::~NetworkProcesser()
{
}

QList<NetworkDetails *> NetworkProcesser::networkDetails()
{
    return QList<NetworkDetails *>();
}

ProxyController *NetworkProcesser::proxyController()
{
    return Q_NULLPTR;
}

VPNController *NetworkProcesser::vpnController()
{
    return Q_NULLPTR;
}

DSLController *NetworkProcesser::dslController()
{
    return Q_NULLPTR;
}

HotspotController *NetworkProcesser::hotspotController()
{
    return Q_NULLPTR;
}

void NetworkProcesser::updateDeviceName()
{
    QList<NetworkDeviceBase *> wiredDevice;
    QList<NetworkDeviceBase *> wirelessDevice;

    QList<NetworkDeviceBase *> allDevices = this->devices();
    for (NetworkDeviceBase *device : allDevices) {
        if (device->deviceType() == DeviceType::Wired)
            wiredDevice << device;
        else if (device->deviceType() == DeviceType::Wireless)
            wirelessDevice << device;
    }

    // 更改有线设备的名称
    if (wiredDevice.size() == 1) {
        wiredDevice[0]->setName(tr("Wired Network"));
    } else if (wiredDevice.size() > 1) {
        for (int i = 0; i < wiredDevice.size(); i++) {
            QString name = QString(tr("Wired Network %1")).arg(i + 1);
            wiredDevice[i]->setName(name);
        }
    }

    // 更改无线设备的名称
    if (wirelessDevice.size() == 1) {
        wirelessDevice[0]->setName(tr("Wireless Network"));
    } else if (wirelessDevice.size() > 1) {
        for (int i = 0; i < wirelessDevice.size(); i++) {
            QString name = QString(tr("Wireless Network %1")).arg(i + 1);
            wirelessDevice[i]->setName(name);
        }
    }
}

void NetworkProcesser::retranslate()
{
    updateDeviceName();
}

void NetworkProcesser::onIpConflictChanged(const QString &devicePath, const QString &ip, bool conflicted)
{
    Q_UNUSED(ip);
    QList<NetworkDeviceBase *> allDevices = devices();
    QList<NetworkDeviceBase *>::iterator itDevice = std::find_if(allDevices.begin(), allDevices.end(), [ devicePath ](NetworkDeviceBase *device) {
         return device->path() == devicePath;
    });

    if (itDevice == allDevices.end())
        return;

    ObjectManager::instance()->deviceRealize(*itDevice)->setIpConflict(conflicted);
}

/**
 * @brief 设备接口基类,具体实现在子类中实现
 * @return
 */
void NetworkDeviceRealize::setDevice(NetworkDeviceBase *device)
{
    m_device = device;
}

bool NetworkDeviceRealize::isEnabled() const
{
    return false;
}

bool NetworkDeviceRealize::available() const
{
    return true;
}

bool NetworkDeviceRealize::IPValid()
{
    // 判读获取IP地址失败需要满足最后一个状态为未连接，上一个状态为失败，并且包含Config和IpConfig
    return !(m_statusQueue.size() == MaxQueueSize
            && m_statusQueue[MaxQueueSize - 1] == DeviceStatus::Disconnected
            && m_statusQueue[MaxQueueSize - 2] == DeviceStatus::Failed
            && m_statusQueue.contains(DeviceStatus::Config)
            && m_statusQueue.contains(DeviceStatus::IpConfig));
}

QString NetworkDeviceRealize::interface() const
{
    return QString();
}

QString NetworkDeviceRealize::driver() const
{
    return QString();
}

bool NetworkDeviceRealize::managed() const
{
    return false;
}

QString NetworkDeviceRealize::vendor() const
{
    return QString();
}

QString NetworkDeviceRealize::uniqueUuid() const
{
    return QString();
}

bool NetworkDeviceRealize::usbDevice() const
{
    return false;
}

QString NetworkDeviceRealize::path() const
{
    return QString();
}

QString NetworkDeviceRealize::activeAp() const
{
    return QString();
}

bool NetworkDeviceRealize::supportHotspot() const
{
    return false;
}

QString NetworkDeviceRealize::realHwAdr() const
{
    return QString();
}

QString NetworkDeviceRealize::usingHwAdr() const
{
    return QString();
}

const QStringList NetworkDeviceRealize::ipv4()
{
    return QStringList();
}

const QStringList NetworkDeviceRealize::ipv6()
{
    return QStringList();
}

void NetworkDeviceRealize::setEnabled(bool enabled)
{
    Q_UNUSED(enabled);
}

void NetworkDeviceRealize::disconnectNetwork()
{
}

bool NetworkDeviceRealize::ipConflicted()
{
    return m_ipConflicted;
}

DeviceStatus NetworkDeviceRealize::deviceStatus() const
{
    return m_deviceStatus;
}

QList<AccessPoints *> NetworkDeviceRealize::accessPointItems() const
{
    return QList<AccessPoints *>();
}

void NetworkDeviceRealize::connectNetwork(const AccessPoints *item)
{
    Q_UNUSED(item);
}

QList<WirelessConnection *> NetworkDeviceRealize::wirelessItems() const
{
    return QList<WirelessConnection *>();
}

AccessPoints *NetworkDeviceRealize::activeAccessPoints() const
{
    return nullptr;
}

bool NetworkDeviceRealize::connectNetwork(WiredConnection *connection)
{
    Q_UNUSED(connection);
    return false;
}

QList<WiredConnection *> NetworkDeviceRealize::wiredItems() const
{
    return QList<WiredConnection *>();
}

bool NetworkDeviceRealize::carrier() const
{
    return true;
}

NetworkDeviceRealize::NetworkDeviceRealize(QObject *parent)
    : QObject (parent)
    , m_device(nullptr)
    , m_deviceStatus(DeviceStatus::Unknown)
    , m_ipConflicted(false)
{
}

NetworkDeviceRealize::~NetworkDeviceRealize()
{
}

NetworkDeviceBase *NetworkDeviceRealize::device() const
{
    return m_device;
}

QString NetworkDeviceRealize::statusStringDetail()
{
    if (!isEnabled() || !m_device)
        return tr("Device disabled");

    // 确认 没有获取IP显示未连接状态（DHCP服务关闭）
    if (!IPValid())
        return tr("Not connected");

    switch (m_deviceStatus) {
    case DeviceStatus::Unknown:
    case DeviceStatus::Unmanaged:
    case DeviceStatus::Unavailable: {
        switch (m_device->deviceType()) {
        case DeviceType::Unknown:      return QString();
        case DeviceType::Wired:       return tr("Network cable unplugged");
        default: break;
        }
        break;
    }
    case DeviceStatus::Disconnected:  return tr("Not connected");
    case DeviceStatus::Prepare:
    case DeviceStatus::Config:        return tr("Connecting");
    case DeviceStatus::Needauth:      return tr("Authenticating");
    case DeviceStatus::IpConfig:
    case DeviceStatus::IpCheck:
    case DeviceStatus::Secondaries:   return tr("Obtaining IP address");
    case DeviceStatus::Activated:     return tr("Connected");
    case DeviceStatus::Deactivation:  return tr("Disconnected");
    case DeviceStatus::IpConflict:    return tr("IP conflict");
    default: break;
    }

    return tr("Failed");
}

QString NetworkDeviceRealize::getStatusName()
{
    if (hotspotEnabled() || !available())
        return tr("Disconnected");

    switch (m_deviceStatus) {
    case DeviceStatus::Unmanaged:
    case DeviceStatus::Unavailable:
    case DeviceStatus::Disconnected:  return tr("Disconnected");
    case DeviceStatus::Prepare:
    case DeviceStatus::Config:        return tr("Connecting");
    case DeviceStatus::Needauth:      return tr("Authenticating");
    case DeviceStatus::IpConfig:
    case DeviceStatus::IpCheck:       return tr("Obtaining address");
    case DeviceStatus::Activated:     return tr("Connected");
    case DeviceStatus::Deactivation:  return tr("Disconnected");
    case DeviceStatus::Failed:        return tr("Failed");
    case DeviceStatus::IpConflict:    return tr("IP conflict");
    default:;
    }

    return QString();
}

void NetworkDeviceRealize::enqueueStatus(const DeviceStatus &status)
{
    if (m_statusQueue.size() >= MaxQueueSize)
        m_statusQueue.dequeue();

    m_statusQueue.enqueue(status);
}

void NetworkDeviceRealize::setDeviceStatus(const DeviceStatus &status)
{
    if (m_deviceStatus == status)
        return;

    m_deviceStatus = status;
    enqueueStatus(status);
    // 状态发生变化后，需要向外抛出一个信号
    Q_EMIT deviceStatusChanged(status);
}

void NetworkDeviceRealize::sortWiredItem(QList<WiredConnection *> &items)
{
    // 按照最后面的数字从小到大排序，如果名字中没有数字，就按照创建的先后顺序来排序(path数字的大小)
    std::sort(items.begin(), items.end(), [ ](WiredConnection *conn1, WiredConnection *conn2) {
        QString lastChar1 = conn1->connection()->id().right(1);
        QString lastChar2 = conn2->connection()->id().right(1);
        int rightCount = 3;
        int index1 = 0;
        int index2 = 0;
        bool ok1 = false, ok2 = false;
        do {
            if (!ok1) {
                lastChar1 = conn1->connection()->id().right(rightCount);
                index1 = lastChar1.toInt(&ok1);
            }
            if (!ok2) {
                lastChar2 = conn2->connection()->id().right(rightCount);
                index2 = lastChar2.toInt(&ok2);
            }
            if ((ok1 && ok2) || rightCount <= 0)
                break;

            rightCount--;
        } while (true);

        if (ok1 && ok2)
            return index1 < index2;
        QString path1 = conn1->connection()->path();
        QString path2 = conn2->connection()->path();
        path1 = path1.replace("\\", "/");
        path2 = path2.replace("\\", "/");
        QString configName1 = path1.mid(path1.lastIndexOf("/") + 1);
        QString configName2 = path2.mid(path2.lastIndexOf("/") + 1);
        return configName1.toInt() < configName2.toInt();
    });
}

bool NetworkDeviceRealize::ipConflicted() const
{
    return m_ipConflicted;
}

void NetworkDeviceRealize::setIpConflict(bool ipConflicted)
{
    if (m_ipConflicted != ipConflicted) {
        m_ipConflicted = ipConflicted;
        m_deviceStatus = deviceStatus();
        Q_EMIT deviceStatusChanged(m_deviceStatus);
    }
}

}
}
