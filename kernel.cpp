__asm("jmp kmain");

#define VIDEO_BUF_PTR (0xb8000)

//обработчик прерываний 
#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)

// Селектор секции кода, установленный загрузчиком ОС
#define GDT_CS (0x8)

//обработчик прерываний клавиатуры 
#define PIC1_PORT (0x20)

// Базовый порт управления курсором текстового экрана
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80) //ширина текстового экрана

#define NULL 0
#define UCHAR_MAX 255

//manual
// Структура описывает данные об обработчике прерывания 
struct idt_entry {
    unsigned short base_lo;
    // Младшие биты адреса обработчика
    unsigned short segm_sel;
    // Селектор сегмента кода
    unsigned char always0;
    // Этот байт всегда 0
    unsigned char flags;
    // Флаги тип. Флаги: P, DPL, Типы - это константы - IDT_TYPE...
    unsigned short base_hi;
    // Старшие биты адреса обработчика
} __attribute__((packed)); // Выравнивание запрещено

//manual
// Структура, адрес которой передается как аргумент команды lidt
struct idt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)); // Выравнивание запрещено

typedef void (*intr_handler)();

struct idt_entry g_idt[256]; // Реальная таблица IDT
struct idt_ptr g_idtp; // Описатель таблицы для команды lidt

unsigned int global_str = 0;
unsigned int global_pos = 0;


//флаги
bool shift = false;
bool boot_options = false;
bool flag = true;

int str_len = 0;

unsigned char temp_str[70];
unsigned int temp_len = 0;
int shifts[256];
bool check_char[256] = {false};


char scan_codes[] =
        {
                0,
                0,  // ESC
                '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
                0,  // BACKSPACE
                '\t',  // TAB
                'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
                ' ',// ENTER
                0,  // CTRL
                'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '<', '>', '+',
                0,  // left SHIFT
                '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
                0,  // right SHIFT
                '*',// NUMPAD *
                0,  // ALT
                ' ', // SPACE
                0, //CAPSLOCK
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //F1 - F10
                0, //NUMLOCK
                0, //SCROLLOCK
                0, //HOME
                0,
                0, //PAGE UP
                '-', //NUMPAD
                0, 0,
                0, //(r)
                '+', //NUMPAD
                0, //END
                0,
                0, //PAGE DOWN
                0, //INS
                0, //DEL
                0, //SYS RQ
                0,
                0, 0, //F11-F12
                0,
                0, 0, 0, //F13-F15
                0, 0, 0, 0, 0, 0, 0, 0, 0, //F16-F24
                0, 0, 0, 0, 0, 0, 0, 0
        };

char shift_char[] =
        {
                'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
                'v', 'w', 'x', 'y', 'z',
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
                'V', 'W', 'X', 'Y', 'Z'
        };

char other_symbols[] =
        {
                '=', '-', '+', '(', ')', '!', '@', '#', '$', '%', '^', '&', '*', '<', '>', '?', ',', '.', '/', ';', ':'
        };

//ПРОТОТИПЫ
void default_intr_handler();

void intr_reg_handler(int num, unsigned short segm_sel, unsigned short
flags, intr_handler hndlr);

void intr_init();

void intr_start();

void intr_enable();

void intr_disable();

static inline unsigned char inb(unsigned short port);

static inline void outb(unsigned short port, unsigned char data);

void keyb_init();

void keyb_handler();

void keyb_process_keys();

void cursor_moveto(unsigned int strnum, unsigned int pos);

void out_str(int color, const char *ptr, unsigned int strnum);

void out_char(int color, unsigned char symbol);

void out_int(int color, unsigned int num);

int strlen_colored(unsigned char *str);

int strlen(unsigned char *str);

bool strcmp(unsigned char *str1, const char *str2);

void clean();

void on_key(unsigned char scan_code);

void command_handler();

void info();

void up_down_case(unsigned char *str, int offset);

void shift_letter(unsigned char *str, int offset);

void titlize(unsigned char *str);

void _template(unsigned char *str);

void shifts_table(int temp_len);

void print_shifts(int temp_len);

void search(unsigned char *search_str);

void shutdown();


///////////////////////////////////////////////////
//manual
void default_intr_handler() {
    asm("pusha");
    // ... (реализация обработки)
    asm("popa; leave; iret");
}

