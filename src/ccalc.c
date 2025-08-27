// calc.c - Simple terminal calculator with menu, history, and UTF-8 boxes
// Build:   gcc -std=c99 -O2 -Wall -Wextra -pedantic ccalc.c -lm -o ccalc
// Usage:   ./ccalc
// Notes:   Pure C (C99). No external deps. UTF-8 output. Works on Debian-based distros.

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <locale.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

// ------------------------- Utility -------------------------
static void clear_input_buffer(void){
    int c; while((c = getchar()) != '\n' && c != EOF){}
}

static void press_enter_to_continue(void){
    printf("\nPress Enter to return to the menu...");
    fflush(stdout);
    int ch = getchar();
    if(ch != '\n') clear_input_buffer();
}

static void print_box(const char *title, const char *body){
    // UTF-8 box
    printf("\n\xE2\x95\x94"); // ╔
    for(size_t i=0;i<26;i++) putchar('\xE2'), putchar('\x95'), putchar('\x90'); // ═
    printf("\xE2\x95\x97\n"); // ╗
    printf("\xE2\x95\x91 %-24s \xE2\x95\x91\n", title); // ║ title ║
    printf("\xE2\x95\xA0"); // ╠
    for(size_t i=0;i<26;i++) putchar('\xE2'), putchar('\x95'), putchar('\x90');
    printf("\xE2\x95\xA3\n"); // ╣
    if(body && *body){
        // print body lines wrapped at 60 chars roughly
        const char *p = body;
        char line[61];
        while(*p){
            size_t n=0; while(p[n] && p[n] != '\n' && n < 60) n++;
            snprintf(line,sizeof(line),"%.*s",(int)n,p);
            printf("\xE2\x95\x91 %-60s \xE2\x95\x91\n", line);
            p += n;
            if(*p=='\n') { p++; }
        }
    } else {
        printf("\xE2\x95\x91 %-60s \xE2\x95\x91\n"," ");
    }
    printf("\xE2\x95\x9A"); // ╚
    for(size_t i=0;i<60;i++) putchar('\xE2'), putchar('\x95'), putchar('\x90');
    printf("\xE2\x95\x9D\n\n"); // ╝
}

static void print_menu(void){
    puts("\n\xE2\x95\x94\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x97");
    puts("\xE2\x95\x91      CALCULATOR        \xE2\x95\x91");
    puts("\xE2\x95\xA0\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\xA3");
    puts("\xE2\x95\x91 (0) Start Calculator   \xE2\x95\x91");
    puts("\xE2\x95\x91 (1) View Calc History  \xE2\x95\x91");
    puts("\xE2\x95\x91 (2) Exit               \xE2\x95\x91");
    puts("\xE2\x95\x9A\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D");
}

// ------------------------- History -------------------------
typedef struct { char **items; size_t count; size_t cap; } History;

static void history_init(History *h){ h->items=NULL; h->count=0; h->cap=0; }
static void history_free(History *h){
    for(size_t i=0;i<h->count;i++) free(h->items[i]);
    free(h->items);
    h->items=NULL; h->count=0; h->cap=0;
}
static void history_add(History *h, const char *line){
    if(h->count+1 > h->cap){
        size_t ncap = h->cap? h->cap*2 : 16;
        char **n = (char**)realloc(h->items, ncap*sizeof(char*));
        if(!n) return; h->items = n; h->cap = ncap;
    }
    h->items[h->count++] = strdup(line?line:"");
}

static const char* history_path(void){
    static char path[1024];
    const char *home = getenv("HOME");
    if(!home) home = ".";
    snprintf(path,sizeof(path),"%s/.ccalc_history.txt", home);
    return path;
}

static void history_load(History *h){
    FILE *f = fopen(history_path(), "r");
    if(!f) return;
    char *line = NULL; size_t len=0; ssize_t n;
    while((n=getline(&line,&len,f))!=-1){
        if(n>0 && (line[n-1]=='\n' || line[n-1]=='\r')) line[n-1]='\0';
        history_add(h,line);
    }
    free(line);
    fclose(f);
}

