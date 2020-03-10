#define MAX_STR_LENGTH 50
#define CONSOLE_LONG_TAB 120

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

// memory model - a doubly linked list
struct memory_block {
    struct memory_block * previous;
    struct memory_block * next;
    int value;
    int address;
};

// a doubly linked list containing variable labels
struct  label {
    struct label * previous;
    struct label * next;
    struct memory_block * location;
    char label[MAX_STR_LENGTH];
};

// a doubly linked list which will store the code to be interpreted
struct command {
    int number;
    char label[MAX_STR_LENGTH];
    char type[4];
    char content[MAX_STR_LENGTH];
    struct command * next;
    struct command * previous;
};

int registry[16];
int program_state_registry;
int num_of_commands = 1;
int log_num = 0;
int termination_flag = 0;                                                                                               // a simple flag indicating whether the interpeter has reached the final line of provided code
struct memory_block * memory_tail = NULL;
struct memory_block * memory_head = NULL;
struct label * label_tail = NULL;
struct label * label_head = NULL;
struct command * command_head = NULL;
struct command * command_tail = NULL;

void create_variable(char label[], int value);
void create_array(char label[], int value, int quantity);
void allocate_memory(int value);
void input_parser(FILE * file, char word[]);                                                                            // building the command list
void get_input(FILE * file);
void interpret(struct command * current, char debug_mode[]);                                                            // code execution
void print_machine_state(struct command * current);
void content_m_splitter(int * registry_num, struct memory_block ** variable, char word[]);                              // splits the registry-to-memory commands
void content_r_splitter(int * registry1_num, int * registry2_num, char word[]);                                         // splits the registry-to-registry commands
void program_state_registry_updater(int value);
void DC(char word[], char label[]);
void arithmetic_operations(char word[], char type[]);
void registry_arithmetic_operations(char word[], char type[]);
void L(char word[]);
void LA(char word[]);
void LR(char word[]);
void ST(char word[]);
void free_memory();
struct memory_block * return_variable(char name[]);
struct command * return_command(char name[]);
struct memory_block * address_shifter(int address_shift, int address_registry_num);
struct memory_block * address_search(int emulated_address);                                                             // returns the pointer to the memory_block of given emulated address
struct memory_block * return_middle(struct memory_block * upper, struct memory_block * lower);                          // for bisection search purposes

int main()
{
    struct command * p1;
    char debug_mode[2];
    char filename[512];
    FILE * file;

    // maximise and clear the console window
    ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
    system("@cls||clear");

    printf("_______ PSEUDO ASSEMBLER INTERPRETER _______\n\n Please provide the filename for the file containing the assembler instructions: ");

    // get file name
    gets(filename);
    file = fopen(filename, "r");

    // check whether the path is correct and the file exists
    while (file == NULL)
    {
        printf("The file you've specified doesn't exist :(. Please provide a correct path: ");
        gets(filename);
        file = fopen(filename, "r");
    }

    // prompt user for debug mode
    printf("Run in debug mode? [y/n]: ");
    gets(debug_mode);

    printf("\n");

    // parse the input
    get_input(file);

    // change font color to green
    SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 10);
    // if not in debug mode, the program will now print how it read the provided input
    printf("\nYour code:\n");
    p1 = command_head;
    while (strcmp(debug_mode, "y") != 0)
    {
        printf("%d.\t %10s\t%s\t%s\n", p1->number, p1->label, p1->type, p1->content);
        if (p1 == command_tail)
            break;
        p1 = p1->next;
    }
    printf("\n");
    //change font color to normal
    SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 15);

    // interpret the code
    interpret(command_head, debug_mode);

    if (strcmp(debug_mode, "y") == 0)
        system("@cls||clear");

    // print the machine's state after the final line of code
    print_machine_state(NULL);

    free_memory();

    printf("\n");
    system("PAUSE");
    return 0;
}// main

