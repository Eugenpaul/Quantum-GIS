/***************************************************************************
    qgstopologicallayer.cpp - QGIS Tools for vector geometry analysis
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

#include "qgstopologicallayer.h"
#include "qgsgeometry.h"
#include "qgssimplify.h"
#include <QLabel>

#include <QDebug>
#include <cmath>
#include <cfloat>
#include <limits>


bool QgsTopologicalLayer::addFeature( QgsFeature* feature)
{
  int size;
  if (feature->geometry()->type() == QGis::Polygon)
  {
    qDebug() << "check OK";
    if (!feature->geometry()->isMultipart())
    {
      qDebug() << "check not multipart";
      QVector<QgsPoint> pts = getPointList( feature );
      if ( pts.size() >= 4 )
      {
        DividedPolygon currentPolygon;
        QgsExtendedPoint point;
        currentPolygon.searchPoints.append(QVector< QgsExtendedPoint >());
        currentPolygon.boundingBoxMax.append(QgsPoint(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()));
        currentPolygon.boundingBoxMin.append(QgsPoint(std::numeric_limits<double>::max(), std::numeric_limits<double>::max()));
        for (int j = 1; j < pts.size(); j++) // starting from 1 because we don't need the similar points
        {
          point = pts[j];
          qDebug() << "CHECK add to " << feature->id() << point.x() << point.y();
          currentPolygon.searchPoints[0].append(point);
          if (point.x() < currentPolygon.boundingBoxMin[0].x())
          {
            currentPolygon.boundingBoxMin[0].setX(point.x());
          }
          if (point.x() > currentPolygon.boundingBoxMax[0].x())
          {
            currentPolygon.boundingBoxMax[0].setX(point.x());
          }
          if (point.y() < currentPolygon.boundingBoxMin[0].y())
          {
            currentPolygon.boundingBoxMin[0].setY(point.y());
          }
          if (point.y() > currentPolygon.boundingBoxMax[0].y())
          {
            currentPolygon.boundingBoxMax[0].setY(point.y());
          }
        }
        currentPolygon.featureID = feature->id();
        mPolygons.push_back(currentPolygon);
      }
    }
    else //if (f.geometry()->isMultipart())
    {
      qDebug() << "check OK 2";
      QVector< QVector< QVector<QgsPoint> > > pts = getMultiPointList( feature );
      DividedPolygon currentPolygon;
      int validPolygonsCounter = 0; //counter of parts in multipart feature
      for (int i = 0; i < pts.size(); i++)
      {
        size = pts[i][0].size();
        qDebug() << "size = " << size;
        /*//if (pts[i].size() > 1)
        {
          for (int j = 0; j < pts[i].size(); j++)
          {
            for (int jj = 0; jj < pts[i][j].size(); jj++)
            {
              qDebug() << i << j << jj << pts[i][j][jj].x() << pts[i][j][jj].y();
            }
          }
          return false;
        }*/
        if ( size >= 4 )
        {
          QgsExtendedPoint point;
          currentPolygon.searchPoints.append(QVector< QgsExtendedPoint >());
          currentPolygon.boundingBoxMax.append(QgsPoint(-std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()));
          currentPolygon.boundingBoxMin.append(QgsPoint(std::numeric_limits<double>::max(), std::numeric_limits<double>::max()));
          for (int j = 1; j < size; j++)
          {
            point = pts[i][0][j];
            qDebug() << "CHECK add to " << feature->id() << point.x() << point.y();
            currentPolygon.searchPoints[validPolygonsCounter].append(point);
            if (point.x() < currentPolygon.boundingBoxMin[validPolygonsCounter].x())
            {
              currentPolygon.boundingBoxMin[validPolygonsCounter].setX(point.x());
            }
            if (point.x() > currentPolygon.boundingBoxMax[validPolygonsCounter].x())
            {
              currentPolygon.boundingBoxMax[validPolygonsCounter].setX(point.x());
            }
            if (point.y() < currentPolygon.boundingBoxMin[validPolygonsCounter].y())
            {
              currentPolygon.boundingBoxMin[validPolygonsCounter].setY(point.y());
            }
            if (point.y() > currentPolygon.boundingBoxMax[validPolygonsCounter].y())
            {
              currentPolygon.boundingBoxMax[validPolygonsCounter].setY(point.y());
            }
          }
          validPolygonsCounter++;
        }
      }
      currentPolygon.featureID = feature->id();
      mPolygons.push_back(currentPolygon);
    }
  }
  else
  {
    qDebug() << "type: " << feature->geometry()->type();
  }
  return true;
}

