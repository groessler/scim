#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include "sc.h"
#include "macros.h"
#include "stdout.h"
#include "string.h"
#include "color.h"

char * long_help[] = {
" This is a simple HELP page. Just press «q» to go back to spreadsheet.",
" You can use <UP> <DOWN> arrows keys, or «j» «k» keys to move throw text.",
" <SPACE> key moves forward an entire page, while <C-f> and <C-b> moves half page down or up.   ",
" «G» moves to bottom, and <C-a> to the begining of the text.                                   ",
" <ENTER> key scrolls one line down, while <DEL> key scrolls one line up, just like in less.    ",
" ----------------------------------------------------------------------------------------------",
" SCIM modes:",
"        NORMAL_MODE:    For navigation and common commands",
"        INSERT_MODE:    For entering new cell content (values and expressions)",
"                        Submodes: = < > \\",
"        EDIT_MODE:      For single line -Vi-like- edition of cell content and expressions",
"        COMMAND_MODE:   For entering special commands and for quiting app and saving files",
"        VISUAL_MODE:    For selecting a range of cells",
"        ----------------  ",
"",
"",
"        NORMAL MODE: ",
"",
"        Navigation commands: ",
"            j k l h     Move cursor down, up, right or left",
"            ^           Up to row 0 of the current column.",
"            #           Down to the last valid row of the current column.",
"            0           Back to column A.",
"            $           Forward to the last valid column of the current row.",
"            b           Back to the previous valid cell.",
"            w           Forward to the next valid cell.",
"            'c          Go to cell or select range marked previously as c. See the m command for details.",
"            gab24       Go to AB24 cell (no need to press ENTER).",
"            g0          moves to first visible column of screen.",
"            g$          moves to last visible column of screen.",
"            gM          moves to the middle column shown in screen.",
"            H           Go to first row seen on screen.",
"            L           Go to last row seen on screen.",
"            M           Go to middle row seen on screen.",
"            gg          Go to home (first cell of spreadsheet).",
"            G           Go to last valid cell of spreadsheet.",
"            gG          Go to last valid cell of spreadsheet.",
"            c-f , c-b:  Scrolls down and up full screen.",
"                        set variable half_page_scroll to 1 to scroll half page,",
"                        instead of full page. see :set command for details.",
"            c-e , c-y:  Scrolls down and up a line.",
"            zh          Scrolls left one column.",
"            zl          Scrolls right one column.",
"            zH          Scrolls left half page.",
"            zL          Scrolls right half page.",
"            zz          Scrolls up or down, until the selected cell shows up vertically in the center of screen.",
"            zm          Scrolls left or right, until the selected cell shows up in center of screen.",
"",
"",
"        Commands for handling cell content: ",
"            x , dd      Deletes the current selected cell or range and save its content in yankbuffer",
"            m           Followed by any lowercase letter, marks the current cell or the selected range",
"                        with that letter.",
"                        NOTE: When a mark is changed, all ranges that use that mark are deleted.",
"            r           Followed by two lowercase letters that represent marks of cell,",
"                        creates a range using those marks and selects it.",
"                        NOTE: If a range already exists, its replaced with new values.",
"            { } |       align cell content left, right or to center",
"",
"",
"            f+ , f-:                Change cell format. Increase or decrease decimal precision.",
"            f< , fh , f-LEFT:       Change cell format. Decrease column width.",
"            f> , fl , f-RIGHT:      Change cell format. Incrementa column width.",
"            ir          inserts a row",
"            ic          inserts a column",
"            sk          shifts up current cell or current range",
"            sj          shifts down current cell or current range",
"            sh          shifts left current cell or current range",
"            sl          shifts right current cell or current range",
"            yy          yanks selected cell",
"            y           if a range is selected, it yanks the selected range",
"            yr          yanks current row",
"            yc          yanks current column",
"            p           paste previous yanked cell or range",
"                        if yr was used to yank a row, 'p' creates a new row below, and paste content there.",
"                        if yc was used to yank a column, 'p' creates a new col at the left of curcol, and paste content there.",
"            P           if yr was used to yank a row, 'P' creates a new row above, and paste content there.",
"                        if yc was used to yank a column, 'P' creates a new col at the right, and paste there.",
"            dr          deletes row under the cursor",
"            dc          deletes column under the cursor",
"            .           repeats last command",
"",
"            u           undo last change",
"            c-f         redo last change",
"                        NOTE: Events implemented for undo / redo:", 
"                        1. delete of cell or range content",
"                        2. cell input",
"                        3. cell edition",
"                        4. cell justify",
"                        5. paste of a cell or range",
"                        6. range or cell shift with sh sj sk sl",
"                        7. insert row or column",
"                        8. delete of a row or column",
"                        9. paste of a row or column",
"",
"",
"",
"        INSERT MODE",
"            =               Enter a numeric constant or expression.",
"            <               Enter a left justified string or string expression.",
"            \\               Enter a centered label.",
"            >               Enter a right justified string or string expression.",
"            <TAB>           in insert mode, goes back to edit mode",
"            <LEFT> <RIGHT>   Cursor movement with arrows keys",
"            Keys <ENTER>, <BS>, <DELETE>, <ESC>",
"            Input of numbers, letters and operators",
"",
"",
"        EDIT MODE",
"            e           in normal mode, enters edit mode for editing numeric value of a cell",
"            E           in normal mode, enters edit mode for editing text value of a cell",
"            h           moves a char left",
"            l           moves a char right",
"            w           moves to beggining of next word",
"            e           if in end of a word, moves to end of next word. otherwise, moves to end of word under the cursor",
"            b           if in beggining of a word, moves to beggining of previous word. otherwise, moves to beggining of word under the cursor",
"            0           moves to bol",
"            $           moves to eol",
"            f{char}     moves to next occurrence of {char} to the right",
"            r{char}     replaces char under the cursor with {char}",
"            R{word}     Each character you type replaces an existing character, starting with the character under the cursor.",
"                        ESC key or ENTER key must be pressed when finished typing the new word.",
"",
"            de          deletes until end of word",
"            dw          deletes until beggining of next word",
"            db          if in beggining of a word, deletes until beggining of previous word. otherwise, deletes until beggining of word under the cursor",
"            daw         deletes word under the cursor",
"            dE          deletes until end of WORD",
"            dW          deletes until beggining of next WORD",
"            dB          if in beggining of a word, deletes until beggining of previous WORD. otherwise, deletes until beggining of WORD under the cursor",
"            daW         deletes WORD under the cursor",
"            dl          deletes char under the cursor",
"            d<RIGHT>    deletes char under the cursor",
"            dh          deletes char before the cursor",
"            d<LEFT>     deletes char before the cursor",
"",
"            ce          same as de but then enters insert mode",
"            cw          same as dw but then enters insert mode",
"            cb          same as db but then enters insert mode",
"            caw         same as daw but then enters insert mode",
"            cE          same as dE but then enters insert mode",
"            cW          same as dW but then enters insert mode",
"            cB          same as dB but then enters insert mode",
"            caW         same as daW but then enters insert mode",
"            cl          same as dl but then enters insert mode",
"            c<RIGHT>    same as d<RIGHT> but then enters insert mode",
"            ch          same as dh but then enters insert mode",
"            c<LEFT>     same as d<LEFT> but then enters insert mode",
"",
"            x           deletes char under the cursor",
"            X           deletes char before the cursor",
"            i or =      goes back to insert mode",
"            a           appends a char after the cursor",
"            s           deletes a char under the cursor and enters insert mode",
"            A           append at eol",
"            I           append at bol",
"            D           deletes entire line",
"            <SPACE>     adds a space under the cursor",
"            <ENTER>     confirm changes",
"",
"",
"        COMMAND MODE",
"            :w foo       save current spreadsheet with 'foo' filename",
"            :w           saves current spreadsheet",
"            :h           show this help",
"            :help        show this help",
"            :q           quit",
"            :quit        quit",
"            :q!          quit ignoring changes",
"            :quit!       quit ignoring changes",
"            :x           saves current spreadsheet and quit app",
"            :version     shows SCIM version number",
"",
"",
"            commandline history is stored in $HOME/.sciminfo",
"            c-p          goes back in command line history",
"            c-n          goes forward in command line history",
"            <UP>         goes back in command line history",
"            <DOWN>       goes forward in command line history",
"",
"",
"            :color <str> ",
"                         changes a color definition.",
"                         example of use:  :color type=HEADINGS bold=0 fg=BLACK bg=YELLOW",
"                         color parameters have to be one of the following:",
"                              type, fg, bg, bold, dim, reverse, stdout, underline, blink.",
"                         the first three are mandatory",
"                         type has to be one of the following:",
"                              HEADINGS, MODE, NUMBER, STRING, EXPRESSION, CELL_ERROR,",
"                              CELL_NEGATIVE, CELL_SELECTION, CELL_SELECTION_SC,",
"                              INFO_MSG, ERROR_MSG, CELL_ID, CELL_FORMAT,",
"                              CELL_CONTENT, WELCOME, NORMAL, INPUT.",
"                         fg and bg have to be one of the following: WHITE, BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN.",
"                         other parameters are booleans and its values have to be 1 or 0.",
"                         Instead of modifing a color at runtime, colors can be specified in:",
"                              a. the .scimrc file stored in your $HOME variable",
"                              b. in current .sc file.",
"                         The format is similar:",
"                         Ex.: color \"type=HEADINGS fg=BLACK bg=YELLOW bold=0\"",
"",
"",
"            :set         changes a configuration parameter",
"                         format:     :set key=value",
"                         example:    :set half_page_scroll=0 other_key=its_value",
"                         if no key is specified, shows all configuration variables and its values",
"",
"            :! cmd       executes command 'cmd' in shell",
"",
"        VISUAL MODE",
"            This mode is used for selecting a range of cells to do an operation",
"            At entering to this mode, top left limit of the selected range, and the bottom right limit is set to current row and column.",
"            The following movements commands helps to complete a selection in different ways.",
"            j k l h     Move cursor down, up, right or left",
"            0           Moves to column A",
"            $           Moves forward to the last valid column of the current row.",
"            #           Moves down to the last valid row of the current column.",
"            ^           Moves up to row 0 of the current column.",
"            'a          Moves to cell or select range marked previously as c.",
"                        See the m command for details.",
"            c-f c-b     Increase selection down or up a full screen.",
"                        set variable half_page_scroll to 1 to scroll half a page,",
"                        instead of full page. see :set command for details.",
"            c-a         Moves to first cell of spreadsheet.",
"",
"            y           yanks selected cell or range and exit VISUAL MODE.",
"            x , dd      Deletes the current range and save its content in yankbuffer",
"            H           Moves to to first row seen on screen.",
"            L           Moves to last row seen on screen.",
"            M           Moves to middle row seen on screen.",
"            w           Moves forward to the next valid cell.",
"            b           Moves back to the previous valid cell.",
"            G           Moves to last valid cell of spreadsheet.",
"",
"",
"        MAPPING",
"            mapping can be done in any SCIM file or in .scimrc file in current home directory.",
"            ex.: ",
"                imap \"d\" \"j\"     ->   d maps to 'j' letter in insert mode",
"                nmap \"d\" \"j\"     ->   d maps to j command in normal mode",
"            Some notes:",
"            Mapping is recursive.",
"            Left part of a mapping cannot contain numbers.",
"            Right part of a mapping cannot contain numbers.",
"            <ESC> cannot be mapped.",
"",
"",
"        COMMAND MULTIPLIER",
"            Commands in NORMAL MODE, VISUAL MODE or EDIT MODE can be multiplied if a number is previously typed.",
"            Ex. '4j' in NORMAL MODE, translates to 4 times 'j'.",
"            Ex. '4yr' in NORMAL MODE, yanks current row and the 3 rows below it.",
"            NOTE: the 'x' command in VISUAL MODE and shift commands in VISUAL MODE and NORMAL MODE, when a range is selected, cannot be multiplied.", 
(char *)0
};

