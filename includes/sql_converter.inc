<?php
// $Id$

/**
 * @file
 */

/**
 * Sql Converter
 *
 * Takes a parsed sql array and converts it into a DBTNG object string.
 *
 * @todo: Add support for converting into actual DBTNG objects (new class).
 */
class SqlConverter {
  protected $parsed_sql;

  protected $output = array();

  public function __construct($parsed_sql) {
    $this->parsed_sql = $parsed_sql;
  }

  public function setValues($values) { }

  public function convert() {
    if (!is_array($this->parsed_sql)) {
      return;
    }
    switch ($this->parsed_sql['command']) {
      case 'select':
        $this->convertSelect();
        break;
      case 'insert':
        $this->convertInsert();
        break;
      case 'update':
        $this->convertUpdate();
        break;
      case 'delete':
        $this->convertDelete();
        break;
    }
    $this->output[] = "\t->execute()";

    return join("\n", $this->output);
  }

  protected function convertSelect() {
    $this->output[] = "db_select('" . $this->parsed_sql['table_names'][0] . "', '" . $this->getTableAlias(0) . "')";

    $this->convertSelectFields($values);

    if ($this->parsed_sql['table_join']) {
      $this->convertJoins();
    }

    if ($this->parsed_sql['where_clause']) {
      $this->convertConditionals($this->parsed_sql['where_clause'], $values);
    }

    if ($this->parsed_sql['group_by']) {
      foreach ($this->parsed_sql['group_by'] as $group_by) {
        $this->output[] = "\t->groupBy($group_by)";
      }

      if ($this->parsed_sql['having_clause']) {
        $this->convertHaving($this->parsed_sql['having_clause'], $values);
      }
    }

    if ($this->parsed_sql['sort_order']) {
      foreach ($this->parsed_sql['sort_order'] as $field => $order) {
        $this->output[] = "\t->orderBy('$field', '$order')";
      }
    } 

    if ($this->parsed_sql['limit_clause']) {
      $this->output[] = "\t->range(" . $this->parsed_sql['limit_clause']['start'] . ', ' . $this->parsed_sql['limit_clause']['length'] . ")";
    }
  }

  protected function convertInsert() {
    $this->output[] = '$id = db_insert(\'' . $this->parsed_sql['table_names'][0] . '\')';
    $this->convertFields($values);
  }

  protected function convertUpdate() {
    $this->output[] = "db_update('" . $this->parsed_sql['table_names'][0] . "')";
    $this->convertFields($values);
    $this->convertConditionals($this->parsed_sql['where_clause'], $values);
  }

  protected function convertDelete() {
  }

  /**
   * Convert Select query fields.
   */
  protected function convertSelectFields($passed_values) {
    // Select all fields.
    if ($this->parsed_sql['column_names'][0] == '*') {
      $this->output[] = "\t->fields()";
    }
    // Select individual fields.
    else {
      // Group fields by table.  If no table is set for a field, then use the first table.
      // If field has an alias, then the field needs to be grouped individually.
      $table_fields = array();
      $aliased_fields = array();
      foreach ($this->parsed_sql['column_names'] as $id => $column_name) {
        if ($this->parsed_sql['column_aliases'][$id]) {
          $aliased_fields[] = $id;
        }
        else {
          if ($this->parsed_sql['column_tables'][$id]) {
            $table_fields[$this->parsed_sql['column_tables'][$id]][] = $column_name;
          }
          else {
            $table_fields[$this->parsed_sql['table_aliases'][0]][] = $column_name;
          }
        }
      }

      // Add fields for each table.
      if ($table_fields) {
        foreach ($table_fields as $table_alias => $columns) {
          $this->output[] = "\t->fields('$table_alias', array('" . join("', '", $columns) . "'))";
        }
      }
      // Add aliased fields.
      if ($aliased_fields) {
        foreach ($aliased_fields as $fid) {
          if (!($table_alias = $this->parsed_sql['column_table'][$fid])) {
            $table_alias = $this->parsed_sql['table_aliases'][0];
          }
          $this->output[] = "\t->addField('$table_alias', '" . $this->parsed_sql['column_names'][$fid] . "', '" . $this->parsed_sql['column_aliases'][$fid] . "')";
        }
      }
    }
  }

  /**
   * Convert Insert and Update query fields.
   */
  protected function convertFields($passed_values) {
    $this->output[] = "\t->fields(array(";
    foreach ($this->parsed_sql['column_names'] as $key => $column) {
      if ($this->parsed_sql['values'][$key]['type'] == 'placeholder') {
        $value = array_shift($passed_values);
      }
      else {
        $value = $this->parsed_sql['values'][$key]['value'];
      }
      $this->output[] = "\t\t'$column' => $value,";
    }
    $this->output[] = "\t))";
  }

