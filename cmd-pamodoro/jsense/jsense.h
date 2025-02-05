/*****************************************************************
*
* JSENSE: minimalist json parsing library
*
* created by Gregg Ink (2023)
* released under zlib license
*
* allows you to read json files, not designed for writing them
* This library follows the specification very closely
* as released on json.org and rfc 8259
*
*
* to use this library in your code, just include jsense.h
* technical.c and technical.h must be present in same directory as jsense.h
*
* to compile on Linux, use -pthread flag to compile
* e.g. gcc -o program_name main.c -pthread
*
* to compile on Windows:
* this code was tested on Windows 10 with MinGW,
* compiled with:
* set PATH=%PATH%;C:\MinGW\bin
* gcc -o program_name main.c -lShlwapi
*
*
******************************************************************/

#ifndef JSENSE_H
#define JSENSE_H

#include "technical.h"
#include <string.h>

//maximum file or buffer size to be parsed in bytes
// 31457280 == 30MB
#define JSE_MAX_SIZE 31457280
#define JSE_ERROR_SIZE 1000

//max size of label, a.k.a key
#define JSE_MAX_LABEL_SIZE 256

//for parsing purposes, nesting limit
#define JSE_STACK_SIZE 120
//actual chunk size will be JSE_CHUNK_SIZE + 1
#define JSE_CHUNK_SIZE 20

//settings for each key-value pair or value
//#define JSE_IS_NUMBER 2
#define JSE_KEY_NEEDS_ESCAPING 4
#define JSE_VALUE_NEEDS_ESCAPING 8
//#define JSE_NEXT_CHUNK 16

typedef struct _JSENSE JSENSE;
typedef struct _json_stuff json_stuff;

struct _JSENSE{

	int from_file;
	char *buffer;
	char *progress;
	int num_tokens;
	int first_token_type;
	int first_token_len;
	char *first_token_data;
	int first_token_needs_escaping;
	char error[JSE_ERROR_SIZE];
	int reached_end;

	char literals[16];
	char *l_true;
	char *l_false;
	char *l_null;

	json_stuff *root;
	json_stuff *last_returned;
	int last_returned_index;

};

struct _json_stuff{

	int settings;
	int data_len;

	char *key;
	char *value;
	json_stuff *sub;

};

enum stuff_type { JSE_TYPE_ERROR, JSE_JUST_VALUE, JSE_STRING, JSE_OBJECT, JSE_ARRAY, JSE_KEY_VALUE};
enum token_types { JSE_TOKEN_ERROR, JSE_TOKEN_OPEN_OBJ, JSE_TOKEN_CLOSE_OBJ, JSE_TOKEN_OPEN_ARRAY, JSE_TOKEN_CLOSE_ARRAY, JSE_TOKEN_COLON, JSE_TOKEN_COMMA, JSE_TOKEN_STRING, JSE_TOKEN_NUMBER, JSE_TOKEN_LITERAL, JSE_TOKEN_END };

JSENSE* jse_from_file(char *file);

/*jse_from_buffer:
	this function might modify the buffer
	make a copy if you need it
	must be NULL-terminated
	jsense will not free this buffer
	do not free buffer before calling jse_free(j)
*/
JSENSE* jse_from_buffer(char *buffer, uint64_t buffer_size);

/*jse_get:
	do not free returned value
	everything gets freed when calling jse_free(j) (if from file)
	this function always returns a string as it appears in the file
	if you need a number, you can use:
	double d = tec_string_to_double( jse_get(j, "example.foo[5].num") );
*/
char* jse_get(JSENSE *j, char *label);

char *jse_get_next(JSENSE *j);

void jse_free(JSENSE *j);

///////////////////////////////////////////////////////////////////////////
//	functions with single letter prefixes are intended for internal use
//	do not call unless you really know what you are doing
///////////////////////////////////////////////////////////////////////////
JSENSE* j_from_buffer(char *buffer, int from_file);
void j_free(json_stuff *st);
void j_parse(JSENSE *j);

char* j_get_token(JSENSE *j, int *type, int *len, int *needs_escaping);
char* j_parse_string(char *buffer, int *len, int *needs_escaping);
char* j_parse_number(JSENSE *j, char *buffer, int *len);
int j_is_white_space(char ch);

json_stuff* j_new_stuff(JSENSE *j);
void j_unescape_string(char *str);

void j_debug_print_structure(JSENSE *j);
void j_debug_print_structure_sub(json_stuff *st, int level);
void j_debug_print_token(char *token, int type, int len);

JSENSE* jse_from_file(char *file){

	if( !tec_file_exists(file) ){
		tec_error("file %s does not exist", file);
		return NULL;
	}

	if( tec_path_is_dir(file) ){
		tec_error("path given is a directory (%s)", file);
		return NULL;
	}

	uint64_t size = tec_file_get_size(file);
	if(size > JSE_MAX_SIZE){
		tec_error("File bigger than max. allowed (JSE_MAX_SIZE == %d bytes)", JSE_MAX_SIZE);
		return NULL;
	}

	char *buffer = tec_file_get_contents(file);
	if(!buffer){
		tec_error("failed to load file %s", file);
		return NULL;
	}

	return j_from_buffer(buffer, 1);

}//jse_from_file*/