// creates a doubly linked list which represents the emulated memory
void allocate_memory(int value)
{
    struct memory_block * chunk = malloc(sizeof(struct memory_block));

    chunk->value = value;

    if (memory_head == NULL)
    {
        chunk->previous = NULL;
        chunk->address = 0;
        chunk->next = NULL;
        registry[14] = 0;
        memory_head = chunk;
        memory_tail = chunk;
    } else {
        chunk->previous = memory_tail;
        (chunk->previous)->next = chunk;
        chunk->address = (chunk->previous)->address + 4;
        chunk->next = NULL;
        memory_tail = chunk;
    }
}// allocate_memory

// creating a doubly linked list which contains variable labels
void create_variable(char label[], int value)
{
    struct label * variable = malloc(sizeof(struct label));

    allocate_memory(value);

    strcpy(variable->label, label);
    variable->location = memory_tail;

    if (label_head == NULL)
    {
        variable->previous = NULL;
        label_head = variable;
        label_tail = variable;
    } else {
        variable->previous = label_tail;
        (variable->previous)->next = variable;
        label_tail = variable;
    }
}// create_variable

void create_array(char label[], int value, int quantity)
{
    create_variable(label, value);

    for (int i = 0; i < quantity - 1; i++)
        allocate_memory(value);
}// create_array

void get_input(FILE * file)
{
    char word[2 * MAX_STR_LENGTH];

    while(fscanf(file, "%s", word) == 1)
    {
        input_parser(file, word);
    }
    printf("log_%d:\t input parsed\n", log_num);
    log_num++;
}// get_input

// creating a doubly linked list containing all the commands
void input_parser(FILE * file, char word[])
{
    struct command * verse = malloc(sizeof(struct command));
    int added = 0;

    if (command_head == NULL)
    {
        command_head = verse;
        command_tail = verse;
        verse->previous = NULL;
    }
    else
    {
        verse->previous = command_tail;
        (verse->previous)->next = verse;
        command_tail = verse;
    }

    strcpy(verse->label, "null");
    verse->number = num_of_commands;

    while (added == 0)
    {
        if (strcmp(word, "DC") == 0 || strcmp(word, "DS") == 0 || strcmp(word, "A") == 0 || strcmp(word, "AR") == 0 ||
            strcmp(word, "S") == 0 || strcmp(word, "SR") == 0 || strcmp(word, "M") == 0 ||strcmp(word, "MR") == 0 ||
            strcmp(word, "D") == 0 || strcmp(word, "DR") == 0 || strcmp(word, "C") == 0 || strcmp(word, "CR") == 0 ||
            strcmp(word, "J") == 0 || strcmp(word, "JZ") == 0 ||strcmp(word, "JP") == 0 || strcmp(word, "JN") == 0 ||
            strcmp(word, "L") == 0 || strcmp(word, "LA") == 0 || strcmp(word, "LR") == 0 || strcmp(word, "ST") == 0)
        {
            strcpy(verse->type, word);
            fscanf(file, "%s", word);
            strcpy(verse->content, word);
            added = 1;
        }
        else
        {
            strcpy(verse->label, word);
            fscanf(file, "%s", word);
        }
    }
    num_of_commands++;
}// input_parser

