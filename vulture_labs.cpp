/**************************************************************
  SENA FAMILY FOUNDATION, LLC
  Vulture Labs Ops Console — Vintage ANSI (CIA-style) Terminal
  Storage: ..\data\opslog.ndjson (append-only NDJSON)
  Build:   g++ -std=c++17 vulture_labs.cpp -o vulture_labs.exe
***************************************************************/

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>     // for std::numeric_limits
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
  #include <windows.h>
#endif

// ---------- ANSI / VT helpers ----------
static inline void enable_vt() {
#if defined(_WIN32)
  SetConsoleOutputCP(65001);
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (GetConsoleMode(hOut, &mode)) {
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
  }
#endif
}

static inline void ansi_reset()      { std::cout << "\x1b[0m"; }
static inline void ansi_bright()     { std::cout << "\x1b[1m"; }
static inline void ansi_dim()        { std::cout << "\x1b[2m"; }
static inline void ansi_cls()        { std::cout << "\x1b[2J\x1b[H"; }
static inline void ansi_green()      { std::cout << "\x1b[32m"; }
static inline void ansi_cyan()       { std::cout << "\x1b[36m"; }
static inline void ansi_yellow()     { std::cout << "\x1b[33m"; }
static inline void ansi_red()        { std::cout << "\x1b[31m"; }

static const char* kLogPath = "..\\data\\opslog.ndjson";

// ---------- Small utils ----------
static std::string now_ts() {
  std::time_t t = std::time(nullptr);
  std::tm tmv{};
#if defined(_WIN32)
  localtime_s(&tmv, &t);
#else
  localtime_r(&t, &tmv);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
  return std::string(buf);
}

static std::string json_escape(const std::string& s) {
  std::string o; o.reserve(s.size()+8);
  for (char c : s) {
    switch (c) {
      case '\\': o += "\\\\"; break;
      case '\"': o += "\\\""; break;
      case '\n': o += "\\n"; break;
      case '\r': o += "\\r"; break;
      case '\t': o += "\\t"; break;
      default:   o += c; break;
    }
  }
  return o;
}

static std::string trim(const std::string& s){
  size_t a=s.find_first_not_of(" \t\r\n"), b=s.find_last_not_of(" \t\r\n");
  if(a==std::string::npos) return "";
  return s.substr(a,b-a+1);
}

static void ignore_rest_of_line() {
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// ---------- Persistence ----------
static void write_event(const std::string& category, const std::string& detail) {
  std::ofstream f(kLogPath, std::ios::app);
  if (!f) {
    ansi_red(); std::cout << "!! Unable to open log file at " << kLogPath << "\n"; ansi_reset();
    return;
  }
  f << "{"
    << "\"ts\":\"" << now_ts() << "\","
    << "\"category\":\"" << json_escape(category) << "\","
    << "\"detail\":\"" << json_escape(detail) << "\"}\n";
}

struct Row {
  std::string ts;
  std::string category;
  std::string detail;
};

static std::vector<Row> read_last(int limit = 20) {
  std::ifstream f(kLogPath);
  std::vector<Row> out;
  if (!f) return out;
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty()) continue;

    Row r;
    auto grab = [&](const char* key)->std::string{
      std::string pat = std::string("\"") + key + "\":\"";
      size_t p = line.find(pat);
      if (p == std::string::npos) return "";
      size_t s = p + pat.size();
      size_t e = line.find("\"", s);
      if (e == std::string::npos) return "";
      return line.substr(s, e - s);
    };
    r.ts       = grab("ts");
    r.category = grab("category");
    r.detail   = grab("detail");

    if (!r.ts.empty() || !r.category.empty() || !r.detail.empty())
      out.push_back(r);

    if ((int)out.size() > limit) {
      out.erase(out.begin()); // keep last N
    }
  }
  return out;
}

// ---------- Theming ----------
static void banner() {
  ansi_green(); ansi_bright();
  std::cout << "===============================================================================\n";
  std::cout << "=                      SENA FAMILY FOUNDATION, LLC                           =\n";
  std::cout << "=                         VULTURE LABS OPS CONSOLE                           =\n";
  std::cout << "=                  (c) 2022-2025  INTERNAL USE ONLY                          =\n";
  std::cout << "===============================================================================\n";
  ansi_reset();
}

