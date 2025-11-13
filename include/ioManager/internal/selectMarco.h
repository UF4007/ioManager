#ifndef IO_SELECETMACRO_H
#define IO_SELECETMACRO_H

#define IO_SELECT_BEGIN(___combinator___) { \
    io::future_tag _____io_select_tag = co_await \
    ___combinator___ ;\
    if (!_____io_select_tag) {

#define IO_SELECT(___fut___) }else if (_____io_select_tag == ___fut___){

#define IO_SELECT_END }}

#endif // IO_SELECETMACRO_H