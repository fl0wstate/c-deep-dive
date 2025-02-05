
int tec_string_equal(char *str1, char *str2){

	if(!str1 && !str2)
		return 1;
	if(!str1 && str2 && !*str2)
		return 1;
	if(!str2 && str1 && !*str1)
		return 1;
	if(!str1)
		return 0;
	if(!str2)
		return 0;

	while(*str1 && *str1 == *str2){
		str1 += 1;
		str2 += 1;
	}
	if(!*str1 && !*str2)
		return 1;

	return 0;

}//tec_string_equal*/

int tec_string_length(char *str){

	if(!str)
		return 0;
	if(!*str)
		return 0;

	int len = 0;

#if __x86_64__

	int64_t *str_i = (int64_t *) str;
	int64_t addr = (int64_t) str_i;

	// 64 bit computer
	// ensure str is on 8-byte boundary before using speed-up trick
	while( addr&0x0000000000000007 && *str ){
		len += 1;
		str += 1;
		addr = (int64_t) str;
	}

	if(!*str){
		return len;
	}

	// check for NULL characters, 8 bytes at a time
	// https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
	str_i = (int64_t *) str;
	while( !( ( *str_i - 0x0101010101010101 ) & ~(*str_i) & 0x8080808080808080 ) ){
		len += 8;
		str_i += 1;
	}

	str = (char *) str_i;
	while(*str){
		len += 1;
		str += 1;
	}

#else

	int32_t *str32_i = (int32_t *) str;
	int32_t addr32 = (int32_t) str32_i;

	// 32 bit computer
	// ensure str is on 4-byte boundary before using speed-up trick
	while( addr32&0x00000003 && *str ){
		len += 1;
		str += 1;
		addr32 = (int32_t) str;
	}

	if(!*str){
		return len;
	}

	// check for NULL characters, 4 bytes at a time
	// https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
	str32_i = (int32_t *) str;
	while( !( ( *str32_i - 0x01010101 ) & ~(*str32_i) & 0x80808080 ) ){
		len += 4;
		str32_i += 1;
	}

	str = (char *) str32_i;
	while(*str){
		len += 1;
		str += 1;
	}

#endif

	return len;

}//tec_string_length*/

int tec_string_copy(char *destination, char *source, int size){

	if(!destination)
		return 0;
	if(!source){
		*destination = 0;
		return 1;
	}
	if(size <= 0)
		return 0;

	size -= 1;

	int i = 0;
	while(*source && i < size){
		destination[i] = *source;
		source += 1;
		i += 1;
	}

	// we don't want to cut off the copying in the middle of a UTF8 codepoint
	// firstly check whether the next byte of source is either not present or the start of a codepoint
	if(*source && (*source & TEC_MOST_SIG_BIT) && !(*source & TEC_2ND_MOST_SIG_BIT) ){
		i -= 1;
		// this while loop goes back while we have the 2nd, 3rd or 4th byte of a UTF8 codepoint
		while( (destination[i] & TEC_MOST_SIG_BIT) && !(destination[i] & TEC_2ND_MOST_SIG_BIT) ){
			i -= 1;
		}
		// this goes back from the head of a UTF8 codepoint
		if( (destination[i] & TEC_MOST_SIG_BIT) && (destination[i] & TEC_2ND_MOST_SIG_BIT) ){
			destination[i] = 0;
		}else{
			// should never happen, this would be invalid UTF8
			destination[i] = 0;
			return 0;
		}
	}

	destination[i] = '\0';

	if(*source){
		return 2;
	}else{
		return 1;
	}

}//tec_string_copy*/

int tec_string_copy_with_ellipsis(char *destination, char *source, int size){

	if(!destination)
		return 0;
	if(!source){
		*destination = 0;
		return 1;
	}
	if(size <= 0)
		return 0;

	int ellipsis = 0;
	int i = 0;

	if(size >= 4 && tec_string_length(source) >= size){
		ellipsis = 1;
		size -= 4;
	}else{
		size -= 1;
	}

	while(*source && i < size){
		destination[i] = *source;
		source += 1;
		i += 1;
	}

	// we don't want to cut off the copying in the middle of a UTF8 codepoint
	// firstly check whether the next byte of source is either not present or the start of a codepoint
	if(*source && (*source & TEC_MOST_SIG_BIT) && !(*source & TEC_2ND_MOST_SIG_BIT) ){
		i -= 1;
		// this while loop goes back while we have the 2nd, 3rd or 4th byte of a UTF8 codepoint
		while( (destination[i] & TEC_MOST_SIG_BIT) && !(destination[i] & TEC_2ND_MOST_SIG_BIT) ){
			i -= 1;
		}
		// this goes back from the head of a UTF8 codepoint
		if( (destination[i] & TEC_MOST_SIG_BIT) && (destination[i] & TEC_2ND_MOST_SIG_BIT) ){
			destination[i] = 0;
		}else{
			// should never happen, this would be invalid UTF8
			destination[i] = 0;
			return 0;
		}
	}

	if(ellipsis){
		destination[i] = '.';
		i += 1;
		destination[i] = '.';
		i += 1;
		destination[i] = '.';
		i += 1;
	}

	destination[i] = '\0';

	if(ellipsis || *source){
		return 2;
	}else{
		return 1;
	}

}//tec_string_copy_with_ellipsis*/

