#include <pebble.h>
#include <PDUtils.h>

#define MINUTE_FULL_COLOR COLOR_FALLBACK(GColorFromRGB(0, 38, 255), GColorBlack)
#define MINUTE_EMPTY_COLOR COLOR_FALLBACK(GColorFromRGB(180, 180, 180), GColorWhite)
#define BATTERY_COLOR COLOR_FALLBACK(GColorFromRGB(80, 80, 80), GColorBlack)
#define LARGE_FONT_ID RESOURCE_ID_BANGERS_FONT_57_BOLD
#define MEDIUM_FONT_ID RESOURCE_ID_BANGERS_FONT_22
#define SMALL_FONT_ID RESOURCE_ID_BANGERS_FONT_16

static Window *s_main_window;

static GColor s_background_color;
static GColor s_primary_color;
static GColor s_secondary_color;
static struct tm *s_day_counter;
static bool s_use_day_counter;
static bool s_show_battery;

static TextLayer *s_ampm_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_time_until_layer;

static char s_battery_buffer[5];

static Layer *top_bars_layer;

static int current_minute;

static GFont large_font;
static GFont medium_font;
static GFont small_font;

static AppTimer *shake_timer;
static bool shake_active = false;
  
static Layer *s_canvas_layer;

static int minuteStatusArray[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static bool using24HourFormat = false;

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // Custom drawing happens here!
  graphics_context_set_fill_color(ctx, s_background_color);
  GRect bounds = layer_get_bounds(layer);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

static void battery_handler(BatteryChargeState new_state) {
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", new_state.charge_percent);
  text_layer_set_text(s_battery_layer, s_battery_buffer);  
}


static void draw_time_until() {
  time_t seconds_event = p_mktime(s_day_counter);
	
  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  time_t seconds_now = p_mktime(now);
	
  // Determine the time difference
  int difference = ((((seconds_event - seconds_now) / 60) / 60) / 24) + 1;
  
  static char buffer[] = "xxxxxx days";
  if (difference == 1 || difference == -1)
    snprintf(buffer, sizeof(buffer), "%d day", difference);
  else
    snprintf(buffer, sizeof(buffer), "%d days", difference);
  
  text_layer_set_text(s_time_until_layer, buffer);
}
static void draw_bars(int minute, GContext* ctx) {
  int newMinuteStatusArray[4] = { 0, 0, 0, 0 };
  for(int i = 1; i < 10; i++) {
    int m = minute % 5;
    newMinuteStatusArray[0] = m >= 1 && m <= 1 ? 1 : 0;
    newMinuteStatusArray[1] = m >= 1 && m <= 2 ? 1 : 0;
    newMinuteStatusArray[2] = m >= 1 && m <= 3 ? 1 : 0;
    newMinuteStatusArray[3] = m >= 1 && m <= 4 ? 1 : 0;
  }

  for(int i = 0; i < 4; i++) {
    int x = ((i % 5) * 36) + 2;
    int y = 0;
    int width = 36 - 4;
    int height = 4;

    graphics_context_set_fill_color(ctx, (newMinuteStatusArray[i] == 1 ? s_primary_color : s_secondary_color));
    graphics_fill_rect(ctx, GRect(x, y, width, height), 2, GCornerNone);
  }
}

void top_bars_update_callback(Layer *me, GContext* ctx) {
  (void)me;
  draw_bars(current_minute, ctx);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
    
   //tick_time->tm_hour = 13;
   //tick_time->tm_min = 26;
  
  current_minute = tick_time->tm_min;

  // Create a long-lived buffer
  static char buffer[] = "00:00";
  static char dateBuffer[] = "XXX, XX XXX";
  static char amPmBuffer[] = "XX";
  
/*
  int m = tick_time->tm_min + 2;
  int h = tick_time->tm_hour + (m / 60);
  m = m % 60;
*/

  int m = tick_time->tm_min;
  int h = tick_time->tm_hour;

  if (!shake_active) {
    m = ((m / 5) * 5);

    if (tick_time->tm_min % 5 > 0) {
       m += 5;
      if (m == 60) {
        m = 0;
        h++;
        if (h == 24)
          h = 0;
      }
    }
  }

  // Write the current hours and minutes into the buffer
  if(using24HourFormat == true) {
    // Use 24 hour format
    snprintf(buffer, sizeof(buffer), "%02d:%02d", h, m);
  } else {
    // Use 12 hour format
    snprintf(buffer, sizeof(buffer), "%d:%02d", (h % 13) + (h / 13), m);

  }
  strftime(dateBuffer, sizeof(dateBuffer), "%a, %b %e", tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
  if (!using24HourFormat) {
    snprintf(amPmBuffer, 3, h < 12 ? "AM" : "PM");
    text_layer_set_text(s_ampm_layer, amPmBuffer);
  }
  text_layer_set_text(s_date_layer, dateBuffer);
  
  draw_time_until();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  using24HourFormat = clock_is_24h_style();
  
  large_font = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  medium_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  small_font = fonts_load_custom_font(resource_get_handle(SMALL_FONT_ID));
  
  // Create background layer
  GRect bounds = layer_get_bounds(window_get_root_layer(window));
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 48, 144, 60));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, s_primary_color);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, large_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Create AM/PM TextLayer
  s_ampm_layer = text_layer_create(GRect(100, 22, 40, 22));
  text_layer_set_background_color(s_ampm_layer, GColorClear);
  text_layer_set_text_color(s_ampm_layer, BATTERY_COLOR);
  text_layer_set_font(s_ampm_layer, medium_font);
  text_layer_set_text(s_ampm_layer, "AM");
  text_layer_set_text_alignment(s_ampm_layer, GTextAlignmentRight);

  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(0, 105, 144, 26));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, s_primary_color);
  text_layer_set_text(s_date_layer, "");
  text_layer_set_font(s_date_layer, medium_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

  // Create battery TextLayer
  s_battery_layer = text_layer_create(GRect(4, 14, 40, 16));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, BATTERY_COLOR);
  text_layer_set_font(s_battery_layer, small_font);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);
  battery_handler(battery_state_service_peek());

  // Create time until TextLayer
  s_time_until_layer = text_layer_create(GRect(0, 168 - 16 - 4, 144, 20));
  text_layer_set_background_color(s_time_until_layer, GColorClear);
  text_layer_set_text_color(s_time_until_layer, BATTERY_COLOR);
  text_layer_set_font(s_time_until_layer, small_font);
  text_layer_set_text_alignment(s_time_until_layer, GTextAlignmentCenter);

  layer_add_child(window_get_root_layer(window), s_canvas_layer);

  // Add it as a child layer to the Window's root layer
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_battery_layer));
  layer_add_child(s_canvas_layer, text_layer_get_layer(s_time_until_layer));
  if (!using24HourFormat)
    layer_add_child(s_canvas_layer, text_layer_get_layer(s_ampm_layer));

  top_bars_layer = layer_create(GRect(0, 2, 144, 12));
  layer_set_update_proc(top_bars_layer, top_bars_update_callback);
  layer_add_child(s_canvas_layer, top_bars_layer);
 
  layer_set_hidden((Layer *)s_battery_layer, !s_show_battery);
  layer_set_hidden((Layer *)s_time_until_layer, !s_use_day_counter);

  update_time();
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_time_until_layer);
  text_layer_destroy(s_ampm_layer);
  
  layer_destroy(top_bars_layer);
  layer_destroy(s_canvas_layer);
}