static void menu_frame() {
  ansi_cyan(); ansi_bright();
  std::cout << "===============================================================================\n";
  std::cout << "=                               VULTURE LABS OPS MENU                        =\n";
  std::cout << "=------------------------------------------------------------------------------=\n";
  std::cout << "=  1) ENTER LOG    2) VIEW LOG (LAST 20)    3) CLEAR SCREEN    4) EXIT         =\n";
  std::cout << "===============================================================================\n";
  ansi_reset();
}

static void view_header(const std::string& title) {
  ansi_green(); ansi_bright();
  std::cout << "===============================================================================\n";
  std::cout << "=  " << title;
  size_t len = title.size();
  for (size_t i=len; i<75; ++i) std::cout << " ";
  std::cout << "=\n";
  std::cout << "===============================================================================\n";
  ansi_reset();
}

static void table_header() {
  ansi_dim(); ansi_green();
  std::cout << "| " << std::left << std::setw(19) << "Timestamp"
            << " | " << std::setw(10) << "Category"
            << " | " << std::setw(43) << "Detail" << "|\n";
  std::cout << "|---------------------+------------+-------------------------------------------|\n";
  ansi_reset();
}

static void color_for(const std::string& cat) {
  std::string c = cat; 
  std::transform(c.begin(), c.end(), c.begin(), [](unsigned char ch){ return (char)std::tolower(ch); });
  if (c == "boot" || c == "created" || c == "deploy" || c == "deployed" || c == "action") {
    ansi_cyan(); return; // operational / lifecycle
  }
  if (c == "note" || c == "info" || c == "statuschange") {
    ansi_yellow(); return; // informational
  }
  if (c == "security" || c == "alert" || c == "error" || c == "fail") {
    ansi_red(); return; // hot items
  }
  ansi_green(); // default
}

// ---------- Screens ----------
static void view_logs_screen() {
  auto rows = read_last(20);
  view_header("VIEW LOG (LAST 20 ENTRIES)");
  table_header();

  for (auto& r : rows) {
    // Timestamp
    ansi_green();
    std::cout << "| " << std::left << std::setw(19) << (r.ts.size()>=19 ? r.ts.substr(0,19) : r.ts);
    std::cout << " | ";

    // Category (colored)
    color_for(r.category);
    std::cout << std::left << std::setw(10) << (r.category.empty() ? "-" : r.category.substr(0,10));
    ansi_reset();
    std::cout << " | ";

    // Detail
    ansi_green();
    std::string d = r.detail;
    if (d.size() > 43) d = d.substr(0,43) + "…";
    std::cout << std::left << std::setw(43) << d;
    std::cout << "|\n";
    ansi_reset();
  }

  ansi_green(); std::cout << "===============================================================================\n"; ansi_reset();
  std::cout << "Press ENTER to continue..."; 
  ignore_rest_of_line();
}

static void enter_log_screen() {
  ansi_cls(); banner();
  ansi_green(); ansi_bright(); std::cout << "ENTER LOG\n\n"; ansi_reset();

  std::string category, detail;
  std::cout << "Category (e.g., Boot, Created, Note, Action, Security): ";
  std::getline(std::cin, category);
  category = trim(category); if (category.empty()) category = "Note";

  std::cout << "Detail: ";
  std::getline(std::cin, detail);
  detail = trim(detail); if (detail.empty()) detail = "-";

  write_event(category, detail);

  ansi_cyan(); std::cout << "\n✔ Event written.\n"; ansi_reset();
  std::cout << "\nPress ENTER to return..."; 
  ignore_rest_of_line();
}

int main() {
  enable_vt();
  ansi_cls();
  banner();

  // boot record
  write_event("Boot", "Application started");

  while (true) {
    menu_frame();
    std::cout << "Selection> ";
    int choice = 0;
    if (!(std::cin >> choice)) {
      std::cin.clear();
      ignore_rest_of_line();
      ansi_red(); std::cout << "Invalid input.\n"; ansi_reset();
      continue;
    }
    ignore_rest_of_line();

    if (choice == 1) {
      enter_log_screen();
      ansi_cls(); banner();
    } else if (choice == 2) {
      ansi_cls(); view_logs_screen();
      ansi_cls(); banner();
    } else if (choice == 3) {
#if defined(_WIN32)
      system("cls");
#else
      system("clear");
#endif
      banner();
    } else if (choice == 4) {
      write_event("Action", "Exit selected");
      ansi_green(); std::cout << "Goodbye.\n"; ansi_reset();
      break;
    } else {
      ansi_yellow(); std::cout << "Use 1/2/3/4.\n"; ansi_reset();
    }
  }
  return 0;
}
