#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------------------------ constants ------------------------------------------------------------------------------------------ */
#define MAXLINESIZE 1024
#define CHANGE 'c'
#define DELETE 'd'
#define PRINT 'p'
#define UNDO 'u'
#define REDO 'r'
#define QUIT 'q'
#define POINT '.'
#define NEWLINE '\n'
#define EMPTY_STATE ".\n"
#define TEXT_ARRAY_INITIAL_SIZE 100000000
#define DO_ARRAY_INITIAL_SIZE 100000000
char* empty_state_text = EMPTY_STATE;

/* ------------------------------------------------------------------------------------------ prototypes ------------------------------------------------------------------------------------------ */

typedef struct cell_s{
    char* text_line;
} cell_t;

typedef struct dynamic_array_s{
    cell_t* array;
    int last_version_used_size;
    int used_size;
    int max_size;
}dynamic_array_t;

typedef struct do_cell_s{
    int begin;
    int end;
} do_cell_t;

typedef struct do_array_s{
    do_cell_t* array;
    int used_size;
    int max_size;
}do_array_t;

/* ------------------------------------------------------------------------------------------ basic functions ------------------------------------------------------------------------------------------ */

int min(int a, int b)
{
    if (a<b)
        return a;
    return b;
}

int max (int a, int b)
{
    if (a > b)
        return  a;
    return b;
}

/* ------------------------------------------------------------------------------------------prototypes ------------------------------------------------------------------------------------------ */

/**
 * Initializes the array containing the text
 * @param a array to allocate
 * @param initial_size size to initialize the array
 */
void init_text_array(dynamic_array_t* a, size_t initial_size);

/**
 * Initializes the array containing the undo/redo indexes
 * @param a array to allocate
 * @param initial_size size to initialize the array
 */
void init_do_array(do_array_t* a, size_t initial_size);

/**
 * Adds new line in the text array. It is called when there are no rows that will be overwritten
 * @param text_array array in which text will be added
 * @param do_array array with the indexes used to speed up undo/redo operations
 * @param line_number number of line in which the string will be added
 * @param effective_string string to insert
 */
void array_insert_no_replace(dynamic_array_t *text_array, do_array_t *do_array, int line_number, char *effective_string);

/**
 * Adds newlines in the text array. It is called when there are rows that will be overwritten
 * @param text_array array in which text will be added
 * @param do_array array with the indexes used to speed up undo/redo operations
 * @param starting index of the command
 * @param ending index of the command
 */
void array_insert_with_replace(dynamic_array_t *text_array, do_array_t *do_array, int start, int end);

/**
 * Acknowledges and updates the undo/redo array after an insertion called in a command that does not overwrite an already present row
 * @param a do_array
 * @param starting index of the command
 * @param ending index of the command
 */
void do_array_insert_only_no_replace(do_array_t* a, int start, int end);

/**
 * Acknowledges and updates the undo/redo array after an insertion called in a command that overwrites an already present row
 * @param do_array do_array
 * @param end ending index of the command
 */
void do_array_insert_with_replace(do_array_t* do_array, int end);

/**
 * Acknowledges and updates the undo/redo array after a delete command
 * @param do_array do_array
 * @param start starting index of the command
 * @param end ending index of the command
 */
void do_array_delete(do_array_t *do_array, int start, int end);

/**
 * Acknowledges and updates the text array after a delete command
 * @param do_array do_array
 * @param text_array text_array
 * @param start starting index of the command
 * @param end ending index of the command
 */
void array_delete(do_array_t* do_array, dynamic_array_t* text_array, int start, int end);

/**
 * Prints the text in the text array
 * @param text_array text_array
 * @param do_array do_array
 * @param start starting index of the command
 * @param end ending index of the command
 */
void print_text_array(dynamic_array_t* text_array, do_array_t* do_array, int start, int end);

/* ------------------------------------------------------------------------------------------ functions ------------------------------------------------------------------------------------------ */

void init_text_array(dynamic_array_t* a, size_t initial_size)
{
    a->array = malloc(initial_size * sizeof(cell_t));
    a->used_size = 0;
    a->last_version_used_size = 0;
    a->max_size = initial_size;
}

