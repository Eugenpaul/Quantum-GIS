/***************************************************************************
    qgstopologicallayer.h - QGIS Tools for vector geometry analysis
                             -------------------
    begin                : June 2012
    copyright            : (C) 2012 by Evgeniy Pashentsev
    email                : ugnpaul at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSTOPOLOGICALLAYERH
#define QGSTOPOLOGICALLAYERH
/*
#include "qgsgeometry.h"
#include "qgsvectorlayer.h"
*/
#include "qgsfeature.h"
#include "qgsextendedpoint.h"
#include <QProgressDialog>

class QgsTopologicalLayer
{
  public:

    bool addFeature( QgsFeature* feature);
    QVector<QgsPolyline> getSimplifiedPolyline( int id );
    bool analyzeTopology( double threshold, QProgressDialog * pd);
    bool simplify( double tolerance, QProgressDialog * pd);

  private:

    QVector<QVector< QVector<QgsPoint> > > getMultiPointList( QgsFeature *f );
    QVector<QgsPoint> getPointList( QgsFeature *f );

    typedef struct DividedPolygon
    {
      QVector < QgsPoint > boundingBoxMin;
      QVector < QgsPoint > boundingBoxMax;
      QVector < QVector < QgsExtendedPoint > > searchPoints;
      int featureID;
    } DividedPolygon;

    QVector< QVector < QgsPoint > > mSegments;
    QVector< DividedPolygon > mPolygons;

};

#endif