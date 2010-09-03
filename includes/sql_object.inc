<?php
// $Id$

/**
 * @file
 */

/**
 * Sql Object
 *
 * Provides a container for storing a SQL statement.
 */
class SqlObject {
  /**
   * SQL command.
   *
   * @var string
   */
  public $command;

  /**
   * SQL tables.
   *
   * @var array
   *   Array of SqlTable objects.
   */
  public $tables;

  /**
   * The array of all placeholders in the SQL statement.
   *
   * @var array
   */
  public $placeholders;

  /**
   * Adds a table object to the list of tables given a table name.
   *
   * A reference to the table object will be returned, which can then be used
   * to add aliases, joins, join conditions, etc.
   *
   * @param table_name
   *   The name of the table.
   *
   * @return
   *   A reference to the new table object.
   */
  function &addTable($table_name = NULL) {
    $this->tables[] = new SqlTable($table_name);

    // Return object by reference so further manipulations can be done.
    $table = &end($this->tables);
    return $table;
  }

  /**
   * Returns the number of placeholders in this SQL statement.
   *
   * @return int
   *   The number of placeholders.
   */
  function numPlaceholders() {
    return count($this->placeholders);
  }
}

/**
 * Sql Select
 *
 * Provides a container for storing SQL select statement.
 */
class SqlSelect extends SqlObject {
  /**
   * Sql set identifier (e.g. 'distinct' or 'all').
   *
   * @var string
   */
  public $set_identifier;

  /**
   * SQL fields.
   *
   * @var array
   *   Array of SqlField objects.
   */
  public $fields;

  /**
   * SQL conditional
   *
   * @var SqlConditional $conditional
   */
  public $conditional;

  /**
   * SQL group by clauses.
   *
   * @var SqlGroupByClause
   */
  public $group_by_clause;

  /**
   * SQL having clauses.
   *
   * @var array
   *   Array of SqlHavingClause objects.
   */
  public $having_clauses;

  /**
   * SQL order by clauses
   *
   * @var array
   *   Array of SqlOrderBy objects.
   */
  public $order_by_clauses;

  /**
   * SQL limit clause.
   *
   * @var SqlLimit.
   */
  public $limit;

  public function __construct() {
    $this->command = 'select';
  }

  /**
   * Set the set identifier of the query (e.g. distinct or all).
   *
   * @param $set_identifier
   *   The set identifier of the query.
   */
  public function setSetIdentifier($set_identifier) {
    $this->set_identifier = $set_identifier;
  }

  /**
   * Add field object to list of fields.
   *
   * A reference to the field object will be returned, which can then be used
   * to add alias and table info to.
   *
   * @param field_name
   *   The name of the field or SqlField object.
   *
   * @return
   *   A SqlField object.
   */
  public function &addField($field) {
    if ($field instanceof SqlField) {
      $this->fields[] = $field;
    }
    else {
      $this->fields[] = new SqlField($field);
    }
    // Return object by reference so further manipulations can be done.
    $field = end($this->fields);
    return $field;
  }

  /**
   * Set condition to SqlConditional object
   *
   * Complex conditional clauses can be created by using subclauses in
   * SqlConditonal->arg1 or SqlConditionalal->arg2.
   *
   * @param SqlConditional $sql_condition
   */
  public function setConditional(SqlConditional $sql_conditional) {
    $this->conditional = $sql_conditional;
  }

  /**
   * Add group by object.
   *
   * @return
   *   A SqlGroupByClause object.
   */
  public function addGroupBy() {
    $this->group_by_clause = new SqlGroupByClause();
    return $this->group_by_clause;
  }

  /**
   * Add having object.
   *
   * @return
   *   A SqlHavingClause object.
   */
  public function addHaving() {
    $this->having_clauses[] = new SqlHavingClause();

    // Return object by reference so further manipulations can be done.
    $having_clause = &end($this->having_clauses);
    return $having_clause;
  }

  /**
   * Add order by object.
   */
  public function addOrderBy($column, $direction = 'asc') {
    $this->order_by_clauses[] = new SqlOrderBy($column, $direction);
  }

  /**
   * Add limit object.
   */
  public function addLimit($row_count, $offset = NULL) {
    $this->limit = new SqlLimit($row_count, $offset);
  }

  public function __toString() {
    $output[] = 'SELECT';

    $output[] = join(', ', $this->fields);
    $output[] = 'FROM';
    $output[] = join(' ', $this->tables);

    if ($this->conditional) {
      $output[] = 'WHERE';
      $output[] = $this->conditional;
    }

    if ($this->group_by_clause) {
      $output[] = 'GROUP BY';
      $output[] = $this->group_by_clause;
    }

    if ($this->having_clauses) {
      $output[] = 'HAVING';
      $output[] = $this->having_clause;
    }

    if ($this->order_by_clauses) {
      $output[] = 'ORDER BY';
      $output[] = join(', ', $this->order_by_clauses);
    }

    if ($this->limit) {
      $output[] = $this->limit;
    }

    return join(' ', $output);
  }
}

