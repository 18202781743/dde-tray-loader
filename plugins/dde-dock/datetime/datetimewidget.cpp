// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "datetimewidget.h"
#include "constants.h"
#include "regionFormat.h"

#include <QApplication>
#include <QPainter>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <DFontSizeManager>

#define PLUGIN_STATE_KEY    "enable"

DWIDGET_USE_NAMESPACE

DatetimeWidget::DatetimeWidget(RegionFormat* regionFormat, QWidget *parent)
    : QWidget(parent)
    , m_timeLabel(new QLabel(this))
    , m_dateLabel(new QLabel(this))
    , m_apLabel(new QLabel(this))
    , m_spacerItem(new QWidget(this))
    , m_24HourFormat(false)
    , m_weekdayFormatType(0)
    , m_shortDateFormat("yyyy-MM-dd")
    , m_dockSize(QSize(1920, 37))
    , m_regionFormat(regionFormat)
{
    initUI();

    m_24HourFormat = m_regionFormat->is24HourFormat();
    adjustFontSize();
    updateDateTimeString();
    installEventFilter(this);

    connect(m_regionFormat, &RegionFormat::longDateFormatChanged, this, &DatetimeWidget::updateDateTime);
    connect(m_regionFormat, &RegionFormat::shortTimeFormatChanged, this, &DatetimeWidget::updateDateTime);
    connect(m_regionFormat, &RegionFormat::shortDateFormatChanged, this, &DatetimeWidget::updateDateTime);
    connect(m_regionFormat, &RegionFormat::longTimeFormatChanged, this, &DatetimeWidget::updateDateTime);
    connect(m_regionFormat, &RegionFormat::localeNameChanged, this, &DatetimeWidget::updateDateTime);
}

void DatetimeWidget::set24HourFormat(const bool value)
{
    if (m_24HourFormat == value) {
        return;
    }

    m_24HourFormat = value;
    m_regionFormat->sync24HourFormatConfig(value);
    update();

    if (isVisible()) {
        emit requestUpdateGeometry();
    }
}

/**
 * @brief DatetimeWidget::setShortDateFormat 根据类型设置时间显示格式
 * @param type 自定义类型
 */
void DatetimeWidget::setShortDateFormat(int type)
{
    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();

    // 任务栏在左/右的时候不显示年份
    const bool removeY = (position == Dock::Top || position == Dock::Bottom) ? false : true;
    switch (type) {
    case 0: removeY ? m_shortDateFormat = "M/d" : m_shortDateFormat = "yyyy/M/d"; break;
    case 1: removeY ? m_shortDateFormat = "M-d" : m_shortDateFormat = "yyyy-M-d"; break;
    case 2: removeY ? m_shortDateFormat = "M.d" : m_shortDateFormat = "yyyy.M.d"; break;
    case 3: removeY ? m_shortDateFormat = "MM/dd" : m_shortDateFormat = "yyyy/MM/dd"; break;
    case 4: removeY ? m_shortDateFormat = "MM-dd" : m_shortDateFormat = "yyyy-MM-dd"; break;
    case 5: removeY ? m_shortDateFormat = "MM.dd" : m_shortDateFormat = "yyyy.MM.dd"; break;
    case 6: removeY ? m_shortDateFormat = "MM.dd" : m_shortDateFormat = "MM.dd.yyyy"; break;
    case 7: removeY ? m_shortDateFormat = "dd.MM" : m_shortDateFormat = "dd.MM.yyyy"; break;
    case 8: removeY ? m_shortDateFormat = "M/d" : m_shortDateFormat = "yy/M/d"; break;
    case 9: removeY ? m_shortDateFormat = "M-d" : m_shortDateFormat = "yy-M-d"; break;
    case 10: removeY ? m_shortDateFormat = "M.d" : m_shortDateFormat = "yy.M.d"; break;
    default: removeY ? m_shortDateFormat = "MM-dd" : m_shortDateFormat = "yyyy-MM-dd"; break;
    }

    update();

    if (isVisible()) {
        emit requestUpdateGeometry();
    }
}


/**
 * @brief DatetimeWidget::setWeekdayFormat 根据类型设置周显示格式
 * @param type 自定义类型
 */