// executing the code
void interpret(struct command * current, char debug_mode[])
{
    char next_step = (char) 10;
    if (strcmp(debug_mode, "y") == 0)
        system("@cls||clear");
    if (strcmp(current->type, "DC") == 0 || strcmp(current->type, "DS") == 0)
        DC(current->content, current->label);
    else if (strcmp(current->type, "A") == 0 || strcmp(current->type, "S") == 0 || strcmp(current->type, "M") == 0 ||
             strcmp(current->type, "D") == 0 || strcmp(current->type, "C") == 0)
        arithmetic_operations(current->content, current->type);
    else if (strcmp(current->type, "AR") == 0 || strcmp(current->type, "SR") == 0 || strcmp(current->type, "MR") == 0 ||
             strcmp(current->type, "DR") == 0 || strcmp(current->type, "CR") == 0)
        registry_arithmetic_operations(current->content, current->type);
    else if (strcmp(current->type, "J") == 0)
    {
        if (strcmp(debug_mode, "y") == 0)
        {
            print_machine_state(current);
            next_step = getchar();
        }
        interpret(return_command(current->content), debug_mode);
    }
    else if (strcmp(current->type, "JN") == 0 && program_state_registry == 2)
    {
        if (strcmp(debug_mode, "y") == 0)
        {
            print_machine_state(current);
            next_step = getchar();
        }
        interpret(return_command(current->content), debug_mode);
    }
    else if (strcmp(current->type, "JZ") == 0 && program_state_registry == 0)
    {
        if (strcmp(debug_mode, "y") == 0)
        {
            print_machine_state(current);
            next_step = getchar();
        }
        interpret(return_command(current->content), debug_mode);
    }
    else if (strcmp(current->type, "JP") == 0 && program_state_registry == 1)
    {
        if (strcmp(debug_mode, "y") == 0)
        {
            print_machine_state(current);
            next_step = getchar();
        }
        interpret(return_command(current->content), debug_mode);
    }
    else if (strcmp(current->type, "L") == 0)
        L(current->content);
    else if (strcmp(current->type, "LA") == 0)
        LA(current->content);
    else if (strcmp(current->type, "LR") == 0)
        LR(current->content);
    else if (strcmp(current->type, "ST") == 0)
        ST(current->content);

    if (strcmp(debug_mode, "y") == 0 && termination_flag != 1)
    {
        print_machine_state(current);
        do
            next_step = getchar();
        while (next_step != (int) 10);
    }

    if (current == command_tail)
        termination_flag = 1;
    else if (termination_flag != 1 && (next_step == (char) 10 || strcmp(debug_mode, "y") != 0))
        interpret(current->next, debug_mode);
}// interpret