/**
 * Sql Insert
 *
 * Provides a container for storing SQL insert statement.
 */
class SqlInsert extends SqlObject {
  /**
   * SQL insert columns.
   *
   * @var array
   */
  public $columns;

  public function __construct() {
    $this->command = 'insert';
  }

  /**
   * Adds column to insert statement.
   *
   * @param $column_name
   */
  public function addColumn(SqlColumn $column) {
    $this->columns[] = $column;
  }
}

/**
 * Sql Update
 *
 * Provides a container for storing SQL update statement.
 */
class SqlUpdate extends SqlObject {
  /**
   * SQL insert columns.
   *
   * @var array
   *   Array of SqlColumn objects
   */
  public $columns;

  /**
   * SQL conditional.
   *
   * @var SqlConditional
   */
  public $conditional;

  /**
   * SQL order by clauses
   *
   * @var array
   *   Array of SqlOrderBy objects.
   */
  public $order_by_clauses;

  /**
   * SQL limit clause.
   *
   * @var SqlLimit.
   */
  public $limit;


  public function __construct() {
    $this->command = 'update';
  }

  /**
   * Adds column/value pair for update statement.
   *
   * @param $column_name
   *   The column name.
   * @param $value
   *   The value of the column.
   * @param $type
   *   The type of the column.
   */
  public function &addColumn($column_name, $value = NULL, $type = NULL) {
    $column = new SqlColumn($column_name);

    if ($value) {
      $column->setValue($value);
    }

    if ($type) {
      $column->setType($type);
    }

    $this->columns[] = $column;

    // Return object by reference so further manipulations can be done.
    $column = &end($this->columns);
    return end($this->columns);
  }

  /**
   * Set condition to SqlConditional object
   *
   * Complex conditional clauses can be created by using subclauses in
   * SqlConditonal->arg1 or SqlConditionalal->arg2.
   *
   * @param SqlConditional $sql_condition
   */
  public function setConditional(SqlConditional $sql_conditional) {
    $this->conditional = $sql_conditional;
  }

  /**
   * Add order by object.
   */
  public function addOrderBy($column, $direction = 'asc') {
    $this->order_by_clauses[] = new SqlOrderBy($column, $direction);
  }

  /**
   * Add limit object.
   */
  public function addLimit($row_count, $offset = NULL) {
    $this->limit = new SqlLimit($row_count, $offset);
  }
}

/**
 * Sql Delete
 *
 * Provides a container for storing SQL delete statement.
 */
class SqlDelete extends SqlObject {

  /**
   * SQL conditional.
   *
   * @var SqlConditional
   */
  public $conditional;

  public function __construct() {
    $this->command = 'delete';
  }

  /**
   * Set condition to SqlConditional object
   *
   * Complex conditional clauses can be created by using subclauses in 
   * SqlConditonal->arg1 or SqlConditionalal->arg2.
   *
   * @param SqlConditional $sql_condition
   */
  public function setConditional(SqlConditional $sql_conditional) {
    $this->conditional = $sql_conditional;
  }
}

/**
 * Sql Column
 *
 * Provides a container for storing SQL column info.
 */
class SqlColumn {
  /**
   * Field name.
   *
   * @var string
   */
  public $name;

  /**
   * Field value.
   *
   * @var string
   */
  public $value;

  /**
   * Field type.
   *
   * @var string
   */
  public $type;

  public function __construct($name) {
    $this->name = $name;
  }

  /**
   * Sets the name of the field.
   *
   * @param $name
   *   The name of the field.
   */
  public function setName($name) {
    $this->name = $name;
  }

  /**
   * Sets the value of the field.
   *
   * Used in insert and update statements.
   *
   * @param $value
   *   The value of the field.
   */
  public function setValue($value) {
    $this->value = $value;
  }

  /**
   * Sets the type of the field.
   *
   * Valid types include:
   * -text_val
   * -int_val
   * -placeholder
   *
   * @param $type
   *   The type of the field.
   */
  public function setType($type) {
    $this->type = $type;
  }
}

/**
 * Sql Field
 *
 * Provides a container for storing SQL field info.
 */
class SqlField extends SqlColumn {

  /**
   * Field table
   *
   * @var string
   */
  public $table;

  /**
   * Field alias
   *
   * @var string
   */
  public $alias;

  /**
   * Sets the table of this field.
   *
   * @param $table
   *   The table of this field.
   */
  public function setTable($table) {
    $this->table = $table;
  }

