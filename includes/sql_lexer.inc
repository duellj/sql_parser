<?php
// $Id: sql_lexer.inc,v 1.9 2010/07/09 21:27:34 duellj Exp $

/**
 * @file
 */

/**
 * SQL Lexer
 *
 * Adapted from the PEAR SQL_Parser package.
 *
 * @see http://pear.php.net/package/SQL_Parser
 *
 * @todo Document, Document, Document!!
 * @todo Scale down the beast that is nextToken().
 * 
 */
class SqlLexer {
  // array of valid tokens for the lexer to recognize
  // format is 'token literal' => TOKEN_VALUE
  var $symbols = array();

  /**
   * The sql string to lex.
   *
   * @var string
   */
  private $sql_string;

  var $tokenPointer = 0;
  var $tokenStart = 0;
  var $tokenLength = 0;
  var $tokenText = '';
  var $lineNumber = 0;
  var $lineBegin = 0;
  var $stringLength = 0;

  // Will not be altered by skip().
  var $tokAbsStart = 0;
  var $skipText = '';

  // Provide lookahead capability.
  var $lookahead = 0;
  // Specify how many tokens to save in tokenStack, so the
  // token stream can be pushed back.
  var $tokenStack = array();
  var $stackPointer = 0;

  public function __construct($sql_string = '', $lookahead=0) {
    $this->sql_string = $sql_string;
    $this->stringLength = strlen($sql_string);
    $this->lookahead = $lookahead;
  }
    
  /**
   * Sets the current token text, pointer and skip text if necessary.
   *
   * Uses tokenStart and tokenLength to set the text of the current token, and
   * tokenAbsStart to set any skipped characters at the beginning of the token.
   * Token is set to token parameter, if set, otherwise is set to tokenText.
   *
   */
  private function setToken() {
    $this->tokenText = substr($this->sql_string, $this->tokenStart, $this->tokenLength);
    $this->tokenStart = $this->tokenPointer;
    $this->skipText = substr($this->sql_string, $this->tokenAbsStart, $this->tokenStart - $this->tokenAbsStart);
  }

  /**
   * Returns the next character from the current token string.
   *
   * Increases the token pointer and token length by one and returns the next
   * single character of the current token string, or NULL if the end of the
   * tokenized string is reached.
   *
   * @return
   *   The next single character or NULL.
   */ 
  private function get() {
    ++$this->tokenPointer;
    ++$this->tokenLength;
    return ($this->tokenPointer <= $this->stringLength) ? $this->sql_string{$this->tokenPointer - 1} : NULL;
  }

  /**
   * Rewinds token pointer to previous character.
   */
  public function unget() {
    --$this->tokenPointer;
    --$this->tokenLength;
  }

  /**
   * Skips the current character and returns the next character.
   *
   * Increases the start of the current token and the token pointer and returns
   * the next single character of the new token, or NULL if the end of the 
   * tokenized string is reached.

   * @return
   *   The next single character or NULL.
   */
  private function skip() {
    ++$this->tokenStart;
    return ($this->tokenPointer != $this->stringLength) ? $this->sql_string{$this->tokenPointer++} : NULL;
  }

  /**
   * Reverts to the previous token and resets token length.
   */
  private function revert() {
    $this->tokenPointer = $this->tokenStart;
    $this->tokenLength = 0;
  }

  /**
   * Checks if character is a comparison operator.
   *
   * @param $character
   *   A single character.
   *
   * @return
   *   TRUE if $character is a comparison operator, FALSE otherwise.
   * @todo Should this live in SqlDialect?
   */
  private function isComparisonOperator($character) {
    return (($character == '<') || ($character == '>') || ($character == '=') || ($character == '!'));
  }

  /**
   * Pushes back a token, so the very next call to lex() will return that token.
   *
   * Calls to this function will be ignored if there is no lookahead specified
   * to the constructor, or the pushBack() function has already been called the
   * maximum number of token's that can be looked ahead.
   */
  private function pushBack() {
    if ($this->lookahead > 0 && count($this->tokenStack) > 0 && $this->stackPointer > 0) {
      $this->stackPointer--;
    }
  }

