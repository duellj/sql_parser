<?php
// $Id: sql_parser.inc,v 1.10 2010/07/09 21:27:34 duellj Exp $

/**
 * @file
 */

/**
 * Sql Parser
 *
 * Adapted from the PEAR SQL_Parser package.
 *
 * @see http://pear.php.net/package/SQL_Parser
 *
 * @todo Document, Document, Document!!
 * @todo Add example code samples.
 * @todo Update classes to use php5 standards.
 * @todo Extract symbols into SqlDialect.
 * @todo Provide sane exception handling in raiseError().
 * @todo Parse multiple SQL strings.
 */
class SqlParser {

  /**
   * The lexer object for the parser.
   *
   * @var SqlLexer
   */
  private $lexer;

  /**
   * The SQL dialect object for the parser.
   *
   * @var SqlDialect
   */
  private $dialect;

  /**
   * The current parsed token.
   */
  private $token;

  /**
   * The array of parsed SQL tokens.
   */
  private $tree;

  /**
   * __construct 
   * 
   * @param $string 
   *   The SQL string to parse.
   *   
   */
  public function __construct($sql_string = NULL) {
    $this->sql_string = $sql_string;
    $this->dialect = new SqlDialect();

    if (is_string($sql_string)) {
      $this->lexer = new SqlLexer($this->sql_string, 1);
      $this->lexer->symbols =& $this->dialect->symbols;
    }
  }

  /**
   * Parses a raw SQL string into an array of SQL components.
   * 
   * @param $string 
   *   The SQL string to parse.

   * @return
   *   An array containing the tokens and token types of all tokens within
   *   the SQL function or list set.
   */
  public function parse($sql_string = NULL) {
    if (is_string($sql_string)) {
      $this->sql_string = $sql_string;
      // Initialize the Lexer with a 3-level look-back buffer.
      $this->lexer = new SqlLexer($this->sql_string, 3);
      $this->lexer->symbols =& $this->dialect->symbols;
    } 
    else {
      if (!is_object($this->lexer)) {
        return $this->raiseError('No initial string specified');
      }
    }

    // Get query action.
    $this->getToken();
    switch ($this->token) {
      case NULL:
        // NULL == end of string
        return $this->raiseError('Nothing to do');
      case 'select':
        return $this->parseSelect();
      case 'update':
        return $this->parseUpdate();
      case 'insert':
        return $this->parseInsert();
      case 'delete':
        return $this->parseDelete();
      case 'create':
        return $this->parseCreate();
      case 'drop':
        return $this->parseDrop();
      default:
        return $this->raiseError('Unknown action :' . $this->token);
    }
  }

  /**
   * Get the parameters of an SQL function or list set.
   *
   * @return
   *   An array containing the tokens and token types of all tokens within
   *   the SQL function or list set.
   */
  function getParameters() {
    $parameters = array(
      'values' => array(),
      'types' => array(),
    );
    while ($this->token != ')') {
      $this->getToken();
      if ($this->dialect->isVal($this->token) || ($this->token == 'identifier')) {
        $parameters['values'][] = $this->lexer->tokenText;
        $parameters['types'][] = $this->token;
      } 
      else {
        return $this->raiseError('Expected a value');
      }

      $this->getToken();
      if ($this->token == ')') {
        return $parameters;
      }
      if (($this->token != ',') && ($this->token != ')')) {
        return $this->raiseError('Expected , or )');
      }
    }
  }

  /**
   * raiseError 
   * 
   * @param mixed $message 
   */
  function raiseError($message) {
    $end = 0;
    if ($this->lexer->string != '') {
      while (($this->lexer->lineBegin+$end < $this->lexer->stringLength) && ($this->lexer->string{$this->lexer->lineBegin+$end} != "\n")) {
        ++$end;
      }
    }

    $message = 'Parse error: ' . $message . ' on line ' . ($this->lexer->lineNumber + 1) . "\n";
    $message .= substr($this->lexer->string, $this->lexer->lineBegin, $end) . "\n";
    $length = is_NULL($this->token) ? 0 : strlen($this->lexer->tokenText);
    $message .= str_repeat(' ', abs($this->lexer->tokenPointer - $this->lexer->lineBegin - $length)) . "^";
    $message .= ' found: "' . $this->lexer->tokenText . '"';
    drupal_set_message("<pre>" . $message . "</pre>");

    return FALSE; //PEAR::raiseError($message);
  }
    