static void history_save(const History *h){
    FILE *f = fopen(history_path(), "w");
    if(!f) return;
    for(size_t i=0;i<h->count;i++) fprintf(f, "%s\n", h->items[i]);
    fclose(f);
}

// ------------------------- Superscript Normalization -------------------------
// Convert unicode superscripts (²³⁴…⁹ and ¹ and ⁰ and ⁻) into ^<digits>
static int is_superscript(unsigned int cp){
    // U+2070 ⁰, U+00B9 ¹, U+00B2 ², U+00B3 ³, U+2074..U+2079 ⁴..⁹, U+207B ⁻
    return cp==0x2070 || cp==0x00B9 || cp==0x00B2 || cp==0x00B3 ||
           (cp>=0x2074 && cp<=0x2079) || cp==0x207B;
}
static char superscript_to_ascii(unsigned int cp){
    switch(cp){
        case 0x2070: return '0';
        case 0x00B9: return '1';
        case 0x00B2: return '2';
        case 0x00B3: return '3';
        case 0x2074: return '4';
        case 0x2075: return '5';
        case 0x2076: return '6';
        case 0x2077: return '7';
        case 0x2078: return '8';
        case 0x2079: return '9';
        case 0x207B: return '-';
        default: return '?';
    }
}

static char* normalize_superscripts(const char *in){
    // Assumes UTF-8 input. Produce new malloc'd ASCII-ish string with ^exponent
    size_t n = strlen(in);
    char *out = (char*)malloc(n*4 + 8); // generous
    if(!out) return NULL;
    size_t oi=0; int in_super=0; int inserted_caret=0;
    for(size_t i=0;i<n;){
        unsigned char c = (unsigned char)in[i];
        unsigned int cp=0; size_t adv=1;
        if(c < 0x80){ cp=c; adv=1; }
        else if((c>>5)==0x6){ if(i+1<n){ cp=((c&0x1F)<<6)|((in[i+1]&0x3F)); adv=2; } }
        else if((c>>4)==0xE){ if(i+2<n){ cp=((c&0x0F)<<12)|((in[i+1]&0x3F)<<6)|((in[i+2]&0x3F)); adv=3; } }
        else if((c>>3)==0x1E){ if(i+3<n){ cp=((c&0x07)<<18)|((in[i+1]&0x3F)<<12)|((in[i+2]&0x3F)<<6)|((in[i+3]&0x3F)); adv=4; } }

        if(is_superscript(cp)){
            if(!in_super){
                // start of superscript run -> inject caret '^'
                out[oi++] = '^';
                inserted_caret=1; in_super=1;
            }
            out[oi++] = superscript_to_ascii(cp);
        } else {
            if(in_super){ in_super=0; inserted_caret=0; }
            // copy original bytes
            for(size_t k=0;k<adv;k++) out[oi++] = in[i+k];
        }
        i += adv;
    }
    out[oi]='\0';
    return out;
}

// ------------------------- Tokenizer & Parser (Shunting-yard) -------------------------
typedef enum { T_NUMBER, T_OP, T_LPAREN, T_RPAREN, T_END, T_INVALID } TokType;

typedef struct { TokType type; double value; char op; } Token;

typedef struct { const char *s; size_t i; } Lexer;

static void skip_ws(Lexer *lx){ while(isspace((unsigned char)lx->s[lx->i])) lx->i++; }

static Token next_token(Lexer *lx){
    skip_ws(lx);
    char c = lx->s[lx->i];
    if(c=='\0') return (Token){T_END,0,0};
    if(c=='('){ lx->i++; return (Token){T_LPAREN,0,'('}; }
    if(c==')'){ lx->i++; return (Token){T_RPAREN,0,')'}; }
    // number: [+-]?\d*(\.\d+)?([eE][+-]?\d+)?
    if(isdigit((unsigned char)c) || c=='.' || ((c=='+'||c=='-') && (isdigit((unsigned char)lx->s[lx->i+1]) || lx->s[lx->i+1]=='.'))){
        char *end=NULL; errno=0; double v = strtod(&lx->s[lx->i], &end);
        if(&lx->s[lx->i]==end) return (Token){T_INVALID,0,0};
        lx->i = (size_t)(end - lx->s);
        return (Token){T_NUMBER,v,0};
    }
    // operators
    if(c=='+'||c=='-'||c=='*'||c=='/'||c=='^'){
        lx->i++; return (Token){T_OP,0,c};
    }
    // constants: pi, e (case-insensitive)
    if(strncasecmp(&lx->s[lx->i], "pi", 2)==0){ lx->i+=2; return (Token){T_NUMBER,M_PI,0}; }
    if((lx->s[lx->i]=='e') || (lx->s[lx->i]=='E')){ lx->i+=1; return (Token){T_NUMBER,M_E,0}; }

    return (Token){T_INVALID,0,0};
}

