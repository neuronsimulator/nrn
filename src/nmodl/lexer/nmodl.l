%{
    #include <cstdio>
    #include <cstdlib>
    #include <iostream>
    #include <stdlib.h>
    #include <algorithm>
    #include "ast/ast.hpp"
    #include "lexer/list.hpp"
    #include "lexer/modtoken.hpp"
    #include "parser/nmodl_context.hpp"
    #include "parser/nmodl_parser.hpp"
    #include "utils/stringutils.hpp"

    /* we create position in YY_USER_ACTION */
    Position pos;

    /* for '~', we return different tokens based on the lexical
     * context, store the current context. Note that this is
     * returned from parser in original implementation, revisit
     * this and check if this is sufficient __todo__ */
    static int lexcontext;

    /* where we update the location */
    void update_location(yyscan_t scanner);

    /* YY_USER_ACTION is "called" before each of token actions */
    #define YY_USER_ACTION update_location(yyscanner);

    /* the scanner state include a field called yyextra that can
     * be used for user-defined state. The type of this field is
     * specified by YY_EXTRA_TYPE
     */
    #define YY_EXTRA_TYPE NmodlContext*

    /* we will parse strings as well as files and hence need to
     * redefine YY_INPUT. This will use the istream from the parser
     * context to read the next character. Note that the parser uses
     * uses std::cin by default. We will set to istringstream as
     * when constructing NmodlContext object.
     */
    #define YY_INPUT(buf,result,max_size)       \
    {                                           \
        char c;                                 \
        (*yyextra->is) >> std::noskipws >> c;   \
        if(yyextra->is->eof())                  \
            result = YY_NULL;                   \
        else {                                  \
            buf[0] = c;                         \
            result = 1;                         \
        }                                       \
   }

%}

D   [0-9]
E   [Ee][-+]?{D}+

/* need to provide this option to flex otherwise complain:
 * "error: 'yymore_used_but_not_detected' was not declared in
 * this scope
 */
%option yymore

/* lexer header file */
%option header-file="nmodl_lexer.hpp"

/* lexer implementation file */
%option outfile="nmodl_lexer.cpp"

/* lexer prefix */
%option prefix="Nmodl_"

/* need to be reentrant */
%option reentrant

/* no need for includes */
%option noyywrap

/* need to unput in buffer for custom routines */
%option unput

/* need to input in buffer for custom routines */
%option input

/* not an interactive lexer, takes a file */
%option batch

/* debug, disable for production */
%option debug

/* bison compatible lexer */
%option bison-bridge

/* bison location information */
%option bison-locations

/* keep line information */
%option yylineno

/* mode for verbatim or comment */
%x COPY_MODE


/* mode for DEFINE variable and it's value (i.e. macro definition) */
%x DEFINE_MODE
%x DEFINE_INT_MODE

%%


[a-zA-Z][a-zA-Z0-9_]*'+ {

                            /*PRIME possibly high order*/
                            int yleng = yyget_leng(yyscanner);
                            char nextch =  yyinput(yyscan_t(yyscanner));
                            std::string name(yytext);

                            if(nextch == '0') {
                                name += "0";
                                ModToken *tok = putintoken(yytext, NAME, pos);
                                yylval->name_ptr = new ast::NameNode( new ast::StringNode(name) );
                                yylval->name_ptr->setToken(tok);
                                return NAME;
                            } else {
                                unput(nextch);

                                ModToken *tok = putintoken(name, PRIME, pos);
                                size_t order = std::count(name.begin(), name.end(), '\'');
                                name.erase(std::remove(name.begin(), name.end(), '\''), name.end());
                                yylval->primename_ptr = new ast::PrimeNameNode(new ast::StringNode(name.c_str()), new ast::IntegerNode(order, NULL));
                                yylval->primename_ptr->setToken(tok);
                                return PRIME;
                            }

                        }

WHILE   |
IF      |
ELSE                    {
                            for(char *cp = yytext; *cp; cp++)
                                *cp = tolower(*cp);
                            ModToken *tok = putintoken(yytext, NAME, pos);
                            yylval->qp = tok;
                            return tok->type();
                        }

"VERBATIM"              {   BEGIN(COPY_MODE); }

