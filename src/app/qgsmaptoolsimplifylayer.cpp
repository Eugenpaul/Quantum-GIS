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
#include <QProgressDialog>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>

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
  QProgressDialog * pd = new QProgressDialog("Progress...", "Cancel", 0, 100);
  QVBoxLayout * layout = new QVBoxLayout;
  QWidget *  win = new QWidget;
  pd->setWindowModality(Qt::WindowModal);
  pd->setAutoClose( true );
  layout->addWidget(pd, Qt::AlignCenter);
  win->setLayout(layout);
  win->show();
  QgsFeature f;
  int i, size;
  vlayer->select( QgsAttributeList() );
  pd->setMaximum( vlayer->featureCount() );
  QgsTopologicalLayer *tplayer = new QgsTopologicalLayer();
  int featureCount = 0;
  pd->setLabel( new QLabel("Reading features...") );
  while ( vlayer->nextFeature( f ) )
  {
    pd->setValue( featureCount ++ );
    win->show();
    if (!tplayer->addFeature( &f ))
    {
      return;
    }
    if (pd->wasCanceled())
    {
      win->hide();
      return;
    }
  }

  if (!tplayer->analyzeTopology( mThreshold, pd ))
  {
    win->hide();
    return;
  }
  win->show();
  if (!tplayer->simplify( mTolerance, pd ))
  {
    win->hide();
    return;
  }
  win->show();

  vlayer->select( QgsAttributeList() );
  QVector<QgsPolyline> poly;
  pd->setLabel( new QLabel("Changing geometry...") );
  pd->setValue( 0 );
  vlayer->beginEditCommand( tr( "Geometry simplified" ) );
  featureCount = 0;
  pd->setMaximum( vlayer->featureCount() );
  while ( vlayer->nextFeature( f ) )
  {
    pd->setValue( featureCount ++ );
    win->show();
    poly = tplayer->getSimplifiedPolyline( f.id() );
    if (!poly.isEmpty())
    {
      //qDebug() << "CHECK set geometry for feature" << f.id();
      vlayer->changeGeometry( f.id(), QgsGeometry::fromPolygon( poly ) );
      //vlayer->deleteFeature( f.id() );
      //qDebug() << "set geometry for feature" << f.id() << "done";
    }
    else
    {
      //qDebug() << "CHECKCHECK" << f.id();
      //return;
      vlayer->deleteFeature( f.id() );
    }
    if (pd->wasCanceled())
    {
      win->hide();
      return;
    }
  }
  vlayer->endEditCommand();
  mCanvas->refresh();
  win->hide();
  return;
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