  /**
   * Sets the alias of this field.
   *
   * @param $alias
   *   The alias of this field.
   */
  public function setAlias($alias) {
    $this->alias = $alias;
  }

  /**
   * Prints out a valid string representation of this field.
   */
  public function __toString() {
    $output = '';
    if ($this->table) {
      $output .= $this->table . '.';
    }

    if ($this->type == 'null') {
      $output .= 'NULL';
    }
    else {
      $output .= $this->name;
    }

    if ($this->alias) {
      $output .= ' AS ' . $this->alias;
    }
    return $output;
  }
}

/**
 * Sql Function
 *
 * Provides a container for storing SQL function info.
 */
class SqlFunction extends SqlField {
  /**
   * Function arguments.
   *
   * @var mixed
   */
  public $arguments;

  /**
   * Distinct operator
   *
   * @var boolean
   */
  public $distinct;

  /**
   * Adds a new argument to the function.
   *
   * @param string $argument
   *   The argument to add to the function.
   */
  public function addArgument($argument) {
    $this->arguments[] = $argument;
  }

  /**
   * Sets the distinct operator.
   *
   * @param boolean $distinct
   *   True to set the distinct operator, FALSE to unset the distinct operator.
   *   Defaults to TRUE
   */
  public function setDistinct($distinct = TRUE) {
    $this->distinct = $distinct;
  }

  public function __toString() {
    return $this->name . '(' . $this->distinct . impode(', ', $this->arguments) . ')';
  }
}

/**
 * Sql Table
 *
 * Provides a container for storing SQL table info.
 */
class SqlTable {
  /**
   * Table name.
   *
   * @var string
   */
  public $name;

  /**
   * Table alias.
   *
   * @var string
   */
  public $alias;

  /**
   * Join type
   *
   * @var string
   */
  public $join;

  /**
   * Join condition
   *
   * @var SqlJoinConditional
   */
  public $join_conditional;

  /**
   * Create a new table object.
   *
   * @param $name
   *   The table name.
   */
  public function __construct($name = NULL) {
    $this->name = $name;
  }

  /**
   * Set the table name.
   *
   * @param $name
   *   The table name.
   */
  public function setName($name) {
    $this->name = $name;
  }

  /**
   * Set the table alias.
   *
   * If the table alias is already set, then replaces table alias, since there can be
   * only one alias per table.
   *
   * @param $alias
   *   The table alias.
   */
  public function setAlias($alias) {
    $this->alias = $alias;
  }

  /**
   * Set the table join type.
   *
   * If the table join type is already set, then replaces table join type,
   * since there can only be one join type per table.  The join type will
   * be converted to lower case.
   *
   * @param $join
   *   The table join type.  Valid join types include:
   *   -join
   *   -[inner | cross] join
   *   -straight_join
   *   -[left | right] [outer] join
   *   -natural [left | right] [outer] join
   */
  public function setJoin($join) {
    $this->join = strtolower($join);
  }

  /**
   * Sets the join condition for this table.
   *
   * Creates a new SqlJoinConditional and returns a reference to the created
   * object.
   *
   * @param $type
   *   The type of join condition to create.
   */
  public function &setJoinCondition($type) {
    $this->join_conditional = new SqlJoinConditional($type);
    return $this->join_conditional;
  }

  public function __toString() {
    $output = '';
    if ($this->join) {
      $output .= strtoupper($this->join) . ' ';
    }
    $output .= $this->name;
    if ($this->alias) {
      $output .= ' ' . $this->alias;
    }
    if ($this->join_conditional) {
      $output .= $this->join_conditional;
    }
    return $output;
  }
}

/**
 * Sql Conditional
 *
 * Provides a container for storing SQL condiitonal info.
 */
class SqlConditional {
  /**
   * Right arg in conditional.
   *
   * @var mixed
   */
  public $arg1;

  /**
   * Left arg in conditonal.
   *
   * @var mixed
   */
  public $arg2;

  /**
   * Conditional operator.
   *
   * @var string
   */
  public $operator;

  /**
   * Logical not opeartor.
   *
   * @var boolean
   */
  public $not_operator;

  public function __construct($arg1 = NULL, $arg2 = NULL, $operator = NULL) {
    $this->arg1 = $arg1;
    $this->arg2 = $arg2;
    $this->operator = $operator;
  }

  /**
   * Set the first argument of this condition.
   *
   * Can either be SqlField or SqlCondition object.
   *
   * @param $arg1
   *   Either a SqlField or SqlCondition.
   */
  public function setArg1($arg1) {
    $this->arg1 = $arg1;
  }