void print_machine_state(struct command * current)
{
    int registry_num1 = -1, registry_num2 = -1, i = 0, ints;
    struct memory_block * variable = NULL;
    struct command * p1;
    struct label * p0 = label_head;
    struct memory_block * chunk;
    COORD c;
    CONSOLE_SCREEN_BUFFER_INFO SBInfo;

    // splitting the line of code to know which parts of memory are being accessed or changed
    if (current != NULL && (strcmp(current->type, "A") == 0 || strcmp(current->type, "S") == 0 || strcmp(current->type, "M") == 0 ||
             strcmp(current->type, "D") == 0 || strcmp(current->type, "C") == 0 ||strcmp(current->type, "L") == 0 ||
             strcmp(current->type, "LA") == 0 || strcmp(current->type, "ST") == 0))
        content_m_splitter(& registry_num1, & variable, current->content);
    if (current != NULL && (strcmp(current->type, "AR") == 0 || strcmp(current->type, "SR") == 0 || strcmp(current->type, "MR") == 0 ||
             strcmp(current->type, "DR") == 0 || strcmp(current->type, "CR") == 0 || strcmp(current->type, "LR") == 0))
        content_r_splitter(& registry_num1, & registry_num2, current->content);

    printf("\n");

    // current will be passed as NULL when the program will print the final machine's state
    if (current != NULL && current->type[0] != 'J')
        printf("The following command has been debugged:\n%d\t%s\t%s\t%s\n\n", current->number, current->label, current->type, current->content);
    else if (current == NULL)
        printf("The machine's state after every line of code in your input file is presented below:\n\n");
    else if (current->type[0] == 'J')
        printf("\nThis line is being debugged (program state registry is being checked): \n%d\t%s\t%s\t%s\n\n", current->number, current->label, current->type, current->content);
    printf("Declared variables:\n");

    // printing variables
    while (p0 != NULL)
    {
        if ((p0 == label_tail && (memory_tail->address != (label_tail->location)->address)) || (p0 != label_tail && (((p0->location)->next)->address != ((p0->next)->location)->address)))
        {
            // if the variable is an array, the program needs a special way of displaying it
            printf("label: %15s\t values: [ ", p0->label);

            chunk = p0->location;
            // ints will represent how many elements of an array have been printed
            ints = 0;
            while (p0 == label_tail || (p0 != label_tail && chunk->address != ((p0->next)->location)->address))
            {
                if (variable != NULL && variable->address == chunk->address)
                {
                    // change font color to red if the current command accesses or changes this particular chunk of memory
                    SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 12);
                    // change font color to blue if this chunk's value is being compared not overwritten
                    if (strcmp(current->type, "C") == 0 || strcmp(current->type, "CR") == 0)
                        SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 9);
                }
                printf("%4d ", chunk->value);
                // change font color to normal
                SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 15);

                chunk = chunk->next;
                if (chunk == NULL)
                    break;

                ints++;
                // for the sake of aesthetics, the program prints out max. 10 array values in one row
                if (ints >= 10)
                {
                    if (variable != NULL && variable->address == chunk->address)
                    {
                        // change font color to red if the current command accesses or changes this particular chunk of memory
                        SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 12);
                        // change font color to blue if this chunk's value is being compared not overwritten
                        if (strcmp(current->type, "C") == 0 || strcmp(current->type, "CR") == 0)
                            SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 9);
                    }
                    ints = 0;
                    if ((p0 != label_tail && chunk->address != ((p0->next)->location)->address) || (p0 == label_tail && (chunk->address != memory_tail->address)))
                        printf("\n%35s","");

                    // change font color to normal
                    SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 15);
                }
            }
            printf("  ]\n");
            if (p0 == label_tail)
                break;
            p0 = p0->next;
        }
        else // print non-array variables
        {
            printf("label: %15s\t value: ", p0->label);
            if (variable != NULL && variable->address == (p0->location)->address)
            {
                // change font color to red if the current command accesses or changes this particular chunk of memory
                SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 12);
                // change font color to blue if this chunk's value is being compared not overwritten
                if (strcmp(current->type, "C") == 0 || strcmp(current->type, "CR") == 0)
                    SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 9);
            }
            printf("%d\t\n", (p0->location)->value);
            if (p0 == label_tail)
                break;
            p0 = p0->next;
            // change font color to normal
            SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 15);
        }
    }

    if (label_head == NULL) printf("no variables yet\n");
    printf("\n");

    // print registry state
    if (current != NULL && (strcmp(current->type, "J") == 0 || strcmp(current->type, "JZ") == 0 || strcmp(current->type, "JP") == 0 || strcmp(current->type, "JN") == 0))
        SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 9);   // change font color to blue
    printf("program state registry: %d\n\n", program_state_registry);
    // change font color to normal
    SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 15);
    for (i = 0; i <= 15; i++)
    {
        if (current != NULL && (registry_num1 == i || registry_num2 == i))
        {
            // change font color to red if the current command accesses or changes this particular registry
            SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 12);
            // change font color to blue if this registry's value is being compared not overwritten
            if (strcmp(current->type, "C") == 0 || strcmp(current->type, "CR") == 0)
                SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 9);
        }
        printf("registry %d:\t%d\n", i, registry[i]);
        // change font color to normal
        SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 15);
    }

    printf("\nPress Enter to go to the next step...\n");

    // save the cursor coordinates for later
    GetConsoleScreenBufferInfo(GetStdHandle( STD_OUTPUT_HANDLE ), & SBInfo);

    // will display the whole pseudo assembler code if in debug mode (not when displaying the final state of the machine
    if (current != NULL)
    {
        // change cursor coordinates (top line, next to the machine's state)
        c.X = CONSOLE_LONG_TAB; c.Y = 0;
        SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE ), c );
        // change font color to green
        SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 10);
        printf("Your code:\n");
        // change cursor coordinates (one line down)
        c.X = CONSOLE_LONG_TAB; c.Y = 1;
        SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE ), c );
        p1 = command_head;
        i = 2;
        while (1)
        {
            // change cursor coordinates (one line down)
            c.X = CONSOLE_LONG_TAB; c.Y = i;
            if (current == p1)
                SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 112); // change font and background color to highlight the current line of code
            printf("%d.\t %10s\t%s\t%s\n", p1->number, p1->label, p1->type, p1->content);
            SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE ), c );
            if (p1 == command_tail)
                break;
            p1 = p1->next;
            //set font color to green
            SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 10);
            i++;
        }
    }

    //set the font color to normal
    SetConsoleTextAttribute (GetStdHandle(STD_OUTPUT_HANDLE), 15);

    //scroll up
    c.X = 0; c.Y = 0;
    SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE ), c );

    //use the saved cursor position
    c.X = SBInfo.dwCursorPosition.X; c.Y = SBInfo.dwCursorPosition.Y;
    SetConsoleCursorPosition( GetStdHandle( STD_OUTPUT_HANDLE ), c );
}// print_machine_state

