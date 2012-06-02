/***************************************************************************
    qgsmaptoolsimplifylayer.cpp  - simplify the whole vector layer
    ---------------------
    begin                : May 2012
    copyright            : (C) 2012 by Evgeniy Pashentsev
    email                : ugnpaul at mail dot com
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

#include <QMouseEvent>
#include <QMessageBox>

#include <cmath>
#include <cfloat>
#include <limits>


QgsMapToolSimplifyLayer::QgsMapToolSimplifyLayer( QgsMapCanvas* canvas )
    : QgsMapToolEdit( canvas )
{
}

QgsMapToolSimplifyLayer::~QgsMapToolSimplifyLayer()
{
}

void QgsMapToolSimplifyLayer::simplify()
{
  QgsVectorLayer * vlayer = currentVectorLayer();
  QgsFeature f;
  int i;
  vlayer->select( QgsAttributeList() );
  while ( vlayer->nextFeature( f ) )
  {
    if (((f.geometry()->type() == QGis::Line) || (f.geometry()->type() == QGis::Polygon)) && (!f.geometry()->isMultipart()))
    {
      QVector<QgsPoint> pts = getPointList( f );
      int size = pts.size();
      if (( f.geometry()->type() == QGis::Line && size >= 2 ) || ( f.geometry()->type() == QGis::Polygon && size >= 4 ))
      {
        i++;
        PolygonComplex currentPolygon;
        currentPolygon.searchPoints.append(pts);
        currentPolygon.featureID = f.id();
        mPolygons.push_back(currentPolygon);
        /*qDebug() << "CHECK simplify feature " << i << " size: " << pts.size();
        if ( f.geometry()->type() == QGis::Line )
        {
          QgsSimplifyLayer::simplifyLine( f, mTolerance );
        }
        else
        {
          QgsSimplifyLayer::simplifyPolygon( f, mTolerance );
        }
        vlayer->beginEditCommand( tr( "Geometry simplified" ) );
        vlayer->changeGeometry( f.id(), f.geometry() );
        vlayer->endEditCommand();*/
      }
    }
    else if (f.geometry()->isMultipart())//if ((f.geometry()->type() == QGis::WKBMultiPolygon) || (f.geometry()->type() == QGis::WKBMultiPolygon25D))
    {
      qDebug() << "CHECK MULTI  outside";
      //currentPolygon.searchPoints.append(QVector<QgsPoint>());
      QVector< QVector< QVector<QgsPoint> > > pts = getMultiPointList( f );
      PolygonComplex currentPolygon;
      for (int i = 0; i < pts.size(); i++)
      {
        qDebug() << "CHECK MULTI inside";
        int size = pts[i][0].size();
        if (size >= 4 )
        {
          currentPolygon.searchPoints.append(pts[i][0]);
          currentPolygon.featureID = f.id();
        }
      }
      qDebug() << "CHECK push_back";
      mPolygons.push_back(currentPolygon);
      qDebug() << "CHECK push_back done";
    }
  }
  PolygonComplex currentPolygon;
  for (int j = 0; j < mPolygons.size(); j++)
  {
    qDebug() << "CHECK j" << j;
    for (int jj = 0; jj < mPolygons[j].searchPoints.size(); jj++) //parts in multipart polygon
    {
      qDebug() << "CHECK j:" << j << "jj" << jj;
      mPolygons[j].boxBottomRight.append(QgsPoint(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()));
      mPolygons[j].boxTopLeft.append(QgsPoint(std::numeric_limits<double>::max(), std::numeric_limits<double>::max()));
      for (int i = 0; i < mPolygons[j].searchPoints[jj].size(); i++)
      {
        if (mPolygons[j].searchPoints[jj][i].x() < mPolygons[j].boxTopLeft[jj].x())
        {
          mPolygons[j].boxTopLeft[jj].setX(mPolygons[j].searchPoints[jj][i].x());
        }
        if (mPolygons[j].searchPoints[jj][i].x() > mPolygons[j].boxBottomRight[jj].x())
        {
          mPolygons[j].boxBottomRight[jj].setX(mPolygons[j].searchPoints[jj][i].x());
        }
        if (mPolygons[j].searchPoints[jj][i].y() < mPolygons[j].boxTopLeft[jj].y())
        {
          mPolygons[j].boxTopLeft[jj].setY(mPolygons[j].searchPoints[jj][i].y());
        }
        if (mPolygons[j].searchPoints[jj][i].y() > mPolygons[j].boxBottomRight[jj].y())
        {
          mPolygons[j].boxBottomRight[jj].setY(mPolygons[j].searchPoints[jj][i].y());
        }
      }
      qDebug() << "feature " << mPolygons[j].featureID << ": "
              << mPolygons[j].boxTopLeft[jj].x() << ", " << mPolygons[j].boxTopLeft[jj].y() << "; "
              << mPolygons[j].boxBottomRight[jj].x() << ", " << mPolygons[j].boxBottomRight[jj].y();
    }
  }
  /* searching for connected segments */
  for (int i = 0; i < mPolygons.size() - 1; i++)
  {
    for (int j = i + 1; j < mPolygons.size(); j++)
    {
      for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
      {
        while (mPolygons[i].segments_id.size() < parti + 1)
        {
          mPolygons[i].segments_id.append(QVector<int>());
        }
        for (int partj = 0; partj < mPolygons[j].searchPoints.size(); partj++)
        {
          while (mPolygons[j].segments_id.size() < partj + 1)
          {
            mPolygons[j].segments_id.append(QVector<int>());
          }
          if (((mPolygons[i].boxTopLeft[parti].x() - mPolygons[j].boxBottomRight[partj].x())*
              (mPolygons[i].boxBottomRight[parti].x() - mPolygons[j].boxTopLeft[partj].x()) <= 0) &&
              ((mPolygons[i].boxTopLeft[parti].y() - mPolygons[j].boxBottomRight[partj].y())*
              (mPolygons[i].boxBottomRight[parti].y() - mPolygons[j].boxTopLeft[partj].y()) <= 0))
          {
              qDebug() << "feature " << mPolygons[i].featureID << "part" << parti
                << "of size" << mPolygons[i].searchPoints[parti].size() << ": "
                << mPolygons[i].boxTopLeft[parti].x() << ", " << mPolygons[i].boxTopLeft[parti].y() << "; "
                << mPolygons[i].boxBottomRight[parti].x() << ", " << mPolygons[i].boxBottomRight[parti].y() << " intersects "
                << "feature " << mPolygons[j].featureID << "part" << partj
                << "of size" << mPolygons[j].searchPoints[partj].size() << ": "
                << mPolygons[j].boxTopLeft[partj].x() << ", " << mPolygons[j].boxTopLeft[partj].y() << "; "
                << mPolygons[j].boxBottomRight[partj].x() << ", " << mPolygons[j].boxBottomRight[partj].y();
            bool found = false;
            int startDeletei;
            int startDeletej;
            int sizeDelete;
            int tempIndex;
            float threshold = 0.01;
            for (int ii = 0; (ii < mPolygons[i].searchPoints[parti].size()) && (!found); ii++)
            {
              for (int jj = mPolygons[j].searchPoints[partj].size() - 1; (jj >= 0) && (!found); jj--)
              {
                if ((abs(mPolygons[i].searchPoints[parti][ii].x() - mPolygons[j].searchPoints[partj][jj].x()) < threshold) &&
                    (abs(mPolygons[i].searchPoints[parti][ii].y() - mPolygons[j].searchPoints[partj][jj].y()) < threshold) &&
                    (mPolygons[i].breaks.indexOf(mPolygons[i].searchPoints[parti][ii]) == -1) &&
                    (mPolygons[j].breaks.indexOf(mPolygons[j].searchPoints[partj][jj]) == -1) &&
                    (jj >= 0) && (ii < mPolygons[i].searchPoints[parti].size()))
                {
                  found = true;
                  startDeletei = ii;
                  startDeletej = jj;
                  sizeDelete = 0;
                  QVector<QgsPoint> temp;
                  do
                  {
                    sizeDelete++;
                    temp.push_back(mPolygons[i].searchPoints[parti][ii]);
                    qDebug() << "Found common point [" << ii <<"]["<< jj << "]" << mPolygons[i].searchPoints[parti][ii].x() << mPolygons[i].searchPoints[parti][ii].y();
                    ii++;
                    jj--;
                    if (jj < 0)
                    {
                      jj = mPolygons[j].searchPoints[partj].size() - 1;
                    }
                    if (ii == mPolygons[i].searchPoints[parti].size())
                    {
                      ii = 0;
                    }
                    /*if ((jj == 0) || (ii == mPolygons[i].searchPoints.size()))
                    {
                      break;
                    }*/
                  }
                  while ((abs(mPolygons[i].searchPoints[parti][ii].x() - mPolygons[j].searchPoints[partj][jj].x()) < threshold) &&
                          (abs(mPolygons[i].searchPoints[parti][ii].y() - mPolygons[j].searchPoints[partj][jj].y()) < threshold) &&
                          (mPolygons[i].breaks.indexOf(mPolygons[i].searchPoints[parti][ii]) == -1) &&
                          (mPolygons[j].breaks.indexOf(mPolygons[j].searchPoints[partj][jj]) == -1) &&
                          (ii != startDeletei) && (jj != startDeletej));
                  mAllSegments.push_back(temp);
                  mPolygons[i].segments_id[parti].push_back(mAllSegments.size() - 1);
                  mPolygons[j].segments_id[partj].push_back(mAllSegments.size() - 1);
                  qDebug() << "found segment of size " << mAllSegments[mAllSegments.size() - 1].size();
                }
              }
              if (found)
              {
                  mPolygons[i].breaks.push_back(mPolygons[i].searchPoints[parti][startDeletei]);
                  qDebug() << "CHECK break i = " << mPolygons[i].breaks[mPolygons[i].breaks.size() - 1].x() << mPolygons[i].breaks[mPolygons[i].breaks.size() - 1].y();
                  tempIndex = (startDeletej - sizeDelete) >= 0 ? (startDeletej - sizeDelete) : (startDeletej - sizeDelete + mPolygons[j].searchPoints[partj].size());
                  mPolygons[j].breaks.push_back(mPolygons[j].searchPoints[partj][tempIndex]);
                  qDebug() << "CHECK break j = " << mPolygons[j].breaks[mPolygons[j].breaks.size() - 1].x() << mPolygons[j].breaks[mPolygons[j].breaks.size() - 1].y();
              }
              if ((found) && ( sizeDelete > 2))
              {
                  int deletesize = 0;
                  if (startDeletei + sizeDelete - 1 >= mPolygons[i].searchPoints[parti].size())
                  {
                    deletesize = mPolygons[i].searchPoints[parti].size() - startDeletei - 1;
                    qDebug() << "CHECK remove from i"<< deletesize << "starting from " << startDeletei + 1;
                    mPolygons[i].searchPoints[parti].remove(startDeletei + 1, deletesize);
                    deletesize = deletesize - sizeDelete;
                    qDebug() << "CHECK remove from i"<< deletesize << "starting from 0";
                    mPolygons[i].searchPoints[parti].remove(0, deletesize);
                  }
                  else
                  {
                    qDebug() << "CHECK remove from i"<< sizeDelete - 2 << "starting from " << startDeletei + 1;
                    mPolygons[i].searchPoints[parti].remove(startDeletei + 1, sizeDelete - 2);
                  }
                  if (startDeletej - sizeDelete + 2 < 0)
                  {
                    deletesize = sizeDelete - startDeletej - 2;
                    qDebug() << "CHECK remove from j" << deletesize << "starting from " << startDeletej - sizeDelete + 2 + mPolygons[j].searchPoints[partj].size();
                    mPolygons[j].searchPoints[partj].remove(startDeletej - sizeDelete + 2 + mPolygons[j].searchPoints[partj].size(), deletesize);
                    deletesize = sizeDelete - deletesize - 2;
                    qDebug() << "CHECK remove from j" << deletesize << "starting from 0";
                    mPolygons[j].searchPoints[partj].remove(0, deletesize);
                  }
                  else
                  {
                    qDebug() << "CHECK remove from j" << sizeDelete - 2 << "starting from " << startDeletej - sizeDelete + 2;
                    mPolygons[j].searchPoints[partj].remove(startDeletej - sizeDelete + 2, sizeDelete - 2);
                  }
                }
                if ((found) && (sizeDelete > 2))
                {
                  ii = 0;
                  found = false;
                }
            }
          }
        }
      }
        //mPolygons[i].
    }
    //qDebug() << "polygon id: " << currentPolygon.featureID << " size: " << currentPolygon.searchPoints.size();
  }
  qDebug() << "Searching common segments finished";
  /* push to segments remaining points */
  QVector<QgsPoint> temp;
  for (int i = 0; i < mPolygons.size(); i++)
  {
    for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
    {
      qDebug() << "Searching segments in remaining points of " << i << "part" << parti;
      for (int j = 0; j < mPolygons[i].searchPoints[parti].size(); j++)
      {
        if ((mPolygons[i].breaks.indexOf(mPolygons[i].searchPoints[parti][j]) != -1) || (j == mPolygons[i].searchPoints[parti].size() - 1))
        {
          temp.push_back(mPolygons[i].searchPoints[parti][j]);
          qDebug() << "Segment size found: " << temp.size();
          if (temp.size() > 1)
          {
            mAllSegments.push_back(temp);
            mPolygons[i].segments_id[parti].push_back(mAllSegments.size() - 1);
          }
          temp.clear();
        }
        else
        {
          temp.push_back(mPolygons[i].searchPoints[parti][j]);
        }
      }
    }
  }

  /* simplification */
  /*qDebug() << "Start simplification for feature number" << i;
  for (int j = 0; j < mPolygons[i].segments_id.size(); j++)
  {
    if (mAllSegments[mPolygons[i].segments_id[j]].size() > 1)
    {
      mAllSegments[mPolygons[i].segments_id[j]].remove(1, mAllSegments[mPolygons[i].segments_id[j]].size() - 2);
    }
  }*/
  qDebug() << "Start simplification";
  for (int j = 0; j < mAllSegments.size(); j++)
  {
    if (mAllSegments[j].size() > 1)
    {
      mAllSegments[j].remove(1, mAllSegments[j].size() - 2); //extreme simplification
    }
  }

  qDebug() << "Finish simplification";
  for (int i = 0; i < mPolygons.size(); i++)
  {
    QVector< QVector < QgsPoint > > temp;
    for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
    {
      /* generating the polygon back */
      qDebug() << "Start generating polygon number" << i << "part" << parti;
      temp.append(QVector< QgsPoint > ());
      QgsPoint firstPoint;
      QgsPoint lastPoint;
      qDebug() << "CHECK mPolygons size: " << mPolygons.size();
      qDebug() << "CHECK i: " << i;
      qDebug() << "CHECK segments_id size: " << mPolygons[i].segments_id[parti].size();
      //qDebug() << "CHECK first segment id: " << mPolygons[i].segments_id[0];
      qDebug() << "CHECK mAllSegments size: " << mAllSegments.size();
      for (int j = 0; j < mPolygons[i].segments_id[parti].size(); j++)
      {
        qDebug() << "CHECK segment " << j << "first" << mAllSegments[mPolygons[i].segments_id[parti][j]][0].x() <<
                                                      mAllSegments[mPolygons[i].segments_id[parti][j]][0].y();
        qDebug() << "last"
          << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].x()
          << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].y();
      }
      firstPoint = mAllSegments[mPolygons[i].segments_id[parti][0]][0];
      qDebug() << "First point added";
      for (int j = 0; j < mAllSegments[mPolygons[i].segments_id[parti][0]].size(); j++)
      {
        //qDebug() << "j = " << j;
        temp[parti].push_back(mAllSegments[mPolygons[i].segments_id[parti][0]][j]);
        qDebug() << "PUSH BACK: " << temp[parti][temp[parti].size() - 1].x() << temp[parti][temp[parti].size() - 1].y();
      }
      qDebug() << "First segment added";
      lastPoint = temp[parti][temp.size() - 1];
      int j = 1;
      int lastj = 0;
      int counter = 20;
      while ((firstPoint != lastPoint) && (j < mPolygons[i].segments_id[parti].size()) && (counter > 0))
      {
        counter--;
        qDebug() << "CHECK 0";
        j = lastj + 1;
        //if (mPolygons[i].segments_id.size() > 1)
        qDebug() << "Searching for " << lastPoint.x() << lastPoint.y();
        while ((j < mPolygons[i].segments_id[parti].size()) &&
              (mAllSegments[mPolygons[i].segments_id[parti][j]][0] != lastPoint) &&
              (mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1] != lastPoint))
        {
          qDebug() << "Not this " << j << mAllSegments[mPolygons[i].segments_id[parti][j]][0].x() << mAllSegments[mPolygons[i].segments_id[parti][j]][0].y();
          qDebug() << "And not this " << j << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].x() << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].y();
          j++;
        }
        qDebug() << "CHECK 0.5";
        if (j == mPolygons[i].segments_id[parti].size())
        {
          j = 0;
          while ((j < lastj) &&
              (mAllSegments[mPolygons[i].segments_id[parti][j]][0] != lastPoint) &&
              (mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1] != lastPoint))
          {
            qDebug() << "Not this " << j << mAllSegments[mPolygons[i].segments_id[parti][j]][0].x() << mAllSegments[mPolygons[i].segments_id[parti][j]][0].y();
            qDebug() << "And not this " << j << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].x() << mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1].y();
            j++;
          }
          if (j == lastj)
          {
            qDebug() << "BREAK";
            break;
          }
        }
        lastj = j;
        qDebug() << "CHECK 1";
        qDebug() << "CHECK found j = " << lastj;
        qDebug() << "CHECK last point" << mAllSegments[mPolygons[i].segments_id[parti][j]][0].x() << mAllSegments[mPolygons[i].segments_id[parti][j]][0].y();
        qDebug() << "CHECK first point" << firstPoint.x() << firstPoint.y();
        if (mAllSegments[mPolygons[i].segments_id[parti][j]][0] == lastPoint)
        {
          for (int jj = 1; jj < mAllSegments[mPolygons[i].segments_id[parti][j]].size(); jj++)
          {
            temp[parti].push_back(mAllSegments[mPolygons[i].segments_id[parti][j]][jj]);
            qDebug() << "PUSH BACK  " << temp[parti][temp[parti].size() - 1].x() << temp[parti][temp[parti].size() - 1].y();
          }
        }
        else if (mAllSegments[mPolygons[i].segments_id[parti][j]][mAllSegments[mPolygons[i].segments_id[parti][j]].size() - 1] == lastPoint)
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
        lastPoint = temp[parti][temp[parti].size() - 1];
      }
    }
      QVector<QgsPolyline> poly;
      //QgsVectorLayer * vlayer = currentVectorLayer();
      //vlayer->select(QgsAttributeList());

      QgsFeature f;

      vlayer->select( QgsAttributeList() );
      bool first = true;
      bool found = false;
      qDebug() << "CHECK set geometry";
      while ( vlayer->nextFeature( f ) )
      {
        if (f.id() == mPolygons[i].featureID)
        {
          found = false;
          for (int parti = 0; parti < temp.size(); parti++)
          {
            if (temp[parti].size() < 4)
            {
              //temp.remove(parti);
              //parti--;
            }
            else
            {
              found = true;
              qDebug() << "CHECK SIMPLIFIED FEATURE NUMBER " << f.id() << "PART" << parti << "LENGTH " << temp[parti].size();
              for (int iii = 0; iii < temp[parti].size(); iii++)
              {
                qDebug() << temp[parti][iii].x() << temp[parti][iii].y();
              }
              poly.append(temp[parti]);
            }
          }
          if (found)
          {
            qDebug() << "CHECK set geometry for feature" << f.id();
            f.setGeometry( QgsGeometry::fromPolygon( poly ) );
            vlayer->beginEditCommand( tr( "Geometry simplified" ) );
            vlayer->changeGeometry( f.id(), f.geometry() );
            vlayer->endEditCommand();
            qDebug() << "set geometry for feature" << f.id() << "done";
            break;
          }
        }
      }
  }

  mCanvas->refresh();
}

void QgsMapToolSimplifyLayer::deactivate()
{
  QgsMapTool::deactivate();
}

void QgsMapToolSimplifyLayer::canvasPressEvent( QMouseEvent * e )
{
  simplify();
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

  qDebug() << "CHECK simplify points: " << pts.size();
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
      return nullresult;
  }
  qDebug() << "Simplified size: " << result.size();
  return result;
}