int tec_string_concatenate(char *destination, char *source, int size){

	if(!destination)
		return 0;
	if(!source)
		return 0;
	if(!*source)
		return 1;
	if(size < 2)
		return 0;

	int len = tec_string_length(destination);
	if(len > size)
		return 0;
	if(len == size && !*source)
		return 1;

	destination += len;
	size -= 1;

	while(*source && len < size){
		*destination = *source;
		len += 1;
		destination += 1;
		source += 1;
	}

	// we don't want to cut off the copying in the middle of a UTF8 codepoint
	// firstly check whether the next byte of source is either not present or the start of a codepoint
	if(*source && (*source & TEC_MOST_SIG_BIT) && !(*source & TEC_2ND_MOST_SIG_BIT) ){
		destination -= 1;
		// this while loop goes back while we have the 2nd, 3rd or 4th byte of a UTF8 codepoint
		while( (*destination & TEC_MOST_SIG_BIT) && !(*destination & TEC_2ND_MOST_SIG_BIT) ){
			destination -= 1;
		}
		// this goes back from the head of a UTF8 codepoint
		if( (*destination & TEC_MOST_SIG_BIT) && (*destination & TEC_2ND_MOST_SIG_BIT) ){
			*destination = 0;
		}else{
			// should never happen, this would be invalid UTF8
			*destination = 0;
			return 0;
		}
	}else{
		*destination = 0;
	}

	if(*source)
		return 2;
	return 1;

}//tec_string_concatenate*/

char** tec_string_split(char *str, char c, int *num){

	if(!str)
		return NULL;
	if(!c)
		return NULL;

	// we start by assuming that there will be no more than 32 instances of c
	int original_limit = 32;
	int limit = original_limit;
	char **table[26];
	int table_index = 0;
	int tmp_num = 0;
	*num = 0;

	char **res = (char **) malloc(sizeof(char *) * limit);
	if(!res){
		return NULL;
	}
	table[table_index] = res;

	res[0] = str;
	while(*str){
		if(*str == c){
			tmp_num += 1;
			*num += 1;
			if(tmp_num == limit){
				limit *= 2;
				table_index += 1;
				res = (char **) malloc(sizeof(char *) * limit);
				if(!res){
					return NULL;
				}
				table[table_index] = res;
				tmp_num = 0;
			}
			*str = '\0';
			str += 1;
			if(*str){
				res[tmp_num] = str;
			}else{
				//Note(GI) this deals with case where char c is last character in the string
				res[tmp_num] = NULL;
			}
		}else{
			str += 1;
		}
	}

	if(*num < original_limit){
		res[(*num)+1] = NULL;
		return res;
	}

	char **real_res = (char **) malloc(sizeof(char *) * (*num + 2));
	int ti = 0;
	int n = 0;
	int n2 = 0;
	limit = original_limit;
	while(ti <= table_index){

		res = table[ti];
		n2 = 0;
		while(n2 < limit && n <= *num){
			real_res[n] = res[n2];
			n2 += 1;
			n += 1;
		}

		free(res);
		limit *= 2;
		ti += 1;

	}
	real_res[(*num) + 1] = NULL;

	return real_res;

}//tec_string_split*/

char** tec_string_split_for_args(char *str, int *num){

	if(!str)
		return NULL;

	// we start by assuming that there will be no more than 32 instances of c
	int original_limit = 32;
	int limit = original_limit;
	char **table[26];
	int table_index = 0;
	int tmp_num = 0;
	*num = 0;
	int in_string = 0;
	char prev_str = 0;

	char **res = (char **) malloc(sizeof(char *) * limit);
	if(!res){
		return NULL;
	}
	table[table_index] = res;

	res[0] = str;
	while(*str){
		switch(*str){
		case '\"':
			if(prev_str != '\\'){
				if(in_string){
					in_string = 0;
				}else{
					in_string = 1;
				}
			}
			prev_str = *str;
			str += 1;
			break;
		case ' ':
			if(!in_string){
				tmp_num += 1;
				*num += 1;
				if(tmp_num == limit){
					limit *= 2;
					table_index += 1;
					res = (char **) malloc(sizeof(char *) * limit);
					if(!res){
						return NULL;
					}
					table[table_index] = res;
					tmp_num = 0;
				}
				*str = '\0';
				prev_str = 0;
				str += 1;
				if(*str){
					res[tmp_num] = str;
				}else{
					//Note(GI) this deals with case where char c is last character in the string
					res[tmp_num] = NULL;
				}
			}else{
				prev_str = *str;
				str += 1;
			}
			break;
		default:
			prev_str = *str;
			str += 1;
		}
	}

	if(*num < original_limit){
		res[(*num)+1] = NULL;
		return res;
	}

	char **real_res = (char **) malloc(sizeof(char *) * (*num + 2));
	int ti = 0;
	int n = 0;
	int n2 = 0;
	limit = original_limit;
	while(ti <= table_index){

		res = table[ti];
		n2 = 0;
		while(n2 < limit && n <= *num){
			real_res[n] = res[n2];
			n2 += 1;
			n += 1;
		}

		free(res);
		limit *= 2;
		ti += 1;

	}
	real_res[(*num) + 1] = NULL;

	return real_res;

}//tec_string_split_for_args*/

int tec_string_find_char(char *s, char to_find){

	if(!s)
		return -1;

	int i = 0;
	while(s[i] && to_find != s[i]){
		i += 1;
	}

	if(s[i]){
		return i;
	}else{
		return -1;
	}

}//tec_string_find_char*/

int tec_string_find_str(char *string, char *to_find){

	if(!string || !to_find)
		return -1;

	int len = tec_string_length(string);
	int len2 = tec_string_length(to_find);

	if(!len || !len2)
		return - 1;
	if(len2 > len)
		return -1;

	int index = 0;
	int i = 0;
	int j = 0;

	// I don't need to check the full length of string
	// only as far as there is still enough space for it to contain to_find
	int len_loop = len - len2 + 1;
	while(index < len_loop){
		while(index < len_loop && string[index] != to_find[0]){
			index += 1;
		}
		if(string[index] == to_find[0]){
			i = index;
			j = 0;
			while(i < len && j < len2 && string[i] == to_find[j]){
				i += 1;
				j += 1;
			}
			if(j == len2){
				return index;
			}
		}
		index += 1;
	}

	return -1;

}//tec_string_find_str*/

