#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;

#define USE_SECONDS 0

#define SETTINGS_KEY 1

typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor TextColor;
  bool TemperatureUnit; // false = Celsius, true = Fahrenheit
  bool ShowDate;
} ClaySettings;

static ClaySettings settings;

static void prv_default_settings() {
  settings.BackgroundColor = GColorBlack;
  settings.TextColor = GColorWhite;
  settings.TemperatureUnit = false;
  settings.ShowDate = true;
}

static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void prv_load_settings() {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}


// Blocks
// 0: empty
// 1: square
// 2: round, top left
// 3: round, top right
// 4: round, bottom right
// 5: round, bottom left
// 6: diagonal, top left
// 7: diagonal, top right
// 8: diagonal, bottom right
// 9: diagonal, bottom left
// 10: notch, left
// 11: notch, right

static const uint16_t DIGITS[10][15]  = {
    { 2, 1, 3, 1, 0, 1, 1, 0, 1, 1, 0, 1, 5, 1, 4 }, // 0
    { 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1 }, // 1
    { 2, 1, 3, 0, 0, 1, 6, 1, 8, 1, 0, 0, 1, 1, 1 }, // 2
    { 2, 1, 3, 0, 0, 1, 0, 1, 11, 0, 0, 1, 5, 1, 4 }, // 3
    { 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1 }, // 4
    { 1, 1, 1, 1, 0, 0, 1, 1, 7, 0, 0, 1, 5, 1, 4 }, // 5
    { 2, 1, 3, 1, 0, 0, 1, 1, 7, 1, 0, 1, 5, 1, 4 }, // 6
    { 1, 1, 1, 0, 6, 8, 6, 8, 0, 1, 0, 0, 1, 0, 0 }, // 7
    { 2, 1, 3, 1, 0, 1, 10, 1, 11, 1, 0, 1, 5, 1, 4 }, // 8
    { 2, 1, 3, 1, 0, 1, 9, 1, 1, 0, 0, 1, 5, 1, 4 } // 9
};

static void draw_block(GContext *ctx, uint16_t x, uint16_t y, uint16_t size, uint16_t block) {
    if (block == 0) {
        return;
    }
    if (block == 1) {        
        graphics_fill_rect(ctx, GRect(x, y, size, size), 0, GCornerNone);
        return;
    }
    uint16_t size_s = size - 1;
    uint16_t threshold = size_s * size_s + size_s;
    if (block == 2) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if ((size_s - px) * (size_s - px) + (size_s - py) * (size_s - py) <= threshold) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }
    if (block == 3) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if (px * px + (size_s - py) * (size_s - py) <= threshold) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }
    if (block == 4) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if (px * px + py * py <= threshold) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }
    if (block == 5) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if ((size_s - px) * (size_s - px) + py * py <= threshold) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }
    if (block == 6) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if (px + py >= size - 1) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }
    if (block == 7) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if (px - py <= 0) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }
    if (block == 8) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if (px + py < size) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }
    if (block == 9) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if (py - px <= 0) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }
    if (block == 10) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if (px + py >= size - 1 || py - px <= 0) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }
    if (block == 11) {
        for (uint16_t px = 0; px < size; px++) {
            for (uint16_t py = 0; py < size; py++) {
                if (px - py <= 0 || px + py < size) {
                    graphics_draw_pixel(ctx, GPoint(x + px, y + py));
                }
            }
        }
        return;
    }

    graphics_fill_rect(ctx, GRect(x, y, size, size), 0, GCornerNone);
}

static void draw_digit(GContext *ctx, uint16_t x, uint16_t y, uint16_t size, uint16_t digit) {
    for (uint16_t i = 0; i < 15; i++) {
        uint16_t local_x = x + (i % 3) * (size + 1);
        uint16_t local_y = y + (i / 3) * (size + 1);
        uint16_t block_value = DIGITS[digit][i];
        draw_block(ctx, local_x, local_y, size, block_value);
    }
}

static void update_canvas(Layer *layer, GContext *ctx)
{
    time_t temp_time = time(NULL);
    tm current_time = *localtime(&temp_time);

    // Fill background black
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorWhite);

    #if USE_SECONDS
        const bool show_seconds = true;
    #else
        const bool show_seconds = false;
    #endif

    const uint16_t gap = 3;
    const uint16_t minute_second_gap = 3;
    uint16_t size;
    uint16_t x_position;
    if (show_seconds) {
        size = (bounds.size.w - gap * 5 - PBL_IF_ROUND_ELSE(bounds.size.w / 16, 0)) * 5 / 78 - 1;
        x_position = (bounds.size.w - (size + 1) * 78 / 5 - gap * 5 - minute_second_gap) / 2;
    } else {
        size = (bounds.size.w - gap * 3 - PBL_IF_ROUND_ELSE(bounds.size.w / 10, 0)) / 12 - 1;
        x_position = (bounds.size.w - (size + 1) * 12 - gap * 3) / 2;
    }
    uint16_t spacing = (size + 1) * 3 + gap;
    uint16_t y_position = (bounds.size.h - (size + 1) * 5) / 2;

    draw_digit(ctx, x_position + spacing * 0, y_position, size, current_time.tm_hour / 10);
    draw_digit(ctx, x_position + spacing * 1, y_position, size, current_time.tm_hour % 10);
    draw_digit(ctx, x_position + spacing * 2, y_position, size, current_time.tm_min / 10);
    draw_digit(ctx, x_position + spacing * 3, y_position, size, current_time.tm_min % 10);

    if (show_seconds) {
        uint16_t seconds_size = size * 3 / 5;
        x_position = x_position + spacing * 4 + minute_second_gap;
        y_position = (bounds.size.h + (size + 1) * 5) / 2 - (seconds_size + 1) * 5;
        spacing = (seconds_size + 1) * 3 + gap;
        draw_digit(ctx, x_position + spacing * 0, y_position, seconds_size, current_time.tm_sec / 10);
        draw_digit(ctx, x_position + spacing * 1, y_position, seconds_size, current_time.tm_sec % 10);
    }
}

static void main_window_load(Window *window)
{
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, update_canvas);
    layer_add_child(window_layer, s_canvas_layer);
}

static void main_window_unload(Window *window)
{
    layer_destroy(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    layer_mark_dirty(s_canvas_layer);
}

static void init(void)
{
    prv_load_settings();
    
    s_main_window = window_create();
    window_set_background_color(s_main_window, GColorBlack);

    window_set_window_handlers(s_main_window, (WindowHandlers){
                                                  .load = main_window_load,
                                                  .unload = main_window_unload});

    window_stack_push(s_main_window, true);
    #ifdef USE_SECONDS
        tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
    #elif
        tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    #endif
}

static void deinit(void)
{
    window_destroy(s_main_window);
    tick_timer_service_unsubscribe();
}

int main(void)
{
    init();
    app_event_loop();
    deinit();
}