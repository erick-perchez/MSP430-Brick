#include "shape.h"
#include "_abPaddle.h"

int abPaddleCheck(const AbRect *paddle, const Vec2 *position, const Vec2 *pixel){
  Vec2 relPos;
  vec2Sub(&relPos, pixel, position);/* vector from center to pixel */
  /* reject pixels in slice */
  if (relPos.axes[0] > -5 && relPos.axes[0] < 5 && relPos.axes[1] > .25)
    return 0;
  
  else
    return abRectCheck(paddle, position, pixel);
}





  