static int precedence(char op){
    switch(op){ case '^': return 4; case '*': case '/': return 3; case '+': case '-': return 2; default: return 0; }
}
static int right_associative(char op){ return op=='^'; }

// Simple stack types

typedef struct { double *v; size_t n,cap; } DStack;
typedef struct { char *v; size_t n,cap; } OStack;

static void ds_init(DStack *s){ s->v=NULL; s->n=0; s->cap=0; }
static void ds_push(DStack *s,double x){ if(s->n==s->cap){ size_t nc=s->cap? s->cap*2:32; s->v=realloc(s->v,nc*sizeof(double)); s->cap=nc;} s->v[s->n++]=x; }
static int ds_pop(DStack *s,double *out){ if(s->n==0) return 0; *out=s->v[--s->n]; return 1; }
static void ds_free(DStack *s){ free(s->v); s->v=NULL; s->n=s->cap=0; }

static void os_init(OStack *s){ s->v=NULL; s->n=0; s->cap=0; }
static void os_push(OStack *s,char c){ if(s->n==s->cap){ size_t nc=s->cap? s->cap*2:32; s->v=realloc(s->v,nc*sizeof(char)); s->cap=nc;} s->v[s->n++]=c; }
static int os_pop(OStack *s,char *out){ if(s->n==0) return 0; *out=s->v[--s->n]; return 1; }
static int os_peek(OStack *s,char *out){ if(s->n==0) return 0; *out=s->v[s->n-1]; return 1; }
static void os_free(OStack *s){ free(s->v); s->v=NULL; s->n=s->cap=0; }

static int apply_op(DStack *vals, char op, char *err, size_t errsz){
    double b,a;
    if(!ds_pop(vals,&b) || !ds_pop(vals,&a)){
        snprintf(err,errsz,"Malformed expression (missing operands)");
        return 0;
    }
    double r=0.0;
    switch(op){
        case '+': r = a+b; break;
        case '-': r = a-b; break;
        case '*': r = a*b; break;
        case '/': if(b==0.0){ snprintf(err,errsz,"Division by zero"); return 0; } r = a/b; break;
        case '^': r = pow(a,b); break;
        default: snprintf(err,errsz,"Unknown operator '%c'",op); return 0;
    }
    if(isnan(r) || isinf(r)){
        snprintf(err,errsz,"Numeric overflow/invalid result");
        return 0;
    }
    ds_push(vals,r);
    return 1;
}