  // getToken()
  function getToken() {
    $this->token = $this->lexer->lex();
  }
    
  // &parseFieldOptions()
  function parseFieldOptions() {
    // parse field options
    $namedConstraint = FALSE;
    $options = array();
    while (($this->token != ',') && ($this->token != ')') && ($this->token != NULL)) {
      $option = $this->token;
      $haveValue = TRUE;
      switch ($option) {
        case 'constraint':
          $this->getToken();
          if ($this->token = 'identifier') {
            $constraintName = $this->lexer->tokenText;
            $namedConstraint = TRUE;
            $haveValue = FALSE;
          }
          else {
            return $this->raiseError('Expected a constraint name');
          }
          break;
        case 'default':
          $this->getToken();
          if ($this->dialect->isVal($this->token)) {
            $constraintOpts = array(
              'type' => 'default_value',
              'value' => $this->lexer->tokenText,
            );
          }
          elseif ($this->dialect->isFunc($this->token)) {
            $results = $this->parseFunctionOpts();
            //if (PEAR::isError($results)) {
            //return $results;
            //}
            $results['type'] = 'default_function';
            $constraintOpts = $results;
          }
          else {
            return $this->raiseError('Expected default value');
          }
          break;
        case 'primary':
          $this->getToken();
          if ($this->token == 'key') {
            $constraintOpts = array(
              'type' => 'primary_key',
              'value' => TRUE,
            );
          } 
          else {
            return $this->raiseError('Expected "key"');
          }
          break;
        case 'not':
          $this->getToken();
          if ($this->token == 'NULL') {
            $constraintOpts = array('type' => 'not_NULL',
              'value' => TRUE);
          }
          else {
            return $this->raiseError('Expected "NULL"');
          }
          break;
        case 'check':
          $this->getToken();
          if ($this->token != '(') {
            return $this->raiseError('Expected (');
          }
          $results = $this->parseSearchClause();
          //if (PEAR::isError($results)) {
          //return $results;
          //}
          $results['type'] = 'check';
          $constraintOpts = $results;
          if ($this->token != ')') {
            return $this->raiseError('Expected )');
          }
          break;
        case 'unique':
          $this->getToken();
          if ($this->token != '(') {
            return $this->raiseError('Expected (');
          }
          $constraintOpts = array('type' => 'unique');
          $this->getToken();
          while ($this->token != ')') {
            if ($this->token != 'identifier') {
              return $this->raiseError('Expected an identifierifier');
            }
            $constraintOpts['column_names'][] = $this->lexer->tokenText;
            $this->getToken();
            if (($this->token != ')') && ($this->token != ',')) {
              return $this->raiseError('Expected ) or ,');
            }
          }
          if ($this->token != ')') {
            return $this->raiseError('Expected )');
          }
          break;
        case 'month': 
        case 'year': 
        case 'day': 
        case 'hour':
        case 'minute': 
        case 'second':
          $intervals = array(
            array(
              'month' => 0,
              'year' => 1
            ),
            array(
              'second' => 0,
              'minute' => 1,
              'hour' => 2,
              'day' => 3
            )
          );
          foreach ($intervals as $class) {
            if (isset($class[$option])) {
              $constraintOpts = array('quantum_1' => $this->token);
              $this->getToken();
              if ($this->token == 'to') {
                $this->getToken();
                if (!isset($class[$this->token])) {
                  return $this->raiseError('Expected interval quanta');
                }
                if ($class[$this->token] >= $class[$constraintOpts['quantum_1']]) {
                  return $this->raiseError($this->token . ' is not smaller than ' . $constraintOpts['quantum_1']);
                } 
                $constraintOpts['quantum_2'] = $this->token;
              }
              else {
                $this->lexer->unget();
              }
              break;
            }
          }
          if (!isset($constraintOpts['quantum_1'])) {
            return $this->raiseError('Expected interval quanta');
          }
          $constraintOpts['type'] = 'values';
          break;
        case 'NULL':
          $haveValue = FALSE;
          break;
        default:
          return $this->raiseError('Unexpected token ' . $this->lexer->tokenText);
      }
      if ($haveValue) {
        if ($namedConstraint) {
          $options['constraints'][$constraintName] = $constraintOpts;
          $namedConstraint = FALSE;
        } 
        else {
          $options['constraints'][] = $constraintOpts;
        }
      }
      $this->getToken();
    }
    return $options;
  }
    
