#include "TempModel.h"

TempModel::TempModel( QObject* parent )
  :QAbstractTableModel( parent )
{
}

int TempModel::rowCount( const QModelIndex& parent ) const{
  if ( parent.isValid() ){
    return 0;
  }
  return _values.size();
}

int TempModel::columnCount( const QModelIndex& parent ) const{
  if ( parent.isValid() ){
    return 0;
  }
  return 2;
}

QVariant TempModel::data( const QModelIndex& index, int role ) const{
  if ( !index.isValid() || index.parent().isValid() ){
    return QVariant();
  }
  switch ( role ){
  case Qt::DisplayRole:
    switch ( index.column() ){
    case 0:
      return _iterators[ index.row() ].key();
    case 1:
      return _iterators[ index.row() ].value();
    }
    break;
  }
  return QVariant();
}

QVariant TempModel::headerData( int section, Qt::Orientation orientation, int role ) const{
  if ( orientation != Qt::Horizontal || role != Qt::DisplayRole ){
    return QVariant();
  }
  switch ( section ){
  case 0:
    return "Sensor";
  case 1:
    return "T";
  }
  return QVariant();
}

void TempModel::setValue( const QString& key, float value ){
  Values::iterator itr = _values.find( key );
  if ( itr == _values.end() ){
    _values.insert( key, value );
    _indicies.clear();
    _iterators.clear();
    int i = 0;
    for( Values::iterator itr = _values.begin(); itr != _values.end(); ++itr ){
      _iterators.insert( i, itr );
      _indicies.insert( itr.key(), i );
      ++i;
    }
    emit layoutChanged();
    return;
  }
  itr.value()= value;
  QModelIndex idx = createIndex( _indicies[ key ], 1 );
  emit dataChanged( idx, idx );
}