static int evaluate_expression(const char *expr, double *out_value, char *err, size_t errsz){
    Lexer lx = { expr, 0 };
    DStack vals; OStack ops; ds_init(&vals); os_init(&ops);
    int ok=1; int expect_unary=1; // allow leading unary +/-
    for(;;){
        Token t = next_token(&lx);
        if(t.type==T_INVALID){ ok=0; snprintf(err,errsz,"Invalid token near index %zu", lx.i); break; }
        if(t.type==T_END) break;
        if(t.type==T_NUMBER){ ds_push(&vals, t.value); expect_unary=0; continue; }
        if(t.type==T_LPAREN){ os_push(&ops,'('); expect_unary=1; continue; }
        if(t.type==T_RPAREN){
            char top;
            while(os_peek(&ops,&top) && top!='('){ if(!apply_op(&vals, top, err, errsz)){ ok=0; goto done; } os_pop(&ops,&top); }
            if(!os_peek(&ops,&top) || top!='('){ ok=0; snprintf(err,errsz,"Mismatched parentheses"); goto done; }
            os_pop(&ops,&top); expect_unary=0; continue; }
        if(t.type==T_OP){
            char op = t.op;
            // handle unary plus/minus by injecting 0 op number pattern
            if(expect_unary && (op=='+'||op=='-')){ ds_push(&vals, 0.0); }
            char top;
            while(os_peek(&ops,&top) && top!='(' && ( precedence(top) > precedence(op) || (precedence(top)==precedence(op) && !right_associative(op)) )){
                if(!apply_op(&vals, top, err, errsz)){ ok=0; goto done; }
                os_pop(&ops,&top);
            }
            os_push(&ops, op); expect_unary=1; continue;
        }
    }
    {
        char top;
        while(os_pop(&ops,&top)){
            if(top=='(' || top==')'){ ok=0; snprintf(err,errsz,"Mismatched parentheses"); goto done; }
            if(!apply_op(&vals, top, err, errsz)){ ok=0; goto done; }
        }
        double result=0.0; if(!ds_pop(&vals,&result) || vals.n!=0){ ok=0; snprintf(err,errsz,"Malformed expression"); goto done; }
        *out_value = result;
    }

done:
    ds_free(&vals); os_free(&ops);
    return ok;
}

// ------------------------- Calculator Loop -------------------------
static void start_calculator(History *hist){
    print_box("CALCULATOR MODE",
              "Type an expression and press Enter.\n"
              "Examples: 45.235*(45-7.8576)/56^3 or 56³\n"
              "Constants: pi, e. Operators: + - * / ^ and ( )\n"
              "Commands: 'back' to menu, 'quit' to exit.");
    char *line=NULL; size_t len=0; ssize_t n;
    for(;;){
        printf("> "); fflush(stdout);
        n = getline(&line,&len,stdin);
        if(n==-1) { puts(""); break; }
        if(n>0 && (line[n-1]=='\n' || line[n-1]=='\r')) line[n-1]='\0';
        // trim leading spaces
        char *p=line; while(isspace((unsigned char)*p)) p++;
        if(*p=='\0') continue;
        if(strcasecmp(p,"back")==0){ break; }
        if(strcasecmp(p,"quit")==0 || strcasecmp(p,"exit")==0){ free(line); exit(0); }
        char *norm = normalize_superscripts(p);
        if(!norm){ print_box("ERROR","Out of memory"); continue; }
        double val=0.0; char err[128];
        if(evaluate_expression(norm,&val,err,sizeof(err))){
            char rec[256]; snprintf(rec,sizeof(rec),"%s = %.15g", p, val);
            history_add(hist, rec);
            printf("= %.15g\n", val);
        } else {
            char msg[200]; snprintf(msg,sizeof(msg),"%s", err);
            print_box("SYNTAX ERROR", msg);
            char rec[256]; snprintf(rec,sizeof(rec),"%s => ERROR: %s", p, err);
            history_add(hist, rec);
        }
        free(norm);
    }
    free(line);
}

static void view_history(const History *hist){
    if(hist->count==0){ print_box("HISTORY","No entries yet."); press_enter_to_continue(); return; }
    printf("\n-- Calculation History --\n");
    for(size_t i=0;i<hist->count;i++) printf("%3zu: %s\n", i+1, hist->items[i]);
    press_enter_to_continue();
}

int main(void){
    // Ensure UTF-8 locale so box characters render correctly on most terminals
    setlocale(LC_ALL, "");

    History hist; history_init(&hist); history_load(&hist);

    for(;;){
        print_menu();
        printf("Select option (0-2): ");
        char buf[32]; if(!fgets(buf,sizeof(buf),stdin)) break;
        int choice = -1; if(sscanf(buf, "%d", &choice)!=1) choice=-1;
        switch(choice){
            case 0: start_calculator(&hist); break;
            case 1: view_history(&hist); break;
            case 2: history_save(&hist); history_free(&hist); puts("Goodbye."); return 0;
            default:
                print_box("INPUT ERROR","Invalid selection. Use 0, 1, or 2.");
                break;
        }
    }

    history_save(&hist); history_free(&hist);
    return 0;
}
