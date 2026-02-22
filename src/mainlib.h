#include <string>

// Init everything driver needs.
// argc/argv are passed for command line argument parsing (-m, -D, -p)
struct event_base* init_main(std::string_view config_file, int argc, char** argv);
void setup_signal_handlers();
std::string get_argument(unsigned int pos, int argc, char** argv);
