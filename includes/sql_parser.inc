<?php
// $Id: sql_parser.inc,v 1.17 2010/09/09 01:21:15 duellj Exp $

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
 * @todo Parse multiple SQL strings.
 * @todo Parse CREATE and DROP statements.
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
   * The array of all placeholders found in the parsed SQL statement.
   *
   * @var array
   */
  protected $placeholders;

  /**
   * The exception if parser encounters an error.
   *
   * @var SqlParserException
   */
  protected $exception;

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
      throw new SqlParserException('No initial string specified', $this->token, $this->lexer);
    }

    // Get query action.
    $this->getToken();
    try {
      switch ($this->token) {
        case NULL:
          // NULL == end of string
          throw new SqlParserException('Nothing to do', $this->token, $this->lexer);

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
          throw new SqlParserException('Unknown action :' . $this->token, $this->token, $this->lexer);
          break;
      }
    } catch (Exception $e) {
      $this->exception = $e;
      return;
    }
    if ($sql_object) {
      $sql_object->placeholders = $this->placeholders;
      sdp(print_r($sql_object, 1), '$sql_object');
      return $sql_object;
    }
  }

  /**
   * Has an error been found during parsing?
   *
   * @return boolean
   */
  public function isException() {
    return !empty($this->exception);
  }

  /**
   * Return the parsing exception.
   *
   * @return SqlParserException
   *   The exception object that contains the error message.
   */
  public function getException() {
    return $this->exception;
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
      throw new SqlParserException('Expected "from"', $this->token, $this->lexer);
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

        case 'offset':
//           $this->parseLimit($sql_object);
          break;

        case 'group':
          // @todo Certain clauses should not restrict tokens to non-reserved words (e.g. module, count)
          // This includes table and column alias expressions, and GROUP BY and ORDER BY clauses which
          // can refer to the column aliases.
          // Table names may not be used in an ON clause of a JOIN, or GROUP BY and ORDER BY clauses.
          // Column aliases may not be used in an ON clause of a JOIN or a WHERE clause.
          $group_by = /*&*/$sql_object->addGroupBy(); // @todo Remove '&' from before function calls
          $this->getToken();
          if ($this->token != 'by') {
            throw new SqlParserException('Expected "by"', $this->token, $this->lexer);
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
          sdp("'" . ord($this->token) . "'", 'ord($this->token)');
          is_null($this->token) ? sdp('token is null') : sdp('token is NOT null');
          throw new SqlParserException('Unexpected clause', $this->token, $this->lexer);
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
      throw new SqlParserException('Expected table name', $this->token, $this->lexer);
    }

    $this->parseSetFields($sql_object);

    while (!is_null($this->token) && $this->token != ')' && $this->token != ')') {
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
          throw new SqlParserException('Unexpected clause', $this->token, $this->lexer);
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
        throw new SqlParserException('Expected table name', $this->token, $this->lexer);
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
            throw new SqlParserException('Expected a value', $this->token, $this->lexer);
          }

          $this->getToken();
          if (($this->token != ',') && ($this->token != ')')) {
            throw new SqlParserException('Expected , or )', $this->token, $this->lexer);
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
            throw new SqlParserException('Expected a value', $this->token, $this->lexer);
          }

          $this->getToken();
          if (($this->token != ',') && ($this->token != ')')) {
            throw new SqlParserException('Expected , or )', $this->token, $this->lexer);
          }
        }

        if (count($columns) > 0 && (count($columns) != count($values))) {
          throw new SqlParserException('field/value mismatch', $this->token, $this->lexer);
        }
        if (count($values) == 0) {
          throw new SqlParserException('No fields to insert', $this->token, $this->lexer);
        }

        foreach ($columns as $column) {
          $sql_object->addColumn($column);
        }
      }
      else {
        throw new SqlParserException('Expected "values"', $this->token, $this->lexer);
      }
    }
    else {
      throw new SqlParserException('Expected "into"', $this->token, $this->lexer);
    }
    return $sql_object;
  }

  /**
   * Parses a SQL delete statement.
   */
  function parseDelete() {
    $sql_object = new SqlDelete();

    $this->getToken();
    if ($this->token != 'from') {
      throw new SqlParserException('Expected "from"', $this->token, $this->lexer);
    }

    $this->getToken();
    if ($this->token != 'identifier') {
      throw new SqlParserException('Expected a table name', $this->token, $this->lexer);
    }
    $sql_object->addTable($this->lexer->tokenText);

    $this->getToken();
    if ($this->token == 'where') {
      $conditional = $this->parseSearchClause();
      $sql_object->setConditional($conditional);
    }

    return $sql_object;
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
          throw new SqlParserException('Expected table name', $this->token, $this->lexer);
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
        throw new SqlParserException('Unknown object to create', $this->token, $this->lexer);
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
          throw new SqlParserException('Expected a table name', $this->token, $this->lexer);
        }
        $this->tree['table_names'][] = $this->lexer->tokenText;
        $this->getToken();
        if (($this->token == 'restrict') || ($this->token == 'cascade')) {
          $this->tree['drop_behavior'] = $this->token;
        }
        $this->getToken();
        if (!is_null($this->token)) {
          throw new SqlParserException('Unexpected token', $this->token, $this->lexer);
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
        throw new SqlParserException('Unknown object to drop', $this->token, $this->lexer);
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
        $field = new SqlField($this->lexer->tokenText);
        $field->setType = $this->token;
        $parameters[] = $field;
      }
      else {
        throw new SqlParserException('Expected a value', $this->token, $this->lexer);
      }

      $this->getToken();
      if ($this->token == ')') {
        return $parameters;
      }
      if (($this->token != ',') && ($this->token != ')')) {
        throw new SqlParserException('Expected , or )', $this->token, $this->lexer);
      }
    }
  }

  /**
   * Get the next token from SqlLexer:lex().
   */
  function getToken() {
    $this->token = $this->lexer->lex();

    // If the token is a placeholder, add to the placeholder array
    if ($this->token == 'placeholder') {
      $this->placeholders[] = $this->lexer->tokenText;
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
      throw new SqlParserException('Expected "set"', $this->token, $this->lexer);
    }

    while (TRUE) {
      $this->getToken();
      if ($this->token != 'identifier') {
        throw new SqlParserException('Expected a column name', $this->token, $this->lexer);
      }
      $column = &$sql_object->addColumn($this->lexer->tokenText);
      $this->getToken();
      if ($this->token != '=') {
        throw new SqlParserException('Expected =', $this->token, $this->lexer);
      }
      $this->getToken();
      if (!$this->dialect->isVal($this->token)) {
        throw new SqlParserException('Expected a value', $this->token, $this->lexer);
      }
      $column->setValue($this->lexer->tokenText);
      $column->setType($this->token);

      $this->getToken();
      if ($this->token == 'where' || $this->token == 'order' || $this->token == 'limit') {
        break;
      }
      elseif ($this->token != ',') {
        throw new SqlParserException('Expected "where", "order", "limit" or ","', $this->token, $this->lexer);
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
        throw new SqlParserException('Expected ")"', $this->token, $this->lexer);
      }
      $foundSubclause = TRUE;
    }
    elseif ($this->dialect->isFunc($this->token)) {
      // Argument is a funciton.
      $sql_conditional->setArg1($this->parseFunction());
    }
    elseif ($this->dialect->isReserved($this->token)) {
      throw new SqlParserException('Expected a column name or value', $this->token, $this->lexer);
    }
    else {
      // Argument is a field.
      $field = $this->parseField();
      $sql_conditional->setArg1($field);
    }

    // Parse the operator.
    if (!$foundSubclause) {
      $this->getToken();
      if (!$this->dialect->isOperator($this->token)) {
        throw new SqlParserException('Expected an operator', $this->token, $this->lexer);
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
            throw new SqlParserException('Expected "NULL"', $this->token, $this->lexer);
          }
          $field = new SqlField(NULL);
          $field->setType($this->token);
          $sql_conditional->setArg2($field);
          break;

        case 'not':
          // Parse for 'not in' operator.
          if ($this->token != 'in') {
            throw new SqlParserException('Expected "in"', $this->token, $this->lexer);
          }
          $sql_conditional->setOperator($this->token);
          $sql_conditional->setNot();
          $this->getToken();
        case 'in':
          // parse for 'in' operator
          if ($this->token != '(') {
            throw new SqlParserException('Expected "("', $this->token, $this->lexer);
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
            throw new SqlParserException('Expected ")"', $this->token, $this->lexer);
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
            throw new SqlParserException('Expected a column name or value', $this->token, $this->lexer);
          }
          if ($this->token == '(') {
            $sql_conditional->arg2($this->parseSearchClause());
            $this->getToken();
            if ($this->token != ')') {
              throw new SqlParserException('Expected ")"', $this->token, $this->lexer);
            }
          }
          else {
            $field = $this->parseField();
            $sql_conditional->setArg2($field);
          }
          break;
      }
    }

    // Check if there is another conditional to parse.
    $this->getToken();
    if (($this->token == 'and') || ($this->token == 'or') || $this->token == 'xor') {
      if (!is_null($sql_conditional->arg2)) {
        $inner_conditional = new SqlConditional();
        $inner_conditional->setOperator($sql_conditional->operator);
        $inner_conditional->setArg1($sql_conditional->arg1);
        $inner_conditional->setArg2($sql_conditional->arg2);
        $inner_conditional->setNot($sql_conditional->not_operator);
        $sql_conditional->setArg1($inner_conditional);
      }

      $sql_conditional->setOperator($this->token);
      $sql_conditional->setArg2($this->parseSearchClause());
      $sql_conditional->setNot(FALSE);
    }
    elseif (!is_null($this->token)) {
      sdp('$this->lexer->unget()');
      $this->lexer->unget();
    }
    else {
      sdp('is null in conditional');
    }
    return $sql_conditional;
  }

  /**
   * Parses field list for create statement.
   */
  protected function parseFieldList() {
    $this->getToken();
    if ($this->token != '(') {
      throw new SqlParserException('Expected (', $this->token, $this->lexer);
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
        throw new SqlParserException('Expected identifierifier', $this->token, $this->lexer);
      }

      // parse field type
      $this->getToken();
      if ($this->dialect->isType($this->token)) {
        $type = $this->token;
      }
      else {
        throw new SqlParserException('Expected a valid type', $this->token, $this->lexer);
      }

      $this->getToken();
      // handle special case two-word types
      if ($this->token == 'precision') {
        // double precision == double
        if ($type == 'double') {
          throw new SqlParserException('Unexpected token', $this->token, $this->lexer);
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
          throw new SqlParserException('Unexpected token', $this->token, $this->lexer);
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
                throw new SqlParserException('Expected an integer', $this->token, $this->lexer);
              }
              $fields[$name]['decimals'] = $parameters['values'][1];
            }
          case 'float':
            if ($parameters['types'][0] != 'int_val') {
              throw new SqlParserException('Expected an integer', $this->token, $this->lexer);
            }
            $fields[$name]['length'] = $parameters['values'][0];
            break;

          case 'char':
          case 'varchar':
          case 'integer':
          case 'int':
            if (count($parameters['values']) != 1) {
              throw new SqlParserException('Expected 1 parameter', $this->token, $this->lexer);
            }
            if ($parameters['types'][0] != 'int_val') {
              throw new SqlParserException('Expected an integer', $this->token, $this->lexer);
            }
            $fields[$name]['length'] = $parameters['values'][0];
            break;

          case 'set':
          case 'enum':
            if (!count($parameters['values'])) {
              throw new SqlParserException('Expected a domain', $this->token, $this->lexer);
            }
            $fields[$name]['domain'] = $parameters['values'];
            break;

          default:
            if (count($parameters['values'])) {
              throw new SqlParserException('Unexpected )', $this->token, $this->lexer);
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
            throw new SqlParserException('Expected a constraint name', $this->token, $this->lexer);
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
            throw new SqlParserException('Expected default value', $this->token, $this->lexer);
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
            throw new SqlParserException('Expected "key"', $this->token, $this->lexer);
          }
          break;

        case 'not':
          $this->getToken();
          if ($this->token == 'null') {
            $constraintOpts = array('type' => 'not_null',
              'value' => TRUE);
          }
          else {
            throw new SqlParserException('Expected "NULL"', $this->token, $this->lexer);
          }
          break;

        case 'check':
          $this->getToken();
          if ($this->token != '(') {
            throw new SqlParserException('Expected (', $this->token, $this->lexer);
          }
          $results = $this->parseSearchClause();
          //if (PEAR::isError($results)) {
          //return $results;
          //}
          $results['type'] = 'check';
          $constraintOpts = $results;
          if ($this->token != ')') {
            throw new SqlParserException('Expected )', $this->token, $this->lexer);
          }
          break;

        case 'unique':
          $this->getToken();
          if ($this->token != '(') {
            throw new SqlParserException('Expected (', $this->token, $this->lexer);
          }
          $constraintOpts = array('type' => 'unique');
          $this->getToken();
          while ($this->token != ')') {
            if ($this->token != 'identifier') {
              throw new SqlParserException('Expected an identifierifier', $this->token, $this->lexer);
            }
            $constraintOpts['column_names'][] = $this->lexer->tokenText;
            $this->getToken();
            if (($this->token != ')') && ($this->token != ',')) {
              throw new SqlParserException('Expected ) or ,', $this->token, $this->lexer);
            }
          }
          if ($this->token != ')') {
            throw new SqlParserException('Expected )', $this->token, $this->lexer);
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
                  throw new SqlParserException('Expected interval quanta', $this->token, $this->lexer);
                }
                if ($class[$this->token] >= $class[$constraintOpts['quantum_1']]) {
                  throw new SqlParserException($this->token . ' is not smaller than ' . $constraintOpts['quantum_1'], $this->token, $this->lexer);
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
            throw new SqlParserException('Expected interval quanta', $this->token, $this->lexer);
          }
          $constraintOpts['type'] = 'values';
          break;

        case 'null':
          $haveValue = FALSE;
          break;

        default:
          throw new SqlParserException('Unexpected token ' . $this->lexer->tokenText, $this->token, $this->lexer);
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
   * Parses field into a new field object.
   * 
   * @note Handles field expressions in ON and WHERE clauses.
   *
   * @return SqlField
   */
  protected function parseField() {
    sdp(__FUNCTION__);
    sdp($this->lexer->tokenText, '$this->lexer->tokenText');

    if (strpos($this->lexer->tokenText, '.')) {
      list($columnTable, $columnName) = explode(".", $this->lexer->tokenText . '.');
    }
    else {
      $columnName = $this->lexer->tokenText;
      $columnTable = '';
    }

    $field = new SqlField($columnName);
    $field->setTable($columnTable);
    $field->setType($this->token);

    return $field;
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
      throw new SqlParserException('Expected "("', $this->token, $this->lexer);
    }

    switch ($function->name) {
      case 'count':
        $this->getToken();
        switch ($this->token) {
          case 'distinct':
            $function->setDistinct();
            $this->getToken();
            if ($this->token != 'identifier') {
              throw new SqlParserException('Expected a column name', $this->token, $this->lexer);
            }

          case 'identifier':
          case '*':
            $function->addArgument($this->lexer->tokenText);
            break;

          default:
            throw new SqlParserException('Invalid argument', $this->token, $this->lexer);
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
              throw new SqlParserException('Expected a string or a column name', $this->token, $this->lexer);
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
      throw new SqlParserException('Expected ")"', $this->token, $this->lexer);
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
        sdp($this->token, 'token type for alias');
        sdp($this->lexer->tokenText, 'tokenText for alias');
        throw new SqlParserException('Expected column alias 1', $this->token, $this->lexer);
      }
    }
    else {
      if ($this->token == 'identifier' ) {
        $function->setAlias($this->lexer->tokenText);
      }
      else {
        sdp($this->token, 'token type for alias');
        sdp($this->lexer->tokenText, 'tokenText for alias');
        throw new SqlParserException('Expected column alias 2, from or comma', $this->token, $this->lexer);
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
    sdp(__FUNCTION__);
    sdp($this->lexer->tokenText, '$this->lexer->tokenText');
    if ($this->token == '*') {
      $sql_object->addField($this->token);
      $this->getToken();
    }
    elseif ($this->token == 'identifier' || $this->dialect->isFunc($this->token)) {
      while ($this->token != 'from') {
        sdp(__FUNCTION__);
        sdp($this->lexer->tokenText, '$this->lexer->tokenText');
        if ($this->token == 'identifier') {
          list($columnTable, $columnName) = explode(".", $this->lexer->tokenText . '.');
          if (!$columnName) {
            // @todo This is setting a.*; storing a as columnName
            $columnName = $columnTable;
            $columnTable = '';
          }
          $column = &$sql_object->addField($columnName);
          $column->setTable($columnTable);

          $this->getToken();
          if ($this->token == 'as') {
            $this->getToken();
            $columnAlias = $this->lexer->tokenText;
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
            throw new SqlParserException('Cannot use "' . $sql_object->set_quantifier . '" with ' . $this->token, $this->token, $this->lexer);
          }
        }
        elseif ($this->token == ',') {
          $this->getToken();
        }
        else {
          throw new SqlParserException('Unexpected token "' . $this->token . '"', $this->token, $this->lexer);
        }
      }
    }
    else {
      throw new SqlParserException('Expected columns or a [set?] function', $this->token, $this->lexer);
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
            throw new SqlParserException('Expected token "join"', $this->token, $this->lexer);
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
              throw new SqlParserException('Expected token "join"', $this->token, $this->lexer);
            }
          }
          else {
            throw new SqlParserException('Expected token "outer" or "join"', $this->token, $this->lexer);
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
                throw new SqlParserException('Expected token "join" or "outer"', $this->token, $this->lexer);
              }
            }
            else {
              throw new SqlParserException('Expected token "join" or "outer"', $this->token, $this->lexer);
            }
          }
          else {
            throw new SqlParserException('Expected token "left", "right" or "join"', $this->token, $this->lexer);
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
          sdp($this->token, 'token type for alias');
          sdp($this->lexer->tokenText, 'tokenText for alias');
          throw new SqlParserException('Expected table alias', $this->token, $this->lexer);
        }
        $this->getToken();
      }

      // Parses join condition.
      if ($this->token == 'on') {
        $join_condition = &$table->setJoinCondition($this->token);
        $this->parseSearchClause($join_condition);
      }
      else if ($this->token == 'using') {
        $join_condition = & $table->setJoinCondition($this->token);

        // Advanced to first (.
        $this->getToken();
        if ($this->token != '(') {
          throw new SqlParserException('Expected (', $this->token, $this->lexer);
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
      throw new SqlParserException('Expected "by"', $this->token, $this->lexer);
    }
    $this->getToken();
    while ($this->token == 'identifier') {
      $column = $this->lexer->tokenText;
      $this->getToken();
      if ($this->dialect->isSynonym($this->token)) {
        $direction = $this->dialect->getSynonym($this->token);
        if (($direction != 'asc') && ($direction != 'desc')) {
          throw new SqlParserException('Unexpected token', $this->token, $this->lexer);
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
    if ($this->token != 'int_val' && $this->token != 'placeholder') {
      throw new SqlParserException('Expected an integer value', $this->token, $this->lexer);
    }
    $row_count = $this->lexer->tokenText;
    $offset = 0;
    $this->getToken();
    if ($this->token == ',') {
      $this->getToken();
      if ($this->token != 'int_val' && $this->token != 'placeholder') {
        throw new SqlParserException('Expected an integer value', $this->token, $this->lexer);
      }
      $offset = $row_count;
      $row_count = $this->lexer->tokenText;
      $this->getToken();
    }
    elseif ($this->token == 'offset') {
      $this->getToken();
      if ($this->token != 'int_val' && $this->token != 'placeholder') {
        throw new SqlParserException('Expected an integer value', $this->token, $this->lexer);
      }
      $offset = $this->lexer->tokenText;
      $this->getToken();
    }
    $sql_object->addLimit($row_count, $offset);
  }
}

/**
 * SQL Parser Exception.
 */
class SqlParserException extends Exception {
  /**
   * Error message.
   * 
   * @var string
   */
  protected $message;

  /**
   * Found token.
   *
   * @var string
   */
  protected $token;

  /**
   * SqlLexer object.
   *
   * Holds original sql string, parsed line number, etc. for pretty error 
   * formatting.
   *
   * @var SqlLexer
   */
  protected $lexer;

  public function __construct($message, $token, SqlLexer $lexer) {
    $this->message = $message;
    $this->token = $token;
    $this->lexer = $lexer;
  }

  public function __toString() {
    $end = 0;
    if ($this->lexer->sql_string != '') {
      while (($this->lexer->lineBegin + $end < $this->lexer->stringLength) && ($this->lexer->sql_string{$this->lexer->lineBegin + $end} != "\n")) {
        ++$end;
      }
    }

    $message = 'Parse error: ' . $this->message . ' on line ' . ($this->lexer->lineNumber + 1) . "\n";
    $message .= substr($this->lexer->sql_string, $this->lexer->lineBegin, $end) . "\n";
    $length = is_null($this->token) ? 0 : strlen($this->lexer->tokenText);
    $message .= str_repeat(' ', abs($this->lexer->tokenPointer - $this->lexer->lineBegin - $length)) . "^";
    $message .= ' found: "' . $this->lexer->tokenText . '"';
    return $message;
  }
}