JSENSE* jse_from_buffer(char *buffer, uint64_t size){

	if(!buffer || !(*buffer) || !size){
		tec_error("empty buffer");
		return NULL;
	}

	if(size > JSE_MAX_SIZE){
		tec_error("Buffer bigger than max. allowed (JSE_MAX_SIZE == %d bytes)", JSE_MAX_SIZE);
		return NULL;
	}

	return j_from_buffer(buffer, 0);

}//jse_from_buffer*/

JSENSE* j_from_buffer(char *buffer, int from_file){

	JSENSE *j = (JSENSE *) malloc(sizeof(JSENSE) );
	if(!j){
		tec_error("memory allocation error");
		if(from_file){
			free(buffer);
		}
		return NULL;
	}
	memset(j, 0, sizeof(JSENSE) );
	j->from_file = from_file;
	j->buffer = buffer;
	j->progress = buffer;
	tec_string_copy(j->literals, "true", 5);
	tec_string_copy(j->literals+5, "false", 6);
	tec_string_copy(j->literals+11, "null", 5);
	j->l_true = j->literals;
	j->l_false = j->literals + 5;
	j->l_null = j->literals + 11;

	j_parse(j);

	if(j->num_tokens == 1 && j->reached_end){
		switch(j->first_token_type){
		case JSE_TOKEN_STRING:
		case JSE_TOKEN_NUMBER:
		case JSE_TOKEN_LITERAL:
			return j;
			break;
		default:
			tec_error("Invalid json file");
			jse_free(j);
			return NULL;
			break;
		}
	}
	j->first_token_type = 0;	//to make jse_get() work well
	if(j->error[0]){
		tec_error("JSENSE parsing error: %s", j->error);
		jse_free(j);
		return NULL;
	}

	return j;

}//j_from_buffer*/

char* jse_get(JSENSE *j, char *label){

	if(!j)
		return NULL;

	//deal with degenerate case first, single token file
	if(j->first_token_type){
		if(j->first_token_type == JSE_TOKEN_STRING){
			if(j->first_token_needs_escaping){
				j_unescape_string(j->first_token_data);
				j->first_token_needs_escaping = 0;
			}
			return j->first_token_data;
		}
		if(j->first_token_type == JSE_TOKEN_NUMBER){
			j->first_token_data[j->first_token_len] = 0;
			return j->first_token_data;
		}
		//literal
		return j->first_token_data;
	}

	if(!label){
		j->last_returned = NULL;
		return NULL;
	}

	json_stuff *tmp = j->root;
	char *to_find = NULL;
	int n = tec_string_find_char(label, '.');
	int i = 0;
	char **labels = NULL;
	int num = 0;

	//label needs to be copied in case
	//label given is a constant and
	//cannot be altered
	//i.e.: value = jse_get(j, "label[5]");
	if(tec_string_length(label) >= JSE_MAX_LABEL_SIZE){
		tec_error("label passed on to jse_get cannot be bigger than %d, incl. NULL character", JSE_MAX_LABEL_SIZE);
		j->last_returned = NULL;
		return NULL;
	}
	char label_copy[JSE_MAX_LABEL_SIZE];
	tec_string_copy(label_copy, label, JSE_MAX_LABEL_SIZE);
	if(n != -1){
		labels = tec_string_split(label_copy, '.', &num);
		to_find = labels[0];
	}else{
		to_find = label_copy;
	}

	int k = 0;
	int index = 0;
	int not_found = 1;
	while(to_find){

		index = 0;
		int original_index = 0;
		int pos_index = tec_string_find_char(to_find, '[');
		if(pos_index != -1){
			to_find[pos_index] = 0;
			pos_index += 1;
			index = tec_string_to_int( &(to_find[pos_index]) );
			original_index = index;
		}else{
			index = -1;
		}

		k = 0;

		//TODO(GI):
		//Do I want to use tmp[JSE_CHUNK_SIZE].settings when there is no index????

		if(to_find[0]){	//to_find[0] could be NULL if we have label that is only an index e.g. "[5]"
			not_found = 1;
			while(not_found){

				if(tmp[k].settings & JSE_KEY_NEEDS_ESCAPING){
					j_unescape_string(tmp[k].key);
					tmp[k].settings -= JSE_KEY_NEEDS_ESCAPING;
				}
				if(tec_string_equal(to_find, tmp[k].key)){
					not_found = 0;
					if(tmp[k].value){
						if(tmp[k].data_len){
							tmp[k].value[tmp[k].data_len] = 0;
						}else{
							if(tmp[k].settings & JSE_VALUE_NEEDS_ESCAPING){
								j_unescape_string(tmp[k].value);
								tmp[k].settings -= JSE_VALUE_NEEDS_ESCAPING;
							}
						}
						if(labels){
							free(labels);
						}
						j->last_returned = tmp;
						j->last_returned_index = k;
						return tmp[k].value;
					}else{
						if(tmp[k].sub){
							tmp = tmp[k].sub;
						}
					}
				}
				k += 1;

				if(k == JSE_CHUNK_SIZE){
					if(tmp[k].sub){
						tmp = tmp[k].sub;
						k = 0;
					}else{
						tec_error("%s not found", to_find);
						j->last_returned = NULL;
						return NULL;
					}
				}

			}
		}

		if(index != -1){

			while(index >= JSE_CHUNK_SIZE){
				if( tmp[JSE_CHUNK_SIZE].sub ){
					tmp = tmp[JSE_CHUNK_SIZE].sub;
					index -= JSE_CHUNK_SIZE;
				}else{
					tec_error("JSENSE-ERROR: index %d not found", original_index);
					if(labels){
						free(labels);
					}
					j->last_returned = NULL;
					return NULL;
				}
			}

			if(tmp[JSE_CHUNK_SIZE].settings <= index){
				if(labels)
					free(labels);
				tec_error("JSENSE-ERROR: index too large");
				return NULL;
			}
			tmp = &(tmp[index]);

			if(tmp->sub){
				tmp = tmp->sub;
				index = 0;
			}else{
				if(tmp->value){
					if(tmp->data_len){
						tmp->value[tmp->data_len] = 0;
					}else{
						if(tmp->settings & JSE_VALUE_NEEDS_ESCAPING){
							j_unescape_string(tmp->value);
							tmp->settings -= JSE_VALUE_NEEDS_ESCAPING;
						}
					}
					if(labels){
						free(labels);
					}
					j->last_returned = tmp;
					j->last_returned_index = index;
					return tmp->value;
				}
			}

		}

		if(n == -1){
			to_find = NULL;
		}else{
			i += 1;
			to_find = labels[i];
		}

		if(!tmp || ( !tmp->key && !tmp->value && !tmp->sub) ){
			if(labels)
				free(labels);
			return NULL;
		}

	}

	if(labels){
		free(labels);
	}

	if(tmp){
		if(tmp->settings & JSE_VALUE_NEEDS_ESCAPING){
			j_unescape_string(tmp->value);
			tmp->settings -= JSE_VALUE_NEEDS_ESCAPING;
		}
		j->last_returned = tmp;
		if(index != -1)
			j->last_returned_index = index;
		return tmp->value;
	}

	j->last_returned = NULL;
	return NULL;

}//jse_get*/

