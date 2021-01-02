#include <iostream>
#include <fstream>
#include <string>
#include <curses.h>
#include <vector>

#define HELP_FLAG 1
#define ERR_FLAG -1
#define FILE_ERR_OPEN -2

int parse_arg(int argv, char ** argc);
int read_file();
void init_session();
void run();
void print(char);
char getchr();
void wait();
void refresh_dbg();

struct script {
	char * data;
	char * ptr;
	size_t data_size;
	std::vector<char> cells;
	size_t cell_ptr;
} script;

struct deb_win {
	WINDOW * cells;
	WINDOW * out;
	WINDOW * code;
	WINDOW * next;
	size_t w, h;
} deb_win;

bool debug_mode;
std::string filepath;

std::string help = "bfi [--debug/-d] [file]";
std::string error;

int parse_arg(int argc, char ** argv) {
	if (argc > 3) {
		error = "Too many arguments";
		return ERR_FLAG;
	}
	
	for (int i = 0; i < argc; ++i) {
		if (std::string(argv[i]) == "-d" || std::string(argv[i]) == "--debug")
			debug_mode = true;
		else if (std::string(argv[i]) == "-h" || std::string(argv[i]) == "--help")
			return HELP_FLAG;
		else
			filepath = argv[i];
	}

	return 0;
}

int read_file() {
	std::ifstream file(filepath, std::ios::in | std::ios::binary | std::ios::ate);

	if (file.is_open()) {
		script.data_size = file.tellg();
		script.data = new char[script.data_size];

		file.seekg(0, std::ios::beg);
		file.read(script.data, script.data_size);

		file.close();
		
		return 0;
	}

	return FILE_ERR_OPEN;
}

void init_session() {
	script.ptr = script.data;
	script.cells.assign(4, 0);
	script.cell_ptr = 0;

	if (debug_mode) {
		setlocale(LC_ALL, "");
		curs_set(0);
		initscr();
		getmaxyx(stdscr, deb_win.h, deb_win.w);
		deb_win.cells = newwin(2, deb_win.h, 0, 0);
		deb_win.out = newwin(deb_win.h - 5, deb_win.w / 2 - 1, 3, 0);
		deb_win.code = newwin(deb_win.h - 5, deb_win.w / 2, 3, deb_win.w / 2);
		deb_win.next = newwin(1, deb_win.w, deb_win.h - 1, 0);
		mvhline(2, 0, ACS_HLINE, deb_win.w);
		mvhline(deb_win.h - 2, 0, ACS_HLINE, deb_win.w);
		mvvline(3, deb_win.w / 2 - 1, ACS_VLINE, deb_win.h - 5);

		refresh();
	}
}

void print(char c) {
	if (debug_mode) {
		wprintw(deb_win.out, "%c", c);
	} else {
		std::cout << c;
	}
}

char getchr() {
	if (debug_mode) {
		wgetch(deb_win.out);
	}
	return getch();
}

void wait() {
	mvwprintw(deb_win.next, 0, 0, "Press Any Key");
	wgetch(deb_win.next);
}

void run() {
	while (script.ptr < script.data + script.data_size) {
		if (*script.ptr == '+') {
			++script.cells[script.cell_ptr];
		}
		else if (*script.ptr == '-') 
			--script.cells[script.cell_ptr];
		else if (*script.ptr == '>') {
			++script.cell_ptr;
			if (script.cell_ptr == script.cells.size())
				script.cells.emplace_back(0);
		} else if (*script.ptr == '<' && script.cell_ptr > 0)
			--script.cell_ptr;
		else if (*script.ptr == '.')
			print(script.cells[script.cell_ptr]);
		else if (*script.ptr == ',')
			script.cells[script.cell_ptr] = getchr();
		else if (*script.ptr == '[' && !script.cells[script.cell_ptr]) {
			size_t bcount = 0;
			while (*script.ptr != ']' || bcount) {
				if (*script.ptr == ']')
					--bcount;

				++script.ptr;

				if (*script.ptr == '[')
					++bcount;
			}
		} else if (*script.ptr == ']' && script.cells[script.cell_ptr]) {
			size_t bcount = 0;
			while (*script.ptr != '[' || bcount) {
				if (*script.ptr == '[')
					--bcount;

				--script.ptr;

				if (*script.ptr == ']')
					++bcount;
			}
		} else {
			++script.ptr;
			continue;
		}
		++script.ptr;
		if (debug_mode) {
			refresh_dbg();
			wait();
		}
	}

	if (debug_mode) {
		wclear(deb_win.next);
		wprintw(deb_win.next, "Press q to quit");
		while (wgetch(deb_win.next) != 'q') {
			wclear(deb_win.next);
			wprintw(deb_win.next, "Press q to quit");
		}
	}
}

void refresh_dbg() {
	wclear(deb_win.next);
	wclear(deb_win.cells);
	wclear(deb_win.code);
	wmove(deb_win.cells, 0, 0);
	wmove(deb_win.code, 0, 0);
	for (int i = 0; i < script.cells.size(); ++i) {
		wprintw(deb_win.cells, "[%i] ", static_cast<int>(script.cells[i]));
	}

	size_t p_pos = 1;

	for (int i = 0; i < script.cell_ptr; ++i) {
		p_pos += std::to_string(script.cells[i]).size() + 3;
	}

	mvwprintw(deb_win.cells, 1, p_pos, "^");

	for (char * c = script.data; c < script.data + script.data_size; ++c) {
		if (c == script.ptr - 1) {
			wattr_on(deb_win.code, A_STANDOUT, NULL);
			wprintw(deb_win.code, "%c", *c);
			wattr_off(deb_win.code, A_STANDOUT, NULL);
		} else {
			wprintw(deb_win.code, "%c", *c);
		}
	}

	wrefresh(deb_win.cells);
	wrefresh(deb_win.out);
	wrefresh(deb_win.code);
}

int main(int argc, char ** argv) {

	int err = parse_arg(argc, argv);
	if (err) {
		if (err == HELP_FLAG)
			std::cout << help << '\n';
		else
			std::cerr << error << '\n';
		return err;
	}

	err = read_file();
	if (err == FILE_ERR_OPEN) {
		std::cerr << "Cannot open file: " << filepath << '\n';
		return FILE_ERR_OPEN;
	}

	init_session();
	run();

	if (debug_mode)
		endwin();
	
	return 0;
}
