/* Copyright 2015 Hallowyn and others.
 * This file is part of libqtssu, see <https://github.com/g76r/libqtssu>.
 * Libqtssu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * Libqtssu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * You should have received a copy of the GNU Affero General Public License
 * along with libqtssu.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "thread/circularbuffer.h"
#include <QThread>
#include <QString>
#include <QtDebug>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>

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
                 << "value:" << value;
      }
    }
    qDebug() << "finishing" << this;
  }
};

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  CircularBuffer<QString> *stringBuffer = new CircularBuffer<QString>(10);
  CircularBuffer<int> *intBuffer = new CircularBuffer<int>(10);
  PutterThread<QString> *t11 = new PutterThread<QString>(
        "This is a test string", stringBuffer);
  GetterThread<QString> *t12 = new GetterThread<QString>(stringBuffer);
  PutterThread<int> *t21 = new PutterThread<int>(8086, intBuffer);
  GetterThread<int> *t22 = new GetterThread<int>(intBuffer);
  QTimer timer1, timer2;
  QObject::connect(&timer1, SIGNAL(timeout()), t11, SLOT(terminate()));
  QObject::connect(&timer1, SIGNAL(timeout()), t12, SLOT(terminate()));
  QObject::connect(&timer1, SIGNAL(timeout()), t21, SLOT(start()));
  QObject::connect(&timer1, SIGNAL(timeout()), t22, SLOT(start()));
  QObject::connect(&timer2, SIGNAL(timeout()), t21, SLOT(terminate()));
  QObject::connect(&timer2, SIGNAL(timeout()), t22, SLOT(terminate()));
  QObject::connect(&timer2, SIGNAL(timeout()), &app, SLOT(quit()));
  timer1.start(5000);
  timer2.start(10000);
  t12->start();
  t11->start();
  app.exec();
}
