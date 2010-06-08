<?php
// $Id$

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
 * @todo Table aliases are not parsing.
 * @todo Provide sane exception handling in raiseError().
 * @todo Parse multiple SQL strings.
 */
class SqlParser {
  var $lexer;
  var $token;
  var $dialect;

  function __construct($string = NULL) {
    $this->dialect = new SqlDialect();

    if (is_string($string)) {
      $this->lexer = new SqlLexer($string, 1);
      $this->lexer->symbols =& $this->dialect->symbols;
    }
  }

  /**
   * getParams 
   * 
   * @param mixed $values 
   * @param mixed $types 

   * @return void
   */
  function getParams(&$values, &$types) {
    $values = array();
    $types = array();
    while ($this->token != ')') {
      $this->getTok();
      if ($this->dialect->isVal($this->token) || ($this->token == 'ident')) {
        $values[] = $this->lexer->tokText;
        $types[] = $this->token;
      } 
      elseif ($this->token == ')') {
        return FALSE;
      } 
      else {
        return $this->raiseError('Expected a value');
      }
      $this->getTok();
      if (($this->token != ',') && ($this->token != ')')) {
        return $this->raiseError('Expected , or )');
      }
    }
  }

  /**
   * raiseError 
   * 
   * @param mixed $message 
   *
   * @return void
   */
  function raiseError($message) {
    $end = 0;
    if ($this->lexer->string != '') {
      while (($this->lexer->lineBegin+$end < $this->lexer->stringLen) && ($this->lexer->string{$this->lexer->lineBegin+$end} != "\n")) {
        ++$end;
      }
    }

    $message = 'Parse error: ' . $message . ' on line ' . ($this->lexer->lineNo + 1) . "\n";
    $message .= substr($this->lexer->string, $this->lexer->lineBegin, $end) . "\n";
    $length = is_NULL($this->token) ? 0 : strlen($this->lexer->tokText);
    $message .= str_repeat(' ', abs($this->lexer->tokPtr - $this->lexer->lineBegin - $length)) . "^";
    $message .= ' found: "' . $this->lexer->tokText . '"';
    drupal_set_message("<pre>" . $message . "</pre>");

    return FALSE; //PEAR::raiseError($message);
  }
    
