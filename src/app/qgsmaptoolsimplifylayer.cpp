/***************************************************************************
    qgsmaptoolsimplifylayer.cpp  - simplify the whole vector layer
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

#include "qgsmaptoolsimplifylayer.h"

#include "qgsgeometry.h"
#include "qgsmapcanvas.h"
#include "qgsvectorlayer.h"
#include "qgstolerance.h"
#include "qgstopologicallayer.h"

#include <QMouseEvent>
#include <QMessageBox>

#include <cmath>
#include <cfloat>
#include <limits>

QgsSimplifyLayerDialog::QgsSimplifyLayerDialog( QWidget* parent )
    : QDialog( parent )
{
  setupUi( this );
  connect( thresholdSpinBox, SIGNAL( valueChanged( double ) ),
           this, SLOT( thresholdChanged( double ) ) );
  connect( toleranceSpinBox, SIGNAL( valueChanged( double ) ),
           this, SLOT( toleranceChanged( double ) ) );
  connect( okButton, SIGNAL( clicked() ),
           this, SLOT( simplify() ) );

}

void QgsSimplifyLayerDialog::toleranceChanged( double value )
{
  emit toleranceChangedSignal( value );
}

void QgsSimplifyLayerDialog::thresholdChanged( double value )
{
  emit thresholdChangedSignal( value );
}

void QgsSimplifyLayerDialog::simplify()
{
  emit simplifySignal();
}

QgsMapToolSimplifyLayer::QgsMapToolSimplifyLayer( QgsMapCanvas* canvas )
    : QgsMapToolEdit( canvas )
{
  mSimplifyLayerDialog = new QgsSimplifyLayerDialog( canvas->topLevelWidget() );

  connect( mSimplifyLayerDialog, SIGNAL( toleranceChangedSignal( double ) ),
           this, SLOT( toleranceChanged( double ) ) );
  connect( mSimplifyLayerDialog, SIGNAL( thresholdChangedSignal( double ) ),
           this, SLOT( thresholdChanged( double ) ) );
  connect( mSimplifyLayerDialog, SIGNAL( simplifySignal() ),
           this, SLOT( simplify() ) );
}

QgsMapToolSimplifyLayer::~QgsMapToolSimplifyLayer()
{
}

double abs(double value)
{
  return (value > 0)? value: -value;
}

void QgsMapToolSimplifyLayer::simplify()
{
  QgsVectorLayer * vlayer = currentVectorLayer();
  QgsFeature f;
  int i, size;
  vlayer->select( QgsAttributeList() );
  QgsTopologicalLayer *tplayer = new QgsTopologicalLayer();
  while ( vlayer->nextFeature( f ) )
  {
    tplayer->addFeature( &f );
  }
  tplayer->analyzeTopology( mThreshold );
  tplayer->simplify( mTolerance );

  vlayer->select( QgsAttributeList() );

  QVector<QgsPolyline> poly;
  while ( vlayer->nextFeature( f ) )
  {
    poly = tplayer->getSimplifiedPolyline( f.id() );
    if (!poly.isEmpty())
    {
      //qDebug() << "CHECK set geometry for feature" << f.id();
      f.setGeometry( QgsGeometry::fromPolygon( poly ) );
      vlayer->beginEditCommand( tr( "Geometry simplified" ) );
      vlayer->changeGeometry( f.id(), f.geometry() );
      vlayer->endEditCommand();
      //qDebug() << "set geometry for feature" << f.id() << "done";
    }
  }

  return;
  //qDebug() << "Searching common segments finished";


  /* simplification */
  /*//qDebug() << "Start simplification for feature number" << i;
  for (int j = 0; j < mPolygons[i].segments_id.size(); j++)
  {
    if (mAllSegments[mPolygons[i].segments_id[j]].size() > 1)
    {
      mAllSegments[mPolygons[i].segments_id[j]].remove(1, mAllSegments[mPolygons[i].segments_id[j]].size() - 2);
    }
  }*/
  /*
  qDebug() << "Start simplification of " << mAllSegments.size() << "segments";
  for (int j = 0; j < mAllSegments.size(); j++)
  {
    qDebug() << "Segment" << j << "size" << mAllSegments[j].size();
    for (int pointi = 0; pointi < mAllSegments[j].size(); pointi++)
    {
      qDebug() << pointi << mAllSegments[j][pointi].x() << mAllSegments[j][pointi].y();
    }
    if (mAllSegments[j].size() > 3)
    {
      mAllSegments[j] = QgsSimplify::simplifyPoints(mAllSegments[j], mTolerance);
      //mAllSegments[j].remove(1, mAllSegments[j].size() - 2); //extreme simplification
    }
  }
  qDebug() << "CHECK after simplification: ";
  qDebug() << "###################################################################";
  for (int j = 0; j < mAllSegments.size(); j++)
  {
    qDebug() << "NEXT";
    for (int pointi = 0; pointi < mAllSegments[j].size(); pointi++)
    {
      qDebug() << mAllSegments[j][pointi].x() << mAllSegments[j][pointi].y();
    }
  }
  //qDebug() << "####################################################################";
  */
  //qDebug() << "Finish simplification";
  /*
  for (int i = 0; i < mPolygons.size(); i++)
  {
    QVector< QVector < QgsPoint > > temp;
    for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
    {
      // generating the polygon back
      qDebug() << "##########################################################";
      qDebug() << "Start generating polygon number" << i << "part" << parti;
      temp.append(QVector< QgsPoint > ());
      QgsPoint firstPoint;
      QgsPoint lastPoint;
      //qDebug() << "CHECK mPolygons size: " << mPolygons.size();
      //qDebug() << "CHECK i: " << i;
      //qDebug() << "CHECK segments_id size: " << mPolygons[i].segments_id[parti].size();
      //qDebug() << "CHECK first segment id: " << mPolygons[i].segments_id[0];
      //qDebug() << "CHECK mAllSegments size: " << mAllSegments.size();
      if (mPolygons[i].segments_id[parti].size() > 2)
      for (int j = 0; j < mPolygons[i].segments_id[parti].size(); j++)
      {
        qDebug() << "CHECK segment " << j;
        qDebug() << "first" << mAllSegments[mPolygons[i].segments_id[parti][j]][0].x() <<
                                                      mAllSegments[mPolygons[i].segments_id[parti][j]][0].y();
        qDebug() << "last"
          << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].x()
          << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].y();
      }
      firstPoint = mAllSegments[mPolygons[i].segments_id[parti][0]][0];
      //qDebug() << "First point added";
      for (int j = 0; j < mAllSegments[mPolygons[i].segments_id[parti][0]].size(); j++)
      {
        //qDebug() << "j = " << j;
        temp[parti].push_back(mAllSegments[mPolygons[i].segments_id[parti][0]][j]);
        qDebug() << "FIRST PUSH BACK: " << temp[parti][temp[parti].size() - 1].x() << temp[parti][temp[parti].size() - 1].y();
      }
      //qDebug() << "First segment added";
      lastPoint = temp[parti][temp[parti].size() - 1];
      int j = 1;
      int lastj = 0;
      int counter = 100;
      while (!((abs(firstPoint.x() - lastPoint.x()) < mThreshold) && (abs(firstPoint.y() - lastPoint.y()) < mThreshold)) && (j < mPolygons[i].segments_id[parti].size()) && (counter > 0))
      {
        counter--;
        //qDebug() << "CHECK 0";
        j = lastj + 1;
        if (j == mPolygons[i].segments_id[parti].size())
        {
          j = 0;
        }
        //if (mPolygons[i].segments_id.size() > 1)
        qDebug() << "Searching for " << lastPoint.x() << lastPoint.y();
        while ((j != lastj) &&
              (!((abs(mAllSegments[mPolygons[i].segments_id[parti][j]][0].x() - lastPoint.x()) < mThreshold) &&
              (abs(mAllSegments[mPolygons[i].segments_id[parti][j]][0].y() - lastPoint.y()) < mThreshold)) &&
              !((abs(mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].x() -lastPoint.x()) < mThreshold) &&
                (abs(mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].y() -lastPoint.y()) < mThreshold))))
        {
          qDebug() << "Not this " << j << mAllSegments[mPolygons[i].segments_id[parti][j]][0].x() << mAllSegments[mPolygons[i].segments_id[parti][j]][0].y();
          qDebug() << "And not this " << j << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].x() << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].y();
          j++;
          if (j == mPolygons[i].segments_id[parti].size())
          {
            j = 0;
          }
        }
        if (j == lastj)
        {
          qDebug() << "BREAK";
          break;
        }
        lastj = j;
        //qDebug() << "CHECK 1";
        //qDebug() << "CHECK found j = " << lastj;
        //qDebug() << "CHECK last point" << mAllSegments[mPolygons[i].segments_id[parti][j]][0].x() << mAllSegments[mPolygons[i].segments_id[parti][j]][0].y();
        //qDebug() << "CHECK first point" << firstPoint.x() << firstPoint.y();
        if ((abs(mAllSegments[mPolygons[i].segments_id[parti][j]][0].x() - lastPoint.x()) < mThreshold) &&
            (abs(mAllSegments[mPolygons[i].segments_id[parti][j]][0].y() - lastPoint.y()) < mThreshold))
        {
          for (int jj = 1; jj < mAllSegments[mPolygons[i].segments_id[parti][j]].size(); jj++)
          {
            temp[parti].push_back(mAllSegments[mPolygons[i].segments_id[parti][j]][jj]);
            qDebug() << "PUSH BACK  " << temp[parti][temp[parti].size() - 1].x() << temp[parti][temp[parti].size() - 1].y();
          }
        }
        else if ((abs(mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].x() - lastPoint.x()) < mThreshold) &&
          (abs(mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].y() - lastPoint.y()) < mThreshold))
        {
          for (int jj = mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 2; jj >=0; jj--)
          {
            temp[parti].push_back(mAllSegments[mPolygons[i].segments_id[parti][j]][jj]);
            qDebug() << "PUSH BACK  " << temp[parti][temp[parti].size() - 1].x() << temp[parti][temp[parti].size() - 1].y();
          }
        }
        /*else if (mAllSegments[mPolygons[i].segments_id[j]][mAllSegments[mPolygons[i].segments_id[j]].size() - 1] == lastPoint)
        {
          for (int jj = mAllSegments[mPolygons[i].segments_id[j]].size() - 1; jj > 1 ; jj--)
          {
            temp.push_back(mAllSegments[mPolygons[i].segments_id[j]][jj]);
          }
        }*/
        /*lastPoint = temp[parti][temp[parti].size() - 1];
      }
      if (temp[parti][temp[parti].size() - 1] != temp[parti][0])
      {
        temp[parti].push_back(temp[parti][0]);
      }
    }

      QVector<QgsPolyline> poly;
      //QgsVectorLayer * vlayer = currentVectorLayer();
      //vlayer->select(QgsAttributeList());

      QgsFeature f;

      vlayer->select( QgsAttributeList() );
      bool first = true;
      bool found = false;
      //qDebug() << "CHECK set geometry";
      while ( vlayer->nextFeature( f ) )
      {
        //qDebug() << "CHECK f.id(): " << f.id() << "mPolygons[" << i << "].featureID:"<< mPolygons[i].featureID;

        if (f.id() == mPolygons[i].featureID)
        {
          found = false;
          for (int parti = 0; parti < temp.size(); parti++)
          {
            //qDebug() << "CHECK temp[parti].size: " << temp[parti].size();
            if (temp[parti].size() < 4)
            {
              //temp.remove(parti);
              //parti--;
            }
            else
            {
              found = true;
              //qDebug() << "CHECK SIMPLIFIED FEATURE NUMBER " << f.id() << "PART" << parti << "LENGTH " << temp[parti].size();
              for (int iii = 0; iii < temp[parti].size(); iii++)
              {
                //qDebug() << temp[parti][iii].x() << temp[parti][iii].y();
              }
              poly.append(temp[parti]);
            }
          }
          if (found)
          {
            //qDebug() << "CHECK set geometry for feature" << f.id();
            f.setGeometry( QgsGeometry::fromPolygon( poly ) );
            vlayer->beginEditCommand( tr( "Geometry simplified" ) );
            vlayer->changeGeometry( f.id(), f.geometry() );
            vlayer->endEditCommand();
            //qDebug() << "set geometry for feature" << f.id() << "done";
          }
        }
      }
  }

  mCanvas->refresh();*/
}

