/****************************************************************************
** Meta object code from reading C++ file 'myUi_First.h'
**
** Created: Tue Mar 13 13:53:21 2007
**      by: The Qt Meta Object Compiler version 59 (Qt 4.2.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "myUi_First.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'myUi_First.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.2.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_myUi_First[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x08,
      26,   11,   11,   11, 0x08,
      43,   40,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_myUi_First[] = {
    "myUi_First\0\0slotButton2()\0slotAction1()\0"
    "mi\0slotView(QModelIndex)\0"
};

const QMetaObject myUi_First::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_myUi_First,
      qt_meta_data_myUi_First, 0 }
};

const QMetaObject *myUi_First::metaObject() const
{
    return &staticMetaObject;
}

void *myUi_First::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_myUi_First))
	return static_cast<void*>(const_cast<myUi_First*>(this));
    return QObject::qt_metacast(_clname);
}

int myUi_First::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: slotButton2(); break;
        case 1: slotAction1(); break;
        case 2: slotView((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        }
        _id -= 3;
    }
    return _id;
}
