<?php
// $Id$

/**
 * @file
 */
class SqlDialect {
  // symbol definitions
  var $symbols = array();
  var $commands = array('alter', 'create', 'drop', 'select', 'delete', 'insert', 'update');

  var $operators = array('=', '!=', '<>', '<', '<=', '>', '>=', 'like', 'clike', 'slike', 'not', 'is', 'in', 'between', 'and', 'or');

  var $types = array('character', 'char', 'varchar', 'nchar', 'bit', 'numeric', 'decimal', 'dec', 'integer', 'int', 'smallint', 'float', 'real', 'double', 'date', 'time', 'timestamp', 'interval', 'bool', 'boolean', 'set', 'enum', 'text');

  var $conjunctions = array('by', 'as', 'on', 'into', 'from', 'where', 'with');

  var $functions = array('avg', 'count', 'max', 'min', 'sum', 'nextval', 'currval');

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
    return isset($this->types[$token]);
  }

  //  isFunc()
  function isFunc($token) {
    return isset($this->functions[$token]);
  }

  //  isCommand()
  function isCommand($token) {
    return in_array($token, $this->commands);
  }

  //  isReserved()
  function isReserved($token) {
    return in_array($token, $this->symbols);
  }

  //  isOperator()
  function isOperator($token) {
    return in_array($token, $this->operators);
  }
}
