/**
 *    Copyright(c) 2016 rryqszq4
 *
 *
 */

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_globals.h"
#include "php_ngx.h"

#include "../ngx_http_php_module.h"

/* If you declare any globals in php_php_ngx.h uncomment this: */
ZEND_DECLARE_MODULE_GLOBALS(php_ngx)

static int ngx_track_zval(zval zv);
static void ngx_track_znode(unsigned int node_type, znode_op node, zend_op_array *op_array TSRMLS_DC);
static void ngx_track_op_array(zend_op_array *op_array TSRMLS_DC);
static int ngx_track_fe_wrapper(zval *el TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key);
static int ngx_track_cle_wrapper (zval *el TSRMLS_DC);
//static int ngx_check_fe_wrapper (zval *el, zend_bool *have_fe TSRMLS_DC);
static int ngx_track_fe(zend_op_array *fe TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key);
static int ngx_track_cle(zend_class_entry *class_entry TSRMLS_DC);
//static int ngx_check_fe(zend_op_array *fe, zend_bool *have_fe TSRMLS_DC);
static inline zend_class_entry *ngx_get_called_scope(const zend_execute_data *e);
static inline const char *ngx_get_executed_filename(void);
static void ngx_function_name(zend_execute_data *execute_data);

/* True global resources - no need for thread safety here */
//static int le_php_ngx;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("php_ngx.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_php_ngx_globals, php_ngx_globals)
    STD_PHP_INI_ENTRY("php_ngx.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_php_ngx_globals, php_ngx_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_php_ngx_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_php_ngx_compiled)
{
    char *arg = NULL;
    size_t arg_len;
    zend_string *strg;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len) == FAILURE) {
        return;
    }

    strg = strpprintf(0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "php_ngx", arg);

    RETURN_STR(strg);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_php_ngx_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_php_ngx_init_globals(zend_php_ngx_globals *php_ngx_globals)
{
    php_ngx_globals->global_value = 0;
    php_ngx_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(php_ngx)
{
    /* If you have INI entries, uncomment these lines
    REGISTER_INI_ENTRIES();
    */

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(php_ngx)
{
    /* uncomment this line if you have INI entries
    UNREGISTER_INI_ENTRIES();
    */
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(php_ngx)
{
#if defined(COMPILE_DL_PHP_NGX) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    //PHP_NGX_G(global_r) = NULL;

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(php_ngx)
{
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(php_ngx)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "php_ngx support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
    */
}
/* }}} */

/* {{{ php_ngx_functions[]
 *
 * Every user visible function must have an entry in php_ngx_functions[].
 */
const zend_function_entry php_ngx_functions[] = {
    PHP_FE(confirm_php_ngx_compiled,    NULL)       /* For testing, remove later. */
    PHP_FE_END  /* Must be the last line in php_ngx_functions[] */
};
/* }}} */

/* {{{ php_ngx_module_entry
 */
zend_module_entry php_ngx_module_entry = {
    STANDARD_MODULE_HEADER,
    "php_ngx",
    php_ngx_functions,
    PHP_MINIT(php_ngx),
    PHP_MSHUTDOWN(php_ngx),
    PHP_RINIT(php_ngx),     /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(php_ngx), /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(php_ngx),
    PHP_PHP_NGX_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PHP_NGX
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(php_ngx)
#endif

#if defined(PHP_WIN32) && defined(ZTS)
ZEND_TSRMLS_CACHE_DEFINE()
#endif

static int php_ngx_startup(sapi_module_struct *sapi_module)
{
    if (php_module_startup(sapi_module, NULL, 0) == FAILURE){
        return FAILURE;
    }
    return SUCCESS;
}

static int php_ngx_deactivate(TSRMLS_D)
{
    return SUCCESS;
}

static size_t php_ngx_ub_write(const char *str, size_t str_length TSRMLS_DC)
{
    return str_length;
}

static void php_ngx_flush(void *server_context)
{
}

static int php_ngx_header_handler(sapi_header_struct *sapi_header, sapi_header_op_enum op, sapi_headers_struct *sapi_headers TSRMLS_DC)
{
    return 0;
}

static size_t php_ngx_read_post(char *buffer, size_t count_bytes TSRMLS_DC)
{
    return 0;
}

static char* php_ngx_read_cookies(TSRMLS_D)
{
    return NULL;
}

static void php_ngx_register_variables(zval *track_vars_array TSRMLS_DC)
{
    php_import_environment_variables(track_vars_array TSRMLS_CC);

    /*if (SG(request_info).request_method) {
        php_register_variable("REQUEST_METHOD", (char *)SG(request_info).request_method, track_vars_array TSRMLS_CC);
    }
    if (SG(request_info).request_uri){
        php_register_variable("DOCUMENT_URI", (char *)SG(request_info).request_uri, track_vars_array TSRMLS_CC);
    }
    if (SG(request_info).query_string){
        php_register_variable("QUERY_STRING", (char *)SG(request_info).query_string, track_vars_array TSRMLS_CC);
    }*/
}

/*static void php_ngx_log_message(char *message)
{
}*/

sapi_module_struct php_ngx_module = {
    "php7-ngx",                       /* name */
    "PHP Embedded Library for nginx-module",        /* pretty name */

    php_ngx_startup,              /* startup */
    php_module_shutdown_wrapper,   /* shutdown */

    NULL,                          /* activate */
    php_ngx_deactivate,           /* deactivate */

    php_ngx_ub_write,             /* unbuffered write */
    php_ngx_flush,                /* flush */
    NULL,                          /* get uid */
    NULL,                          /* getenv */

    php_error,                     /* error handler */

    php_ngx_header_handler,                          /* header handler */
    NULL,                          /* send headers handler */
    NULL,          /* send header handler */

    php_ngx_read_post,                          /* read POST data */
    php_ngx_read_cookies,         /* read Cookies */

    php_ngx_register_variables,   /* register server variables */
    NULL,          /* Log message */
    NULL,                           /* Get request time */
    NULL,                           /* Child terminate */

    "",                             /* php_ini_path_override */

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,

    0,
    0,

    NULL,

    NULL,

    NULL,
    NULL,

    NULL,

    NULL,
    0,

    NULL,
    NULL,
    NULL
};
/* }}} */

/* {{{ arginfo ext/standard/dl.c */
ZEND_BEGIN_ARG_INFO(arginfo_dl, 0)
    ZEND_ARG_INFO(0, extension_filename)
ZEND_END_ARG_INFO()
/* }}} */

static const zend_function_entry additional_functions[] = {
    ZEND_FE(dl, arginfo_dl)
    {NULL, NULL, NULL, 0, 0}
};

int php_ngx_module_init()
{
    zend_llist global_vars;
/*#ifdef ZTS
    void ***tsrm_ls = NULL;
#endif*/

#ifdef HAVE_SIGNAL_H
#if defined(SIGPIPE) && defined(SIG_IGN)
    signal(SIGPIPE, SIG_IGN); /* ignore SIGPIPE in standalone mode so
                                 that sockets created via fsockopen()
                                 don't kill PHP if the remote site
                                 closes it.  in apache|apxs mode apache
                                 does that for us!  thies@thieso.net
                                 20000419 */
#endif
#endif

#ifdef ZTS
  tsrm_startup(1, 1, 0, NULL);
  (void)ts_resource(0);
  ZEND_TSRMLS_CACHE_UPDATE();
#endif

#ifdef ZEND_SIGNALS
    zend_signal_startup();
#endif

  sapi_startup(&php_ngx_module);

#if (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION > 3)
  php_ngx_module.php_ini_ignore_cwd = 1;
#endif

#ifdef PHP_WIN32
  _fmode = _O_BINARY;           /*sets default for file streams to binary */
  setmode(_fileno(stdin), O_BINARY);        /* make the stdio mode be binary */
  setmode(_fileno(stdout), O_BINARY);       /* make the stdio mode be binary */
  setmode(_fileno(stderr), O_BINARY);       /* make the stdio mode be binary */
#endif

  php_ngx_module.additional_functions = additional_functions;

  php_ngx_module.executable_location = NULL;

  if (php_ngx_module.startup(&php_ngx_module) == FAILURE){
    return FAILURE;
  }

  zend_llist_init(&global_vars, sizeof(char *), NULL, 0); 

  return SUCCESS;
}

int php_ngx_request_init(TSRMLS_D)
{
    if (php_request_startup(TSRMLS_C)==FAILURE) {
        return FAILURE;
    }

    SG(headers_sent) = 0;
    SG(request_info).no_headers = 1;
    php_register_variable("PHP_SELF", "-", NULL TSRMLS_CC);

    return SUCCESS;
}

void php_ngx_request_shutdown(TSRMLS_D)
{
    SG(headers_sent) = 1;
    php_request_shutdown((void *)0);
}

void php_ngx_module_shutdown(TSRMLS_D)
{
    php_module_shutdown(TSRMLS_C);
    sapi_shutdown();
#ifdef ZTS
    tsrm_shutdown();
#endif
    if (php_ngx_module.ini_entries){
        free(php_ngx_module.ini_entries);
        php_ngx_module.ini_entries = NULL;
    }
}



zend_op_array *ngx_compile_string(zval *source_string, char *filename TSRMLS_DC)
{
    zend_op_array *op_array;

    op_array = ori_compile_string(source_string, filename TSRMLS_CC);

    ngx_http_request_t *r = ngx_php_request;
    ngx_http_php_ctx_t *ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_php_module);

    if (op_array && (ctx->output_type & OUTPUT_OPCODE)) {
        ctx->output_type = OUTPUT_CONTENT;

        //ngx_http_set_ctx(r, ctx, ngx_http_php_module);
php_printf("    ___                   __    \n");
php_printf("  /`__ \\___  ___  ___  __/ /__  \n");
php_printf(" / /_/ / _ \\/ _ `/ _ \\/ _ / __\\ \n");
php_printf(" \\___./ .__/\\___.\\___/\\___\\__..   /ngx_php7_tracker\n"); 
php_printf("     /_/                         /version: %s\n", NGX_HTTP_PHP_MODULE_VERSION);
php_printf("\n/* ~: IS_TMP_VAR, $: IS_VAR, !: IS_CV */\n\n");
        ngx_track_op_array(op_array TSRMLS_CC);

        zend_hash_apply_with_arguments(CG(function_table) TSRMLS_CC, (apply_func_args_t) ngx_track_fe_wrapper, 0);
    
        zend_hash_apply(CG(class_table), (apply_func_t) ngx_track_cle_wrapper TSRMLS_CC);
        
        ctx->output_type = OUTPUT_OPCODE;

        ngx_http_set_ctx(r, ctx, ngx_http_php_module);
    }

    //ctx->enable_output = 0;

    //ngx_http_set_ctx(r, ctx, ngx_http_php_module);

    return op_array;
}

static int ngx_track_zval(zval zv)
{
    switch (zv.u1.v.type) {
        case IS_UNDEF: 
            php_printf("%-16s","<undef>");
            return IS_UNDEF;
        case IS_NULL : 
            php_printf("%-16s","<null>");
            return IS_NULL;
        case IS_FALSE : 
            php_printf("%-16s","<false>");
            return IS_FALSE;
        case IS_TRUE : 
            php_printf("%-16s","<true>");
            return IS_TRUE;
        case IS_LONG : 
            php_printf("%-16ld", zv.value.lval);
            return IS_LONG;
        case IS_DOUBLE : 
            php_printf("%-16g", zv.value.dval);
            return IS_DOUBLE;
        case IS_STRING: 
            php_printf("%-16s", ZSTR_VAL(zv.value.str));
            return IS_STRING;
        case IS_ARRAY: 
            php_printf("%-16s","<array>");
            return IS_ARRAY;
        case IS_OBJECT: 
            php_printf("%-16s","<object>");
            return IS_OBJECT;
        case IS_RESOURCE: 
            php_printf("%-16s","<resource>");
            return IS_RESOURCE;
        case IS_REFERENCE: 
            php_printf("%-16s","<reference>");
            return IS_REFERENCE;
        case IS_CONSTANT: 
            php_printf("%-16s","<constant>");
            return IS_CONSTANT;
        case IS_CALLABLE: 
            php_printf("%-16s","<callable>");
            return IS_CALLABLE;
        case IS_INDIRECT: 
            php_printf("%-16s","<indirect>");
            return IS_INDIRECT;
        case IS_PTR: 
            php_printf("%-16s","<prt>");
            return IS_PTR;
        default: 
            php_printf("%-16s","<unknown>");
            return -1;
    }
}

static void ngx_track_znode(unsigned int node_type, znode_op node, zend_op_array *op_array TSRMLS_DC)
{
    switch (node_type) {
        case IS_UNDEF:
            ngx_track_zval(*RT_CONSTANT_EX(op_array->literals, node));
            break;
        case IS_CONST:
            ngx_track_zval(*RT_CONSTANT_EX(op_array->literals, node));
            break;
        case IS_TMP_VAR:
            php_printf("~%-15d", EX_VAR_TO_NUM(node.var));
            break;
        case IS_VAR:
            php_printf("$%-15d", EX_VAR_TO_NUM(node.var));
            break;
        case IS_CV:
            php_printf("!%-15d", EX_VAR_TO_NUM(node.var));
            break;
        default:
            php_printf("%-16s", " ");
            break;
    }
}

static void ngx_track_op_array(zend_op_array *op_array TSRMLS_DC)
{
    unsigned int i;
    zend_op op;

    php_printf("filename: '%s'\n", op_array->filename?ZSTR_VAL(op_array->filename):NULL);

    if (op_array->scope) {
        php_printf("function_name: %s::%s\n", ZSTR_VAL(op_array->scope->name), op_array->function_name?ZSTR_VAL(op_array->function_name):NULL);
    }else {
        php_printf("function_name: %s\n", op_array->function_name?ZSTR_VAL(op_array->function_name):NULL);
    }

    php_printf("    %-6s%-6s%-38s%-16s%-16s%-16s\n","id","line","opcode","op1","op2","result");
    php_printf("    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

    for (i = 1; i < op_array->last; i++) {
        op = op_array->opcodes[i];
        php_printf("    %-6d%-6d%-38s", 
            i, 
            op.lineno, 
            zend_get_opcode_name(op.opcode)
            //op.op1_type,
            //op.op2_type,
            //op.result_type
            );
        ngx_track_znode(op.op1_type, op.op1, op_array TSRMLS_CC);
        ngx_track_znode(op.op2_type, op.op2, op_array TSRMLS_CC);
        ngx_track_znode(op.result_type, op.result, op_array TSRMLS_CC);
        php_printf("\n");
    }

    php_printf("\n");
}

static int ngx_track_fe_wrapper(zval *el TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key)
{
    return ngx_track_fe((zend_op_array *) Z_PTR_P(el) TSRMLS_CC, num_args, args, hash_key);
}

static int ngx_track_cle_wrapper (zval *el TSRMLS_DC)
{
    return ngx_track_cle((zend_class_entry *) Z_PTR_P(el) TSRMLS_CC);
}

/*static int ngx_check_fe_wrapper (zval *el, zend_bool *have_fe TSRMLS_DC)
{
    return ngx_check_fe((zend_op_array *) Z_PTR_P(el), have_fe TSRMLS_CC);
}*/

static int ngx_track_fe(zend_op_array *fe TSRMLS_DC, int num_args, va_list args, zend_hash_key *hash_key)
{
    if (fe->type == ZEND_USER_FUNCTION) {
        ngx_track_op_array(fe TSRMLS_CC);
    }

    return ZEND_HASH_APPLY_KEEP;
}

static int ngx_track_cle(zend_class_entry *class_entry TSRMLS_DC)
{
    zend_class_entry *ce;
    //zend_bool have_fe = 0;

    ce = class_entry;

    if (ce->type != ZEND_INTERNAL_CLASS) {
        //zend_hash_apply_with_arguments(&ce->function_table, (apply_func_args_t) ngx_check_fe_wrapper, (int )&have_fe TSRMLS_CC);

        //if (have_fe) {
            zend_hash_apply_with_arguments(&ce->function_table TSRMLS_CC, (apply_func_args_t) ngx_track_fe_wrapper, 0);
        //}
    }

    return ZEND_HASH_APPLY_KEEP; 
}

/*static int ngx_check_fe(zend_op_array *fe, zend_bool *have_fe TSRMLS_DC)
{
    if (fe->type == ZEND_USER_FUNCTION) {
        *have_fe = 1;
    }

    return 0;
}*/

static inline zend_class_entry *ngx_get_called_scope(const zend_execute_data *e)
{
#if PHP_VERSION_ID < 70100
    return e->called_scope;
#else
    return zend_get_called_scope((zend_execute_data *) e);
#endif
}

static inline const char *ngx_get_executed_filename(void)
{
    zend_execute_data *ex = EG(current_execute_data);

    while (ex && (!ex->func || !ZEND_USER_CODE(ex->func->type))) {
        ex = ex->prev_execute_data;
    }
    if (ex) {
        return ZSTR_VAL(ex->func->op_array.filename);
    } else {
        return zend_get_executed_filename();
    }
}

static void ngx_function_name(zend_execute_data *execute_data)
{
    zend_execute_data *data;
    zend_function *curr_func;
    const char * cls;
    const char * func;
    const zend_op *opline;

    data = EG(current_execute_data);

    if (data) {
        curr_func = data->func;
        if (curr_func->common.function_name) {
            func = curr_func->common.function_name->val;
            cls = curr_func->common.scope ?
                    curr_func->common.scope->name->val :
                    (ngx_get_called_scope(data) ?
                            ngx_get_called_scope(data)->name->val : NULL);
            if (cls) {
                php_printf("%s::%-20s\n", cls, func);
            }else {
                php_printf("%-30s\n", func);
            }
        }else {
            if (data->prev_execute_data) {
                opline = data->prev_execute_data->opline;
            }else {
                opline = data->opline;
            }

            switch (opline->extended_value) {
                case ZEND_EVAL:
                    php_printf("%-30s", "eval");
                    break;
                case ZEND_INCLUDE:
                    php_printf("%-30s", "include");
                    break;
                case ZEND_REQUIRE:
                    php_printf("%-30s", "require");
                    break;
                case ZEND_INCLUDE_ONCE:
                    php_printf("%-30s", "include_once");
                    break;
                case ZEND_REQUIRE_ONCE:
                    php_printf("%-30s", "require_once");
                    break;
                default:
                    php_printf("%-30s", "main");
                    break;
            }
            php_printf("\n");
        }
    }
}

void ngx_execute_ex(zend_execute_data *execute_data TSRMLS_DC)
{
    int lineno;
    const char *filename;

    ngx_http_request_t *r = ngx_php_request;
    ngx_http_php_ctx_t *ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_php_module);

    if (ctx->output_type & OUTPUT_STACK) {
        ctx->output_type = OUTPUT_CONTENT;

        //ngx_http_set_ctx(r, ctx, ngx_http_php_module);
        
        lineno = zend_get_executed_lineno();
        php_printf("%-6d", lineno);

        filename = ngx_get_executed_filename();
        php_printf("%-60s", filename);

        ngx_function_name(execute_data);

        ctx->output_type = OUTPUT_STACK;

        ngx_http_set_ctx(r, ctx, ngx_http_php_module);
    }

    ori_execute_ex(execute_data TSRMLS_CC);
}

void ngx_execute_internal(zend_execute_data *execute_data, zval *return_value TSRMLS_DC)
{
    int lineno;
    const char *filename;

    ngx_http_request_t *r = ngx_php_request;
    ngx_http_php_ctx_t *ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_php_module);

    if (ctx->output_type & OUTPUT_STACK) {
        ctx->output_type = OUTPUT_CONTENT;

        //ngx_http_set_ctx(r, ctx, ngx_http_php_module);
        
        lineno = zend_get_executed_lineno();
        php_printf("%-6d", lineno);

        filename = ngx_get_executed_filename();
        php_printf("%-60s", filename);

        ngx_function_name(execute_data);

        ctx->output_type = OUTPUT_STACK;

        ngx_http_set_ctx(r, ctx, ngx_http_php_module);
    }

    execute_internal(execute_data, return_value);
}

