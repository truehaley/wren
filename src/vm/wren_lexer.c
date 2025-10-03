#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "wren_common.h"
#include "wren_lexer.h"
#include "wren_vm.h"

// This is written in bottom-up order to minimize the number of explicit forward
// declarations needed.

// The buffer size used to format a compile error message, excluding the header
// with the module name and error location. Using a hardcoded buffer for this
// is kind of hairy, but fortunately we can control what the longest possible
// message is and handle that. Ideally, we'd use `snprintf()`, but that's not
// available in standard C++98.
#define ERROR_MESSAGE_SIZE (80 + MAX_VARIABLE_NAME + 15)

void wrenPrintError(Parser* parser, int line, const char* label,
                       const char* format, va_list args)
{
  parser->hasError = true;
  if (!parser->printErrors) return;

  // Only report errors if there is a WrenErrorFn to handle them.
  if (parser->vm->config.errorFn == NULL) return;

  // Format the label and message.
  char message[ERROR_MESSAGE_SIZE];
  int length = sprintf(message, "%s: ", label);
  length += vsprintf(message + length, format, args);
  ASSERT(length < ERROR_MESSAGE_SIZE, "Error should not exceed buffer.");

  ObjString* module = parser->module->name;
  const char* module_name = module ? module->value : "<unknown>";

  parser->vm->config.errorFn(parser->vm, WREN_ERROR_COMPILE,
                             module_name, line, message);
}

// Outputs a lexical error.
static void lexError(Parser* parser, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  wrenPrintError(parser, parser->currentLine, "Error", format, args);
  va_end(args);
}

// Lexing ----------------------------------------------------------------------

typedef struct
{
  const char* identifier;
  size_t      length;
  TokenType   tokenType;
} Keyword;

// The table of reserved words and their associated token types.
static Keyword keywords[] =
{
  {"break",     5, TOKEN_BREAK},
  {"continue",  8, TOKEN_CONTINUE},
  {"class",     5, TOKEN_CLASS},
  {"construct", 9, TOKEN_CONSTRUCT},
  {"else",      4, TOKEN_ELSE},
  {"false",     5, TOKEN_FALSE},
  {"for",       3, TOKEN_FOR},
  {"foreign",   7, TOKEN_FOREIGN},
  {"if",        2, TOKEN_IF},
  {"import",    6, TOKEN_IMPORT},
  {"as",        2, TOKEN_AS},
  {"in",        2, TOKEN_IN},
  {"is",        2, TOKEN_IS},
  {"null",      4, TOKEN_NULL},
  {"return",    6, TOKEN_RETURN},
  {"static",    6, TOKEN_STATIC},
  {"super",     5, TOKEN_SUPER},
  {"this",      4, TOKEN_THIS},
  {"true",      4, TOKEN_TRUE},
  {"var",       3, TOKEN_VAR},
  {"while",     5, TOKEN_WHILE},
  {NULL,        0, TOKEN_EOF} // Sentinel to mark the end of the array.
};