  /**
   * Convert table joins.
   */
  function convertJoins() {
    foreach ($this->parsed_sql['table_join'] as $key => $table_join) {
      switch ($table_join) {
        case 'join':
        case 'inner join':
          $join_statement = "\t->join(";
          break;

        case 'left join':
          $join_statement = "\t->leftJoin(";
          break;

        case 'right join':
          $join_statement = "\t->rightJoin(";
          break;
      }
      $right_table_alias = $this->getTableAlias($key + 1);
      $left_table_alias = $this->getTableAlias($key);

      $join_statement .= "'" . $this->parsed_sql['table_names'][$key + 1] . "', ";
      $join_statement .= "'" . $right_table_alias . "', ";

      // Add join condition.
      $join_condition = $this->parsed_sql['table_join_clause'][$key + 1];
      if ($join_condition['type'] == 'using') {
        $join_conditions = array();
        foreach ($join_condition['value']['columns'] as $column) {
          $join_conditions[] = $left_table_alias . '.' . $column . ' = ' . $right_table_alias . '.' . $column;
        }
        $join_statement .= "'" . join(' AND ', $join_conditions) . "'";
      }
      else {
        // @todo: extract table names to assign the correct table alias to fields.
        // @todo: check if args are placeholders.
        $join_statement .= "'" . $join_condition['arg_1']['value'] . " " . $join_condition['op'] . " " . $join_condition['arg_2']['value'] . "'";
      }

      $join_statement .= ")";

      $this->output[] = $join_statement;
    }
  }

  /**
   * Transform parsed sql into conditional object strings.
   *
   * @param $conditionals
   *   Parsed conditional array.
   * @param $values
   *   Placeholder values
   * @param $level
   *   Level of conditional statements.
   */
  protected function convertConditionals($conditionals, &$values, $level = 1) {
    $closing_parenthesis = FALSE;

    // Convert operator.
    switch ($conditionals['op']) {
      case 'and':
        if ($level > 1) {
          $this->output[] = str_repeat("\t", $level - 1) . "->condition(db_and()";
          $closing_parenthesis = TRUE;
        }
        break;
      case 'or':
        $this->output[] = str_repeat("\t", $level - 1) . "->condition(db_or()";
        $closing_parenthesis = TRUE;
        break;
      case 'xor':
        $this->output[] = str_repeat("\t", $level - 1) . "->condition(db_xort()";
        $closing_parenthesis = TRUE;
        break;
    }

    // Convert conjunction.
    if (in_array($conditionals['op'], array('and', 'or', 'xor'))) {
      $this->convertConditionalFragment($conditionals['arg_1'], $values, $level);
      $this->convertConditionalFragment($conditionals['arg_2'], $values, $level);
    }
    // Convert single conditional fragment.
    else {
      $this->convertConditionalFragment($conditionals, $values, $level);
    }

    if ($closing_parenthesis) {
      $this->output[] = str_repeat("\t", $level - 1) . ")";
    }
  }

  /**
   * Transform conditional fragment into conditional object string.
   *
   * If conditional fragment is a subclause, pass it back to 
   * coder_upgrade_convert_conditional.
   *
   * @param $conditionals
   *   Parsed conditional fragment array.
   * @param $values
   *   Placeholder values
   * @param $level
   *   Level of conditional statements.
   *
   * @return
   *   Array of conditional object string.
   */
  protected function convertConditionalFragment($fragment, &$values = array(), $level = 1) {
    if ($fragment['arg_1']['type'] == 'subclause') {
      $this->convertConditionals($fragment['arg_1']['value'], $values, $level + 1);
    }
    else {
      $output = str_repeat("\t", $level) . "->condition('" . $fragment['arg_1']['value'];
      if ($fragment['arg_2']['type'] == 'placeholder') {
        $value = array_shift($values);

        // IN operator now just takes an array, rather than imploded array.
        if ($fragment['op'] == 'in' && preg_match("/db_placeholders\(([^,]*).*\)/", $value, $matches)) {
          $value = $matches[1];
        }
      }
      else {
        $value = $fragment['arg_2']['value'];
      }
      $output .=  "', $value";

      // Add operator if necessary.
      if ($fragment['op'] != '=') {
        $output .= ", '" . $fragment['op'] . "'";
      }

      $output .= ")";

      $this->output[] = $output;
    }
  }

