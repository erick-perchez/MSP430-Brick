/** \file shapemotion.c
 *  \Builds upon shapemotion demo, it includes 8 bricks,
 *  the paddle used to bounce the ball, and the field. Whenever
 *  the ball touches the paddle, the score is incremented, when the
 *  ball touches the bottom part of the layer, score resets to 0
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"
#include "buzzer.c"
#include "_abPaddle.h"
#include "abPaddle.c"

#define SW1 BIT0
#define SW2 BIT1
#define SW3 BIT2
#define SW4 BIT3
#define GREEN_LED BIT6

#define SWITCHES SW1|SW2|SW3|SW4

int sound = 1500;

int score = 0;

AbRect rect10 = {abRectGetBounds, abRectCheck, {10,4}}; /**< 10x10 rectangle */
//MAKE FIELD//
AbRect rect1 = {abRectGetBounds, abRectCheck, {10,4}}; //10x4 rectangles will make the bricks
AbRect rect2 = {abRectGetBounds, abRectCheck, {10,4}};
AbRect rect3 = {abRectGetBounds, abRectCheck, {10,4}};
AbRect rect4 = {abRectGetBounds, abRectCheck, {10,4}};
AbRect rect5 = {abRectGetBounds, abRectCheck, {10,4}};
AbRect rect6 = {abRectGetBounds, abRectCheck, {10,4}};
AbRect rect7 = {abRectGetBounds, abRectCheck, {10,4}};
AbRect rect8 = {abRectGetBounds, abRectCheck, {10,4}};
//AbRect rectPad = {abRectGetBounds, abRectCheck, {14,2}};

AbPaddle rectPad = {abRectGetBounds, abPaddleCheck, {14,2}};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

/*BRICK 0 -> 7 IS THE PRINTED BRICKS */

Layer brick7 = {
  (AbShape *)&rect8,
  {97,40},
  {0,0}, {0,0},
  COLOR_PINK,
  0,
};

Layer brick6 = {
  (AbShape *)&rect7,
  {75,40},
  {0,0}, {0,0},
  COLOR_PINK,
  &brick7,
};

Layer brick5 = {
  (AbShape *)&rect6,
  {53,40},
  {0,0}, {0,0},
  COLOR_PINK,
  &brick6,
};

Layer brick4 = {
  (AbShape *)&rect5,
  {31,40},
  {0,0}, {0,0},
  COLOR_PINK,
  &brick5,
};

Layer brick3 = {
  (AbShape *)&rect4,
  {97,30},
  {0,0}, {0,0},
  COLOR_PINK,
  &brick4,
  
};

Layer brick2 = {
  (AbShape *)&rect3,
  {75,30},
  {0,0}, {0,0},
  COLOR_PINK,
  &brick3,
};

Layer brick1 = {
  (AbShape *)&rect2,
  {53,30},
  {0,0}, {0,0},
  COLOR_PINK,
  &brick2,
};
  

Layer brick0 = {		
  (AbShape *)&rect1,
  {31,30}, 
  {0,0}, {0,0},
  COLOR_PINK,
  &brick1
};

//paddle being used//

Layer paddle = {
  (AbShape*)&rectPad,
  {62,145},
  {0,0}, {0,0},
  COLOR_RED,
  &brick0,
};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &paddle,
};

