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
  protected $lexer;

  /**
   * The SQL dialect object for the parser.
   *
   * @var SqlDialect
   */
  protected $dialect;

  /**
   * The current parsed token.
   *
   * @var string
   */
  protected $token;

  /**
   * The sql object.
   *
   * @var SqlObject
   */
  protected $sql_object;

  /**
   * The number of placeholders in the parsed sql.
   *
   * @var int
   */
  protected $placeholders;

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
    }

    if ($this->sql_string) {
      // Initialize the Lexer with a 3-level look-back buffer.
      $this->lexer = new SqlLexer($this->sql_string, 3);
      $this->lexer->symbols = & $this->dialect->symbols;
    }
    else {
      return $this->raiseError('No initial string specified');
    }

    // Get query action.
    $this->getToken();
    switch ($this->token) {
      case NULL:
        // NULL == end of string
        return $this->raiseError('Nothing to do');

      case 'select':
        $sql_object = $this->parseSelect();
        break;

      case 'update':
        $sql_object = $this->parseUpdate();
        break;

      case 'insert':
        $sql_object = $this->parseInsert();
        break;

      case 'delete':
        $sql_object = $this->parseDelete();
        break;

      case 'create':
        $sql_object = $this->parseCreate();
        break;

      case 'drop':
        $sql_object = $this->parseDrop();
        break;

      default:
        return $this->raiseError('Unknown action :' . $this->token);
        break;
    }
    $sql_object->placeholders = $this->placeholders;
    return $sql_object;
  }

  /**
   * Parses a SQL select statement.
   *
   * @param $subSelect
   *   If TRUE then statement is treated as a subselect.
   */
  protected function parseSelect($subSelect = FALSE) {
    $sql_object = new SqlSelect();

    $this->parseSelectFields($sql_object);

    if ($this->token != 'from') {
      return $this->raiseError('Expected "from"');
    }

    $this->parseTables($sql_object);

    while (!is_null($this->token) && (!$subSelect || $this->token != ')') && $this->token != ')') {
      switch ($this->token) {
        case 'where':
          $conditional = $this->parseSearchClause();
          $sql_object->setConditional($conditional);
          break;

        case 'order':
          $this->parseOrderBy($sql_object);
          break;

        case 'limit':
          $this->parseLimit($sql_object);
          break;

        case 'group':
          $group_by = &$sql_object->addGroupBy();
          $this->getToken();
          if ($this->token != 'by') {
            return $this->raiseError('Expected "by"');
          }
          $this->getToken();
          while ($this->token == 'identifier') {
            $group_by->addColumn($this->lexer->tokenText);
            $this->getToken();
            if ($this->token == ',') {
              $this->getToken();
            }
          }
          break;

        case 'having':
          $having_clause = &$sql_object->addHaving();
          $this->parseSearchClause($having_clause);
          break;

        case ';':
          // @todo: handle multiple sql strings.
          $this->getToken();
          continue;

        default:
          return $this->raiseError('Unexpected clause');
      }
    }
    return $sql_object;
  }

  /**
   * Parses a SQL update statement.
   */
  function parseUpdate() {
    $sql_object = new SqlUpdate();
    $this->getToken();
    if ($this->token == 'identifier') {
      $sql_object->addTable($this->lexer->tokenText);
    }
    else {
      return $this->raiseError('Expected table name');
    }

    $this->parseSetFields($sql_object);

    while (!is_null($this->token) && (!$subSelect || $this->token != ')') && $this->token != ')') {
      switch ($this->token) {
        case 'where':
          $conditional = $this->parseSearchClause();
          $sql_object->setConditional($conditional);
          break;

        case 'order':
          $this->parseOrderBy($sql_object);
          break;

        case 'limit':
          $this->parseLimit($sql_object);
          break;

        case ';':
          // @todo: handle multiple sql strings.
          $this->getToken();
          continue;

        default:
          return $this->raiseError('Unexpected clause');
      }
    }
    return $sql_object;
  }

  /**
   * Parses a SQL insert statement.
   */
  function parseInsert() {
    $sql_object = new SqlInsert();

    $this->getToken();
    if ($this->token == 'into') {
      $this->getToken();

      // Parse table name.
      if ($this->token == 'identifier') {
        $sql_object->addTable($this->lexer->tokenText);
        $this->getToken();
      }
      else {
        return $this->raiseError('Expected table name');
      }

      // Parse field list.
      if ($this->token == '(') {
        $columns = array();
        while ($this->token != ')') {
          $this->getToken();
          if ($this->dialect->isVal($this->token) || ($this->token == 'identifier')) {
            $columns[] = new SqlColumn($this->lexer->tokenText);
          }
          else {
            return $this->raiseError('Expected a value');
          }

          $this->getToken();
          if (($this->token != ',') && ($this->token != ')')) {
            return $this->raiseError('Expected , or )');
          }
        }
        $this->getToken();
      }

      // Parse field values.
      if ($this->token == 'values') {
        $values = array();
        $this->getToken();
        while ($this->token != ')') {
          $this->getToken();
          if ($this->dialect->isVal($this->token) || ($this->token == 'identifier')) {
            $i = count($values);
            $columns[$i]->setValue($this->lexer->tokenText);
            $columns[$i]->setType($this->token);
            $values[] = $this->lexer->tokenText;
          }
          else {
            return $this->raiseError('Expected a value');
          }

          $this->getToken();
          if (($this->token != ',') && ($this->token != ')')) {
            return $this->raiseError('Expected , or )');
          }
        }

        if (count($columns) > 0 && (count($columns) != count($values))) {
          return $this->raiseError('field/value mismatch');
        }
        if (count($values) == 0) {
          return $this->raiseError('No fields to insert');
        }

        foreach ($columns as $column) {
          $sql_object->addColumn($column);
        }
      }
      else {
        return $this->raiseError('Expected "values"');
      }
    }
    else {
      return $this->raiseError('Expected "into"');
    }
    return $sql_object;
  }

  /**
   * Parses a SQL delete statement.
   */
  function parseDelete() {
    $this->sql_object = new SqlDelete();
    $this->getToken();
    if ($this->token != 'from') {
      return $this->raiseError('Expected "from"');
    }
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
    $this->tree['where_clause'] = $clause;
    return $this->tree;
  }

  /**
   * Parses a SQL create statement.
   *
   * @todo SqlCreate object doesn't actually exist.
   */
  protected function parseCreate() {
    $this->sql_object = new SqlCreate();
    $this->getToken();
    switch ($this->token) {
      case 'table':
        $this->tree = array('command' => 'create_table');
        $this->getToken();
        if ($this->token == 'identifier') {
          $this->tree['table_names'][] = $this->lexer->tokenText;
          $fields = $this->parseFieldList();
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

  /**
   * Parses a SQL drop statement.
   *
   * @todo SqlDrop object doesn't actually exists.
   */
  protected function parseDrop() {
    $this->sql_object = new SqlDrop();
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
        if (!is_null($this->token)) {
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

  /**
   * Get the parameters of an SQL function or list set.
   *
   * @return
   *   An array containing the tokens and token types of all tokens within
   *   the SQL function or list set.
   */
  protected function getParameters() {
    $parameters = array();
    while ($this->token != ')') {
      $this->getToken();
      if ($this->dialect->isVal($this->token) || ($this->token == 'identifier')) {
        $field = new &SqlField($this->lexer->tokenText);
        $field->setType = $this->token;
        $parameters[] = $field;
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
   * Throw a well formatted error.
   *
   * @param $message
   */
  protected function raiseError($message) {
    $end = 0;
    if ($this->lexer->string != '') {
      while (($this->lexer->lineBegin + $end < $this->lexer->stringLength) && ($this->lexer->string{$this->lexer->lineBegin + $end} != "\n")) {
        ++$end;
      }
    }

    $message = 'Parse error: ' . $message . ' on line ' . ($this->lexer->lineNumber + 1) . "\n";
    $message .= substr($this->lexer->string, $this->lexer->lineBegin, $end) . "\n";
    $length = is_null($this->token) ? 0 : strlen($this->lexer->tokenText);
    $message .= str_repeat(' ', abs($this->lexer->tokenPointer - $this->lexer->lineBegin - $length)) . "^";
    $message .= ' found: "' . $this->lexer->tokenText . '"';
    drupal_set_message("<pre>" . $message . "</pre>");

    return FALSE;
  }

  /**
   * Get the next token from SqlLexer:lex().
   */
  function getToken() {
    $this->token = $this->lexer->lex();

    // If the token is a placeholder, increase the placeholder count.
    if ($this->token == 'placeholder') {
      $this->placeholders++;
    }
  }

  /**
   * Parses the fields from a SET clause
   *
   * @param SqlObject $sql_object
   */
  protected function parseSetFields(SqlObject $sql_object) {
    $this->getToken();
    if ($this->token != 'set') {
      return $this->raiseError('Expected "set"');
    }

    while (TRUE) {
      $this->getToken();
      if ($this->token != 'identifier') {
        return $this->raiseError('Expected a column name');
      }
      $column = &$sql_object->addColumn($this->lexer->tokenText);
      $this->getToken();
      if ($this->token != '=') {
        return $this->raiseError('Expected =');
      }
      $this->getToken();
      if (!$this->dialect->isVal($this->token)) {
        return $this->raiseError('Expected a value');
      }
      $column->setValue($this->lexer->tokenText);
      $column->setType($this->token);

      $this->getToken();
      if ($this->token == 'where' || $this->token == 'order' || $this->token == 'limit') {
        break;
      }
      elseif ($this->token != ',') {
        return $this->raiseError('Expected "where", "order", "limit" or ","');
      }
    }
  }

  /**
   * Parses the where clause of an SQL statement.
   *
   * @param SqlConditional $sql_conditional
   */
  protected function parseSearchClause(SqlConditional &$sql_conditional = NULL) {
    if (is_null($sql_conditional)) {
      $sql_conditional = new SqlConditional();
    }

    // parse the first argument
    $this->getToken();
    if ($this->token == 'not') {
      $sql_conditional->setNot();
      $this->getToken();
    }

    $foundSubclause = FALSE;
    if ($this->token == '(') {
      // Argument is a subclause.
      $sql_conditional->setArg1($this->parseSearchClause());
      if ($this->token != ')') {
        return $this->raiseError('Expected ")"');
      }
      $foundSubclause = TRUE;
    }
    elseif ($this->dialect->isFunc($this->token)) {
      // Argument is a funciton.
      $sql_conditional->setArg1($this->parseFunction());
    }
    elseif ($this->dialect->isReserved($this->token)) {
      return $this->raiseError('Expected a column name or value');
    }
    else {
      // Argument is a field.
      list($columnTable, $columnName) = explode(".", $this->lexer->tokenText);
      if (!$columnName) {
        $columnName = $columnTable;
        unset($columnTable);
      }
      $field = new SqlField($columnName);
      $field->setTable($columnTable);
      $field->setType($this->token);

      $sql_conditional->setArg1($field);
    }

    // Parse the operator.
    if (!$foundSubclause) {
      $this->getToken();
      if (!$this->dialect->isOperator($this->token)) {
        return $this->raiseError('Expected an operator');
      }

      $sql_conditional->setOperator($this->token);

      $this->getToken();
      switch ($sql_conditional->operator) {
        case 'is':
          // parse for 'is' operator
          if ($this->token == 'not') {
            $sql_conditional->setNot();
            $this->getToken();
          }
          if ($this->token != 'null') {
            return $this->raiseError('Expected "NULL"');
          }
          $field = new SqlField(NULL);
          $field->setType($this->token);
          $sql_conditional->setArg2($field);
          break;
        case 'not':
          // Parse for 'not in' operator.
          if ($this->token != 'in') {
            return $this->raiseError('Expected "in"');
          }
          $sql_conditional->setOperator($this->token);
          $sql_conditional->setNot();
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
            $sql_conditional->setArg2($this->parseSelect(TRUE));
          }
          elseif ($this->token == 'placeholder') {
            $field = new SqlField($this->lexer->tokenText);
            $field->setType($this->token);
            $sql_conditional->setArg2($field);
            $this->getToken();
          }
          else {
            $this->lexer->pushBack();
            // Parse the set.
            $sql_conditional->setArg2($this->getParameters());
          }
          if ($this->token != ')') {
            return $this->raiseError('Expected ")"');
          }
          break;
        case 'and':
        case 'or':
        case 'xor':
          $this->lexer->unget();
          break;
        default:
          // Parse for in-fix binary operators.
          if ($this->dialect->isReserved($this->token)) {
            return $this->raiseError('Expected a column name or value');
          }
          if ($this->token == '(') {
            $sql_conditional->arg2($this->parseSearchClause());
            $this->getToken();
            if ($this->token != ')') {
              return $this->raiseError('Expected ")"');
            }
          }
          else {
            list($columnTable, $columnName) = explode(".", $this->lexer->tokenText);
            if (!$columnName) {
              $columnName = $columnTable;
              unset($columnTable);
            }
            $field = new SqlField($columnName);
            $field->setTable($columnTable);
            $field->setType($this->token);

            $sql_conditional->setArg2($field);
          }
          break;
      }
    }

    // Check if there is another conditional to parse.
    $this->getToken();
    if (($this->token == 'and') || ($this->token == 'or') || $this->token == 'xor') {
      $inner_conditional = $sql_conditional;
      $sql_conditional = new SqlConditional();
      $sql_conditional->setOperator($this->token);
      $sql_conditional->setArg1($inner_conditional);
      $sql_conditional->setArg2($this->parseSearchClause());
    }
    else {
      $this->lexer->unget();
    }
    return $sql_conditional;
  }

  /**
   * Parses field list for create statement.
   */
  protected function parseFieldList() {
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
            if (count($parameters['values']) != 1) {
              return $this->raiseError('Expected 1 parameter');
            }
            if ($parameters['types'][0] != 'int_val') {
              return $this->raiseError('Expected an integer');
            }
            $fields[$name]['length'] = $parameters['values'][0];
            break;
          case 'set':
          case 'enum':
            if (!count($parameters['values'])) {
              return $this->raiseError('Expected a domain');
            }
            $fields[$name]['domain'] = $parameters['values'];
            break;
          default:
            if (count($parameters['values'])) {
              return $this->raiseError('Unexpected )');
            }
        }
        $this->getToken();
      }

      $options = $this->parseFieldOptions();

      $fields[$name] += $options;

      if ($this->token == ')') {
        return $fields;
      }
      elseif (is_null($this->token)) {
        return $this->raiseerror('expected )');
      }
    }
  }

  /**
   * Parses field options for create statement.
   */
  protected function parseFieldOptions() {
    // parse field options
    $namedconstraint = false;
    $options = array();
    while (($this->token != ',') && ($this->token != ')') && ($this->token != nULL)) {
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
            $results = $this->parseFunctionArguments();
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
          if ($this->token == 'null') {
            $constraintOpts = array('type' => 'not_null',
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
        case 'null':
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

  /**
   * Parses function.
   *
   * @return SqlFunction
   *
   */
  protected function parseFunction() {
    $function = new SqlFunction($this->token);

    $this->getToken();
    if ($this->token != '(') {
      return $this->raiseError('Expected "("');
    }

    switch ($function->name) {
      case 'count':
        $this->getToken();
        switch ($this->token) {
          case 'distinct':
            $function->setDistinct();
            $this->getToken();
            if ($this->token != 'identifier') {
              return $this->raiseError('Expected a column name');
            }
          case 'identifier':
          case '*':
            $function->addArgument($this->lexer->tokenText);
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
            case 'text_val':
              $function->addArgument($this->lexer->tokenText);
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
        $function->addArgument($this->lexer->tokenText);
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
        $function->setAlias($this->lexer->tokenText);
      }
      else {
        return $this->raiseError('Expected column alias');
      }
    }
    else {
      if ($this->token == 'identifier' ) {
        $function->setAlias($this->lexer->tokenText);
      }
      else {
        return $this->raiseError('Expected column alias, from or comma');
      }
    }
    return $function;
  }

  /**
   * Parses the select expression.
   *
   * @param SqlObject $sql_object
   */
  protected function parseSelectFields(SqlObject &$sql_object) {
    $this->getToken();
    if (($this->token == 'distinct') || ($this->token == 'all')) {
      $sql_object->setSetIdentifier($this->token);
      $this->getToken();
    }
    if ($this->token == '*') {
      $sql_object->addField($this->token);
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
          $column = &$sql_object->addField($columnName);
          $column->setTable($columnTable);

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

          $column->setAlias($columnAlias);

          if ($this->token != 'from') {
            $this->getToken();
          }
          if ($this->token == ',') {
            $this->getToken();
          }
        }
        elseif ($this->dialect->isFunc($this->token)) {
          // @todo handle functions w/ SqlObjects
          if (!isset($this->tree['set_quantifier'])) {
            $function = $this->parseFunction();
            $sql_object->addField($function);
            $this->getToken();
          }
          else {
            return $this->raiseError('Cannot use "' . $sql_object->set_quantifier . '" with ' . $this->token);
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
   * Parses query table reference clause list.
   *
   * @param SqlObject $sql_object
   */
  protected function parseTables(SqlObject &$sql_object) {
    $this->getToken();
    while (TRUE) {
      $table = &$sql_object->addTable();
      // Parses join type.
      if (in_array($this->token, array(',', 'inner', 'cross', 'left', 'right', 'outer', 'natural', 'join'))) {
        if ($this->token == ',') {
          $table->setJoin(',');
          $this->getToken();
        }
        elseif ($this->token == 'join') {
          $table->setJoin('join');
          $this->getToken();
        }
        elseif (($this->token == 'cross') || ($this->token == 'inner')) {
          $join = strtolower($this->lexer->tokenText);
          $this->getToken();
          if ($this->token != 'join') {
            return $this->raiseError('Expected token "join"');
          }
          $table->setJoin($join . ' join');
          $this->getToken();
        }
        elseif (($this->token == 'left') || ($this->token == 'right')) {
          $join = strtolower($this->lexer->tokenText);
          $this->getToken();
          if ($this->token == 'join') {
            $table->setJoin($join . ' join');
          }
          elseif ($this->token == 'outer') {
            $join .= ' outer';
            $this->getToken();
            if ($this->token == 'join') {
              $table->setJoin($join . ' join');
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
            $table->setJoin($join . ' join');
          }
          elseif (($this->token == 'left') || ($this->token == 'right')) {
            $join .= ' ' . $this->token;
            $this->getToken();
            if ($this->token == 'join') {
              $table->setJoin($join . ' join');
            }
            elseif ($this->token == 'outer') {
              $join .= ' ' . $this->token;
              $this->getToken();
              if ($this->token == 'join') {
                $table->setJoin($join . ' join');
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
      }

      // Parses the table name
      $table->setName($this->lexer->tokenText);

      // Parses the table alias
      $this->getToken();
      if ($this->token == 'identifier') {
        $table->setAlias($this->lexer->tokenText);
        $this->getToken();
      }
      elseif ($this->token == 'as') {
        $this->getToken();
        if ($this->token == 'identifier') {
          $table->setAlias($this->lexer->tokenText);
        }
        else {
          return $this->raiseError('Expected table alias');
        }
        $this->getToken();
      }

      // Parses join condition.
      if ($this->token == 'on') {
        // @todo need to figure out how to properly parse and add this to
        // SqlJoinCondition.
        $join_condition = &$table->setJoinCondition($this->token);
        $clause = $this->parseSearchClause();
        $this->tree['table_join_clause'][] = $clause;
      }
      else if ($this->token == 'using') {
        $join_condition = & $table->setJoinCondition($this->token);

        // Advanced to first (.
        $this->getToken();
        if ($this->token != '(') {
          return $this->raiseError('Expected (');
        }

        // Get all join columns.
        do {
          $this->getToken();
          if ($this->token == 'identifier') {
            $join_condition->addColumn($this->lexer->tokenText);
          }
        } while ($this->token != ')');
        $this->getToken();
      }

      if (($this->token == 'where') || ($this->token == 'order') || ($this->token == 'limit') || (is_null($this->token))) {
        break;
      }
    }
  }

  /**
   * Parses order by clauses.
   *
   * @param SqlObject $sql_object
   */
  protected function parseOrderBy(SqlObject $sql_object) {
    $this->getToken();
    if ($this->token != 'by') {
      return $this->raiseError('Expected "by"');
    }
    $this->getToken();
    while ($this->token == 'identifier') {
      $column = $this->lexer->tokenText;
      $this->getToken();
      if ($this->dialect->isSynonym($this->token)) {
        $direction = $this->dialect->getSynonym($this->token);
        if (($direction != 'asc') && ($direction != 'desc')) {
          return $this->raiseError('Unexpected token');
        }
        $this->getToken();
      }
      else {
        $direction = 'asc';
      }
      if ($this->token == ',') {
        $this->getToken();
      }
      $sql_object->addOrderBy($column, $direction);
    }
  }

  /**
   * Parses limit clause.
   *
   * @param SqlObject $sql_object
   */
  protected function parseLimit(SqlObject $sql_object) {
    $this->getToken();
    if ($this->token != 'int_val') {
      return $this->raiseError('Expected an integer value');
    }
    $row_count = $this->lexer->tokenText;
    $offset = 0;
    $this->getToken();
    if ($this->token == ',') {
      $this->getToken();
      if ($this->token != 'int_val') {
        return $this->raiseError('Expected an integer value');
      }
      $offset = $row_count;
      $row_count = $this->lexer->tokenText;
      $this->getToken();
    }
    $sql_object->addLimit($row_count, $offset);
  }
}

/**
 * SQL Parser Exception.
 */
class SqlParserException extends Exception { }
