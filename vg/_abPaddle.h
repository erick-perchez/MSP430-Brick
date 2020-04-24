#ifndef abPaddle_included
#define abPaddle_included
#include "shape.h"

/*Paddle will be used to make the ball bounce and
  keep it from touching the floor
*/

typedef struct AbPaddle_s{
  void(*getBounds)(const struct AbRect_s *paddle, const Vec2 *centerPos, Region *bounds);
  int (*check)(const struct AbRect_s *paddle, const Vec2 *centerPos, const Vec2 *pixel);
  const Vec2 halfSize;
} AbPaddle;

int abPaddleCheck(const AbRect *paddle, const Vec2 *position, const Vec2 *pixel);

#endif