  /**
   * Either returns the next token from the token stack or lexes the next token.
   *
   * If lookahead is enabled and there are tokens available from the token
   * stack, then the next token from the token stack is returned.  Otherwise
   * the next token is lexed and returned.
   *
   * @return
   *   The next token.
   */
  public function lex() {
    if ($this->lookahead > 0) {
      // The stackPointer, should always be the same as the count of
      // elements in the tokenStack.  The stackPointer, can be thought
      // of as pointing to the next token to be added.  If however
      // a pushBack() call is made, the stackPointer, will be less than the
      // count, to indicate that we should take that token from the
      // stack, instead of calling nextToken for a new token.
      if ($this->stackPointer < count($this->tokenStack)) {

        $this->tokenText = $this->tokenStack[$this->stackPointer]['tokenText'];
        $this->skipText = $this->tokenStack[$this->stackPointer]['skipText'];
        $token = $this->tokenStack[$this->stackPointer]['token'];

        // We have read the token, so now iterate again.
        $this->stackPointer++;
        return $token;

      } 
      else {

        // If $tokenStack is full (equal to lookahead), pop the oldest
        // element off, to make room for the new one.
        if ($this->stackPointer == $this->lookahead) {
          // For some reason array_shift and
          // array_pop screw up the indexing, so we do it manually.
          for ($i=0; $i < (count($this->tokenStack) - 1); $i++) {
            $this->tokenStack[$i] = $this->tokenStack[$i + 1];
          }

          // Indicate that we should put the element in
          // at the stackPointer position.
          $this->stackPointer--;
        }

        $token = $this->nextToken();
        $this->tokenStack[$this->stackPointer] = array(
          'token' => $token,
          'tokenText' => $this->tokenText,
          'skipText' => $this->skipText
        );
        $this->stackPointer++;
        return $token;
      }
    }
    else {
      return $this->nextToken();
    }
  }

