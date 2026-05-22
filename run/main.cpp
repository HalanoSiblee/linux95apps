#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>

#define WIN_W 400
#define WIN_H 100
#define MAX_CMDS 10000
#define MAX_PATH_LEN 1024

char *cmds[MAX_CMDS];
int cmd_count = 0;

void cleanup_cmds() {
    for (int i = 0; i < cmd_count; i++) {
        free(cmds[i]);
    }
    cmd_count = 0;
}

void scan_path() {
    char *env = getenv("PATH");
    if (!env) return;
    char *path = strdup(env), *dir = strtok(path, ":");
    while (dir && cmd_count < MAX_CMDS) {
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *de;
            while ((de = readdir(d)) && cmd_count < MAX_CMDS) {
                if (de->d_name[0] != '.') cmds[cmd_count++] = strdup(de->d_name);
            }
            closedir(d);
        }
        dir = strtok(NULL, ":");
    }
    free(path);
}

int is_dir(const char *path) {
    struct stat s;
    if (stat(path, &s) == 0) {
        return S_ISDIR(s.st_mode);
    }
    return 0;
}

void execute_and_mutate(const char *val) {
    Fl::first_window()->hide();
    Fl::flush();

    int fd = open("/dev/null", O_WRONLY);
    if (fd >= 0) {
        dup2(fd, 1);
        dup2(fd, 2);
        close(fd);
    }
    close(0);

    char exec_str[MAX_PATH_LEN];
    if (is_dir(val)) {
        snprintf(exec_str, sizeof(exec_str), "exec xdg-open \"%s\"", val);
    } else {
        snprintf(exec_str, sizeof(exec_str), "exec %s", val);
    }

    char *args[] = {(char*)"/bin/sh", (char*)"-c", exec_str, NULL};
    
    execvp(args[0], args);

    _exit(1);
}

class RunInput : public Fl_Input {
public:
    RunInput(int X, int Y, int W, int H, const char *L = 0) : Fl_Input(X, Y, W, H, L) {}

    int handle(int event) override {
        if (event == FL_KEYBOARD) {
            int key = Fl::event_key();
            int ctrl = Fl::event_state(FL_CTRL);

            if (key == FL_Enter) {
                const char *val = value();
                if (val && strlen(val) > 0) {
                    execute_and_mutate(val);
                }
                exit(0);
            }
            
            if (key == FL_Escape) exit(0);

            if (key == FL_Tab) {
                const char *val = value();
                int len = strlen(val);
                if (len == 0) return 1;

                // PATH AUTO-COMPLETE (Starts with /)
                if (val[0] == '/') {
                    // Create working copies because dirname/basename modify strings inline
                    char *dir_buf = strdup(val);
                    char *base_buf = strdup(val);
                    
                    char *d_name = dirname(dir_buf);
                    char *b_name = basename(base_buf);
                    
                    // Handle edge case where user just typed '/' or is at root level
                    if (strcmp(b_name, "/") == 0 || val[len - 1] == '/') {
                        b_name = (char*)""; 
                        d_name = (char*)val;
                    }

                    DIR *d = opendir(d_name);
                    if (d) {
                        struct dirent *de;
                        int b_len = strlen(b_name);
                        while ((de = readdir(d))) {
                            // Skip hidden dotfiles
                            if (de->d_name[0] == '.' && b_name[0] != '.') continue;

                            if (b_len == 0 || strncmp(b_name, de->d_name, b_len) == 0) {
                                char completed_path[MAX_PATH_LEN];
                                // Construct the clean full path string
                                if (strcmp(d_name, "/") == 0) {
                                    snprintf(completed_path, sizeof(completed_path), "/%s", de->d_name);
                                } else {
                                    snprintf(completed_path, sizeof(completed_path), "%s/%s", d_name, de->d_name);
                                }

                                // If the match is a directory, append a trailing slash for immediate deep navigation
                                struct stat s;
                                if (stat(completed_path, &s) == 0 && S_ISDIR(s.st_mode)) {
                                    strncat(completed_path, "/", sizeof(completed_path) - strlen(completed_path) - 1);
                                }

                                value(completed_path);
                                insert_position(strlen(completed_path));
                                break;
                            }
                        }
                        closedir(d);
                    }
                    free(dir_buf);
                    free(base_buf);
                    return 1;
                } 
                
                else {
                    for (int i = 0; i < cmd_count; i++) {
                        if (strncmp(val, cmds[i], len) == 0) {
                            value(cmds[i]);
                            insert_position(strlen(cmds[i])); 
                            return 1;
                        }
                    }
                }
                return 1; 
            }

            if (ctrl && key == 'a') {
                insert_position(0, strlen(value()));
                return 1;
            }
        }
        return Fl_Input::handle(event);
    }
};

int main() {
    scan_path();

    int x = 12; 
    int y = Fl::h() - WIN_H - 45; 

    Fl_Window *win = new Fl_Window(x, y, WIN_W, WIN_H, "Run");
    win->color(fl_rgb_color(0xd4, 0xd0, 0xc8));

    Fl_Box *label = new Fl_Box(20, 15, 360, 20, "Type the name of a program or directory:");
    label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    label->labelsize(12);

    RunInput *input = new RunInput(20, 45, 360, 25);
    input->textsize(12);
    input->take_focus();

    win->size_range(WIN_W, WIN_H, WIN_W, WIN_H);
    win->end();
    win->show();

    int result = Fl::run();
    cleanup_cmds();
    return result;
}