int tec_string_is_plain_ascii(char *text){

	if(!text)
		return 0;

	while(*text && !(*text & TEC_MOST_SIG_BIT)){
		if(*text == 127){
			return 0;
		}else{
			if(*text < 32){
				return 0;
			}
		}
		text += 1;
	}
	if(*text){
		return 0;
	}

	return 1;

}//tec_string_is_plain_ascii*/

void tec_string_shift(char *str){

	if(!str)
		return;

	int len = tec_string_length(str);
	int i = 1;
	int j = 1;

	if( (str[i-j] & TEC_MOST_SIG_BIT) && (str[i-j] & TEC_2ND_MOST_SIG_BIT) ){
		j += 1;
		i += 1;
		while( (str[i] & TEC_MOST_SIG_BIT) && !(str[i] & TEC_2ND_MOST_SIG_BIT) ){
			j += 1;
			i += 1;
		}
	}

	while(i < len){
		str[ i - j ] = str[i];
		i += 1;
	}
	str[i - j] = '\0';

}//tec_string_shift*/

int tec_string_utf8(uint32_t n, char *str){

	if(n > 1114112 || ( n >= 0xd800 && n <= 0xdfff ) ){
		//encodes U+fffd; replacement character
		str[0] = 0xef;
		str[1] = 0xbf;
		str[2] = 0xbd;
		str[3] = 0;
		return 3;
	}

	// just like 7-bit ascii
	if(n < 128){
		str[0] = n;
		str[1] = '\0';
		return 1;
	}

	uint32_t len = 0;
	if(n < 2048){
		len = 2;
	}else{
		if( n < 65636){
			len = 3;
		}else{
			len = 4;
		}
	}
	str[len] = '\0';

	// set the bits at the start to indicate number of bytes
	unsigned char m = TEC_MOST_SIG_BIT;
	uint32_t i = 0;
	str[0] = 0;
	while(i < len){
		str[0] |= m;
		i += 1;
		m >>= 1;
	}

	//set the most significant bits in the other bytes
	i = 1;
	while(i < len){
		str[i] = TEC_MOST_SIG_BIT;
		i += 1;
	}

	//fill in the codepoint
	int j = len - 1;
	while(j >= 0){
		m = 1;
		i = 0;
		while(n && i < 6){
			if(n%2){
				str[j] |= m;
			}
			n >>= 1;
			m <<= 1;
			i += 1;
		}
		j -= 1;
	}

	return len;

}//tec_string_utf8*/

int tec_string_to_int(char *s){

	if(!s)
		return 0;

	int sign = 1;
	int result = 0;

	while(tec_char_is_white_space(*s)){
		s += 1;
	}
	if(*s == '-'){
		sign = -1;
		s += 1;
	}else{
		if(*s == '+'){
			s += 1;
		}
	}

	while(*s){
		if(*s > '9' || *s < '0'){
			return result * sign;
		}
		result *= 10;
		result += *s - '0';
		s += 1;
	}

	return result * sign;

}//tec_string_to_int*/

uint32_t tec_string_hex_to_int(char *str){

	uint32_t result = 0;
	int hit = 1;

	while( tec_char_is_white_space(*str) ){
		str += 1;
	}

	if(str[0] == '0' && str[1] == 'x'){
		str += 2;
	}

	while(*str && hit){
		hit = 0;
		if(*str >= 'a' && *str <= 'f'){
			result *= 16;
			result += 10 + *str - 'a';
			hit = 1;
		}
		if(*str >= 'A' && *str <= 'F'){
			result *= 16;
			result += 10 + *str - 'A';
			hit = 1;
		}
		if(*str >= '0' && *str <= '9'){
			result *= 16;
			result += *str - '0';
			hit = 1;
		}
		str += 1;
	}

	return result;

}//tec_string_hex_to_int*/

int tec_string_from_int(int32_t i, char *result, int32_t buffer_size){

	if(!result)
		return -1;

	int tmp = i;
	int len = 1;
	int j = 0;
	int negative = 0;

	if(i < 0){
		negative = 1;
		len += 1;
		tmp *= -1;
	}

	while(tmp > 9){
		tmp /= 10;
		len += 1;
	}

	if(len + 1 > buffer_size){
		result[0] = '\0';
		return -1;
	}

	result[len] = '\0';
	j = len;
	if(negative){
		result[0] = '-';
		i *= -1;
	}
	while(j - negative){
		j -= 1;
		result[j] = i%10 + '0';
		i /= 10;
	}

	return len + 1;	//+1 accounts for NULL character

}//tec_string_from_int*/

double tec_string_to_double(char *string, int *error){

	if(!string)
		return 0;

	double result = 0;
	char *dec_pos = NULL;	//position of digits past decimal point
	char sign = '+';	//assume plus at first
	int exp = 0;
	int e_present = 0;
	*error = -1;

	while(tec_char_is_white_space(*string)){
		string += 1;
	}

	if(*string == '-'){
		sign = '-';
		string += 1;
	}else{
		if(*string == '+'){
			string += 1;
		}
	}

	while(*string >= '0' && *string <= '9'){
		result *= 10;
		result += (*string - '0');
		string += 1;
		*error = 0;
	}

	if(*string == '.'){
		e_present = 1;
		string += 1;
		dec_pos = string;
		while(*string && (*string >= '0' && *string <= '9')){
			string += 1;
			*error = 0;
		}
	}
	if(*string && (*string == 'e' || *string == 'E')){
		e_present = 1;
		string += 1;
		exp = tec_string_to_int(string);
	}

	while(dec_pos && *dec_pos >= '0' && *dec_pos <= '9'){
		result *= 10;
		result += *dec_pos - '0';
		dec_pos += 1;
		exp -= 1;
	}

	if(sign == '-'){
		result *= -1;
	}

	if(!e_present)
		return result;

	if(exp){
		if(exp > 0){
			while(exp){
				result *= 10;
				exp -= 1;
			}
		}else{
			while(exp){
				result /= 10;
				exp += 1;
			}
		}
	}

	return result;

}//tec_string_to_double*/