static void timer_callback(void *data) {
  shake_active = false;
  update_time();
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if(axis == ACCEL_AXIS_Y) {
    shake_active = true;
    update_time();
    shake_timer = app_timer_register(10000, timer_callback, NULL);
  }
}

static void set_colors() {
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {

  // Read color preferences
  Tuple *bg_color_t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if(bg_color_t) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "background color: %d", (int)bg_color_t->value->int32);
    s_background_color = GColorFromHEX(bg_color_t->value->int32);
  }

  Tuple *p_color_t = dict_find(iter, MESSAGE_KEY_PrimaryColor);
  if(p_color_t) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "primary color: %d", (int)p_color_t->value->int32);
    s_primary_color = GColorFromHEX(p_color_t->value->int32);
  }

  Tuple *s_color_t = dict_find(iter, MESSAGE_KEY_SecondaryColor);
  if(s_color_t) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "secondary color: %d", (int)s_color_t->value->int32);
    s_secondary_color = GColorFromHEX(s_color_t->value->int32);
  }

  Tuple *s_date_t = dict_find(iter, MESSAGE_KEY_DayCounterDate);
  if(s_date_t) {
    char keyvalue[64] = "";

    APP_LOG(APP_LOG_LEVEL_DEBUG, "date: %s", s_date_t->value->cstring);
    persist_write_string(MESSAGE_KEY_DayCounterDate, s_date_t->value->cstring);
    persist_read_string(MESSAGE_KEY_DayCounterDate, keyvalue, 64);
    char year[] = "xxxx";
    char month[] = "xx";
    char day[] = "xx";
    strncpy(year, keyvalue+0, 4);
    strncpy(month, keyvalue+5, 2);
    strncpy(day, keyvalue+8, 2);
    
    s_day_counter->tm_year = atoi(year) - 1900;
    s_day_counter->tm_mon = atoi(month) - 1;
    s_day_counter->tm_mday = atoi(day);
    s_day_counter->tm_hour = 0;
    s_day_counter->tm_min = 0;
    s_day_counter->tm_sec = 0;
  }

  Tuple *s_use_day_counter_t = dict_find(iter, MESSAGE_KEY_UseDayCounter);
  if(s_use_day_counter_t) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Use day counter: %d", (int)s_use_day_counter_t->value->int32);
    s_use_day_counter = s_use_day_counter_t->value->int32 == 1;
  }

  Tuple *s_show_battery_t = dict_find(iter, MESSAGE_KEY_ShowBattery);
  if(s_show_battery_t) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Show battery percentage: %d", (int)s_show_battery_t->value->int32);
    s_show_battery = s_show_battery_t->value->int32 == 1;
  }
  
  persist_write_int(MESSAGE_KEY_BackgroundColor, bg_color_t->value->int32);
  persist_write_int(MESSAGE_KEY_PrimaryColor, p_color_t->value->int32);
  persist_write_int(MESSAGE_KEY_SecondaryColor, s_color_t->value->int32);
  persist_write_int(MESSAGE_KEY_UseDayCounter, s_use_day_counter_t->value->int32);
  persist_write_int(MESSAGE_KEY_ShowBattery, s_show_battery_t->value->int32);

  layer_mark_dirty(s_canvas_layer);
  layer_mark_dirty(top_bars_layer);

  text_layer_set_text_color(s_time_layer, s_primary_color);
  text_layer_set_text_color(s_date_layer, s_primary_color);
  text_layer_set_text_color(s_ampm_layer, s_primary_color);
  text_layer_set_text_color(s_battery_layer, s_primary_color);
  text_layer_set_text_color(s_time_until_layer, s_primary_color);
  
  layer_set_hidden((Layer *)s_battery_layer, !s_show_battery);
  layer_set_hidden((Layer *)s_time_until_layer, !s_use_day_counter);

  update_time();
}