void init_do_array(do_array_t* a, size_t initial_size)
{
    a->array = malloc(initial_size * sizeof(do_cell_t));
    a->used_size = 0;
    a->array[0].begin = -1; // initial state is identified with keys -1,-1
    a->array[0].end = -1;
    a->max_size = initial_size;
}

void array_insert_no_replace(dynamic_array_t *text_array, do_array_t *do_array, int line_number, char *effective_string)
{
    if (text_array->used_size == text_array->max_size){
        text_array->max_size *= 2;
        text_array->array = realloc(text_array->array, text_array->max_size * sizeof(cell_t));
    }
    text_array->used_size++;
    text_array->last_version_used_size++; //used values increase only in this case, in the other cases it is just a replace
    text_array->array[line_number-1+ do_array->array[do_array->used_size].begin].text_line = effective_string;
}

void array_insert_with_replace(dynamic_array_t *text_array, do_array_t *do_array, int start, int end)
{
    int previous_state_begin = do_array->array[do_array->used_size-1].begin; // assignment with the beginning of the previous state, easier to read
    int line_number;
    int i;
    char text[MAXLINESIZE+1];
    char * effective_string;

    text_array->last_version_used_size = 0;
    for (i = 0; i < (start-1); i++){ //copies values between i and start (i.e. 3,6c --> copy index 1-2)
        if (text_array->used_size == text_array->max_size){
            text_array->max_size *= 2;
            text_array->array = realloc(text_array->array, text_array->max_size * sizeof(cell_t));
        }
        text_array->array[do_array->array[do_array->used_size].begin+i].text_line = text_array->array[previous_state_begin+i].text_line;
        text_array->used_size++;
        text_array->last_version_used_size++;
    }

    for (line_number = start; line_number <= end; line_number++){ // performs a substitution of already present values
        fgets(text, MAXLINESIZE+1, stdin);
        effective_string = malloc(strlen(text)+1);
        strcpy(effective_string, text);

        if (text_array->used_size == text_array->max_size){
            text_array->max_size *= 2;
            text_array->array = realloc(text_array->array, text_array->max_size * sizeof(cell_t));
        }

        text_array->used_size++;
        text_array->last_version_used_size++; //used values increase only in this case, in the other cases it is just a replace
        text_array->array[line_number-1+ do_array->array[do_array->used_size].begin].text_line = effective_string;
    }

    for (i = text_array->last_version_used_size + do_array->array[do_array->used_size].begin-1; i < do_array->array[do_array->used_size].end ; i++){
        text_array->array[do_array->array[do_array->used_size].begin + text_array->last_version_used_size].text_line = text_array->array[do_array->array[do_array->used_size-1].begin + text_array->last_version_used_size].text_line;
        text_array->used_size++;
        text_array->last_version_used_size++;
    }
}

void do_array_insert_only_no_replace(do_array_t* a, int start, int end)
{
    if (a->used_size == a->max_size){
        a->max_size *= 2;
        a->array = realloc(a->array, a->max_size * sizeof(do_cell_t));
    }
    a->used_size++;

    if (a->used_size == 1){ //case when it's at the beginning
        a->array[a->used_size].begin = 0;
        if (start == end) //command such as "1,1c"
            a->array[a->used_size].end = 0;
        else //command such as "1,3c"
            a->array[a->used_size].end = end - start;
    } else {
        a->array[a->used_size].begin = a->array[a->used_size-1].begin;
        a->array[a->used_size].end =  a->array[a->used_size-1].end + end - start + 1;
    }
}