char *jse_get_next(JSENSE *j){

	if(!j)
		return NULL;

	if(j->first_token_type)
		return NULL;

	if(!j->last_returned)
		return NULL;

	json_stuff *tmp = j->last_returned;
	int index = j->last_returned_index + 1;

	if(index == JSE_CHUNK_SIZE){
		if(tmp[index].sub){
			tmp = tmp[index].sub;
			index = 0;
		}else{
			return NULL;
		}
	}

//	if(tmp[index].key){
		j->last_returned_index = index;
		return tmp[index].value;
//	}

	return NULL;

}//jse_get_next*/

void jse_free(JSENSE *j){

	if(!j)
		return;

	if(j->root)
		j_free(j->root);
	if(j->from_file)
		free(j->buffer);
	free(j);

}//jse_free*/

void j_free(json_stuff *st){

	int i = 0;
	int not_done = 1;
	int in_chunk = 0;
	json_stuff *tmp = NULL;

	while(not_done){

		not_done = 0;
		in_chunk = 1;
		while(i < JSE_CHUNK_SIZE && in_chunk){

			if(!st[i].key && !st[i].value && !st[i].sub){
				in_chunk = 0;
			}

			if(st[i].sub){
				j_free(st[i].sub);
			}
			i += 1;
		}

		if(st[JSE_CHUNK_SIZE].sub){
			tmp = st[JSE_CHUNK_SIZE].sub;
			free(st);
			st = tmp;
			not_done = 1;
			i = 0;
		}

	}

	free(st);

}//j_free*/

