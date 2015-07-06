#include <pebble.h>

#define MINUTE_FULL_COLOR COLOR_FALLBACK(GColorBlueMoon, GColorBlack)
#define MINUTE_EMPTY_COLOR COLOR_FALLBACK(GColorFromRGB(160, 160, 160), GColorWhite)
#define BATTERY_COLOR COLOR_FALLBACK(GColorFromRGB(160, 160, 160), GColorBlack)
#define LARGE_FONT_ID RESOURCE_ID_BANGERS_FONT_58
#define MEDIUM_FONT_ID RESOURCE_ID_BANGERS_FONT_22
#define SMALL_FONT_ID RESOURCE_ID_BANGERS_FONT_16

static Window *s_main_window;

static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;

static char s_battery_buffer[5];

static Layer *top_bars_layer;
static Layer *bottom_bars_layer;

static int current_minute;

static GFont large_font;
static GFont medium_font;
static GFont small_font;

static int minuteStatusArray[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static void battery_handler(BatteryChargeState new_state) {
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", new_state.charge_percent);
  text_layer_set_text(s_battery_layer, s_battery_buffer);  
}


static void draw_minute_rectangle(int minute, int isBottom, GContext* ctx) {
  int newMinuteStatusArray[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  for(int i = 1; i < 10; i++) {
    newMinuteStatusArray[i - 1] = minute % 10 >= i ? 1 : 0;
  }
  int start = isBottom * 5;
  int end = start == 0 ? 5 : 9;

  for(int i = start; i < end; i++) {
    int x = ((i % 5) * (i / 5 == 0 ? 28 : 36) + (i / 5 == 0 ? 2 : 0)) + 2;
    int y = 0;//(i / 5) * 142;
    int width = (i / 5 == 0 ? 28 : 36) - 4;
    int height = 12;

    graphics_context_set_fill_color(ctx, (newMinuteStatusArray[i] == 1 ? MINUTE_FULL_COLOR : MINUTE_EMPTY_COLOR));
    graphics_fill_rect(ctx, GRect(x, y, width, height), 2, GCornersAll);

    minuteStatusArray[i] = newMinuteStatusArray[i];
  }
}

void top_bars_update_callback(Layer *me, GContext* ctx) {
  (void)me;
  draw_minute_rectangle(current_minute, 0, ctx);
}

void bottom_bars_update_callback(Layer *me, GContext* ctx) {
  (void)me;
  draw_minute_rectangle(current_minute, 1, ctx);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  current_minute = tick_time->tm_min;
  
  // Create a long-lived buffer
  static char buffer[] = "00:00";
  static char dateBuffer[] = "XXX XX XXX";
  
/*
  int m = tick_time->tm_min + 2;
  int h = tick_time->tm_hour + (m / 60);
  m = m % 60;
*/
  int m = tick_time->tm_min;
  int h = tick_time->tm_hour;
  m = (m / 5) * 5;

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    snprintf(buffer, sizeof(buffer), "%02d:%02d", h, m);
  } else {
    // Use 12 hour format
    snprintf(buffer, sizeof(buffer), "%02d:%02d", (h % 13) + (h / 13), m);
  }
  strftime(dateBuffer, sizeof(dateBuffer), "%a %b %e", tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  text_layer_set_text(s_date_layer, dateBuffer);
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  large_font = fonts_load_custom_font(resource_get_handle(LARGE_FONT_ID));
  medium_font = fonts_load_custom_font(resource_get_handle(MEDIUM_FONT_ID));
  small_font = fonts_load_custom_font(resource_get_handle(SMALL_FONT_ID));

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 41, 144, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, MINUTE_FULL_COLOR);
  text_layer_set_text(s_time_layer, "00:00");

  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(0, 105, 144, 22));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, MINUTE_FULL_COLOR);
  text_layer_set_text(s_date_layer, "");

  // Create battery TextLayer
  s_battery_layer = text_layer_create(GRect(4, 22, 140, 16));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, BATTERY_COLOR);
  battery_handler(battery_state_service_peek());

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, large_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  text_layer_set_font(s_date_layer, medium_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  text_layer_set_font(s_battery_layer, small_font);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));

  top_bars_layer = layer_create(GRect(0, 2, 144, 12));
  layer_set_update_proc(top_bars_layer, top_bars_update_callback);
  layer_add_child(window_get_root_layer(window), top_bars_layer);

  bottom_bars_layer = layer_create(GRect(0, 168 - 14, 144, 167));
  layer_set_update_proc(bottom_bars_layer, bottom_bars_update_callback);
  layer_add_child(window_get_root_layer(window), bottom_bars_layer);
  
  update_time();
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  
  layer_destroy(top_bars_layer);
  layer_destroy(bottom_bars_layer);
  
  fonts_unload_custom_font(large_font);
  fonts_unload_custom_font(medium_font);
  fonts_unload_custom_font(small_font);
}

static void init() {
  current_minute = 0;
  battery_state_service_subscribe(battery_handler);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}