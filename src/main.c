#include <pebble.h>

#define ANTIALIASING true
#define SWEEP_HOURS false
#define SWEEP_MINUTES false

typedef struct {
  int hours;
  int minutes;
  int seconds;
} Time;

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_time_layer;
static TextLayer *s_day_layer;
static TextLayer *s_month_layer;

static GPoint s_center;
static GRect window_bounds;
static Time s_last_time;
static GFont s_custom_font;

static float HOURS_TRACK_RADIUS, HOURS_TRACK_STROKE, HOURS_HAND_RADIUS;
static float MINUTES_TRACK_RADIUS, MINUTES_TRACK_STROKE, MINUTES_HAND_RADIUS;
static float SECONDS_TRACK_RADIUS, SECONDS_TRACK_STROKE, SECONDS_HAND_RADIUS;

/************************************ functions **************************************/ 

float myround(float f) {
  if (f >= 0x1.0p23) return f;
  if (f <= 0.5) return 0;
  return (float) (unsigned int) (f + 0.5f);
}

static void get_angles_60(int time, int time_fraction, int delta, int* start, int* end) {
	*start = myround((time + (float)time_fraction/60) * 6 + delta);
	*end = *start + 360 - 2*delta;
} 

static void get_angles_12(int hour, int minute, int delta, int* start, int* end) {
	*start = myround((hour + (float)(minute)/60) * 30 + delta);
	*end = *start + 360 - 2*delta;
}
/************************************ UI **************************************/

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";
  static char month_buffer[] = "XXX";
  static char day_buffer[] ="00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
	
  strftime(month_buffer, sizeof(month_buffer), "%b", tick_time);
  
	strftime(day_buffer, sizeof("00"), "%d", tick_time);
	
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
   text_layer_set_text(s_day_layer, day_buffer);
  text_layer_set_text(s_month_layer, month_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  // Store time
  s_last_time.hours = tick_time->tm_hour;
  s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
  s_last_time.minutes = tick_time->tm_min;
  s_last_time.seconds = tick_time->tm_sec;
  
  // Redraw
  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
	//update_time();
}