"COMMENT"               {   BEGIN(COPY_MODE); }

"DEFINE"                {
                            ModToken *tok = putintoken(yytext, NAME, pos);
                            yylval->qp = tok;
                            BEGIN(DEFINE_MODE);
                            return tok->type();
                        }

<DEFINE_MODE>[a-zA-Z][a-zA-Z0-9_]*  {
                                        ModToken *tok = putintoken(yytext, INTEGER, pos);
                                        yylval->name_ptr = new ast::NameNode( new ast::StringNode(yytext) );
                                        yylval->name_ptr->setToken(tok);
                                        BEGIN(DEFINE_INT_MODE);
                                        return NAME;
                                    }

<DEFINE_INT_MODE>{D}+               {
                                        ModToken *tok = putintoken(yytext, INTEGER, pos);
                                        yylval->integer_ptr = new ast::IntegerNode(atoi(yytext), NULL);
                                        yylval->integer_ptr->setToken(tok);
                                        BEGIN(INITIAL);
                                        return INTEGER;
                                    }

[a-zA-Z][a-zA-Z0-9_]*   {
                            ModToken *tok = putintoken(yytext, NAME, pos);
                            yylval->qp = tok;
                            int type = tok->type();

                             /* if the NAME is variable from macro definition then return INTEGER
                              * Make sure yyextra is not NULL because we also use lexer-only for
                              * testing purpose
                              */
                            if(type == NAME && yyextra->is_defined_var(yytext)) {
                                int value = yyextra->get_defined_var_value(yytext);
                                ast::NameNode* macro_name = new ast::NameNode(new ast::StringNode(yytext));
                                macro_name->setToken(tok->clone());
                                yylval->integer_ptr = new ast::IntegerNode(value, macro_name);
                                yylval->integer_ptr->setToken(tok);
                                return INTEGER;
                            }

                            if(type == NAME || type == METHOD || type == SUFFIX || type == VALENCE || type == DEL || type == DEL2) {
                                yylval->name_ptr = new ast::NameNode( new ast::StringNode(yytext) );
                                yylval->name_ptr->setToken(tok);
                            }

                            if( type == NONLINEAR || type == LINEAR || type == PARTIAL || type == KINETIC) {
                                lexcontext = type;
                            }

                            return type;
                        }


{D}+                    {
                            ModToken *tok = putintoken(yytext, INTEGER, pos);
                            yylval->integer_ptr = new ast::IntegerNode(atoi(yytext), NULL);
                            yylval->integer_ptr->setToken(tok);
                            return INTEGER;
                        }

{D}+"."{D}*({E})? |
{D}*"."{D}+({E})? |
{D}+{E}                 {
                            ModToken *tok = putintoken(yytext, REAL, pos);
                            yylval->double_ptr = new ast::DoubleNode(atof(yytext));
                            yylval->double_ptr->setToken(tok);
                            return REAL;
                        }


\"[^\"]*\"              {
                            /* can't quote \" */
                            ModToken *tok = putintoken(yytext, STRING, pos);
                            yylval->string_ptr = new ast::StringNode(yytext);
                            yylval->string_ptr->setToken(tok);
                            return STRING;
                        }

">"                     {
                            ModToken *tok = putintoken(yytext, GT, pos);
                            yylval->qp = tok;
                            return GT;
                        }

">="                    {
                            ModToken *tok = putintoken(yytext, GE, pos);
                            yylval->qp = tok;
                            return GE;
                        }

"<"                     {
                            ModToken *tok = putintoken(yytext, LT, pos);
                            yylval->qp = tok;
                            return LT;
                        }

"<="                    {
                            ModToken *tok = putintoken(yytext, LE, pos);
                            yylval->qp = tok;
                            return LE;
                        }

"=="                    {
                            ModToken *tok = putintoken(yytext, EQ, pos);
                            yylval->qp = tok;
                            return EQ;
                        }

"!="                    {
                            ModToken *tok = putintoken(yytext, NE, pos);
                            yylval->qp = tok;
                            return NE;
                        }

"!"                     {
                            ModToken *tok = putintoken(yytext, NOT, pos);
                            yylval->qp = tok;
                            return NOT;
                        }