void QgsMapToolSimplifyLayer::deactivate()
{
  QgsMapTool::deactivate();
}

void QgsMapToolSimplifyLayer::canvasPressEvent( QMouseEvent * e )
{
  mSimplifyLayerDialog->show();
  //simplify();
}

QVector<QgsPoint> QgsMapToolSimplifyLayer::getPointList( QgsFeature& f )
{
  QgsGeometry* line = f.geometry();
  if (( line->type() != QGis::Line && line->type() != QGis::Polygon ) || line->isMultipart() )
  {
    return QVector<QgsPoint>();
  }
  if (( line->type() == QGis::Line ) )
  {
    return line->asPolyline();
  }
  else
  {
    if ( line->asPolygon().size() > 1 )
    {
      return QVector<QgsPoint>();
    }
    return line->asPolygon()[0];
  }
}

QVector<QVector< QVector<QgsPoint> > > QgsMapToolSimplifyLayer::getMultiPointList( QgsFeature& f)
{
  QgsGeometry* geom = f.geometry();
  if (0)//(geom->type() != QGis::UnknownGeometry)//((geom->type() != QGis::WKBMultiPolygon) && (geom->type() != QGis::WKBMultiPolygon25D))
  {
    return QVector<QVector< QVector<QgsPoint> > >();
  }
  else
  {
    return geom->asMultiPolygon();
  }
}