  // parseSearchClause()
  function parseSearchClause($subSearch = FALSE) {
    $clause = array();
    // parse the first argument
    $this->getToken();
    if ($this->token == 'not') {
      $clause['neg'] = TRUE;
      $this->getToken();
    }

    $foundSubclause = FALSE;
    if ($this->token == '(') {
      $clause['arg_1']['value'] = $this->parseSearchClause(TRUE);
      $clause['arg_1']['type'] = 'subclause';
      if ($this->token != ')') {
        return $this->raiseError('Expected ")"');
      }
      $foundSubclause = TRUE;
    } 
    elseif ($this->dialect->isFunc($this->token)) {
      $clause['arg_1']['value'] = $this->parseFunctionOpts();
      $clause['arg_1']['type'] = 'function';
    }
    elseif ($this->dialect->isReserved($this->token)) {
      return $this->raiseError('Expected a column name or value');
    } 
    else {
      $clause['arg_1']['value'] = $this->lexer->tokenText;
      $clause['arg_1']['type'] = $this->token;
    }

    // parse the operator
    if (!$foundSubclause) {
      $this->getToken();
      if (!$this->dialect->isOperator($this->token)) {
        return $this->raiseError('Expected an operator');
      }
      $clause['op'] = $this->token;

      $this->getToken();
      switch ($clause['op']) {
        case 'is':
          // parse for 'is' operator
          if ($this->token == 'not') {
            $clause['neg'] = TRUE;
            $this->getToken();
          }
          if ($this->token != 'NULL') {
            return $this->raiseError('Expected "NULL"');
          }
          $clause['arg_2']['value'] = '';
          $clause['arg_2']['type'] = $this->token;
          break;
        case 'not':
          // parse for 'not in' operator
          if ($this->token != 'in') {
            return $this->raiseError('Expected "in"');
          }
          $clause['op'] = $this->token;
          $clause['neg'] = TRUE;
          $this->getToken();
        case 'in':
          // parse for 'in' operator 
          if ($this->token != '(') {
            return $this->raiseError('Expected "("');
          }

          // read the subset
          $this->getToken();
          // is this a subselect?
          if ($this->token == 'select') {
            $clause['arg_2']['value'] = $this->parseSelect(TRUE);
            $clause['arg_2']['type'] = 'command';
          } 
          elseif ($this->token == 'placeholder') {
            $clause['arg_2']['value'] = $this->lexer->tokenText;
            $clause['arg_2']['type'] = $this->token;
            $this->getToken();
          }
          else {
            $this->lexer->pushBack();
            // parse the set
            $parameters = $this->getParameters();
            $clause['arg_2']['value'] = $parameters['values'];
            $clause['arg_2']['type'] = $parameters['types'];
          }
          if ($this->token != ')') {
            return $this->raiseError('Expected ")"');
          }
          break;
        case 'and': 
        case 'or':
          $this->lexer->unget();
          break;
        default:
          // parse for in-fix binary operators
          if ($this->dialect->isReserved($this->token)) {
            return $this->raiseError('Expected a column name or value');
          }
          if ($this->token == '(') {
            $clause['arg_2']['value'] = $this->parseSearchClause(TRUE);
            $clause['arg_2']['type'] = 'subclause';
            $this->getToken();
            if ($this->token != ')') {
              return $this->raiseError('Expected ")"');
            }
          } 
          else {
            $clause['arg_2']['value'] = $this->lexer->tokenText;
            $clause['arg_2']['type'] = $this->token;
          }
          break;
      }
    }

    $this->getToken();
    if (($this->token == 'and') || ($this->token == 'or')) {
      $op = $this->token;
      $subClause = $this->parseSearchClause($subSearch);
      //if (PEAR::isError($subClause)) {
      //return $subClause;
      //} else {
      $clause = array('arg_1' => $clause,
        'op' => $op,
        'arg_2' => $subClause);
      //}
    } 
    else {
      $this->lexer->unget();
    }
    return $clause;
  }
    