Layer ball = {		/**< Layer with an orange circle */
  (AbShape *)&circle3,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GRAY,
  &fieldLayer,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;



int collided(MovLayer *ball, Layer *p2);



// initial value of {0,0} will be overwritten 
MovLayer ml1 = { &paddle, {0,0}, 0 };
MovLayer ml0 = { &ball, {3,2}, &ml1 }; 



void movLayerDraw(MovLayer *movLayers, Layer *layers)
{ 
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off)*/ 
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { // for each moving layer 
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on)*/ 


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving laye*/ 
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check*/ 
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	 




//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);

	/////////////////////////////////////////////////////////////////////////////////
	buzzer_set_period(sound);
      }	// if outside of fence
    } //for axis/
    if(shapeBoundary.botRight.axes[1] > fence->botRight.axes[1]){
      score = 0;
    }
    ml->layer->posNext = newPos;
  } //< for ml
}
///////////////////////////////////////////////////////////////////////////////
void padCollision(MovLayer *ml, MovLayer *paddle){
  //Vec2 newPos, padPos;
  u_char axis;
  Region bBoundary, pBoundary;
  
  abShapeGetBounds(ml->layer->abShape, &(ml->layer)->pos, &bBoundary);
  abShapeGetBounds(paddle->layer->abShape, &(paddle->layer)->pos, &pBoundary);
  if((bBoundary.botRight.axes[0] > pBoundary.topLeft.axes[0] &&
      bBoundary.topLeft.axes[0] < pBoundary.botRight.axes[0]) &&
     (bBoundary.botRight.axes[1] > pBoundary.topLeft.axes[1])){
    
    ml->velocity.axes[1] = -ml->velocity.axes[1];
    score++;
    
  }
}
/////////////////////////////////////////////////////////////////////////////
void brickCollision(MovLayer *ball, Layer* brick){
  u_char axis;
  int within;
  Layer *layers;
  Layer *prev = brick;
  brick = brick->next;
  for(layers = brick; brick; brick = brick->next){
    within = collided(ball,brick);
    if(within == 1){
      ball->velocity.axes[1] = -ball->velocity.axes[1];
      prev->next = brick->next;
      
           
    }
    else{
      prev = prev->next;
    }
  }
}
/////////////////////////////////////////////////////////////////////////////
int collided(MovLayer *ball, Layer *p2){
  Region bBall, bP2;
  u_char axis;
  int within;
  layerGetBounds(ball->layer, &bBall);
  layerGetBounds(p2, &bP2);
  for(axis = 0; axis<2; axis++){
    if(bBall.topLeft.axes[axis] > bP2.topLeft.axes[axis]&&
       bBall.botRight.axes[axis] < bP2.botRight.axes[axis]){
      within = 1;
    }    
  }  
  return within;
}

u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */

void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;
  buzzer_init();
  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&ball);
  
  layerDraw(&ball);

  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &ball);
    drawString5x7(2,2, "SCORE:", COLOR_WHITE, COLOR_BLACK);

    
    int pScore = score % 10;
    if(score < 10){
      drawChar5x7(40,2,'0', COLOR_RED, COLOR_BLACK);
      drawChar5x7(45,2, '0'+ pScore , COLOR_RED, COLOR_BLACK);
    }
    if(score >= 10 && score < 20){
      drawChar5x7(40,2, '1', COLOR_RED, COLOR_BLACK);
      drawChar5x7(45,2, '0' + pScore, COLOR_RED, COLOR_BLACK);
    }
    if(score >= 20 && score < 30){
      drawChar5x7(40,2,'2', COLOR_RED, COLOR_BLACK);
      drawChar5x7(45,2, '0'+ pScore, COLOR_RED, COLOR_BLACK);
    }
    if(score>29){
      drawChar5x7(40,2,'~', COLOR_RED, COLOR_BLACK);
    }
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  u_int switches = p2sw_read();
  if (count == 15) {
    if(!(switches & (1<<0)) || !(switches & (1<<1))){ //button 1/2
      paddle.posNext.axes[0] -= 3;
      
    }
    if(!(switches &(1<<2)) || !(switches &(1<<3))){//button3/4
      paddle.posNext.axes[0] +=3;
      
    }
    

    
    padCollision(&ml0, &ml1);
    buzzer_set_period(0);
    brickCollision(&ml0, &paddle);
 
    mlAdvance(&ml0, &fieldFence);
    if (p2sw_read()){
      redrawScreen = 1;
      
    }
      count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
