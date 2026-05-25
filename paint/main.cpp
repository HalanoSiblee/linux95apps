#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl_Value_Input.H>
#include <FL/fl_draw.H>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>

#define WIN95_GRAY fl_rgb_color(192, 192, 192)
#define WIN95_DARK_GRAY fl_rgb_color(128, 128, 128)

enum Tool { PENCIL, BRUSH, ERASER, BUCKET };

const Fl_Color hotkey_colors[] = {
    FL_BLACK, FL_RED, fl_rgb_color(255, 128, 0), FL_YELLOW,
    FL_GREEN, FL_CYAN, FL_BLUE, FL_MAGENTA, fl_rgb_color(128, 0, 128)
};

// --- DRAWING ENGINE ---
class PaintCanvas : public Fl_Box {
    int last_x, last_y;
    bool is_drawing;
public:
    int img_w, img_h;
    std::vector<uchar> pixels;
    Tool current_tool = PENCIL;
    Fl_Color primary_color = FL_BLACK;
    Fl_Color secondary_color = FL_WHITE;

    PaintCanvas(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H, "") {
        box(FL_FLAT_BOX);
        img_w = W; img_h = H;
        is_drawing = false;
        pixels.resize(W * H * 3, 255);
    }

    void resize_canvas(int new_w, int new_h) {
        // Guard against negative or insanely massive bounds crashing the memory allocator
        if (new_w < 1 || new_h < 1 || new_w > 10000 || new_h > 10000) return;

        std::vector<uchar> new_pixels(new_w * new_h * 3, 255);
        int copy_w = std::min(img_w, new_w);
        int copy_h = std::min(img_h, new_h);
        for (int y = 0; y < copy_h; ++y) {
            std::copy(pixels.begin() + (y * img_w * 3), 
                      pixels.begin() + (y * img_w * 3) + (copy_w * 3), 
                      new_pixels.begin() + (y * new_w * 3));
        }
        img_w = new_w; img_h = new_h;
        pixels = std::move(new_pixels);
        size(img_w, img_h);
        if (parent()) { 
            parent()->init_sizes(); 
            parent()->redraw(); 
        }
        redraw();
    }

    void poke_pixel(int px, int py, uchar r, uchar g, uchar b) {
        if (px >= 0 && px < img_w && py >= 0 && py < img_h) {
            int idx = (py * img_w + px) * 3;
            pixels[idx] = r; pixels[idx+1] = g; pixels[idx+2] = b;
        }
    }

    void draw_brush_point(int cx, int cy, int size, uchar r, uchar g, uchar b) {
        for (int dx = -size; dx <= size; dx++) {
            for (int dy = -size; dy <= size; dy++) {
                if (dx*dx + dy*dy <= size*size) poke_pixel(cx + dx, cy + dy, r, g, b);
            }
        }
    }

