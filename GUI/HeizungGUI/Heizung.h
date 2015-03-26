#ifndef HEIZUNG_H
#define HEIZUNG_H

#include <QMainWindow>
#include <QUdpSocket>

#include "TempModel.h"

namespace Ui{
  class Heizung;
} // namespace Ui

class Heizung
  :public QMainWindow
{
  Q_OBJECT

public:
  explicit Heizung( QWidget* parent = 0 );
  ~Heizung();

private:
  void processDatagram( const QByteArray& datagram );

  Ui::Heizung* _ui;
  QUdpSocket _udpSocket;

  TempModel* _tempModel;
  TempModel* _controllerModel;

private slots:
  void readPendingDatagrams();

};

#endif // HEIZUNG_H