// returns the registry number and variable on which the program will operate in a given line of code
void content_m_splitter(int * registry_num, struct memory_block ** variable, char word[])
{
    char c_registry_num[MAX_STR_LENGTH] = "";
    char label[MAX_STR_LENGTH];
    char address_shift[MAX_STR_LENGTH];
    char address_registry_num[MAX_STR_LENGTH] = "14";
    int i = 0;

    // registry number
    while (i < strlen(word))
    {
        if (word[i] == ',')
            break;
        c_registry_num[i] = word[i];
        c_registry_num[i+1] = '\0';
        i++;
    }

    i++;
    int l = 0;

    // when there is an address shift provided e.g. 0(14)
    if (word[i] >= 48 && word[i] <= 57)
    {
        while ((word[i] >= 48 && word[i] <= 57) || word[i] == '(')
        {
            address_shift[l] = word[i];
            address_shift[l+1] = '\0';
            i++;
            l++;
            if (word[i] == '(')
            {
                i++;
                int g = 0;
                while (word[i] != ')') {
                    address_registry_num[g] = word[i];
                    address_registry_num[g + 1] = '\0';
                    i++;
                    g++;
                }
            }
        }
        * variable = address_shifter(atoi(address_shift), atoi(address_registry_num));

    }
    else  // when only the variable name is provided
    {

        while (i < strlen(word) && word[i] != '(') {
            label[l] = word[i];
            label[l + 1] = '\0';
            l++;
            i++;
        }

        if ((int) label[0] < 48 || (int) label[0] > 57)
            *variable = return_variable(label);
    }

    l = 0;
    // when the variable name is provided with an address shift e.g. TAB(12)
    if (strlen(word) > i && word[i] == '(')
    {
        while (word[i] != ')')
        {
            i++;
            address_registry_num[l] = word[i];
            address_registry_num[l+1] = '\0';
            l++;
        }
        * variable = address_shifter((* variable)->address, atoi(address_registry_num));
    }

    (* registry_num) = atoi(c_registry_num);
}// content_m_splitter

// returns the registry numbers on which the program will operate in a given line of code
void content_r_splitter(int * registry1_num, int * registry2_num, char word[])
{
    char c_registry1_num[MAX_STR_LENGTH];
    char c_registry2_num[MAX_STR_LENGTH];

    int i = 0;
    while(i < strlen(word))
    {
        if (word[i] == ',')
            break;
        c_registry1_num[i] = word[i];
        c_registry1_num[i + 1] = '\0';
        i++;
    }

    i++;
    int l = 0;
    while (i < strlen(word))
    {
        c_registry2_num[l] = word[i];
        c_registry2_num[l + 1] = '\0';
        l++;
        i++;
    }

    (* registry1_num) = atoi(c_registry1_num);
    (* registry2_num) = atoi(c_registry2_num);
}// content_r_splitter

void program_state_registry_updater(int value)
{
    if (value > 0)
        program_state_registry = 1;
    else if (value == 0)
        program_state_registry = 0;
    else if (value < 0)
        program_state_registry = 2;

    printf(" the program state registry is equal to: %d\n", program_state_registry);
}// program_state_registry_updater

