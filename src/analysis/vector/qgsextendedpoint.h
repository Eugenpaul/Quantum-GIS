#ifndef QGSEXTENDEDPOINT_H
#define QGSEXTENDEDPOINT_H

#include "qgsgeometry.h"

class QgsExtendedPoint: public QgsPoint
{
    int m_breakForwardId;       /* id of the segment connected */
    int m_breakBackwardId;
    bool m_breakForwardStart;   /* true if the forward segment connected with its first point (last otherwise)*/
    bool m_breakBackwardStart;  /* true if the backward segment connected with its first point (last otherwise)*/

  public:

    QgsExtendedPoint() : QgsPoint(), m_breakForwardId( -1 ), m_breakBackwardId( -1 ),m_breakForwardStart( true ), m_breakBackwardStart( true )
    {}

    QgsExtendedPoint( double x, double y ) : QgsPoint( x, y ), m_breakForwardId( -1 ), m_breakBackwardId( -1 ), m_breakForwardStart( true ), m_breakBackwardStart( true )
    {}

    bool isBreak(int way)
    {
      if (((way > 0) && (m_breakForwardId >= 0)) || ((way < 0) && (m_breakBackwardId >= 0)))
      {
        return true;
      }
      else
      {
        return false;
      }
    }

    bool isBreakForward()
    {
      return ( m_breakForwardId >= 0 );
    }

    bool isBreakBackward()
    {
      return ( m_breakBackwardId >= 0 );
    }

    void setBreakForwardId( int breakForwardId, bool breakForwardStart )
    {
      m_breakForwardId = breakForwardId;
      m_breakForwardStart = breakForwardStart;
    }

    void setBreakBackwardId( int breakBackwardId, bool breakBackwardStart )
    {
      m_breakBackwardId = breakBackwardId;
      m_breakBackwardStart = breakBackwardStart;
    }

    int getBreakForwardId()
    {
      return m_breakForwardId;
    }

    int getBreakBackwardId()
    {
      return m_breakBackwardId;
    }

    bool breakForwardIsStart()
    {
      return m_breakForwardStart;
    }

    bool breakBackwardIsStart()
    {
      return m_breakBackwardStart;
    }

    QgsExtendedPoint & operator=( const QgsPoint & other )
    {
      this->setX( other.x() );
      this->setY( other.y() );
      m_breakForwardId = -1;
      m_breakBackwardId = -1;
      m_breakForwardStart = true;
      m_breakBackwardStart = true;
      return *this;
    }
};

#endif