  // parseFieldList()
  function parseFieldList() {
    $this->getToken();
    if ($this->token != '(') {
      return $this->raiseError('Expected (');
    }

    $fields = array();
    while (1) {
      // parse field identifierifier
      $this->getToken();
      if ($this->token == 'identifier') {
        $name = $this->lexer->tokenText;
      } 
      elseif ($this->token == ')') {
        return $fields;
      } 
      else {
        return $this->raiseError('Expected identifierifier');
      }

      // parse field type
      $this->getToken();
      if ($this->dialect->isType($this->token)) {
        $type = $this->token;
      } 
      else {
        return $this->raiseError('Expected a valid type');
      }

      $this->getToken();
      // handle special case two-word types
      if ($this->token == 'precision') {
        // double precision == double
        if ($type == 'double') {
          return $this->raiseError('Unexpected token');
        }
        $this->getToken();
      } 
      elseif ($this->token == 'varying') {
        // character varying() == varchar()
        if ($type == 'character') {
          $type == 'varchar';
          $this->getToken();
        } 
        else {
          return $this->raiseError('Unexpected token');
        }
      }
      $fields[$name]['type'] = $this->dialect->getSynonym($type);
      // parse type parameters
      if ($this->token == '(') {
        $parameters = $this->getParameters();
        //if (PEAR::isError($results)) {
        //return $results;
        //}
        switch ($fields[$name]['type']) {
          case 'numeric':
            if (isset($parameters['values'][1])) {
              if ($parameters['types'][1] != 'int_val') {
                return $this->raiseError('Expected an integer');
              }
              $fields[$name]['decimals'] = $parameters['values'][1];
            }
          case 'float':
            if ($parameters['types'][0] != 'int_val') {
              return $this->raiseError('Expected an integer');
            }
            $fields[$name]['length'] = $parameters['values'][0];
            break;
          case 'char': 
          case 'varchar':
          case 'integer': 
          case 'int':
            if (sizeof($parameters['values']) != 1) {
              return $this->raiseError('Expected 1 parameter');
            }
            if ($parameters['types'][0] != 'int_val') {
              return $this->raiseError('Expected an integer');
            }
            $fields[$name]['length'] = $parameters['values'][0];
            break;
          case 'set': 
          case 'enum':
            if (!sizeof($parameters['values'])) {
              return $this->raiseError('Expected a domain');
            }
            $fields[$name]['domain'] = $parameters['values'];
            break;
          default:
            if (sizeof($parameters['values'])) {
              return $this->raiseError('Unexpected )');
            }
        }
        $this->getToken();
      }

      $options = $this->parseFieldOptions();
      //if (PEAR::isError($options)) {
      //return $options;
      //}

      $fields[$name] += $options;

      if ($this->token == ')') {
        return $fields;
      } 
      elseif (is_NULL($this->token)) {
        return $this->raiseError('Expected )');
      }
    }
  }
    