void DC(char word[], char label[])
{
    char number[MAX_STR_LENGTH] = "";
    char value[MAX_STR_LENGTH] = "0";

    // handles situations like 100*INTEGER(1)
    if ((int) word[0] >= 48 && (int) word[0] <= 57)
    {
        for (int i = 0; i < strlen(word); i++)
        {
            if ((int) word[i] >= 48 && (int) word[i] <= 57)
            {
                number[i] = word[i];
                number[i+1] = '\0';
            }
            if (word[i] == '(')
            {
                for (int j = i+1; j < strlen(word); j++) {
                    int l = 0;
                    if ((int) word[j] >= 48 && (int) word[j] <= 57)
                    {
                        value[l] = word[j];
                        value[l+1] = '\0';
                        l++;
                    }
                    else
                        break;
                }
                break;
            }
        }
        create_array(label, atoi(value), atoi(number));
        printf("log_%d:\t created and array of %s elements labeled %s with an assigned value: %s\n", log_num, number, label, value);
        log_num++;
    }
    else // single variable creation
    {
        int l = 0;
        for (int i = 0; i < strlen(word); i++)
        {
            if (word[i] == '(')
            {
                for (int j = i+1; j < strlen(word); j++)
                {
                    if (word[j] == ')')
                        break;
                    value[l] = word[j];
                    value[l+1] = '\0';
                    l++;
                }
                break;
            }
        }
        create_variable(label, atoi(value));
        printf("log_%d:\t created a variable labeled %s with an assigned value: %s\n", log_num, label,  value);
        log_num++;
    }
}// DC

void arithmetic_operations(char word[], char type[])
{
    int registry_num;
    struct memory_block * variable;

    content_m_splitter(&registry_num, &variable, word);

    if (strcmp(type, "A") == 0)
    {
        registry[registry_num] = registry[registry_num] + variable->value;

        printf("log_%d:\t registry %d has been assigned a new value equal to %d;", log_num, registry_num, registry[registry_num]);
        log_num++;
        program_state_registry_updater(registry[registry_num]);
    }
    else if (strcmp(type, "S") == 0)
    {
        registry[registry_num] = registry[registry_num] - variable->value;

        printf("log_%d:\t registry %d has been assigned a new value equal to %d;", log_num, registry_num, registry[registry_num]);
        log_num++;
        program_state_registry_updater(registry[registry_num]);
    }
    else if (strcmp(type, "M") == 0)
    {
        registry[registry_num] = registry[registry_num] * variable->value;

        printf("log_%d:\t registry %d has been assigned a new value equal to %d;", log_num, registry_num, registry[registry_num]);
        log_num++;
        program_state_registry_updater(registry[registry_num]);
    }
    else if (strcmp(type, "D") == 0)
    {
        registry[registry_num] = registry[registry_num] / variable->value;

        printf("log_%d:\t registry %d has been assigned a new value equal to %d;", log_num, registry_num, registry[registry_num]);
        log_num++;
        program_state_registry_updater(registry[registry_num]);
    }
    else if (strcmp(type, "C") == 0)
    {
        int sign = registry[registry_num] - variable->value;

        printf("log_%d:\t registry %d has been compared with %d;", log_num, registry_num, variable->value);
        log_num++;
        program_state_registry_updater(sign);
    }
}// arithmetic_operations

void registry_arithmetic_operations(char word[], char type[])
{
    int registry1_num;
    int registry2_num;

    content_r_splitter(&registry1_num, &registry2_num, word);

    if (strcmp(type, "AR") == 0)
    {
        registry[registry1_num] = registry[registry1_num] + registry[registry2_num];

        printf("log_%d:\t registry %d has been assigned a new value equal to %d;", log_num, registry1_num, registry[registry1_num]);
        log_num++;
        program_state_registry_updater(registry[registry1_num]);
    }
    else if (strcmp(type, "SR") == 0)
    {
        registry[registry1_num] = registry[registry1_num] - registry[registry2_num];

        printf("log_%d:\t registry %d has been assigned a new value equal to %d;", log_num, registry1_num, registry[registry1_num]);
        log_num++;
        program_state_registry_updater(registry[registry1_num]);
    }
    else if (strcmp(type, "MR") == 0)
    {
        registry[registry1_num] = registry[registry1_num] * registry[registry2_num];

        printf("log_%d:\t registry %d has been assigned a new value equal to %d;", log_num, registry1_num, registry[registry1_num]);
        log_num++;
        program_state_registry_updater(registry[registry1_num]);
    }
    else if (strcmp(type, "DR") == 0)
    {
        registry[registry1_num] = registry[registry1_num] / registry[registry2_num];

        printf("log_%d:\t registry %d has been assigned a new value equal to %d;", log_num, registry1_num, registry[registry1_num]);
        log_num++;
        program_state_registry_updater(registry[registry1_num]);
    }
    else if (strcmp(type, "CR") == 0)
    {
        int sign = registry[registry1_num] - registry[registry2_num];

        printf("log_%d:\t registry %d has been compared with registry %d;", log_num, registry1_num, registry2_num);
        log_num++;
        program_state_registry_updater(sign);
    }
}// registry_arithmetic_operations

