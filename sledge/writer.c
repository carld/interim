#include "minilisp.h"
#include "stream.h"
#include <stdio.h>
#include <stdint.h>

#define TMP_BUF_SIZE 1024
#define INTFORMAT "%ld"

char* tag_to_str(int tag) {
  switch (tag) {
  case TAG_FREED: return "freed"; 
  case TAG_INT: return "int";
  case TAG_CONS: return "cons";
  case TAG_SYM: return "sym";
  case TAG_LAMBDA: return "lambda";
  case TAG_BUILTIN: return "builtin";
  case TAG_BIGNUM: return "bignum";
  case TAG_STR: return "string";
  case TAG_BYTES: return "bytes";
  case TAG_VEC: return "vector";
  case TAG_ERROR: return "error";
  case TAG_ANY: return "any";
  case TAG_VOID: return "void";
  case TAG_STREAM: return "stream";
  case TAG_FS: return "filesystem";
  case TAG_MARK: return "gc_mark";
  default: return "unknown";
  }
}

char* write_(Cell* cell, char* buffer, int in_list, int bufsize) {
  //printf("writing %p (%d) to %p, size: %d\n",cell,cell->tag,buffer,bufsize);
  bufsize--;
  
  buffer[0]=0;
  if (cell == NULL) {
    snprintf(buffer, bufsize, "null");
  } else if (cell->tag == TAG_INT) {
    snprintf(buffer, bufsize, INTFORMAT, cell->value);
  } else if (cell->tag == TAG_CONS) {
    if (cell->addr == 0 && cell->next == 0) {
      if (!in_list) {
        snprintf(buffer, bufsize, "nil");
      }
    } else {
      char tmpl[TMP_BUF_SIZE];
      char tmpr[TMP_BUF_SIZE];
      write_((Cell*)cell->addr, tmpl, 0, TMP_BUF_SIZE);

      if (cell->next && ((Cell*)cell->next)->tag==TAG_CONS) {
        write_((Cell*)cell->next, tmpr, 1, TMP_BUF_SIZE);
        if (in_list) {
          if (tmpr[0]) {
            snprintf(buffer, bufsize, "%s %s", tmpl, tmpr);
          } else {
            snprintf(buffer, bufsize, "%s", tmpl); // skip nil at end of list
          }
        } else {
          // we're head of a list
          snprintf(buffer, bufsize, "(%s %s)", tmpl, tmpr);
        }
      } else {
        write_((Cell*)cell->next, tmpr, 0, TMP_BUF_SIZE);
        // improper list
        snprintf(buffer, bufsize, "(%s.%s)", tmpl, tmpr);
      }
    }
  } else if (cell->tag == TAG_SYM) {
    snprintf(buffer, bufsize, "%s", (char*)cell->addr);
  } else if (cell->tag == TAG_STR) {
    snprintf(buffer, min(bufsize,cell->size+2), "\"%s\"", (char*)cell->addr);
  } else if (cell->tag == TAG_BIGNUM) {
    snprintf(buffer, bufsize, "%s", (char*)cell->addr);
  } else if (cell->tag == TAG_LAMBDA) {
    char tmp_args[TMP_BUF_SIZE];
    char tmp_body[TMP_BUF_SIZE*4];
    Cell* args = car(cell->addr);
    int ai = 0;
    while (args && args->addr && args->next) {
      ai += snprintf(tmp_args+ai, TMP_BUF_SIZE-ai, "%s ", (char*)(car(car(args)))->addr);
      args = cdr(args);
    }
    write_(cdr(cell->addr), tmp_body, 0, TMP_BUF_SIZE);
    snprintf(buffer, bufsize, "(fn %s %s)", tmp_args, tmp_body);
  } else if (cell->tag == TAG_BUILTIN) {
    snprintf(buffer, bufsize, "(op "INTFORMAT")", cell->value);
  } else if (cell->tag == TAG_ERROR) {
    switch (cell->value) {
      case ERR_SYNTAX: snprintf(buffer, bufsize, "<e0:syntax error.>"); break;
      case ERR_MAX_EVAL_DEPTH: snprintf(buffer, bufsize, "<e1:deepest level of evaluation reached.>"); break;
      case ERR_UNKNOWN_OP: snprintf(buffer, bufsize, "<e2:unknown operation.>"); break;
      case ERR_APPLY_NIL: snprintf(buffer, bufsize, "<e3:cannot apply nil.>"); break;
      case ERR_INVALID_PARAM_TYPE: snprintf(buffer, bufsize, "<e4:invalid or no parameter given.>"); break;
      case ERR_OUT_OF_BOUNDS: snprintf(buffer, bufsize, "<e5:out of bounds.>"); break;
      default: snprintf(buffer, bufsize, "<e"INTFORMAT":unknown>", cell->value); break;
    }
  } else if (cell->tag == TAG_BYTES) {
    char* hex_buffer = malloc(cell->size*2+2);
    unsigned int i;
    for (i=0; i<cell->size && i<bufsize; i++) {
      // FIXME: buffer overflow?
      snprintf(hex_buffer+i*2, bufsize-i*2, "%02x",((uint8_t*)cell->addr)[i]);
    }
    snprintf(buffer, bufsize, "\[%s]", hex_buffer);
    free(hex_buffer);
  } else if (cell->tag == TAG_STREAM) {
    Stream* s = (Stream*)cell->addr;
    snprintf(buffer, bufsize, "<stream:%d:%s:%s>", s->id, s->path->addr, s->fs->mount_point->addr);
  } else {
    snprintf(buffer, bufsize, "<tag:%i>", cell->tag);
  }
  return buffer;
}

char* lisp_write(Cell* cell, char* buffer, int bufsize) {
  return write_(cell, buffer, 0, bufsize);
}

Cell* lisp_write_to_cell(Cell* cell, Cell* buffer_cell) {
  if (buffer_cell->tag == TAG_STR || buffer_cell->tag == TAG_BYTES) {
    lisp_write(cell, buffer_cell->addr, buffer_cell->size-1);
  }
  return buffer_cell;
}
