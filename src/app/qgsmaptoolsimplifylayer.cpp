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
  while ( vlayer->nextFeature( f ) )
  {
    if (f.geometry()->type() == QGis::Polygon)
    {
      if (!f.geometry()->isMultipart())
      {
        QVector<QgsPoint> pts = getPointList( f );
        size = pts.size();
        if ( size >= 4 )
        {
          i++;
          PolygonComplex currentPolygon;
          ExtendedQgsPoint point;
          currentPolygon.searchPoints.append(QVector< ExtendedQgsPoint >());
          for (int j = 0; j < pts.size(); j++)
          {
            point = pts[j];
            currentPolygon.searchPoints[0].append(point);
          }
          currentPolygon.featureID = f.id();
          mPolygons.push_back(currentPolygon);
        }
      }
      else //if (f.geometry()->isMultipart())
      {
        QVector< QVector< QVector<QgsPoint> > > pts = getMultiPointList( f );
        PolygonComplex currentPolygon;
        int validPolygonsCounter = 0;
        for (i = 0; i < pts.size(); i++)
        {
          size = pts[i][0].size();
          if ( size >= 4 )
          {
            ExtendedQgsPoint point;
            currentPolygon.searchPoints.append(QVector< ExtendedQgsPoint >());
            for (int j = 0; j < pts[i][0].size(); j++)
            {
              //qDebug() << "CHECK 000";
              point = pts[i][0][j];
              currentPolygon.searchPoints[validPolygonsCounter].append(point);
              //qDebug() << "CHECK 111";
            }
            //qDebug() << "CHECK 222";
            validPolygonsCounter++;
          }
        }
        //qDebug() << "CHECK 333";
        mPolygons.push_back(currentPolygon);
        //qDebug() << "CHECK 444";
      }
    }
  }
  PolygonComplex currentPolygon;
  for (int j = 0; j < mPolygons.size(); j++)
  {
    for (int jj = 0; jj < mPolygons[j].searchPoints.size(); jj++) //parts in multipart polygon
    {
      mPolygons[j].searchPoints[jj].pop_back();
      mPolygons[j].boxBottomRight.append(QgsPoint(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()));
      mPolygons[j].boxTopLeft.append(QgsPoint(std::numeric_limits<double>::max(), std::numeric_limits<double>::max()));
      for (i = 0; i < mPolygons[j].searchPoints[jj].size(); i++)
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
      //qDebug() << "feature " << mPolygons[j].featureID << ": "<< mPolygons[j].boxTopLeft[jj].x() << ", " << mPolygons[j].boxTopLeft[jj].y() << "; "<< mPolygons[j].boxBottomRight[jj].x() << ", " << mPolygons[j].boxBottomRight[jj].y();
    }
  }
  /* searching for connected segments */
  for (i = 0; i < mPolygons.size() - 1; i++)
  {
    for (int j = i; j < mPolygons.size(); j++)
    {
      for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
      {
        while (mPolygons[i].segments_id.size() < parti + 1)
        {
          mPolygons[i].segments_id.append(QVector<int>());
        }
        for (int partj = ((i == j) ? parti + 1: 0 ); partj < mPolygons[j].searchPoints.size(); partj++)
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
              //qDebug() << "feature " << mPolygons[i].featureID << "part" << parti
//                << "of size" << mPolygons[i].searchPoints[parti].size() << ": "
//                << mPolygons[i].boxTopLeft[parti].x() << ", " << mPolygons[i].boxTopLeft[parti].y() << "; "
//                << mPolygons[i].boxBottomRight[parti].x() << ", " << mPolygons[i].boxBottomRight[parti].y() << " intersects "
//               << "feature " << mPolygons[j].featureID << "part" << partj
//                << "of size" << mPolygons[j].searchPoints[partj].size() << ": "
//                << mPolygons[j].boxTopLeft[partj].x() << ", " << mPolygons[j].boxTopLeft[partj].y() << "; "
//                << mPolygons[j].boxBottomRight[partj].x() << ", " << mPolygons[j].boxBottomRight[partj].y();
            bool found = false;
            bool firstfound = false;
            int way = 0;
            int startDeletei;
            int startDeletej;
            int tempi;
            int tempj;
            int sizeDelete;
            int tempIndex;
            int tempIndex2;
            int lastjj = -1;
            int lastii = -1;
            /* for every point in this part starting from first */
            for (int ii = 0; (ii < mPolygons[i].searchPoints[parti].size()) && (!found); ii++)
            {
              /* for every point in other part starting from last */
              if (!mPolygons[i].searchPoints[parti][ii].isBreakForward())
              for (int jj = mPolygons[j].searchPoints[partj].size() - 1; (jj >= 0) && (!found); jj--)
              {
                firstfound = false;
                /* found first connected point, then we need to check how to move
                 * clockwise or counterclockwise */
                if ((abs(mPolygons[i].searchPoints[parti][ii].x() - mPolygons[j].searchPoints[partj][jj].x()) < mThreshold) &&
                    (abs(mPolygons[i].searchPoints[parti][ii].y() - mPolygons[j].searchPoints[partj][jj].y()) < mThreshold) &&
                    (jj >= 0) && (ii < mPolygons[i].searchPoints[parti].size()))
                {
                  firstfound = true;
                  /* TODO found = true if there are more than 2 connected points */
                  // found = true;
                  startDeletei = ii;
                  startDeletej = jj;
                  sizeDelete = 0;
                }
                if (firstfound)
                {
                  way = 0;
                  /* check which way to move */
                  tempj = startDeletej - 1;
                  if (tempj < 0)
                  {
                    tempj = mPolygons[j].searchPoints[partj].size() - 1;
                  }
                  tempi = startDeletei + 1;
                  if (tempi == mPolygons[i].searchPoints[parti].size())
                  {
                    tempi = 0;
                  }
                  /* if true move i forward, j back */
                  if ((abs(mPolygons[i].searchPoints[parti][tempi].x() - mPolygons[j].searchPoints[partj][tempj].x()) < mThreshold) &&
                    (abs(mPolygons[i].searchPoints[parti][tempi].y() - mPolygons[j].searchPoints[partj][tempj].y()) < mThreshold) &&
                    (!(mPolygons[i].searchPoints[parti][startDeletei].isBreakBackward())))
                  {
                    found = true;
                    way = -1;
                  }

                  if (way == 0)
                  {
                    tempj = startDeletej + 1;
                    if (tempj == mPolygons[j].searchPoints[partj].size())
                    {
                      tempj = 0;
                    }
                    tempi = startDeletei + 1;
                    if (tempi == mPolygons[i].searchPoints[parti].size())
                    {
                      tempi = 0;
                    }

                    if ((abs(mPolygons[i].searchPoints[parti][tempi].x() - mPolygons[j].searchPoints[partj][tempj].x()) < mThreshold) &&
                      (abs(mPolygons[i].searchPoints[parti][tempi].y() - mPolygons[j].searchPoints[partj][tempj].y()) < mThreshold) &&
                      (!(mPolygons[i].searchPoints[parti][startDeletei].isBreakForward())))
                    {
                      found = true;
                      way == 1;
                    }
                  }
                  if (way != 0)
                  {
                    bool endOfSegment = false;
                    QVector<QgsPoint> temp;
                    temp.push_back(mPolygons[i].searchPoints[parti][ii]);
                    lastii = ii;
                    lastjj = jj;
                    qDebug() << "Found common point [" << ii <<"]["<< jj << "]" << mPolygons[i].searchPoints[parti][ii].x() << mPolygons[i].searchPoints[parti][ii].y() << "and" << mPolygons[j].searchPoints[partj][jj].x() << mPolygons[j].searchPoints[partj][jj].y();
                    do
                    {
                      ii++;
                      jj += way;
                      qDebug() << jj << "/" << mPolygons[j].searchPoints[partj].size();
                      if (jj < 0)
                      {
                        jj = mPolygons[j].searchPoints[partj].size() - 1;
                      }
                      else if (jj == mPolygons[j].searchPoints[partj].size())
                      {
                        jj = 0;
                      }
                      if (ii == mPolygons[i].searchPoints[parti].size())
                      {
                        ii = 0;
                      }
                      if ((abs(mPolygons[i].searchPoints[parti][ii].x() - mPolygons[j].searchPoints[partj][jj].x()) < mThreshold) &&
                            (abs(mPolygons[i].searchPoints[parti][ii].y() - mPolygons[j].searchPoints[partj][jj].y()) < mThreshold))
                      {
                        //qDebug() << "abs(" << mPolygons[i].searchPoints[parti][ii].x() <<"-"
                        //        << mPolygons[j].searchPoints[partj][jj].x() <<") ="<< abs(mPolygons[i].searchPoints[parti][ii].x() - mPolygons[j].searchPoints[partj][jj].x());
                        //qDebug() << "mThreshold" << mThreshold;
                        qDebug() << "Found common point [" << ii <<"]["<< jj << "]" << mPolygons[i].searchPoints[parti][ii].x() << mPolygons[i].searchPoints[parti][ii].y() << "and" << mPolygons[j].searchPoints[partj][jj].x() << mPolygons[j].searchPoints[partj][jj].y();
                        sizeDelete++;
                        temp.push_back(mPolygons[i].searchPoints[parti][ii]);
                        lastjj = jj;
                        lastii = ii;
                      }
                      else
                      {
                        endOfSegment = true;
                      }
                    }
                    while ((!endOfSegment) &&
                           (!mPolygons[i].searchPoints[parti][ii].isBreakForward()) &&
                           (!mPolygons[j].searchPoints[partj][jj].isBreak(way)) &&
                           (ii != startDeletei) && (jj != startDeletej));
                    mAllSegments.push_back(temp);
                    mPolygons[i].segments_id[parti].push_back(mAllSegments.size() - 1);
                    mPolygons[j].segments_id[partj].push_back(mAllSegments.size() - 1);
                    qDebug() << "found segment of size " << mAllSegments[mAllSegments.size() - 1].size();
                    /* case when segment could contains last and first point of polygons */
                    if ((ii != startDeletei) && (jj != startDeletej) &&
                        (startDeletei == 0) && (!mPolygons[i].searchPoints[parti][startDeletei].isBreakBackward()))
                    {
                      bool endExtraLoop = false;
                      do
                      {
                        tempi = startDeletei - 1;
                        if (tempi < 0)
                        {
                          tempi = mPolygons[i].searchPoints[parti].size() - 1;
                        }
                        if (way > 0)
                        {
                          tempj = startDeletej - 1;
                          if (tempj < 0)
                          {
                            tempj = mPolygons[j].searchPoints[partj].size() - 1;
                          }
                        }
                        else if (way < 0)
                        {
                          tempj = lastjj - 1;
                          if (tempj < 0)//if (tempj == mPolygons[j].searchPoints[partj].size())
                          {
                            tempj = mPolygons[j].searchPoints[partj].size() - 1;
                          }
                        }
                        if ((abs(mPolygons[i].searchPoints[parti][tempi].x() - mPolygons[j].searchPoints[partj][tempj].x()) < mThreshold)
                        && (abs(mPolygons[i].searchPoints[parti][tempi].y() - mPolygons[j].searchPoints[partj][tempj].y()) < mThreshold))
                        {
                          qDebug() << "Found backward common point [" << tempi <<"]["<< tempj << "]" << mPolygons[i].searchPoints[parti][tempi].x() << mPolygons[i].searchPoints[parti][tempi].y() << "and" << mPolygons[j].searchPoints[partj][tempj].x() << mPolygons[j].searchPoints[partj][tempj].y();
                          sizeDelete++;
                          temp.prepend(mPolygons[i].searchPoints[parti][tempi]);
                          startDeletei = tempi;
                          way > 0 ? startDeletej = tempj: lastjj = tempj;
                        }
                        else
                        {
                          endExtraLoop = true;
                        }
                      } while ((!mPolygons[i].searchPoints[parti][tempi].isBreakBackward()) && (!endExtraLoop));
                    }
                  }
                }
              }
              if ((found) && (way != 0) && (sizeDelete > 0))
              {
                  mPolygons[i].searchPoints[parti][startDeletei].setBreakForward(true);
                  qDebug() <<"set break i" << startDeletei << mPolygons[i].searchPoints[parti][startDeletei].x() << mPolygons[i].searchPoints[parti][startDeletei].y();
                  mPolygons[i].searchPoints[parti][lastii].setBreakBackward(true);
                  qDebug() <<"set break i" << lastii << mPolygons[i].searchPoints[parti][lastii].x() << mPolygons[i].searchPoints[parti][lastii].y();
                  if (way < 0)
                  {
                    tempIndex = lastjj;//(startDeletej - sizeDelete) >= 0 ? (startDeletej - sizeDelete) : (startDeletej - sizeDelete + mPolygons[j].searchPoints[partj].size());
                    tempIndex2 = startDeletej;
                  }
                  else if (way > 0)
                  {
                    tempIndex = startDeletej;
                    tempIndex2 = lastjj;
                  }
                  mPolygons[j].searchPoints[partj][tempIndex].setBreakForward(true);
                  mPolygons[j].searchPoints[partj][tempIndex2].setBreakBackward(true);
                  qDebug() << "set break j" << tempIndex << mPolygons[j].searchPoints[partj][tempIndex].x() << mPolygons[j].searchPoints[partj][tempIndex].y();
                  qDebug() << "set break j" << tempIndex2 << mPolygons[j].searchPoints[partj][tempIndex2].x() << mPolygons[j].searchPoints[partj][tempIndex2].y();
              }
              if ((found) && ( sizeDelete > 1))
              {
                  int deletesize = 0;
                  if (startDeletei + sizeDelete - 1 >= mPolygons[i].searchPoints[parti].size())
                  {
                    deletesize = mPolygons[i].searchPoints[parti].size() - startDeletei - 1;
                    qDebug() << "CHECK remove from i"<< deletesize << "starting from " << startDeletei + 1;
                    mPolygons[i].searchPoints[parti].remove(startDeletei + 1, deletesize);
                    deletesize = sizeDelete - deletesize - 1;
                    qDebug() << "CHECK remove from i"<< deletesize << "starting from 0";
                    mPolygons[i].searchPoints[parti].remove(0, deletesize);
                  }
                  else
                  {
                    qDebug() << "CHECK remove from i"<< sizeDelete - 1 << "starting from " << startDeletei + 1;
                    mPolygons[i].searchPoints[parti].remove(startDeletei + 1, sizeDelete - 1);
                  }
                  if (way < 0)
                  {
                    if (startDeletej - sizeDelete + 1 < 0)
                    {
                      deletesize = sizeDelete - startDeletej - 1;
                      qDebug() << "CHECK remove from j" << deletesize << "starting from " << startDeletej - sizeDelete + 1 + mPolygons[j].searchPoints[partj].size();
                      mPolygons[j].searchPoints[partj].remove(startDeletej - sizeDelete + 1 + mPolygons[j].searchPoints[partj].size(), deletesize);
                      deletesize = sizeDelete - deletesize - 1;
                      qDebug() << "CHECK remove from j" << deletesize << "starting from 0";
                      mPolygons[j].searchPoints[partj].remove(0, deletesize);
                    }
                    else
                    {
                      qDebug() << "CHECK remove from j" << sizeDelete - 1 << "starting from " << startDeletej - sizeDelete + 1;
                      mPolygons[j].searchPoints[partj].remove(startDeletej - sizeDelete + 1, sizeDelete - 1);
                    }
                  }
                  else if (way > 0)
                  {
                    if (startDeletej + sizeDelete - 1 >= mPolygons[j].searchPoints[partj].size())
                    {
                      deletesize = mPolygons[j].searchPoints[partj].size() - startDeletej - 1;
                      qDebug() << "CHECK remove from j"<< deletesize << "starting from " << startDeletej + 1;
                      mPolygons[j].searchPoints[partj].remove(startDeletej + 1, deletesize);
                      deletesize = sizeDelete - deletesize - 1;
                      qDebug() << "CHECK remove from i"<< deletesize << "starting from 0";
                      mPolygons[j].searchPoints[partj].remove(0, deletesize);
                    }
                    else
                    {
                      qDebug() << "CHECK remove from j"<< sizeDelete - 1 << "starting from " << startDeletej + 1;
                      mPolygons[j].searchPoints[partj].remove(startDeletej + 1, sizeDelete - 2);
                    }
                  }
                }
                if ((found) && (sizeDelete > 2))
                {
                  ii = 0;
                }
                found = false;
                //qDebug() << "i" << i << "j" << j << "parti" << parti << "partj" << partj << "ii" << ii;
            }
          }
        }
      }
        //mPolygons[i].
    }
    ////qDebug() << "polygon id: " << currentPolygon.featureID << " size: " << currentPolygon.searchPoints.size();
  }
  //qDebug() << "Searching common segments finished";
  /* push to segments remaining points */
  QVector<QgsPoint> temp;
  for (int i = 0; i < mPolygons.size(); i++)
  {
    for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
    {
      //qDebug() << "Searching segments in remaining points of " << i << "part" << parti;
      int lastj = 0;
      do
      {
        lastj--;
        if (lastj < 0)
        {
          lastj = mPolygons[i].searchPoints[parti].size() - 1;
        }
      } while (( !mPolygons[i].searchPoints[parti][lastj].isBreakBackward()) && (lastj != 0));
      int j = lastj;
      bool first_time = true;
      do
      {
        if ((mPolygons[i].searchPoints[parti][j].isBreakForward()) || ((j == lastj) && (!first_time)))
        {
          qDebug() << "Found last alone point [" << j <<"]" << mPolygons[i].searchPoints[parti][j].x() << mPolygons[i].searchPoints[parti][j].y();
          temp.push_back(mPolygons[i].searchPoints[parti][j]);
          //qDebug() << "Segment size found: " << temp.size();
          if (temp.size() > 1)
          {
            mAllSegments.push_back(temp);
            mPolygons[i].segments_id[parti].push_back(mAllSegments.size() - 1);
          }
          temp.clear();
        }
        else
        {
          first_time = false;
          qDebug() << "Found alone point [" << j <<"]" << mPolygons[i].searchPoints[parti][j].x() << mPolygons[i].searchPoints[parti][j].y();
          temp.push_back(mPolygons[i].searchPoints[parti][j]);
        }
        j++;
        if (j == mPolygons[i].searchPoints[parti].size())
        {
          j = 0;
        }
        if (j == lastj)
        {
          qDebug() << "Found last alone point [" << j <<"]" << mPolygons[i].searchPoints[parti][j].x() << mPolygons[i].searchPoints[parti][j].y();
          temp.push_back(mPolygons[i].searchPoints[parti][j]);
          //qDebug() << "Segment size found: " << temp.size();
          if (temp.size() > 1)
          {
            mAllSegments.push_back(temp);
            mPolygons[i].segments_id[parti].push_back(mAllSegments.size() - 1);
          }
          temp.clear();
        }
      } while (j != lastj);
    }
  }

  /* simplification */
  /*//qDebug() << "Start simplification for feature number" << i;
  for (int j = 0; j < mPolygons[i].segments_id.size(); j++)
  {
    if (mAllSegments[mPolygons[i].segments_id[j]].size() > 1)
    {
      mAllSegments[mPolygons[i].segments_id[j]].remove(1, mAllSegments[mPolygons[i].segments_id[j]].size() - 2);
    }
  }*/
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

  //qDebug() << "Finish simplification";
  for (int i = 0; i < mPolygons.size(); i++)
  {
    QVector< QVector < QgsPoint > > temp;
    for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
    {
      /* generating the polygon back */
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
        lastPoint = temp[parti][temp[parti].size() - 1];
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

  mCanvas->refresh();
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