void DatetimeWidget::setWeekdayFormat(int type)
{
    if (m_weekdayFormatType == type)
        return;

    m_weekdayFormatType = type;
    updateWeekdayFormat();
    updateDateTimeString();
}

/**
 * @brief DatetimeWidget::updateWeekdayFormat 更新周的显示格式
 */
void DatetimeWidget::updateWeekdayFormat()
{
    const QDateTime currentDateTime = QDateTime::currentDateTime();
    auto dayOfWeek = currentDateTime.date().dayOfWeek();

    if (0 == m_weekdayFormatType) {
        switch (dayOfWeek) {
        case 1:
            m_weekFormat = tr("Monday"); //星期一
            break;
        case 2:
            m_weekFormat = tr("Tuesday"); //星期二
            break;
        case 3:
            m_weekFormat = tr("Wednesday"); //星期三
            break;
        case 4:
            m_weekFormat = tr("Thursday"); //星期四
            break;
        case 5:
            m_weekFormat = tr("Friday"); //星期五
            break;
        case 6:
            m_weekFormat = tr("Saturday"); //星期六
            break;
        case 7:
            m_weekFormat = tr("Sunday"); //星期天
            break;
        default:
            m_weekFormat = tr("Monday"); //星期一
            break;
        }
    } else {
        switch (dayOfWeek) {
        case 1:
            m_weekFormat = tr("monday"); //周一
            break;
        case 2:
            m_weekFormat = tr("tuesday"); //周二
            break;
        case 3:
            m_weekFormat = tr("wednesday"); //周三
            break;
        case 4:
            m_weekFormat = tr("thursday"); //周四
            break;
        case 5:
            m_weekFormat = tr("friday"); //周五
            break;
        case 6:
            m_weekFormat = tr("saturday"); //周六
            break;
        case 7:
            m_weekFormat = tr("sunday"); //周天
            break;
        default:
            m_weekFormat = tr("monday"); //周一
            break;
        }
    }
}

void DatetimeWidget::setRegionFormat(RegionFormat *newRegionFormat)
{
    m_regionFormat = newRegionFormat;
}

/**
 * @brief DatetimeWidget::updateWeekdayTimeString 更新任务栏时间标签的显示
 */
void DatetimeWidget::updateDateTimeString()
{
    QLocale locale(m_regionFormat->getLocaleName());
    m_dateTime = locale.toString(QDate::currentDate(), m_regionFormat->getLongDateFormat()) + " " + QDateTime::currentDateTime().toString(m_regionFormat->getLongTimeFormat());

    QDateTime current = QDateTime::currentDateTime();

    const auto position = qApp->property(PROP_POSITION).value<Dock::Position>();
    QString timeStr, dateString;
    if (position == Dock::Bottom || position == Dock::Top) {
        if (!m_24HourFormat) {
            timeStr = current.toString(m_regionFormat->getShortTimeFormat());
        } else {
            timeStr = current.toString("hh:mm");
        }
        dateString = current.toString(m_regionFormat->getShortDateFormat());

        m_timeLabel->setText(timeStr);
        m_dateLabel->setText(dateString);
    } else {
        if (!m_24HourFormat) {
            QString apText = current.toString("AP");
            m_apLabel->setText(apText);

            timeStr = current.toString("h:mm");
        } else {
            timeStr = current.toString("hh:mm");
        }

        m_timeLabel->setText(timeStr);
        dateString = current.toString(m_regionFormat->getShortDateFormat());
        m_dateLabel->setText(dateString);
    }

#if 0 // for debug
    m_timeLabel->setStyleSheet("background-color: red;");
    m_dateLabel->setStyleSheet("background-color: blue;");
    QFontMetrics fontMetrics(m_timeLabel->font());
    qInfo() << "timeLabel font height:" << fontMetrics.height() << " sizeHint:" << m_timeLabel->sizeHint();;
#endif
}

void DatetimeWidget::updateDateTime()
{
    m_24HourFormat = m_regionFormat->is24HourFormat();
    adjustUI();
    updateDateTimeString();
    update();

    if (isVisible()) {
        emit requestUpdateGeometry();
    }
}