  // parseFunctionOpts()
  function parseFunctionOpts() {
    $function = $this->token;
    $opts['name'] = $function;
    $this->getToken();
    if ($this->token != '(') {
      return $this->raiseError('Expected "("');
    }
    switch ($function) {
      case 'count':
        $this->getToken();
        switch ($this->token) {
          case 'distinct':
            $opts['distinct'] = TRUE;
            $this->getToken();
            if ($this->token != 'identifier') {
              return $this->raiseError('Expected a column name');
            }
          case 'identifier': 
          case '*':
            $opts['arg'][] = $this->lexer->tokenText;
            break;
          default:
            return $this->raiseError('Invalid argument');
        }
        break;
      case 'concat':
        $this->getToken();
        while ($this->token != ')') {
          switch ($this->token) {
            case 'identifier': 
              $opts['arg'][] = $this->lexer->tokenText;
              break;
            case 'text_val':
              $opts['arg'][] = "'" .  $this->lexer->tokenText . "'";
              break;
            case ',':
              // do nothing
              break;
            default:
              return $this->raiseError('Expected a string or a column name');
          }
          $this->getToken();
        }
        $this->lexer->pushBack();
        break;
      case 'avg': 
      case 'min': 
      case 'max': 
      case 'sum':
      default:
        $this->getToken();
        $opts['arg'] = $this->lexer->tokenText;
        break;
    }
    $this->getToken();
    if ($this->token != ')') {
      return $this->raiseError('Expected ")"');
    }

    // check for an alias, from, or operator
    $this->getToken();
    if ($this->token == ',' || $this->token == 'from' || $this->dialect->isOperator($this->token)) {
      $this->lexer->pushBack();
    } 
    elseif ($this->token == 'as') {
      $this->getToken();
      if ($this->token == 'identifier' ) {
        $opts['alias'] = $this->lexer->tokenText;
      } 
      else {
        return $this->raiseError('Expected column alias');
      }
    } 
    else {
      if ($this->token == 'identifier' ) {
        $opts['alias'] = $this->lexer->tokenText;
      } 
      else {
        return $this->raiseError('Expected column alias, from or comma');
      }
    }
    return $opts;
  }
    
  // parseCreate()
  function parseCreate() {
    $this->getToken();
    switch ($this->token) {
      case 'table':
        $this->tree = array('command' => 'create_table');
        $this->getToken();
        if ($this->token == 'identifier') {
          $this->tree['table_names'][] = $this->lexer->tokenText;
          $fields = $this->parseFieldList();
          //if (PEAR::isError($fields)) {
          //return $fields;
          //}
          $this->tree['column_defs'] = $fields;
          //                    $this->tree['column_names'] = array_keys($fields);
        } 
        else {
          return $this->raiseError('Expected table name');
        }
        break;
      case 'index':
        $this->tree = array('command' => 'create_index');
        break;
      case 'constraint':
        $this->tree = array('command' => 'create_constraint');
        break;
      case 'sequence':
        $this->tree = array('command' => 'create_sequence');
        break;
      default:
        return $this->raiseError('Unknown object to create');
    }
    return $this->tree;
  }
    
  // parseInsert()
  function parseInsert() {
    $this->getToken();
    if ($this->token == 'into') {
      $this->tree = array('command' => 'insert');
      $this->getToken();
      if ($this->token == 'identifier') {
        $this->tree['table_names'][] = $this->lexer->tokenText;
        $this->getToken();
      } 
      else {
        return $this->raiseError('Expected table name');
      }
      if ($this->token == '(') {
        $parameters = $this->getParameters();
        //if (PEAR::isError($results)) {
        //return $results;
        //} else {
        if (sizeof($parameters['values'])) {
          $this->tree['column_names'] = $parameters['values'];
        }
        //}
        $this->getToken();
      }
      if ($this->token == 'values') {
        $this->getToken();
        $parameters = $this->getParameters();
        //if (PEAR::isError($results)) {
        //return $results;
        //} else {
        if (isset($this->tree['column_defs']) && (sizeof($this->tree['column_defs']) != sizeof($parameters['values']))) {
          return $this->raiseError('field/value mismatch');
        }
        if (sizeof($parameters['values'])) {
          foreach ($parameters['values'] as $key => $value) {
            $parameters['values'][$key] = array('value' => $value,
              'type' => $parameters['types'][$key]);
          }
          $this->tree['values'] = $parameters['values'];
        } 
        else {
          return $this->raiseError('No fields to insert');
        }
        //}
      } 
      else {
        return $this->raiseError('Expected "values"');
      }
    } 
    else {
      return $this->raiseError('Expected "into"');
    }
    return $this->tree;
  }
    
