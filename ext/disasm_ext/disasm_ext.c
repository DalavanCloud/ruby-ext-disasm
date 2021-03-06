#include "ruby.h"
#include <libdis.h>

#define LINE_SIZE 1024

static VALUE t_init(VALUE self)
{
    return INT2NUM(x86_init(opt_none, NULL, NULL));
}

static VALUE t_disassemble2yield( 
        VALUE self, 
        VALUE _data, 
        VALUE _rva, 
        VALUE _offset, 
        VALUE _syntax,
        VALUE _allow_invalid)
{
    x86_insn_t insn;
    int size, line_len;
    char line[LINE_SIZE];

    if( !_data || _data == Qnil ) return Qnil;

    char*buf             = RSTRING_PTR(_data);
    unsigned int bufsize = RSTRING_LEN(_data);
    unsigned int rva     = NUM2UINT(_rva);
    unsigned int offset  = NUM2UINT(_offset);
    int syntax           = NUM2INT(_syntax);
    int n_ok             = 0; // number of successfully disassembled instructions
    int allow_invalid    = (_allow_invalid == Qtrue);

    if(!buf || !bufsize) return INT2NUM(0);

    switch(syntax){
        case native_syntax:
        case intel_syntax:
        case att_syntax:
        case xml_syntax:
        case raw_syntax:
            break;
        default:
            // TODO: raise exception
            syntax = native_syntax;
            break;
    }

    while( offset < bufsize ){
        size = x86_disasm(buf, bufsize, rva, offset, &insn);
        if( size || allow_invalid ){
            line_len = x86_format_insn(&insn, line, LINE_SIZE, syntax);
            rb_yield_values(2, rb_str_new(line, line_len), UINT2NUM(offset+rva));
	    if( size ){
                // success
		offset += size;
                n_ok++;
	    } else {
                // invalid instruction : allowed
                offset += 1;
	    }
        } else {
            // invalid instruction : raise exception
            char err_buf[1024];
            sprintf(err_buf, "raise InvalidOpcode.new(0x%x, 0x%x)", offset, offset+rva);
            rb_eval_string(err_buf);
            //rb_raise(ex,"invalid instruction at offset 0x%x (VA 0x%x)", offset, offset+rva);
            break;
        }
    }

    return INT2NUM(n_ok);
}

VALUE mDisasm;

void Init_disasm_ext() {
    x86_init(opt_none, NULL, NULL);
    mDisasm = rb_define_module("Disasm");
    rb_define_singleton_method(mDisasm, "init", t_init, 0);
    rb_define_singleton_method(mDisasm, "disassemble2yield", t_disassemble2yield, 5);
}
