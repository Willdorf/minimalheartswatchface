#include <pebble.h>

#define KEY_BACKGROUND_COLOR 0
#define KEY_HEART_COLOR 1

static Window *window;
static Layer *s_layer;
static TextLayer *s_time_layer;

static GColor background_color;
static GColor heart_color;

static uint8_t s_hour;
static uint8_t s_min;

static const GPathInfo HEART_INFO = {
	.num_points = 8,
	.points = (GPoint []) {{72,34},{74,32},{75,33},{75,35},{72,38},{69,35},{69,33},{70,32}}
};

static GPath *heart = NULL;

static uint8_t s_sec;

static void update_time(struct tm *tick_time) {
	s_hour = tick_time->tm_hour;
	s_min = tick_time->tm_min;
	s_sec = tick_time->tm_sec;
	layer_mark_dirty(s_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	update_time(tick_time);

	static char buffer[] = "00";

	//update minutes
	strftime(buffer, sizeof("00"), "%M", tick_time);
	text_layer_set_text(s_time_layer, buffer);
}

static void draw_watchface(Layer *layer, GContext *ctx) {
	graphics_context_set_stroke_color(ctx, gcolor_legible_over(background_color));
	graphics_context_set_fill_color(ctx, COLOR_FALLBACK(heart_color, GColorRed)); 

	//top heart
	gpath_move_to(heart, GPoint(0,0));
	gpath_draw_outline(ctx, heart);
	gpath_draw_filled(ctx, heart);

	gpath_move_to(heart, GPoint(36,49));
	gpath_draw_outline(ctx, heart);
	gpath_draw_filled(ctx, heart);

	gpath_move_to(heart, GPoint(0,97));
	gpath_draw_outline(ctx, heart);
	gpath_draw_filled(ctx, heart);

	gpath_move_to(heart, GPoint(-36,49));
	gpath_draw_outline(ctx, heart);
	gpath_draw_filled(ctx, heart);

	//position the minutes
	uint8_t cur_hour = s_hour % 12;
	if (cur_hour == 0) {
		cur_hour = 12;
	}

	GRect f = layer_get_frame((Layer *) s_time_layer);
	int shift = 6;
	switch (cur_hour) {
		case 12:
			f.origin.x = 59;
			f.origin.y = 13 - shift;
			break;
		case 1:
			f.origin.x = 84;
			f.origin.y = 24 - shift;
			break;
		case 2:
			f.origin.x = 105;
			f.origin.y = 48 - shift;
			break;
		case 3:
			f.origin.x = 113;
			f.origin.y = 72 - shift;
			break;
		case 4:
			f.origin.x = 105;
			f.origin.y = 96 - shift;
			break;
		case 5:
			f.origin.x = 84;
			f.origin.y = 120 - shift;
			break;
		case 6:
			f.origin.x = 59;
			f.origin.y = 131 - shift;
			break;
		case 7:
			f.origin.x = 34;
			f.origin.y = 120 - shift;
			break;
		case 8:
			f.origin.x = 13;
			f.origin.y = 96 - shift;
			break;
		case 9:
			f.origin.x = 5;
			f.origin.y = 72 - shift;
			break;
		case 10:
			f.origin.x = 12;
			f.origin.y = 48 - shift;
			break;
		case 11:
			f.origin.x = 34;
			f.origin.y = 24 - shift;
			break;
		default:
			APP_LOG(APP_LOG_LEVEL_DEBUG, "invalid hour %d", cur_hour);
	}
	layer_set_frame((Layer *) s_time_layer, f);
}

static void setup_paths() {
	heart = gpath_create(&HEART_INFO);
}

static void set_background_and_text_color(int color) {
	background_color = GColorFromHEX(color);
	window_set_background_color(window, background_color);
	text_layer_set_text_color(s_time_layer, gcolor_legible_over(background_color));
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "inbox received handler");
	Tuple *background_color_t = dict_find(iter, KEY_BACKGROUND_COLOR);
	Tuple *heart_color_t = dict_find(iter, KEY_HEART_COLOR);

	if (background_color_t) {
		int bc = background_color_t->value->int32;

		persist_write_int(KEY_BACKGROUND_COLOR, bc);

		set_background_and_text_color(bc);

		APP_LOG(APP_LOG_LEVEL_DEBUG, "background color: %d", bc);
	}

	if (heart_color_t) {
		int hc = heart_color_t->value->int32;

		persist_write_int(KEY_HEART_COLOR, hc);

		heart_color = GColorFromHEX(hc);

		APP_LOG(APP_LOG_LEVEL_DEBUG, "heart color: %d", hc);
	}

	//update
	time_t start_time = time(NULL);
	update_time(localtime(&start_time));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_layer = layer_create(layer_get_bounds(window_get_root_layer(window)));
  layer_add_child(window_get_root_layer(window), s_layer);
  layer_set_update_proc(s_layer, draw_watchface);

  //text layer for displaying the minutes
  s_time_layer = text_layer_create(GRect(0,0,24,24));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  setup_paths();

  if (persist_read_int(KEY_BACKGROUND_COLOR)) {
	  set_background_and_text_color(persist_read_int(KEY_BACKGROUND_COLOR));
  } else {
	  background_color = GColorVividCerulean;
  }

  if (persist_read_int(KEY_HEART_COLOR)) {
	  heart_color = GColorFromHEX(persist_read_int(KEY_HEART_COLOR));
  } else {
	  heart_color = GColorRed;
  }

}

static void window_unload(Window *window) {
	text_layer_destroy(s_time_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  //Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  //display the time right away
  time_t start_time = time(NULL);
  update_time(localtime(&start_time));
}

static void deinit(void) {
  window_destroy(window);
  tick_timer_service_unsubscribe();
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}