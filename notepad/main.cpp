#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>
#include <cstdlib>
#include <string>
#include <sys/stat.h>

class Notepad : public Fl_Window {
    Fl_Text_Buffer *textBuffer;
    Fl_Text_Editor *editor;
    Fl_Menu_Bar    *menu;
    std::string    current_file;

    void update_title() {
        if (current_file.empty()) {
            label("Untitled - Notepad");
        } else {
            label((current_file + " - Notepad").c_str());
        }
    }

public:
    Notepad(int w, int h, const char* t) : Fl_Window(w, h, t), current_file("") {
        textBuffer = new Fl_Text_Buffer();
        
        menu = new Fl_Menu_Bar(0, 0, w, 30);
        menu->add("&File/&New",     FL_CTRL + 'n', (Fl_Callback*)cb_new, this);
        menu->add("&File/&Open",    FL_CTRL + 'o', (Fl_Callback*)cb_open, this);
        menu->add("&File/&Save",    FL_CTRL + 's', (Fl_Callback*)cb_save, this);
        menu->add("&File/Save &As", FL_CTRL + FL_SHIFT + 's', (Fl_Callback*)cb_save_as, this);
        menu->add("&File/&Close",   FL_CTRL + 'w', (Fl_Callback*)cb_close, this);
        menu->add("&File/E&xit",    FL_CTRL + 'q', (Fl_Callback*)cb_exit, this);
        
        menu->add("&Edit/&Undo",  FL_CTRL + 'z', (Fl_Callback*)cb_undo, this);
        menu->add("&Edit/Cu&t",   FL_CTRL + 'x', (Fl_Callback*)cb_cut, this);
        menu->add("&Edit/&Copy",  FL_CTRL + 'c', (Fl_Callback*)cb_copy, this);
        menu->add("&Edit/&Paste", FL_CTRL + 'v', (Fl_Callback*)cb_paste, this);

        menu->add("&Help/&About Notepad", 0, (Fl_Callback*)cb_about);
        
        editor = new Fl_Text_Editor(0, 30, w, h - 30);
        editor->buffer(textBuffer);
        editor->textfont(FL_COURIER);
        editor->textsize(14);
        
        this->resizable(editor);
        this->end();
        update_title();
    }

    static void cb_about(Fl_Widget*, void*) {
        fl_message_title("About Notepad");
        fl_message("FLTK Notepad\nBuild: 2026-05-10\n128MB Limit Enabled\n\n © Project linux95apps.");
    }

    static void cb_new(Fl_Widget*, Notepad* n) {
        n->textBuffer->text("");
        n->current_file = "";
        n->update_title();
    }

    static void cb_close(Fl_Widget*, Notepad* n) {
        n->textBuffer->text("");
        n->current_file = "";
        n->update_title();
    }

    static void cb_exit(Fl_Widget*, void*) { std::exit(0); }
    static void cb_undo(Fl_Widget*, Notepad* n)  { Fl_Text_Editor::kf_undo(0, n->editor); }
    static void cb_cut(Fl_Widget*, Notepad* n)   { Fl_Text_Editor::kf_cut(0, n->editor); }
    static void cb_copy(Fl_Widget*, Notepad* n)  { Fl_Text_Editor::kf_copy(0, n->editor); }
    static void cb_paste(Fl_Widget*, Notepad* n) { Fl_Text_Editor::kf_paste(0, n->editor); }

    static void cb_open(Fl_Widget*, Notepad* n) {
        Fl_Native_File_Chooser fnc;
        fnc.title("Open File");
        fnc.type(Fl_Native_File_Chooser::BROWSE_FILE);
        if (fnc.show() == 0) {
            const char* file = fnc.filename();
            struct stat st;
            if (stat(file, &st) == 0) {
                if (st.st_size > 128LL * 1024 * 1024) {
                    fl_alert("Error: File is larger than 128 MB.");
                    return;
                }
            }
            if (n->textBuffer->loadfile(file) == 0) {
                n->current_file = file;
                n->update_title();
            } else {
                fl_alert("Error loading file.");
            }
        }
    }

    static void cb_save(Fl_Widget*, Notepad* n) {
        if (n->current_file.empty()) {
            cb_save_as(NULL, n);
        } else {
            if (n->textBuffer->savefile(n->current_file.c_str()) != 0) {
                fl_alert("Error saving file.");
            }
        }
    }

    static void cb_save_as(Fl_Widget*, Notepad* n) {
        Fl_Native_File_Chooser fnc;
        fnc.title("Save File As");
        fnc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
        if (fnc.show() == 0) {
            if (n->textBuffer->savefile(fnc.filename()) == 0) {
                n->current_file = fnc.filename();
                n->update_title();
            } else {
                fl_alert("Error saving file.");
            }
        }
    }
};

int main(int argc, char **argv) {
    Notepad *win = new Notepad(640, 480, "Untitled - Notepad");
    win->show(argc, argv);
    return Fl::run();
}