"&&"                    {
                            ModToken *tok = putintoken(yytext, AND, pos);
                            yylval->qp = tok;
                            return AND;
                        }

"||"                    {
                            ModToken *tok = putintoken(yytext, OR, pos);
                            yylval->qp = tok;
                            return OR;
                        }

"<->"                   {
                            ModToken *tok = putintoken(yytext, REACT1, pos);
                            yylval->qp = tok;
                            return REACT1;
                        }

"~+"                    {
                            /* syntactic sugar for equation addition */
                            ModToken *tok = putintoken(yytext, NONLIN1, pos);
                            yylval->qp = tok;
                            return NONLIN1;
                        }

"~"                     {
                            /* syntactic sugar for equations */
                            if (lexcontext == NONLINEAR) return NONLIN1;
                            if (lexcontext == LINEAR) return LIN1;
                            if (lexcontext == PARTIAL) return yytext[0];
                            if (lexcontext == KINETIC) return REACTION;
                            std::cout << "\n ERROR: check lexer for ~ \n";
                            abort();
                        }

"{"                     {
                            ModToken *tok = putintoken(yytext, BEGINBLK, pos);
                            yylval->qp = tok;
                            return yytext[0];
                        }

"}"                     {
                            ModToken *tok = putintoken(yytext, ENDBLK, pos);
                            yylval->qp = tok;
                            return yytext[0];
                        }

"("                     {
                            ModToken *tok = putintoken(yytext, LEFT_PAREN, pos);
                            yylval->qp = tok;
                            return yytext[0];
                        }

")"                     {
                            ModToken *tok = putintoken(yytext, RIGHT_PAREN, pos);
                            yylval->qp = tok;
                            return yytext[0];
                        }

[ \t]                   { /*ignore spacing characters*/ }
\r\n                    { yyset_column(1, yyscanner); }
\r                      { yyset_column(1, yyscanner); }
\n.*                    {
                            yyset_column(1, yyscanner);
                            std::string str = std::string(yytext);

                            StringUtils::trim(str);

                            /* if non-empty string, then store it for error reporting */
                            if(str.length()) {
                                StringUtils::trimnewline(str);
                                std::cout << std::endl << "LINE "<< yylineno << ": " << str;
                            } else
                                std::cout << std::endl << "LINE " << yylineno << ": ";

                            /* pass back the entire string except newline character */
                            yyless(1);
                            yyset_column(1, yyscanner);
                        }

:.* |
\?.*                    {
                            /* Need to change parser grammar to accept one line comment
                            std::string str = "COMMENT " + std::string(yytext) + " ENDCOMMENT";
                            yylval->str_ptr = new std::string(str);
                            return COMMENT;
                            */
                        }

.                       {
                            /* @todo: need to put this token */
                            ModToken *tok = putintoken(yytext, 0, pos);
                            yylval->qp = tok;
                            return yytext[0];
                        }

 /* for varbatim mode i.e. comment and verbatim */

<COPY_MODE>[ \t]        { yymore(); }
<COPY_MODE>\n           { yymore(); yyset_column(1, yyscanner); }
<COPY_MODE>\r\n         { yymore(); yyset_column(1, yyscanner); }
<COPY_MODE>\r           { yymore(); yyset_column(1, yyscanner); }

<COPY_MODE>"ENDVERBATIM" {
                            std::string str = "VERBATIM " + std::string(yytext);
                            yylval->str_ptr = new std::string(str);
                            BEGIN(INITIAL);
                            return VERBATIM;
                         }

<COPY_MODE>"ENDCOMMENT" {
                            std::string str = "COMMENT " + std::string(yytext);
                            yylval->str_ptr = new std::string(str);
                            BEGIN(INITIAL);
                            return COMMENT;
                        }


<COPY_MODE><<EOF>>      {
                            std::cout << "\n ERROR: Unexpected end of file in COPY_MODE! \n";
                            return 0;
                        }

<COPY_MODE>.           {
                            yymore();
                        }

    /* @todo: adding new rules here, needed? */

\"[^\"\n]*$             {
                            printf("\n Unterminated String, e.g. for printf ");
                        }

%%