void DatetimeWidget::adjustFontSize()
{
    const auto position = qApp->property(PROP_POSITION).value<Dock::Position>();
    int validDistance = m_dockSize.height() / devicePixelRatioF();
    if (position == Dock::Left || position == Dock::Right) {
        validDistance = m_dockSize.width() / devicePixelRatioF();
    }

    // 根据时间和日期字体大小的跨度，将dock栏大小分为不同的区间，每个区域对应不同的字体大小，然后通过判断dock栏大小所在的区间来设置字体大小
    // 如果任务栏小于37，则字体始终取最小值；如果任务栏大于61，则字体始终取最大值；如果任务栏在37和61之间，则字体大小随任务栏大小线性变化
    static const QMap<int, QPair<int, int>> fontSizeMap {
        {0, {12, 9}},
        {37, {12, 9}},
        {40, {13, 10}},
        {43, {14, 10}},
        {46, {15, 11}},
        {49, {16, 11}},
        {52, {17, 12}},
        {55, {18, 12}},
        {58, {19, 13}},
        {61, {20, 14}},
        {999, {20, 14}}
    };

    int timeFontSize = 0;
    int dateFontSize = 0;
    QList<int> distances = fontSizeMap.keys();
    for (int i = 0; i < distances.size() - 1; ++i) {
        if (validDistance >= distances.at(i) && validDistance < distances.at(i + 1)) {
            timeFontSize = fontSizeMap.value(distances.at(i)).first;
            dateFontSize = fontSizeMap.value(distances.at(i)).second;
            break;
        }
    }

    Q_ASSERT(timeFontSize != 0 && dateFontSize != 0);

    QFont timeFont = m_timeLabel->font();
    timeFont.setPixelSize(timeFontSize);
    m_timeLabel->setFont(timeFont);
    m_apLabel->setFont(timeFont);

    QFont dateFont = m_dateLabel->font();
    dateFont.setPixelSize(dateFontSize);
    m_dateLabel->setFont(dateFont);
}

void DatetimeWidget::resizeEvent(QResizeEvent *event)
{
    if (isVisible())
        emit requestUpdateGeometry();

    QWidget::resizeEvent(event);
}

bool DatetimeWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::DevicePixelRatioChange && watched == this) {
        adjustFontSize();
    }

    return QWidget::eventFilter(watched, event);
}

void DatetimeWidget::setDockPanelSize(const QSize &dockSize)
{
    // 任务栏高度最小是37，小于37说明在隐藏和显示动画中
    if (dockSize.width() < 37) {
        return;
    }
    if (dockSize.height() < 37) {
        return;
    }

    if (m_dockSize != dockSize) {
        m_dockSize = dockSize;
        adjustFontSize();
        update();
    }

    Q_EMIT requestUpdateGeometry();
}

void DatetimeWidget::dockPositionChanged()
{
    // 等待位置变换完成后再更新
    QTimer::singleShot(300, this, [this]{
        updateDateTime();
    });

    adjustUI();
    adjustFontSize();
}

void DatetimeWidget::initUI()
{
    setContentsMargins(0, 0, 0, 0);

    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_dateLabel->setAlignment(Qt::AlignCenter);
    m_apLabel->setAlignment(Qt::AlignCenter);

    m_timeLabel->setContentsMargins(0, 0, 0, 0);
    m_dateLabel->setContentsMargins(0, 0, 0, 0);
    m_apLabel->setContentsMargins(0, 0, 0, 0);

    // 当任务栏在左/右时，时间文本和日期的间距
    m_spacerItem->setFixedSize(10, 5);

    m_timeLabel->setForegroundRole(QPalette::BrightText);
    m_apLabel->setForegroundRole(QPalette::BrightText);

    auto *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_apLabel);
    layout->addWidget(m_timeLabel);
    layout->addWidget(m_spacerItem);
    layout->addWidget(m_dateLabel);

    setLayout(layout);

    adjustUI();
}

void DatetimeWidget::adjustUI()
{
    const auto position = qApp->property(PROP_POSITION).value<Dock::Position>();
    if (position == Dock::Left || position == Dock::Right) {
        if (!m_24HourFormat) {
            m_spacerItem->setVisible(true);
            m_apLabel->setVisible(true);
            return;
        }
    }

    m_spacerItem->setVisible(false);
    m_apLabel->setVisible(false);
}