void do_array_insert_with_replace(do_array_t* do_array, int end)
{
    if (do_array->used_size == do_array->max_size){
        do_array->max_size *= 2;
        do_array->array = realloc(do_array->array, do_array->max_size * sizeof(do_cell_t));
    }
    do_array->used_size++;

    /*there are mainly two cases:   1) simple replace
                                    2) replace + extra rows */
    int size_of_previous_state = do_array->array[do_array->used_size-1].end - do_array->array[do_array->used_size-1].begin + 1;

    if (end <= size_of_previous_state){ //case 1: replace
        do_array->array[do_array->used_size].begin = do_array->array[do_array->used_size-1].end + 1;
        do_array->array[do_array->used_size].end = do_array->array[do_array->used_size-1].end + size_of_previous_state;
    }
    else{ //case 2: replace + new cells
        do_array->array[do_array->used_size].begin = do_array->array[do_array->used_size-1].end + 1;
        do_array->array[do_array->used_size].end = do_array->array[do_array->used_size-1].end + end;
    }
}

void array_delete (do_array_t* do_array, dynamic_array_t* text_array, int start, int end)
{
    int previous_state_size = do_array->array[do_array->used_size-1].end - do_array->array[do_array->used_size-1].begin + 1;

    if (previous_state_size == 1){ //cheks that the previous state is not the "initial state" or the "empty state"
        if (text_array->used_size == text_array->max_size){
            text_array->max_size *= 2;
            text_array->array = realloc(text_array->array, text_array->max_size * sizeof(cell_t));
        }
        if (text_array->array[do_array->array[do_array->used_size-1].begin].text_line == empty_state_text || do_array->array[do_array->used_size-1].begin == -1){
            text_array->array[do_array->array[do_array->used_size].begin].text_line = empty_state_text;
            text_array->used_size++;
            text_array->last_version_used_size = 1;
            return;
        }
    }

    int i;
    if (end < 1 || start >text_array->last_version_used_size){ //there are no deletions: it is a sort of empty command. In practice copies eventual values in the previous state
        text_array->last_version_used_size = 0;
        int lines_to_be_copied = do_array->array[do_array->used_size].end - do_array->array[do_array->used_size].begin + 1;
        for (i = 0; i < lines_to_be_copied; i++) { //copies the eventual values before the nodes that have to be deleted
            if (text_array->used_size == text_array->max_size){
                text_array->max_size *= 2;
                text_array->array = realloc(text_array->array, text_array->max_size * sizeof(cell_t));
            }
            text_array->array[do_array->array[do_array->used_size].begin+i].text_line = text_array->array[do_array->array[do_array->used_size-1].begin+i].text_line; //copies eventual values present in the previous state
            text_array->used_size++;
            text_array->last_version_used_size++;
        }
    }else{
        text_array->last_version_used_size = 0;

        int start_delete = max (1,start);
        int end_delete = min (end, previous_state_size);

        if (start_delete == 1 && end_delete == previous_state_size){
            if (text_array->used_size == text_array->max_size){
                text_array->max_size *= 2;
                text_array->array = realloc(text_array->array, text_array->max_size * sizeof(cell_t));
            }
            text_array->array[do_array->array[do_array->used_size].begin].text_line = empty_state_text;
            text_array->used_size++;
            text_array->last_version_used_size = 1;
            return;
        }

        for (i = 0; i < start-1; i++) { //copies the eventual values before the nodes that have to be deleted
            if (text_array->used_size == text_array->max_size) {
                text_array->max_size *= 2;
                text_array->array = realloc(text_array->array, text_array->max_size * sizeof(cell_t));
            }
            text_array->array[do_array->array[do_array->used_size].begin+i].text_line = text_array->array[do_array->array[do_array->used_size-1].begin +i].text_line; //copies eventual values present in the previous state
            text_array->used_size++;
            text_array->last_version_used_size++;
        }

        for (i = 0; i < previous_state_size - end; i++){
            if (text_array->used_size == text_array->max_size) {
                text_array->max_size *= 2;
                text_array->array = realloc(text_array->array, text_array->max_size * sizeof(cell_t));
            }
            text_array->array[do_array->array[do_array->used_size].begin+text_array->last_version_used_size].text_line = text_array->array[do_array->array[do_array->used_size-1].begin+end+i].text_line; //ricopio gli eventuali valori presenti nel vecchio stato
            text_array->used_size++;
            text_array->last_version_used_size++;
        }
    }
}

