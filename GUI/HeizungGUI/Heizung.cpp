#include "Heizung.h"
#include "ui_Heizung.h"

Heizung::Heizung( QWidget* parent )
  :QMainWindow( parent )
  ,_ui( new Ui::Heizung() )
  ,_tempModel( new TempModel( parent ) )
{
  _ui->setupUi( this );
  _ui->tempList->setModel( _tempModel );
  _udpSocket.bind( 12888, QUdpSocket::ShareAddress );
  connect( &_udpSocket, SIGNAL( readyRead() ), this, SLOT( readPendingDatagrams() ) );
}

Heizung::~Heizung(){
  delete _ui;
  _ui = 0;
  delete _tempModel;
  _tempModel = 0;
}

void Heizung::readPendingDatagrams(){
  while ( _udpSocket.hasPendingDatagrams() ){
    QByteArray datagram;
    datagram.resize( _udpSocket.pendingDatagramSize() );
    QHostAddress sender;
    quint16 senderPort;
    _udpSocket.readDatagram( datagram.data(), datagram.size(), &sender, &senderPort );
    processDatagram( datagram );
  }
}

void Heizung::processDatagram( const QByteArray& datagram ){
  QString text( datagram );
  _ui->rawData->appendPlainText( text );
  QStringList parts = text.split( ' ' );
  if ( parts.empty() ){
    return;
  }
  switch ( parts[ 0 ][ 0 ].toLatin1() ){
  case 'T':
    if ( parts.size() == 3 ){
      _tempModel->setValue( parts[ 1 ], parts[ 2 ].toFloat() );
    }
    break;
  }
}