void j_parse(JSENSE *j){

	int token_type = 0;
	int len = 0;
	int needs_escaping = 0;
	json_stuff *st = NULL;
	json_stuff *prev = NULL;
	json_stuff *pprev = NULL;
	int prev_token_type = 0;
	//next_key_value:
	//set to 1 : we expect key next
	//set to 2 : we expect value next
	//set to 0 : we expect neither next
	int next_key_value = 0;

	char stack[JSE_STACK_SIZE];
	int stack_index = -1;
	json_stuff *stack_pointers[JSE_STACK_SIZE];
	int stack_positions[JSE_STACK_SIZE];
	memset(stack, 0, sizeof(char) * JSE_STACK_SIZE);
	memset(stack_pointers, 0, sizeof(json_stuff *) * JSE_STACK_SIZE);
	memset(stack_positions, 0, sizeof(int) * JSE_STACK_SIZE);

	//if present, removing useless byte order mark (bom)
	uint32_t bom = 0x00bfbbef;
	uint32_t *first_bytes = (uint32_t *) j->buffer;
	if( (bom & *first_bytes) == 0x00bfbbef){
		j->progress += 3;
	}

	char *token = j_get_token(j, &token_type, &len, &needs_escaping);
	j->first_token_type = token_type;
	j->first_token_len = len;
	j->first_token_data = token;
	j->first_token_needs_escaping = needs_escaping;
	while(token){
		j->num_tokens += 1;

		switch(token_type){
		case JSE_TOKEN_ERROR:
		/////////////////////
			if(!j->error[0]){
				tec_string_copy(j->error, "Unknown token", JSE_ERROR_SIZE);
				return;
			}
			return;
			break;

		case JSE_TOKEN_OPEN_OBJ:
		////////////////////////
			prev = st;
			st = j_new_stuff(j);

			switch(prev_token_type){
			case 0:
				//do nothing
				break;
			case JSE_TOKEN_COMMA:
			case JSE_TOKEN_COLON:
			case JSE_TOKEN_OPEN_ARRAY:
				if(stack_index < 0){
					//should never run
					tec_string_copy(j->error, "unexpected situation (5)", JSE_ERROR_SIZE);
					return;
				}
				if(stack_positions[stack_index] == JSE_CHUNK_SIZE){
					pprev = prev;
					prev = j_new_stuff(j);
					pprev[stack_positions[stack_index]].sub = prev;
					stack_positions[stack_index] = 0;
					stack_pointers[stack_index] = prev;
				}

				(prev[stack_positions[stack_index]]).sub = st;
				stack_positions[stack_index] += 1;
				break;

				//////// handling error cases
			case JSE_TOKEN_OPEN_OBJ:
				tec_string_copy(j->error, "Two open object characters in a row. Not valid json.", JSE_ERROR_SIZE);
				return;
				break;
			case JSE_TOKEN_CLOSE_OBJ:
				tec_string_copy(j->error, "Open object character follows closed one without comma. Not valid json.", JSE_ERROR_SIZE);
				return;
				break;
			case JSE_TOKEN_CLOSE_ARRAY:
				tec_string_copy(j->error, "Open object character follows closed array character without comma. Not valid json.", JSE_ERROR_SIZE);
				return;
				break;
			case JSE_TOKEN_STRING:
			case JSE_TOKEN_NUMBER:
			case JSE_TOKEN_LITERAL:
				tec_string_copy(j->error, "Open object character immediately follows string/number/literal. Not valid json.", JSE_ERROR_SIZE);
				return;
				break;
			default:
				tec_string_copy(j->error, "unexpected situation (1)", JSE_ERROR_SIZE);
				return;
			}

			stack_index += 1;
			if(stack_index >= JSE_STACK_SIZE){
				tec_string_copy(j->error, "too many levels deep", JSE_ERROR_SIZE);
				return;
			}

			if(prev){
				// increase counter
				prev[JSE_CHUNK_SIZE].settings += 1;
			}

			stack[stack_index] = 'o';
			stack_pointers[stack_index] = st;

			next_key_value = 1;
			prev = st;
			break;

		case JSE_TOKEN_CLOSE_OBJ:
		/////////////////////////
			next_key_value = 0;
			if(!st){
				tec_string_copy(j->error, "Closed object, no open object found.", JSE_ERROR_SIZE);
				return;
			}
			switch(prev_token_type){
			case JSE_TOKEN_COMMA:
				tec_string_copy(j->error, "Stray comma before '}'", JSE_ERROR_SIZE);
				return;
				break;
			case JSE_TOKEN_COLON:
			case JSE_TOKEN_OPEN_ARRAY:
				tec_string_copy(j->error, "Unexpected symbol before '}'", JSE_ERROR_SIZE);
				return;
				break;
			}

			if(stack_index < 0){
				tec_string_copy(j->error, "stack_index is smaller than zero", JSE_ERROR_SIZE);
				return;
			}
			if(stack[stack_index] != 'o'){
				tec_string_copy(j->error, "incorrect nesting of objects/arrays", JSE_ERROR_SIZE);
				return;
			}
			stack[stack_index] = 0;
			stack_pointers[stack_index] = 0;
			stack_positions[stack_index] = 0;
			stack_index -= 1;
			if(stack_index >= 0)
				st = stack_pointers[stack_index];
			prev = st;

			break;

		case JSE_TOKEN_OPEN_ARRAY:
		//////////////////////////
			prev = st;
			st = j_new_stuff(j);

			switch(prev_token_type){
			case 0:
				//do nothing
				break;
			case JSE_TOKEN_COMMA:
			case JSE_TOKEN_COLON:
			case JSE_TOKEN_OPEN_ARRAY:

				if(stack_index < 0){
					//should never run
					tec_string_copy(j->error, "unexpected situation (4)", JSE_ERROR_SIZE);
					return;
				}
				if(stack_positions[stack_index] == JSE_CHUNK_SIZE){
					pprev = prev;
					prev = j_new_stuff(j);
					pprev[stack_positions[stack_index]].sub = prev;
					stack_positions[stack_index] = 0;
					stack_pointers[stack_index] = prev;
				}

				(prev[stack_positions[stack_index]]).sub = st;
				stack_positions[stack_index] += 1;
				break;

				//////// handling error cases
			case JSE_TOKEN_OPEN_OBJ:
				tec_string_copy(j->error, "Open array character follows open object character. Not valid json.", JSE_ERROR_SIZE);
				return;
				break;
			case JSE_TOKEN_CLOSE_OBJ:
				tec_string_copy(j->error, "Open array character follows closed object character without comma. Not valid json.", JSE_ERROR_SIZE);
				return;
				break;
			case JSE_TOKEN_CLOSE_ARRAY:
				tec_string_copy(j->error, "Open array character follows closed array character without comma. Not valid json.", JSE_ERROR_SIZE);
				return;
				break;
			case JSE_TOKEN_NUMBER:
			case JSE_TOKEN_STRING:
			case JSE_TOKEN_LITERAL:
				tec_string_copy(j->error, "Open array character immediately follows string/number/literal. Not valid json.", JSE_ERROR_SIZE);
				return;
				break;
			default:
				tec_string_copy(j->error, "unexpected situation (2)", JSE_ERROR_SIZE);
				return;
				break;
			}

			stack_index += 1;
			if(stack_index >= JSE_STACK_SIZE){
				tec_string_copy(j->error, "too many levels deep", JSE_ERROR_SIZE);
				return;
			}

			if(prev){
				// increase counter
				prev[JSE_CHUNK_SIZE].settings += 1;
			}

			stack[stack_index] = 'a';
			stack_pointers[stack_index] = st;

			next_key_value = 2;
			prev = st;
			break;

		case JSE_TOKEN_CLOSE_ARRAY:
		///////////////////////////
			next_key_value = 0;
			if(!st){
				tec_string_copy(j->error, "Closed array, no open array found.", JSE_ERROR_SIZE);
				return;
			}
			switch(prev_token_type){
			case JSE_TOKEN_COMMA:
				tec_string_copy(j->error, "Stray comma before ']'", JSE_ERROR_SIZE);
				return;
			case JSE_TOKEN_OPEN_OBJ:
			case JSE_TOKEN_COLON:
				tec_string_copy(j->error, "Unexpected symbol before ']'", JSE_ERROR_SIZE);
				return;
				break;
			}

			if(stack_index < 0){
				tec_string_copy(j->error, "stack_index is smaller than zero", JSE_ERROR_SIZE);
				return;
			}
			if(stack[stack_index] != 'a'){
				tec_string_copy(j->error, "incorrect nesting of objects/arrays", JSE_ERROR_SIZE);
				return;
			}
			stack[stack_index] = 0;
			stack_pointers[stack_index] = 0;
			stack_positions[stack_index] = 0;
			stack_index -= 1;
			if(stack_index >= 0)
				st = stack_pointers[stack_index];
			prev = st;
			break;

		case JSE_TOKEN_LITERAL:
			//deliberately falling through, we treat literals the same as strings
		case JSE_TOKEN_STRING:
		//////////////////////

			if(!st){
				//this should be a string only situation
				//return;
				break;
			}
			if(!next_key_value){
				tec_string_copy(j->error, "not expecting string/literal here", JSE_ERROR_SIZE);
				return;
			}

			if(stack_positions[stack_index] == JSE_CHUNK_SIZE){
				prev = st;
				st = j_new_stuff(j);
				prev[stack_positions[stack_index]].sub = st;
				stack_positions[stack_index] = 0;
				stack_pointers[stack_index] = st;
			}

			if( stack[stack_index] == 'a' || (stack[stack_index] == 'o' && next_key_value == 1) ){
				// increase counter
				st[JSE_CHUNK_SIZE].settings += 1;
			}

			switch(next_key_value){
			case 1:
				(st[stack_positions[stack_index]]).key = token;
				if(needs_escaping){
					st[stack_positions[stack_index]].settings |= JSE_KEY_NEEDS_ESCAPING;
				}
				break;
			case 2:
				(st[stack_positions[stack_index]]).value = token;
				if(needs_escaping){
					st[stack_positions[stack_index]].settings |= JSE_VALUE_NEEDS_ESCAPING;
				}
				stack_positions[stack_index] += 1;
				break;
			default:
				tec_string_copy(j->error, "unexpected situation (3)", JSE_ERROR_SIZE);
				return;
				break;
			}

			next_key_value = 0;
			break;
		case JSE_TOKEN_NUMBER:
		//////////////////////

			if(!st){
				//this should be a number only situation
				break;
			}
			if(next_key_value != 2){
				tec_string_copy(j->error, "number in wrong place", JSE_ERROR_SIZE);
				return;
			}

			if(stack_positions[stack_index] == JSE_CHUNK_SIZE){
				prev = st;
				st = j_new_stuff(j);
				prev[stack_positions[stack_index]].sub = st;
				stack_positions[stack_index] = 0;
				stack_pointers[stack_index] = st;
			}

			//increase counter
			st[JSE_CHUNK_SIZE].settings += 1;

			(st[stack_positions[stack_index]]).value = token;
			(st[stack_positions[stack_index]]).data_len = len;
			stack_positions[stack_index] += 1;

			next_key_value = 0;
			break;
		case JSE_TOKEN_COLON:
		/////////////////////
			if(stack[stack_index] != 'o'){
				tec_string_copy(j->error, "colon outside of an object", JSE_ERROR_SIZE);
				return;
			}
			switch(prev_token_type){
			case JSE_TOKEN_CLOSE_OBJ:
			case JSE_TOKEN_CLOSE_ARRAY:
			case JSE_TOKEN_COMMA:
			case JSE_TOKEN_COLON:
			case JSE_TOKEN_NUMBER:
			case JSE_TOKEN_LITERAL:
				tec_string_copy(j->error, "colon after wrong symbol", JSE_ERROR_SIZE);
				return;
			}
			next_key_value = 2;
			break;
		case JSE_TOKEN_COMMA:
		/////////////////////
			switch(prev_token_type){
			case JSE_TOKEN_OPEN_OBJ:
			case JSE_TOKEN_OPEN_ARRAY:
			case JSE_TOKEN_COMMA:
			case JSE_TOKEN_COLON:
				tec_string_copy(j->error, "comma found in wrong location", JSE_ERROR_SIZE);
				return;
				break;
			}
			if(stack_index < 0){
				tec_string_copy(j->error, "stray comma", JSE_ERROR_SIZE);
				return;
			}
			if(stack[stack_index] == 'o'){
				next_key_value = 1;
			}else{
				next_key_value = 2;
			}
			break;
		default:
			tec_string_copy(j->error, "unexpected symbol encountered while parsing", JSE_ERROR_SIZE);
			return;
		}

		prev_token_type = token_type;
		token = j_get_token(j, &token_type, &len, &needs_escaping);
	}

	if(token_type == JSE_TOKEN_END){
		j->reached_end = 1;
	}

	if(!j->error[0] && (stack_index > -1 || stack[0]) ){
		tec_string_copy(j->error, "end of object / array not found", JSE_ERROR_SIZE);
		return;
	}

}//j_parse*/