  /**
   * Transform parsed having sql into having object strings.
   *
   * @param $having
   *   Parsed having array.
   * @param $values
   *   Placeholder values
   * @param $level
   *   Level of having statements.
   *
   * @todo Refactor with convertCondtionals().
   */
  protected function convertHaving($havings, &$values, $level = 1) {
    $closing_parenthesis = FALSE;

    // Convert operator.
    switch ($havings['op']) {
      case 'and':
        if ($level > 1) {
          $this->output[] = str_repeat("\t", $level - 1) . "->havingCondition(db_and()";
          $closing_parenthesis = TRUE;
        }
        break;
      case 'or':
        $this->output[] = str_repeat("\t", $level - 1) . "->havingCondition(db_or()";
        $closing_parenthesis = TRUE;
        break;
      case 'xor':
        $this->output[] = str_repeat("\t", $level - 1) . "->havingCondition(db_xort()";
        $closing_parenthesis = TRUE;
        break;
    }

    // Convert conjunction.
    if (in_array($havings['op'], array('and', 'or', 'xor'))) {
      $this->convertHavingFragment($havings['arg_1'], $values, $level);
      $this->convertHavingFragment($havings['arg_2'], $values, $level);
    }
    // Convert single conditional fragment.
    else {
      $this->convertHavingFragment($havings, $values, $level);
    }

    if ($closing_parenthesis) {
      $this->output[] = str_repeat("\t", $level - 1) . ")";
    }
  }

  /**
   * Transform having fragment into conditional object string.
   *
   * @param $havings
   *   Parsed having fragment array.
   * @param $values
   *   Placeholder values
   * @param $level
   *   Level of conditional statements.
   *
   * @return
   *   Array of having object string.
   *
   * @todo Refactor with convertConditionalFragment
   */
  protected function convertHavingFragment($fragment, &$values = array(), $level = 1) {
    // If arg_1 or arg_2 is a function, then we have to use the having() function.
    if ($fragment['arg_1']['type'] == 'function' || $fragment['arg_2']['type'] == 'function') {
      $output = str_repeat("\t", $level) . '->having("';
      $output .= $this->formatArg($fragment['arg_1']);
      $output .= ' ' . $fragment['op'] . ' ';
      $output .= $this->formatArg($fragment['arg_2']);
      $output .= '")';
    }
    else {
      $output = str_repeat("\t", $level) . "->havingCondition('" . $fragment['arg_1']['value'];
      if ($fragment['arg_2']['type'] == 'placeholder') {
        $value = array_shift($values);

        // IN operator now just takes an array, rather than imploded array.
        if ($fragment['op'] == 'in' && preg_match("/db_placeholders\(([^,]*).*\)/", $value, $matches)) {
          $value = $matches[1];
        }
      }
      else {
        $value = "'" . $fragment['arg_2']['value'] . "'";
      }
      $output .=  "', $value";

      // Add operator if necessary.
      if ($fragment['op'] != '=') {
        $output .= ", '" . $fragment['op'] . "'";
      }

      $output .= ")";
    }
    $this->output[] = $output;
  }

  /**
   * Get a table alias.
   *
   * If a table alias isn't already set, then generate one.
   *
   * @param $table_key
   *   The key of the table in the parsed sql array.
   *
   * @return
   *   The alias of the table name.
   */
  function getTableAlias($table_key) {
    if (!($alias = $this->parsed_sql['table_aliases'][$table_key])) {
      $alias = $this->generateTableAlias($table_key);
    }
    return $alias;
  }

  /**
   * Generate table alias.
   *
   * Usually the first letter of each word of the table
   * name (e.g. `n` for * `node` or `nr` for `node_revisions`).
   * If an alias is taken, then increments alias.
   *
   * @param $table_key
   *   The key of the table in the parsed sql array.
   *
   * @return
   *   The alias of the table name.
   */
  protected function generateTableAlias($table_key) {
    $table_name = $this->parsed_sql['table_names'][$table_key];
    $table_words = explode('_', $table_name);
    foreach ($table_words as $table_word) {
      $alias .= $table_word[0];
    }
    $original_alias = $alias;
    $i = 0;
    while (in_array($alias, $this->parsed_sql['table_aliases'])) {
      $alias = $original_alias . ++$i;
    }
    $this->parsed_sql['table_aliases'][$table_key] = $alias;
    return $alias;
  }

  /**
   * Format field or argument into a string.
   *
   * Tests if a field or argument is a function, and if so, flattens function
   * into a string.
   *
   * @param $arg
   *   The parsed arg array to format.
   *
   * @return
   *   The formatted string.
   */
  function formatArg($arg) {
    if ($arg['type'] == 'function') {
      $output = $arg['value']['name'] . "(" . join(', ', $arg['value']['arg']) . ")";
    }
    else {
      $output = $arg['value'];
    }
    return $output;
  }
}
