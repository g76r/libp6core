/* Copyright 2015-2024 Hallowyn, Gregoire Barbier and others.
 * This file is part of libpumpkin, see <http://libpumpkin.g76r.eu/>.
 * Libpumpkin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libpumpkin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libpumpkin.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "thread/circularbuffer.h"
#include <QThread>
#include <QString>
#include <QtDebug>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>
#include "util/utf8string.h"

template <class T>
class PutterThread : public QThread {
  T _value;
  CircularBuffer<T> *_buffer;

public:
  PutterThread(T value, CircularBuffer<T> *buffer)
    : _value(value), _buffer(buffer) { }

protected:
  void run() {
    qDebug() << "running putter" << this << "with value" << _value;
    while (!isInterruptionRequested()) {
      detach(_value);
      _buffer->put(_value);
    }
    qDebug() << "finishing" << this;
  }
  void detach(T &value) {
    Q_UNUSED(value)
  }
};

template<>
void PutterThread<QString>::detach(QString &value) {
  value.detach();
}

template <class T>
class GetterThread : public QThread {
  CircularBuffer<T> *_buffer;

public:
  GetterThread(CircularBuffer<T> *buffer)
    : _buffer(buffer) { }

protected:
  void run() {
    qDebug() << "running getter" << this;
    long start = QDateTime::currentMSecsSinceEpoch();
    while (!isInterruptionRequested()) {
      T value;
      value = _buffer->get();
      if (_buffer->getCounter() % 1000000 == 0) {
        qDebug() << "total exchanges:" << _buffer->getCounter()
                 << "rate:"
                 << 1000.0*_buffer->getCounter()
                    /(QDateTime::currentMSecsSinceEpoch()-start)
                 << "exchanges/s"
                 << "latency:"
                 << 1000.0*(QDateTime::currentMSecsSinceEpoch()-start)
                    /_buffer->getCounter() << "us/exchange"
                 << "free:" << _buffer->free()
                 << "value:" << value;
      }
    }
    qDebug() << "finishing" << this;
  }
};

struct S1 {
  size_t _id;
  QList<QString> _strings;
};

QDebug operator<<(QDebug dbg, const S1 &s) {
  dbg << "{" << s._id << s._strings << "}";
  return dbg;
}

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  auto timer = new QTimer;
  timer->setSingleShot(true);
#if 1
  auto stringBuffer = new CircularBuffer<QString>(10);
  auto t11 = new PutterThread<QString>("This is a utf16 test string", stringBuffer);
  //auto t11 = new PutterThread<QString>("u16", stringBuffer);
  auto t12 = new GetterThread<QString>(stringBuffer);
  auto intBuffer = new CircularBuffer<int>(10);
  auto t21 = new PutterThread<int>(8086, intBuffer);
  auto t22 = new GetterThread<int>(intBuffer);
  auto utf8Buffer = new CircularBuffer<Utf8String>(10);
  auto t31 = new PutterThread<Utf8String>("This is a utf8 test string", utf8Buffer);
  //auto t31 = new PutterThread<Utf8String>("u8", utf8Buffer);
  auto t32 = new GetterThread<Utf8String>(utf8Buffer);
  auto sizeBuffer = new CircularBuffer<size_t>(10);
  auto t41 = new PutterThread<size_t>(8087, sizeBuffer);
  auto t42 = new GetterThread<size_t>(sizeBuffer);
  QObject::connect(timer, &QTimer::timeout, t11, &QThread::terminate);
  QObject::connect(timer, &QTimer::timeout, t12, &QThread::terminate);
  QObject::connect(timer, &QTimer::timeout, [&]() {
    timer->disconnect();
    QObject::connect(timer, &QTimer::timeout, t21, &QThread::terminate);
    QObject::connect(timer, &QTimer::timeout, t22, &QThread::terminate);
    QObject::connect(timer, &QTimer::timeout, [&]() {
      timer->disconnect();
      QObject::connect(timer, &QTimer::timeout, t31, &QThread::terminate);
      QObject::connect(timer, &QTimer::timeout, t32, &QThread::terminate);
      QObject::connect(timer, &QTimer::timeout, [&]() {
        timer->disconnect();
        QObject::connect(timer, &QTimer::timeout, t41, &QThread::terminate);
        QObject::connect(timer, &QTimer::timeout, t42, &QThread::terminate);
        QObject::connect(timer, &QTimer::timeout, &app, &QCoreApplication::quit);
        t42->start();
        t41->start();
        timer->start(5000);
      });
      t32->start();
      t31->start();
      timer->start(5000);
    });
    t22->start();
    t21->start();
    timer->start(5000);
  });
  t12->start();
  t11->start();
  timer->start(5000);
  app.exec();
  timer->disconnect();
  QThread::sleep(1);
#endif
  auto structBuffer = new CircularBuffer<S1>(10);
  QList<QThread*> threads;
  for (size_t i: {1,2,3,4}) {
    if (true) {
      QList<QString> strings = { "foo", "bar", "baz" };
      auto p = new PutterThread<S1>(S1{i, strings}, structBuffer);
      threads += p;
      p->setObjectName("p"+QString::number(i));
      QObject::connect(timer, &QTimer::timeout, p, &QThread::terminate);
    }
    if (true) {
      auto c = new GetterThread<S1>(structBuffer);
      threads += c;
      c->setObjectName("c"+QString::number(i));
      QObject::connect(timer, &QTimer::timeout, c, &QThread::terminate);
    }
  }
  QObject::connect(timer, &QTimer::timeout, &app, &QCoreApplication::quit);
  for (auto t: threads)
    t->start();
  timer->start(30'000);
  app.exec();
}