  // parseUpdate()
  function parseUpdate() {
    $this->getToken();
    if ($this->token == 'identifier') {
      $this->tree = array('command' => 'update');
      $this->tree['table_names'][] = $this->lexer->tokenText;
    } 
    else {
      return $this->raiseError('Expected table name');
    }
    $this->getToken();
    if ($this->token != 'set') {
      return $this->raiseError('Expected "set"');
    }
    while (TRUE) {
      $this->getToken();
      if ($this->token != 'identifier') {
        return $this->raiseError('Expected a column name');
      }
      $this->tree['column_names'][] = $this->lexer->tokenText;
      $this->getToken();
      if ($this->token != '=') {
        return $this->raiseError('Expected =');
      }
      $this->getToken();
      if (!$this->dialect->isVal($this->token)) {
        return $this->raiseError('Expected a value');
      }
      $this->tree['values'][] = array('value' => $this->lexer->tokenText,
        'type' => $this->token);
      $this->getToken();
      if ($this->token == 'where') {
        $clause = $this->parseSearchClause();
        //if (PEAR::isError($clause)) {
        //return $clause;
        //}
        $this->tree['where_clause'] = $clause;
        break;
      } 
      elseif ($this->token != ',') {
        return $this->raiseError('Expected "where" or ","');
      }
    }
    return $this->tree;
  }
    
  // parseDelete()
  function parseDelete() {
    $this->getToken();
    if ($this->token != 'from') {
      return $this->raiseError('Expected "from"');
    }
    $this->tree = array('command' => 'delete');
    $this->getToken();
    if ($this->token != 'identifier') {
      return $this->raiseError('Expected a table name');
    }
    $this->tree['table_names'][] = $this->lexer->tokenText;
    $this->getToken();
    if ($this->token != 'where') {
      return $this->raiseError('Expected "where"');
    }
    $clause = $this->parseSearchClause();
    //if (PEAR::isError($clause)) {
    //return $clause;
    //}
    $this->tree['where_clause'] = $clause;
    return $this->tree;
  }
    
  // parseDrop()
  function parseDrop() {
    $this->getToken();
    switch ($this->token) {
      case 'table':
        $this->tree = array('command' => 'drop_table');
        $this->getToken();
        if ($this->token != 'identifier') {
          return $this->raiseError('Expected a table name');
        }
        $this->tree['table_names'][] = $this->lexer->tokenText;
        $this->getToken();
        if (($this->token == 'restrict') || ($this->token == 'cascade')) {
          $this->tree['drop_behavior'] = $this->token;
        }
        $this->getToken();
        if (!is_NULL($this->token)) {
          return $this->raiseError('Unexpected token');
        }
        return $this->tree;
        break;
      case 'index':
        $this->tree = array('command' => 'drop_index');
        break;
      case 'constraint':
        $this->tree = array('command' => 'drop_constraint');
        break;
      case 'sequence':
        $this->tree = array('command' => 'drop_sequence');
        break;
      default:
        return $this->raiseError('Unknown object to drop');
    }
    return $this->tree;
  }
    
