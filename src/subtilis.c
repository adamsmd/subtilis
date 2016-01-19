#include <pebble.h>

static Window *window;
static TextLayer *time_layer;
static TextLayer *date_layer;
static Layer *seconds_layer;
static struct tm current_time;

static void handle_seconds_tick(struct tm* tick_time, TimeUnits units_changed) {
  current_time = *tick_time;

  static char date_text[16] = "Sun 00/00/0000"; // Needs to be static because it's used by the system later. TODO: init with null at end?
  strftime(date_text, sizeof(date_text), "%a  %m/%d/%Y", tick_time);
  text_layer_set_text(date_layer, date_text);

  static char time_text[16] = "00\n00"; // Needs to be static because it's used by the system later.
  strftime(time_text, sizeof(time_text), "%l\n%M", tick_time);
  text_layer_set_text(time_layer, time_text);

  layer_mark_dirty(seconds_layer);
}

void seconds_update_proc(struct Layer *layer, GContext *gc) {
  graphics_context_set_stroke_color(gc, GColorWhite);
  graphics_context_set_fill_color(gc, GColorWhite);
  int width = 5;
  int height = 2;
  int column = 144 / 2+2;
  int row = 4;
  for (int i = 1; i <= 60; i++) {
    if (i <= 1) { column += 0; row += 0; }
    else if (i <= 8) { column += 9; row += 0; }
    else if (i <= (8 + 15)) { column += 0; row += 9; }
    else if (i <= (8 + 30)) { column -= 9; row -= 0; }
    else if (i <= (8 + 45)) { column -= 0; row -= 9; }
    else { column += 9; row += 0; }
    if ((i <= current_time.tm_sec) /*^ (current_time.tm_min % 2 != 0)*/) { graphics_draw_rect(gc, (GRect) { .origin = { column, row }, .size = { width, height } }); }
    if (i == 56 || i == 1 || i == 6) { graphics_fill_rect(gc, (GRect) { .origin = { column - 3, row + height }, .size = { 2, 7 } }, 0, GCornerNone); }
    if (i == 11 || i == 16 || i == 21) { graphics_fill_rect(gc, (GRect) { .origin = { column - 7, row - 5 }, .size = { 7, 3 } }, 0, GCornerNone); }
    if (i == 26 || i == 31 || i == 36) { graphics_fill_rect(gc, (GRect) { .origin = { column - 3 + 9, row - 7 }, .size = { 2, 7 } }, 0, GCornerNone); }
    if (i == 41 || i == 46 || i == 51) { graphics_fill_rect(gc, (GRect) { .origin = { column + width, row - 5 + 9 }, .size = { 7, 3 } }, 0, GCornerNone); }
  }

  // center between: 8 vertical pixels between "second" ticks and date "slashes"
  BatteryChargeState current_charge_state = battery_state_service_peek();
  for (int i = 0; i * 10 < current_charge_state.charge_percent; i++) {
    //12 + 2 == 120+18 == 138 == 144-6
    graphics_fill_rect(gc, (GRect) { .origin = { 4 + i * 14, 168 - 2 /*- (24+3) + 3*/}, .size = { 10, 2 } }, 0, GCornerNone);
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  date_layer = text_layer_create(GRect(0, 168 - (24+3) - 5, 144, 24+3));
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_layer));

  //time_layer = text_layer_create(GRect(43, 17, 144 - 2*43, 168));
  time_layer = text_layer_create(GRect(0, 4, 144 - 38, 168));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorWhite);
  //text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD/*FONT_KEY_ROBOTO_BOLD_SUBSET_49*/));
  text_layer_set_font(time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ROBOTO_BLACK_60)));
  text_layer_set_text_alignment(time_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

// 144 × 168 pixel
// (GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } }
  seconds_layer = layer_create(layer_get_bounds(window_layer));
  layer_set_update_proc(seconds_layer, seconds_update_proc);
  layer_add_child(window_layer, seconds_layer);

  time_t t = time(NULL);
  handle_seconds_tick(localtime(&t), SECOND_UNIT);
  tick_timer_service_subscribe(SECOND_UNIT, &handle_seconds_tick);
}

static void window_unload(Window *window) {
  text_layer_destroy(date_layer);
  text_layer_destroy(time_layer);
  layer_destroy(seconds_layer);
}

static void init(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, false /*animated*/);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