//manual+
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short
flags, intr_handler hndlr) {
    unsigned int hndlr_addr = (unsigned int) hndlr;
    g_idt[num].base_lo = (unsigned short) (hndlr_addr & 0xFFFF);
    g_idt[num].segm_sel = segm_sel;
    g_idt[num].always0 = 0;
    g_idt[num].flags = flags;
    g_idt[num].base_hi = (unsigned short) (hndlr_addr >> 16);
}

// manual+
// Функция инициализации системы прерываний: заполнение массива с адресами обработчиков
void intr_init() {
    int i;
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
    for (i = 0; i < idt_count; i++)
        intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,
                         default_intr_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
}

//manual+
void intr_start() {
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
    g_idtp.base = (unsigned int) (&g_idt[0]);
    g_idtp.limit = (sizeof(struct idt_entry) * idt_count) - 1;
    asm("lidt %0" : : "m" (g_idtp));
}

//manual+
void intr_enable() {
    asm("sti");
}

//manual+
void intr_disable() {
    asm("cli");
}

//manual+
//обработка прерываний клавиатуры
static inline unsigned char inb(unsigned short port) // Чтение из порта
{
    unsigned char data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

//manual+
static inline void outb(unsigned short port, unsigned char data) // Запись
{
    asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

//manual+
void keyb_init() {
    // Регистрация обработчика прерывания
    intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler);
    // segm_sel=0x8, P=1, DPL=0, Type=Intr
    // Разрешение только прерываний клавиатуры от контроллера 8259
    outb(PIC1_PORT + 1, 0xFF ^ 0x02);
    // Разрешены будут только прерывания, чьи биты установлены в 0
}

//manual+
void keyb_handler() {
    asm("pusha");
    // Обработка поступивших данных
    keyb_process_keys();

    outb(PIC1_PORT, 0x20);
    asm("popa; leave; iret");
}

//manual+edited
void keyb_process_keys() {
    // Проверка что буфер PS/2 клавиатуры не пуст (младший бит присутствует)
    if (inb(0x64) & 0x01) {
        unsigned char scan_code;
        unsigned char state;

        scan_code = inb(0x60); // Считывание символа с PS/2 клавиатуры
        if (scan_code < 128) // Скан-коды выше 128 - это отпускание клавиши
            on_key(scan_code);

            //edit
        else if (scan_code == 170 || scan_code == 182)
            shift = 0;
    }
}

//manual
// Функция переводит курсор на строку strnum (0 – самая верхняя) в позицию
//pos на этой строке(0 – самое левое положение).
void cursor_moveto(unsigned int strnum, unsigned int pos) {
    unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
    outb(CURSOR_PORT, 0x0F);
    outb(CURSOR_PORT + 1, (unsigned char) (new_pos & 0xFF));
    outb(CURSOR_PORT, 0x0E);
    outb(CURSOR_PORT + 1, (unsigned char) ((new_pos >> 8) & 0xFF));
}

//manual
void out_str(int color, const char *ptr, unsigned int strnum) {
    unsigned char *video_buf = (unsigned char *) VIDEO_BUF_PTR;
    video_buf += 80 * 2 * strnum;
    while (*ptr) {
        video_buf[0] = (unsigned char) *ptr; // Символ (код)
        video_buf[1] = color; // Цвет символа и фона
        video_buf += 2;
        ptr++;
    }
}
///////////////////////////////////////////////////

void out_char(int color, unsigned char symbol) {
    unsigned char *video_buf = (unsigned char *) VIDEO_BUF_PTR;
    video_buf += 2 * (global_str * VIDEO_WIDTH + global_pos);
    video_buf[0] = symbol; // Символ (код)
    video_buf[1] = color; // Цвет символа и фона
    cursor_moveto(global_str, ++global_pos);
}

void out_int(int color, unsigned int num) {
    unsigned char *video_buf = (unsigned char *) VIDEO_BUF_PTR;
    video_buf += 2 * (global_str * VIDEO_WIDTH + global_pos);
    video_buf[0] = num; // Символ (код)
    video_buf[1] = color; // Цвет символа и фона
    cursor_moveto(global_str, ++global_pos);
}

///////////////////////////////////////////////////


//реализация библиотечных функций

int strlen_colored(unsigned char *str) {
    int i = 0;
    while (*str != '\0') {
        i++;
        str += 2;
    }
    return i;

}

int strlen(unsigned char *str) {
    int i = 0;
    while (*str != '\0') {
        i++;
        str++;
    }
    return i;

}

bool strcmp(unsigned char *str1, const char *str2) {

    while (*str1 != '\0' && *str1 != ' ' && *str2 != '\0' && *str1 == *str2) {
        str1 += 2;
        str2++;
    }

    if (*str1 == *str2)
        return true;

    return false;

}

//////////////////////////////////////////////////

void clean() {
    unsigned char *video_buf = (unsigned char *) VIDEO_BUF_PTR;

    for (int i = 0; i < 80 * 25; i++)
        video_buf[i] = '\0';

    global_str = 0;
    global_pos = 0;
}

//NADO SDELAT'
void scroll() {
    //прокрутка консоли
}

//проверить как работают шифты
void on_key(unsigned char scan_code) {
    if (flag) {
        flag = 0;
        clean();
        global_str = 0;
        global_pos = 0;
        out_str(0x0A, "# ", ++global_str);
        global_pos = 2;
        cursor_moveto(global_str, global_pos);
        return;
    }

    switch (scan_code) {
        case 14: //backspace
            if (global_pos >= 3) {
                unsigned char *video_buf = (unsigned char *) VIDEO_BUF_PTR;
                video_buf += 2 * (global_str * VIDEO_WIDTH + global_pos - 1);
                video_buf[0] = '\0';
                str_len--;
                cursor_moveto(global_str, --global_pos);
            }
            break;

        case 15: //tab
            if (global_pos < 76) {
                global_pos += 4;
                cursor_moveto(global_str, global_pos);
            }
            break;

        case 28: //enter
        {
            command_handler();
            out_str(0x07, "# ", ++global_str);
            global_pos = 2;
            cursor_moveto(global_str, global_pos);
            str_len--;
        }
            break;

        case 42: //shift
            shift = true;
            break;

        case 54: //shift
            shift = true;
            break;

        default: {
            str_len++;
            char c = scan_codes[(unsigned int) scan_code];
            if (shift == false)
                out_char(0x0A, c);
            else {
                int i = 0;
                for (i; i < 26; i++)
                    if (c == shift_char[i])
                        break;
                out_char(0x0A, shift_char[i + 26]);
            }

        }

    }
}

void command_handler() {
    unsigned char *video_buf = (unsigned char *) VIDEO_BUF_PTR;
    video_buf += 2 * (global_str * VIDEO_WIDTH + 2);


    if (strcmp(video_buf, "info"))
        info();

    else if (strcmp(video_buf, "upcase ")) {
        str_len -= 7;
        up_down_case((video_buf + 14), 0);
    } else if (strcmp(video_buf, "downcase ")) {
        str_len -= 9;
        up_down_case((video_buf + 18), 26);
    } else if (strcmp(video_buf, "titlize ")) {
        str_len -= 8;
        titlize((video_buf + 16));
    } else if (strcmp(video_buf, "template ")) {
        str_len -= 9;
        _template((video_buf + 18));
    } else if (strcmp(video_buf, "search ")) {
        str_len -= 7;
        search((video_buf + 14));
    } else if (strcmp(video_buf, "shutdown"))
        shutdown();

    else out_str(0x07, "Unknown command", ++global_str);

}

/////////////////////////////////////////////////////

//функции
//+
void info() {
    global_pos = 0;

    const char *line = "System info";
    out_str(0x07, line, ++global_str);

    const char *author = "Author: A. Kotlyarova";
    out_str(0x07, author, ++global_str);

    const char *translator = "Translator: GNU assembler (AT&T syntax)";
    out_str(0x07, translator, ++global_str);

    const char *compiler = "Compiler: GCC";
    out_str(0x07, compiler, ++global_str);

    const char *os = "OS: Linux ";
    out_str(0x07, os, ++global_str);

    /////////////////////////////////
    if (boot_options)
        out_str(0x07, "BM algorithm", ++global_str);
    else
        out_str(0x07, "STD algorithm", ++global_str);

}

//vrode ok
void up_down_case(unsigned char *str, int offset) {

    while (str_len > 0) {
        shift_letter(str, offset);
        str += 2;
        str_len--;
    }
}

//vrode ok
void shift_letter(unsigned char *str, int offset) {

    int i = offset;
    for (i; i < offset + 26; i++) {
        if (*str == shift_char[i]) {
            //*str == shift_char[i+26-2*offset];
            out_char(0x07, shift_char[i + 26 - 2 * offset]);
            break;
        }

    }
    if (i == offset + 26)
        out_char(0x07, *str);


}

//vrode ok
void titlize(unsigned char *str) {
    while (*str == ' ')
        str += 2;;
    shift_letter(str, 0);

    while (str_len > 0) {
        if (*(str - 2) == ' ')
            shift_letter(str, 0);
        else
            out_char(0x0A, *str);
        str += 2;
        str_len--;
    }


}

//vrode ok
void _template(unsigned char *str) {

    temp_len = str_len;
    int i = 0;
    for (i; i < str_len; i++)
        temp_str[i] = str[2 * i];
    i++;
    temp_str[i] = '\0';


    global_pos = 0;
    cursor_moveto(++global_str, global_pos);

    const char *template_ = "Template '";
    const char *loaded_ = "' loaded.";
    out_str(0x0A, template_, global_str);
    out_str(0x0A, (const char *) temp_str, global_str);
    out_str(0x0A, loaded_, global_str);

    shifts_table(temp_len);
    if (boot_options)
        print_shifts(temp_len);

}


void shifts_table(int temp_len) {
    for (int i = 0; i < 256; i++)
        shifts[i] = temp_len;
    for (int i = 1; i < temp_len; i++) {
        shifts[temp_str[i - 1]] = temp_len - i;
    }


}


void print_shifts(int temp_len) {
    const char *BM = " BM info:";
    out_str(0x0A, BM, global_str);

    global_pos = 0;
    cursor_moveto(++global_str, global_pos);

    for (int i = 0; i < temp_len; i++) {
        if (!check_char[temp_str[i]]) {
            check_char[temp_str[i]] = true;
            out_char(0x0A, temp_str[i]);
            out_char(0x0A, ':');
            out_int(0x0A, shifts[temp_str[i]]);
            out_char(0x0A, ' ');
        }

    }
}

void search(unsigned char *search_str) {
    int search_len = strlen(search_str);
    if (search_len < temp_len) {
        const char *error = "Incorrect lenght of the string.";
        out_str(0x0A, error, ++global_str);
        return;
    }
    int i = temp_len - 1;
    int tmp = i;
    int offset = 0;
    bool success = false;
    while (tmp >= 0 && i < search_len) {
        tmp = i;
        for (int j = temp_len - 1; j >= 0; j--) {
            if (temp_str[j] == search_str[tmp]) {
                tmp--;
                success = true;
            } else {
                i += shifts[search_str[tmp]];
                success = false;
                break;
            }
        }
    }

    if (!success) {

        const char *not_found = "Template not found.";
        out_str(0x0A, not_found, ++global_str);
        return;
    }


    global_pos = 0;
    cursor_moveto(++global_str, global_pos);
    const char *found = "Found '";
    const char *at_pos = "' at a pos : ";
    out_str(0x0A, found, ++global_str);
    out_str(0x0A, (const char *) temp_str, ++global_str);
    out_str(0x0A, at_pos, ++global_str);
    out_int(0x0A, tmp + 1);


}

void shutdown() {
    outb((unsigned short) 0xB004, (unsigned char) (0x0 | 0x2000));
    outb((unsigned short) 0x604, (unsigned char) (0x0 | 0x2000));
}


//manual+edited
extern "C" int kmain() {
    //!!!
    if (*(char *) 0x000 == '1')
        boot_options = true;
    clean();

    //manual
    const char *hello = "Welcome to StringsOS (gcc edition)!";

    //edit
    global_pos = 22;
    global_str = 12;

    //manual
    out_str(0x07, hello, global_str);

    //edit
    const char *hint = "(Press any key to start)";
    global_pos = 28;
    global_str = 24;
    out_str(0x07, hint, global_str);
    cursor_moveto(80, 25);


    //edit
    intr_disable();
    intr_init();
    keyb_init();
    intr_start();
    intr_enable();

    //manual
    while (1) {
        asm("hlt");
    }

    return 0;
}