  function parseSelect($subSelect = FALSE) {
    $this->tree = array('command' => 'select');

    $this->parseSelectFields();
    if ($this->token != 'from') {
      return $this->raiseError('Expected "from"');
    }

    $this->parseTables();

    while (!is_NULL($this->token) && (!$subSelect || $this->token != ')') && $this->token != ')') {
      switch ($this->token) {
        case 'where':
          $clause = $this->parseSearchClause();
          //if (PEAR::isError($clause)) {
          //return $clause;
          //}
          $this->tree['where_clause'] = $clause;
          break;
        case 'order':
          $this->getToken();
          if ($this->token != 'by') {
            return $this->raiseError('Expected "by"');
          }
          $this->getToken();
          while ($this->token == 'identifier') {
            $col = $this->lexer->tokenText;
            $this->getToken();
            if ($this->dialect->isSynonym($this->token)) {
              $order = $this->dialect->getSynonym($this->token);
              if (($order != 'asc') && ($order != 'desc')) {
                return $this->raiseError('Unexpected token');
              }
              $this->getToken();
            } 
            else {
              $order = 'asc';
            }
            if ($this->token == ',') {
              $this->getToken();
            }
            $this->tree['sort_order'][$col] = $order;
          }
          break;
        case 'limit':
          $this->getToken();
          if ($this->token != 'int_val') {
            return $this->raiseError('Expected an integer value');
          }
          $length = $this->lexer->tokenText;
          $start = 0;
          $this->getToken();
          if ($this->token == ',') {
            $this->getToken();
            if ($this->token != 'int_val') {
              return $this->raiseError('Expected an integer value');
            }
            $start = $length;
            $length = $this->lexer->tokenText;
            $this->getToken();
          }
          $this->tree['limit_clause'] = array('start' => $start,
            'length' => $length);
          break;
        case 'group':
          $this->getToken();
          if ($this->token != 'by') {
            return $this->raiseError('Expected "by"');
          }
          $this->getToken();
          while ($this->token == 'identifier') {
            $col = $this->lexer->tokenText;
            $this->getToken();
            if ($this->token == ',') {
              $this->getToken();
            }
            $this->tree['group_by'][] = $col;
          }
          break;
        case 'having':
          $clause = $this->parseSearchClause();
          $this->tree['having_clause'] = $clause;
          break;
        case ';':
          // @todo: handle multiple sql strings.
          $this->getToken();
          continue;
        default:
          return $this->raiseError('Unexpected clause');
      }
    }
    return $this->tree;
  }

  private function parseSelectFields() {
    $this->getToken();
    if (($this->token == 'distinct') || ($this->token == 'all')) {
      $this->tree['set_quantifier'] = $this->token;
      $this->getToken();
    }
    if ($this->token == '*') {
      $this->tree['column_names'][] = '*';
      $this->getToken();
    } 
    elseif ($this->token == 'identifier' || $this->dialect->isFunc($this->token)) {
      while ($this->token != 'from') {
        if ($this->token == 'identifier') {
          list($columnTable, $columnName) = explode(".", $this->lexer->tokenText);
          if (!$columnName) {
            $columnName = $columnTable;
            unset($columnTable);
          }

          $this->getToken();
          if ($this->token == 'as') {
            $this->getToken();
            if ($this->token == 'identifier' ) {
              $columnAlias = $this->lexer->tokenText;
            } 
            else {
              return $this->raiseError('Expected column alias');
            }
          } 
          elseif ($this->token == 'identifier') {
            $columnAlias = $this->lexer->tokenText;
          } 
          else {
            $columnAlias = '';
          }

          $this->tree['column_tables'][] = $columnTable;
          $this->tree['column_names'][] = $columnName;
          $this->tree['column_aliases'][] = $columnAlias;
          if ($this->token != 'from') {
            $this->getToken();
          }
          if ($this->token == ',') {
            $this->getToken();
          }
        }
        elseif ($this->dialect->isFunc($this->token)) {
          if (!isset($this->tree['set_quantifier'])) {
            $result = $this->parseFunctionOpts();
            //if (PEAR::isError($result)) {
            //return $result;
            //}
            $this->tree['column_functions'][] = $result;
            $this->getToken();

            if ($this->token == 'as') {
              $this->getToken();
              if ($this->token == 'identifier' ) {
                $columnAlias = $this->lexer->tokenText;
              } 
              else {
                return $this->raiseError('Expected column alias');
              }
            } 
            else {
              $columnAlias = '';
            }
          } 
          else {
            return $this->raiseError('Cannot use "' . $this->tree['set_quantifier'] . '" with ' . $this->token);
          }
        } 
        elseif ($this->token == ',') {
          $this->getToken();
        } 
        else {
          return $this->raiseError('Unexpected token "' . $this->token . '"');
        }
      }
    } 
    else {
      return $this->raiseError('Expected columns or a set function');
    }
  }