static void load_settings() {
  s_background_color = GColorFromRGB(255, 255, 255);
  s_primary_color = MINUTE_FULL_COLOR;
  s_secondary_color = MINUTE_EMPTY_COLOR;
  s_use_day_counter = true;
  s_show_battery = true;
  
  time_t t = time(NULL);
  tm* time = localtime(&t);
  s_day_counter->tm_year = time->tm_year;
  s_day_counter->tm_mon = 11;
  s_day_counter->tm_mday = 25;
  s_day_counter->tm_hour = 0;
  s_day_counter->tm_min = 0;
  s_day_counter->tm_sec = 0;
  
  if (persist_exists(MESSAGE_KEY_BackgroundColor)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "(init) background color: %d", (int)persist_read_int(MESSAGE_KEY_BackgroundColor));
    s_background_color = GColorFromHEX(persist_read_int(MESSAGE_KEY_BackgroundColor));
  }

	if (persist_exists(MESSAGE_KEY_PrimaryColor)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "(init) primary color: %d", (int)persist_read_int(MESSAGE_KEY_PrimaryColor));
    s_primary_color = GColorFromHEX(persist_read_int(MESSAGE_KEY_PrimaryColor));
  }
  
	if (persist_exists(MESSAGE_KEY_SecondaryColor)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "(init) secondary color: %d", (int)persist_read_int(MESSAGE_KEY_SecondaryColor));
    s_secondary_color = GColorFromHEX(persist_read_int(MESSAGE_KEY_SecondaryColor));
  }

	if (persist_exists(MESSAGE_KEY_UseDayCounter)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "(init) Use day counter: %d", (int)persist_read_int(MESSAGE_KEY_UseDayCounter));
    s_use_day_counter = persist_read_int(MESSAGE_KEY_UseDayCounter) == 1;
  }

  char dateBuffer[] = "yyyymmdd";
  snprintf(dateBuffer, sizeof(dateBuffer), "%04d%02d%02d", s_day_counter->tm_year + 1900, s_day_counter->tm_mon + 1, s_day_counter->tm_mday);
  
  if (!persist_exists(MESSAGE_KEY_DayCounterDate)) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "(init) Creating new date...");
      persist_write_string(MESSAGE_KEY_DayCounterDate, dateBuffer);
  }
  char keyvalue[64] = "";
  persist_read_string(MESSAGE_KEY_DayCounterDate, keyvalue, 64);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "(init) Day counter date: %s", keyvalue);
  char year[] = "xxxx";
  char month[] = "xx";
  char day[] = "xx";
  strncpy(year, keyvalue+0, 4);
  strncpy(month, keyvalue+5, 2);
  strncpy(day, keyvalue+8, 2);

  s_day_counter->tm_year = atoi(year) - 1900;
  s_day_counter->tm_mon = atoi(month) - 1;
  s_day_counter->tm_mday = atoi(day);
  s_day_counter->tm_hour = 0;
  s_day_counter->tm_min = 0;
  s_day_counter->tm_sec = 0;
	
  if (persist_exists(MESSAGE_KEY_ShowBattery)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "(init) Show battery percentage: %d", (int)persist_read_int(MESSAGE_KEY_ShowBattery));
    s_show_battery = persist_read_int(MESSAGE_KEY_ShowBattery) == 1;
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "(init) Show battery: %s", s_show_battery ? "SHOW" : "HIDE");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "(init) Use day counter: %s", s_use_day_counter ? "SHOW" : "HIDE");
}

static void init() {
  s_day_counter = malloc(sizeof(struct tm));

  // Open AppMessage connection
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
  
  load_settings();  
  
  current_minute = 0;
  battery_state_service_subscribe(battery_handler);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register with tap service
  accel_tap_service_subscribe(tap_handler);
  
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
  free(s_day_counter);
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