void L(char word[])
{
    int registry_num;
    struct memory_block * variable;

    content_m_splitter(&registry_num, &variable, word);

    registry[registry_num] = variable->value;

    printf("log_%d:\t registry %d has been assigned a new value equal to: %d\n", log_num, registry_num, variable->value);
    log_num++;
}// L

void LA(char word[])
{
    int registry_num;
    struct memory_block * variable;

    content_m_splitter(&registry_num, &variable, word);

    registry[registry_num] = variable->address;

    printf("log_%d:\t registry %d has been assigned a new value: %d\n", log_num, registry_num, variable->address);
    log_num++;
}// LA

void LR(char word[])
{
    int registry1_num;
    int registry2_num;

    content_r_splitter(&registry1_num, &registry2_num, word);

    registry[registry1_num] = registry[registry2_num];

    printf("log_%d:\t registry %d has been assigned a new value equal to %d\n", log_num, registry1_num, registry[registry1_num]);
    log_num++;
}// LR

void ST(char word[])
{
    int registry_num;
    int i = 0;
    int l = 0;
    char label[MAX_STR_LENGTH];
    struct memory_block * variable;

    content_m_splitter(&registry_num, &variable, word);

    variable->value = registry[registry_num];

    while (i < strlen(word))
    {
        if (word[i] == ',')
            break;
        i++;
    }
    i++;
    while (i < strlen(word))
    {
        label[l] = word[i];
        label[l + 1] = '\0';
        l++;
        i++;
    }

    printf("log_%d:\t variable %s has been assigned a new value equal to %d\n", log_num, label, registry[registry_num]);
    log_num++;
}// SR

struct memory_block * return_variable(char name[])
{
    struct label * p0 = label_tail;
    while (p0 != NULL)
    {
        if (strcmp(p0->label, name) == 0)
            return p0->location;
        p0 = p0->previous;
    }
    return 0;
}// return_variable

struct command * return_command(char name[])
{
    struct  command * p0 = command_tail;
    while (p0 != NULL)
    {
        if (strcmp(p0->label, name) == 0)
            return p0;
        p0 = p0->previous;
    }
    return 0;
}// return_command

struct memory_block * address_search(int emulated_address)
{
    struct memory_block * upper = memory_head;
    struct memory_block * lower = memory_tail;
    struct memory_block * middle;

    while (upper->address <= lower->address)
    {
        if (upper->address == emulated_address)
            return upper;
        if (lower->address == emulated_address)
            return lower;
        middle = return_middle(upper, lower);
        if (middle->address == emulated_address)
            return middle;
        if (middle->address > emulated_address)
            lower = middle->previous;
        else
            upper = middle->next;
    }
}// address_search

struct memory_block * return_middle(struct memory_block * upper, struct memory_block * lower)
{
    while (upper != lower && upper->address > lower->address)
    {
        upper = upper->next;
        lower = lower->previous;
    }

    return upper;
}// return_middle

struct memory_block * address_shifter(int address_shift, int address_registry_num)
{
    struct memory_block * chunk = address_search(registry[address_registry_num]);
    for (int i = 0; i < address_shift; i += 4)
    {
        chunk = chunk->next;
    }
    return chunk;
}// address_shifter

void free_memory()
{
    struct memory_block * memory = memory_tail;
    struct memory_block * m_prev;
    struct label * label = label_tail;
    struct label * l_prev;
    struct command * command = command_tail;
    struct command * c_prev;

    while(memory != NULL)
    {
        m_prev = memory->previous;
        free(memory);
        memory = m_prev;
    }

    while (label != NULL)
    {
        l_prev = label->previous;
        free(label);
        label = l_prev;
    }

    while(command != NULL)
    {
        c_prev = command->previous;
        free(command);
        command = c_prev;
    }
    printf("log_%d:\t memory freed", log_num);
    log_num++;
}// free_memory