int tec_char_is_white_space(char c){

	if(c == 32 || c == 9 || c == 10 || c == 11 || c == 12 || c == 13)
		return 1;
	return 0;

}//tec_char_is_white_space*/

int tec_buf_begins(char *buffer, char *str){

	if(!buffer)
		return 0;
	if(!str)
		return 0;

	while(*str && *buffer == *str){
		str += 1;
		buffer += 1;
	}

	if(*str){
		return 0;
	}

	return 1;

}//tec_buf_begins*/

int tec_buf_find_str(char *buffer, int buffer_size, char *to_find){

	if(!buffer || !to_find)
		return -1;
	if(buffer_size < 1)
		return -1;

	int len = tec_string_length(to_find);

	if(!len)
		return -1;
	if(len > buffer_size)
		return -1;

	int index = 0;
	int i = 0;
	int j = 0;

	// I don't need to check the full length of string
	// only as far as there is still enough space for it to contain to_find
	int len_loop = buffer_size - len + 1;
	while(index < len_loop){
		while(index < len_loop && buffer[index] != to_find[0]){
			index += 1;
		}
		if(buffer[index] == to_find[0]){
			i = index;
			j = 0;
			while(i < buffer_size && j < len && buffer[i] == to_find[j]){
				i += 1;
				j += 1;
			}
			if(j == len){
				return index;
			}
		}
		index += 1;
	}

	return -1;

}//tec_buf_find_str*/

int tec_buf_cmp(char *buf1, char *buf2, int begin, int end){

	if(!buf1 || !buf2)
		return 0;
	if(end < begin)
		return 0;

	if(end - begin < 32){	//32 is a bit arbitrary, must be worth it

		while(begin <= end){
			if(buf1[begin] != buf2[begin]){
				return 0;
			}
			begin += 1;
		}
		return 1;

	}

#if __x86_64__

	int64_t addr = (int64_t) (buf1 + begin);
	int64_t *addr_buf1 = NULL;
	int64_t *addr_buf1_end = (int64_t *) (buf1 + end);
	int64_t *addr_buf2 = NULL;

	// 64 bit computer
	// ensure buffers are on 8-byte boundary before using speed-up trick
	while( addr&0x0000000000000007 && begin <= end){

		if(buf1[begin] != buf2[begin]){
			return 0;
		}
		begin += 1;
		addr += 1;
	}

	addr_buf1 = (int64_t *) (buf1 + begin);
	addr_buf2 = (int64_t *) (buf2 + begin);
	while(addr_buf1 < addr_buf1_end){

		if( *addr_buf1 != *addr_buf2 ){
			return 0;
		}
		addr_buf1 += 1;
		addr_buf2 += 1;

	}

	addr_buf1 -= 1;
	addr_buf2 -= 1;
	char *ch_buf1 = (char *) addr_buf1;
	char *ch_buf2 = (char *) addr_buf2;
	while(ch_buf1 <= &(buf1[end]) ){

		if( *ch_buf1 != *ch_buf2 ){
			return 0;
		}

		ch_buf1 += 1;
		ch_buf2 += 1;

	}

#else

	int32_t addr = (int32_t) (buf1 + begin);
	int32_t *addr_buf1 = NULL;
	int32_t *addr_buf1_end = (int32_t *) (buf1 + end);
	int32_t *addr_buf2 = NULL;

	// 32 bit computer
	// ensure buffers are on 4-byte boundary before using speed-up trick
	while( addr&0x00000003 && begin <= end){

		if(buf1[begin] != buf2[begin]){
			return 0;
		}
		begin += 1;
		addr += 1;
	}

	addr_buf1 = (int32_t *) (buf1 + begin);
	addr_buf2 = (int32_t *) (buf2 + begin);
	while(addr_buf1 < addr_buf1_end){

		if( *addr_buf1 != *addr_buf2 ){
			return 0;
		}
		addr_buf1 += 1;
		addr_buf2 += 1;

	}

	addr_buf1 -= 1;
	addr_buf2 -= 1;
	char *ch_buf1 = (char *) addr_buf1;
	char *ch_buf2 = (char *) addr_buf2;
	while(ch_buf1 <= &(buf1[end]) ){

		if( *ch_buf1 != *ch_buf2 ){
			return 0;
		}

		ch_buf1 += 1;
		ch_buf2 += 1;

	}

#endif

	return 1;

}//tec_buf_cmp*/

int tec_file_exists(char *path){

	if(!path)
		return 0;

#ifdef __linux
	if(access(path, F_OK) != -1){
		return 1;
	}else{
		return 0;
	}
#endif
#ifdef __WIN32
	return PathFileExistsA(path);
#endif

}//tec_file_exists*/

int64_t tec_file_get_size(char *path){

	if(!path)
		return -1;

#ifdef __linux
	struct stat st;
	int error = stat(path, &st);
	if(!error){
		return (int64_t) st.st_size;
	}else{
		return -1;
	}
#endif

#ifdef __WIN32
	BOOL test = FALSE;
	int64_t size;
	HANDLE fp = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	test = GetFileSizeEx(fp, (PLARGE_INTEGER) &size);
	CloseHandle(fp);
	if(test){
		return size;
	}
#endif

	return -1;

}//tec_file_get_size*/

