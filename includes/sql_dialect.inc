<?php
// $Id: sql_dialect.inc,v 1.6 2010/06/22 23:00:28 duellj Exp $

/**
 * @file
 */
class SqlDialect {
  // symbol definitions
  public $symbols = array();

  public $commands = array(
    'alter',
    'create',
    'drop',
    'select',
    'delete',
    'insert',
    'update'
  );

  public $operators = array(
    '=',
    '!=',
    '<>',
    '<',
    '<=',
    '>',
    '>=',
    'like',
    'clike',
    'slike',
    'not',
    'is',
    'in',
    'between',
    'and',
    'or'
  );

  public $types = array(
    'character',
    'char',
    'varchar',
    'nchar',
    'bit',
    'numeric',
    'decimal',
    'dec',
    'integer',
    'int',
    'smallint',
    'float',
    'real',
    'double',
    'date',
    'time',
    'timestamp',
    'interval',
    'bool',
    'boolean',
    'set',
    'enum',
    'text'
  );

  public $conjunctions = array(
    'by',
    'as',
    'on',
    'into',
    'from',
    'where',
    'with'
  );

  public $functions = array(
    'avg',
    'count',
    'max',
    'min',
    'sum',
    'nextval',
    'currval'
  );

  var $reserved = array('absolute', 'action', 'add', 'all', 'allocate', 'and', 'any', 'are', 'asc', 'ascending', 'assertion', 'at', 'authorization', 'begin', 'bit_length', 'both', 'cascade', 'cascaded', 'case', 'cast', 'catalog', 'char_length', 'character_length', 'check', 'close', 'coalesce', 'collate', 'collation', 'column', 'commit', 'connect', 'connection', 'constraint', 'constraints', 'continue', 'convert', 'corresponding', 'cross', 'current', 'current_date', 'current_time', 'current_timestamp', 'current_user', 'cursor', 'day', 'deallocate', 'declare', 'default', 'deferrable', 'deferred', 'desc', 'descending', 'describe', 'descriptor', 'diagnostics', 'disconnect', 'distinct', 'domain', 'else', 'end', 'end-exec', 'escape', 'except', 'exception', 'exec', 'execute', 'exists', 'external', 'extract', 'false', 'fetch', 'first', 'for', 'foreign', 'found', 'full', 'get', 'global', 'go', 'goto', 'grant', 'group', 'having', 'hour', 'identity', 'immediate', 'indicator', 'initially', 'inner', 'input', 'insensitive', 'intersect', 'isolation', 'join', 'key', 'language', 'last', 'leading', 'left', 'level', 'limit', 'local', 'lower', 'match', 'minute', 'module', 'month', 'names', 'national', 'natural', 'next', 'no', 'null', 'nullif', 'octet_length', 'of', 'only', 'open', 'option', 'or', 'order', 'outer', 'output', 'overlaps', 'pad', 'partial', 'position', 'precision', 'prepare', 'preserve', 'primary', 'prior', 'privileges', 'procedure', 'public', 'read', 'references', 'relative', 'restrict', 'revoke', 'right', 'rollback', 'rows', 'schema', 'scroll', 'second', 'section', 'session', 'session_user', 'size', 'some', 'space', 'sql', 'sqlcode', 'sqlerror', 'sqlstate', 'substring', 'system_user', 'table', 'temporary', 'then', 'timezone_hour', 'timezone_minute', 'to', 'trailing', 'transaction', 'translate', 'translation', 'trim', 'true', 'union', 'unique', 'unknown', 'upper', 'usage', 'user', 'using', 'value', 'values', 'varying', 'view', 'when', 'whenever', 'work', 'write', 'year', 'zone', 'eoc');

  var $synonyms = array('decimal' => 'numeric', 'dec' => 'numeric', 'numeric' => 'numeric', 'float' => 'float', 'real' => 'real', 'double' => 'real', 'int' => 'int', 'integer' => 'int', 'interval' => 'interval', 'smallint' => 'smallint', 'timestamp' => 'timestamp', 'bool' => 'bool', 'boolean' => 'bool', 'set' => 'set', 'enum' => 'enum', 'text' => 'text', 'char' => 'char', 'character' => 'char', 'varchar' => 'varchar', 'ascending' => 'asc', 'asc' => 'asc', 'descending' => 'desc', 'desc' => 'desc', 'date' => 'date', 'time' => 'time');

  function __construct() {
    $this->symbols = array_flip(array_merge(
      $this->types,
      $this->functions,
      $this->operators,
      $this->commands,
      $this->reserved,
      $this->conjunctions
    ));
  }

  /**
   * Test if given token is in the symbols list.
   */
  function isSymbol($token) {
    return isset($this->symbols[$token]);
  }

  //  isType()
  function isType($token) {
    return in_array($token, $this->types);
  }

  function isVal($token) {
    return (($token == 'real_val') ||
      ($token == 'int_val') ||
      ($token == 'text_val') ||
      ($token == 'placeholder') ||
      ($token == 'NULL'));
  }

  //  isFunc()
  function isFunc($token) {
    return in_array($token, $this->functions);
  }

  //  isCommand()
  function isCommand($token) {
    return in_array($token, $this->commands);
  }

  //  isReserved()
  function isReserved($token) {
    return isset($this->symbols[$token]);
  }

  //  isOperator()
  function isOperator($token) {
    return in_array($token, $this->operators);
  }

  function isSynonym($token) {
    return isset($this->synonyms[$token]);
  }

  function getSynonym($token) {
    return $this->synonyms[$token];
  }
}