/* some of the utility functions can't be defined
 * outside lexer file due to internal macros. These
 * are utility functions ported from original nocmodl
 * implementation.
 */
namespace LexerUtils {

    /* this routine is needed to input the unit
     * from the UNIT block. yylex cant be used
     * directly. Also, defining this routine outise
     * lexer file gives compilation error as yyunput
     * macros are defined in the lexer generated file
     */
    ast::StringNode * inputtoparpar(void *yyscanner) {
        char lastch;
        std::string str;

        struct yyguts_t * yyg;
        yyg = (struct yyguts_t*)yyscanner;

        int colno = yyget_column(yyscan_t(yyscanner));
        int lineno = yyget_lineno(yyscan_t(yyscanner));

        while (1) {
            lastch =  yyinput(yyscan_t(yyscanner));

            if(lastch == ')') {
                unput(')');
                break;
            }
            else if ( lastch == '\n' || lastch == 0) {
                std::cout << " Error: While parsing unit, closing parenthis not found!   ";
                break;
            }
            str += lastch;
        }

        /* create new position for token */
        pos = Position(lineno, lineno, colno, colno + str.size());

        ModToken *tok = putintoken(str, UNITS, pos);
        ast::StringNode * node = new ast::StringNode(str);
        node->setToken(tok);
        return node;
    }

    /* this routine is needed to input TITLE line. As
     * we are ignoring new line in the lexer, we need
     * custom routine to input a line. (@todo)
     */
    std::string inputline(void *yyscanner) {
        char lastch;
        std::string str;

        /* note yyinput requires yyg and also yyscanner variable */
        struct yyguts_t * yyg;
        yyg = (struct yyguts_t*)yyscanner;

        while (1) {
            lastch =  yyinput(yyscan_t(yyscanner));
            if ( lastch == '\n' || lastch == 0 || lastch == EOF) {
                unput('\n');
                break;
            }
            str += lastch;
        }
        return str;
    }

    /* this routine is needed to input COMMENT and VERBATIM
     * blocks until ENDCOMMENT and ENDVERBATIM respectively
     * @todo: this looses \n and \t which is bad for verbatim
     * blocks. Need to fix this.
     * @TODO: REMOVE THIS, LEFT FOR REFERENCE!!
     */
    std::string input_until_token(std::string w, void *yyscanner) {

        std::string str;
        char *text;

        /* required for yylex */
        YYSTYPE *yytype = yyget_lval(yyscanner);
        YYLTYPE *yltype = yyget_lloc(yyscanner);

        StringUtils::trim(w);

        while(1) {

            /* get a token */
            yylex(yytype, yltype, yyscanner);
            text = yyget_text(yyscanner);

            std::string s(text);
            StringUtils::trim(s);

            /* break if we got expected token */
            if(strcmp(s.c_str(), w.c_str())==0)
                break;

            /* otherwise just append */
            str += text;
        }

        struct yyguts_t * yyg;
        yyg = (struct yyguts_t*)yyscanner;
        BEGIN(INITIAL);
        return str;
    }

    /* used for unit testing where stream is set to string buffer */
    int get_next_token(std::string name, YYSTYPE *stype, YYLTYPE *ltype) {

        int tok;
        yyscan_t scanner;
        YY_BUFFER_STATE buf;
        NmodlContext context;

        yylex_init_extra(&context, &scanner);

        buf = yy_scan_string(name.c_str(), scanner);
        tok = yylex(stype, ltype, scanner);

        yy_delete_buffer(buf, scanner);
        yylex_destroy(scanner);

        return tok;
    }
}

/* initialize nmodl lexer context */
void NmodlContext::init_scanner() {
    yylex_init(&scanner);
    yyset_extra(this, scanner);
}

/* delete nmodl lexer context */
void NmodlContext::destroy_scanner() {
    yylex_destroy(scanner);
}

/* update location information when current token being parsed */
void update_location(yyscan_t scanner) {

    /* get column, line and length og token */
    int colno = yyget_column(scanner);
    int lineno = yyget_lineno(scanner);
    int yleng = yyget_leng(scanner);

    /* create new position for token */
    pos = Position(lineno, lineno, colno, colno+yleng-1);

    /* update scanner state */
    yyset_column(colno+yleng, scanner);
}