// Returns true if [c] is a valid (non-initial) identifier character.
static bool isName(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// Returns true if [c] is a digit.
static bool isDigit(char c)
{
  return c >= '0' && c <= '9';
}

// Returns the current character the parser is sitting on.
static char peekChar(Parser* parser)
{
  return *parser->currentChar;
}

// Returns the character after the current character.
static char peekNextChar(Parser* parser)
{
  // If we're at the end of the source, don't read past it.
  if (peekChar(parser) == '\0') return '\0';
  return *(parser->currentChar + 1);
}

// Advances the parser forward one character.
static char nextChar(Parser* parser)
{
  char c = peekChar(parser);
  parser->currentChar++;
  if (c == '\n') parser->currentLine++;
  return c;
}

// If the current character is [c], consumes it and returns `true`.
static bool matchChar(Parser* parser, char c)
{
  if (peekChar(parser) != c) return false;
  nextChar(parser);
  return true;
}

// Sets the parser's current token to the given [type] and current character
// range.
static void makeToken(Parser* parser, TokenType type)
{
  parser->next.type = type;
  parser->next.start = parser->tokenStart;
  parser->next.length = (int)(parser->currentChar - parser->tokenStart);
  parser->next.line = parser->currentLine;

  // Make line tokens appear on the line containing the "\n".
  if (type == TOKEN_LINE) parser->next.line--;
}

// If the current character is [c], then consumes it and makes a token of type
// [two]. Otherwise makes a token of type [one].
static void twoCharToken(Parser* parser, char c, TokenType two, TokenType one)
{
  makeToken(parser, matchChar(parser, c) ? two : one);
}

// Skips the rest of the current line.
static void skipLineComment(Parser* parser)
{
  while (peekChar(parser) != '\n' && peekChar(parser) != '\0')
  {
    nextChar(parser);
  }
}

// Skips the rest of a block comment.
static void skipBlockComment(Parser* parser)
{
  int nesting = 1;
  while (nesting > 0)
  {
    if (peekChar(parser) == '\0')
    {
      lexError(parser, "Unterminated block comment.");
      return;
    }

    if (peekChar(parser) == '/' && peekNextChar(parser) == '*')
    {
      nextChar(parser);
      nextChar(parser);
      nesting++;
      continue;
    }

    if (peekChar(parser) == '*' && peekNextChar(parser) == '/')
    {
      nextChar(parser);
      nextChar(parser);
      nesting--;
      continue;
    }

    // Regular comment character.
    nextChar(parser);
  }
}

// Reads the next character, which should be a hex digit (0-9, a-f, or A-F) and
// returns its numeric value. If the character isn't a hex digit, returns -1.
static int readHexDigit(Parser* parser)
{
  char c = nextChar(parser);
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;

  // Don't consume it if it isn't expected. Keeps us from reading past the end
  // of an unterminated string.
  parser->currentChar--;
  return -1;
}

// Parses the numeric value of the current token.
static void makeNumber(Parser* parser, bool isHex)
{
  errno = 0;

  if (isHex)
  {
    parser->next.value = NUM_VAL((WrenNum)strtoll(parser->tokenStart, NULL, 16));
  }
  else
  {
    parser->next.value = NUM_VAL(strtod(parser->tokenStart, NULL));
  }

  if (errno == ERANGE)
  {
    lexError(parser, "Number literal was too large (%d).", sizeof(long int));
    parser->next.value = NUM_VAL(0);
  }

  // We don't check that the entire token is consumed after calling strtoll()
  // or strtod() because we've already scanned it ourselves and know it's valid.

  makeToken(parser, TOKEN_NUMBER);
}

// Finishes lexing a hexadecimal number literal.
static void readHexNumber(Parser* parser)
{
  // Skip past the `x` used to denote a hexadecimal literal.
  nextChar(parser);

  // Iterate over all the valid hexadecimal digits found.
  while (readHexDigit(parser) != -1) continue;

  makeNumber(parser, true);
}

// Finishes lexing a number literal.
static void readNumber(Parser* parser)
{
  while (isDigit(peekChar(parser))) nextChar(parser);

  // See if it has a floating point. Make sure there is a digit after the "."
  // so we don't get confused by method calls on number literals.
  if (peekChar(parser) == '.' && isDigit(peekNextChar(parser)))
  {
    nextChar(parser);
    while (isDigit(peekChar(parser))) nextChar(parser);
  }

  // See if the number is in scientific notation.
  if (matchChar(parser, 'e') || matchChar(parser, 'E'))
  {
    // Allow a single positive/negative exponent symbol.
    if(!matchChar(parser, '+'))
    {
      matchChar(parser, '-');
    }

    if (!isDigit(peekChar(parser)))
    {
      lexError(parser, "Unterminated scientific notation.");
    }

    while (isDigit(peekChar(parser))) nextChar(parser);
  }

  makeNumber(parser, false);
}

// Finishes lexing an identifier. Handles reserved words.
static void readName(Parser* parser, TokenType type, char firstChar)
{
  ByteBuffer string;
  wrenByteBufferInit(&string);
  wrenByteBufferWrite(parser->vm, &string, firstChar);

  while (isName(peekChar(parser)) || isDigit(peekChar(parser)))
  {
    char c = nextChar(parser);
    wrenByteBufferWrite(parser->vm, &string, c);
  }

  // Update the type if it's a keyword.
  size_t length = parser->currentChar - parser->tokenStart;
  for (int i = 0; keywords[i].identifier != NULL; i++)
  {
    if (length == keywords[i].length &&
        memcmp(parser->tokenStart, keywords[i].identifier, length) == 0)
    {
      type = keywords[i].tokenType;
      break;
    }
  }

  parser->next.value = wrenNewStringLength(parser->vm,
                                            (char*)string.data, string.count);

  wrenByteBufferClear(parser->vm, &string);
  makeToken(parser, type);
}

// Reads [digits] hex digits in a string literal and returns their number value.
static int readHexEscape(Parser* parser, int digits, const char* description)
{
  int value = 0;
  for (int i = 0; i < digits; i++)
  {
    if (peekChar(parser) == '"' || peekChar(parser) == '\0')
    {
      lexError(parser, "Incomplete %s escape sequence.", description);

      // Don't consume it if it isn't expected. Keeps us from reading past the
      // end of an unterminated string.
      parser->currentChar--;
      break;
    }

    int digit = readHexDigit(parser);
    if (digit == -1)
    {
      lexError(parser, "Invalid %s escape sequence.", description);
      break;
    }

    value = (value * 16) | digit;
  }

  return value;
}

// Reads a hex digit Unicode escape sequence in a string literal.
static void readUnicodeEscape(Parser* parser, ByteBuffer* string, int length)
{
  int value = readHexEscape(parser, length, "Unicode");

  // Grow the buffer enough for the encoded result.
  int numBytes = wrenUtf8EncodeNumBytes(value);
  if (numBytes != 0)
  {
    wrenByteBufferFill(parser->vm, string, 0, numBytes);
    wrenUtf8Encode(value, string->data + string->count - numBytes);
  }
}

static void readRawString(Parser* parser)
{
  ByteBuffer string;
  wrenByteBufferInit(&string);
  TokenType type = TOKEN_STRING;

  //consume the second and third "
  nextChar(parser);
  nextChar(parser);

  int skipStart = 0;
  int firstNewline = -1;

  int skipEnd = -1;
  int lastNewline = -1;

  for (;;)
  {
    char c = nextChar(parser);
    char c1 = peekChar(parser);
    char c2 = peekNextChar(parser);

    if (c == '\r') continue;

    if (c == '\n') {
      lastNewline = string.count;
      skipEnd = lastNewline;
      firstNewline = firstNewline == -1 ? string.count : firstNewline;
    }

    if (c == '"' && c1 == '"' && c2 == '"') break;

    bool isWhitespace = c == ' ' || c == '\t';
    skipEnd = c == '\n' || isWhitespace ? skipEnd : -1;

    // If we haven't seen a newline or other character yet,
    // and still seeing whitespace, count the characters
    // as skippable till we know otherwise
    bool skippable = skipStart != -1 && isWhitespace && firstNewline == -1;
    skipStart = skippable ? string.count + 1 : skipStart;

    // We've counted leading whitespace till we hit something else,
    // but it's not a newline, so we reset skipStart since we need these characters
    if (firstNewline == -1 && !isWhitespace && c != '\n') skipStart = -1;

    if (c == '\0' || c1 == '\0' || c2 == '\0')
    {
      lexError(parser, "Unterminated raw string.");

      // Don't consume it if it isn't expected. Keeps us from reading past the
      // end of an unterminated string.
      parser->currentChar--;
      break;
    }

    wrenByteBufferWrite(parser->vm, &string, c);
  }

  //consume the second and third "
  nextChar(parser);
  nextChar(parser);

  int offset = 0;
  int count = string.count;

  if(firstNewline != -1 && skipStart == firstNewline) offset = firstNewline + 1;
  if(lastNewline != -1 && skipEnd == lastNewline) count = lastNewline;

  count -= (offset > count) ? count : offset;

  parser->next.value = wrenNewStringLength(parser->vm,
                         ((char*)string.data) + offset, count);

  wrenByteBufferClear(parser->vm, &string);
  makeToken(parser, type);
}

// Finishes lexing a string literal.
static void readString(Parser* parser)
{
  ByteBuffer string;
  TokenType type = TOKEN_STRING;
  wrenByteBufferInit(&string);

  for (;;)
  {
    char c = nextChar(parser);
    if (c == '"') break;
    if (c == '\r') continue;

    if (c == '\0')
    {
      lexError(parser, "Unterminated string.");

      // Don't consume it if it isn't expected. Keeps us from reading past the
      // end of an unterminated string.
      parser->currentChar--;
      break;
    }

    if (c == '%')
    {
      if (parser->numParens < MAX_INTERPOLATION_NESTING)
      {
        // TODO: Allow format string.
        if (nextChar(parser) != '(') lexError(parser, "Expect '(' after '%%'.");

        parser->parens[parser->numParens++] = 1;
        type = TOKEN_INTERPOLATION;
        break;
      }

      lexError(parser, "Interpolation may only nest %d levels deep.",
               MAX_INTERPOLATION_NESTING);
    }

    if (c == '\\')
    {
      switch (nextChar(parser))
      {
        case '"':  wrenByteBufferWrite(parser->vm, &string, '"'); break;
        case '\\': wrenByteBufferWrite(parser->vm, &string, '\\'); break;
        case '%':  wrenByteBufferWrite(parser->vm, &string, '%'); break;
        case '0':  wrenByteBufferWrite(parser->vm, &string, '\0'); break;
        case 'a':  wrenByteBufferWrite(parser->vm, &string, '\a'); break;
        case 'b':  wrenByteBufferWrite(parser->vm, &string, '\b'); break;
        case 'e':  wrenByteBufferWrite(parser->vm, &string, '\33'); break;
        case 'f':  wrenByteBufferWrite(parser->vm, &string, '\f'); break;
        case 'n':  wrenByteBufferWrite(parser->vm, &string, '\n'); break;
        case 'r':  wrenByteBufferWrite(parser->vm, &string, '\r'); break;
        case 't':  wrenByteBufferWrite(parser->vm, &string, '\t'); break;
        case 'u':  readUnicodeEscape(parser, &string, 4); break;
        case 'U':  readUnicodeEscape(parser, &string, 8); break;
        case 'v':  wrenByteBufferWrite(parser->vm, &string, '\v'); break;
        case 'x':
          wrenByteBufferWrite(parser->vm, &string,
                              (uint8_t)readHexEscape(parser, 2, "byte"));
          break;

        default:
          lexError(parser, "Invalid escape character '%c'.",
                   *(parser->currentChar - 1));
          break;
      }
    }
    else
    {
      wrenByteBufferWrite(parser->vm, &string, c);
    }
  }

  parser->next.value = wrenNewStringLength(parser->vm,
                                              (char*)string.data, string.count);

  wrenByteBufferClear(parser->vm, &string);
  makeToken(parser, type);
}

// Lex the next token and store it in [parser.next].
void wrenNextToken(Parser* parser)
{
  parser->previous = parser->current;
  parser->current = parser->next;

  // If we are out of tokens, don't try to tokenize any more. We *do* still
  // copy the TOKEN_EOF to previous so that code that expects it to be consumed
  // will still work.
  if (parser->next.type == TOKEN_EOF) return;
  if (parser->current.type == TOKEN_EOF) return;

  while (peekChar(parser) != '\0')
  {
    parser->tokenStart = parser->currentChar;

    char c = nextChar(parser);
    switch (c)
    {
      case '(':
        // If we are inside an interpolated expression, count the unmatched "(".
        if (parser->numParens > 0) parser->parens[parser->numParens - 1]++;
        makeToken(parser, TOKEN_LEFT_PAREN);
        return;

      case ')':
        // If we are inside an interpolated expression, count the ")".
        if (parser->numParens > 0 &&
            --parser->parens[parser->numParens - 1] == 0)
        {
          // This is the final ")", so the interpolation expression has ended.
          // This ")" now begins the next section of the template string.
          parser->numParens--;
          readString(parser);
          return;
        }

        makeToken(parser, TOKEN_RIGHT_PAREN);
        return;

      case '[': makeToken(parser, TOKEN_LEFT_BRACKET); return;
      case ']': makeToken(parser, TOKEN_RIGHT_BRACKET); return;
      case '{': makeToken(parser, TOKEN_LEFT_BRACE); return;
      case '}': makeToken(parser, TOKEN_RIGHT_BRACE); return;
      case ':': makeToken(parser, TOKEN_COLON); return;
      case ',': makeToken(parser, TOKEN_COMMA); return;
      case '*': makeToken(parser, TOKEN_STAR); return;
      case '%': makeToken(parser, TOKEN_PERCENT); return;
      case '#': {
        // Ignore shebang on the first line.
        if (parser->currentLine == 1 && peekChar(parser) == '!' && peekNextChar(parser) == '/')
        {
          skipLineComment(parser);
          break;
        }
        // Otherwise we treat it as a token
        makeToken(parser, TOKEN_HASH);
        return;
      }
      case '^': makeToken(parser, TOKEN_CARET); return;
      case '+': makeToken(parser, TOKEN_PLUS); return;
      case '-': makeToken(parser, TOKEN_MINUS); return;
      case '~': makeToken(parser, TOKEN_TILDE); return;
      case '?': makeToken(parser, TOKEN_QUESTION); return;

      case '|': twoCharToken(parser, '|', TOKEN_PIPEPIPE, TOKEN_PIPE); return;
      case '&': twoCharToken(parser, '&', TOKEN_AMPAMP, TOKEN_AMP); return;
      case '=': twoCharToken(parser, '=', TOKEN_EQEQ, TOKEN_EQ); return;
      case '!': twoCharToken(parser, '=', TOKEN_BANGEQ, TOKEN_BANG); return;

      case '.':
        if (matchChar(parser, '.'))
        {
          twoCharToken(parser, '.', TOKEN_DOTDOTDOT, TOKEN_DOTDOT);
          return;
        }

        makeToken(parser, TOKEN_DOT);
        return;

      case '/':
        if (matchChar(parser, '/'))
        {
          skipLineComment(parser);
          break;
        }

        if (matchChar(parser, '*'))
        {
          skipBlockComment(parser);
          break;
        }

        makeToken(parser, TOKEN_SLASH);
        return;

      case '<':
        if (matchChar(parser, '<'))
        {
          makeToken(parser, TOKEN_LTLT);
        }
        else
        {
          twoCharToken(parser, '=', TOKEN_LTEQ, TOKEN_LT);
        }
        return;

      case '>':
        if (matchChar(parser, '>'))
        {
          makeToken(parser, TOKEN_GTGT);
        }
        else
        {
          twoCharToken(parser, '=', TOKEN_GTEQ, TOKEN_GT);
        }
        return;

      case '\n':
        makeToken(parser, TOKEN_LINE);
        return;

      case ' ':
      case '\r':
      case '\t':
        // Skip forward until we run out of whitespace.
        while (peekChar(parser) == ' ' ||
               peekChar(parser) == '\r' ||
               peekChar(parser) == '\t')
        {
          nextChar(parser);
        }
        break;

      case '"': {
        if(peekChar(parser) == '"' && peekNextChar(parser)  == '"') {
          readRawString(parser);
          return;
        }
        readString(parser); return;
      }
      case '_':
        readName(parser,
                 peekChar(parser) == '_' ? TOKEN_STATIC_FIELD : TOKEN_FIELD, c);
        return;

      case '0':
        if (peekChar(parser) == 'x')
        {
          readHexNumber(parser);
          return;
        }

        readNumber(parser);
        return;

      default:
        if (isName(c))
        {
          readName(parser, TOKEN_NAME, c);
        }
        else if (isDigit(c))
        {
          readNumber(parser);
        }
        else
        {
          if (c >= 32 && c <= 126)
          {
            lexError(parser, "Invalid character '%c'.", c);
          }
          else
          {
            // Don't show non-ASCII values since we didn't UTF-8 decode the
            // bytes. Since there are no non-ASCII byte values that are
            // meaningful code units in Wren, the lexer works on raw bytes,
            // even though the source code and console output are UTF-8.
            lexError(parser, "Invalid byte 0x%x.", (uint8_t)c);
          }
          parser->next.type = TOKEN_ERROR;
          parser->next.length = 0;
        }
        return;
    }
  }

  // If we get here, we're out of source, so just make EOF tokens.
  parser->tokenStart = parser->currentChar;
  makeToken(parser, TOKEN_EOF);
}
