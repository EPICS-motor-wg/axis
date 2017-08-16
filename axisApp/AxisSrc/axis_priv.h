/* axis_priv.h: private fields, which could be outside the record */

#ifndef INC_axis_priv_H
#define INC_axis_priv_H

#include "epicsTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct axis_priv {
    struct {
      double position;           /**< Commanded motor position */
      double encoderPosition;    /**< Actual encoder position */
      double motorHighLimitRO;   /**< read only high limit from controller. Valid if mflg & MF_HIGH_LIMIT_RO */
      double motorLowLimitRO;    /**< read only low limit from controller. Valid if mflg & MF_LOW_LIMIT_RO */
    } readBack;
    struct {
      double val;               /* last .VAL */
      double dval;               /* last .DVAL */
      double rval;               /* last .RVAL */
      double rlv;                /* Last Rel Value (EGU) */
      double alst;               /* Last Value Archived */
      double mlst;               /* Last Val Monitored */
      short  dmov;               /* last .DMOV */
    } last;
  };
  
#ifdef __cplusplus
}
#endif
#endif /* INC_axis_priv_H */
