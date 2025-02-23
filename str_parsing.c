
/**
 * str_parsing.c
 * 
 * Contains miscellaneous string parsing functions.
 */



#include <windows.h>
#include <iso646.h>



/**
 * first_nonescaped_dquote
 * 
 * Searches a string for a non-escaped (\") double quote L'"'.
 * 
 * str: NULL-terminated string to be searched
 * 
 * Return Value: Returns a pointer to the next real L'"' character.
 *               Returns NULL if there are no L'"' characters.
 */
WCHAR *first_nonescaped_dquote(const WCHAR *str) {

    // edge case: first character is dquote
    // If we don't treat this as edge case, potential invalid mem access
    if (*str == L'"') {
        return (WCHAR *)str;
    }

    const WCHAR *quote_p,
                *str_p = str + 1;

    do {
        quote_p = wcschr(str_p, L'"');
        str_p = quote_p + 1;
    } while (quote_p && *(quote_p - 1) == L'\\');
    
    return (WCHAR *)quote_p;
}



/**
 * nonquoted_wcschr
 * 
 * Finds the first non-quoted instance of a given character in a string.
 * All instances of c within "double quotes" will be ignored.
 * 
 * str: NULL-terminated to be searched.
 * c: Character to search for.
 * 
 * Return Value: Returns a pointer to the first non-quoted instance of c in 
 *               str. Returns NULL if non-quoted c isn't present in str.
 */
WCHAR *nonquoted_wcschr(const WCHAR *str, WCHAR c) {
    
    const WCHAR *str_p = str, 
          *quote_block_start_p,
          *quote_block_end_p, 
          *c_p;
    BOOL valid;
    
    do {
        valid = TRUE;
        c_p = wcschr(str_p, c);
        quote_block_start_p = wcschr(str_p, L'"');
        while (c_p && quote_block_start_p 
                && quote_block_start_p < c_p) {
            quote_block_end_p = 
                first_nonescaped_dquote(quote_block_start_p + 1);
            if (quote_block_end_p > c_p) {
                valid = FALSE;
                break;
            }
            else {
                quote_block_start_p = wcschr(quote_block_end_p + 1, L'"');
            }
        }
        if (!valid) {
            str_p = quote_block_end_p + 1;
        }
    } while(!valid);

    return (WCHAR *)c_p;
}



/**
 * skip_whitespace
 * 
 * Searches a string for the first non-whitespace character.
 * 
 * str: NULL-terminated string to be searched.
 * 
 * Return Value: Returns a pointer to the first non-whitespace character
 *               in str. If str is all whitespace, returns a pointer to the
 *               terminating L'\0'.
 */
WCHAR *skip_whitespace(const WCHAR *str) {

    const WCHAR *str_p = str;
    for (str_p = str; 
         *str_p != L'\0' && iswspace(*str_p); 
         str_p++) { }
    return (WCHAR *)str_p;
}



/**
 * arg_end
 * 
 * Finds the end of the given argument by searching for the first non-quoted
 * whitespace.
 * 
 * cmdline: NULL-string that contains a series of whitespace separated 
 *          arguments.
 * 
 * Return Value: Returns a pointer to the end of the first argument
 */
WCHAR *arg_end(const WCHAR *cmdline) {

    while (*cmdline != L'\0' && !iswspace(*cmdline)) {
        if (*cmdline == L'"') {
            cmdline = first_nonescaped_dquote(cmdline + 1);
            if (cmdline == NULL)
                return NULL;
        }
        cmdline++;
    }

    return (WCHAR *)cmdline;
}



/**
 * readjust_quotes
 * 
 * Scans str for double quotes.
 * If double quotes are present, they are removed and the whole string is 
 * enclosed in double quotes.
 * If not, the string is left as is.
 * 
 * str: NULL-terminated string to be scanned and adjusted.
 * 
 * Return Value: Returns TRUE on success, FALSE on failure.
 *               Will only fail if string ends with unclosed quotes.
 */
BOOL readjust_quotes(WCHAR *str) {
    
    WCHAR *first_quote = wcschr(str, L'"');
    if (first_quote == NULL) {
        return TRUE;
    }
    WCHAR *block_start = first_quote + 1,
          *block_end;
    DWORD chars_removed = 1;
    BOOL curr_quoted = TRUE;

    while (TRUE) {

        BOOL done = FALSE;

        if (curr_quoted) {
            block_end = first_nonescaped_dquote(block_start);
            if (block_end == NULL) 
                return FALSE;
        }
        else {
            block_end = wcschr(block_start, L'"');
            if (block_end == NULL) {
                block_end = block_start + wcslen(block_start);
                done = TRUE;
            }
        }

        memmove(
            block_start - chars_removed, 
            block_start,
            (block_end - block_start) * sizeof(WCHAR)
        );

        if (done)
            break;
        
        block_start = block_end + 1;
        curr_quoted = not curr_quoted;
        chars_removed++;
    }

    DWORD str_len = (DWORD)(block_end - chars_removed - str);
    memmove(str + 1, str, str_len * sizeof(WCHAR));
    str[0] = L'"';
    str[str_len + 1] = L'"';
    str[str_len + 2] = L'\0';

    return TRUE;
}