void do_array_delete(do_array_t *do_array, int start, int end)
{
    if (do_array->used_size == do_array->max_size){
        do_array->max_size *= 2;
        do_array->array = realloc(do_array->array, do_array->max_size * sizeof(do_cell_t));
    }
    do_array->used_size++;

    int previous_version_size = (do_array->array[do_array->used_size-1].end - do_array->array[do_array->used_size-1].begin) + 1;
    if(end < 1 || start > previous_version_size){ //case for a command such as "0,0d" or "-1,-1d" + delete of values subsequent the end
        do_array->array[do_array->used_size].begin = do_array->array[do_array->used_size-1].end + 1;
        do_array->array[do_array->used_size].end = do_array->array[do_array->used_size].begin + previous_version_size -1;
    } else {
        do_array->array[do_array->used_size].begin = do_array->array[do_array->used_size-1].end + 1;

        int start_delete = max (1,start);
        int end_delete = min (end, previous_version_size);
        int number_of_nodes_to_del = end_delete - start_delete + 1;
        int remaining_nodes = previous_version_size - number_of_nodes_to_del;

        if (remaining_nodes == 0 || remaining_nodes == 1)
            do_array->array[do_array->used_size].end = do_array->array[do_array->used_size].begin;
        else{
            do_array->array[do_array->used_size].end = do_array->array[do_array->used_size].begin + remaining_nodes - 1;
        }
    }

}

void print_text_array(dynamic_array_t* text_array, do_array_t* do_array, int start, int end)
{
    for (int i = start-1; i < end; i++) // i < end because also end must be print
        fputs(text_array->array[i+(do_array->array[do_array->used_size].begin)].text_line, stdout); //prints the i-th row adding an offset specified in the do-array
}