int tec_file_copy(char *source, char *dest, int fail_if_file_exists){

	if(!source)
		return 0;
	if(!dest)
		return 0;

#ifdef __WIN32
	return CopyFile(source, dest, fail_if_file_exists);
#else

	if(fail_if_file_exists && tec_file_exists(dest)){
		return 0;
	}

	struct stat st;
	stat(source, &st);
	FILE *fp_in = fopen(source, "rb");
	if(!fp_in)
		return 0;
	FILE *fp_out = fopen(dest, "wb");
	if(!fp_out)
		return 0;

	char line[1000];
	int i;
	while(!feof(fp_in)){
		i = fread(line, sizeof(char), 1000, fp_in);
		fwrite(line, sizeof(char), i, fp_out);
	}
	fclose(fp_in);
	fclose(fp_out);
	chmod(dest, st.st_mode);

	return 1;

#endif

}//tec_file_copy*/

char* tec_file_get_contents(char *path){

	if(!path)
		return NULL;

	int64_t size = tec_file_get_size(path);
	if(size == -1)
		return NULL;

	size += 1;
	FILE *fp = fopen(path, "rb");
	if(!fp){
		return NULL;
	}
	char *buffer = (char *) malloc(sizeof(char) * size);
	fread(buffer, sizeof(char), size, fp);
	fclose(fp);
	buffer[size-1] = 0;
	return buffer;

}//tec_file_get_contents*/

int tec_dir_make(char *path){

	if(!path)
		return 0;

	/*NOTE(GI):
	we want the path to be 248 (i.e. MAX_PATH - 12)
	to make sure we don't have a silly case
	where final characters are double up
	e.g. "/path/to/dir/////"
	we remove last slashes
	length now should be MAX_PATH - 13
	*/
	int len = tec_string_length(path);
	if(len > MAX_PATH){
		return 0;
	}
	char path_cpy[MAX_PATH_INC];
	tec_string_copy(path_cpy, path, MAX_PATH_INC);
	len -= 2;
	while(len && (path_cpy[len] == '/' || path_cpy[len] == '\\') ){
		path_cpy[len] = 0;
		len -= 1;
	}
	if(tec_string_length(path_cpy) > MAX_PATH - 13)
		return 0;
	if(!tec_string_is_plain_ascii(path_cpy) || tec_string_find_char(path_cpy, ' ') != -1)
		return 0;

#ifdef __WIN32
	return CreateDirectoryA(path_cpy, NULL);
#endif
#ifdef __linux
	return !(mkdir(path_cpy, 0755));
#endif

}//tec_dir_make*/

int tec_dir_parent(char *path){

	if(!path)
		return 0;

	char type = '\0';
	int len = tec_string_length(path);
	int start = 0;
	int i = 0;

	if(len == 1){
		// we have either "/" or an invalid path
		return 0;
	}

	if(path[0] == '/'){
		// linux absolute path
		type = '/';
		start = 1;
	}else{
		if(path[1] == ':' && path[2] == '\\'){
			// windows absolute path
			type = '\\';
			start = 3;
		}else{
			// relative paths
			while(i < len && path[i] != '/' && path[i] != '\\'){
				i += 1;
			}
			if(path[i] == '\\'){
				type = '\\';
			}else{
				type = '/';
			}
			start = i+1;
		}
	}

	if(len == 2){
		if(path[0] == '/'){
			path[1] = '\0';
			return 1;
		}else{
			// if we get here then the path could be "a:" or "./" or something invalid
			// either way we just return zero since we cannot go to the upper directory
			// GI
			return 0;
		}
	}

	i = len - 1;
	while(i && path[i] == type){
		// just in case we have a path with multiple '/'
		// e.g. /some//directory//path
		i -= 1;
	}
	while(i && path[i] != type){
		i -= 1;
	}
	while(i && path[i-1] == type){
		i -= 1;
	}
	if(path[i] == type){
		if(i == start - 1){
			i = start;
		}else{
			if(i < start){
				return 0;
			}
		}
		path[i] = '\0';
		return 1;
	}
	return 0;

}//tec_dir_parent*/

int tec_dir_sub(char *path, char *sub_dir, int path_size){

	if(!path)
		return 0;
	if(!sub_dir)
		return 0;

	int len = tec_string_length(path);
	if(!len){
		return 0;
	}
	int len_sub = tec_string_length(sub_dir);
	if(!len_sub){
		return 0;
	}

	char type[2] = "";
	if( !(path[len-1] == '\\' || path[len-1] == '/') ){
		int i = 0;
		while(i < len && path[i] != '/' && path[i] != '\\'){
			i += 1;
		}
		if(path[i] == '/' || path[i] == '\\'){
			type[0] = path[i];
		}else{
#ifdef __WIN32
				type[0] = '\\';
#else
				type[0] = '/';
#endif
		}
	}

	if(type[0]){
		tec_string_cat(path, type, path_size);
	}
	tec_string_cat(path, sub_dir, path_size);

	return 1;

}//tec_dir_sub*/

int tec_path_get_filename(char *path){

	if(!path)
		return 0;

	int len = tec_string_length(path);
	if(!len)
		return 0;

	int i = len - 1;
	while(i >= 0 && path[i] != '/' && path[i] != '\\'){
		i -= 1;
	}

	if(i == len - 1){
		// path without a filename
		path[0] = 0;
		return 0;
	}
	if(i < 0){
		// path is already just a filename
		return 1;
	}

	i += 1;
	int j = 0;
	len -= i;
	while(j < len){
		path[j] = path[j+i];
		j += 1;
	}
	path[j] = 0;
	return 1;

}//tec_path_get_filename*/

