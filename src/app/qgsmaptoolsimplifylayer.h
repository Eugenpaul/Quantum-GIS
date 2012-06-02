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

#include <QVector>
#include "qgsfeature.h"

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


    typedef struct PolygonComplex
    {
      QVector<QgsPoint> boxTopLeft;
      QVector<QgsPoint> boxBottomRight;
      QVector<QgsPoint> breaks;
      QVector<QVector<QgsPoint> > searchPoints;
      QVector<QVector<int> > segments_id;
      int featureID;
    }PolygonComplex;

    QVector< QVector<QgsPoint> > mAllSegments;
    QVector<PolygonComplex> mPolygons;

    void simplify();

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