char* j_get_token(JSENSE *j, int *type, int *len, int *needs_escaping){

	char *buffer = j->progress;
	char *str = NULL;

	while( j_is_white_space(*buffer) ){
		buffer += 1;
	}

	if(!buffer || !*buffer){
		*len = 0;
		*type = JSE_TOKEN_END;
		return 0;
	}

	//check for literals: false, true and null
	int literal = 0;
	if( tec_buf_begins(buffer, "true") ){
		literal = 1;
		*len = 4;
		str = j->l_true;
	}
	if( tec_buf_begins(buffer, "null") ){
		literal = 1;
		*len = 4;
		str = j->l_null;
	}
	if( tec_buf_begins(buffer, "false") ){
		literal = 1;
		*len = 5;
		str = j->l_false;
	}
	if(literal){
		buffer += *len;
		if(*buffer &&  (*buffer != ',' && *buffer != ']' && *buffer != '}' && !j_is_white_space(*buffer) ) ){
			tec_string_copy(j->error, "Invalid literal", JSE_ERROR_SIZE);
			*len = 0;
			*type = JSE_TOKEN_ERROR;
			return NULL;
		}
		*type = JSE_TOKEN_LITERAL;
		j->progress = buffer;
		return str;
	}

	switch(*buffer){
	case '{':
		*type = JSE_TOKEN_OPEN_OBJ;
		*len = 1;
		j->progress = buffer + 1;
		return buffer;
		break;
	case '}':
		*type = JSE_TOKEN_CLOSE_OBJ;
		*len = 1;
		j->progress = buffer + 1;
		return buffer;
		break;
	case '[':
		*type = JSE_TOKEN_OPEN_ARRAY;
		*len = 1;
		j->progress = buffer + 1;
		return buffer;
		break;
	case ']':
		*type = JSE_TOKEN_CLOSE_ARRAY;
		*len = 1;
		j->progress = buffer + 1;
		return buffer;
		break;
	case ':':
		*type = JSE_TOKEN_COLON;
		*len = 1;
		j->progress = buffer + 1;
		return buffer;
		break;
	case ',':
		*type = JSE_TOKEN_COMMA;
		*len = 1;
		j->progress = buffer + 1;
		return buffer;
		break;
	case '\"':
		*type = JSE_TOKEN_STRING;
		buffer += 1;	//skip over that quotation mark
		str = buffer;
		j->progress = j_parse_string(buffer, len, needs_escaping);
		if(!j->progress){
			tec_string_copy(j->error, "error parsing string", JSE_ERROR_SIZE);
			*type = JSE_TOKEN_ERROR;
			return 0;
		}
		return str;
		break;
	case '\'':
		tec_string_copy(j->error, "Single quote strings are not valid json", JSE_ERROR_SIZE);
		*type = JSE_TOKEN_ERROR;
		return 0;
		break;
	}

	if(*buffer == '-' || (*buffer >= '0' && *buffer <= '9') ){
		*type = JSE_TOKEN_NUMBER;
		j->progress = j_parse_number(j, buffer, len);
		if(!j->progress){
			if(!j->error[0]){
				tec_string_copy(j->error, "error parsing number", JSE_ERROR_SIZE);
			}
			*type = JSE_TOKEN_ERROR;
			return 0;
		}

		return buffer;
	}

	*type = JSE_TOKEN_ERROR;
	*len = 0;
	*needs_escaping = 0;
	return buffer;

}//j_get_token*/