int tec_path_is_directory(char *path){

	if(!path || !*path)
		return 0;

#ifdef __linux

	struct stat st;
	stat(path, &st);
	if(st.st_mode & S_IFDIR){
		return 1;
	}
#endif

#ifdef __WIN32

	DWORD attr = GetFileAttributesA((LPCSTR) path);

	if(attr == INVALID_FILE_ATTRIBUTES){
		return 0;
	}
	if(attr & FILE_ATTRIBUTE_DIRECTORY){
		return 1;
	}

#endif

	return 0;

}//tec_path_is_directory*/

int tec_sys_launch_exe(char *command){

	if(!command)
		return 0;

	// Note(GI): we do the length check for all OS
	// we want to ensure consistency accross platforms
	int len = tec_string_length(command);
	if(len > 1023)
		return 0;

#ifdef __WIN32
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	return CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
#endif

#ifdef __linux

	pid_t pid = fork();
	if(pid < 0){
		return 0;
	}
	if(pid == 0){
		int num = 0;
		char cmd[1024];
		tec_string_copy(cmd, command, 1024);
		char *tmp_cmd = cmd;
		while(tmp_cmd[1]){
			if(*tmp_cmd == ' ' && tmp_cmd[1] == ' '){
				// we need to strip doubled up spaces as they can cause problems
				tec_string_shift(tmp_cmd);
			}else{
				tmp_cmd += 1;
			}
		}
		char **spl = tec_string_split_for_args(cmd, &num);
		execv(spl[0], spl);
		return 0;
	}

#endif

	return 0;		// just to keep compiler happy ;-)

}//tec_sys_launch_exe*/

tec_process* tec_sys_launch_process(char *cmd, void (*func_out)(), void *data_out, void (*func_error)(), void *data_error){

	if(!cmd)
		return NULL;

	int len = tec_string_length(cmd);
	if(len > 1023)
		return 0;

	tec_process *tp = (tec_process *) malloc(sizeof(tec_process));
	tp->buf_stdout[0] = 0;
	tp->buf_stderr[0] = 0;
	tp->tp_output_cb = func_out;
	tp->dp = data_out;
	tp->tp_output_error_cb = func_error;
	tp->dp_error = data_error;

#ifdef __linux

	char path[1024];
	tec_string_copy(path, cmd, 1024);
	int num = 0;
	char **spl = tec_string_split_for_args(path, &num);

	if(pipe(tp->pipes[PARENT_READ_PIPE]) == -1){
		tec_error("tec_sys_launch_process error: failed to create pipe.");
	}
	if(pipe(tp->pipes[PARENT_WRITE_PIPE]) == -1){
		tec_error("tec_sys_launch_process error: failed to create pipe.");
	}
	if(pipe(tp->pipes[PARENT_READ_ERROR_PIPE]) == -1){
		tec_error("tec_sys_launch_process error: failed to create pipe.");
	}

	pid_t pid = fork();

	if(pid){

		//parent side
		tp->child = pid;
		// close fds not required by parent
		close(tp->pipes[PARENT_WRITE_PIPE][READ_FD]);
		close(tp->pipes[PARENT_READ_PIPE][WRITE_FD]);
		close(tp->pipes[PARENT_READ_ERROR_PIPE][WRITE_FD]);

	}else{

		//child side
		dup2(tp->pipes[PARENT_WRITE_PIPE][READ_FD], STDIN_FILENO);
		dup2(tp->pipes[PARENT_READ_PIPE][WRITE_FD], STDOUT_FILENO);
		dup2(tp->pipes[PARENT_READ_ERROR_PIPE][WRITE_FD], STDERR_FILENO);

		close(tp->pipes[PARENT_WRITE_PIPE][READ_FD]);
		close(tp->pipes[PARENT_READ_PIPE][WRITE_FD]);
		close(tp->pipes[PARENT_READ_PIPE][READ_FD]);
		close(tp->pipes[PARENT_WRITE_PIPE][WRITE_FD]);
		close(tp->pipes[PARENT_READ_ERROR_PIPE][WRITE_FD]);
		close(tp->pipes[PARENT_READ_ERROR_PIPE][READ_FD]);

		if(execv(spl[0], spl) == -1){
			tec_error("tec_sys_launch_process: some error occured (execv)");
		}

	}

	pthread_t thr_output;
	pthread_t thr_error;

	pthread_create(&thr_output, NULL, &t_sys_launch_process_output, (void *) tp);
	pthread_create(&thr_error, NULL, &t_sys_launch_process_error, (void *) tp);

#endif

#ifdef __WIN32

	tp->child_std_in_read = NULL;
	tp->child_std_in_write = NULL;
	tp->child_std_out_read = NULL;
	tp->child_std_out_write = NULL;
	tp->child_std_err_read = NULL;
	tp->child_std_err_write = NULL;

	SECURITY_ATTRIBUTES attr; 
	// Set the bInheritHandle flag so pipe handles are inherited. 
	attr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	attr.bInheritHandle = TRUE; 
	attr.lpSecurityDescriptor = NULL; 

	// Create a pipe for the child process's STDOUT.
	if( !CreatePipe(&(tp->child_std_out_read), &(tp->child_std_out_write), &attr, 0) ){
		tec_error("tec_sys_launch_process: Failed to create StdoutRd pipe");
	}

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if( !SetHandleInformation(tp->child_std_out_read, HANDLE_FLAG_INHERIT, 0) ){
		tec_error("tec_sys_launch_process: Stdout SetHandleInformation failure");
	}

	// Create a pipe for the child process's STDIN. 
	if( !CreatePipe(&(tp->child_std_in_read), &(tp->child_std_in_write), &attr, 0)){
		tec_error("tec_sys_launch_process: Stdin pipe creation failure");
	}

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	if( !SetHandleInformation(tp->child_std_in_write, HANDLE_FLAG_INHERIT, 0) ){
		tec_error("tec_sys_launch_process: Stdin SetHandleInformation failure");
	}

	// Create a pipe for the child process's STDERR. 
	if( !CreatePipe(&(tp->child_std_err_read), &(tp->child_std_err_write), &attr, 0)){
		tec_error("tec_sys_launch_process: Stdin pipe creation failure");
	}

	// Ensure the write handle to the pipe for STDERR is not inherited. 
	if( !SetHandleInformation(tp->child_std_err_read, HANDLE_FLAG_INHERIT, 0) ){
		tec_error("tec_sys_launch_process: Stdin SetHandleInformation failure");
	}

	PROCESS_INFORMATION proc_info;
	STARTUPINFO start_info;
	int success = 0;

	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory( &proc_info, sizeof(PROCESS_INFORMATION) );

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory( &start_info, sizeof(STARTUPINFO) );
	start_info.cb = sizeof(STARTUPINFO);
	start_info.hStdError = tp->child_std_err_write;
	start_info.hStdOutput = tp->child_std_out_write;
	start_info.hStdInput = tp->child_std_in_read;
	start_info.dwFlags |= STARTF_USESTDHANDLES;


	// Create the child process. 
	success = CreateProcess(NULL,
		cmd,           // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&start_info,   // STARTUPINFO pointer 
		&proc_info);   // receives PROCESS_INFORMATION 

	// If an error occurs, exit the application. 
	if( !success ){
		tec_error("failure tec_sys_launch_process");
		free(tp);
		return NULL;
	}else{
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 
		tp->process_info = proc_info.hProcess;
		CloseHandle(proc_info.hThread);//
	}

	DWORD thr_output;
	DWORD thr_error;

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) &t_sys_launch_process_output, (void *) tp, 0, &thr_output);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) &t_sys_launch_process_error, (void *) tp, 0, &thr_error);

