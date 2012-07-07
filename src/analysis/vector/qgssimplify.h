
#include <QVector>
#include "qgsgeometry.h"
#include "qgsfeature.h"

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

    /** simplify a line given by a vector of points and tolerance. Returns simplified vector of points */
    static QVector<QgsPoint> simplifyPoints( const QVector<QgsPoint>& pts, double tolerance );


};