char* j_parse_string(char *buffer, int *len, int *needs_escaping){

	char *start = buffer;
	int not_end = 1;
	*needs_escaping = 0;

	while(*buffer && not_end){
		if(*buffer == '\\'){
			buffer += 1;
			if( !(*buffer) ){
				return 0;
			}
			buffer += 1;
			*needs_escaping = 1;
		}else{
			if(*buffer == '\"'){
				not_end = 0;
			}else{
				buffer += 1;
			}
		}
	}

	if(*buffer != '\"'){
		return 0;
	}

	*buffer = 0;

	*len = buffer - start;

	return buffer + 1;

}//j_parse_string*/

char* j_parse_number(JSENSE *j, char *buffer, int *len){

	char *start = buffer;

	//NOTE(GI):
	//'+' is not permitted here for some reason
	//I am just basing this on json.org and the rfc
	if(*buffer == '-'){
		buffer += 1;
	}

	if(*buffer == '0'){
		//once here, this is wrong because we have a leading zero
		//unless, the number is just zero
		if( *(buffer+1) && ( *(buffer+1) >= '0' && *(buffer+1) <= '9') ){
			tec_string_copy(j->error, "leading zeroes not allowed in json for some reason", JSE_ERROR_SIZE);
			return 0;
		}
	}

	if( !(*buffer >= '0' && *buffer <= '9') ){
		//we could get here if we have only a '-'; not followed by digits
		tec_string_copy(j->error, "invalid number", JSE_ERROR_SIZE);
		return 0;
	}

	while( *buffer >= '0' && *buffer <= '9'){
		buffer += 1;
	}

	if(*buffer == '.'){
		buffer += 1;

		if( !(*buffer >= '0' && *buffer <= '9') ){
			tec_string_copy(j->error, "parsing number; no digits after the decimal point", JSE_ERROR_SIZE);
			return 0;
		}

		while( *buffer >= '0' && *buffer <= '9'){
			buffer += 1;
		}
	}

	if(*buffer == 'e' || *buffer == 'E'){
		buffer += 1;

		//NOTE(GI):
		//exponents can have a plus sign in front
		//because reasons
		if(*buffer == '+' || *buffer == '-'){
			buffer += 1;
		}

		if( !(*buffer >= '0' && *buffer <= '9') ){
			tec_string_copy(j->error, "parsing number; no digits in the exponent", JSE_ERROR_SIZE);
			return 0;
		}

		//NOTE(GI):
		//exponents apparently can have leading zeroes
		//just fine
		while( *buffer >= '0' && *buffer <= '9'){
			buffer += 1;
		}

	}

	*len = buffer - start;

	return buffer;

}//j_parse_number*/