  /**
   * Returns the next token in the string, or NULL if end of string is reached.
   *
   * @return
   *   The next full token or NULL if end of string is reached.
   */
  private function nextToken() {
    if ($this->sql_string == '') {
      return;
    }
    $state = 0;
    $this->tokenAbsStart = $this->tokenStart;

    while (TRUE) {
      switch($state) {
        // State 0 : Start of token.
        case 0:
          $this->tokenPointer = $this->tokenStart;
          $this->tokenText = '';
          $this->tokenLength = 0;
          $c = $this->get();

          // End of string is reached.
          if (is_NULL($c)) {
            $state = 1000;
            break;
          }

          // Skip whitespace at beginning of token.
          while (($c == ' ') || ($c == "\t") || ($c == "\n") || ($c == "\r")) {
            if ($c == "\n" || $c == "\r") {
              // Handles MAC/Unix/Windows line endings for multiline sql strings.
              if ($c == "\r") {
                $c = $this->skip();

                // If not DOS newline
                if($c != "\n")
                  $this->unget();
              }
              ++$this->lineNumber;
              $this->lineBegin = $this->tokenPointer;
            }
            $c = $this->skip();
            $this->tokenLength = 1;
          }

          // Escape quotes and backslashes.
          if ($c == '\\') {
            $t = $this->get();
            if ($t == '\'' || $t == '\\' || $t == '"') {
              $this->tokenText = $t;
              $this->tokenStart = $this->tokenPointer;
              return $this->tokenText;
            } 
            else {
              $this->unget();
              // Unknown token.  Revert to single chararacter.
              $state = 999;
              break;
            }
          }

          // Text string.
          if (($c == '\'') || ($c == '"')) {
            $quote = $c;
            $state = 12;
            break;
          }

          // System variable.
          if ($c == '_') {
            $state = 18;
            break;
          }

          // Reserved keyword or identifier (e.g. column or table name).
          if (ctype_alpha(ord($c))) {
            $state = 1;
            break;
          }

          // Real number or integer.
          if (ctype_digit(ord($c))) { 
            $state = 5;
            break;
          }

          if ($c == '.') {
            $t = $this->get();
            // Ellipsis.
            if ($t == '.') {
              if ($this->get() == '.') {
                $this->tokenText = '...';
                $this->tokenStart = $this->tokenPointer;
                return $this->tokenText;
              } 
              else {
                // Unknown token.  Revert to single character.
                $state = 999;
                break;
              }
            } 
            // Real number.
            elseif (ctype_digit(ord($t))) {
              $this->unget();
              $state = 7;
              break;
            } 
            else { // period
              $this->unget();
            }
          }

          // Pound sign comment style.
          if ($c == '#') {
            $state = 14;
            break;
          }
          // Double dash comment style.
          if ($c == '-') {
            $t = $this->get();
            // Double dashes start comments.
            if ($t == '-') {
              $state = 14;
              break;
            } 
            else { // negative number
              $this->unget();
              $state = 5;
              break;
            }
          }

          // Drupal 6 placeholders.
          // @todo possibly abstract this out to make sql_lexer code agnostic
          // i.e. not contain any drupalisms
          if ($c == '%') {
            $state = 20;
            break;
          }

          // Comparison operator.
          if ($this->isComparisonOperator($c)) {
            $state = 10;
            break;
          }

          // Unknown token.  Revert to single char.
          $state = 999;
          break;

        // State 1 : Incomplete keyword or identifier.
        case 1:
          $c = $this->get();
          if (ctype_alnum(ord($c)) || ($c == '_') || ($c == '.')) {
            $state = 1;
            break;
          }
          $state = 2;
          break;

        // State 2 : Complete keyword or identifier (e.g. column or table name).
        case 2:
          $this->unget();
          $this->setToken();

          // Check if this token is a keyword in the symbols list.
          $testToken = strtolower($this->tokenText);
          if (isset($this->symbols[$testToken])) {
            return $testToken;
          } 
          else {
            return 'identifier';
          }
          break;

        // State 5: Incomplete real or int number.
        case 5:
          $c = $this->get();
          if (ctype_digit(ord($c))) {
            $state = 5;
            break;
          } 
          elseif ($c == '.') {
            $t = $this->get();
            if($t == '.') { // ellipsis
              $this->unget();
            } 
            else { // real number
              $state = 7;
              break;
            }
          } 
          elseif (ctype_alpha(ord($c))) { // number must end with non-alpha character
            $state = 999;
            break;
          } 
          else {
            // complete number
            $state = 6;
            break;
          }

        // State 6: Complete integer number.
        case 6:
          $this->unget();
          $this->setToken();
          // Cast tokenText to int.
          // @todo is this necessary?
          $this->tokenText = intval($this->tokenText);
          return 'int_val';
          break;

        // State 7: Incomplete real number.
        case 7:
          $c = $this->get();

          // Check if number is in scientific notation.
          if ($c == 'e' || $c == 'E') {
            $state = 15;
            break;
          }

          if (ctype_digit(ord($c))) {
            $state = 7;
            break;
          }
          $state = 8;
          break;

        // State 8: Complete real number.
        case 8:
          $this->unget();
          $this->setToken();
          // Cast tokenText to float.
          // @todo is this necessary?
          $this->tokenText = floatval($this->tokenText);
          return 'real_val';

        // State 10: Incomplete comparison operator.
        case 10:
          $c = $this->get();
          if ($this->isComparisonOperator($c)) {
            $state = 10;
            break;
          }
          $state = 11;
          break;

        // State 11: Complete comparison operator.
        case 11:
          $this->unget();
          $this->setToken();
          if($this->tokenText) {
            return $this->tokenText;
          }
          $state = 999;
          break;

        // State 12: Incomplete text string.
        case 12:
          $bail = FALSE;
          while (!$bail) {
            switch ($this->get()) {
              case '':
                $this->tokenText = NULL;
                $bail = TRUE;
                break;
              case "\\":
                if (!$this->get()) {
                  $this->tokenText = NULL;
                  $bail = TRUE;
                }
                break;
              // String placeholder
              case '%':
                if ($this->get() == 's') {
                  $this->unget();
                  $state = 20;
                  break 3;
                }
                $this->unget();
                break;
              case $quote:
                $bail = TRUE;
                break;
            }
          }
          if (!is_NULL($this->tokenText)) {
            $state = 13;
            break;
          }
          $state = 999;
          break;

        // State 13: Complete text string.
        case 13:
          $this->setToken();
          return 'text_val';
          break;

        // State 14: Comment.
        case 14:
          $c = $this->skip();
          if ($c == "\n" || $c == "\r" || $c == "") {
            // Handle MAC/Unix/Windows line endings.
            if ($c == "\r") {
              $c = $this->skip();
              // If not DOS newline
              if ($c != "\n") {
                $this->unget();
              }
            }

            if ($c != "") {
              ++$this->lineNumber;
              $this->lineBegin = $this->tokenPointer;
            }

            // We need to skip all the text.
            $this->tokenStart = $this->tokenPointer;
            $state = 0;
          } 
          else {
            $state = 14;
          }
          break;

        // State 15: Exponent Sign in Scientific Notation.
        case 15:
          $c = $this->get();
          if($c == '-' || $c == '+') {
            $state = 16;
            break;
          }
          $state = 999;
          break;

        // state 16: Exponent Value-first digit in Scientific Notation
        case 16:
          $c = $this->get();
          if (ctype_digit(ord($c))) {
            $state = 17;
            break;
          }
          $state = 999;  // if no digit, then token is unknown
          break;

        // State 17: Exponent Value in Scientific Notation.
        case 17:
          $c = $this->get();
          if (ctype_digit(ord($c))) {
            $state = 17;
            break;
          }
          $state = 8;  // At least 1 exponent digit was required
          break;

        // State 18 : Incomplete System Variable.
        case 18:
          $c = $this->get();
          if (ctype_alnum(ord($c)) || $c == '_') {
            $state = 18;
            break;
          }
          $state = 19;
          break;

        // State 19: Complete System Variable.
        case 19:
          $this->unget();
          $this->setToken();
          return 'sys_var';

        //  State 20: Placeholder.
        case 20:
          $c = $this->get();
          if ($c == '%') {
            $state = 999;
            break;
          }
          elseif ($c == 's') {
            // Check if this is actually an incomplete string.
            if ($this->get() != $quote) {
              $state = 12;
              break;
            }
          }
          $this->setToken();
          return 'placeholder';
          break;

        // State 999 : Unknown token.  Revert to single char.
        case 999:
          $this->revert();
          $this->get();
          $this->setToken();
          return $this->tokenText;

        // State 1000 : End Of Input.
        case 1000:
          $this->tokenText = '*end of input*';
          $this->skipText = substr($this->sql_string, $this->tokenAbsStart, $this->tokenStart - $this->tokenAbsStart);
          $this->tokenStart = $this->tokenPointer;
          return NULL;
      }
    }
  }
}

