/****************************************************************************
** Meta object code from reading C++ file 'AppController.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/app/AppController.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AppController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13AppControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto AppController::qt_create_metaobjectdata<qt_meta_tag_ZN13AppControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AppController",
        "bannerChanged",
        "",
        "offlineChanged",
        "historyAvailableChanged",
        "currentSensorIdChanged",
        "currentParamCodeChanged",
        "mapStationMetricsChanged",
        "mapMetricRangeChanged",
        "mapOverlayStatusChanged",
        "stationsChanged",
        "sensorsChanged",
        "chartPointsChanged",
        "statsChanged",
        "refreshStations",
        "loadSensors",
        "stationId",
        "loadOnline",
        "sensorId",
        "saveCurrentToDb",
        "loadHistory",
        "fromIso",
        "toIso",
        "loadCurrentHistory",
        "days",
        "refreshMapMeasurements",
        "QVariantList",
        "stationIds",
        "paramCode",
        "clearMapMeasurements",
        "banner",
        "offline",
        "historyAvailable",
        "currentSensorId",
        "currentParamCode",
        "mapStationMetrics",
        "QVariantMap",
        "mapMetricRange",
        "mapOverlayStatus",
        "stations",
        "sensors",
        "chartPoints",
        "stats"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'bannerChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'offlineChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'historyAvailableChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentSensorIdChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'currentParamCodeChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'mapStationMetricsChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'mapMetricRangeChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'mapOverlayStatusChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'stationsChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'sensorsChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'chartPointsChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'statsChanged'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'refreshStations'
        QtMocHelpers::MethodData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'loadSensors'
        QtMocHelpers::MethodData<void(int)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 16 },
        }}),
        // Method 'loadOnline'
        QtMocHelpers::MethodData<void(int)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 18 },
        }}),
        // Method 'saveCurrentToDb'
        QtMocHelpers::MethodData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'loadHistory'
        QtMocHelpers::MethodData<void(int, const QString &, const QString &)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 18 }, { QMetaType::QString, 21 }, { QMetaType::QString, 22 },
        }}),
        // Method 'loadCurrentHistory'
        QtMocHelpers::MethodData<void(int)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 24 },
        }}),
        // Method 'refreshMapMeasurements'
        QtMocHelpers::MethodData<void(const QVariantList &, const QString &)>(25, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 26, 27 }, { QMetaType::QString, 28 },
        }}),
        // Method 'clearMapMeasurements'
        QtMocHelpers::MethodData<void()>(29, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'banner'
        QtMocHelpers::PropertyData<QString>(30, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'offline'
        QtMocHelpers::PropertyData<bool>(31, QMetaType::Bool, QMC::DefaultPropertyFlags, 1),
        // property 'historyAvailable'
        QtMocHelpers::PropertyData<bool>(32, QMetaType::Bool, QMC::DefaultPropertyFlags, 2),
        // property 'currentSensorId'
        QtMocHelpers::PropertyData<int>(33, QMetaType::Int, QMC::DefaultPropertyFlags, 3),
        // property 'currentParamCode'
        QtMocHelpers::PropertyData<QString>(34, QMetaType::QString, QMC::DefaultPropertyFlags, 4),
        // property 'mapStationMetrics'
        QtMocHelpers::PropertyData<QVariantMap>(35, 0x80000000 | 36, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 5),
        // property 'mapMetricRange'
        QtMocHelpers::PropertyData<QVariantMap>(37, 0x80000000 | 36, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 6),
        // property 'mapOverlayStatus'
        QtMocHelpers::PropertyData<QString>(38, QMetaType::QString, QMC::DefaultPropertyFlags, 7),
        // property 'stations'
        QtMocHelpers::PropertyData<QVariantList>(39, 0x80000000 | 26, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 8),
        // property 'sensors'
        QtMocHelpers::PropertyData<QVariantList>(40, 0x80000000 | 26, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 9),
        // property 'chartPoints'
        QtMocHelpers::PropertyData<QVariantList>(41, 0x80000000 | 26, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 10),
        // property 'stats'
        QtMocHelpers::PropertyData<QVariantMap>(42, 0x80000000 | 36, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 11),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AppController, qt_meta_tag_ZN13AppControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AppController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13AppControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13AppControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13AppControllerE_t>.metaTypes,
    nullptr
} };

void AppController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AppController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->bannerChanged(); break;
        case 1: _t->offlineChanged(); break;
        case 2: _t->historyAvailableChanged(); break;
        case 3: _t->currentSensorIdChanged(); break;
        case 4: _t->currentParamCodeChanged(); break;
        case 5: _t->mapStationMetricsChanged(); break;
        case 6: _t->mapMetricRangeChanged(); break;
        case 7: _t->mapOverlayStatusChanged(); break;
        case 8: _t->stationsChanged(); break;
        case 9: _t->sensorsChanged(); break;
        case 10: _t->chartPointsChanged(); break;
        case 11: _t->statsChanged(); break;
        case 12: _t->refreshStations(); break;
        case 13: _t->loadSensors((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 14: _t->loadOnline((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->saveCurrentToDb(); break;
        case 16: _t->loadHistory((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3]))); break;
        case 17: _t->loadCurrentHistory((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 18: _t->refreshMapMeasurements((*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 19: _t->clearMapMeasurements(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::bannerChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::offlineChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::historyAvailableChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::currentSensorIdChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::currentParamCodeChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::mapStationMetricsChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::mapMetricRangeChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::mapOverlayStatusChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::stationsChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::sensorsChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::chartPointsChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppController::*)()>(_a, &AppController::statsChanged, 11))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->banner(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->offline(); break;
        case 2: *reinterpret_cast<bool*>(_v) = _t->historyAvailable(); break;
        case 3: *reinterpret_cast<int*>(_v) = _t->currentSensorId(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->currentParamCode(); break;
        case 5: *reinterpret_cast<QVariantMap*>(_v) = _t->mapStationMetrics(); break;
        case 6: *reinterpret_cast<QVariantMap*>(_v) = _t->mapMetricRange(); break;
        case 7: *reinterpret_cast<QString*>(_v) = _t->mapOverlayStatus(); break;
        case 8: *reinterpret_cast<QVariantList*>(_v) = _t->stations(); break;
        case 9: *reinterpret_cast<QVariantList*>(_v) = _t->sensors(); break;
        case 10: *reinterpret_cast<QVariantList*>(_v) = _t->chartPoints(); break;
        case 11: *reinterpret_cast<QVariantMap*>(_v) = _t->stats(); break;
        default: break;
        }
    }
}

const QMetaObject *AppController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AppController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13AppControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AppController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 20;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void AppController::bannerChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AppController::offlineChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AppController::historyAvailableChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void AppController::currentSensorIdChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void AppController::currentParamCodeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void AppController::mapStationMetricsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void AppController::mapMetricRangeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void AppController::mapOverlayStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void AppController::stationsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void AppController::sensorsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void AppController::chartPointsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void AppController::statsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}
QT_WARNING_POP