int j_is_white_space(char ch){

	//NOTE(GI):
	//this is whitespace according to json.org
	//I deliberately ignored other symbols which
	//could be considered whitespace, e.g. vertical tab

	switch(ch){
	//deliberately falling through the cases
	case ' ':	//space
	case '\r':	//carriage return
	case '\t':	//hor. tab
	case '\n':	//linefeed
		return 1;
	}

	return 0;

}//j_is_white_space*/

json_stuff* j_new_stuff(JSENSE *j){

	json_stuff *st = (json_stuff *) malloc(sizeof(json_stuff) * (JSE_CHUNK_SIZE+1) );
	memset(st, 0, sizeof(json_stuff) * (JSE_CHUNK_SIZE+1) );

	if(!j->root){
		j->root = st;
	}

	return st;

}//j_new_stuff*/

void j_unescape_string(char *str){

	if(!str || !(*str) )
		return;

	char hex[5];
	hex[4] = 0;
	int bytes = 0;
	int i = 0;
	uint32_t prev_hex = 0;

	while(*str){
		if(*str == '\\'){
			char c = *(str+1);
			if(c){
				switch(c){
				case '\\':
				case '\"':
				case '/':
					tec_string_shift(str);
					break;
				case 'b':
					tec_string_shift(str);
					*str = '\b';
					break;
				case 'f':
					tec_string_shift(str);
					*str = '\f';
					break;
				case 'n':
					tec_string_shift(str);
					*str = '\n';
					break;
				case 'r':
					tec_string_shift(str);
					*str = '\r';
					break;
				case 't':
					tec_string_shift(str);
					*str = '\t';
					break;
				case 'u':
					i = 0;
					while(i < 4){
						hex[i] = *(str+i+2);
						i += 1;
					}
					i = 0;
					uint32_t h = tec_string_hex_to_int(hex);
					if(prev_hex && (h & 0xdc00) ){
						//dealing with utf-16 surrogate crap
						prev_hex &= 0x03ff;
						prev_hex <<= 10;
						h = prev_hex + (h & 0x03ff);
						h += 0x10000;
						prev_hex = 0;
					}else{
						if(h & 0xd800){
							prev_hex = h;
							i = 6;
							while(i){
								tec_string_shift(str);
								i -= 1;
							}
							str -= 1;
						}
					}
					if(!prev_hex){
						bytes = tec_string_utf8(h, str);
						str += bytes;
						*str = '*';	//removing null; otherwise tec_string_shift won't work
						i += (6 - bytes);
						while(i){
							tec_string_shift(str);
							i -= 1;
						}
						str -= 1;
					}
					break;
				default:
					tec_error("unexpected escape sequence");
					break;
				}
			}else{
				//should never run
				tec_error("backslash at end of string");
			}
		}
		str += 1;
	}

}//j_unescape_string*/

