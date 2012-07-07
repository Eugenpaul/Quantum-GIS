/***************************************************************************
    qgsmaptoolsimplifylayer.h  - simplify the whole vector layer
    ---------------------
    begin                : May 2012
    copyright            : (C) 2012 by Evgeniy Pashentsev
    email                : ugnpaul at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPTOOLSIMPLIFYLAYER_H
#define QGSMAPTOOLSIMPLIFYLAYER_H

#include "qgsmaptooledit.h"
#include "ui_qgssimplifylayerdialog.h"

#include <QVector>
#include "qgsfeature.h"

class QgsSimplifyLayerDialog : public QDialog, private Ui::SimplifyLayerDialog
{
    Q_OBJECT

  public:

    QgsSimplifyLayerDialog( QWidget* parent = NULL );


  signals:

    void thresholdChangedSignal( double threshold );
    void toleranceChangedSignal( double tolerance );
    void simplifySignal();


  private slots:

    void thresholdChanged( double threshold );
    void toleranceChanged( double tolerance );
    void simplify();
};

/*class ExtendedQgsPoint: public QgsPoint
{
    bool m_breakForward;
    bool m_breakBackward;

  public:

    ExtendedQgsPoint() : QgsPoint(), m_breakForward( false ), m_breakBackward( false )
    {}

    ExtendedQgsPoint( double x, double y ) : QgsPoint( x, y ), m_breakForward( false ), m_breakBackward( false )
    {}

    bool isBreak(int way)
    {
      return way > 0 ? m_breakForward : m_breakBackward;
    }

    bool isBreakForward()
    {
      return m_breakForward;
    }

    bool isBreakBackward()
    {
      return m_breakBackward;
    }

    void setBreakForward(bool breakForward)
    {
      m_breakForward = breakForward;
    }

    void setBreakBackward(bool breakBackward)
    {
      m_breakBackward = breakBackward;
    }

    ExtendedQgsPoint & operator=( const QgsPoint & other )
    {
      this->setX( other.x() );
      this->setY( other.y() );
      m_breakForward = false;
      m_breakBackward = false;
      return *this;
    }

};*/

class QgsMapToolSimplifyLayer: public QgsMapToolEdit
{
    Q_OBJECT

  public:
    QgsMapToolSimplifyLayer( QgsMapCanvas* canvas );
    virtual ~QgsMapToolSimplifyLayer();

    void canvasPressEvent( QMouseEvent * e );
    void deactivate();

  private:

    QVector<QgsPoint> getPointList( QgsFeature& f );
    QVector<QVector<QVector<QgsPoint> > > getMultiPointList( QgsFeature& f);

    /*typedef struct PolygonComplex
    {
      QVector< QgsPoint > boxTopLeft;
      QVector< QgsPoint > boxBottomRight;
      QVector< QVector <ExtendedQgsPoint> > searchPoints;
      QVector< QVector <int> > segments_id;
      int featureID;
    } PolygonComplex;
*/
//    QVector< QVector<QgsPoint> > mAllSegments;
//    QVector<PolygonComplex> mPolygons;

    QgsSimplifyLayerDialog *mSimplifyLayerDialog;

    double mThreshold;
    double mTolerance;

  private slots:
    void simplify();
    void thresholdChanged( double threshold );
    void toleranceChanged( double tolerance );
};

/**
  Implementation of Douglas-Peucker simplification algorithm taken from qgsmaptoolsimplify
 */
class QgsSimplify
{
    /** structure for one entry in stack for simplification algorithm */
    struct StackEntry
    {
      int anchor;
      int floater;
    };

  public:
    /** simplify line feature with specified tolerance. Returns true on success */
    static bool simplifyLine( QgsFeature &lineFeature, double tolerance );
    /** simplify polygon feature with specified tolerance. Returns true on success */
    static bool simplifyPolygon( QgsFeature &polygonFeature, double tolerance );
    /** simplify a line given by a vector of points and tolerance. Returns simplified vector of points */
    static QVector<QgsPoint> simplifyPoints( const QVector<QgsPoint>& pts, double tolerance );


};

#endif