  /**
   * Parse query table reference clause lsit.
   *
   */
  function parseTables() {
    $this->getToken();
    while ($this->token == 'identifier') {
      $this->tree['table_names'][] = $this->lexer->tokenText;
      $this->getToken();
      if ($this->token == 'identifier') {
        $this->tree['table_aliases'][] = $this->lexer->tokenText;
        $this->getToken();
      } 
      elseif ($this->token == 'as') {
        $this->getToken();
        if ($this->token == 'identifier') {
          $this->tree['table_aliases'][] = $this->lexer->tokenText;
        } 
        else {
          return $this->raiseError('Expected table alias');
        }
        $this->getToken();
      } 
      else {
        $this->tree['table_aliases'][] = '';
      }
      if ($this->token == 'on') {
        $clause = $this->parseSearchClause();
        //if (PEAR::isError($clause)) {
        //return $clause;
        //}
        $this->tree['table_join_clause'][] = $clause;
      } 
      else if ($this->token == 'using') {
        // Advanced to first (.
        $this->getToken();
        if ($this->token != '(') {
          return $this->raiseError('Expected (');
        }

        // Get all join columns.
        $columns = array();
        do {
          $this->getToken();
          if ($this->token == 'identifier') {
            $columns[] = $this->lexer->tokenText;
          }
        }
        while ($this->token != ')');

        $this->tree['table_join_clause'][] = array(
          'type' => 'using',
          'value' => array(
            'columns' => $columns,
          ),
        );
        $this->getToken();
      }
      else {
        // @todo This is setting the join clause for the first table, which is unnecessary.
        $this->tree['table_join_clause'][] = '';
      }
      if ($this->token == ',') {
        $this->tree['table_join'][] = ',';
        $this->getToken();
      } 
      elseif ($this->token == 'join') {
        $this->tree['table_join'][] = 'join';
        $this->getToken();
      } 
      elseif (($this->token == 'cross') || ($this->token == 'inner')) {
        $join = strtolower($this->lexer->tokenText);
        $this->getToken();
        if ($this->token != 'join') {
          return $this->raiseError('Expected token "join"');
        }
        $this->tree['table_join'][] = $join . ' join';
        $this->getToken();
      } 
      elseif (($this->token == 'left') || ($this->token == 'right')) {
        $join = strtolower($this->lexer->tokenText);
        $this->getToken();
        if ($this->token == 'join') {
          $this->tree['table_join'][] = $join . ' join';
        } 
        elseif ($this->token == 'outer') {
          $join .= ' outer';
          $this->getToken();
          if ($this->token == 'join') {
            $this->tree['table_join'][] = $join . ' join';
          } 
          else {
            return $this->raiseError('Expected token "join"');
          }
        } 
        else {
          return $this->raiseError('Expected token "outer" or "join"');
        }
        $this->getToken();
      } 
      elseif ($this->token == 'natural') {
        $join = $this->lexer->tokenText;
        $this->getToken();
        if ($this->token == 'join') {
          $this->tree['table_join'][] = $join . ' join';
        } 
        elseif (($this->token == 'left') || ($this->token == 'right')) {
          $join .= ' ' . $this->token;
          $this->getToken();
          if ($this->token == 'join') {
            $this->tree['table_join'][] = $join . ' join';
          } 
          elseif ($this->token == 'outer') {
            $join .= ' ' . $this->token;
            $this->getToken();
            if ($this->token == 'join') {
              $this->tree['table_join'][] = $join . ' join';
            } 
            else {
              return $this->raiseError('Expected token "join" or "outer"');
            }
          } 
          else {
            return $this->raiseError('Expected token "join" or "outer"');
          }
        } 
        else {
          return $this->raiseError('Expected token "left", "right" or "join"');
        }
        $this->getToken();
      } 
      elseif (($this->token == 'where') || ($this->token == 'order') || ($this->token == 'limit') || (is_NULL($this->token))) {
        break;
      }
    }
  }
}

/**
 * SQL Parser Exception.
 */
class SqlParserException extends Exception { }