void j_debug_print_structure(JSENSE *j){

	printf("%sDebugging: printing JSENSE structure%s\n", TEC_PRINT_CYAN, TEC_PRINT_NORMAL);

	if(!j){
		printf("main JSENSE struct is NULL\n");
		return;
	}

	json_stuff *st = j->root;
	if(!st){
		printf("structure is empty\n");
		return;
	}

	j_debug_print_structure_sub(st, 0);

}//j_debug_print_structure*/

void j_debug_print_structure_sub(json_stuff *st, int level){

	int tmp = level;

	int i = 0;
	while(st[i].key || st[i].value || st[i].sub){

		//number and tabs
		printf("%d)", level);
		while(tmp){
			putchar(9);
			tmp -= 1;
		}
		tmp = level;

		if(st[i].key){
			printf("key %s", st[i].key);
			if(!st[i].sub){
				putchar(10);
			}
		}

		if(st[i].value){
			//number and tabs
			printf("%d)", level);
			while(tmp){
				putchar(9);
				tmp -= 1;
			}
			tmp = level;

			int len = st[i].data_len;
			if(len){
				char *val = st[i].value;
				printf("value: ");
				while(len){
					putchar(*val);
					val += 1;
					len -= 1;
				}
				printf(" len: %d\n", st[i].data_len);
			}else{
				printf("value %s\n", st[i].value);
			}
		}

		if(st[i].sub){
			putchar(10);
			j_debug_print_structure_sub(st[i].sub, level + 1);
		}

		i += 1;
	}

}//j_debug_print_structure_sub*/

void j_debug_print_token(char *token, int type, int len){

	printf("%s", TEC_PRINT_GREEN);
	switch(type){
	case JSE_TOKEN_OPEN_OBJ:
		printf("JSE_TOKEN_OPEN_OBJ%s (%d)\n\t{\n", TEC_PRINT_NORMAL, len);
		break;
	case JSE_TOKEN_CLOSE_OBJ:
		printf("JSE_TOKEN_CLOSE_OBJ%s (%d)\n\t}\n", TEC_PRINT_NORMAL, len);
		break;
	case JSE_TOKEN_OPEN_ARRAY:
		printf("JSE_TOKEN_OPEN_ARRAY%s (%d)\n\t[\n", TEC_PRINT_NORMAL, len);
		break;
	case JSE_TOKEN_CLOSE_ARRAY:
		printf("JSE_TOKEN_CLOSE_ARRAY%s (%d)\n\t]\n", TEC_PRINT_NORMAL, len);
		break;
	case JSE_TOKEN_COLON:
		printf("JSE_TOKEN_COLON%s (%d)\n\t:\n", TEC_PRINT_NORMAL, len);
		break;
	case JSE_TOKEN_COMMA:
		printf("JSE_TOKEN_COMMA%s (%d)\n\t,\n", TEC_PRINT_NORMAL, len);
		break;
	case JSE_TOKEN_STRING:
		printf("JSE_TOKEN_STRING%s (%d)\n\t%s\n", TEC_PRINT_NORMAL, len, token);
		break;
	case JSE_TOKEN_LITERAL:
		printf("JSE_TOKEN_LITERAL%s (%d)\n\t%s\n", TEC_PRINT_NORMAL, len, token);
		break;
	case JSE_TOKEN_NUMBER:
		//NOTE(GI):
		//this will print a lot more than just the number
		//I don't care
		printf("JSE_TOKEN_NUMBER%s (%d)\n\t%s\n", TEC_PRINT_NORMAL, len, token);
		break;
	case JSE_TOKEN_ERROR:
		printf("%sJSE_TOKEN_ERROR%s (%d)\n\t%s\n", TEC_PRINT_RED, TEC_PRINT_NORMAL, len, token);
		break;
	default:
		printf("%sunknown token%s\n", TEC_PRINT_YELLOW, TEC_PRINT_NORMAL);
		break;
	}

}//j_debug_print_token*/

#endif