  /**
   * Set the second argument of this condition.
   *
   * Can either be SqlField, SqlCondition or SqlSelect (for subselects) object.
   * In the case of IN operators, the second argument could be an array of
   * SqlField objects.
   *
   * @param $arg1
   *   Either a SqlField or SqlCondition.
   */
  public function setArg2($arg2) {
    $this->arg2 = $arg2;
  }

  /**
   * Set the operator of this condition.
   *
   * @param $operator
   *   The operator.
   */
  public function setOperator($operator) {
    $this->operator = $operator;
  }

  /**
   * Set the logical not operator for this condition.
   *
   * @param boolean $not_operator
   *   Defaults to TRUE to set the not operator.
   */
  public function setNot($not_operator = TRUE) {
    $this->not_operator = $not_operator;
  }

  /**
   * Prints out a valid string representation of this condition.
   *
   * @todo parenthesis aren't correctly placed for SQL statements that don't 
   * originallly contain parenthesis, e.g.:
   *   column1 = 'test' AND column2 = 'test' OR column3 = 'test'
   * is converted to:
   *   column1 = 'test' AND (column2 = 'test' OR column3 = 'test')
   * instead of:
   *   (column1 = 'test' AND column2 = 'test') OR column3 = 'test'
   */
  public function __toString() {
    $output = '';

    // Check if arg1 is a grouped conditional. If so, enclose in parenthesis.
    if ($this->arg1 instanceof SqlConditional && ($this->arg1->arg1 instanceof SqlConditional || $this->arg1->arg2 instanceof SqlConditional)) {
      $output .= '(' . $this->arg1 . ')';
    }
    else {
      $output .= $this->arg1;
    }

    $output .= ' ' . strtoupper($this->operator) . ' ' . ($this->not_operator ? 'NOT ' : '');

    if ($this->arg2) {
      // Check if arg2 is a grouped conditional. If so, enclose in parenthesis.
      if ($this->arg2 instanceof SqlConditional && ($this->arg2->arg2 instanceof SqlConditional || $this->arg2->arg2 instanceof SqlConditional)) {
        $output .= '(' . $this->arg2 . ')';
      }
      else {
        $output .= $this->arg2;
      }
    }

    return $output;
  }
}

/**
 * Sql Join Conditional
 *
 * Provides a container for storing SQL join conditional info.
 */
class SqlJoinConditional extends SqlConditional {
  /**
   * The join condition type (either 'on' or 'using').
   *
   * @var string
   */
  public $type;

  /**
   * If type is 'using', then list of columns to join by.
   *
   * @var array
   */
  public $columns;

  public function __construct($type = 'on') {
    $this->type = $type;
  }

  /**
   * If type join clause is 'using' type, then adds a column to the columns
   * field.
   *
   * @param @column_name
   *   The column name to add.
   */
  function addColumn($column_name) {
    if ($this->type == 'using') {
      $this->columns[] = $column_name;
    }
  }

  public function __toString() {
    $output = ' ' . strtoupper($this->type);
    if ($this->type == 'using') {
      $output .= '(' . join(', ', $this->columns) . ')';
    }
    else {
      $output .= ' ' . parent::__toString();
    }
    return $output;
  }
}

/**
 * Sql Group By Clause
 *
 * Provides a container for storing SQL group by clause info.
 */
class SqlGroupByClause {
  /**
   * Group By Columns
   *
   * @var array
   */
  public $columns;

  public function __construct() { }

  /**
   * Adds a column to the group by clause
   *
   * @param $column_name
   *   The group by column.
   */
  public function addColumn($column_name) {
    $this->columns[] = $column_name;
  }
}

/**
 * Sql Having Clause
 *
 * Provides a container for storing SQL having clause info.
 */
class SqlHavingClause extends SqlConditional { }

/**
 * Sql Order By
 *
 * Provides a container for storing SQL order by clause info.
 */
class SqlOrderBy {
  /**
   * Order by column
   *
   * @var string
   */
  public $column;

  /**
   * Order by direction
   *
   * @var string
   */
  public $direction;

  public function __construct($column, $direction = 'asc') {
    $this->column = $column;
    $this->direction = $direction;
  }

  public function __toString() {
    return $this->column . ' ' . strtoupper($this->direction);
  }
}

/**
 * Sql Limit
 *
 * Provides a container for storing SQL limit clause info.
 */
class SqlLimit {
  /**
   * Limit row count
   *
   * @var int
   */
  public $row_count;

  /**
   * Limit offset
   *
   * @var int
   */
  public $offset;

  public function __construct($row_count, $offset = 0) {
    $this->row_count = $row_count;
    $this->offset = $offset;
  }

  public function __toString() {
    return 'LIMIT ' . $this->row_count . ($this->offset != 0 ? ' OFFSET ' . $this->offset : '');
  }
}