bool QgsTopologicalLayer::analyzeTopology( double threshold, QProgressDialog *pd )
{
  pd->setLabel( new QLabel("Analyzing...") );
  pd->setMaximum( mPolygons.size() );
  for (int i = 0; i < mPolygons.size(); i++)
  {
    pd->setValue( i );
    /*if (mPolygons[i].featureID == 13)
    {
      for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
      {
        for (int ii = 0; ii < mPolygons[i].searchPoints[parti].size(); ii++)
        {
          qDebug() << i << parti << ii << mPolygons[i].searchPoints[parti][ii].x() << mPolygons[i].searchPoints[parti][ii].y();
        }
      }
      return false;
    }*/
    for (int j = i; j < mPolygons.size(); j++)
    {
      for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
      {
        for (int partj = ((i == j) ? parti + 1: 0 ); partj < mPolygons[j].searchPoints.size(); partj++)
        {
          if (((mPolygons[i].boundingBoxMin[parti].x() - mPolygons[j].boundingBoxMax[partj].x())*
              (mPolygons[i].boundingBoxMax[parti].x() - mPolygons[j].boundingBoxMin[partj].x()) <= 0) &&
              ((mPolygons[i].boundingBoxMin[parti].y() - mPolygons[j].boundingBoxMax[partj].y())*
              (mPolygons[i].boundingBoxMax[parti].y() - mPolygons[j].boundingBoxMin[partj].y()) <= 0))
          {
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
              if (((mPolygons[j].boundingBoxMin[partj].x() - mPolygons[i].searchPoints[parti][ii].x())*
              (mPolygons[j].boundingBoxMax[partj].x() - mPolygons[i].searchPoints[parti][ii].x()) <= 0) &&
              ((mPolygons[j].boundingBoxMin[partj].y() - mPolygons[i].searchPoints[parti][ii].y())*
              (mPolygons[j].boundingBoxMax[partj].y() - mPolygons[i].searchPoints[parti][ii].y()) <= 0))
              {
                /* for every point in other part starting from last */
                found = false;
                //if (!mPolygons[i].searchPoints[parti][ii].isBreakForward())
                for (int jj = mPolygons[j].searchPoints[partj].size() - 1; (jj >= 0) && (!found); jj--)
                {
                  //qDebug() << "Comparing " << mPolygons[i].searchPoints[parti][ii].x() << mPolygons[i].searchPoints[parti][ii].y() <<
                  //            "and" << mPolygons[j].searchPoints[partj][jj].x() << mPolygons[j].searchPoints[partj][jj].y();
                  firstfound = false;
                  /* found first connected point, then we need to check how to move
                  * clockwise or counterclockwise */
                  if ((qAbs(mPolygons[i].searchPoints[parti][ii].x() - mPolygons[j].searchPoints[partj][jj].x()) < threshold) &&
                      (qAbs(mPolygons[i].searchPoints[parti][ii].y() - mPolygons[j].searchPoints[partj][jj].y()) < threshold) &&
                      (jj >= 0) && (ii < mPolygons[i].searchPoints[parti].size()))
                  {
                    qDebug() << "points are near";
                    firstfound = true;
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
                    qDebug() << "i" << i << "parti" << parti << "tempi" << tempi << "startDeletei" << startDeletei;
                    qDebug() << "j" << j << "partj" << partj << "tempj" << tempj << "startDeletej" << startDeletej;
                    qDebug() << "CHECKING way-" << mPolygons[i].searchPoints[parti][tempi].x() <<
                                                  mPolygons[i].searchPoints[parti][tempi].y() <<
                                          "and" << mPolygons[j].searchPoints[partj][tempj].x() <<
                                                  mPolygons[j].searchPoints[partj][tempj].y();
                    if ((qAbs(mPolygons[i].searchPoints[parti][tempi].x() - mPolygons[j].searchPoints[partj][tempj].x()) < threshold) &&
                      (qAbs(mPolygons[i].searchPoints[parti][tempi].y() - mPolygons[j].searchPoints[partj][tempj].y()) < threshold) &&
                      (!(mPolygons[i].searchPoints[parti][startDeletei].isBreakForward())) &&
                      (!(mPolygons[j].searchPoints[partj][startDeletej].isBreakBackward())))
                    {
                      found = true;
                      way = -1;
                      qDebug() << "way = -1";
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

                      qDebug() << "CHECKING way+" << mPolygons[i].searchPoints[parti][tempi].x() <<
                                                  mPolygons[i].searchPoints[parti][tempi].y() <<
                                          "and" << mPolygons[j].searchPoints[partj][tempj].x() <<
                                                  mPolygons[j].searchPoints[partj][tempj].y();
                      if ((qAbs(mPolygons[i].searchPoints[parti][tempi].x() - mPolygons[j].searchPoints[partj][tempj].x()) < threshold) &&
                        (qAbs(mPolygons[i].searchPoints[parti][tempi].y() - mPolygons[j].searchPoints[partj][tempj].y()) < threshold) &&
                        (!(mPolygons[i].searchPoints[parti][startDeletei].isBreakForward())) &&
                        (!(mPolygons[j].searchPoints[partj][startDeletej].isBreakForward())))
                      {
                        found = true;
                        way = 1;
                      }
                    }
                    qDebug() << "Found way: " << way;
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
                        qDebug() <<  "jj" << jj << "/" << mPolygons[j].searchPoints[partj].size();
                        qDebug() << "ii" << ii << "/" << mPolygons[i].searchPoints[parti].size();
                        qDebug() << "qAbs(" << mPolygons[i].searchPoints[parti][ii].x() <<"-"
                                  << mPolygons[j].searchPoints[partj][jj].x() <<") ="<< qAbs(mPolygons[i].searchPoints[parti][ii].x() - mPolygons[j].searchPoints[partj][jj].x());
                        if ((qAbs(mPolygons[i].searchPoints[parti][ii].x() - mPolygons[j].searchPoints[partj][jj].x()) < threshold) &&
                              (qAbs(mPolygons[i].searchPoints[parti][ii].y() - mPolygons[j].searchPoints[partj][jj].y()) < threshold))
                        {
                          //qDebug() << "qAbs(" << mPolygons[i].searchPoints[parti][ii].x() <<"-"
                          //        << mPolygons[j].searchPoints[partj][jj].x() <<") ="<< qAbs(mPolygons[i].searchPoints[parti][ii].x() - mPolygons[j].searchPoints[partj][jj].x());
                          //qDebug() << "threshold" << threshold;
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

                      /* case when segment could contain last and first point of polygons */
                      if (//(ii != startDeletei) && (jj != startDeletej) &&
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
                          if ((qAbs(mPolygons[i].searchPoints[parti][tempi].x() - mPolygons[j].searchPoints[partj][tempj].x()) < threshold)
                          && (qAbs(mPolygons[i].searchPoints[parti][tempi].y() - mPolygons[j].searchPoints[partj][tempj].y()) < threshold))
                          {
                            qDebug() << "Found backward common point [" << tempi <<"]["<< tempj << "]" << mPolygons[i].searchPoints[parti][tempi].x() << mPolygons[i].searchPoints[parti][tempi].y() << "and" << mPolygons[j].searchPoints[partj][tempj].x() << mPolygons[j].searchPoints[partj][tempj].y();
                            sizeDelete++;
                            temp.prepend(mPolygons[i].searchPoints[parti][tempi]);
                            startDeletei = tempi;
                            way > 0 ? startDeletej = tempj: lastjj;// = tempj;
                          }
                          else
                          {
                            endExtraLoop = true;
                          }
                        } while ((!mPolygons[i].searchPoints[parti][tempi].isBreakBackward()) && (!endExtraLoop));
                      }

                      if (temp.size() > 1)
                      {
                        mSegments.push_back(temp);
                        //mPolygons[i].segments_id[parti].push_back(mSegments.size() - 1);
                        //mPolygons[j].segments_id[partj].push_back(mSegments.size() - 1);
                        qDebug() << "found segment of size " << mSegments[mSegments.size() - 1].size();
                      }
                      temp.clear();
                    }
                  }
                }
              }
              if ((found) && (way != 0) && (sizeDelete > 0))
              {
                  mPolygons[i].searchPoints[parti][startDeletei].setBreakForwardId( mSegments.size() - 1, true);
                  qDebug() <<"set reference to " << mSegments.size() - 1 << "forward break i (true)" << startDeletei << mPolygons[i].searchPoints[parti][startDeletei].x() << mPolygons[i].searchPoints[parti][startDeletei].y();
                  mPolygons[i].searchPoints[parti][lastii].setBreakBackwardId( mSegments.size() - 1, false);
                  qDebug() <<"set reference to " << mSegments.size() - 1 << "backward break i (false)" << lastii << mPolygons[i].searchPoints[parti][lastii].x() << mPolygons[i].searchPoints[parti][lastii].y();
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
                  mPolygons[j].searchPoints[partj][tempIndex].setBreakForwardId(mSegments.size() - 1, (way >= 0) );// true);
                  mPolygons[j].searchPoints[partj][tempIndex2].setBreakBackwardId(mSegments.size() - 1, (way < 0) );//false);
                  qDebug() << "set reference to " << mSegments.size() - 1 << "forward break j ("<< way << ")" << tempIndex << mPolygons[j].searchPoints[partj][tempIndex].x() << mPolygons[j].searchPoints[partj][tempIndex].y();
                  qDebug() << "set reference to " << mSegments.size() - 1 << "backward break j ("<< -way << ")" << tempIndex2 << mPolygons[j].searchPoints[partj][tempIndex2].x() << mPolygons[j].searchPoints[partj][tempIndex2].y();
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
  /* push to segments remaining points */
  QVector<QgsPoint> temp;
  for (int i = 0; i < mPolygons.size(); i++)
  {
    for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
    {
      qDebug() << "Searching segments in remaining points of " << i << "part" << parti;
      qDebug() << "CHECK";
      for (int j = 0; j < mPolygons[i].searchPoints[parti].size(); j ++)
      {
        qDebug() << j;
        qDebug() << mPolygons[i].searchPoints[parti][j].x() << mPolygons[i].searchPoints[parti][j].y();
        if (mPolygons[i].searchPoints[parti][j].isBreakBackward())
        {
          qDebug() << "break backward: " << mPolygons[i].searchPoints[parti][j].getBreakBackwardId();
        }
        if (mPolygons[i].searchPoints[parti][j].isBreakForward())
        {
          qDebug() << "break forward: " << mPolygons[i].searchPoints[parti][j].getBreakForwardId();
        }
      }

      bool again_needed = false;
      again:
      int lastj = 0;
      do
      {
        lastj--;
        if (lastj < 0)
        {
          lastj = mPolygons[i].searchPoints[parti].size() - 1;
        }
      } while (( !mPolygons[i].searchPoints[parti][lastj].isBreakBackward() || mPolygons[i].searchPoints[parti][lastj].isBreakForward()) && (lastj != 0));
      int j = lastj;
      int startj = j;
      bool first_time = true;
      again_needed = false;
      do
      {
        qDebug() << "CHECK next alone? or not point: " << mPolygons[i].searchPoints[parti][j].x() << mPolygons[i].searchPoints[parti][j].y();
        if ((mPolygons[i].searchPoints[parti][j].isBreakForward()) || ((j == lastj) && (!first_time)))
        {
          qDebug() << "polygon" << i << "part" << parti;
          qDebug() << "Found last alone point [" << j <<"]" << mPolygons[i].searchPoints[parti][j].x() << mPolygons[i].searchPoints[parti][j].y();
          temp.push_back(mPolygons[i].searchPoints[parti][j]);
          //qDebug() << "Segment size found: " << temp.size();
          if (temp.size() > 1)
          {
            again_needed = true;
            qDebug() << "0.1";
            qDebug() << "j: " << j << "lastj: " << lastj << "startj: " << startj;
            qDebug() << "mPolygons[i].searchPoints[parti].size(): "<< mPolygons[i].searchPoints[parti].size();
            mSegments.push_back(temp);
            qDebug() << "set reference to" << mSegments.size() - 1 << "in" << i << parti;
            qDebug() << "from" << mPolygons[i].searchPoints[parti][startj].x() << mPolygons[i].searchPoints[parti][startj].y()
                     << "to" << mPolygons[i].searchPoints[parti][j].x() << mPolygons[i].searchPoints[parti][j].y();
            mPolygons[i].searchPoints[parti][j].setBreakBackwardId(mSegments.size() - 1, false);
            mPolygons[i].searchPoints[parti][startj].setBreakForwardId(mSegments.size() - 1, true);
            if (startj < j)
            {
              qDebug() << "remove from " << i << parti << j - startj - 1 << "starting from " << startj + 1;
              mPolygons[i].searchPoints[parti].remove(startj + 1, j - startj - 1);
              lastj = startj;
              startj ++;
              if (startj >= mPolygons[i].searchPoints[parti].size())
              {
                startj = 0;
              }
              j = startj;
              if (j >= mPolygons[i].searchPoints[parti].size())
              {
                j = 0;
              }
            }
            else
            {
              if ( startj + 1 >= mPolygons[i].searchPoints[parti].size() )
              {
                if (j - 1 > 0)
                {
                  qDebug() << "remove from " << i << parti << j - 1 << "starting from 0";
                  mPolygons[i].searchPoints[parti].remove(0, j - 1);
                  qDebug() << "done";
                }
              }
              else
              {
                if (mPolygons[i].searchPoints[parti].size() - startj - 1 > 0)
                {
                  qDebug() << "remove from " << i << parti << mPolygons[i].searchPoints[parti].size() - startj - 1 << "starting from" << startj + 1;
                  mPolygons[i].searchPoints[parti].remove(startj + 1, mPolygons[i].searchPoints[parti].size() - startj - 1);
                  qDebug() << "done";
                }
                if (j - 1 > 0)
                {
                  qDebug() << "remove from " << i << parti << j - 1 << "starting from 0";
                  mPolygons[i].searchPoints[parti].remove(0, j - 1);
                  qDebug() << "done";
                }
              }

              lastj = 0;
              j = lastj + 1;
              startj = lastj;
              if (j >= mPolygons[i].searchPoints[parti].size())
              {
                j = 0;
              }
            }
            qDebug() << "0.2";
          }
          temp.clear();
          if (again_needed)
            goto again;
        }
        else
        {
          if (first_time)
          {
            startj = j;
          }
          first_time = false;
          qDebug() << "Found alone point [" << j <<"]" << mPolygons[i].searchPoints[parti][j].x() << mPolygons[i].searchPoints[parti][j].y();
          temp.push_back(mPolygons[i].searchPoints[parti][j]);
        }
        j++;
        if (j >= mPolygons[i].searchPoints[parti].size())
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
            mSegments.push_back(temp);
            mPolygons[i].searchPoints[parti][startj].setBreakForwardId(mSegments.size() - 1, true);
            mPolygons[i].searchPoints[parti][j].setBreakBackwardId(mSegments.size() - 1, false);
            qDebug() << "1";
            mPolygons[i].searchPoints[parti].remove(1, mPolygons[i].searchPoints[parti].size() - 1);
            qDebug() << "2";
            again_needed = true;
          }
          temp.clear();
          if (again_needed)
            goto again;
        }
      } while (j != lastj);
    }
  }
  return true;
}

bool QgsTopologicalLayer::simplify( double tolerance, QProgressDialog * pd )
{
  pd->setLabel( new QLabel("Simplifying...") );
  pd->setMaximum( mSegments.size() );
  for (int j = 0; j < mSegments.size(); j++)
  {
    pd->setValue( j );
    if (mSegments[j].size() > 2)
    {
      mSegments[j] = QgsSimplify::simplifyPoints(mSegments[j], tolerance);
    }
    if (pd->wasCanceled())
    {
      return false;
    }
  }
  return true;
}


QVector<QgsPolyline> QgsTopologicalLayer::getSimplifiedPolyline( int id )
{
  QVector< QgsPolyline > result;
  QVector< QVector < QgsPoint > > temp;
  bool found = false;
  int segmentId;
  for (int i = 0; i < mPolygons.size(); i++)
  {
    if (id == mPolygons[i].featureID)
    {
      for (int parti = 0; parti < mPolygons[i].searchPoints.size(); parti++)
      {
        //qDebug() << "polygon number " << i << "part " << parti;
        temp.append( QVector<QgsPoint>() );
        for (int ii  = 0; ii < mPolygons[i].searchPoints[parti].size(); ii++)
        {
          if (mPolygons[i].searchPoints[parti][ii].isBreakForward())
          {
            segmentId = mPolygons[i].searchPoints[parti][ii].getBreakForwardId();
            //qDebug() << "segmentId: " << segmentId;
            if (!mSegments[segmentId].isEmpty())
            {
              if (mPolygons[i].searchPoints[parti][ii].breakForwardIsStart())
              {
                for (int j = 0; j < mSegments[segmentId].size(); j++)
                {
                  temp[temp.size() - 1].append( mSegments[segmentId][j] );
                  //qDebug() << mSegments[segmentId][j].x() << mSegments[segmentId][j].y();
                }
              }
              else
              {
                for (int j = mSegments[segmentId].size() - 1; j >= 0; j--)
                {
                  temp[temp.size() - 1].append( mSegments[segmentId][j] );
                  //qDebug() << mSegments[segmentId][j].x() << mSegments[segmentId][j].y();
                }
              }
            }
            else
            {
              qDebug() << "segment is empty";
            }
          }
        }
        if (temp[temp.size() - 1].size() > 3)
        {
          found = true;
          result.append(temp[temp.size() - 1]);
        }
      }
    }
  }
  return result;
}

QVector<QgsPoint> QgsTopologicalLayer::getPointList( QgsFeature *f )
{
  QgsGeometry* line = f->geometry();
  if (( line->type() != QGis::Line && line->type() != QGis::Polygon ))// || line->isMultipart() )
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
      return line->asPolygon()[0];//QVector<QgsPoint>();
    }
    return line->asPolygon()[0];
  }
}

QVector<QVector< QVector<QgsPoint> > > QgsTopologicalLayer::getMultiPointList( QgsFeature *f)
{
  QgsGeometry* geom = f->geometry();
  return geom->asMultiPolygon();
}