#endif

	return tp;

}//tec_sys_launch_process*/

void tec_sys_process_input(tec_process *tp, char *text){

	if(!tp)
		return;
	if(!text)
		return;

#ifdef __linux
	write(tp->pipes[PARENT_WRITE_PIPE][WRITE_FD], text, tec_string_length(text));
#endif

#ifdef __WIN32
	DWORD bytes_written;
	WriteFile(tp->child_std_in_write, text, tec_string_length(text), &bytes_written, NULL);
#endif

}//tec_sys_process_input*/

void* t_sys_launch_process_output(void *data){

	if(!data)
		return 0;

	tec_process *tp = (tec_process *) data;

	void (*tp_output_cb)(tec_process *tpr, void *dp);
	tp_output_cb = (void *) tp->tp_output_cb;

	if(!tp_output_cb){
		return 0;
	}

#ifdef __linux

	int count = 0;
	count = read(tp->pipes[PARENT_READ_PIPE][READ_FD], tp->buf_stdout, TEC_STDOUT_BUFFER_SIZE-1);

	while(count > 0) {
		tp->buf_stdout[count] = 0;
		tp_output_cb(tp, tp->dp);
		count = read(tp->pipes[PARENT_READ_PIPE][READ_FD], tp->buf_stdout, TEC_STDOUT_BUFFER_SIZE-1);
	}

#endif

#ifdef __WIN32

	DWORD count = 0;
	int success = ReadFile(tp->child_std_out_read, tp->buf_stdout, TEC_STDOUT_BUFFER_SIZE-1, &count, NULL);

	while(success){
		tp->buf_stdout[count] = 0;	// do I need this? better be safe, GI
		tp_output_cb(tp, tp->dp);
		success = ReadFile(tp->child_std_out_read, tp->buf_stdout, TEC_STDOUT_BUFFER_SIZE-1, &count, NULL);
	}

#endif

	return 0;

}//t_sys_launch_process_output*/

void* t_sys_launch_process_error(void *data){

	if(!data)
		return 0;

	tec_process *tp = (tec_process *) data;

	void (*tp_output_error_cb)(tec_process *tpr, void *dp);
	tp_output_error_cb = (void *) tp->tp_output_error_cb;

	if(!tp_output_error_cb)
		return 0;

#ifdef __linux

	int count = 0;
	count = read(tp->pipes[PARENT_READ_ERROR_PIPE][READ_FD], tp->buf_stderr, TEC_STDOUT_BUFFER_SIZE-1);

	while(count > 0) {
		tp->buf_stderr[count] = 0;
		(*tp_output_error_cb)(tp, tp->dp_error);
		count = read(tp->pipes[PARENT_READ_ERROR_PIPE][READ_FD], tp->buf_stderr, TEC_STDOUT_BUFFER_SIZE-1);
	}

#endif

#ifdef __WIN32

	DWORD count = 0;
	int success = ReadFile(tp->child_std_err_read, tp->buf_stderr, TEC_STDOUT_BUFFER_SIZE-1, &count, NULL);

	while(success){
		tp->buf_stderr[count] = 0;
		tp_output_error_cb(tp, tp->dp_error);
		success = ReadFile(tp->child_std_err_read, tp->buf_stderr, TEC_STDOUT_BUFFER_SIZE-1, &count, NULL);
	}

#endif

	return 0;

}//t_sys_launch_process_error*/