int main() {
    int start, end, i;
    int line_number;
    int command;

    int start_do; //variable used to implement the "algebraic sum" between undo-s and redo-s in order to avoid useless undo/redo
    int temporary_undo_stack_size;
    int tmp_start;
    int redo_available = 0;

    char text[MAXLINESIZE+1];
    char * effective_string;

    dynamic_array_t* text_array;
    do_array_t* do_array;
    text_array = malloc(sizeof(dynamic_array_t));
    do_array = malloc(sizeof(do_array_t));
    init_text_array(text_array, TEXT_ARRAY_INITIAL_SIZE);
    init_do_array(do_array, DO_ARRAY_INITIAL_SIZE);

    do {
        scanf("%d,%d", &start, &end);
        command = getchar_unlocked();
        //analysis of the various cases based on command

        if (command == UNDO){
            getchar_unlocked();
            temporary_undo_stack_size = do_array->used_size;
            tmp_start = min(start, temporary_undo_stack_size);
            start_do = tmp_start;  //undo-s are counted as positive, redo-s as negative
            temporary_undo_stack_size = temporary_undo_stack_size - tmp_start;
            redo_available = redo_available + tmp_start;
            do{
                scanf("%d,%d", &start, &end);
                command = getchar_unlocked();
                if (command == UNDO){
                    tmp_start = min(start, temporary_undo_stack_size);
                    start_do = start_do + tmp_start;  //undo-s are counted as positive
                    temporary_undo_stack_size = temporary_undo_stack_size - tmp_start;
                    redo_available = redo_available + tmp_start;
                }
                else if (command == REDO){
                    tmp_start = min(start, redo_available);
                    start_do = start_do - tmp_start;
                    redo_available = redo_available - tmp_start;
                    temporary_undo_stack_size = temporary_undo_stack_size + tmp_start;
                }
            } while (command == UNDO || command == REDO);
            if (start_do > 0){ //it has to carry out a number of undo equal to start_do
                do_array->used_size = do_array->used_size - start_do; //moves to the correct state in the undo/red array
                if (do_array->used_size == 0)
                    text_array->last_version_used_size = 0;
                else
                    text_array->last_version_used_size = do_array->array[do_array->used_size].end - do_array->array[do_array->used_size].begin + 1;
                text_array->used_size = do_array->array[do_array->used_size].end + 1;
            } else if (start_do < 0 ){ //it has to carry out a number of redo equal to (-start_do)
                do_array->used_size = do_array->used_size - start_do; //because start-do is negative, it is actually summed
                text_array->last_version_used_size = do_array->array[do_array->used_size].end - do_array->array[do_array->used_size].begin + 1;
                text_array->used_size = do_array->array[do_array->used_size].end + 1;
            }
        }
        else if (command == REDO){
            getchar_unlocked();
            //undo-s are counted as positive, redo-s as negative

            temporary_undo_stack_size = do_array->used_size;
            tmp_start = min(start, redo_available);
            start_do = - tmp_start;
            redo_available = redo_available - tmp_start;
            temporary_undo_stack_size = temporary_undo_stack_size + tmp_start;
            do{
                scanf("%d,%d", &start, &end);
                command = getchar_unlocked();
                if (command == UNDO){
                    tmp_start = min(start, temporary_undo_stack_size);
                    start_do = start_do + tmp_start;  // undo-s are counted as positive
                    temporary_undo_stack_size = temporary_undo_stack_size - tmp_start;
                    redo_available = redo_available + tmp_start;
                }
                else if (command == REDO) {
                    tmp_start = min(start, redo_available);
                    start_do = start_do - tmp_start;
                    redo_available = redo_available - tmp_start;
                    temporary_undo_stack_size = temporary_undo_stack_size + tmp_start;
                }
            } while (command == UNDO || command == REDO);
            if (start_do > 0){ //it has to carry out a number of undo equal to start_do
                do_array->used_size = do_array->used_size - start_do; //moves to the correct state in the undo/red array
                if (do_array->used_size == 0)
                    text_array->last_version_used_size = 0;
                else
                    text_array->last_version_used_size = do_array->array[do_array->used_size].end - do_array->array[do_array->used_size].begin + 1;
                text_array->used_size = do_array->array[do_array->used_size].end + 1;
            } else if (start_do < 0 ){ //it has to carry out a number of redo equal to (-start_do)
                do_array->used_size = do_array->used_size - start_do; //because start-do is negative, it is actually summed
                text_array->last_version_used_size = do_array->array[do_array->used_size].end - do_array->array[do_array->used_size].begin + 1;
                text_array->used_size = do_array->array[do_array->used_size].end + 1;
            }
        }
        if (command == CHANGE){
            getchar_unlocked();
            if (start > text_array->last_version_used_size){ //case in which are added elements in the array without overwriting an already present row
                do_array_insert_only_no_replace(do_array, start, end);
                for (line_number = start; line_number <= end; line_number++){
                    fgets(text, MAXLINESIZE+1, stdin);
                    effective_string = malloc(strlen(text)+1);
                    strcpy(effective_string, text);
                    array_insert_no_replace(text_array, do_array, line_number, effective_string);
                }
            } else{ //case in which at least one element has to be overwritten
                do_array_insert_with_replace(do_array, end);
                array_insert_with_replace(text_array, do_array, start, end);
            }
            redo_available = 0; //"empty" stack_redo
        }

        else if (command == DELETE){
            getchar_unlocked();
            do_array_delete(do_array, start, end);
            array_delete(do_array, text_array, start, end);

            redo_available = 0; //"empty" stack_redo
        }

        else if (command == PRINT){
            getchar_unlocked();
            while (start<1){
                fputc_unlocked(POINT, stdout);
                fputc_unlocked(NEWLINE, stdout);
                start++;
            }
            if (start > text_array->last_version_used_size){
                for (i = start; i <= end; i++){
                    fputc_unlocked(POINT, stdout);
                    fputc_unlocked(NEWLINE, stdout);
                }
            } else {
                print_text_array(text_array, do_array,  start, min(end, text_array->last_version_used_size));
                for (i = text_array->last_version_used_size; i < end; i++){
                    fputc_unlocked(POINT, stdout);
                    fputc_unlocked(NEWLINE, stdout);
                }
            }
        }
    } while (command != QUIT);

    return 0;

}