static void update_proc(Layer *layer, GContext *ctx) {
  int padding;
  
  PBL_IF_ROUND_ELSE(padding = 16, padding = 12);
  
  HOURS_TRACK_RADIUS = (window_bounds.size.w - padding) / 2; //66
  HOURS_TRACK_STROKE = 2;
  
  MINUTES_TRACK_RADIUS = HOURS_TRACK_RADIUS - 10; //56
  MINUTES_TRACK_STROKE = 2;
  
  SECONDS_TRACK_RADIUS = HOURS_TRACK_RADIUS - 20; //46
  SECONDS_TRACK_STROKE = 2;
  
  SECONDS_HAND_RADIUS = 2;
  MINUTES_HAND_RADIUS = 3;
  HOURS_HAND_RADIUS = 4;
  
  // Color background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, window_bounds, 0, GCornerNone);
  
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "bounds: %d %d %d %d", window_bounds.origin.x, window_bounds.origin.y, window_bounds.size.h, window_bounds.size.w );
  

  //set colour for tracks
  graphics_context_set_stroke_color(ctx, GColorWhite );

  graphics_context_set_antialiased(ctx, ANTIALIASING);

  // Don't use current time while animating
  Time mode_time = s_last_time;
  
  // generate position of hands
  
  GPoint second_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * mode_time.seconds / 60) * (int32_t)(SECONDS_TRACK_RADIUS) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * mode_time.seconds / 60) * (int32_t)(SECONDS_TRACK_RADIUS) / TRIG_MAX_RATIO) + s_center.y,
  };
	
  float minutes, hours;	
	
  if (SWEEP_MINUTES == true) {
	minutes = mode_time.minutes + (float)mode_time.seconds / 60;
  } else {
	minutes = mode_time.minutes;
  }
	
  GPoint minute_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * minutes / 60) * (int32_t)(MINUTES_TRACK_RADIUS) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * minutes/ 60) * (int32_t)(MINUTES_TRACK_RADIUS) / TRIG_MAX_RATIO) + s_center.y,
  };

  if( SWEEP_HOURS == true) {
    hours = mode_time.hours + (float)mode_time.minutes / 60;
  } else {
	hours = mode_time.hours; 
  }

  GPoint hour_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * hours / 12) * (int32_t)(HOURS_TRACK_RADIUS) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * hours / 12) * (int32_t)(HOURS_TRACK_RADIUS) / TRIG_MAX_RATIO) + s_center.y,
  };
	
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  GRect seconds_rect = GRect(s_center.x - SECONDS_TRACK_RADIUS, s_center.y - SECONDS_TRACK_RADIUS, SECONDS_TRACK_RADIUS * 2, SECONDS_TRACK_RADIUS * 2);
  
  GRect minutes_rect = GRect(s_center.x - MINUTES_TRACK_RADIUS, s_center.y - MINUTES_TRACK_RADIUS, MINUTES_TRACK_RADIUS * 2, MINUTES_TRACK_RADIUS * 2);
  
  GRect hours_rect = GRect(s_center.x - HOURS_TRACK_RADIUS, s_center.y - HOURS_TRACK_RADIUS, HOURS_TRACK_RADIUS * 2, HOURS_TRACK_RADIUS * 2);
  
 //----------------------------------
    
  int seconds_start_angle, seconds_end_angle;
  
  int seconds_delta = 12;
  int minutes_delta = 12;
  int hours_delta = 12;
	
  get_angles_60(mode_time.seconds, 0, seconds_delta, &seconds_start_angle, &seconds_end_angle);
  
  //----------------------------------
  
  //int minutes_angle = mode_time.minutes * 360 / 60;
  int minutes_start_angle, minutes_end_angle;
    
  get_angles_60(mode_time.minutes, (SWEEP_MINUTES == true) ? mode_time.seconds : 0, minutes_delta, &minutes_start_angle, &minutes_end_angle);
  //get_angles_60(mode_time.minutes, mode_time.seconds, minutes_delta, &minutes_start_angle, &minutes_end_angle);
  
  //----------------------------------
  
  //int hours_angle = mode_time.hours * 360 / 12;
  int hours_start_angle, hours_end_angle;

  get_angles_12(mode_time.hours,(SWEEP_HOURS == true) ? mode_time.minutes : 0, hours_delta, &hours_start_angle, &hours_end_angle);
  
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "seconds: %d, start: %d, end: %d",  mode_time.seconds, seconds_start_angle, seconds_end_angle);
  
   //set colour for arcs and "hands"
    graphics_context_set_fill_color(ctx, GColorWhite );
    
  //draw seconds arc
  graphics_fill_radial(ctx, seconds_rect, GOvalScaleModeFitCircle, SECONDS_TRACK_STROKE,  DEG_TO_TRIGANGLE(seconds_start_angle), DEG_TO_TRIGANGLE(seconds_end_angle));
  
  //draw minutes arc
  graphics_fill_radial(ctx, minutes_rect, GOvalScaleModeFitCircle, MINUTES_TRACK_STROKE,  DEG_TO_TRIGANGLE(minutes_start_angle), DEG_TO_TRIGANGLE(minutes_end_angle));
  
  //draw hours arc
  graphics_fill_radial(ctx, hours_rect, GOvalScaleModeFitCircle, HOURS_TRACK_STROKE,  DEG_TO_TRIGANGLE(hours_start_angle), DEG_TO_TRIGANGLE(hours_end_angle));
  
  //draw minute hand
  graphics_fill_circle(ctx, minute_hand, MINUTES_HAND_RADIUS);
  
  //draw hour hand
  graphics_fill_circle(ctx, hour_hand, HOURS_HAND_RADIUS);
  
  //draw second hand
  graphics_context_set_fill_color(ctx, GColorRed );
  graphics_fill_circle(ctx, second_hand, SECONDS_HAND_RADIUS);
	
	update_time();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  window_bounds = layer_get_bounds(window_layer);
  
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "bounds: %d %d %d %d", window_bounds.origin.x, window_bounds.origin.y, window_bounds.size.h, window_bounds.size.w );

  s_center = grect_center_point(&window_bounds);
  
  s_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_canvas_layer, update_proc);
  layer_add_child(window_layer, s_canvas_layer);
	
// Create time TextLayer
  
  int font_height;
  
  PBL_IF_ROUND_ELSE(font_height = 45, font_height = 36); //fudge factor to get text vertically centred
  
  s_time_layer = text_layer_create(GRect(window_bounds.origin.x, (window_bounds.size.h-font_height)/2, window_bounds.size.w, font_height));
	
  s_day_layer = text_layer_create(GRect(0,50,window_bounds.size.w,18)); //need to calculate proper y-location
  s_month_layer = text_layer_create(GRect(0,100,window_bounds.size.w,18)); //need to calculate proper y-location
  
  text_layer_set_background_color(s_day_layer, GColorClear);
  text_layer_set_text_color(s_day_layer, GColorWhite);
  text_layer_set_text(s_day_layer, "00");
  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
	
  text_layer_set_background_color(s_month_layer, GColorClear);
  text_layer_set_text_color(s_month_layer, GColorWhite);
  text_layer_set_text(s_month_layer, "XXX");
  
  PBL_IF_ROUND_ELSE(s_custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DOSIS_SEMIBOLD_40)), s_custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DOSIS_SEMIBOLD_30)));

  text_layer_set_font(s_time_layer, s_custom_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
  text_layer_set_font(s_day_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DOSIS_SEMIBOLD_18)));
  text_layer_set_text_alignment(s_day_layer, GTextAlignmentCenter);

  text_layer_set_font(s_month_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DOSIS_SEMIBOLD_18)));
  text_layer_set_text_alignment(s_month_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_day_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_month_layer));
	// Make sure the time is displayed from the start
  update_time();
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_day_layer);
  text_layer_destroy(s_month_layer);
  fonts_unload_custom_font(s_custom_font);
}

/*********************************** App **************************************/

static void init() {
  srand(time(NULL));

  time_t t = time(NULL);
  struct tm *time_now = localtime(&t);
  tick_handler(time_now, SECOND_UNIT);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
	accel_tap_service_unsubscribe();
}

int main() {
  init();
  app_event_loop();
  deinit();
}