void t_error(char *file, int line, char *error_msg, ...){

	va_list vl;

	char *tmp = NULL;
	int len = 0;
	char percent[2] = "%";
	char *string = error_msg;

#ifdef __linux

	int arr_size = 8;
	int i = 1; //not zero, because prefix

	////prefix
	char prefix[100] = "\x1B[1;31mtec_error\x1B[0m";
	if(file){
		tec_string_cat(prefix, " (\x1B[1;33m", 100);
		tec_string_cat(prefix, file, 100);
		if(line){
			tec_string_cat(prefix, ":", 100);
			len = tec_string_length(prefix);
			tmp = prefix + len;
			tec_string_from_int(line, tmp, 100 - len);
		}
		tec_string_cat(prefix, "\x1B[0m)", 100);
	}
	tec_string_cat(prefix, ": ", 100);

	char str_null[24] = "\x1B[0;36m<NULL>\x1B[0m";
	struct iovec iov[arr_size];	//size is multiple of cache-line on 64bit
	iov[0].iov_base = prefix;
	iov[0].iov_len = tec_string_length(prefix);
	char buf_integer[arr_size][64];

	if(!string || !*string){
		write(2, str_null, 17 );
		write(2, "\n", 1);
		return;
	}

#endif

#ifdef __WIN32

	DWORD bytes_written = 0;

	char fd_prefix[100] = "tec_error (";
	tec_string_cat(fd_prefix, file, 100);
	tec_string_cat(fd_prefix, ":", 100);
	int fd_len = tec_string_length(fd_prefix);
	tmp = fd_prefix + fd_len;
	tec_string_from_int(line, tmp, 100 - fd_len);
	tec_string_cat(fd_prefix, "): ", 100);
	char fd_str_null[7] = "<NULL>";

	HANDLE h_out = GetStdHandle(STD_ERROR_HANDLE);	
	WriteConsole(h_out, fd_prefix, tec_string_length(fd_prefix), &bytes_written, 0);

	if(!string || !*string){
		WriteConsole(h_out, fd_str_null, 6, &bytes_written, 0);
		WriteConsole(h_out, "\n", 1, &bytes_written, 0);
		return;
	}

#endif

	char fd_buf_integer[32];
	va_start(vl, error_msg);
	char *str = NULL;
	int32_t integer32 = 0;
	char c;
	char *fd_str = NULL;
	int err = 0;

	while(*string){
		if(*string == '%'){
			string += 1;
////////////////////////////////////////////////////////
// The switch for Linux
////////////////////////////////////////////////////////
#ifdef __linux
			switch(*string){
			case 's':
				string += 1;
				str = va_arg(vl, char *);

				if(!str || !*str){
					str = str_null;
				}else{
					iov[i].iov_base = TEC_PRINT_YELLOW_DARK;
					iov[i].iov_len = tec_string_length(TEC_PRINT_YELLOW_DARK);
					i += 1;
					iov[i].iov_base = str;
					iov[i].iov_len = tec_string_length(str);
					i += 1;
					str = TEC_PRINT_NORMAL;
				}
				len = tec_string_length(str);
				break;
			case 'd':
				string += 1;
				integer32 = va_arg(vl, int32_t);
				tec_string_from_int(integer32, fd_buf_integer, 32);
				if(integer32 < 0){
					tec_string_copy(buf_integer[i], "\x1B[38;2;255;164;0m", 64);
				}else{
					tec_string_copy(buf_integer[i], "\x1B[38;2;255;200;100m", 64);
				}
				tec_string_cat(buf_integer[i], fd_buf_integer, 64);
				tec_string_cat(buf_integer[i], "\x1B[m", 64);
				str = buf_integer[i];
				len = tec_string_length(str);
				break;
			case 'c':
				string += 1;
				//va_arg won't accept 'char', so we use int32_t
				c = (char) va_arg(vl, int32_t);
				tec_string_copy(buf_integer[i], "\x1B[38;2;255;164;0m", 64);
				buf_integer[i][17] = c;
				buf_integer[i][18] = 0;
				tec_string_cat(buf_integer[i], "\x1B[m", 64);
				str = buf_integer[i];
				len = 21;
				break;
			case '%':
				string += 1;
				str = percent;
				len = 1;
				break;
			default:
				//TODO(GI):
				//handle this case
				break;
			}
#endif

////////////////////////////////////////////////////////
// The switch for Windows
////////////////////////////////////////////////////////
#ifdef __WIN32

			switch(*string){
			case 's':
				string += 1;
				str = va_arg(vl, char *);
				fd_str = str;

				if(!str || !*str){
					fd_str = fd_str_null;
				}

				fd_len = tec_string_length(fd_str);
				break;
			case 'd':
				string += 1;
				integer32 = va_arg(vl, int32_t);
				tec_string_from_int(integer32, fd_buf_integer, 32);
				fd_str = fd_buf_integer;
				fd_len = tec_string_length(fd_str);
				break;
			case 'c':
				string += 1;
				//va_arg won't accept 'char', so we use int32_t
				c = (char) va_arg(vl, int32_t);
				fd_buf_integer[0] = c;
				fd_buf_integer[1] = 0;
				fd_str = fd_buf_integer;
				fd_len = 1;
				break;
			case '%':
				string += 1;
				fd_str = percent;
				fd_len = 1;
				break;
			default:
				//TODO(GI):
				//handle this case
				break;
			}
#endif

		}else{

			str = string;
			len = 0;

			while(*string && *string != '%'){
				string += 1;
				len += 1;
			}
#ifdef __WIN32
			fd_str = str;
			fd_len = len;
#endif
		}

#ifdef __WIN32
		WriteConsole(h_out, fd_str, fd_len, &bytes_written, 0);
#endif

#ifdef __linux
		iov[i].iov_base = str;
		iov[i].iov_len = len;
		i += 1;
		if(i == arr_size){
			err = writev(2, iov, i);
			if(err == -1){
				return;
			}
			i = 0;
		}
#endif

	}

#ifdef __linux

	err = writev(2, iov, i);
	if(err == -1)
		return;
	write(2, "\n", 1);

#endif

#ifdef __WIN32
	WriteConsole(h_out, "\n", 1, &bytes_written, 0);
#endif

	va_end(vl);

}//t_error*/
