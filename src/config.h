#define TTY_COLOR_HIGHLIGHT TTY_COLOR_YELLOW

#define SCORE_GAP_LEADING -0.005
#define SCORE_GAP_TRAILING -0.005
#define SCORE_GAP_INNER -0.01
#define SCORE_MATCH_CONSECUTIVE 1.0
#define SCORE_MATCH_SLASH 0.9
#define SCORE_MATCH_WORD 0.8
#define SCORE_MATCH_CAPITAL 0.7
#define SCORE_MATCH_DOT 0.6

/* Time (in ms) to wait for additional bytes of an escape sequence */
#define KEYTIMEOUT 25

#define DEFAULT_TTY "/dev/tty"
#define DEFAULT_PROMPT "> "
#define DEFAULT_NUM_LINES 10
#define DEFAULT_WORKERS 0
#define DEFAULT_SHOW_INFO 0
#define DEFAULT_DELIMITER '\n'
#define DEFAULT_BENCHMARK 0
#define DEFAULT_SCORES 0
#define DEFAULT_SCROLLOFF 0
#define DEFAULT_FILTER NULL
#define DEFAULT_INIT_SEARCH NULL
#define DEFAULT_MARKER '*'
#define DEFAULT_POINTER '>'
#define DEFAULT_PAD 0
#define DEFAULT_MULTI 0
#define DEFAULT_CYCLE 0
#define DEFAULT_TAB_ACCEPTS 0
#define DEFAULT_RIGHT_ACCEPTS 0
#define DEFAULT_LEFT_ABORTS 0
#define DEFAULT_NO_COLOR 0
#define DEFAULT_REVERSE 0

#define DEFAULT_COLORS "b6b1b2b40"
#define NC "\x1b[0m" /* Reset attributes */

/* Color indices: colors (from FZY_COLORS env var) will be parsed
 * exactly in this order. See tty_interface.c */
#define PROMPT_COLOR 0
#define POINTER_COLOR 1
#define MARKER_COLOR 2
#define SEL_FG_COLOR 3
#define SEL_BG_COLOR 4
#define COLOR_ITEMS_NUM 5
#define MAX_COLOR_LEN 48

#define VERSION "1.1"