bool QgsSimplify::simplifyLine( QgsFeature& lineFeature,  double tolerance )
{
  QgsGeometry* line = lineFeature.geometry();
  if ( line->type() != QGis::Line )
  {
    return false;
  }

  QVector<QgsPoint> resultPoints = simplifyPoints( line->asPolyline(), tolerance );
  lineFeature.setGeometry( QgsGeometry::fromPolyline( resultPoints ) );
  return true;
}

bool QgsSimplify::simplifyPolygon( QgsFeature& polygonFeature,  double tolerance )
{
  QgsGeometry* polygon = polygonFeature.geometry();
  if ( polygon->type() != QGis::Polygon )
  {
    return false;
  }

  QVector<QgsPoint> resultPoints = simplifyPoints( polygon->asPolygon()[0], tolerance );
  if (resultPoints.size() < 4)
  {
    return true;
  }
  //resultPoints.push_back(resultPoints[0]);
  QVector<QgsPolyline> poly;
  poly.append( resultPoints );
  polygonFeature.setGeometry( QgsGeometry::fromPolygon( poly ) );
  return true;
}


QVector<QgsPoint> QgsSimplify::simplifyPoints( const QVector<QgsPoint>& pts, double tolerance )
{
  //just safety precaution
  if ( tolerance < 0 )
    return pts;
  // Douglas-Peucker simplification algorithm

  //qDebug() << "CHECK simplify points: " << pts.size();
  int anchor  = 0;
  int floater = pts.size() - 1;

  QList<StackEntry> stack;
  StackEntry temporary;
  StackEntry entry = {anchor, floater};
  stack.append( entry );

  QSet<int> keep;
  double anchorX;
  double anchorY;
  double seg_len;
  double max_dist;
  int farthest;
  double dist_to_seg;
  double vecX;
  double vecY;

  while ( !stack.empty() )
  {
    temporary = stack.takeLast();
    anchor = temporary.anchor;
    floater = temporary.floater;
    // initialize line segment
    if ( pts[floater] != pts[anchor] )
    {
      anchorX = pts[floater].x() - pts[anchor].x();
      anchorY = pts[floater].y() - pts[anchor].y();
      seg_len = sqrt( anchorX * anchorX + anchorY * anchorY );
      // get the unit vector
      anchorX /= seg_len;
      anchorY /= seg_len;
    }
    else
    {
      anchorX = anchorY = seg_len = 0.0;
    }
    // inner loop:
    max_dist = 0.0;
    farthest = anchor + 1;
    for ( int i = anchor + 1; i < floater; i++ )
    {
      dist_to_seg = 0.0;
      // compare to anchor
      vecX = pts[i].x() - pts[anchor].x();
      vecY = pts[i].y() - pts[anchor].y();
      seg_len = sqrt( vecX * vecX + vecY * vecY );
      // dot product:
      double proj = vecX * anchorX + vecY * anchorY;
      if ( proj < 0.0 )
      {
        dist_to_seg = seg_len;
      }
      else
      {
        // compare to floater
        vecX = pts[i].x() - pts[floater].x();
        vecY = pts[i].y() - pts[floater].y();
        seg_len = sqrt( vecX * vecX + vecY * vecY );
        // dot product:
        proj = vecX * ( -anchorX ) + vecY * ( -anchorY );
        if ( proj < 0.0 )
        {
          dist_to_seg = seg_len;
        }
        else
        {  // calculate perpendicular distance to line (pythagorean theorem):
          dist_to_seg = sqrt( qAbs( seg_len * seg_len - proj * proj ) );
        }
        if ( max_dist < dist_to_seg )
        {
          max_dist = dist_to_seg;
          farthest = i;
        }
      }
    }
    if ( max_dist <= tolerance )
    { // # use line segment
      keep.insert( anchor );
      keep.insert( floater );
    }
    else
    {
      StackEntry s = {anchor, farthest};
      stack.append( s );

      StackEntry r = {farthest, floater};
      stack.append( r );
    }
  }

  QList<int> keep2 = keep.toList();
  qSort( keep2 );

  QVector<QgsPoint> result;
  QVector<QgsPoint> nullresult;
  int position;
  while ( !keep2.empty() )
  {
    position = keep2.takeFirst();
    result.append( pts[position] );
  }
  if (result.size() <4 )
  {
      //return nullresult;
  }
  //qDebug() << "Simplified size: " << result.size();
  return result;
}

void QgsMapToolSimplifyLayer::toleranceChanged( double tolerance )
{
  mTolerance = tolerance;
}

void QgsMapToolSimplifyLayer::thresholdChanged( double threshold )
{
  mThreshold = threshold;
}