static int pscreen(char *screen[]);

static int delta = 0;

void help() {
    set_ucolor(main_win, NORMAL);
    wtimeout(input_win, -1);
    noecho();
    curs_set(0);

    int option;
    int max = (sizeof(long_help) / sizeof(char *));

    while( (option = pscreen(long_help)) != 'q' && option != 'Q' && option != OKEY_ESC ) {
        switch (option) {
        case OKEY_ENTER:
        case OKEY_DOWN:
        case 'j':
            if (max > delta + LINES + 1) delta++;
            break;
        case OKEY_DEL:
        case OKEY_UP:
        case 'k':
            if (delta) delta--;
            break;
        case ' ':
            if (max > delta + LINES + LINES) delta += LINES;
            else if (max > delta + LINES) delta = max - 1 - LINES;
            break;
        case ctl('b'):
            if (delta - LINES/2 > 0) delta -= LINES/2;
            else if (delta) delta = 0;
            break;
        case ctl('f'):
            if (delta + LINES + LINES/2 < max) delta += LINES/2;
            else if (max > delta + LINES) delta = max - 1 - LINES;
            break;
        case 'G':
            delta = max - 1 - LINES;
            break;
        case ctl('a'):
            delta = 0;
            break;
        }
    }
    wtimeout(input_win, TIMEOUT_CURSES);
    wmove(input_win, 0,0);
    wclrtobot(input_win);
    wmove(main_win, 0,0);
    wclrtobot(main_win);
    //wrefresh(main_win);
    //refresh(input_win);
    show_header(input_win);
    update();
}

static int pscreen(char *screen[]) {
    int lineno;
    //(void) wmove(input_win, 0,0);
    //wclrtobot(input_win);
    //(void) wrefresh(input_win);

    for (lineno = 0; screen[lineno + delta] && lineno < LINES; lineno++) {
        mvwprintw(main_win, lineno, 0, screen[lineno + delta]);
        wclrtoeol(main_win);
    }
    (void) wrefresh(main_win);
    return wgetch(input_win);
}