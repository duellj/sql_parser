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
   * SQL placeholders
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
  function &addTable($table_name) {
    $this->tables[] = new SqlTable($table_name);
    return end($this->tables[]);
  }

  function setParameters
}

/**
 * Sql Select
 *
 * Provides a container for storing SQL select statement.
 */
class SqlSelect extends SqlObject {
  /**
   * SQL fields.
   *
   * @var array
   *   Array of SqlField objects.
   */
  public $fields;

  /**
   * SQL conditions.
   *
   * @var array
   *   Array of SqlConditional objects.
   */
  public $conditions;

  /**
   * SQL group by clauses.
   *
   * @var array
   *   Array of SqlGroupByClause objects.
   */
  public $group_by_clauses;

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

  protected function __construct() {
    $this->command = 'select';
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

  /**
   * SQL insert values.
   *
   * @var array
   */
  public $values;

  protected function __construct() {
    $this->command = 'insert';
  }

  /**
   * Sets column/value pair for insert statement.
   *
   * @param $column_name
   *   The column name.
   * @param $value
   *   The value of the column.
   */
  public function setColumnValue($column_name, $value) {
  }
}

/**
 * Sql Update
 *
 * Provides a container for storing SQL update statement.
 */
class SqlUpdate extends SqlObject {

  protected function __construct() {
    $this->command = 'update';
  }
}

/**
 * Sql Delete
 *
 * Provides a container for storing SQL delete statement.
 */
class SqlDelete extends SqlObject {

  protected function __construct() {
    $this->command = 'delete';
  }
}

/**
 * Sql Field
 *
 * Provides a container for storing SQL field info.
 */
class SqlField {
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
   * @todo make this a join condition object?
   */
  public $join_condition;

  /**
   * Create a new table object.
   *
   * @param $name
   *   The table name.
   */
  protected function __construct($name) {
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
   * also be converted to lower case.
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
}

/**
 * Sql Conditional
 *
 * Provides a container for storing SQL condiitonal info.
 */
class SqlConditional {
}

/**
 * Sql Group By Clause
 *
 * Provides a container for storing SQL group by clause info.
 */
class SqlGroupByClause {
}

/**
 * Sql Having Clause
 *
 * Provides a container for storing SQL having clause info.
 */
class SqlHavingClause extends SqlConditional {
}

/**
 * Sql Order By
 *
 * Provides a container for storing SQL order by clause info.
 */
class SqlOrderBy {
}

/**
 * Sql Limit
 *
 * Provides a container for storing SQL limit clause info.
 */
class SqlLimit {
}