    void draw_line(int x0, int y0, int x1, int y1, int size, uchar r, uchar g, uchar b) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;
        while (true) {
            if (current_tool == PENCIL) poke_pixel(x0, y0, r, g, b);
            else draw_brush_point(x0, y0, size, r, g, b);
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void flood_fill(int start_x, int start_y, uchar target_r, uchar target_g, uchar target_b) {
        if (start_x < 0 || start_x >= img_w || start_y < 0 || start_y >= img_h) return;
        int base_idx = (start_y * img_w + start_x) * 3;
        uchar src_r = pixels[base_idx], src_g = pixels[base_idx+1], src_b = pixels[base_idx+2];
        if (src_r == target_r && src_g == target_g && src_b == target_b) return;

        std::vector<std::pair<int, int>> queue;
        queue.push_back({start_x, start_y});
        while (!queue.empty()) {
            auto coord = queue.back(); queue.pop_back();
            int cx = coord.first, cy = coord.second;
            if (cx < 0 || cx >= img_w || cy < 0 || cy >= img_h) continue;
            int idx = (cy * img_w + cx) * 3;
            if (pixels[idx] == src_r && pixels[idx+1] == src_g && pixels[idx+2] == src_b) {
                pixels[idx] = target_r; pixels[idx+1] = target_g; pixels[idx+2] = target_b;
                queue.push_back({cx + 1, cy}); queue.push_back({cx - 1, cy});
                queue.push_back({cx, cy + 1}); queue.push_back({cx, cy - 1});
            }
        }
    }

    int handle(int event) override {
        int mx = Fl::event_x() - x(); int my = Fl::event_y() - y();
        switch (event) {
            case FL_PUSH: {
                is_drawing = true; last_x = mx; last_y = my;
                uchar r, g, b;
                if (Fl::event_button() == FL_RIGHT_MOUSE || current_tool == ERASER) Fl::get_color(secondary_color, r, g, b);
                else Fl::get_color(primary_color, r, g, b);

                if (current_tool == BUCKET) flood_fill(mx, my, r, g, b);
                else {
                    int size = (current_tool == ERASER) ? 12 : 4;
                    if (current_tool == PENCIL) poke_pixel(mx, my, r, g, b);
                    else draw_brush_point(mx, my, size, r, g, b);
                }
                redraw(); return 1;
            }
            case FL_DRAG: {
                if (!is_drawing || current_tool == BUCKET) return 1;
                uchar r, g, b;
                if (Fl::event_state(FL_BUTTON3) || current_tool == ERASER) Fl::get_color(secondary_color, r, g, b);
                else Fl::get_color(primary_color, r, g, b);
                int size = (current_tool == ERASER) ? 12 : 4;
                draw_line(last_x, last_y, mx, my, size, r, g, b);
                last_x = mx; last_y = my;
                redraw(); return 1;
            }
            case FL_RELEASE: is_drawing = false; return 1;
        }
        return Fl_Box::handle(event);
    }

    void draw() override {
        fl_push_clip(x(), y(), w(), h());
        fl_draw_image(pixels.data(), x(), y(), img_w, img_h, 3, 0);
        fl_pop_clip();
    }

    void save_bmp(const char* filename) {
        std::ofstream f(filename, std::ios::binary); if (!f) return;
        int row_size = ((3 * img_w + 3) / 4) * 4; int padding = row_size - (img_w * 3);
        int size = 54 + (row_size * img_h);
        uchar f_hdr[14] = {'B','M', 0,0,0,0, 0,0,0,0, 54,0,0,0};
        f_hdr[2] = (uchar)(size); f_hdr[3] = (uchar)(size >> 8); f_hdr[4] = (uchar)(size >> 16); f_hdr[5] = (uchar)(size >> 24);
        uchar i_hdr[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
        i_hdr[4] = (uchar)(img_w); i_hdr[5] = (uchar)(img_w >> 8); i_hdr[6] = (uchar)(img_w >> 16); i_hdr[7] = (uchar)(img_w >> 24);
        i_hdr[8] = (uchar)(img_h); i_hdr[9] = (uchar)(img_h >> 8); i_hdr[10] = (uchar)(img_h >> 16); i_hdr[11] = (uchar)(img_h >> 24);
        f.write((char*)f_hdr, 14); f.write((char*)i_hdr, 40);
        uchar pad[3] = {0,0,0};
        for (int y = img_h - 1; y >= 0; y--) {
            for (int x = 0; x < img_w; x++) {
                int idx = (y * img_w + x) * 3;
                uchar bgr[3] = { pixels[idx+2], pixels[idx+1], pixels[idx] };
                f.write((char*)bgr, 3);
            }
            if (padding > 0) f.write((char*)pad, padding);
        }
    }

    void load_bmp(const char* filename) {
        std::ifstream f(filename, std::ios::binary); if (!f) return;
        char hdr[54]; f.read(hdr, 54); if (hdr[0] != 'B' || hdr[1] != 'M') return;
        int width = *(int*)&hdr[18]; int height = *(int*)&hdr[22];
        
        // Safety bounds sanity check
        if (width < 1 || height < 1 || width > 10000 || height > 10000) return;

        resize_canvas(width, height);
        int row_size = ((3 * img_w + 3) / 4) * 4; int padding = row_size - (img_w * 3);
        for (int y = img_h - 1; y >= 0; y--) {
            for (int x = 0; x < img_w; x++) {
                uchar bgr[3]; f.read((char*)bgr, 3); int idx = (y * img_w + x) * 3;
                pixels[idx] = bgr[2]; pixels[idx+1] = bgr[1]; pixels[idx+2] = bgr[0];
            }
            if (padding > 0) f.seekg(padding, std::ios::cur);
        }
        redraw();
    }
};

PaintCanvas* global_canvas = nullptr;
Fl_Box* primary_preview = nullptr;
Fl_Box* secondary_preview = nullptr;

// --- UTILITY ENGINE CALLBACKS ---
void open_cb(Fl_Widget* = nullptr, void* = nullptr) { char* f = fl_file_chooser("Open Bitmap", "*.bmp", nullptr); if (f) global_canvas->load_bmp(f); }
void save_cb(Fl_Widget* = nullptr, void* = nullptr) { char* f = fl_file_chooser("Save Bitmap", "*.bmp", "untitled.bmp"); if (f) global_canvas->save_bmp(f); }
void clear_cb(Fl_Widget* = nullptr, void* = nullptr) { if(global_canvas) { std::fill(global_canvas->pixels.begin(), global_canvas->pixels.end(), 255); global_canvas->redraw(); } }
void exit_cb(Fl_Widget* = nullptr, void* = nullptr)  { exit(0); }
void update_preview() { primary_preview->color(global_canvas->primary_color); primary_preview->parent()->redraw(); }

// Static Dialog Callbacks to completely prevent memory leaks
void dialog_ok_cb(Fl_Widget* w, void* d) {
    Fl_Value_Input** ins = (Fl_Value_Input**)d;
    global_canvas->resize_canvas((int)ins[0]->value(), (int)ins[1]->value());
    Fl_Window* win = w->window();
    win->hide();
    delete[] ins; // FIX: Deletes the heap pointer array safely
    Fl::delete_widget(win); // FIX: Tells FLTK to safely free the window widget structure
}

void dialog_cancel_cb(Fl_Widget* w, void* d) {
    Fl_Value_Input** ins = (Fl_Value_Input**)d;
    Fl_Window* win = w->window();
    win->hide();
    delete[] ins; // FIX: Prevent leak on cancel
    Fl::delete_widget(win);
}

void resize_dialog_cb(Fl_Widget*, void*) {
    Fl_Window* dialog = new Fl_Window(240, 140, "Attributes");
    dialog->set_modal(); dialog->color(WIN95_GRAY);

    Fl_Value_Input* w_in = new Fl_Value_Input(80, 20, 120, 25, "Width:");  w_in->value(global_canvas->img_w);
    Fl_Value_Input* h_in = new Fl_Value_Input(80, 55, 120, 25, "Height:"); h_in->value(global_canvas->img_h);

    Fl_Value_Input** inputs = new Fl_Value_Input*[2]{w_in, h_in};

    Fl_Button* ok = new Fl_Button(40, 100, 70, 25, "OK");
    ok->callback(dialog_ok_cb, inputs);

    Fl_Button* cancel = new Fl_Button(130, 100, 70, 25, "Cancel");
    cancel->callback(dialog_cancel_cb, inputs);

    dialog->end(); dialog->show();
}
// --- KEYBOARD & SHORTCUT ROUTER ---
class PaintWindow : public Fl_Window {
public:
    PaintWindow(int W, int H, const char* T) : Fl_Window(W, H, T) {}
    int handle(int event) override {
        if (event == FL_KEYDOWN || event == FL_SHORTCUT) {
            int key = Fl::event_key();
            int state = Fl::event_state();

            if (state & FL_CTRL) {
                if (key == 's' || key == 'S') { save_cb(); return 1; }
                if (key == 'o' || key == 'O') { open_cb(); return 1; }
                if (key == 'e' || key == 'E') { resize_dialog_cb(nullptr, nullptr); return 1; }
                if (key == 'n' || key == 'N') { clear_cb(); return 1; }
            }
            if (key == 'p' || key == 'P') { global_canvas->current_tool = PENCIL; return 1; }
            if (key == 'b' || key == 'B') { global_canvas->current_tool = BRUSH; return 1; }
            if (key == 'e' || key == 'E') { global_canvas->current_tool = ERASER; return 1; }
            if (key == 'f' || key == 'F') { global_canvas->current_tool = BUCKET; return 1; }
            if (key == FL_BackSpace)       { clear_cb(); return 1; } 
            if (key == FL_Escape || key == 'q' || key == 'Q') exit_cb();

            if (key >= '1' && key <= '9') {
                global_canvas->primary_color = hotkey_colors[key - '1'];
                update_preview(); return 1;
            }
        }
        return Fl_Window::handle(event);
    }
};

class ColorWellSelection : public Fl_Box {
public:
    ColorWellSelection(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H, "") { box(FL_BORDER_BOX); }
    int handle(int event) override {
        if (event == FL_PUSH) {
            uchar r, g, b; bool is_right = (Fl::event_button() == FL_RIGHT_MOUSE);
            Fl::get_color(is_right ? global_canvas->secondary_color : global_canvas->primary_color, r, g, b);
            if (fl_color_chooser("Color Customizer", r, g, b)) {
                Fl_Color chosen = fl_rgb_color(r, g, b);
                if (is_right) { global_canvas->secondary_color = chosen; secondary_preview->color(chosen); }
                else { global_canvas->primary_color = chosen; primary_preview->color(chosen); }
                parent()->redraw();
            }
            return 1;
        }
        return Fl_Box::handle(event);
    }
};

void palette_cb(Fl_Widget* w, long color_val) {
    if (Fl::event_button() == FL_RIGHT_MOUSE) {
        global_canvas->secondary_color = (Fl_Color)color_val; secondary_preview->color((Fl_Color)color_val);
    } else {
        global_canvas->primary_color = (Fl_Color)color_val; primary_preview->color((Fl_Color)color_val);
    }
    primary_preview->parent()->redraw();
}

int main() {
    PaintWindow* window = new PaintWindow(800, 665, "untitled - Paint");
    window->color(WIN95_GRAY);

    Fl_Menu_Bar* menu = new Fl_Menu_Bar(0, 0, 800, 25);
    menu->box(FL_FLAT_BOX); menu->color(WIN95_GRAY);
    menu->add("File/New",        FL_CTRL + 'n', clear_cb);
    menu->add("File/Open...",    FL_CTRL + 'o', open_cb);
    menu->add("File/Save As...", FL_CTRL + 's', save_cb);
    menu->add("Image/Attributes...", FL_CTRL + 'e', resize_dialog_cb);
    menu->add("File/Exit",       FL_ALT  + FL_F + 4, exit_cb);

    Fl_Group* toolbox = new Fl_Group(0, 25, 56, 640);
    toolbox->box(FL_FLAT_BOX); toolbox->color(WIN95_GRAY);
    struct { const char* label; Tool type; } tools[] = {
        {"Pencil", PENCIL}, {"Brush", BRUSH}, {"Eraser", ERASER}, {"Fill", BUCKET}
    };
    for (int i = 0; i < 4; i++) {
        Fl_Button* btn = new Fl_Button(6, 40 + (i * 30), 44, 26, tools[i].label);
        btn->box(FL_BORDER_BOX); btn->color(WIN95_GRAY);
        btn->callback([](Fl_Widget*, long t) { global_canvas->current_tool = (Tool)t; }, (long)tools[i].type);
    }
    toolbox->end();

    Fl_Group* colorbox = new Fl_Group(56, 585, 744, 80);
    colorbox->box(FL_DOWN_BOX); colorbox->color(WIN95_GRAY);

    ColorWellSelection* master_box = new ColorWellSelection(64, 595, 34, 34);
    master_box->color(WIN95_GRAY);
    secondary_preview = new Fl_Box(76, 607, 16, 16);
    secondary_preview->box(FL_FLAT_BOX); secondary_preview->color(FL_WHITE);
    primary_preview = new Fl_Box(68, 599, 16, 16);
    primary_preview->box(FL_FLAT_BOX); primary_preview->color(FL_BLACK);
    colorbox->end();

    Fl_Color classic_colors[] = {
        FL_BLACK, fl_rgb_color(128,128,128), fl_rgb_color(128,0,0), fl_rgb_color(128,128,0),
        fl_rgb_color(0,128,0), fl_rgb_color(0,128,128), fl_rgb_color(0,0,128), fl_rgb_color(128,0,128),
        FL_WHITE, fl_rgb_color(192,192,192), FL_RED, FL_YELLOW,
        FL_GREEN, FL_CYAN, FL_BLUE, FL_MAGENTA
    };
    colorbox->begin();
    for (int r = 0; r < 2; r++) {
        for (int i = 0; i < 7; i++) {
            Fl_Button* c_btn = new Fl_Button(110 + (i * 22), 595 + (r * 20), 18, 18);
            c_btn->box(FL_BORDER_BOX);
            int color_idx = (r * 7) + i;
            c_btn->color(classic_colors[color_idx]);
            c_btn->callback(palette_cb, (long)classic_colors[color_idx]);
        }
    }
    colorbox->end();

    Fl_Scroll* scroll_area = new Fl_Scroll(56, 25, 744, 560);
    scroll_area->box(FL_DOWN_BOX); scroll_area->color(WIN95_DARK_GRAY);
    global_canvas = new PaintCanvas(70, 45, 640, 480);
    scroll_area->end();

    window->end();
    window->resizable(scroll_area);
    window->show();
    return Fl::run();
}