  // getTok()
  function getTok() {
    $this->token = $this->lexer->lex();
    //echo $this->token."\t".$this->lexer->tokText."\n";
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
          $this->getTok();
          if ($this->token = 'ident') {
            $constraintName = $this->lexer->tokText;
            $namedConstraint = TRUE;
            $haveValue = FALSE;
          }
          else {
            return $this->raiseError('Expected a constraint name');
          }
          break;
        case 'default':
          $this->getTok();
          if ($this->dialect->isVal($this->token)) {
            $constraintOpts = array(
              'type' => 'default_value',
              'value' => $this->lexer->tokText,
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
          $this->getTok();
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
          $this->getTok();
          if ($this->token == 'NULL') {
            $constraintOpts = array('type' => 'not_NULL',
              'value' => TRUE);
          }
          else {
            return $this->raiseError('Expected "NULL"');
          }
          break;
        case 'check':
          $this->getTok();
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
          $this->getTok();
          if ($this->token != '(') {
            return $this->raiseError('Expected (');
          }
          $constraintOpts = array('type' => 'unique');
          $this->getTok();
          while ($this->token != ')') {
            if ($this->token != 'ident') {
              return $this->raiseError('Expected an identifier');
            }
            $constraintOpts['column_names'][] = $this->lexer->tokText;
            $this->getTok();
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
              $this->getTok();
              if ($this->token == 'to') {
                $this->getTok();
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
          return $this->raiseError('Unexpected token ' . $this->lexer->tokText);
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
      $this->getTok();
    }
    return $options;
  }
    
  // parseSearchClause()
  function parseSearchClause($subSearch = FALSE) {
    $clause = array();
    // parse the first argument
    $this->getTok();
    if ($this->token == 'not') {
      $clause['neg'] = TRUE;
      $this->getTok();
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
      $clause['arg_1']['value'] = $this->lexer->tokText;
      $clause['arg_1']['type'] = $this->token;
    }

    // parse the operator
    if (!$foundSubclause) {
      $this->getTok();
      if (!$this->dialect->isOperator($this->token)) {
        return $this->raiseError('Expected an operator');
      }
      $clause['op'] = $this->token;

      $this->getTok();
      switch ($clause['op']) {
        case 'is':
          // parse for 'is' operator
          if ($this->token == 'not') {
            $clause['neg'] = TRUE;
            $this->getTok();
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
          $this->getTok();
        case 'in':
          // parse for 'in' operator 
          if ($this->token != '(') {
            return $this->raiseError('Expected "("');
          }

          // read the subset
          $this->getTok();
          // is this a subselect?
          if ($this->token == 'select') {
            $clause['arg_2']['value'] = $this->parseSelect(TRUE);
            $clause['arg_2']['type'] = 'command';
          } 
          elseif ($this->token == 'placeholder') {
            $clause['arg_2']['value'] = $this->lexer->tokText;
            $clause['arg_2']['type'] = $this->token;
            $this->getTok();
          }
          else {
            $this->lexer->pushBack();
            // parse the set
            $result = $this->getParams($clause['arg_2']['value'], $clause['arg_2']['type']);
            //if (PEAR::isError($result)) {
            //return $result;
            //}
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
            $this->getTok();
            if ($this->token != ')') {
              return $this->raiseError('Expected ")"');
            }
          } 
          else {
            $clause['arg_2']['value'] = $this->lexer->tokText;
            $clause['arg_2']['type'] = $this->token;
          }
          break;
      }
    }

    $this->getTok();
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
    $this->getTok();
    if ($this->token != '(') {
      return $this->raiseError('Expected (');
    }

    $fields = array();
    while (1) {
      // parse field identifier
      $this->getTok();
      if ($this->token == 'ident') {
        $name = $this->lexer->tokText;
      } 
      elseif ($this->token == ')') {
        return $fields;
      } 
      else {
        return $this->raiseError('Expected identifier');
      }

      // parse field type
      $this->getTok();
      if ($this->dialect->isType($this->token)) {
        $type = $this->token;
      } 
      else {
        return $this->raiseError('Expected a valid type');
      }

      $this->getTok();
      // handle special case two-word types
      if ($this->token == 'precision') {
        // double precision == double
        if ($type == 'double') {
          return $this->raiseError('Unexpected token');
        }
        $this->getTok();
      } 
      elseif ($this->token == 'varying') {
        // character varying() == varchar()
        if ($type == 'character') {
          $type == 'varchar';
          $this->getTok();
        } 
        else {
          return $this->raiseError('Unexpected token');
        }
      }
      $fields[$name]['type'] = $this->dialect->getSynonym($type);
      // parse type parameters
      if ($this->token == '(') {
        $results = $this->getParams($values, $types);
        //if (PEAR::isError($results)) {
        //return $results;
        //}
        switch ($fields[$name]['type']) {
          case 'numeric':
            if (isset($values[1])) {
              if ($types[1] != 'int_val') {
                return $this->raiseError('Expected an integer');
              }
              $fields[$name]['decimals'] = $values[1];
            }
          case 'float':
            if ($types[0] != 'int_val') {
              return $this->raiseError('Expected an integer');
            }
            $fields[$name]['length'] = $values[0];
            break;
          case 'char': 
          case 'varchar':
          case 'integer': 
          case 'int':
            if (sizeof($values) != 1) {
              return $this->raiseError('Expected 1 parameter');
            }
            if ($types[0] != 'int_val') {
              return $this->raiseError('Expected an integer');
            }
            $fields[$name]['length'] = $values[0];
            break;
          case 'set': 
          case 'enum':
            if (!sizeof($values)) {
              return $this->raiseError('Expected a domain');
            }
            $fields[$name]['domain'] = $values;
            break;
          default:
            if (sizeof($values)) {
              return $this->raiseError('Unexpected )');
            }
        }
        $this->getTok();
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
    $this->getTok();
    if ($this->token != '(') {
      return $this->raiseError('Expected "("');
    }
    switch ($function) {
      case 'count':
        $this->getTok();
        switch ($this->token) {
          case 'distinct':
            $opts['distinct'] = TRUE;
            $this->getTok();
            if ($this->token != 'ident') {
              return $this->raiseError('Expected a column name');
            }
          case 'ident': 
          case '*':
            $opts['arg'][] = $this->lexer->tokText;
            break;
          default:
            return $this->raiseError('Invalid argument');
        }
        break;
      case 'concat':
        $this->getTok();
        while ($this->token != ')') {
          switch ($this->token) {
            case 'ident': 
              $opts['arg'][] = $this->lexer->tokText;
              break;
            case 'text_val':
              $opts['arg'][] = "'" .  $this->lexer->tokText . "'";
              break;
            case ',':
              // do nothing
              break;
            default:
              return $this->raiseError('Expected a string or a column name');
          }
          $this->getTok();
        }
        $this->lexer->pushBack();
        break;
      case 'avg': 
      case 'min': 
      case 'max': 
      case 'sum':
      default:
        $this->getTok();
        $opts['arg'] = $this->lexer->tokText;
        break;
    }
    $this->getTok();
    if ($this->token != ')') {
      return $this->raiseError('Expected ")"');
    }

    // check for an alias, from, or operator
    $this->getTok();
    if ($this->token == ',' || $this->token == 'from' || $this->dialect->isOperator($this->token)) {
      $this->lexer->pushBack();
    } 
    elseif ($this->token == 'as') {
      $this->getTok();
      if ($this->token == 'ident' ) {
        $opts['alias'] = $this->lexer->tokText;
      } 
      else {
        return $this->raiseError('Expected column alias');
      }
    } 
    else {
      if ($this->token == 'ident' ) {
        $opts['alias'] = $this->lexer->tokText;
      } 
      else {
        return $this->raiseError('Expected column alias, from or comma');
      }
    }
    return $opts;
  }
    
  // parseCreate()
  function parseCreate() {
    $this->getTok();
    switch ($this->token) {
      case 'table':
        $tree = array('command' => 'create_table');
        $this->getTok();
        if ($this->token == 'ident') {
          $tree['table_names'][] = $this->lexer->tokText;
          $fields = $this->parseFieldList();
          //if (PEAR::isError($fields)) {
          //return $fields;
          //}
          $tree['column_defs'] = $fields;
          //                    $tree['column_names'] = array_keys($fields);
        } 
        else {
          return $this->raiseError('Expected table name');
        }
        break;
      case 'index':
        $tree = array('command' => 'create_index');
        break;
      case 'constraint':
        $tree = array('command' => 'create_constraint');
        break;
      case 'sequence':
        $tree = array('command' => 'create_sequence');
        break;
      default:
        return $this->raiseError('Unknown object to create');
    }
    return $tree;
  }
    
  // parseInsert()
  function parseInsert() {
    $this->getTok();
    if ($this->token == 'into') {
      $tree = array('command' => 'insert');
      $this->getTok();
      if ($this->token == 'ident') {
        $tree['table_names'][] = $this->lexer->tokText;
        $this->getTok();
      } 
      else {
        return $this->raiseError('Expected table name');
      }
      if ($this->token == '(') {
        $results = $this->getParams($values, $types);
        //if (PEAR::isError($results)) {
        //return $results;
        //} else {
        if (sizeof($values)) {
          $tree['column_names'] = $values;
        }
        //}
        $this->getTok();
      }
      if ($this->token == 'values') {
        $this->getTok();
        $results = $this->getParams($values, $types);
        //if (PEAR::isError($results)) {
        //return $results;
        //} else {
        if (isset($tree['column_defs']) && (sizeof($tree['column_defs']) != sizeof($values))) {
          return $this->raiseError('field/value mismatch');
        }
        if (sizeof($values)) {
          foreach ($values as $key => $value) {
            $values[$key] = array('value' => $value,
              'type' => $types[$key]);
          }
          $tree['values'] = $values;
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
    return $tree;
  }
    
  // parseUpdate()
  function parseUpdate() {
    $this->getTok();
    if ($this->token == 'ident') {
      $tree = array('command' => 'update');
      $tree['table_names'][] = $this->lexer->tokText;
    } 
    else {
      return $this->raiseError('Expected table name');
    }
    $this->getTok();
    if ($this->token != 'set') {
      return $this->raiseError('Expected "set"');
    }
    while (TRUE) {
      $this->getTok();
      if ($this->token != 'ident') {
        return $this->raiseError('Expected a column name');
      }
      $tree['column_names'][] = $this->lexer->tokText;
      $this->getTok();
      if ($this->token != '=') {
        return $this->raiseError('Expected =');
      }
      $this->getTok();
      if (!$this->dialect->isVal($this->token)) {
        return $this->raiseError('Expected a value');
      }
      $tree['values'][] = array('value' => $this->lexer->tokText,
        'type' => $this->token);
      $this->getTok();
      if ($this->token == 'where') {
        $clause = $this->parseSearchClause();
        //if (PEAR::isError($clause)) {
        //return $clause;
        //}
        $tree['where_clause'] = $clause;
        break;
      } 
      elseif ($this->token != ',') {
        return $this->raiseError('Expected "where" or ","');
      }
    }
    return $tree;
  }
    
  // parseDelete()
  function parseDelete() {
    $this->getTok();
    if ($this->token != 'from') {
      return $this->raiseError('Expected "from"');
    }
    $tree = array('command' => 'delete');
    $this->getTok();
    if ($this->token != 'ident') {
      return $this->raiseError('Expected a table name');
    }
    $tree['table_names'][] = $this->lexer->tokText;
    $this->getTok();
    if ($this->token != 'where') {
      return $this->raiseError('Expected "where"');
    }
    $clause = $this->parseSearchClause();
    //if (PEAR::isError($clause)) {
    //return $clause;
    //}
    $tree['where_clause'] = $clause;
    return $tree;
  }
    
  // parseDrop()
  function parseDrop() {
    $this->getTok();
    switch ($this->token) {
      case 'table':
        $tree = array('command' => 'drop_table');
        $this->getTok();
        if ($this->token != 'ident') {
          return $this->raiseError('Expected a table name');
        }
        $tree['table_names'][] = $this->lexer->tokText;
        $this->getTok();
        if (($this->token == 'restrict') || ($this->token == 'cascade')) {
          $tree['drop_behavior'] = $this->token;
        }
        $this->getTok();
        if (!is_NULL($this->token)) {
          return $this->raiseError('Unexpected token');
        }
        return $tree;
        break;
      case 'index':
        $tree = array('command' => 'drop_index');
        break;
      case 'constraint':
        $tree = array('command' => 'drop_constraint');
        break;
      case 'sequence':
        $tree = array('command' => 'drop_sequence');
        break;
      default:
        return $this->raiseError('Unknown object to drop');
    }
    return $tree;
  }
    
  // parseSelect()
  function parseSelect($subSelect = FALSE) {
    $tree = array('command' => 'select');
    $this->getTok();
    if (($this->token == 'distinct') || ($this->token == 'all')) {
      $tree['set_quantifier'] = $this->token;
      $this->getTok();
    }
    if ($this->token == '*') {
      $tree['column_names'][] = '*';
      $this->getTok();
    } 
    elseif ($this->token == 'ident' || $this->dialect->isFunc($this->token)) {
      while ($this->token != 'from') {
        if ($this->token == 'ident') {
          list($columnTable, $columnName) = explode(".", $this->lexer->tokText);
          if (!$columnName) {
            $columnName = $columnTable;
            unset($columnTable);
          }

          $this->getTok();
          if ($this->token == 'as') {
            $this->getTok();
            if ($this->token == 'ident' ) {
              $columnAlias = $this->lexer->tokText;
            } 
            else {
              return $this->raiseError('Expected column alias');
            }
          } 
          elseif ($this->token == 'ident') {
            $columnAlias = $this->lexer->tokText;
          } 
          else {
            $columnAlias = '';
          }

          $tree['column_tables'][] = $columnTable;
          $tree['column_names'][] = $columnName;
          $tree['column_aliases'][] = $columnAlias;
          if ($this->token != 'from') {
            $this->getTok();
          }
          if ($this->token == ',') {
            $this->getTok();
          }
        }
        elseif ($this->dialect->isFunc($this->token)) {
          if (!isset($tree['set_quantifier'])) {
            $result = $this->parseFunctionOpts();
            //if (PEAR::isError($result)) {
            //return $result;
            //}
            $tree['column_functions'][] = $result;
            $this->getTok();

            if ($this->token == 'as') {
              $this->getTok();
              if ($this->token == 'ident' ) {
                $columnAlias = $this->lexer->tokText;
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
            return $this->raiseError('Cannot use "' . $tree['set_quantifier'] . '" with ' . $this->token);
          }
        } 
        elseif ($this->token == ',') {
          $this->getTok();
        } 
        else {
          return $this->raiseError('Unexpected token "' . $this->token . '"');
        }
      }
    } 
    else {
      return $this->raiseError('Expected columns or a set function');
    }
    if ($this->token != 'from') {
      return $this->raiseError('Expected "from"');
    }
    $this->getTok();
    while ($this->token == 'ident') {
      $tree['table_names'][] = $this->lexer->tokText;
      $this->getTok();
      if ($this->token == 'ident') {
        $tree['table_aliases'][] = $this->lexer->tokText;
        $this->getTok();
      } 
      elseif ($this->token == 'as') {
        $this->getTok();
        if ($this->token == 'ident') {
          $tree['table_aliases'][] = $this->lexer->tokText;
        } 
        else {
          return $this->raiseError('Expected table alias');
        }
        $this->getTok();
      } 
      else {
        $tree['table_aliases'][] = '';
      }
      if ($this->token == 'on') {
        $clause = $this->parseSearchClause();
        //if (PEAR::isError($clause)) {
        //return $clause;
        //}
        $tree['table_join_clause'][] = $clause;
      } 
      else if ($this->token == 'using') {
        // Advanced to first (.
        $this->getTok();
        if ($this->token != '(') {
          return $this->raiseError('Expected (');
        }

        // Get all join columns.
        $columns = array();
        do {
          $this->getTok();
          if ($this->token == 'ident') {
            $columns[] = $this->lexer->tokText;
          }
        }
        while ($this->token != ')');

        $tree['table_join_clause'][] = array(
          'type' => 'using',
          'value' => array(
            'columns' => $columns,
          ),
        );
      }
      else {
        // @todo This is setting the join clause for the first table, which is unnecessary.
        $tree['table_join_clause'][] = '';
      }
      if ($this->token == ',') {
        $tree['table_join'][] = ',';
        $this->getTok();
      } 
      elseif ($this->token == 'join') {
        $tree['table_join'][] = 'join';
        $this->getTok();
      } 
      elseif (($this->token == 'cross') || ($this->token == 'inner')) {
        $join = strtolower($this->lexer->tokText);
        $this->getTok();
        if ($this->token != 'join') {
          return $this->raiseError('Expected token "join"');
        }
        $tree['table_join'][] = $join . ' join';
        $this->getTok();
      } 
      elseif (($this->token == 'left') || ($this->token == 'right')) {
        $join = $this->lexer->tokText;
        $this->getTok();
        if ($this->token == 'join') {
          $tree['table_join'][] = $join . ' join';
        } 
        elseif ($this->token == 'outer') {
          $join .= ' outer';
          $this->getTok();
          if ($this->token == 'join') {
            $tree['table_join'][] = $join . ' join';
          } 
          else {
            return $this->raiseError('Expected token "join"');
          }
        } 
        else {
          return $this->raiseError('Expected token "outer" or "join"');
        }
        $this->getTok();
      } 
      elseif ($this->token == 'natural') {
        $join = $this->lexer->tokText;
        $this->getTok();
        if ($this->token == 'join') {
          $tree['table_join'][] = $join . ' join';
        } 
        elseif (($this->token == 'left') || ($this->token == 'right')) {
          $join .= ' ' . $this->token;
          $this->getTok();
          if ($this->token == 'join') {
            $tree['table_join'][] = $join . ' join';
          } 
          elseif ($this->token == 'outer') {
            $join .= ' ' . $this->token;
            $this->getTok();
            if ($this->token == 'join') {
              $tree['table_join'][] = $join . ' join';
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
        $this->getTok();
      } 
      elseif (($this->token == 'where') || ($this->token == 'order') || ($this->token == 'limit') || (is_NULL($this->token))) {
        break;
      }
    }
    while (!is_NULL($this->token) && (!$subSelect || $this->token != ')') && $this->token != ')') {
      switch ($this->token) {
        case 'where':
          $clause = $this->parseSearchClause();
          //if (PEAR::isError($clause)) {
          //return $clause;
          //}
          $tree['where_clause'] = $clause;
          break;
        case 'order':
          $this->getTok();
          if ($this->token != 'by') {
            return $this->raiseError('Expected "by"');
          }
          $this->getTok();
          while ($this->token == 'ident') {
            $col = $this->lexer->tokText;
            $this->getTok();
            if ($this->dialect->isSynonym($this->token)) {
              $order = $this->dialect->getSynonym($this->token);
              if (($order != 'asc') && ($order != 'desc')) {
                return $this->raiseError('Unexpected token');
              }
              $this->getTok();
            } 
            else {
              $order = 'asc';
            }
            if ($this->token == ',') {
              $this->getTok();
            }
            $tree['sort_order'][$col] = $order;
          }
          break;
        case 'limit':
          $this->getTok();
          if ($this->token != 'int_val') {
            return $this->raiseError('Expected an integer value');
          }
          $length = $this->lexer->tokText;
          $start = 0;
          $this->getTok();
          if ($this->token == ',') {
            $this->getTok();
            if ($this->token != 'int_val') {
              return $this->raiseError('Expected an integer value');
            }
            $start = $length;
            $length = $this->lexer->tokText;
            $this->getTok();
          }
          $tree['limit_clause'] = array('start' => $start,
            'length' => $length);
          break;
        case 'group':
          $this->getTok();
          if ($this->token != 'by') {
            return $this->raiseError('Expected "by"');
          }
          $this->getTok();
          while ($this->token == 'ident') {
            $col = $this->lexer->tokText;
            $this->getTok();
            if ($this->token == ',') {
              $this->getTok();
            }
            $tree['group_by'][] = $col;
          }
          break;
        case 'having':
          $clause = $this->parseSearchClause();
          $tree['having_clause'] = $clause;
          break;
        case ';':
          // @todo: handle multiple sql strings.
          $this->getTok();
          continue;
        default:
          return $this->raiseError('Unexpected clause');
      }
    }
    return $tree;
  }
    
  // parse($string)
  function parse($string = NULL) {
    if (is_string($string)) {
      // Initialize the Lexer with a 3-level look-back buffer
      $this->lexer = new SqlLexer($string, 3);
      $this->lexer->symbols =& $this->dialect->symbols;
    } 
    else {
      if (!is_object($this->lexer)) {
        return $this->raiseError('No initial string specified');
      }
    }

    // get query action
    $this->getTok();
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
}

/**
 * SQL Parser Exception.
 */
class SqlParserException extends Exception { }
