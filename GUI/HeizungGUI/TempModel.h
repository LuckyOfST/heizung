#ifndef TEMPMODEL_H
#define TEMPMODEL_H

#include <QAbstractTableModel>

class TempModel
  :public QAbstractTableModel
{
  Q_OBJECT
public:
  explicit TempModel( QObject* parent = 0 );
  virtual int rowCount( const QModelIndex& parent ) const;
  virtual int columnCount( const QModelIndex& parent ) const;
  virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
  virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

  void setValue( const QString& key, float value, int age );

signals:

public slots:

private:
  mutable QMutex _m;

  typedef QMap<QString,float> Values;
  Values _values;

  typedef QVector<Values::iterator> Iterators;
  Iterators _iterators;

  typedef QMap<QString,int> Indicies;
  Indicies _indicies;

};

#endif // TEMPMODEL_H
